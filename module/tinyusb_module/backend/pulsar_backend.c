/** @file  pulsar_backend.c
 *  @brief Implement TinyUSB module facade prototype functions.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "pulsar.h"
#include "tinyusb_module_facade.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void tusb_hal_usb_peripheral_init(void)
{
    /* Other USB related peripheral for the pulsar is initialized in the datacom main */
    pulsar_usb_init_clock();
}

void tusb_hal_set_usb_irq_callback(void (*irq_callback)(void))
{
    pulsar_it_set_usb_irq_callback(irq_callback);
}
