/** @file  puretone_unidirectional_backend.c
 *  @brief Implement puretone unidirectional facade prototype functions.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "at_cmd_core_facade.h"
#include "puretone_unidirectional_facade.h"
#include "quasar.h"
#include "quasar_it.h" /* dual-radio HW counters + HardFault snapshot (u535 & u5a5) */
#include "sac_cfg.h"
#include "tusb.h"  /* CONSOLE_ON_CDC: tud_cdc_* for the console */
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

/* The AT console selection sits OUTSIDE the QUASAR_U535 guard below, because the six
 * facade functions it configures are also outside it -- every board links them. Inside,
 * AT_CONSOLE_UART_SELECTION would be undefined on u5a5, every one of those functions would
 * take its no-op branch, and the build would be perfectly clean: the command table is in
 * the image, the host sees an AT port that never answers. */
/* AT COMMAND CONSOLE *********************************************************/
/** @brief Which UART carries the AT command channel.
 *
 *  Board dependent, and it has to be: the two u535 variants do not have the same working
 *  pins, and AT is the first thing in this application that needs to RECEIVE. The console
 *  above only ever transmits, which is why it could ignore the difference.
 *
 *    LDO   PA2 is an unreliable pad on the boards here, so LPUART1 cannot be trusted to
 *          receive. AT therefore takes UART4 (PC10/PC11) through the ST-Link VCP, which is
 *          wired through the on-board debugger and always usable. Statistics keep LPUART1,
 *          so nothing is lost -- the two channels simply separate.
 *
 *    SMPS  PA2 and PA3 both work in both directions and this variant has no UART4 header,
 *          so AT takes LPUART1 and the statistics share it. See facade_print_string().
 *
 *  U535_PWR_LDO is the discriminator, as it is for the console: CMakeLists.txt defines it
 *  only when the preset selects LDO, so its absence means SMPS. Override per build with
 *  -DAT_CONSOLE_ON_STLINK=0/1 rather than editing this file.
 */
#ifndef AT_CONSOLE_ON_STLINK
#if defined(U535_PWR_LDO)
#define AT_CONSOLE_ON_STLINK 1
#else
#define AT_CONSOLE_ON_STLINK 0
#endif
#endif

/** @brief How long facade_expansion_uart_flush() waits for the transmitter to drain. */
#define EXPANSION_UART_TX_TIMEOUT_MS 1000

#if AT_CONSOLE_ON_STLINK && defined(QUASAR_DEF_STLINK_UART_TX_PORT)
#define AT_CONSOLE_UART_SELECTION QUASAR_DEF_UART_SELECTION_DEBUG
#define AT_CONSOLE_UART_TX_PORT   QUASAR_DEF_STLINK_UART_TX_PORT
#define AT_CONSOLE_UART_TX_PIN    QUASAR_DEF_STLINK_UART_TX_PIN
#define AT_CONSOLE_UART_RX_PORT   QUASAR_DEF_STLINK_UART_RX_PORT
#define AT_CONSOLE_UART_RX_PIN    QUASAR_DEF_STLINK_UART_RX_PIN
#define AT_CONSOLE_UART_GPIO_AF   QUASAR_GPIO_ALTERNATE_AF8  /* UART4 on PC10/PC11 */
#elif defined(QUASAR_DEF_UART_SELECTION_EXPANSION)
#define AT_CONSOLE_UART_SELECTION QUASAR_DEF_UART_SELECTION_EXPANSION
#define AT_CONSOLE_UART_TX_PORT   QUASAR_DEF_EXPANSION_UART_TX_PORT
#define AT_CONSOLE_UART_TX_PIN    QUASAR_DEF_EXPANSION_UART_TX_PIN
#define AT_CONSOLE_UART_RX_PORT   QUASAR_DEF_EXPANSION_UART_RX_PORT
#define AT_CONSOLE_UART_RX_PIN    QUASAR_DEF_EXPANSION_UART_RX_PIN
/* The alternate function is per PERIPHERAL, not per board, and the two boards put a
 * different peripheral on their expansion pins: LPUART1 on u535, USART2 on u5a5. Writing one
 * value here compiles on both and muxes the u5a5 pins to a peripheral that is not driving
 * them, so nothing reaches the wire and nothing says why. */
