/** @file  quasar.c
 *  @brief Board Support Package for the SPARK hardware development board.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar.h"
#include "quasar_adc.h"
#include "quasar_debug.h"
#include "quasar_def.h"
#include "quasar_gpio.h"
#include "quasar_it.h"
#include "quasar_power.h"
#include "quasar_timer_ext.h"

/* PRIVATE GLOBALS ************************************************************/
static uint8_t board_revision;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void patch_board_revision(quasar_revision_t board_revision);

/* PUBLIC FUNCTIONS ************************************************************/
void quasar_init(quasar_config_t quasar_config)
{
    quasar_clock_init(quasar_config.clk_freq);
    quasar_power_up();
    quasar_gpio_clock_enable();

    quasar_power_init_gpios();
    quasar_power_set_vdd_level(quasar_config.quasar_vdd_selection);
    quasar_power_enable_ldo_led();
    /* Both GPIOs are set to power up the ADC circuitry to allow getting the board revision. */
    quasar_power_enable_ldo_mcu(QUASAR_REVA);
    quasar_power_enable_ldo_mcu(QUASAR_REVB);
    HAL_Delay(100);

    /* Initialize ADC and get board revision. */
    board_revision = quasar_adc_init();

    /* Deinitialize the unnecessary GPIO based on the detected revision. */
    patch_board_revision(board_revision);

    if (!quasar_config.adc_enabled) {
        /* Deinitialize the ADC peripheral to save power. */
        quasar_adc_deinit();
    }

    HAL_ICACHE_Enable();

    /* Initialize radio 1 peripherals. */
    if (quasar_config.radio1_enabled) {
        quasar_radio_1_init();
    }

    /* Initialize radio 2 peripherals. */
    if (quasar_config.radio2_enabled) {
        /* The radio 2 MOSI pin differs depending on the board revision. */
        quasar_radio_2_init(board_revision);
    }

    /* TODO: Handle SPI/QSPI driver switch. */
    quasar_radio_init_unused_qspi_gpios();

    /* Initialize the debug port's UART (for the STLink USB to UART bridge) and debug IOs. */
    if (quasar_config.debug_enabled) {
        quasar_debug_init();
    }

    /* Initialize LEDs and buttons. */
    quasar_led_init();
    quasar_rgb_init();
    quasar_button_init();

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
/** @brief To power the ADC, the pins for both revisions are initialized. The pin is deinitialized depending on the
 *         revision.
 *
 *  @note This function must be called before the radio initialization because the pin deinitialized here is used for
 *        SPI on the radio.
 *
 *  @param[in] board_revision  The board revision.
 */
static void patch_board_revision(quasar_revision_t board_revision)
{
    /* The GPIO that had been initialized as output for LDO enable is deinitialized. */
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
