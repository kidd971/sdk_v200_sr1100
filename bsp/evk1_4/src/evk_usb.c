/** @file  evk_usb.c
 *  @brief This module controls USB features of the SPARK EVK board.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "evk_usb.h"
#include "evk_it.h"

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void init_usb_peripheral(void);
static void init_usb_detect_gpio(void);

/* PUBLIC FUNCTIONS ***********************************************************/
void evk_usb_init(void)
{
    init_usb_detect_gpio();
    evk_usb_clock_init();
    init_usb_peripheral();
}

void evk_usb_clock_init(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
    RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
    PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        Error_Handler();
    }
}

bool evk_is_usb_detected(void)
{
    if (HAL_GPIO_ReadPin(USB_DETECT_PORT, USB_DETECT_PIN) == GPIO_PIN_RESET) {
        return true;
    } else {
        return false;
    }
}

void evk_usb_enter_critical(void)
{
    NVIC_DisableIRQ(NVIC_USB_LP_IRQ);
}

void evk_usb_exit_critical(void)
{
    NVIC_EnableIRQ(NVIC_USB_LP_IRQ);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize usb peripheral to detect a USB connection.
 */
static void init_usb_peripheral(void)
{
    /* USB Device Interrupt */
    HAL_NVIC_SetPriority(NVIC_USB_LP_IRQ, PRIO_USB_LP_IRQ, 0);

    /* USB Device Clock */
    __HAL_RCC_USB_CLK_ENABLE();
}

/** @brief Initialize the GPIOs to detect a USB connection.
 */
static void init_usb_detect_gpio(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Configure GPIO pins */
    GPIO_InitStruct.Pin   = USB_DETECT_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_IT_RISING_FALLING;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(USB_DETECT_PORT, &GPIO_InitStruct);

    /* EXTI interrupt init*/
    HAL_NVIC_SetPriority(NVIC_USB_DET_IRQ, PRIO_USB_DET_IRQ, 0);
    HAL_NVIC_EnableIRQ(NVIC_USB_DET_IRQ);
}
