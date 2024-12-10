/** @file  evk_power.c
 *  @brief TThis module controls the power features of the SPARK EVK board.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "evk_clock.h"
#include "evk_power.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void evk_power_vdd_select_gpio_init(evk_vdd_t vdd)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Configure GPIO pin Output Level */
    evk_power_set_board_voltage(vdd);

    /* Configure GPIO pins */
    GPIO_InitStruct.Pin   = VOLTAGE_SEL_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(VOLTAGE_SEL_PORT, &GPIO_InitStruct);
}

void evk_power_set_board_voltage(evk_vdd_t vdd)
{
    if (vdd == VDD_1V8) {
        HAL_GPIO_WritePin(VOLTAGE_SEL_PORT, VOLTAGE_SEL_PIN, GPIO_PIN_RESET);
    } else if (vdd == VDD_3V3) {
        HAL_GPIO_WritePin(VOLTAGE_SEL_PORT, VOLTAGE_SEL_PIN, GPIO_PIN_SET);
    }
}

void evk_power_shutdown_and_wake_up(uint32_t ms)
{
    evk_clock_rtc_for_wakeup_init();
    evk_clock_start_rtc_for_wakeup(ms);
    HAL_PWREx_EnterSHUTDOWNMode();
}
