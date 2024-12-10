/** @file  tinyusb_module_baremetal_facade.h
 *  @brief Declaration of baremetal TinyUSB function prototype facade.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef TINYUSB_MODULE_BAREMETAL_FACADE_H_
#define TINYUSB_MODULE_BAREMETAL_FACADE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize hardware USB peripheral.
 */
void tusb_hal_usb_peripheral_init(void);

/** @brief This function sets the function callback for the USB IRQ.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void tusb_hal_set_usb_irq_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the USB timer IRQ.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void tusb_hal_set_timer_callback(void (*irq_callback)(void));

/** @brief Initialize USB timer peripheral.
 */
void tusb_hal_timer_init(void);

/** @brief Start USB timer peripheral.
 */
void tusb_hal_timer_start(void);

#ifdef __cplusplus
}
#endif

#endif /* TINYUSB_MODULE_BAREMETAL_FACADE_H_ */
