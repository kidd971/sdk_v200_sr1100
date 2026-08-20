/** @file  puretone_unidirectional_backend.c
 *  @brief Implement puretone unidirectional facade prototype functions.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "puretone_unidirectional_facade.h"
#include "quasar.h"
#include "quasar_it.h" /* dual-radio HW counters + HardFault snapshot (u535 & u5a5) */
#include "sac_cfg.h"
#include <string.h>

/* CONSTANTS ******************************************************************/
#define IRQ_PRIORITY_TIMER_AUDIO_PROCESS QUASAR_IRQ_PRIORITY_14
#define IRQ_PRIORITY_TIMER_DATA          QUASAR_IRQ_PRIORITY_15

#define TIMER_SELECTION_DATA             QUASAR_TIMER_SELECTION_TIMER16
#define TIMER_SELECTION_AUDIO_PROCESS    QUASAR_TIMER_SELECTION_TIMER17

#define DELAY_MS_LONG_PERIOD             250
#define LED_BLINK_REPEAT                 2

#define USER_RESPONSE_DELAY_MS           1000
#define LED_BLINK_CERTIFICATION_MODE_1   1
#define LED_BLINK_CERTIFICATION_MODE_2   2
#define LED_BLINK_CERTIFICATION_MODE_3   3
#define LED_BLINK_CERTIFICATION_MODE_4   4
#define LED_BLINK_CERTIFICATION_MODE_5   5

/* TYPES **********************************************************************/
/** @brief Structure tracking a button's state.
 */
typedef struct button_handle {
    /*! The ID of the button. */
    quasar_button_selection_t button_id;
    /*! Indicates whether the button is active. */
    bool active;
} button_handle_t;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void led1_blink(uint8_t blink_count);
static void handle_button_state(button_handle_t *button_handle, void (*button_callback)(void));

/* PRIVATE GLOBALS ************************************************************/
static facade_button_callbacks_t local_button_callbacks;

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
         * -> Entering in audio 96kHz 24-bit certification mode.
         */
        return FACADE_CERTIF_AUDIO_96k_24_BIT;
    }

    led1_blink(LED_BLINK_CERTIFICATION_MODE_2);
    quasar_timer_delay_ms(USER_RESPONSE_DELAY_MS);

    if (!quasar_button_read_state(QUASAR_BUTTON_USER_2)) {
        /* Button held for less than 2 delay periods.
         * -> Entering in audio 48kHz 24-bit certification mode.
         */
        return FACADE_CERTIF_AUDIO_48k_24_BIT;
    }

    led1_blink(LED_BLINK_CERTIFICATION_MODE_3);
    quasar_timer_delay_ms(USER_RESPONSE_DELAY_MS);

    if (!quasar_button_read_state(QUASAR_BUTTON_USER_2)) {
        /* Button held for less than 3 delay periods.
         * -> Entering in audio 48kHz 16-bit certification mode.
         */
        return FACADE_CERTIF_AUDIO_48k_16_BIT;
    }

    led1_blink(LED_BLINK_CERTIFICATION_MODE_4);
    quasar_timer_delay_ms(USER_RESPONSE_DELAY_MS);

    if (!quasar_button_read_state(QUASAR_BUTTON_USER_2)) {
        /* Button held for less than 4 delay periods.
         * -> Entering in audio 48kHz ADPCM certification mode.
         */
        return FACADE_CERTIF_AUDIO_48k_ADPCM;
    }

    /* Button held for more than 4 delay periods.
     * -> Entering in data certification mode.
     */
    led1_blink(LED_BLINK_CERTIFICATION_MODE_5);

    return FACADE_CERTIF_DATA;
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
    if (facade_is_certification_mode_active()) {
        return;
    }
    quasar_led_toggle(QUASAR_LED_USER_1);
}

void facade_tx_data_conn_status(void)
{
}

void facade_rx_audio_conn_status(void)
{
    if (facade_is_certification_mode_active()) {
        return;
    }
    quasar_led_toggle(QUASAR_LED_USER_2);
}

void facade_rx_data_conn_status(void)
{
}

void facade_fallback_status(bool on)
{
    if (facade_is_certification_mode_active()) {
        return;
    }
    if (on) {
        quasar_led_set(QUASAR_LED_USER_3);
    } else {
        quasar_led_clear(QUASAR_LED_USER_3);
    }
}

void facade_audio_process_timer_init(void (*callback)(void))
{
    quasar_timer_config_t timer_config = {
        .timer_selection = TIMER_SELECTION_AUDIO_PROCESS,
        /* Initialize timer base value to 1 second. */
        .time_base = QUASAR_TIMER_TIME_BASE_MILLISECOND,
        .time_period = 1000,
        .irq_priority = IRQ_PRIORITY_TIMER_AUDIO_PROCESS,
    };
    quasar_timer_init(&timer_config);
    quasar_it_set_timer17_callback(callback);
}

