/** @file  quasar_backend.c
 *  @brief Implement TinyUSB module facade prototype functions.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_it.h"
#include "quasar.h"
#include "tinyusb_module_baremetal_facade.h"

/* CONSTANTS ******************************************************************/
#define IRQ_PRIORITY_TMER_USB_TASK QUASAR_IRQ_PRIORITY_10
#define TIMER_SELECTION_USB_TASK   QUASAR_TIMER_SELECTION_TIMER1

/* PUBLIC FUNCTIONS ***********************************************************/
void tusb_hal_usb_peripheral_init(void)
{
    quasar_usb_init();
    quasar_usb_enable_irq();
}

void tusb_hal_set_usb_irq_callback(void (*irq_callback)(void))
{
    quasar_it_set_usb_irq_callback(irq_callback);
}

void tusb_hal_set_timer_callback(void (*irq_callback)(void))
{
    quasar_it_set_timer1_callback(irq_callback);
}

void tusb_hal_timer_init(void)
{
    quasar_timer_config_t timer_config = {
        .timer_selection = TIMER_SELECTION_USB_TASK,
        .time_base = QUASAR_TIMER_TIME_BASE_MICROSECOND,
        .time_period = 500,
        .irq_priority = IRQ_PRIORITY_TMER_USB_TASK
    };
    quasar_timer_init(&timer_config);
}

void tusb_hal_timer_start(void)
{
    quasar_timer_start(TIMER_SELECTION_USB_TASK);
}
