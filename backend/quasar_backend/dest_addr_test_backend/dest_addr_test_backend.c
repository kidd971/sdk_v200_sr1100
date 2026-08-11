/** @file  dest_addr_test_backend.c
 *  @brief Implement dest_addr_test facade prototype functions.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "dest_addr_test_facade.h"
#include "quasar.h"
#include "tinyusb_baremetal.h"

/* CONSTANTS ******************************************************************/
#define IRQ_PRIORITY_TMER_PACKET_GENERATION QUASAR_IRQ_PRIORITY_8
#define TIMER_SELECTION_PACKET_GENERATION   QUASAR_TIMER_SELECTION_TIMER6

/* TYPES **********************************************************************/
/** @brief Structure tracking a button's state.
 */
typedef struct button_handle {
    /*! Button identifier. */
    quasar_button_selection_t button_id;
    /*! Button active state. */
    bool active;
} button_handle_t;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void handle_button_state(button_handle_t *button_handle, void (*button_callback)(void));

/* PUBLIC FUNCTIONS ***********************************************************/
void facade_packet_generation_set_timer_callback(void (*irq_callback)(void))
{
    quasar_it_set_timer6_callback(irq_callback);
}

void facade_packet_generation_timer_init(uint32_t timeslot)
{
    quasar_timer_config_t timer_config = {
        .timer_selection = TIMER_SELECTION_PACKET_GENERATION,
        .time_base = QUASAR_TIMER_TIME_BASE_MICROSECOND,
        .time_period = timeslot / 2,
        .irq_priority = IRQ_PRIORITY_TMER_PACKET_GENERATION,
    };
    quasar_timer_init(&timer_config);
}

void facade_button_handling(void (*button1_callback)(void), void (*button2_callback)(void),
                            void (*button3_callback)(void), void (*button4_callback)(void))
{
    static button_handle_t btn1_handle = {QUASAR_BUTTON_USER_1, false};
    static button_handle_t btn2_handle = {QUASAR_BUTTON_USER_2, false};
    static button_handle_t btn3_handle = {QUASAR_BUTTON_USER_3, false};
    static button_handle_t btn4_handle = {QUASAR_BUTTON_USER_4, false};

    handle_button_state(&btn1_handle, button1_callback);
    handle_button_state(&btn2_handle, button2_callback);
    handle_button_state(&btn3_handle, button3_callback);
    handle_button_state(&btn4_handle, button4_callback);
}

void facade_packet_generation_timer_start(void)
{
    quasar_timer_start(TIMER_SELECTION_PACKET_GENERATION);
}

void facade_packet_generation_timer_stop(void)
{
    quasar_timer_stop(TIMER_SELECTION_PACKET_GENERATION);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Manages the state of a button, detecting presses and triggering a callback.
 *
 *  @param[in] button_handle    Pointer to the button state structure.
 *  @param[in] button_callback  Function to call when a press is detected.
 */
static void handle_button_state(button_handle_t *button_handle, void (*button_callback)(void))
{
    if (!button_handle->active) {
        if (quasar_button_read_state(button_handle->button_id)) {
            button_handle->active = true;
            if (button_callback != NULL) {
                button_callback();
            }
        }
    } else {
        if (!quasar_button_read_state(button_handle->button_id)) {
            button_handle->active = false;
        }
    }
}