void facade_audio_process_timer_start(void)
{
    quasar_timer_start(TIMER_SELECTION_AUDIO_PROCESS);
}

void facade_audio_process_timer_trigger(void)
{
    quasar_timer_generate_event(TIMER_SELECTION_AUDIO_PROCESS);
}

void facade_audio_process_timer_stop(void)
{
    quasar_timer_stop(TIMER_SELECTION_AUDIO_PROCESS);
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
    quasar_it_set_timer16_callback(callback);
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
    if (facade_is_certification_mode_active()) {
        return;
    }
    quasar_led_clear(QUASAR_LED_USER_4);
}

void facade_payload_received_status(void)
{
    if (facade_is_certification_mode_active()) {
        return;
    }
    quasar_led_set(QUASAR_LED_USER_4);
}

bool facade_read_button_state(void)
{
    return quasar_button_read_state(QUASAR_BUTTON_USER_2);
}

void facade_notify_pairing_successful(void)
{
    if (facade_is_certification_mode_active()) {
        return;
    }
    quasar_rgb_configure_color(QUASAR_RGB_COLOR_GREEN);
    quasar_rgb_set();
}

/* **** Diagnostics ****
 *
 * Readers only -- every one of these reports state the BSP already keeps. The counters, the
 * scheduler timer and the HardFault snapshot live in quasar_it.c, which both quasar BSPs share,
 * so nothing new is being captured here; it was simply unreachable from this application.
 *
 * That is worth knowing when reading a crash: the snapshot below was already being written by
 * binaries built before this file changed. A board that hung can be interrogated over SWD by
 * reading hardfault_cfsr directly, with or without these accessors.
 */

bool facade_get_radio_hw_counters(uint32_t *r1_irq, uint32_t *r2_irq, uint32_t *r1_dma, uint32_t *r2_dma)
{
#if defined(STM32U535xx) || defined(STM32U5A5xx)
    *r1_irq = radio1_irq_count;
    *r2_irq = radio2_irq_count;
    *r1_dma = radio1_dma_count;
    *r2_dma = radio2_dma_count;
    return true;
#else
    (void)r1_irq;
    (void)r2_irq;
    (void)r1_dma;
    (void)r2_dma;
    return false; /* per-radio debug counters exist only on the quasar u535/u5a5 BSPs */
#endif
}

bool facade_get_sched_liveness(uint32_t *mrt, uint32_t *frt, bool *irq1, bool *irq2)
{
#if defined(STM32U535xx) || defined(STM32U5A5xx)
    *mrt = multi_radio_timer_count;
    *frt = (uint32_t)quasar_timer_free_running_ms_get_tick_count();
    *irq1 = quasar_radio_1_read_irq_pin();
    *irq2 = quasar_radio_2_read_irq_pin();
    return true;
#else
    (void)mrt;
    (void)frt;
    (void)irq1;
    (void)irq2;
    return false; /* scheduler-liveness signals exist only on the quasar u535/u5a5 BSPs */
#endif
}

bool facade_get_multi_radio_timer_regs(uint32_t *cr1, uint32_t *arr, uint32_t *cnt, uint32_t *dier)
{
#if defined(STM32U535xx) || defined(STM32U5A5xx)
    /* TIM4 is the SWC dual-radio "multi-radio" scheduler timer on both quasar BSPs
     * (quasar_timer_multi_radio_set_callback -> timer4). Read its live state to see
     * whether/why it stopped generating updates when the radio HW counters froze. */
    *cr1 = TIM4->CR1;
    *arr = TIM4->ARR;
    *cnt = TIM4->CNT;
    *dier = TIM4->DIER;
    return true;
#else
    (void)cr1;
    (void)arr;
    (void)cnt;
    (void)dier;
    return false; /* multi-radio timer is TIM4 only on the quasar u535/u5a5 BSPs */
#endif
}

bool facade_get_hardfault_snapshot(uint32_t *cfsr, uint32_t *hfsr, uint32_t *pc, uint32_t *lr)
{
#if defined(STM32U535xx) || defined(STM32U5A5xx)
    *cfsr = hardfault_cfsr;
    *hfsr = hardfault_hfsr;
    *pc = hardfault_regs.pc;
    *lr = hardfault_regs.lr;
    return true;
#else
    (void)cfsr;
    (void)hfsr;
    (void)pc;
    (void)lr;
    return false; /* HardFault snapshot globals exist only on the quasar u535/u5a5 BSPs */
#endif
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
            /* The button is released, deactivate the button. */
            button_handle->active = false;
        }
    }
}

#if defined(QUASAR_U535)
/** @brief Timeout for one console line, in ms. Generous: this is a diagnostic path. */
#define CONSOLE_UART_TX_TIMEOUT_MS 1000

