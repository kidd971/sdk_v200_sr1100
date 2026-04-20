/** @file  quasar_backend.c
 *  @brief Implement hello-world-rtos facade prototype functions.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "hello_world_rtos_facade.h"
#include "quasar.h"
#include "tinyusb_freertos.h"

/* CONSTANTS ******************************************************************/
#define DELAY_MS_LONG_PERIOD           250
#define LED_BLINK_REPEAT               2

#define DELAY_MS_LONG_PERIOD           250
#define DELAY_MS_SHORT_PERIOD          100
#define LED_BLINK_REPEAT               2

#define USER_RESPONSE_DELAY_MS         1000
#define LED_BLINK_CERTIFICATION_MODE_1 1

/* TYPES **********************************************************************/
/** @brief Structure tracking a button's state.
 */
typedef struct button_handle {
    quasar_button_selection_t button_id;
    bool active;
} button_handle_t;

/* PRIVATE GLOBALS ************************************************************/
static void (*button_app_callback)(void);

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void led1_blink(uint8_t blink_count);
static void handle_button_state(button_handle_t *button_handle, void (*button_callback)(void));
static void button_isr(void);
static facade_button_callbacks_t local_button_callbacks;

/* PUBLIC FUNCTIONS ***********************************************************/
void facade_button_init(void)
{
    /* Configure buttons IRQs. */
    quasar_gpio_configure_rising_and_falling_edges_irq(QUASAR_DEF_BUTTON_USER_1_PORT, QUASAR_DEF_BUTTON_USER_1_PIN,
                                                       QUASAR_IRQ_PRIORITY_15);
    quasar_gpio_enable_irq(QUASAR_DEF_BUTTON_USER_1_PIN);
    quasar_gpio_configure_rising_and_falling_edges_irq(QUASAR_DEF_BUTTON_USER_2_PORT, QUASAR_DEF_BUTTON_USER_2_PIN,
                                                       QUASAR_IRQ_PRIORITY_15);
    quasar_gpio_enable_irq(QUASAR_DEF_BUTTON_USER_2_PIN);
    quasar_gpio_configure_rising_and_falling_edges_irq(QUASAR_DEF_BUTTON_USER_3_PORT, QUASAR_DEF_BUTTON_USER_3_PIN,
                                                       QUASAR_IRQ_PRIORITY_15);
    quasar_gpio_enable_irq(QUASAR_DEF_BUTTON_USER_3_PIN);
    quasar_gpio_configure_rising_and_falling_edges_irq(QUASAR_DEF_BUTTON_USER_4_PORT, QUASAR_DEF_BUTTON_USER_4_PIN,
                                                       QUASAR_IRQ_PRIORITY_15);
    quasar_gpio_enable_irq(QUASAR_DEF_BUTTON_USER_4_PIN);

    /* Configure buttons ISRs. */
    quasar_button_set_button1_callback(button_isr);
    quasar_button_set_button2_callback(button_isr);
    quasar_button_set_button3_callback(button_isr);
    quasar_button_set_button4_callback(button_isr);
}

facade_certification_mode_t facade_get_certification_mode(void)
{
    if (!quasar_button_read_state(QUASAR_BUTTON_USER_2)) {
        /* If button 2 is not pressed, the application runs normally without entering any certification mode. */
        return FACADE_CERTIF_NONE;
    }

    /* If button 2 is pressed at board startup, the application enters in a certification selection mode. */
    led1_blink(LED_BLINK_CERTIFICATION_MODE_1);
    quasar_timer_delay_ms(USER_RESPONSE_DELAY_MS);

    return FACADE_CERTIF_HELLO_WORLD_RTOS;
}

void facade_set_button_callbacks(facade_button_callbacks_t button_callbacks)
{
    local_button_callbacks = button_callbacks;
}

void facade_button_handling(void)
{
    static button_handle_t btn1_handle = {QUASAR_BUTTON_USER_1, false};
    static button_handle_t btn2_handle = {QUASAR_BUTTON_USER_2, false};

    handle_button_state(&btn1_handle, local_button_callbacks.pairing_callback);
    handle_button_state(&btn2_handle, local_button_callbacks.reset_stats_callback);
}

void facade_set_button_event_callback(void (*button_event_callback)(void))
{
    button_app_callback = button_event_callback;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Button interrupt service routine.
 */
static void button_isr(void)
{
    if (button_app_callback != NULL) {
        button_app_callback();
    }
}

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
