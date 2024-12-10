/** @file  tinyusb_module_baremetal.h
 *  @brief TinyUSB module with baremetal task setup functionality.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef TINYUSB_MODULE_BAREMETAL_H_
#define TINYUSB_MODULE_BAREMETAL_H_

/* INCLUDES *******************************************************************/
#include "tusb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize and set up the TinyUSB module.
 *
 * This function is responsible for the initial setup of the baremetal TinyUSB module.
 * It initializes the USB peripheral and configures a periodic timer interrupt.
 */
void tinyusb_module_baremetal_setup(void);


#ifdef __cplusplus
}
#endif

#endif /* TINYUSB_MODULE_BAREMETAL_H_ */
