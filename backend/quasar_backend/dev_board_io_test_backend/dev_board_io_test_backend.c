/** @file  dev_board_io_test_backend.c
 *  @brief Implement dev_board_io_test facade prototype functions for the Quasar backend.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>

#include "dev_board_io_test_facade.h"
#include "quasar.h"
#include "tusb.h"

/* TYPES **********************************************************************/
/** @brief Handle for tracking a button's identity and active state.
 */
typedef struct button_handle {
    /*! Button identifier. */
    quasar_button_selection_t button_id;
    /*! Whether the button is currently active. */
    bool active;
} button_handle_t;

/** @brief State of the RGB LED channels.
 */
typedef struct rgb_state {
    /*! Red channel on. */
    bool red_on;
    /*! Green channel on. */
    bool green_on;
    /*! Blue channel on. */
    bool blue_on;
} rgb_state_t;

/* PRIVATE GLOBALS ************************************************************/
static facade_button_callbacks_t local_button_callbacks;
static rgb_state_t rgb_state;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void handle_button_state(button_handle_t *button_handle, void (*press_callback)(void),
                                void (*release_callback)(void));
static void apply_rgb_state(void);

/* PUBLIC FUNCTIONS ***********************************************************/
void facade_set_button_callbacks(facade_button_callbacks_t button_callbacks)
{
    local_button_callbacks = button_callbacks;
}

void facade_delay(uint32_t ms_delay)
{
    uint64_t start_tick = quasar_timer_free_running_ms_get_tick_count();

    while ((quasar_timer_free_running_ms_get_tick_count() - start_tick) < ms_delay) {}
}

void facade_button_handling(void)
{
    static button_handle_t btn1_handle = {QUASAR_BUTTON_USER_1, false};
    static button_handle_t btn2_handle = {QUASAR_BUTTON_USER_2, false};
    static button_handle_t btn3_handle = {QUASAR_BUTTON_USER_3, false};
    static button_handle_t btn4_handle = {QUASAR_BUTTON_USER_4, false};

    handle_button_state(&btn1_handle, local_button_callbacks.button1_press_callback,
                        local_button_callbacks.button1_release_callback);
    handle_button_state(&btn2_handle, local_button_callbacks.button2_press_callback,
                        local_button_callbacks.button2_release_callback);
    handle_button_state(&btn3_handle, local_button_callbacks.button3_press_callback,
                        local_button_callbacks.button3_release_callback);
    handle_button_state(&btn4_handle, local_button_callbacks.button4_press_callback,
                        local_button_callbacks.button4_release_callback);
}

void facade_dev_board_io_led_set(dev_board_io_led_id_t led, bool on)
{
    switch (led) {
    case DEV_BOARD_IO_LED_1:
        if (on) {
            quasar_led_set(QUASAR_LED_USER_1);
        } else {
            quasar_led_clear(QUASAR_LED_USER_1);
        }
        break;
    case DEV_BOARD_IO_LED_2:
        if (on) {
            quasar_led_set(QUASAR_LED_USER_2);
        } else {
            quasar_led_clear(QUASAR_LED_USER_2);
        }
        break;
    case DEV_BOARD_IO_LED_3:
        if (on) {
            quasar_led_set(QUASAR_LED_USER_3);
        } else {
            quasar_led_clear(QUASAR_LED_USER_3);
        }
        break;
    case DEV_BOARD_IO_LED_4:
        if (on) {
            quasar_led_set(QUASAR_LED_USER_4);
        } else {
            quasar_led_clear(QUASAR_LED_USER_4);
        }
        break;
    case DEV_BOARD_IO_LED_R:
        rgb_state.red_on = on;
        apply_rgb_state();
        break;
    case DEV_BOARD_IO_LED_G:
        rgb_state.green_on = on;
        apply_rgb_state();
        break;
    case DEV_BOARD_IO_LED_B:
        rgb_state.blue_on = on;
        apply_rgb_state();
        break;
    default:
        break;
    }
}

void facade_dev_board_io_led_all_off(void)
{
    quasar_led_clear(QUASAR_LED_USER_1);
    quasar_led_clear(QUASAR_LED_USER_2);
    quasar_led_clear(QUASAR_LED_USER_3);
    quasar_led_clear(QUASAR_LED_USER_4);
    quasar_rgb_clear();
    rgb_state.red_on = false;
    rgb_state.green_on = false;
    rgb_state.blue_on = false;
}

bool facade_dev_board_io_led_supported(dev_board_io_led_id_t led)
{
    switch (led) {
    case DEV_BOARD_IO_LED_1:
    case DEV_BOARD_IO_LED_2:
    case DEV_BOARD_IO_LED_3:
    case DEV_BOARD_IO_LED_4:
    case DEV_BOARD_IO_LED_R:
    case DEV_BOARD_IO_LED_G:
    case DEV_BOARD_IO_LED_B:
        return true;
    default:
        break;
    }

    return false;
}

bool facade_wait_for_go(uint32_t timeout_ms)
{
    uint8_t c;
    uint64_t start_tick = quasar_timer_free_running_ms_get_tick_count();

    while ((quasar_timer_free_running_ms_get_tick_count() - start_tick) < timeout_ms) {
        tud_task();
        if (tud_cdc_available() && tud_cdc_read(&c, 1) == 1) {
            if (c == 'G') {
                return true;
            }
        }
    }

    return false;
}

/* PRIVATE FUNCTIONS **********************************************************/
static void handle_button_state(button_handle_t *button_handle, void (*press_callback)(void),
                                void (*release_callback)(void))
{
    if (!button_handle->active) {
        if (!quasar_button_read_state(button_handle->button_id)) {
            return;
        }

        button_handle->active = true;
        if (press_callback != NULL) {
            press_callback();
        }
        return;
    }

    if (quasar_button_read_state(button_handle->button_id)) {
        return;
    }

    button_handle->active = false;
    if (release_callback != NULL) {
        release_callback();
    }
}

/** @brief Apply the current RGB state to the LED hardware.
 */
static void apply_rgb_state(void)
{
    uint8_t mask = 0;

    if (rgb_state.red_on) {
        mask |= 0x1;
    }
    if (rgb_state.green_on) {
        mask |= 0x2;
    }
    if (rgb_state.blue_on) {
        mask |= 0x4;
    }

    if (mask == 0) {
        quasar_rgb_clear();
        return;
    }

    switch (mask) {
    case 0x1:
        quasar_rgb_configure_color(QUASAR_RGB_COLOR_RED);
        break;
    case 0x2:
        quasar_rgb_configure_color(QUASAR_RGB_COLOR_GREEN);
        break;
    case 0x4:
        quasar_rgb_configure_color(QUASAR_RGB_COLOR_BLUE);
        break;
    case 0x3:
        quasar_rgb_configure_color(QUASAR_RGB_COLOR_YELLOW);
        break;
    case 0x5:
        quasar_rgb_configure_color(QUASAR_RGB_COLOR_MAGENTA);
        break;
    case 0x6:
        quasar_rgb_configure_color(QUASAR_RGB_COLOR_CYAN);
        break;
    case 0x7:
        quasar_rgb_configure_color(QUASAR_RGB_COLOR_WHITE);
        break;
    default:
        quasar_rgb_configure_color(QUASAR_RGB_COLOR_WHITE);
        break;
    }

    quasar_rgb_set();
}
