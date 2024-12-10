/** @file  tinyusb_module_baremetal.c
 *  @brief This file contains TinyUSB module baremetal task setup functionality.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "tinyusb_module_baremetal.h"
#include "tinyusb_module_baremetal_facade.h"

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void board_usb_irq_cb(void);

/* PUBLIC FUNCTIONS ***********************************************************/
void tinyusb_module_baremetal_setup(void)
{
    /* Initialize USB peripheral. */
    tusb_hal_usb_peripheral_init();

    /* Assign USB hardware IRQ to tinyUSB callback function. */
    tusb_hal_set_usb_irq_callback(board_usb_irq_cb);

    /* Initialize the TinyUSB device. */
    tud_init(BOARD_TUD_RHPORT);

    tusb_hal_timer_init();

    tusb_hal_set_timer_callback(tud_task);

    tusb_hal_timer_start();
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Function handler for the USB Interrupt.
 */
static void board_usb_irq_cb(void)
{
    tud_int_handler(BOARD_TUD_RHPORT);
}
