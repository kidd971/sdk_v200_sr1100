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

/* PRIVATE FUNCTIONS **********************************************************/
static void led_all_off(void);

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
    static bool btn1_active;
    static bool btn2_active;
    static bool btn3_active;
    static bool btn4_active;

    if (btn1_active) {
        if (!quasar_button_read_state(QUASAR_BUTTON_USER_1)) {
            btn1_active = false;
        }
    }
    if (btn2_active) {
        if (!quasar_button_read_state(QUASAR_BUTTON_USER_2)) {
            btn2_active = false;
        }
    }
    if (btn3_active) {
        if (!quasar_button_read_state(QUASAR_BUTTON_USER_3)) {
            btn3_active = false;
        }
    }
    if (btn4_active) {
        if (!quasar_button_read_state(QUASAR_BUTTON_USER_4)) {
            btn4_active = false;
        }
    }
    if (!btn1_active && !btn2_active && !btn3_active && !btn4_active) {
        if (quasar_button_read_state(QUASAR_BUTTON_USER_1)) {
            if (button1_callback != NULL) {
                button1_callback();
            }
            btn1_active = true;
        } else if (quasar_button_read_state(QUASAR_BUTTON_USER_2)) {
            if (button2_callback != NULL) {
                button2_callback();
            }
            btn2_active = true;
        } else if (quasar_button_read_state(QUASAR_BUTTON_USER_3)) {
            if (button3_callback != NULL) {
                button3_callback();
            }
            btn3_active = true;
        } else if (quasar_button_read_state(QUASAR_BUTTON_USER_4)) {
            if (button4_callback != NULL) {
                button4_callback();
            }
            btn4_active = true;
        }
    }
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
