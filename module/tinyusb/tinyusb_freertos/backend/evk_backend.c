/** @file  evk_backend.c
 *  @brief Implement TinyUSB facade prototype functions.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "evk.h"
#include "tinyusb_freertos_facade.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void facade_tinyusb_usb_peripheral_init(void)
{
    /* USB related peripheral for the evk is initialized in the datacom main */
    return;
}

void facade_tinyusb_set_usb_irq_callback(void (*irq_callback)(void))
{
    evk_set_usb_irq_callback(irq_callback);
}