#ifdef QUASAR_U535
#define AT_CONSOLE_UART_GPIO_AF   QUASAR_GPIO_ALTERNATE_AF8  /* LPUART1 on PA3/PA2 */
#else
#define AT_CONSOLE_UART_GPIO_AF   QUASAR_GPIO_ALTERNATE_AF7  /* USART2 on PA2/PA3 */
#endif
#endif

/* Set once facade_expansion_uart_init() has configured the console. Every function that
 * touches that UART checks it, because the AT core initialises well after main() starts and
 * facade_print_string() can reach this file before then. */
static bool at_console_ready;

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
/** @brief Whether any statistics output still goes straight to a UART of its own.
 *
 *  False only on the SMPS variant, where AT takes the single available port and the
 *  statistics are handed to its queue instead. Named rather than repeated because the helper
 *  below and both of its call sites have to agree; when they did not, the helper was compiled
 *  in with no callers and the build warned. */
#if AT_CONSOLE_ON_STLINK || defined(U535_PWR_LDO)
#define CONSOLE_HAS_OWN_UART 1
#else
#define CONSOLE_HAS_OWN_UART 0
#endif

#if CONSOLE_HAS_OWN_UART
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
 *  Which port depends on the board variant, and the two u535 builds differ:
 *
 *    - LDO board  -- BOTH. UART4 on PC10 (TX) / PC11 (RX), the ST-Link VCP header, AND
 *      LPUART1 on PA3 (TX) / PA2 (RX), the expansion header. Both headers are populated and
 *      both get used on the bench, so both stay. This costs one extra blocking transmit per
 *      line, roughly 2 ms for a short one, and that is accepted deliberately rather than
 *      being a leftover from bring-up.
 *
 *    - SMPS board -- LPUART1 only. That is the header this variant has.
 *
 *  Nothing here reads on either board, because this is a one-way diagnostic channel -- not
 *  because reception is impossible. The receive side differs between the two variants and the
 *  distinction matters for anything that needs RX, the AT layer above all:
 *
 *    - LDO   PA2 is an unreliable pad on the board we have, so LPUART1 cannot be trusted to
 *            receive. Anything needing RX has to go on UART4 (PC10/PC11) here.
 *    - SMPS  PA2 and PA3 both work, transmit and receive. LPUART1 is usable as a full console.
 *
 *  U535_PWR_LDO is the discriminator: CMakeLists.txt only defines it when the preset selects
 *  LDO, so its absence means SMPS.
 *
 *  Blocking, deliberately. There is no realtime consumer, and the one caller that must not be
 *  lost is on its way into an infinite loop, so there is nothing for a queue to be kind to.
 */
#endif /* CONSOLE_HAS_OWN_UART */

void facade_print_string(char *string)
{
#if CONSOLE_HAS_OWN_UART && !AT_CONSOLE_ON_STLINK
    static bool stlink_ready;
#endif
#if AT_CONSOLE_ON_STLINK
    static bool expansion_ready;
#endif

    if (string == NULL) {
        return;
    }

#if CONSOLE_HAS_OWN_UART && !AT_CONSOLE_ON_STLINK
    /* LDO board only, and only while AT is not using it: this variant has the ST-Link VCP
     * header populated as well. */
    console_port_write(QUASAR_DEF_UART_SELECTION_DEBUG, QUASAR_DEF_STLINK_UART_TX_PORT,
                       QUASAR_DEF_STLINK_UART_TX_PIN, QUASAR_DEF_STLINK_UART_RX_PORT,
                       QUASAR_DEF_STLINK_UART_RX_PIN, &stlink_ready, string);
#endif

#if AT_CONSOLE_ON_STLINK
    console_port_write(QUASAR_DEF_UART_SELECTION_EXPANSION, QUASAR_DEF_EXPANSION_UART_TX_PORT,
                       QUASAR_DEF_EXPANSION_UART_TX_PIN, QUASAR_DEF_EXPANSION_UART_RX_PORT,
                       QUASAR_DEF_EXPANSION_UART_RX_PIN, &expansion_ready, string);
#else
    /* AT owns LPUART1 on this variant, and it is the only port there is. Rather than silence
     * the statistics -- which would leave the board that goes to the ODM with no diagnostic
     * output at all -- they go out through the SAME queue as the AT replies.
     *
     * Sharing the queue is what makes sharing the wire safe. Two writers on one UART, one
     * blocking and one interrupt driven, interleave mid-character; one FIFO fed by both
     * interleaves between whole strings, so a host sees complete "[HS] ..." lines beside
     * complete "+EVENT: ..." lines and can ignore what it does not recognise.
     *
     * Before the AT console is initialised this drops the string rather than blocking, which
     * costs the boot banner on this variant. Recorded rather than worked around: the banner
     * is printed before any UART exists, and giving it one would mean configuring the same
     * pins twice with different settings. */
    facade_expansion_uart_write(string);
#endif
}
#endif /* QUASAR_U535 */

