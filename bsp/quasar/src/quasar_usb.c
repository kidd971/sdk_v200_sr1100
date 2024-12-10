/** @file  quasar_usb.c
 *  @brief This module configures the USB peripheral.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_usb.h"
#include "stm32u5xx_hal_dma.h"

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void init_usb_detect_gpio(void);

/* PRIVATE GLOBALS ************************************************************/
PCD_HandleTypeDef hpcd_USB_OTG_HS;

/* PUBLIC FUNCTIONS ***********************************************************/
void quasar_usb_init(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();

    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /* Initializes the peripherals clock. */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USBPHY;
    PeriphClkInit.UsbPhyClockSelection = RCC_USBPHYCLKSOURCE_HSE;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        while (1);
    }

    /* Set the OTG PHY reference clock selection. */
    HAL_SYSCFG_SetOTGPHYReferenceClockSelection(SYSCFG_OTG_HS_PHY_CLK_SELECT_1);

    /* Peripheral clock enable/ */
    __HAL_RCC_USB_OTG_HS_CLK_ENABLE();
    __HAL_RCC_USBPHYC_CLK_ENABLE();

    /* Enable VDDUSB */
    if (__HAL_RCC_PWR_IS_CLK_DISABLED()) {
        __HAL_RCC_PWR_CLK_ENABLE();
        HAL_PWREx_EnableVddUSB();

        /* Configure VOSR register of USB. */
        HAL_PWREx_EnableUSBHSTranceiverSupply();
        __HAL_RCC_PWR_CLK_DISABLE();
    } else {
        HAL_PWREx_EnableVddUSB();

        /* Configure VOSR register of USB. */
        HAL_PWREx_EnableUSBHSTranceiverSupply();
    }

    /* Configuring the SYSCFG registers OTG_HS PHY. */
    /* OTG_HS PHY enable. */
    HAL_SYSCFG_EnableOTGPHY(SYSCFG_OTG_HS_PHY_ENABLE);
    /* USB_OTG_HS interrupt Init. */
    HAL_NVIC_SetPriority(OTG_HS_IRQn, QUASAR_DEF_PRIO_OTG_HS_IRQ, 0);
    HAL_NVIC_EnableIRQ(OTG_HS_IRQn);

    hpcd_USB_OTG_HS.Instance = USB_OTG_HS;
    hpcd_USB_OTG_HS.Init.dev_endpoints = 9;
    hpcd_USB_OTG_HS.Init.speed = PCD_SPEED_HIGH;
    hpcd_USB_OTG_HS.Init.phy_itface = USB_OTG_HS_EMBEDDED_PHY;
    hpcd_USB_OTG_HS.Init.Sof_enable = DISABLE;
    hpcd_USB_OTG_HS.Init.low_power_enable = DISABLE;
    hpcd_USB_OTG_HS.Init.lpm_enable = DISABLE;
    hpcd_USB_OTG_HS.Init.use_dedicated_ep1 = DISABLE;
    hpcd_USB_OTG_HS.Init.vbus_sensing_enable = DISABLE;
    hpcd_USB_OTG_HS.Init.dma_enable = DISABLE;
    if (HAL_PCD_Init(&hpcd_USB_OTG_HS) != HAL_OK) {
        while (1);
    }

    /* Initialize USB detection GPIO in interrupt mode */
    init_usb_detect_gpio();
}

void quasar_usb_deinit(void)
{
    __HAL_RCC_USB_OTG_HS_CLK_DISABLE();
    __HAL_RCC_USBPHYC_CLK_DISABLE();

    /* USB_OTG_HS interrupt DeInit */
    HAL_NVIC_DisableIRQ(OTG_HS_IRQn);

    quasar_gpio_deinit(QUASAR_DEF_USB_5V_IN_DETECT_PORT, QUASAR_DEF_USB_5V_IN_DETECT_PIN);
}

void quasar_usb_disable_irq(void)
{
    NVIC_DisableIRQ(OTG_HS_IRQn);
}

void quasar_usb_enable_irq(void)
{
    NVIC_EnableIRQ(OTG_HS_IRQn);
}

bool quasar_is_usb_detected(void)
{
    if (quasar_gpio_read_state(QUASAR_DEF_USB_5V_IN_DETECT_PORT, QUASAR_DEF_USB_5V_IN_DETECT_PIN)) {
        return false;
    } else {
        return true;
    }
}

void quasar_usb_connection_event_callback(void (*irq_callback)(void))
{
    quasar_it_set_falling_edge_exti15_irq_callback(irq_callback);
}

void quasar_usb_disconnection_event_callback(void (*irq_callback)(void))
{
    quasar_it_set_rising_edge_exti15_irq_callback(irq_callback);
}

void quasar_usb_detection_enable_irq_it(void)
{
    quasar_gpio_enable_irq(QUASAR_DEF_USB_5V_IN_DETECT_PIN);
}

void quasar_usb_detection_disable_irq_it(void)
{
    quasar_gpio_disable_irq(QUASAR_DEF_USB_5V_IN_DETECT_PIN);
}

/* PRIVATE FUNCTIONS **********************************************************/
 /** @brief Initialize the GPIO to detect a USB connection.
  */
static void init_usb_detect_gpio(void)
{
    quasar_gpio_config_t usb_detect_config = {
        .port         = QUASAR_DEF_USB_5V_IN_DETECT_PORT,
        .pin          = QUASAR_DEF_USB_5V_IN_DETECT_PIN,
        .mode         = QUASAR_GPIO_MODE_INPUT,
        .type         = QUASAR_GPIO_TYPE_OD,
        .pull         = QUASAR_GPIO_PULL_NONE,
        .speed        = QUASAR_GPIO_SPEED_MEDIUM,
        .alternate    = QUASAR_GPIO_ALTERNATE_NONE,
    };

    quasar_gpio_init(usb_detect_config);
    quasar_gpio_configure_rising_and_falling_edges_irq(usb_detect_config.port, usb_detect_config.pin,
                                                       QUASAR_DEF_PRIO_USB_DETECT_IRQ);
}
