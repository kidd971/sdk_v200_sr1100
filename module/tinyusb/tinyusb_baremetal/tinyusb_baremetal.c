/** @file  tinyusb_baremetal.c
 *  @brief This file contains TinyUSB module baremetal task setup functionality.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "tinyusb_baremetal.h"
#include "tinyusb_baremetal_facade.h"

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void usb_irq_callback(void);

/* PUBLIC FUNCTIONS ***********************************************************/
void tinyusb_baremetal_setup(void)
{
    /* Initialize USB peripheral. */
    facade_tinyusb_usb_peripheral_init();

    /* Assign USB hardware IRQ to tinyUSB callback function. */
    facade_tinyusb_set_usb_irq_callback(usb_irq_callback);

    /* Initialize the TinyUSB device. */
    tusb_rhport_init_t dev_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO,
    };
    tusb_init(BOARD_TUD_RHPORT, &dev_init);

    facade_tinyusb_timer_init();

    facade_tinyusb_set_timer_callback(tud_task);

    facade_tinyusb_timer_start();
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Function handler for the USB Interrupt.
 */
static void usb_irq_callback(void)
{
    tusb_int_handler(BOARD_TUD_RHPORT, true);
    /* Trigger tinyusb task processing. */
    facade_tinyusb_timer_trigger();
}