#if defined(AT_CONSOLE_UART_SELECTION) && !AT_CONSOLE_ON_STLINK
/* The expansion pins are not the same peripheral on the two boards, and on the u535 they are
 * not even the same way round. USART2 does not exist there, so PA3/PA2 are LPUART1_TX/RX
 * through AF8; on the u5a5 the same pads are USART2 with PA2 as TX, through AF7. That has
 * been established once already and then lost, which is what these assertions are for --
 * getting it wrong costs nothing at build time and presents as a port that never answers.
 *
 * Asserted rather than hardcoded: the values still come from the BSP, and these only refuse
 * to build if the BSP stops saying what this code was written against. */
#ifdef QUASAR_U535
_Static_assert(AT_CONSOLE_UART_SELECTION == QUASAR_UART_SELECTION_LPUART1,
               "u535 has no USART2 -- the expansion console is LPUART1");
_Static_assert(AT_CONSOLE_UART_TX_PIN == QUASAR_GPIO_PIN_3 && AT_CONSOLE_UART_RX_PIN == QUASAR_GPIO_PIN_2,
               "u535 expansion is PA3=TX PA2=RX, the opposite way round from the u5a5");
#else
_Static_assert(AT_CONSOLE_UART_TX_PIN == QUASAR_GPIO_PIN_2 && AT_CONSOLE_UART_RX_PIN == QUASAR_GPIO_PIN_3,
               "u5a5 expansion is PA2=TX PA3=RX");
#endif
#endif

/** @brief Send the console to the USB CDC port instead of the expansion UART.
 *
 *  Default ON for u5a5, OFF for u535, and a -DCONSOLE_ON_CDC=<0|1> on the configure line
 *  overrides either.
 *
 *  The two boards are not in the same position. On the u5a5 the console shares USART2 on the
 *  expansion header with AT, so reading it costs an external adapter and a second port, and
 *  the USB socket that is already plugged in for power sits idle. The default was OFF because
 *  the UART path was the tested one -- but the effect was that the ordinary way to build a
 *  u5a5 produced a board whose console could not be read without extra hardware, and a
 *  console nobody can reach is the same as no console. USB CDC is up on u5a5 either way:
 *  common_backend.c calls tinyusb_baremetal_setup() unconditionally on every baremetal build.
 *
 *  The u535 keeps the UART. Its expansion pins are LPUART1 and both directions work there,
 *  which is why AT lives on that line, and that routing is what has been verified on hardware.
 *
 *  QUASAR_U535 is reliable HERE and not everywhere: it is PUBLIC on the hardware target, this
 *  file is compiled into unidir_backend which links hardware directly, and the define is in
 *  the compile line -- checked, not assumed. Further down the link chain it is lost, which is
 *  what BOARD_NAME exists for. Do not copy this test into an application file.
 */
#ifndef CONSOLE_ON_CDC
#ifdef QUASAR_U535
#define CONSOLE_ON_CDC 0
#else
#define CONSOLE_ON_CDC 1
#endif
#endif

#if CONSOLE_ON_CDC
/** @brief Console output over USB CDC, with everything printed before enumeration kept.
 *
 *  The weak implementation in common_backend.c drops any string written while
 *  tud_cdc_connected() is false, and on this application that silently loses the most
 *  valuable output there is. The boot banner prints within milliseconds of reset; USB
 *  enumeration takes hundreds. So the banner -- which is what says WHICH binary is on the
 *  board -- never reaches the terminal, while messages from a second or two later arrive
 *  perfectly. A log that begins mid-sequence looks like a board that started mid-sequence.
 *
 *  Everything written before the host is listening therefore goes into a buffer, and the
 *  first write after the port opens flushes it in order. The terminal then shows the whole
 *  boot from the banner onward, however late it was opened.
 *
 *  Overflow drops the NEWEST text rather than the oldest, which is the opposite of a normal
 *  ring buffer and deliberate: the early lines are the ones that cannot be obtained any other
 *  way, while later ones will be reprinted by the next periodic pass anyway.
 */
