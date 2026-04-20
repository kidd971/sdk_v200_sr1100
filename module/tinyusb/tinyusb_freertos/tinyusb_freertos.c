/** @file  tinyusb_freertos.c
 *  @brief This file contains TinyUSB module task setup functionality.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "tinyusb_freertos.h"
#include "cmsis_os2.h"
#include "tinyusb_freertos_facade.h"

/* PRIVATE VARIABLES **********************************************************/
static const osThreadAttr_t tinyusb_thread_attr = {
    .name = "tinyusb_thread",
    .stack_size = 4096,
    .priority = osPriorityBelowNormal7,
};

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void usb_irq_callback(void);
static void tinyusb_thread(void *argument);

/* PUBLIC FUNCTIONS ***********************************************************/
void tinyusb_freertos_task_setup(void)
{
    /* Create TinyUSB Thread for continuously processing its tasks within an infinite loop. */
    osThreadNew(tinyusb_thread, NULL, &tinyusb_thread_attr);

    /* Initializing board USB peripheral. */
    facade_tinyusb_usb_peripheral_init();

    /* Assign USB hardware IRQ to tinyUSB callback function. */
    facade_tinyusb_set_usb_irq_callback(usb_irq_callback);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initializes TinyUSB and enters a loop to handle its tasks.
 *
 * @param argument Unused.
 */
static void tinyusb_thread(void *argument)
{
    (void)argument;

    /* Initialize the TinyUSB device. */
    tusb_rhport_init_t dev_init = {
        .role = TUSB_ROLE_DEVICE,
        .speed = TUSB_SPEED_AUTO,
    };
    tusb_init(BOARD_TUD_RHPORT, &dev_init);

    /* RTOS forever loop. */
    while (1) {
        /* tud_task will suspend the thread until an event is generated from the tusb_int_handler. */
        tud_task();

        /* following code only run if tud_task() process at least 1 event */
        tud_cdc_write_flush();
    }
}

/** @brief Function handler for the USB Interrupt.
 */
static void usb_irq_callback(void)
{
    tusb_int_handler(BOARD_TUD_RHPORT, true);
}
