/** @file  quasar_backend.c
 *  @brief Implement hello-world-rtos facade prototype functions.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "hello_world_rtos_facade.h"
#include "quasar.h"
#include "tinyusb_module.h"

/* CONSTANTS ******************************************************************/
#define DELAY_MS_LONG_PERIOD 250
#define LED_BLINK_REPEAT     2

/* TYPES **********************************************************************/
/** @brief Structure tracking a button's state.
 */
typedef struct button_handle {
    quasar_button_selection_t button_id;
    bool active;
} button_handle_t;

/* PRIVATE FUNCTIONS **********************************************************/
static void led_all_off(void);
static void handle_button_state(button_handle_t *button_handle, void (*button_callback)(void));

/* PUBLIC FUNCTIONS ***********************************************************/
void facade_board_init(void)
{
    quasar_config_t quasar_cfg = {
        .clk_freq = QUASAR_CLK_160MHZ,
        .debug_enabled = false,
        .radio1_enabled = true,
        .adc_enabled = false,
        .quasar_vdd_selection = QUASAR_VDD_SELECTION_3V3,
    };

    if (SWC_RADIO_COUNT == 1) {
        quasar_cfg.radio2_enabled = false;
    } else {
        quasar_cfg.radio2_enabled = true;
    }

    quasar_init(quasar_cfg);

    tinyusb_module_task_setup();
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

void facade_tx_conn_status(void)
{
    quasar_led_toggle(QUASAR_LED_USER_1);
}

void facade_rx_conn_status(void)
{
    quasar_led_toggle(QUASAR_LED_USER_2);
}

void facade_delay(uint32_t ms_delay)
{
    quasar_timer_delay_ms(ms_delay);
}

void facade_print_string(char *string)
{
    if (tud_cdc_connected()) {
        tud_cdc_write_str(string);
        tud_cdc_write_flush();
    }
}

void facade_notify_enter_pairing(void)
{
    uint16_t delay_ms = DELAY_MS_LONG_PERIOD;
    uint8_t repeat = LED_BLINK_REPEAT;

    quasar_rgb_clear();
    quasar_rgb_configure_color(QUASAR_RGB_COLOR_BLUE);

    for (uint8_t i = 0; i < repeat; i++) {
        quasar_timer_delay_ms(delay_ms);
        quasar_rgb_set();
        quasar_timer_delay_ms(delay_ms);
        quasar_rgb_clear();
    }
}

void facade_notify_not_paired(void)
{
    uint16_t delay_ms = DELAY_MS_LONG_PERIOD;
    uint8_t repeat = LED_BLINK_REPEAT;

    led_all_off();
    quasar_rgb_clear();
    quasar_rgb_configure_color(QUASAR_RGB_COLOR_RED);

    for (uint8_t i = 0; i < repeat; i++) {
        quasar_timer_delay_ms(delay_ms);
        quasar_rgb_set();
        quasar_timer_delay_ms(delay_ms);
        quasar_rgb_clear();
    }
}

void facade_notify_pairing_successful(void)
{
    quasar_rgb_configure_color(QUASAR_RGB_COLOR_MAGENTA);
    quasar_rgb_set();
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Turn off all LEDs.
 */
static void led_all_off(void)
{
    quasar_led_clear(QUASAR_LED_USER_1);
    quasar_led_clear(QUASAR_LED_USER_2);
    quasar_led_clear(QUASAR_LED_USER_3);
    quasar_led_clear(QUASAR_LED_USER_4);
}

/** @brief Manages the state of a button, detecting presses and triggering a callback.
 *
 *  @param button_handle    Pointer to the button state structure.
 *  @param button_callback  Function to call when a press is detected.
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
