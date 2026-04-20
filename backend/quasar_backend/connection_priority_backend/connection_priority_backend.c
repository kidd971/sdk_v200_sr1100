/** @file  quasar_backend.c
 *  @brief Implement connection_priority facade prototype functions.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "connection_priority_facade.h"
#include "quasar.h"
#include "tinyusb_baremetal.h"

/* CONSTANTS ******************************************************************/
#define IRQ_PRIORITY_TIMER_PACKET_RATE_1 QUASAR_IRQ_PRIORITY_14
#define IRQ_PRIORITY_TIMER_PACKET_RATE_2 QUASAR_IRQ_PRIORITY_14
#define IRQ_PRIORITY_TIMER_STATS         QUASAR_IRQ_PRIORITY_15

#define TIMER_SELECTION_STATS            QUASAR_TIMER_SELECTION_TIMER16
#define TIMER_SELECTION_PAKET_RATE_1     QUASAR_TIMER_SELECTION_TIMER17
#define TIMER_SELECTION_PAKET_RATE_2     QUASAR_TIMER_SELECTION_TIMER3

#define DELAY_MS_LONG_PERIOD             250
#define LED_BLINK_REPEAT                 2

#define USER_RESPONSE_DELAY_MS           1000
#define LED_BLINK_CERTIFICATION_MODE_1   1
#define LED_BLINK_CERTIFICATION_MODE_2   2
#define LED_BLINK_CERTIFICATION_MODE_3   3

/* TYPES **********************************************************************/
/** @brief Structure tracking a button's state.
 */
typedef struct button_handle {
    quasar_button_selection_t button_id;
    bool active;
} button_handle_t;

/* PRIVATE GLOBALS ************************************************************/
static facade_button_callbacks_t local_button_callbacks;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void led1_blink(uint8_t blink_count);
static void handle_button_state(button_handle_t *button_handle, void (*button_callback)(void));

/* PUBLIC FUNCTIONS ***********************************************************/
facade_certification_mode_t facade_get_coord_certification_mode(void)
{
    if (!quasar_button_read_state(QUASAR_BUTTON_USER_2)) {
        /* If button 2 is not pressed, the application runs normally without entering any certification mode. */
        return FACADE_CERTIF_NONE;
    }

    /* If button 2 is pressed at board startup, the application enters in a certification selection mode. */
    led1_blink(LED_BLINK_CERTIFICATION_MODE_1);
    quasar_timer_delay_ms(USER_RESPONSE_DELAY_MS);

    if (!quasar_button_read_state(QUASAR_BUTTON_USER_2)) {
        /* Button held for less than 1 delay period.
         * -> Entering in connection ID 0 certification mode.
         */
        return FACADE_CERTIF_CONNECTION_ID_0;
    }

    led1_blink(LED_BLINK_CERTIFICATION_MODE_2);
    quasar_timer_delay_ms(USER_RESPONSE_DELAY_MS);

    if (!quasar_button_read_state(QUASAR_BUTTON_USER_2)) {
        /* Button held for less than 2 delay periods.
         * -> Entering in connection ID 1 certification mode.
         */
        return FACADE_CERTIF_CONNECTION_ID_1;
    }

    /* Button held for more than 2 delay periods.
     * -> Entering in connection ID 2 certification mode.
     */
    led1_blink(LED_BLINK_CERTIFICATION_MODE_3);
    return FACADE_CERTIF_CONNECTION_ID_2;
}

facade_certification_mode_t facade_get_node_certification_mode(void)
{
    if (!quasar_button_read_state(QUASAR_BUTTON_USER_2)) {
        /* If button 2 is not pressed, the application runs normally without entering any certification mode. */
        return FACADE_CERTIF_NONE;
    }

    /* If button 2 is pressed at board startup, the application enters in a certification selection mode. */
    led1_blink(LED_BLINK_CERTIFICATION_MODE_1);
    quasar_timer_delay_ms(USER_RESPONSE_DELAY_MS);

    if (!quasar_button_read_state(QUASAR_BUTTON_USER_2)) {
        /* Button held for less than 1 delay period.
         * -> Entering in connection ID 3 certification mode.
         */
        return FACADE_CERTIF_CONNECTION_ID_3;
    }

    /* Button held for more than 1 delay periods.
     * -> Entering in connection ID 4 certification mode.
     */
    led1_blink(LED_BLINK_CERTIFICATION_MODE_2);
    return FACADE_CERTIF_CONNECTION_ID_4;
}

