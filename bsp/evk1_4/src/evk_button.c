/** @file  evk_button.c
 *  @brief This module controls button feature of the SPARK EVK board.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "evk_button.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void evk_buttons_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Configure GPIO pin : PtPin */
    GPIO_InitStruct.Pin  = BTN0_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BTN0_PORT, &GPIO_InitStruct);

    /* Configure GPIO pin : PtPin */
    GPIO_InitStruct.Pin  = BTN1_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BTN1_PORT, &GPIO_InitStruct);
}

bool evk_read_btn_state(evk_btn_t btn)
{
    bool btn_state = false;

    switch (btn) {
    case BTN1:
        btn_state = HAL_GPIO_ReadPin(BTN0_PORT, BTN0_PIN);
        break;
    case BTN2:
        btn_state = HAL_GPIO_ReadPin(BTN1_PORT, BTN1_PIN);
        break;
    default:
        break;
    }
    return btn_state;
}
