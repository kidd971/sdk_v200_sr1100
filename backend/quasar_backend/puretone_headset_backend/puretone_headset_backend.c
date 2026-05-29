/** @file  puretone_headset_backend.c
 *  @brief Implement puretone headset facade prototype functions.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "puretone_headset_facade.h"
#include "at_cmd_core_facade.h"
#include "quasar.h"
#include "quasar_adc.h"
#include "sac_cfg.h"
#include <string.h>

/* CONSTANTS ******************************************************************/
#define EXPANSION_UART_TX_TIMEOUT_MS                  1000

#define IRQ_PRIORITY_TIMER_MAIN_CHANNEL_AUDIO_PROCESS QUASAR_IRQ_PRIORITY_13
#define IRQ_PRIORITY_TIMER_BACK_CHANNEL_AUDIO_PROCESS QUASAR_IRQ_PRIORITY_14
#define IRQ_PRIORITY_TIMER_DATA                       QUASAR_IRQ_PRIORITY_15

#define TIMER_SELECTION_DATA                          QUASAR_TIMER_SELECTION_TIMER15
#define TIMER_SELECTION_MAIN_CHANNEL_AUDIO_PROCESS    QUASAR_TIMER_SELECTION_TIMER16
#define TIMER_SELECTION_BACK_CHANNEL_AUDIO_PROCESS    QUASAR_TIMER_SELECTION_TIMER17

#define DELAY_MS_LONG_PERIOD                          250
#define LED_BLINK_REPEAT                              2

#define USER_RESPONSE_DELAY_MS                        1000
#define LED_BLINK_CERTIFICATION_MODE_1                1
#define LED_BLINK_CERTIFICATION_MODE_2                2
#define LED_BLINK_CERTIFICATION_MODE_3                3
#define LED_BLINK_CERTIFICATION_MODE_4                4
#define LED_BLINK_CERTIFICATION_MODE_5                5

/* TYPES **********************************************************************/
/** @brief Structure tracking a button's state.
 */
typedef struct button_handle {
    quasar_button_selection_t button_id;
    bool active;
} button_handle_t;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void led1_blink(uint8_t blink_count);
static void handle_button_state(button_handle_t *button_handle, void (*button_callback)(void));

/* Forward declaration from i2s backend */
void facade_i2s_backend_track_mux(quasar_i2s_mux_select_t select);

/* PRIVATE GLOBALS ************************************************************/
static facade_button_callbacks_t local_button_callbacks;

/* PUBLIC FUNCTIONS ***********************************************************/
facade_certification_mode_t facade_coord_get_certification_mode(void)
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
         * -> Entering in audio 96k 24 bit certification mode.
         */
        return FACADE_CERTIF_AUDIO_96k_24_BIT;
    }

    led1_blink(LED_BLINK_CERTIFICATION_MODE_2);
    quasar_timer_delay_ms(USER_RESPONSE_DELAY_MS);

    if (!quasar_button_read_state(QUASAR_BUTTON_USER_2)) {
        /* Button held for less than 2 delay periods.
         * -> Entering in audio 48k 24 bit certification mode.
         */
        return FACADE_CERTIF_AUDIO_48k_24_BIT;
    }

    led1_blink(LED_BLINK_CERTIFICATION_MODE_3);
    quasar_timer_delay_ms(USER_RESPONSE_DELAY_MS);

    if (!quasar_button_read_state(QUASAR_BUTTON_USER_2)) {
        /* Button held for less than 3 delay periods.
         * -> Entering in audio 48k 16 bit certification mode.
         */
        return FACADE_CERTIF_AUDIO_48k_16_BIT;
    }

    led1_blink(LED_BLINK_CERTIFICATION_MODE_4);
    quasar_timer_delay_ms(USER_RESPONSE_DELAY_MS);

    if (!quasar_button_read_state(QUASAR_BUTTON_USER_2)) {
        /* Button held for less than 4 delay periods.
         * -> Entering in audio 48k ADPCM certification mode.
         */
        return FACADE_CERTIF_AUDIO_48k_ADPCM;
    }

    /* Button held for more than 4 delay periods.
     * -> Entering in data certification mode.
     */
    led1_blink(LED_BLINK_CERTIFICATION_MODE_5);
    return FACADE_CERTIF_DATA;
}