static char s_boot_log[3072];
static uint16_t s_boot_log_len;
static bool s_cdc_flushed;

/** @brief Push a whole string into the CDC FIFO, letting the timer-driven tud_task drain it.
 *
 *  tud_cdc_write() accepts only what currently fits, and the buffered boot log is larger than
 *  the FIFO. Bounded rather than a plain retry loop: a host that has opened the port and then
 *  stopped reading must not be able to hold the caller here forever.
 */
static void cdc_write_all(const char *data, uint32_t len)
{
    uint32_t sent = 0;
    uint32_t spins = 0;

    while (sent < len && spins < 200000) {
        uint32_t n = tud_cdc_write(data + sent, len - sent);

        if (n == 0) {
            spins++;
        } else {
            sent += n;
            spins = 0;
        }
        tud_cdc_write_flush();
    }
}

void facade_print_string(char *string)
{
    uint32_t len;

    if (string == NULL) {
        return;
    }
    len = strlen(string);

    if (!tud_cdc_connected()) {
        if ((s_boot_log_len + len) < sizeof(s_boot_log)) {
            memcpy(&s_boot_log[s_boot_log_len], string, len);
            s_boot_log_len = (uint16_t)(s_boot_log_len + len);
        }
        return;
    }

    if (!s_cdc_flushed) {
        s_cdc_flushed = true;
        if (s_boot_log_len > 0) {
            cdc_write_all(s_boot_log, s_boot_log_len);
            s_boot_log_len = 0;
        }
    }

    cdc_write_all(string, len);
}
#endif /* CONSOLE_ON_CDC */

#if !defined(QUASAR_U535) && !CONSOLE_ON_CDC
/** @brief Console output for boards that are not the u535.
 *
 *  Overrides the weak USB CDC implementation in common_backend.c.
 *
 *  CORRECTION, 2026-08-25. This comment used to claim the CDC path "prints nothing at all"
 *  because "nothing in this tree ever calls tusb_init() or tud_task()". That is false, and
 *  believing it cost a bench session: common_backend.c:57-61 calls tinyusb_baremetal_setup()
 *  UNCONDITIONALLY on every baremetal build -- there is no USB_AUDIO_ENABLED guard -- and
 *  that function calls tusb_init() and hands tud_task() to a timer. USB CDC is up on u5a5,
 *  which is exactly how the puretone_headset line prints its statistics there
 *  (puretone_headset_backend.c:533-535, which never overrides facade_print_string at all).
 *
 *  Whatever symptom prompted this override, the mechanism recorded for it was wrong. It is no
 *  longer the u5a5 default -- CONSOLE_ON_CDC now defaults on there -- so this block is reached
 *  only by an explicit -DCONSOLE_ON_CDC=0. Kept as that escape hatch rather than deleted:
 *  it is the routing the u535 boards were verified on, and it is the thing to fall back to if
 *  the CDC console ever turns out to disturb something on the u5a5.
 *
 *  Shares the AT queue rather than taking a second UART, for the reason set out in the u535
 *  branch above: one FIFO fed by both writers interleaves between whole strings, whereas two
 *  writers on one UART interleave mid-character. A host sees complete "[DG] ..." lines beside
 *  complete "+EVENT: ..." lines.
 */
void facade_print_string(char *string)
{
    facade_expansion_uart_write(string);
}
#endif /* !QUASAR_U535 */

