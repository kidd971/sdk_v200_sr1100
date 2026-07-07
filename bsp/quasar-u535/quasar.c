/** @file  quasar.c
 *  @brief Board Support Package for the SPARK hardware development board.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar.h"
#include "quasar_adc.h"
#include "quasar_debug.h"
#include "quasar_def.h"
#include "quasar_error.h"
#include "quasar_gpio.h"
#include "quasar_it.h"
#include "quasar_power.h"
#include "quasar_timer_ext.h"

/* PRIVATE GLOBALS ************************************************************/
static uint8_t board_revision;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void patch_board_revision(quasar_revision_t board_revision);

/* PUBLIC FUNCTIONS ************************************************************/
void quasar_init(quasar_config_t quasar_config, quasar_bsp_status_t *err)
{
    *err = QUASAR_OK;

    quasar_clock_init(quasar_config.clk_freq, err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);

    quasar_power_up(err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);

    quasar_gpio_clock_enable();

    quasar_power_init_gpios();

    // quasar_power_set_vdd_level(quasar_config.quasar_vdd_selection);
    // quasar_power_enable_ldo_led();
    /* U535: fixed config, no board-revision detection. Enable the MCU analog/USB 3V3 LDO
     * on PB15 (the "REVB" enable pin). PD4 (the "REVA" enable pin) is Radio2 SPI2 MOSI on
     * U535 and is freed in patch_board_revision() before radio init. */
    quasar_power_enable_ldo_mcu(QUASAR_REVB, err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);

    HAL_Delay(100);

    /* Initialize the debug port's UART (for the STLink USB to UART bridge) and debug IOs. */
    if (quasar_config.debug_enabled) {
        quasar_debug_init();
    }

    /* Initialize ADC (battery monitoring). U535 has no board-revision divider, so the
     * revision is fixed rather than read; radio2 MOSI is PD4 for both revs regardless. */
    quasar_adc_init(err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);
    board_revision = QUASAR_REVB;

    /* Free the "REVA" LDO-enable GPIO (PD4), reused as Radio2 MOSI, before radio init. */
    patch_board_revision(board_revision);

    /* Initialize LEDs and buttons. */
    quasar_led_init();
    quasar_rgb_init();
    quasar_button_init();

    if (!quasar_config.adc_enabled) {
        /* Deinitialize the ADC peripheral to save power. */
        quasar_adc_deinit(err);
        QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);
    }

    HAL_ICACHE_Enable();

    /* Initialize radio 1 peripherals. */
    if (quasar_config.radio1_enabled) {
#if RADIO_QSPI_ENABLED
        quasar_radio_1_init_unused_spi_gpios();
#endif
        quasar_radio_1_init(err);
        QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);
    }

    /* Initialize radio 2 peripherals. */
    if (quasar_config.radio2_enabled) {
#if RADIO_QSPI_ENABLED
        quasar_radio_2_init_unused_spi_gpios(board_revision);
#endif
        /* The radio 2 MOSI pin differs depending on the board revision. */
        quasar_radio_2_init(board_revision, err);
        QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);
    }

// #if !RADIO_QSPI_ENABLED
//     quasar_radio_init_unused_qspi_gpios();
// #endif

    /* Initialize the free running timer tick at 1 ms. */
    quasar_timer_free_running_ms_init(FREE_RUNNING_TIMER_PRIORITY);

    /* Initialize USB detect GPIOs. */
    quasar_usb_init_usb_detect_gpio();
}

void quasar_system_reset(void)
{
    NVIC_SystemReset();
}

void quasar_jump_to_bootloader(void)
{
    uint32_t i = 0;
    void (*SysMemBootJump)(void);

    /* Set the address of the entry point to bootloader. */
    volatile uint32_t BootAddr = 0x0BF90000;

    /* Disable all interrupts. */
    __disable_irq();

    /* Disable Systick timer. */
    SysTick->CTRL = 0;

    /* Set the clock to the default state. */
    HAL_RCC_DeInit();

    /* Clear Interrupt Enable Register & Interrupt Pending Register. */
    for (i = 0; i < 5; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;
        NVIC->ICPR[i] = 0xFFFFFFFF;
    }

    /* Re-enable all interrupts. */
    __enable_irq();

    /* Set up the jump to (bootloader address + 4). */
    SysMemBootJump = (void (*)(void))(*((uint32_t *)((BootAddr + 4))));

    /* Set the main stack pointer to the bootloader stack. */
    __set_MSP(*(uint32_t *)BootAddr);

    /* Call the function to jump to bootloader location. */
    SysMemBootJump();
}

quasar_revision_t quasar_get_board_revision(void)
{
    return board_revision;
}

/* MISCELLANEOUS FUNCTIONS **********************************************************/
/** @brief U535 fixed-config power patch (no board-revision detection).
 *
 *  Deinitializes the "REVA" LDO-enable GPIO (PD4), which on U535 is reused as Radio2 SPI2
 *  MOSI, so it is free before radio init. Leaves the "REVB" LDO-enable GPIO (PB15) driving
 *  the MCU analog/USB 3V3 LDO.
 *
 *  @note Must be called before radio initialization because the pin freed here (PD4) is
 *        used for Radio2 SPI MOSI.
 *
 *  @param[in] board_revision  Fixed to QUASAR_REVB on U535 (no detection).
 */
static void patch_board_revision(quasar_revision_t board_revision)
{
    /* REVB path: keep PB15 (MCU LDO), deinit PD4 (Radio2 MOSI). */
    switch (board_revision) {
    case QUASAR_REVA:
        quasar_gpio_deinit(QUASAR_DEF_LDO_MCU_EN_PORT_REVB, QUASAR_DEF_LDO_MCU_EN_PIN_REVB);
        break;
    case QUASAR_REVB:
        quasar_gpio_deinit(QUASAR_DEF_LDO_MCU_EN_PORT_REVA, QUASAR_DEF_LDO_MCU_EN_PIN_REVA);
        break;
    default:
        /* Unsupported revision. */
        Error_Handler();
    }
}
