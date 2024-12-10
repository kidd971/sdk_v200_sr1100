/** @file  evk_usb_device.c
 *  @brief This module controls USB features of the SPARK EVK board.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "evk_it.h"
#include "evk_usb.h"
#include "evk_clock.h"
#include "evk_usb_device.h"
#include "usbd_cdc_if.h"

/* EXTERNS ********************************************************************/
extern USBD_HandleTypeDef hUsbDeviceFS;
extern USBD_DescriptorsTypeDef CDC_Desc;
extern PCD_HandleTypeDef hpcd_USB_FS;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void usb_irq_callback(void);

/* PUBLIC FUNCTIONS ***********************************************************/
void evk_usb_device_cdc_init_rx_cb(int8_t (*usb_cdc_receive_cb)(uint8_t *buf, uint32_t *len))
{
    usbd_cdc_init_rx_cb(usb_cdc_receive_cb);
}

void evk_usb_device_cdc_init_tx_cb(int8_t (*usb_cdc_transmit_cb)(uint8_t *buf, uint32_t *len))
{
    usbd_cdc_init_tx_cb(usb_cdc_transmit_cb);
}

void evk_usb_device_cdc_init_init_cb(int8_t (*usb_cdc_init_cb)(void))
{
    usbd_cdc_init_init_cb(usb_cdc_init_cb);
}

bool evk_usb_device_cdc_init(void)
{
    USBD_StatusTypeDef err;

    evk_usb_clock_init();

    evk_set_usb_irq_callback(usb_irq_callback);

    err = USBD_Init(&hUsbDeviceFS, &CDC_Desc, DEVICE_FS);
    if (err != USBD_OK) {
        return false;
    }
    err = USBD_RegisterClass(&hUsbDeviceFS, &USBD_CDC);
    if (err != USBD_OK) {
        return false;
    }
    err = USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS);
    if (err != USBD_OK) {
        return false;
    }
    err = USBD_Start(&hUsbDeviceFS);
    if (err != USBD_OK) {
        return false;
    }

    return true;
}

bool evk_usb_device_deinit(void)
{
    USBD_StatusTypeDef err;

    err = USBD_DeInit(&hUsbDeviceFS);
    if (err != USBD_OK) {
        return false;
    }
    evk_usb_clock_deinit();

    return true;
}

bool evk_usb_device_connect(void)
{
    bool success = false;

    if (evk_is_usb_detected()) {
        success = evk_usb_device_cdc_init();
    }
    return success;
}

bool evk_usb_device_disconnect(void)
{
    bool success = false;

    if (!evk_is_usb_detected()) {
        success =  evk_usb_device_deinit();
    }
    return success;
}

void evk_usb_device_cdc_putc(uint8_t c)
{
    CDC_Transmit_FS(&c, 1);
}

void evk_usb_device_cdc_send_buf(uint8_t *buf, uint16_t buf_len)
{
    CDC_Transmit_FS(buf, buf_len);
}

void evk_usb_device_cdc_receive_buf(uint8_t *buf, uint16_t buf_len)
{
    CDC_Receive_FS(buf, buf_len);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief USB IRQ Callback.
 */
static void usb_irq_callback(void)
{
    HAL_PCD_IRQHandler(&hpcd_USB_FS);
}