/** @brief Bring one console UART up on first use and write to it.
 *
 *  Lazy because debug_enabled is false, so quasar_debug_init() never claimed UART4, and
 *  initialising only the two pins named here avoids the extra DEBUG_IO (PA4) GPIO
 *  configuration it would also do.
 *
 *  @param[in]     selection  UART peripheral.
 *  @param[in]     tx_port    TX GPIO port.
 *  @param[in]     tx_pin     TX GPIO pin.
 *  @param[in]     rx_port    RX GPIO port.
 *  @param[in]     rx_pin     RX GPIO pin.
 *  @param[in,out] ready      Per-port "already initialised" flag.
 *  @param[in]     string     Null-terminated text to send.
 */
static void console_port_write(quasar_uart_selection_t selection, GPIO_TypeDef *tx_port,
                               quasar_gpio_pin_t tx_pin, GPIO_TypeDef *rx_port,
                               quasar_gpio_pin_t rx_pin, bool *ready, char *string)
{
    quasar_bsp_status_t err = QUASAR_OK;

    if (!*ready) {
        quasar_gpio_config_t gpio_tx = {
            .port      = tx_port,
            .pin       = tx_pin,
            .mode      = QUASAR_GPIO_MODE_ALTERNATE,
            .type      = QUASAR_GPIO_TYPE_PP,
            .pull      = QUASAR_GPIO_PULL_UP,
            .speed     = QUASAR_GPIO_SPEED_LOW,
            .alternate = QUASAR_GPIO_ALTERNATE_AF8, /* UART4 on PC10/PC11, LPUART1 on PA3/PA2 */
        };
        quasar_gpio_config_t gpio_rx = {
            .port      = rx_port,
            .pin       = rx_pin,
            .mode      = QUASAR_GPIO_MODE_ALTERNATE,
            .type      = QUASAR_GPIO_TYPE_OD,
            .pull      = QUASAR_GPIO_PULL_UP,
            .speed     = QUASAR_GPIO_SPEED_LOW,
            .alternate = QUASAR_GPIO_ALTERNATE_AF8,
        };
        quasar_uart_config_t uart_cfg = {
            .uart_selection = selection,
            .baud_rate      = QUASAR_UART_BAUD_RATE_115200,
            .parity         = QUASAR_UART_PARITY_NONE,
            .stop           = QUASAR_UART_STOP_BITS_1B,
            .irq_priority   = QUASAR_IRQ_PRIORITY_NONE, /* TX only, nothing is read back */
            .gpio_config_tx = gpio_tx,
            .gpio_config_rx = gpio_rx,
        };

        quasar_uart_init(uart_cfg);
        *ready = true;
    }

    quasar_uart_transmit_blocking(selection, (uint8_t *)string, strlen(string),
                                  CONSOLE_UART_TX_TIMEOUT_MS, &err);
}

/** @brief Send a string to the console. u535 only -- everything else keeps the USB CDC default.
 *
 *  Overrides the weak facade_print_string() in common_backend, which writes to the USB CDC
 *  port. That is the right default on the u5a5 EVK and useless on this board: the u535's own
 *  UART header is not reliably populated and USB CDC is not necessarily up, so every stats
 *  line, every LINK_WATCH line and -- worst -- the "Quasar Error! Code: %d" that
 *  quasar_bsp_error_handler() prints before it starts blinking blue forever, all went
 *  nowhere. A fatal error that leaves no trace but a blinking LED costs an afternoon.
 *
 *  Written to BOTH serial ports this board can reach:
 *
 *    - UART4  on PC10 (TX) / PC11 (RX) -- the ST-Link VCP header.
 *    - LPUART1 on PA3 (TX) / PA2 (RX)  -- the expansion UART header. PA2 is documented as an
 *      unreliable pad, which is why nothing here reads; PA3 transmits.
 *
 *  Sending to both is a bring-up measure, not a design: which of the two headers is actually
 *  populated varies by board, and a diagnostic channel that might not be connected is worth
 *  very little. It costs one extra blocking transmit per line -- about 2 ms for a short one --
 *  and once it is known which port this board answers on, drop the other.
 *
 *  Blocking, deliberately. There is no realtime consumer, and the one caller that must not be
 *  lost is on its way into an infinite loop, so there is nothing for a queue to be kind to.
 */
void facade_print_string(char *string)
{
    static bool stlink_ready;
    static bool expansion_ready;

    if (string == NULL) {
        return;
    }

    console_port_write(QUASAR_DEF_UART_SELECTION_DEBUG, QUASAR_DEF_STLINK_UART_TX_PORT,
                       QUASAR_DEF_STLINK_UART_TX_PIN, QUASAR_DEF_STLINK_UART_RX_PORT,
                       QUASAR_DEF_STLINK_UART_RX_PIN, &stlink_ready, string);

    console_port_write(QUASAR_DEF_UART_SELECTION_EXPANSION, QUASAR_DEF_EXPANSION_UART_TX_PORT,
                       QUASAR_DEF_EXPANSION_UART_TX_PIN, QUASAR_DEF_EXPANSION_UART_RX_PORT,
                       QUASAR_DEF_EXPANSION_UART_RX_PIN, &expansion_ready, string);
}
#endif /* QUASAR_U535 */
