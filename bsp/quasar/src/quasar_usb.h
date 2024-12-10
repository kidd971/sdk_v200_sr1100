/** @file  quasar_usb.h
 *  @brief This module configures the USB peripheral.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_USB_H_
#define QUASAR_USB_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include "quasar_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize and configure the USB peripheral.
 */
void quasar_usb_init(void);

/** @brief De-initialize the USB peripheral.
 */
void quasar_usb_deinit(void);

/** @brief Disable USB interrupt.
 */
void quasar_usb_disable_irq(void);

/** @brief Enable USB interrupt.
 */
void quasar_usb_enable_irq(void);

/** @brief Check if a powered USB cable is connected to the board.
 *
 *  @retval True   USB is connected.
 *  @retval False  USB is not connected.
 */
bool quasar_is_usb_detected(void);

/** @brief Set usb detected interrupt callback.
 *
 *  @note The same EXTI is also used for button 3 press.
 *
 *  @param[in] irq_callback  Usb detected callback.
 */
void quasar_usb_connection_event_callback(void (*irq_callback)(void));

/** @brief Set usb not detected interrupt callback.
 *
 *  @note The same EXTI is also used for button 3 press.
 *
 *  @param[in] irq_callback  Usb not detected callback.
 */
void quasar_usb_disconnection_event_callback(void (*irq_callback)(void));

/** @brief Enable USB detection interrupt.
 */
void quasar_usb_detection_enable_irq_it(void);

/** @brief Disable USB detection interrupt.
 */
void quasar_usb_detection_disable_irq_it(void);

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_USB_H_ */
