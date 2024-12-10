/** @file  evk_db.c
 *  @brief This module controls debug GPIO of SPARK EVK board.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "evk_dbg.h"

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void init_debug_pin_gpio_push_pull(GPIO_TypeDef_t *port, uint16_t pin, GPIO_PinState state);

/* PUBLIC FUNCTIONS ***********************************************************/
void evk_dbg_debug_pin_init(void)
{
    init_debug_pin_gpio_push_pull(EXP_PC7_PORT,  EXP_PC7_PIN,  GPIO_PIN_RESET);
    init_debug_pin_gpio_push_pull(EXP_PC8_PORT,  EXP_PC8_PIN,  GPIO_PIN_RESET);
    init_debug_pin_gpio_push_pull(EXP_PC10_PORT, EXP_PC10_PIN, GPIO_PIN_RESET);
    init_debug_pin_gpio_push_pull(EXP_PC11_PORT, EXP_PC11_PIN, GPIO_PIN_RESET);
    init_debug_pin_gpio_push_pull(EXP_PA3_PORT, EXP_PA3_PIN, GPIO_PIN_RESET);
}

void evk_dbg_on(evk_dbg_t gpio)
{
    switch (gpio) {
    case DBG0:
        HAL_GPIO_WritePin(EXP_PC8_PORT, EXP_PC8_PIN, GPIO_PIN_SET);
        break;
    case DBG1:
        HAL_GPIO_WritePin(EXP_PC7_PORT, EXP_PC7_PIN, GPIO_PIN_SET);
        break;
    case DBG2:
        HAL_GPIO_WritePin(EXP_PA3_PORT, EXP_PA3_PIN, GPIO_PIN_SET);
        break;
    case DBG3:
        HAL_GPIO_WritePin(EXP_PC11_PORT, EXP_PC11_PIN, GPIO_PIN_SET);
        break;
    default:
        break;
    }
}

void evk_dbg_off(evk_dbg_t gpio)
{
    switch (gpio) {
    case DBG0:
        HAL_GPIO_WritePin(EXP_PC8_PORT, EXP_PC8_PIN, GPIO_PIN_RESET);
        break;
    case DBG1:
        HAL_GPIO_WritePin(EXP_PC7_PORT, EXP_PC7_PIN, GPIO_PIN_RESET);
        break;
    case DBG2:
        HAL_GPIO_WritePin(EXP_PA3_PORT, EXP_PA3_PIN, GPIO_PIN_RESET);
        break;
    case DBG3:
        HAL_GPIO_WritePin(EXP_PC11_PORT, EXP_PC11_PIN, GPIO_PIN_RESET);
        break;
    default:
        break;
    }
}

void evk_dbg_toggle(evk_dbg_t gpio)
{
    switch (gpio) {
    case DBG0:
        HAL_GPIO_TogglePin(EXP_PC8_PORT, EXP_PC8_PIN);
        break;
    case DBG1:
        HAL_GPIO_TogglePin(EXP_PC7_PORT, EXP_PC7_PIN);
        break;
    case DBG2:
        HAL_GPIO_TogglePin(EXP_PA3_PORT, EXP_PA3_PIN);
        break;
    case DBG3:
        HAL_GPIO_TogglePin(EXP_PC11_PORT, EXP_PC11_PIN);
        break;
    default:
        break;
    }
}

void evk_dbg_pulse(evk_dbg_t gpio)
{
    switch (gpio) {
    case DBG0:
        HAL_GPIO_WritePin(EXP_PC8_PORT, EXP_PC8_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(EXP_PC8_PORT, EXP_PC8_PIN, GPIO_PIN_RESET);
        break;
    case DBG1:
        HAL_GPIO_WritePin(EXP_PC7_PORT, EXP_PC7_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(EXP_PC7_PORT, EXP_PC7_PIN, GPIO_PIN_RESET);
        break;
    case DBG2:
        HAL_GPIO_WritePin(EXP_PA3_PORT, EXP_PA3_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(EXP_PA3_PORT, EXP_PA3_PIN, GPIO_PIN_RESET);
        break;
    case DBG3:
        HAL_GPIO_WritePin(EXP_PC11_PORT, EXP_PC11_PIN, GPIO_PIN_SET);
        HAL_GPIO_WritePin(EXP_PC11_PORT, EXP_PC11_PIN, GPIO_PIN_RESET);
        break;
    default:
        break;
    }
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize the GPIOs to use debug pins in push pull mode.
 *
 *  @param port   GPIO Port.
 *  @param pin    GPIO Pin.
 *  @param state  Pin state initialization.
 */
static void init_debug_pin_gpio_push_pull(GPIO_TypeDef_t *port, uint16_t pin, GPIO_PinState state)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(port, pin, state);

    /* Configure GPIO pins */
    GPIO_InitStruct.Pin   = pin;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}