facade_certification_mode_t facade_node_get_certification_mode(void)
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
         * -> Entering in audio 48k 16 bit certification mode.
         */
        return FACADE_CERTIF_AUDIO_48k_16_BIT;
    }

    led1_blink(LED_BLINK_CERTIFICATION_MODE_2);
    quasar_timer_delay_ms(USER_RESPONSE_DELAY_MS);

    if (!quasar_button_read_state(QUASAR_BUTTON_USER_2)) {
        /* Button held for less than 2 delay periods.
         * -> Entering in audio 48k ADPCM certification mode.
         */
        return FACADE_CERTIF_AUDIO_48k_ADPCM;
    }

    /* Button held for more than 2 delay periods.
     * -> Entering in data certification mode.
     */
    led1_blink(LED_BLINK_CERTIFICATION_MODE_3);
    return FACADE_CERTIF_DATA;
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
    static button_handle_t btn4_handle = {QUASAR_BUTTON_USER_4, false};

    handle_button_state(&btn1_handle, local_button_callbacks.pairing_callback);
    handle_button_state(&btn2_handle, local_button_callbacks.fallback_callback);
    handle_button_state(&btn3_handle, local_button_callbacks.volume_up_callback);
    handle_button_state(&btn4_handle, local_button_callbacks.volume_down_callback);
}

void facade_tx_audio_conn_status(void)
{
    quasar_led_toggle(QUASAR_LED_USER_1);
}

void facade_tx_data_conn_status(void)
{
}

void facade_rx_audio_conn_status(void)
{
    quasar_led_toggle(QUASAR_LED_USER_2);
}

void facade_rx_data_conn_status(void)
{
}

void facade_audio_process_main_channel_timer_init(void (*callback)(void))
{
    quasar_timer_config_t timer_config = {
        .timer_selection = TIMER_SELECTION_MAIN_CHANNEL_AUDIO_PROCESS,
        /* Initialize timer base value to 1 second. */
        .time_base = QUASAR_TIMER_TIME_BASE_MILLISECOND,
        .time_period = 1000,
        .irq_priority = IRQ_PRIORITY_TIMER_MAIN_CHANNEL_AUDIO_PROCESS,
    };
    quasar_timer_init(&timer_config);
    quasar_it_set_timer16_callback(callback);
}

void facade_audio_process_back_channel_timer_init(void (*callback)(void))
{
    quasar_timer_config_t timer_config = {
        .timer_selection = TIMER_SELECTION_BACK_CHANNEL_AUDIO_PROCESS,
        /* Initialize timer base value to 1 second. */
        .time_base = QUASAR_TIMER_TIME_BASE_MILLISECOND,
        .time_period = 1000,
        .irq_priority = IRQ_PRIORITY_TIMER_BACK_CHANNEL_AUDIO_PROCESS,
    };
    quasar_timer_init(&timer_config);
    quasar_it_set_timer17_callback(callback);
}

void facade_audio_process_main_channel_timer_start(void)
{
    quasar_timer_start(TIMER_SELECTION_MAIN_CHANNEL_AUDIO_PROCESS);
}

void facade_audio_process_back_channel_timer_start(void)
{
    quasar_timer_start(TIMER_SELECTION_BACK_CHANNEL_AUDIO_PROCESS);
}

void facade_audio_process_main_channel_timer_trigger(void)
{
    quasar_timer_generate_event(TIMER_SELECTION_MAIN_CHANNEL_AUDIO_PROCESS);
}

void facade_audio_process_back_channel_timer_trigger(void)
{
    quasar_timer_generate_event(TIMER_SELECTION_BACK_CHANNEL_AUDIO_PROCESS);
}

void facade_audio_process_main_channel_timer_stop(void)
{
    quasar_timer_stop(TIMER_SELECTION_MAIN_CHANNEL_AUDIO_PROCESS);
}

void facade_audio_process_back_channel_timer_stop(void)
{
    quasar_timer_stop(TIMER_SELECTION_BACK_CHANNEL_AUDIO_PROCESS);
}

void facade_data_timer_init(uint32_t period_ms)
{
    quasar_timer_config_t timer_config = {
        .timer_selection = TIMER_SELECTION_DATA,
        .time_base = QUASAR_TIMER_TIME_BASE_MILLISECOND,
        .time_period = period_ms,
        .irq_priority = IRQ_PRIORITY_TIMER_DATA,
    };
    quasar_timer_init(&timer_config);
}

void facade_data_timer_set_callback(void (*callback)(void))
{
    quasar_it_set_timer15_callback(callback);
}

void facade_data_timer_start(void)
{
    quasar_timer_start(TIMER_SELECTION_DATA);
}

void facade_data_timer_stop(void)
{
    quasar_timer_stop(TIMER_SELECTION_DATA);
}

