/** @file  quasar_backend.c
 *  @brief Implement audio unidrectional facade prototype functions.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "audio_bidirectional_facade.h"
#include "max98091.h"
#include "quasar.h"
#include "tinyusb_module_baremetal.h"

/* CONSTANTS ******************************************************************/
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

/* PRIVATE GLOBALS ************************************************************/
static max98091_i2c_hal_t codec_hal = {
    .i2c_addr = MAX98091A_I2C_ADDR,
    .read = quasar_audio_i2c_read_byte_blocking,
    .write = quasar_audio_i2c_write_byte_blocking,
};

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void led1_blink(uint8_t blink_count);

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

    /* Initialize the Codec's I2C interface. */
    quasar_audio_init_i2c();

    tinyusb_module_baremetal_setup();
}

void facade_audio_coord_init(void)
{
    quasar_sai_config_t sai_config = {
        /* SAI communicating from the codec to the CPU; the Coordinator sends stereo. */
        .rx_sai_mono_stereo = QUASAR_SAI_MODE_STEREO,
        /* SAI communicating from the CPU to the codec; the Coordinator receives mono. */
        .tx_sai_mono_stereo = QUASAR_SAI_MODE_MONO,
        .sai_bit_depth = QUASAR_SAI_BIT_DEPTH_24BITS,
    };

    /* Reset codec before initializing the SAI. */
    max98091_reset_codec(&codec_hal);
    quasar_timer_delay_ms(1);

    quasar_audio_init(sai_config);

    max98091_codec_cfg_t cfg = {
        .sampling_rate = MAX98091_AUDIO_48KHZ,
        .word_size = MAX98091_AUDIO_24BITS,
        .record_enabled = true,
        .playback_enabled = true,
        .record_filter_enabled = false,
        .playback_filter_enabled = false,
    };
    max98091_init(&codec_hal, &cfg);
}

void facade_audio_node_init(void)
{
    quasar_sai_config_t sai_config = {
        /* SAI communicating from the codec to the CPU; the Node sends mono. */
        .rx_sai_mono_stereo = QUASAR_SAI_MODE_MONO,
        /* SAI communicating from the CPU to the codec; the Node receives stereo. */
        .tx_sai_mono_stereo = QUASAR_SAI_MODE_STEREO,
        .sai_bit_depth = QUASAR_SAI_BIT_DEPTH_24BITS,
    };

    /* Reset codec before initializing the SAI. */
    max98091_reset_codec(&codec_hal);
    quasar_timer_delay_ms(1);

    quasar_audio_init(sai_config);

    max98091_codec_cfg_t cfg = {
        .sampling_rate = MAX98091_AUDIO_48KHZ,
        .word_size = MAX98091_AUDIO_24BITS,
        .record_enabled = true,
        .playback_enabled = true,
        .record_filter_enabled = false,
        .playback_filter_enabled = false,
    };
    max98091_init(&codec_hal, &cfg);
}

void facade_audio_deinit(void)
{
    quasar_audio_deinit();
    max98091_reset_codec(&codec_hal);
}

void facade_set_sai_complete_callback(void (*tx_callback)(void), void (*rx_callback)(void))
{
    quasar_audio_set_sai_tx_dma_cplt_callback((irq_callback)tx_callback);
    quasar_audio_set_sai_rx_dma_cplt_callback((irq_callback)rx_callback);
}

facade_certification_mode_t facade_get_certification_mode(void)
{
    /* If button 2 is pressed at board startup, the application enters in a certification selection mode. */
    if (quasar_button_read_state(QUASAR_BUTTON_USER_2)) {
        led1_blink(LED_BLINK_CERTIFICATION_MODE_1);
        quasar_timer_delay_ms(USER_RESPONSE_DELAY_MS);

        if (quasar_button_read_state(QUASAR_BUTTON_USER_2)) {
            led1_blink(LED_BLINK_CERTIFICATION_MODE_2);
            quasar_timer_delay_ms(USER_RESPONSE_DELAY_MS);

            if (quasar_button_read_state(QUASAR_BUTTON_USER_2)) {
                led1_blink(LED_BLINK_CERTIFICATION_MODE_3);
                /* Entering in certification mode 3. */
                return FACADE_CERTIF_DATA;

            } else {
                /* Entering in certification mode 2. */
                return FACADE_CERTIF_AUDIO_COMPRESSED;
            }
        } else {
            /* Entering in certification mode 1. */
            return FACADE_CERTIF_AUDIO_UNCOMPRESSED;
        }
    } else {
        /* If button 2 is not pressed, the application runs normally without entering any certification mode. */
        return FACADE_CERTIF_NONE;
    }
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

void facade_fallback_status(bool on)
{
    if (on) {
        quasar_led_set(QUASAR_LED_USER_3);
    } else {
        quasar_led_clear(QUASAR_LED_USER_3);
    }
}

void facade_audio_process_main_channel_timer_init(uint32_t period_us)
{
    quasar_timer_config_t timer_config = {
        .timer_selection = TIMER_SELECTION_MAIN_CHANNEL_AUDIO_PROCESS,
        .time_base = QUASAR_TIMER_TIME_BASE_MICROSECOND,
        .time_period = period_us,
        .irq_priority = IRQ_PRIORITY_TIMER_MAIN_CHANNEL_AUDIO_PROCESS,
    };
    quasar_timer_init(&timer_config);
}

void facade_audio_process_back_channel_timer_init(uint32_t period_us)
{
    quasar_timer_config_t timer_config = {
        .timer_selection = TIMER_SELECTION_BACK_CHANNEL_AUDIO_PROCESS,
        .time_base = QUASAR_TIMER_TIME_BASE_MICROSECOND,
        .time_period = period_us,
        .irq_priority = IRQ_PRIORITY_TIMER_BACK_CHANNEL_AUDIO_PROCESS,
    };
    quasar_timer_init(&timer_config);
}

void facade_audio_process_main_channel_set_timer_callback(void (*callback)(void))
{
    quasar_it_set_timer16_callback(callback);
}

void facade_audio_process_back_channel_set_timer_callback(void (*callback)(void))
{
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

void facade_print_string(char *string)
{
    if (tud_cdc_connected()) {
        tud_cdc_write_str(string);
        tud_cdc_write_flush();
    }
}

void facade_empty_payload_received_status(void)
{
    quasar_led_clear(QUASAR_LED_USER_4);
}

void facade_payload_received_status(void)
{
    quasar_led_set(QUASAR_LED_USER_4);
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

void facade_led_all_off(void)
{
    quasar_led_clear(QUASAR_LED_USER_1);
    quasar_led_clear(QUASAR_LED_USER_2);
    quasar_led_clear(QUASAR_LED_USER_3);
    quasar_led_clear(QUASAR_LED_USER_4);
}

uint32_t facade_get_tick_ms(void)
{
    return quasar_timer_free_running_ms_get_tick_count();
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Blinks the LED 1 a specified number of times.
 *
 * @param blink_count The number of times to blink the LED.
 */
static void led1_blink(uint8_t blink_count)
{
    quasar_led_clear(QUASAR_LED_USER_1);
    for (int i = 0; i < blink_count * LED_BLINK_REPEAT; i++) {
        quasar_led_toggle(QUASAR_LED_USER_1);
        quasar_timer_delay_ms(DELAY_MS_LONG_PERIOD);
    }
}
