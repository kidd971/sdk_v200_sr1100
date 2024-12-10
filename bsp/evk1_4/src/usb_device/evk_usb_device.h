/** @file  evk_usb_device.h
 *  @brief This module controls USB features of the SPARK EVK board.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef EVK_USB_DEVICE_H_
#define EVK_USB_DEVICE_H_

/* INCLUDES *******************************************************************/
#include "evk_def.h"
#include "usbd_cdc_if.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Set the callback function for data received via communication device class.
 *
 *  @param[in] usb_cdc_receive_cb  Pointer to the callback function.
 */
void evk_usb_device_cdc_init_rx_cb(int8_t (*usb_cdc_receive_cb)(uint8_t *buf, uint32_t *len));

/** @brief Set the callback function for data transmitted via communication device class.
 *
 *  @param[in] usb_cdc_receive_cb  Pointer to the callback function.
 */
void evk_usb_device_cdc_init_tx_cb(int8_t (*usb_cdc_transmit_cb)(uint8_t *buf, uint32_t *len));

/** @brief Set the callback function for usb initialization complete.
 *
 *  @param[in]  usb_cdc_init_cb  Pointer to the callback function.
 */
void evk_usb_device_cdc_init_init_cb(int8_t (*usb_cdc_init_cb)(void));

/** @brief Initialize the USB in communication device class mode.
 *
 *  @return true if successful, false otherwise.
 */
bool evk_usb_device_cdc_init(void);

/** @brief De-initialize the USB from communication device class mode.
 *
 *  @return true if successful, false otherwise.
 */
bool evk_usb_device_deinit(void);

/** @brief Init the peripheral but only if there is a USB detected
 *
 *  @return true if successful, false otherwise.
 */
bool evk_usb_device_connect(void);

/** @brief De-init the peripheral but only if there is no USB detected
 *
 *  @return true if successful, false otherwise.
 */
bool evk_usb_device_disconnect(void);

/** @brief Send character on USB.
 *
 *  @param[out] c    Character to write.
 */
void evk_usb_device_cdc_putc(uint8_t c);

/** @brief Send buffer on USB.
 *
 *  @param[out] buf      Buffer to write.
 *  @param[out] buf_len  Buffer length.
 */
void evk_usb_device_cdc_send_buf(uint8_t *buf, uint16_t buf_len);

/** @brief Receive buffer on USB.
 *
 *  @param[in] buf      Buffer to read.
 *  @param[in] buf_len  Buffer length.
 */
void evk_usb_device_cdc_receive_buf(uint8_t *buf, uint16_t buf_len);

#ifdef __cplusplus
}
#endif

#endif /* EVK_USB_DEVICE_H_ */

