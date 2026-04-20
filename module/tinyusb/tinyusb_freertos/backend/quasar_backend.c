/** @file  quasar_backend.c
 *  @brief Implement TinyUSB module facade prototype functions.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar.h"
#include "quasar_it.h"
#include "tinyusb_freertos_facade.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void facade_tinyusb_usb_peripheral_init(void)
{
    quasar_bsp_status_t quasar_err = QUASAR_OK;

    quasar_usb_init(&quasar_err);
    ASSERT_QUASAR_BSP_STATUS(quasar_err);

    quasar_usb_enable_irq();
}

void facade_tinyusb_set_usb_irq_callback(void (*irq_callback)(void))
{
    quasar_it_set_usb_irq_callback(irq_callback);
}