void facade_set_button_callbacks(facade_button_callbacks_t button_callbacks)
{
    local_button_callbacks = button_callbacks;
}

void facade_button_handling(void)
{
    static button_handle_t btn1_handle = {QUASAR_BUTTON_USER_1, false};
    static button_handle_t btn2_handle = {QUASAR_BUTTON_USER_2, false};
    static button_handle_t btn3_handle = {QUASAR_BUTTON_USER_3, false};

    handle_button_state(&btn1_handle, local_button_callbacks.pairing_callback);
    handle_button_state(&btn2_handle, local_button_callbacks.reset_stats_callback);
    handle_button_state(&btn3_handle, local_button_callbacks.stop_cid0_toggle_callback);
}

void facade_packet_rate_timer1_init(uint32_t period_us)
{
    quasar_timer_config_t timer_config = {
        .timer_selection = TIMER_SELECTION_PAKET_RATE_1,
        .time_base = QUASAR_TIMER_TIME_BASE_MICROSECOND,
        .time_period = period_us,
        .irq_priority = IRQ_PRIORITY_TIMER_PACKET_RATE_1,
    };
    quasar_timer_init(&timer_config);
}

void facade_packet_rate_set_timer1_callback(void (*callback)(void))
{
    quasar_it_set_timer17_callback(callback);
}

void facade_packet_rate_timer1_start(void)
{
    quasar_timer_start(TIMER_SELECTION_PAKET_RATE_1);
}

void facade_packet_rate_timer1_stop(void)
{
    quasar_timer_stop(TIMER_SELECTION_PAKET_RATE_1);
}

void facade_packet_rate_timer2_init(uint32_t period_us)
{
    quasar_timer_config_t timer_config = {
        .timer_selection = TIMER_SELECTION_PAKET_RATE_2,
        .time_base = QUASAR_TIMER_TIME_BASE_MICROSECOND,
        .time_period = period_us,
        .irq_priority = IRQ_PRIORITY_TIMER_PACKET_RATE_2,
    };
    quasar_timer_init(&timer_config);
}

void facade_packet_rate_set_timer2_callback(void (*callback)(void))
{
    quasar_it_set_timer3_callback(callback);
}

void facade_packet_rate_timer2_start(void)
{
    quasar_timer_start(TIMER_SELECTION_PAKET_RATE_2);
}

void facade_packet_rate_timer2_stop(void)
{
    quasar_timer_stop(TIMER_SELECTION_PAKET_RATE_2);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Blinks the LED 1 a specified number of times.
 *
 *  @param[in] blink_count  The number of times to blink the LED.
 */
static void led1_blink(uint8_t blink_count)
{
    quasar_led_clear(QUASAR_LED_USER_1);
    for (int i = 0; i < blink_count * LED_BLINK_REPEAT; i++) {
        quasar_led_toggle(QUASAR_LED_USER_1);
        quasar_timer_delay_ms(DELAY_MS_LONG_PERIOD);
    }
}

/** @brief Manages the state of a button, detecting presses and triggering a callback.
 *
 *  @param[in] button_handle    Pointer to the button state structure.
 *  @param[in] button_callback  Function to call when a press is detected.
 */
static void handle_button_state(button_handle_t *button_handle, void (*button_callback)(void))
{
    if (!button_handle->active) {
        /* If the button is not active and is pressed, activate it and call the callback. */
        if (quasar_button_read_state(button_handle->button_id)) {
            /* The button is pressed, activate the button. */
            button_handle->active = true;
            if (button_callback != NULL) {
                /* Execute the callback. */
                button_callback();
            }
        }
    } else {
        /* If the button is active (pressed), do nothing for now, it remains pressed. */
        if (!quasar_button_read_state(button_handle->button_id)) {
            /* The button is released, desactivate the button. */
            button_handle->active = false;
        }
    }
}
