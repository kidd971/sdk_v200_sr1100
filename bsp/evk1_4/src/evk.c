/** @file  evk.c
 *  @brief Board Support Package for the SPARK EVK board.
 *
 *  @copyright Copyright (C) 2020-2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "evk.h"

/* PUBLIC FUNCTION ************************************************************/
void evk_init(evk_cfg_t *evk_cfg)
{
    HAL_Init();

    evk_clock_all_gpio_init();
    /* UARTs init */
    /* Initialize MPU as soon as possible. */
    evk_exp_lpuart_init();
    evk_mpu_enable();

    /* Set default system and USB clock frequency */
    evk_set_system_clock(evk_cfg->freq);
    evk_exp_lpuart_update_config();

    /* Button initialization */
    evk_buttons_init();

    /* LEDs init */
    evk_led_init();

    /* Debug pins init */
    evk_dbg_debug_pin_init();

    /* Power Management */
    evk_power_vdd_select_gpio_init(evk_cfg->vdd);

    /* USB detect init */
    evk_usb_init();

    /* Radio init */
    evk_radio_peripherals_init(evk_cfg->pendsv_prio);
}

void evk_system_reset(void)
{
    NVIC_SystemReset();
}

void evk_jump_to_bootloader(void)
{
    uint32_t i = 0;
    void (*SysMemBootJump)(void);

    /* Set the address of the entry point to bootloader. */
    volatile uint32_t BootAddr = 0x1FFF0000;

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