void facade_expansion_uart_init(uint32_t baud_rate)
{
#if defined(AT_CONSOLE_UART_SELECTION)
    quasar_gpio_config_t gpio_tx = {
        .port      = AT_CONSOLE_UART_TX_PORT,
        .pin       = AT_CONSOLE_UART_TX_PIN,
        .mode      = QUASAR_GPIO_MODE_ALTERNATE,
        .type      = QUASAR_GPIO_TYPE_PP,
        .pull      = QUASAR_GPIO_PULL_UP,
        .speed     = QUASAR_GPIO_SPEED_LOW,
        .alternate = AT_CONSOLE_UART_GPIO_AF,
    };
    quasar_gpio_config_t gpio_rx = {
        .port      = AT_CONSOLE_UART_RX_PORT,
        .pin       = AT_CONSOLE_UART_RX_PIN,
        .mode      = QUASAR_GPIO_MODE_ALTERNATE,
        .type      = QUASAR_GPIO_TYPE_OD,
        .pull      = QUASAR_GPIO_PULL_UP,
        .speed     = QUASAR_GPIO_SPEED_LOW,
        .alternate = AT_CONSOLE_UART_GPIO_AF,
    };
    quasar_uart_config_t uart_cfg = {
        .uart_selection = AT_CONSOLE_UART_SELECTION,
        .baud_rate      = baud_rate,
        .parity         = QUASAR_UART_PARITY_NONE,
        .stop           = QUASAR_UART_STOP_BITS_1B,
        /* Priority 12: above the audio process timer (13) and the SWC data timer (15), below
         * the radio. The handler is a few register reads into a software FIFO, so it cannot
         * starve them; sitting UNDER them would, because their callbacks run long enough to
         * exhaust the 8-byte hardware FIFO and the tail of a pasted command goes missing. */
        .irq_priority   = QUASAR_IRQ_PRIORITY_12,
        .gpio_config_tx = gpio_tx,
        .gpio_config_rx = gpio_rx,
    };
    quasar_uart_init(uart_cfg);
    at_console_ready = true;
#else
    (void)baud_rate;  /* No usable console UART on this board variant. */
#endif
}

void facade_expansion_uart_write(char *string)
{
#if defined(AT_CONSOLE_UART_SELECTION)
    if ((string == NULL) || !at_console_ready) {
        return;
    }

    /* IRQ-driven, not blocking. A blocking write spins until the last byte has left -- about
     * 2.3 ms for an event line at 115200 -- and the callers that matter are the +EVENT
     * notifiers, which run in the wireless RX callback at PendSV priority 12, ABOVE the audio
     * process timer (13) and the SWC data timer (15). Every event would freeze the audio
     * pipeline for the length of its line. On the headset line that was audible at one vendor
     * command per second: a per-event cost, not a load effect, which is why changing the
     * command rate did not change it.
     *
     * The cost is that "written" no longer means "sent", so anything that stops the CPU
     * afterwards has to call facade_expansion_uart_flush() first. */
    quasar_uart_transmit_string_irq(AT_CONSOLE_UART_SELECTION, string, strlen(string));
#else
    (void)string;
#endif
}

void facade_expansion_uart_flush(void)
{
#if defined(AT_CONSOLE_UART_SELECTION)
    uint32_t deadline;

    if (!at_console_ready) {
        return;
    }

    deadline = facade_get_tick_ms() + EXPANSION_UART_TX_TIMEOUT_MS;

    /* Bounded: this only runs on a path that is about to reset or power down, and a
     * peripheral that never reports completion must not be able to hold that path open. */
    while (!quasar_uart_transmit_is_complete(AT_CONSOLE_UART_SELECTION)) {
        if ((int32_t)(facade_get_tick_ms() - deadline) >= 0) {
            break;
        }
    }
#endif
}

uint8_t facade_expansion_uart_read_byte(void)
{
#if defined(AT_CONSOLE_UART_SELECTION)
    if (!at_console_ready) {
        return 0;
    }

    return quasar_uart_receive_irq(AT_CONSOLE_UART_SELECTION);
#else
    return 0;
#endif
}

void facade_system_reset(void)
{
    quasar_system_reset();
}

void facade_uwb_shutdown(void)
{
    /* Both radios unconditionally. Not gated on SWC_RADIO_COUNT: a single-radio build can be
     * bound to the second physical radio through SWC_SINGLE_RADIO_ID, so counting radios
     * would leave the live one powered. Asserting the shutdown pin of a radio this build does
     * not use is harmless. */
    quasar_radio_1_set_shutdown_pin();
    quasar_radio_2_set_shutdown_pin();
}
