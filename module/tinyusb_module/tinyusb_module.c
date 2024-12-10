/** @file  tinyusb_module.c
 *  @brief This file contains TinyUSB module task setup functionality.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "tinyusb_module.h"
#include "cmsis_os2.h"
#include "tinyusb_module_facade.h"

/* PRIVATE VARIABLES **********************************************************/
static const osThreadAttr_t tinyusb_thread_attr = {
    .name = "tinyusb_thread",
    .stack_size = 4096,
    .priority = osPriorityBelowNormal7,
};

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void board_usb_irq_cb(void);
static void tinyusb_thread(void *argument);

/* PUBLIC FUNCTIONS ***********************************************************/
void tinyusb_module_task_setup(void)
{
    /* Create TinyUSB Thread for continuously processing its tasks within an infinite loop. */
    osThreadNew(tinyusb_thread, NULL, &tinyusb_thread_attr);

    /* Initializing board USB peripheral. */
    tusb_hal_usb_peripheral_init();

    /* Assign USB hardware IRQ to tinyUSB callback function. */
    tusb_hal_set_usb_irq_callback(board_usb_irq_cb);
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
    tud_init(BOARD_TUD_RHPORT);
    /* RTOS forever loop. */
    while (1) {
        tud_task();

        /* Small delay for RTOS yielding. */
        osDelay(1);
    }
}

/** @brief Function handler for the USB Interrupt.
 */
static void board_usb_irq_cb(void)
{
    tud_int_handler(BOARD_TUD_RHPORT);
}
