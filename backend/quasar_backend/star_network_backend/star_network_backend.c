/** @file  quasar_backend.c
 *  @brief Implement star-network facade prototype functions.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar.h"
#include "star_network_facade.h"
#include "tinyusb_module_baremetal.h"

/* CONSTANTS ******************************************************************/
#define PRINTF_BUF_SIZE_BYTE 64

/* PRIVATE FUNCTIONS **********************************************************/
static void led_all_off(void);

/* PUBLIC FUNCTIONS ***********************************************************/
void facade_context_switch_trigger(void)
{
    quasar_radio_callback_context_switch();
}

void facade_set_context_switch_handler(void (*callback)(void))
{
    quasar_it_set_pendsv_callback(callback);
}

void facade_board_init(void)
{
    quasar_config_t quasar_cfg = {
        .clk_freq = QUASAR_CLK_160MHZ,
        .debug_enabled = false,
        .radio1_enabled = true,
        .radio2_enabled = false,
        .adc_enabled = false,
        .quasar_vdd_selection = QUASAR_VDD_SELECTION_3V3,
    };
    quasar_init(quasar_cfg);

    tinyusb_module_baremetal_setup();
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

void facade_delay(uint32_t ms_delay)
{
    quasar_timer_delay_ms(ms_delay);
}

bool facade_read_button_status(star_network_btn_t button)
{
    return quasar_button_read_state((quasar_button_selection_t)button);
}

void facade_usb_printf(const char *fmt, ...)
{
    char std_buf[PRINTF_BUF_SIZE_BYTE];
    va_list va;

    va_start(va, fmt);

    vsprintf(std_buf, fmt, va);

    if (tud_cdc_connected()) {
        tud_cdc_write_str(std_buf);
        tud_cdc_write_flush();
    }

    va_end(va);
}

void facade_payload_sent_status(void)
{
    quasar_led_set(QUASAR_LED_USER_1);
}

void facade_empty_payload_sent_status(void)
{
    quasar_led_clear(QUASAR_LED_USER_1);
}

void facade_payload_received_status(void)
{
    quasar_led_set(QUASAR_LED_USER_2);
}

void facade_empty_payload_received_status(void)
{
    quasar_led_clear(QUASAR_LED_USER_2);
}

void facade_notify_enter_pairing(void)
{
    uint16_t delay_ms = 250;
    uint8_t repeat = 2;

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
    uint16_t delay_ms = 250;
    uint8_t repeat = 2;

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
