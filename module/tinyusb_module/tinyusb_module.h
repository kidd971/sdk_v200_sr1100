/** @file  tinyusb_module.h
 *  @brief TinyUSB module with task setup functionality.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef TINYUSB_MODULE_H_
#define TINYUSB_MODULE_H_

/* INCLUDES *******************************************************************/
#include "tusb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize and set up the TinyUSB module.
 *
 * This function is responsible for the initial setup of the TinyUSB module. It performs
 * three main tasks: initializing the USB peripheral, setting the USB interrupt callback,
 * and initializing the TinyUSB RTOS thread.
 */
void tinyusb_module_task_setup(void);

#ifdef __cplusplus
}
#endif

#endif /* TINYUSB_MODULE_H_ */