void facade_empty_payload_received_status(void)
{
    quasar_led_clear(QUASAR_LED_USER_4);
}

void facade_payload_received_status(void)
{
    quasar_led_set(QUASAR_LED_USER_4);
}

bool facade_read_button_state(void)
{
    return quasar_button_read_state(QUASAR_BUTTON_USER_2);
}

void facade_notify_pairing_successful(void)
{
    quasar_rgb_configure_color(QUASAR_RGB_COLOR_GREEN);
    quasar_rgb_set();
}

void facade_expansion_uart_init(uint32_t baud_rate)
{
#if defined(QUASAR_DEF_UART_SELECTION_EXPANSION)
    quasar_gpio_config_t gpio_tx = {
        .port      = QUASAR_DEF_EXPANSION_UART_TX_PORT,
        .pin       = QUASAR_DEF_EXPANSION_UART_TX_PIN,
        .mode      = QUASAR_GPIO_MODE_ALTERNATE,
        .type      = QUASAR_GPIO_TYPE_PP,
        .pull      = QUASAR_GPIO_PULL_UP,
        .speed     = QUASAR_GPIO_SPEED_LOW,
        .alternate = QUASAR_GPIO_ALTERNATE_AF7,
    };
    quasar_gpio_config_t gpio_rx = {
        .port      = QUASAR_DEF_EXPANSION_UART_RX_PORT,
        .pin       = QUASAR_DEF_EXPANSION_UART_RX_PIN,
        .mode      = QUASAR_GPIO_MODE_ALTERNATE,
        .type      = QUASAR_GPIO_TYPE_OD,
        .pull      = QUASAR_GPIO_PULL_UP,
        .speed     = QUASAR_GPIO_SPEED_LOW,
        .alternate = QUASAR_GPIO_ALTERNATE_AF7,
    };
    quasar_uart_config_t uart_cfg = {
        .uart_selection = QUASAR_DEF_UART_SELECTION_EXPANSION,
        .baud_rate      = baud_rate,
        .parity         = QUASAR_UART_PARITY_NONE,
        .stop           = QUASAR_UART_STOP_BITS_1B,
        .irq_priority   = QUASAR_IRQ_PRIORITY_0,
        .gpio_config_tx = gpio_tx,
        .gpio_config_rx = gpio_rx,
    };
    quasar_uart_init(uart_cfg);
#else
    (void)baud_rate;  /* Expansion UART not available on this board variant. */
#endif
}

void facade_expansion_uart_write(char *string)
{
#if defined(QUASAR_DEF_UART_SELECTION_EXPANSION)
    quasar_bsp_status_t err;

    quasar_uart_transmit_blocking(QUASAR_DEF_UART_SELECTION_EXPANSION,
                                  (uint8_t *)string, strlen(string),
                                  EXPANSION_UART_TX_TIMEOUT_MS, &err);
#else
    (void)string;  /* Expansion UART not available on this board variant. */
#endif
}

uint8_t facade_expansion_uart_read_byte(void)
{
#if defined(QUASAR_DEF_UART_SELECTION_EXPANSION)
    return quasar_uart_receive_irq(QUASAR_DEF_UART_SELECTION_EXPANSION);
#else
    return 0;  /* Expansion UART not available on this board variant. */
#endif
}

void facade_system_reset(void)
{
    quasar_system_reset();
}

void facade_uwb_shutdown(void)
{
#if !defined(STM32U535xx)
    quasar_radio_1_set_shutdown_pin();
    quasar_radio_2_set_shutdown_pin();
#endif
}

void facade_set_i2s_mux(bool use_ext)
{
    quasar_i2s_mux_select_t select = use_ext ? QUASAR_SELECT_EXT_CODEC : QUASAR_SELECT_ON_BOARD_CODEC;

    quasar_audio_set_i2s_mux_selection(select);
    facade_i2s_backend_track_mux(select);
}

void facade_battery_init(void)
{
    quasar_bsp_status_t err;

    quasar_adc_init(&err);
}

uint8_t facade_read_battery_level_pct(void)
{
    quasar_bsp_status_t err;
    uint16_t mv;
    int32_t pct;

    mv = quasar_adc_get_battery_level_mv_polling(&err);
    if (err != QUASAR_OK) {
        return 0;
    }
    /* Li-ion: 3000mV=0%, 4200mV=100% */
    pct = ((int32_t)mv - 3000) * 100 / 1200;
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    return (uint8_t)pct;
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
