/** @file  common_backend.c
 *  @brief Implement common_backend facade prototype functions that are used across all application examples.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include <stdio.h>
#include "common_facade.h"
#include "quasar.h"
#if (RTOS_ENABLED == 1)
#include "tinyusb_freertos.h"
#else
#include "tinyusb_baremetal.h"
#endif

/* CONSTANTS ******************************************************************/
#define ERROR_MESSAGE_BUFFER_SIZE 50

/* PUBLIC FUNCTIONS ***********************************************************/
__attribute__((weak)) void facade_context_switch_trigger(void)
{
    quasar_radio_callback_context_switch();
}

__attribute__((weak)) void facade_set_context_switch_handler(void (*callback)(void))
{
    quasar_it_set_pendsv_callback(callback);
}

__attribute__((weak)) void facade_board_init(void)
{
    quasar_bsp_status_t quasar_err = QUASAR_OK;

    quasar_config_t quasar_cfg = {
        .clk_freq = QUASAR_CLK_160MHZ,
        .debug_enabled = false,
        .radio1_enabled = true,
        .radio2_enabled = (SWC_RADIO_COUNT > 1),
        .adc_enabled = false,
        .quasar_vdd_selection = QUASAR_VDD_SELECTION_1V8,
    };

    quasar_init(quasar_cfg, &quasar_err);
    ASSERT_QUASAR_BSP_STATUS(quasar_err);

#if (RTOS_ENABLED == 1)
    tinyusb_freertos_task_setup();
#else
    tinyusb_baremetal_setup();
#endif
}

__attribute__((weak)) void facade_tx_conn_status(void)
{
    quasar_led_toggle(QUASAR_LED_USER_1);
}

__attribute__((weak)) void facade_rx_conn_status(void)
{
    quasar_led_toggle(QUASAR_LED_USER_2);
}

__attribute__((weak)) void facade_delay(uint32_t ms_delay)
{
    quasar_timer_delay_ms(ms_delay);
}

__attribute__((weak)) void facade_print_string(char *string)
{
    if (tud_cdc_connected()) {
        tud_cdc_write_str(string);
        tud_cdc_write_flush();
    }
}

__attribute__((weak)) void facade_print_error_string(char *string)
{
    /* Turn OFF LEDs to show nothing is happening. */
    facade_led_all_off();
    /* Set RGB to RED to indicate an error happened. */
    quasar_rgb_configure_color(QUASAR_RGB_COLOR_RED);
    quasar_rgb_set();

    /* Set to highest priority. */
#if defined(STM32U535xx)
    HAL_NVIC_SetPriority(USB_IRQn, 0, 0);
#else
    HAL_NVIC_SetPriority(OTG_HS_IRQn, 0, 0);
#endif
    facade_print_string(string);
}

__attribute__((weak)) void facade_notify_enter_pairing(void)
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
#ifdef QUASAR_U535_91EVB
    /* 91EVB: restore always-on power indicator after pairing animation. */
    quasar_rgb_set();
#endif
}

__attribute__((weak)) void facade_notify_not_paired(void)
{
#ifdef QUASAR_U535_91EVB
    /* 91EVB: LED is always-on power indicator. Flash off 4 times to signal failure. */
    quasar_rgb_configure_color(QUASAR_RGB_COLOR_BLUE);
    for (uint8_t i = 0; i < 4; i++) {
        quasar_rgb_clear();
        quasar_timer_delay_ms(250);
        quasar_rgb_set();
        quasar_timer_delay_ms(250);
    }
    quasar_rgb_clear();
#else
    quasar_rgb_clear();
    quasar_rgb_configure_color(QUASAR_RGB_COLOR_BLUE);
    quasar_rgb_set();
    quasar_timer_delay_ms(250);
    quasar_rgb_clear();
#endif
}

__attribute__((weak)) void facade_notify_pairing_successful(void)
{
#ifdef QUASAR_U535_91EVB
    /* 91EVB: LED is always-on power indicator. Flash off once to signal success. */
    quasar_rgb_configure_color(QUASAR_RGB_COLOR_BLUE);
    quasar_rgb_clear();
    quasar_timer_delay_ms(250);
    quasar_rgb_set();
#else
    quasar_rgb_configure_color(QUASAR_RGB_COLOR_BLUE);
    quasar_rgb_set();
#endif
}

__attribute__((weak)) void facade_led_all_off(void)
{
    quasar_led_clear(QUASAR_LED_USER_1);
    quasar_led_clear(QUASAR_LED_USER_2);
    quasar_led_clear(QUASAR_LED_USER_3);
    quasar_led_clear(QUASAR_LED_USER_4);
}

__attribute__((weak)) uint32_t facade_get_tick_ms(void)
{
    return quasar_timer_free_running_ms_get_tick_count();
}

__attribute__((weak)) void facade_wait_for_interrupt(void)
{
    __WFI();
}

void quasar_bsp_error_handler(quasar_bsp_status_t quasar_err)
{
    char buffer[ERROR_MESSAGE_BUFFER_SIZE];

    snprintf(buffer, ERROR_MESSAGE_BUFFER_SIZE, "Quasar Error! Code: %d\n\r", quasar_err);
    facade_print_error_string(buffer);

    quasar_rgb_configure_color(QUASAR_RGB_COLOR_BLUE);

    while (1) {
        quasar_rgb_set();
        HAL_Delay(500);
        quasar_rgb_clear();
        HAL_Delay(500);
    }
}
