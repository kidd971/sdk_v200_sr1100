/** @file  puretone_headset_node.c
 *  @brief This application creates a bidirectional audio stream at 96kHz/24-bit from the audio interface of the
 *         Coordinator to the audio interface of the Node, and at 48kHz/16-bit from the Node to the Coordinator.
 *         It utilizes multiple fallback modes to reduce audio quality down to 48 kHz ADPCM to preserve link quality
 *         under varying conditions.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES ******************************************************************/
#include <stdio.h>
#include "at_cmd_core.h"
#include "at_cmd_core_facade.h"  /* facade_expansion_uart_write: route link_watch to the AT UART pin */
#include "pairing_api.h"
#include "pairing_cfg.h"
#include "puretone_headset_facade.h"
#include "reconnect_store.h"  /* boot auto-reconnect: persist/restore the pairing address */
#include "sac_api.h"
#include "sac_cfg.h"
#include "sac_compression.h"
#include "sac_dummy_endpoint.h"
#include "sac_endpoint_swc.h"
#include "sac_fallback.h"
#include "sac_fallback_gate.h"
#include "sac_hal_facade.h"
#include "sac_mute_on_underflow.h"
#include "sac_mute_packet.h"
#include "sac_packing.h"
#include "sac_sample_accumulator.h"
#include "sac_src_cmsis.h"
#include "sac_stats.h"
#include "sac_utils.h"
#include "sac_volume.h"
#include "swc_api.h"
#include "swc_cfg.h"
#include "swc_cfg_node.h"
#include "swc_stats.h"

/* CONSTANTS ******************************************************************/
/* Total memory needed for the Audio Core. */
#define SAC_MEM_POOL_SIZE 52000
/* Total memory needed for the Wireless Core. */
#define SWC_MEM_POOL_SIZE 10500
/* The data connection supports up to 16 bytes. */
#define MAX_DATA_PAYLOAD_SIZE 16
/* Length of the statistics array used for terminal display. */
#define STATS_ARRAY_LENGTH 5000
/* Period for data transmission timer in ms. */
#define DATA_TX_PERIOD_MS 10
/* Boot auto-reconnect: max time to wait for the persisted peer before falling back to pairing. */
#define RECONNECT_TIMEOUT_MS 10000
/* Period for statistics print timer in ms. */
#define STATS_PRINT_PERIOD_MS 1000
/* Size of the buffer used to print errors (room for the assert file:line + code). */
#define ERROR_MESSAGE_BUFFER_SIZE 160
/* Interval to print statistics in ms. */
#define PRINT_INTERVAL_MS 2000
/* Certification-mode heartbeat: status LED toggles every this many ms (~2 Hz blink,
 * deliberately faster than the pairing patterns so cert mode is visually distinct). */
#define CERTIF_LED_TOGGLE_MS 250

/* **** Link watch ****
 * Lightweight diagnostic that polls the RX-audio connection and prints a one-line
 * report so we can see WHY/WHEN the HS link drops and whether it recovers.
 *   - All SWC reads use a local err and DO NOT call ASSERT_SWC_STATUS, so the watch
 *     itself can never trap in swc_error_handler()'s while(1).
 *   - A monotonically increasing sequence number lets you tell a real RF loss
 *     (keeps printing "LOST") apart from a firmware hang (log freezes mid-stream).
 * Set LINK_WATCH to 0 to compile it out. */
#ifndef LINK_WATCH
#define LINK_WATCH 1  /* ON for dual-radio crash-log collection: prints every LINK_WATCH_INTERVAL_MS
                       * on the AT/expansion UART (LPUART1 TX). Set to 0 for release. */
#endif
/* Poll/print cadence for the link watch in ms. */
#define LINK_WATCH_INTERVAL_MS 2000

/* Periodic on-board crash/stall snapshot: emit the consolidated dump every N ms,
 * WITHOUT waiting for a stall or any command (the AT-UART RX pad is unusable on this
 * board, so the log is grabbed automatically). Output goes to LPUART1 TX. Set to 0
 * to disable. */
#ifndef CRASH_DUMP_PERIODIC_MS
#define CRASH_DUMP_PERIODIC_MS 2000
#endif

/* Periodic print_stats() dump. Turned OFF by default on this debug branch so the
 * CDC log is quiet enough to watch the crash-dump / assert-trap lines. print_stats()
 * is also one of the assert-trap sources (see ASSERT_SWC/SAC_STATUS inside it), so
 * disabling it removes that noise AND that trap while investigating. Set to 1 to
 * restore normal stats output. */
#ifndef PRINT_STATS_ENABLED
#define PRINT_STATS_ENABLED 0
#endif

/* **** Latch-recovery test hooks (dual-radio wedge investigation) ****
 * When the radio wedges, the main loop stays alive (the periodic crash-dump keeps
 * printing) so the polled buttons are still serviced — as long as you DON'T press the
 * pairing button (USER_1), which traps in unpair_device()->swc_disconnect. So we
 * repurpose the two volume buttons as non-trapping recovery tests you can press WHILE
 * wedged, to fill in the recovery matrix without pulling board power:
 *   USER_3 (vol+) -> dbg_soft_reset(): MCU reset only, radio stays powered.
 *   USER_4 (vol-) -> dbg_radio_por():  assert both radios' shutdown pins + hold + MCU reset.
 * Outcome tells us whether the SR1100 latch clears on a plain reboot, only after an
 * explicit radio power-down, or only after real board-power removal (deep POR).
 * Set to 0 to restore normal volume buttons. */
#ifndef LATCH_TEST_HOOKS
#define LATCH_TEST_HOOKS 1
#endif

/* **** Standby test hook (bench only — off in product builds) ****
 * Set to 1 to bind USER_3 to at_start_disconnect(), i.e. the Standby power-down, so the
 * sleep path can be exercised from the board. Needed on benches where the AT-UART RX pad
 * is unusable (see CRASH_DUMP_PERIODIC_MS above), which makes AT+UWB_DISCONNECT
 * unreachable. Takes precedence over LATCH_TEST_HOOKS for that button. Left at 0 so the
 * product keeps its normal button behaviour and reaches Standby over AT. */
#ifndef STANDBY_TEST_HOOKS
#define STANDBY_TEST_HOOKS 0
#endif

/* **** Stall auto-recovery (EXPERIMENTAL mitigation — NOT the real fix) ****
 * Dual-radio has no aggressive re-sync after prolonged link loss (fast-sync is hard-blocked
 * with two radios), so a node whose scheduler stalls re-syncs only very slowly. As a stopgap,
 * watch the radio HW liveness counters: if all four stay frozen for STALL_AUTO_RECOVER_MS
 * while paired (the wedge signature), force a scheduler/radio re-init via swc_disconnect()
 * (tolerating the disconnect-timeout) + swc_connect(). This is a band-aid; the proper fix
 * (dual-radio aggressive re-sync) belongs in SPARK's SWC core — report to HQ. 0 disables.
 * Only meaningful on the node/HS and on boards with the HW counters (u535/u5a5 dual-radio). */
#ifndef STALL_AUTO_RECOVER_MS
#define STALL_AUTO_RECOVER_MS 6000
#endif
/* How often to sample the counters (must be well below STALL_AUTO_RECOVER_MS). */
#define STALL_SAMPLE_MS 1000

/* **** Assert-site capture (no ST-Link needed) ****
 * The library ASSERT_SWC/SAC_STATUS macros only pass the error CODE to the fatal
 * handler, not where the assert tripped. Re-define them for THIS translation unit
 * so they stash __FILE__/__LINE__ into the globals below before trapping; the fatal
 * handler then prints "<TAG> TRAP <file>:<line> code=<n>" to the CDC. So a repro is
 * self-diagnosing: the last line before the log freezes names the exact assert. */
static volatile const char *s_assert_file = NULL;
static volatile uint32_t s_assert_line = 0;

#undef ASSERT_SWC_STATUS
#define ASSERT_SWC_STATUS(swc_status)        \
    do {                                     \
        if ((swc_status) == SWC_ERR_NONE) {  \
            break;                           \
        }                                    \
        if ((swc_status) > 0) {              \
            swc_warning_handler(swc_status); \
            break;                           \
        }                                    \
        s_assert_file = __FILE__;            \
        s_assert_line = __LINE__;            \
        swc_error_handler(swc_status);       \
    } while (0)

#undef ASSERT_SAC_STATUS
#define ASSERT_SAC_STATUS(sac_status)        \
    do {                                     \
        if ((sac_status) == SAC_OK) {        \
            break;                           \
        }                                    \
        if ((sac_status) > SAC_OK) {         \
            sac_warning_handler(sac_status); \
            break;                           \
        }                                    \
        s_assert_file = __FILE__;            \
        s_assert_line = __LINE__;            \
        sac_error_handler(sac_status);       \
    } while (0)

/* **** Fallback **** */
/* Fallback channel index. */
#define FALLBACK_INDEX_0 0

/* TYPES **********************************************************************/
/** @brief Enumeration representing device pairing states.
 */
typedef enum device_pairing_state {
    /*! The device is unpaired with the Coordinator. */
    DEVICE_UNPAIRED,
    /*! The device pairing is active. */
    DEVICE_PAIRING,
    /*! The device is paired with the Coordinator. */
    DEVICE_PAIRED,
} device_pairing_state_t;

/** @brief Enumeration representing the connection priorities.
 */
typedef enum connection_priority {
    /*! Audio connection priority allows prioritizing audio transfers. */
    AUDIO_CONNECTION_PRIORITY = 0,
    /*! Data connection priority allows data transfers without compromising audio transfers. */
    DATA_CONNECTION_PRIORITY = 1,
} connection_priority_t;

/** @brief Data used for transmitting and receiving link margin and button state.
 */
typedef struct user_data {
    /*! A boolean indicating the button's state. */
    bool button_state;
    /*! The link margin to monitor link quality. */
    uint8_t link_margin;
    /*! Pending command from node: 0=none, 1=next_track, 2=pre_track, 3=play, 4=stop. */
    uint8_t cmd_type;
    /*! Battery level of the node (0-100%). */
    uint8_t battery_pct;
} user_data_t;

/** @brief Enumeration representing the fallback states.
 */
typedef enum fallback_states {
    /*! Default fallback state with automatic algorithm. */
    FALLBACK_AUTO,
    /*! Forced fallback state to 48kHz uncompressed audio. */
    FALLBACK_48K_UNCOMPRESSED,
    /*! Forced fallback state to 48kHz compressed audio. */
    FALLBACK_48K_ADPCM,
    /*! Total number of fallback states. */
    FALLBACK_STATE_COUNT,
} fallback_states_t;

/* PRIVATE GLOBALS ************************************************************/
/* **** Audio Core **** */
/** Sample format of audio samples produced or received by the codec of the Coordinator.
 *
 *  The audio format produced by the codec is configured according to the requirements of the main channel. However,
 *  since the codec configuration remains the same for both audio produced and received, the audio from the back
 *  channel received by the codec also needs to adhere to the same audio format.
 */
static const sac_sample_format_t I2S_SAC_SAMPLE_FORMAT = {
    .bit_depth = SAC_24BITS,
    .sample_encoding = SAC_SAMPLE_UNPACKED,
};

/* Sample format of audio samples sent by the SWC of the Coordinator. */
static const sac_sample_format_t MAIN_CHANNEL_SAC_SAMPLE_FORMAT = {
    .bit_depth = SAC_24BITS,
    .sample_encoding = SAC_SAMPLE_PACKED,
};

/* Sample format of audio samples received by the SWC of the Coordinator. */
static const sac_sample_format_t BACK_CHANNEL_SAC_SAMPLE_FORMAT = {
    .bit_depth = SAC_16BITS,
    .sample_encoding = SAC_SAMPLE_PACKED,
};

#define MAIN_CHANNEL_CONSUMER_SAC_SAMPLE_FORMAT \
    (USB_AUDIO_ENABLED ? MAIN_CHANNEL_SAC_SAMPLE_FORMAT : I2S_SAC_SAMPLE_FORMAT)

static uint8_t audio_memory_pool[SAC_MEM_POOL_SIZE];
static sac_pipeline_t *main_channel_sac_pipeline;
static sac_pipeline_t *main_channel_accumulator_pipeline;
static sac_pipeline_t *back_channel_sac_pipeline;
static sac_pipeline_t *back_channel_src_pipeline;

#if SINE_INJECT_HS
/* SINE_INJECT_HS (HS local test): free-running 1 kHz sine straight to SAI TX (SD_A).
 * The external I2S clock keeps completing the TX DMA; each completion refills this buffer
 * and re-arms the write, so a tone comes out SD_A with no radio link and without the pipeline.
 * Sized to one normal I2S consumer payload so cadence/format match real audio. */
static int32_t main_channel_tx_sine_buf[MAIN_CHANNEL_I2S_PAYLOAD_SIZE / sizeof(int32_t)];
#endif

/* **** Processing Stages **** */
/* **** Main Channel Processing Stages **** */
static sac_fallback_instance_t main_channel_fallback_instance;
static sac_processing_t *main_channel_fallback_processing;
static sac_packing_instance_t main_channel_unpacking_instance;
static sac_processing_t *main_channel_unpacking_processing;
static sac_compression_instance_t main_channel_decompression_instance;
static sac_processing_t *main_channel_decompression_processing;
static sac_processing_t *main_channel_cdc_processing;
static sac_mute_on_underflow_instance_t main_channel_mute_on_underflow_instance;
static sac_processing_t *main_channel_mute_on_underflow_processing;
static src_cmsis_instance_t main_channel_upsampling_instance;
static sac_processing_t *main_channel_upsampling_processing;
static sac_mute_packet_instance_t main_channel_mute_packet_instance;
static sac_processing_t *main_channel_mute_packet_processing;
static sac_packing_instance_t main_channel_fbk_unpacking_instance;
static sac_processing_t *main_channel_fbk_unpacking_processing;
static sac_sample_accumulator_instance_t main_channel_sample_accumulator_instance;
static sac_processing_t *main_channel_sample_accumulator_processing;
static sac_volume_instance_t main_channel_volume_instance;
static sac_processing_t *main_channel_volume_processing;

/* **** Back Channel Processing Stages **** */
static sac_fallback_instance_t back_channel_fallback_instance;
static sac_processing_t *back_channel_fallback_processing;
static sac_compression_instance_t back_channel_compression_instance;
static sac_processing_t *back_channel_compression_processing;
static sac_processing_t *back_channel_compression_discard_processing;
static src_cmsis_instance_t back_channel_downsampling_instance;
static sac_processing_t *back_channel_downsampling_processing;
static sac_sample_accumulator_instance_t back_channel_sample_accumulator_instance;
static sac_processing_t *back_channel_sample_accumulator_processing;

/* **** Endpoints **** */
static sac_endpoint_t *main_channel_consumer_endpoint;
static ep_swc_instance_t main_channel_swc_producer_instance;
static sac_endpoint_t *main_channel_swc_producer_endpoint;
static sac_endpoint_t *back_channel_producer_endpoint;
static ep_swc_instance_t back_channel_swc_consumer_instance;
static sac_endpoint_t *back_channel_swc_consumer_endpoint;
static sac_endpoint_t *dummy_audio_consumer;
static sac_endpoint_t *dummy_audio_producer;
static sac_endpoint_t *back_channel_src_consumer_endpoint;
static sac_endpoint_t *back_channel_src_swc_producer;

/* **** Wireless Core **** */
static uint8_t swc_memory_pool[SWC_MEM_POOL_SIZE];

/* ** RX Connections ** */
static swc_connection_t *rx_audio_conn;
static swc_connection_t *tx_data_conn;

/* ** TX Connections ** */
static swc_connection_t *tx_audio_conn;
static swc_connection_t *rx_data_conn;

static uint32_t timeslot_us[] = SCHEDULE;
static uint32_t channel_sequence[] = CHANNEL_SEQUENCE;
static uint32_t channel_frequency[] = CHANNEL_FREQ;

/* There is a bidirectional link for audio and a bidirectional link for data with a lower connection priority. */
static int32_t tx_timeslots[] = NODE_TIMESLOTS;
static int32_t rx_timeslots[] = COORD_TIMESLOTS;

/* Non-fatal error tracking for the 10 ms data path. These used to be ASSERT_SWC_STATUS
 * traps (swc_error_handler -> while(1)): on a link drop swc_connection_send() returns a
 * queue-full / not-connected error every 10 ms, which would hang the MCU so the HS could
 * never recover. We now record the codes here and keep running; link_watch() surfaces them. */
static volatile swc_error_t s_last_fb_info_err;  /* last err from swc_connection_get_fallback_info */
static volatile swc_error_t s_last_send_err;     /* last err from swc_connection_send */
static volatile uint32_t s_send_err_count;       /* cumulative data-send failures */

/* **** Application Specific **** */
static facade_certification_mode_t certification_mode;
static fallback_states_t fallback_state;
/* Variables supporting pairing between the two devices. */
static device_pairing_state_t device_pairing_state;
static pairing_cfg_t app_pairing_cfg;
static pairing_assigned_address_t pairing_assigned_address;
/* True while try_boot_reconnect() owns the half-open link and is polling it. During that
 * window device_pairing_state is already DEVICE_PAIRED, so any teardown reached from the
 * polled button / AT handlers (unpair_device(), at_start_disconnect()) would NULL the
 * connection handles under the polling loop -- and swc_connection_get_connect_status()
 * dereferences its argument with no NULL check. Teardown is therefore deferred: the
 * handlers only raise s_boot_reconnect_abort and the loop unwinds itself. */
static volatile bool s_boot_reconnect_active;
static volatile bool s_boot_reconnect_abort;

/* Fallback latency. */
uint8_t main_channel_fbk_latency_queue_size[] = MAIN_CHANNEL_FALLBACK_LATENCY_QUEUE_SIZE;
uint8_t main_channel_fbk_latency_fifo_size[] = MAIN_CHANNEL_FALLBACK_LATENCY_FIFO_SIZE;

static volatile uint32_t main_channel_trigger_count;

/* Main channel audio sample accumulator settings. */
uint8_t back_channel_acc_mul[] = BACK_CHANNEL_ACC_MUL;
uint8_t back_channel_acc_div[] = BACK_CHANNEL_ACC_DIV;

static uint8_t s_pending_cmd = 0;
static uint8_t s_battery_pct = 0;

/* PRIVATE FUNCTION PROTOTYPE *************************************************/
static void app_init(void);
static void app_swc_core_init(pairing_assigned_address_t *app_pairing, swc_error_t *swc_err);
static void app_audio_core_init(void);

/* **** Callbacks **** */
/* Callbacks that are used for the main channel. */
static void conn_rx_audio_success_callback(void *conn, void *arg);
static void conn_tx_data_success_callback(void *conn, void *arg);
static void main_channel_audio_tx_complete_callback(void);
static void audio_process_main_channel_callback(void);
/* Callbacks that are used for the back channel. */
static void conn_tx_audio_success_callback(void *conn, void *arg);
static void conn_rx_data_success_callback(void *conn, void *arg);
static void back_channel_audio_rx_complete_callback(void);
static void audio_process_back_channel_callback(void);
/* Callbacks that are used for data and pairing processes. */
static void data_callback(void);
static void pairing_process_callback(void);
static void pairing_button_callback(void);
static void volume_up(void) __attribute__((unused));
static void volume_down(void) __attribute__((unused));
static void change_fallback_state(void);
#if LATCH_TEST_HOOKS
/* Unreferenced while STANDBY_TEST_HOOKS owns USER_3; kept so setting that flag to 0 hands
 * the button back to LATCH_TEST_HOOKS unchanged. */
static void dbg_soft_reset(void) __attribute__((unused));
static void dbg_radio_por(void);
#endif

/* **** Processing Stages **** */
/* Processing stages that are used for the main channel. */
static void app_audio_core_fallback_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_upsampling_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_mute_packet_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_unpacking_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_decompressing_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_volume_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_mute_on_underflow_interface_init(sac_processing_interface_t *iface);
/* Processing stages that are used for the back channel. */
static void app_audio_core_packing_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_downsampling_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_compressing_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_compression_discard_interface_init(sac_processing_interface_t *iface);

/* Outcome of a boot auto-reconnect attempt. */
typedef enum {
    BOOT_RECONNECT_OK,   /* The stored link was re-established; stay paired and stream. */
    BOOT_RECONNECT_PAIR, /* No usable record, or the user asked to pair mid-attempt: enter pairing. */
    BOOT_RECONNECT_IDLE, /* Had a record but the peer was unreachable: stay idle and let the SoC
                            re-drive a reconnect (AT+UWB_CONNECT) rather than silently re-pairing. */
} boot_reconnect_result_t;

/* **** Button Actions **** */
static void enter_pairing_mode(void);
static void unpair_device(void);
static void abort_pairing_procedure(void);
static boot_reconnect_result_t try_boot_reconnect(void);

/* **** Fallback LED and Terminal Display **** */
static bool should_print_stats(void);
static void print_stats(void);
#if LINK_WATCH
static void link_watch(void);
#endif

static void wireless_send_data(void *transmitted_data, uint8_t size, swc_error_t *swc_err);
static uint16_t wireless_read_data(void *received_data, uint8_t size, swc_error_t *swc_err);
static void fatal_trap(const char *tag, int code);
#if STALL_AUTO_RECOVER_MS
static void stall_auto_recover(void);
#endif
static uint32_t get_accumulator_size(sac_pipeline_t *pipeline);

/* **** AT Command Core Callbacks **** */
static void at_start_pairing(void);
static void at_start_connect(void);
static void at_start_disconnect(void);
static void at_start_shutdown(void);
static void app_teardown(void);
static bool at_get_link_status(void);
static int32_t at_get_link_margin(void);
static void emit_crash_dump(void);
static void at_play(void);
static void at_stop(void);
static void at_set_vol(uint8_t vol);
static void at_next_track(void);
static void at_pre_track(void);

/* PUBLIC FUNCTIONS ***********************************************************/
int main(void)
{
#if USB_AUDIO_ENABLED
    /* Configure usb audio before board initialization. */
    facade_configure_node_usb_audio();
#endif
    /* Initialize the board and all GPIOs and peripherals for minimal operations. */
    facade_board_init();
    facade_battery_init();
    s_battery_pct = facade_read_battery_level_pct();

    /* Initialize wireless core context switch handler before pairing is available */
    facade_set_context_switch_handler(swc_connection_callbacks_processing_handler);

    facade_button_callbacks_t button_callbacks = {
        .pairing_callback = pairing_button_callback,
#if STANDBY_TEST_HOOKS
        /* USER_3 drives the Standby power-down, because the AT-UART RX pad is unusable on
         * this board and AT+UWB_DISCONNECT is otherwise unreachable here. Takes precedence
         * over LATCH_TEST_HOOKS, which claims the same button for dbg_soft_reset() -- that
         * is a plain facade_system_reset(), i.e. reboot-and-reconnect, which looks very
         * similar from the outside and is easy to mistake for a failed sleep. */
        .volume_up_callback = at_start_disconnect,
        .volume_down_callback = dbg_radio_por,
#elif LATCH_TEST_HOOKS
        /* Volume buttons repurposed as latch-recovery tests (see LATCH_TEST_HOOKS). */
        .volume_up_callback = dbg_soft_reset,
        .volume_down_callback = dbg_radio_por,
#else
        .volume_up_callback = volume_up,
        .volume_down_callback = volume_down,
#endif
        .fallback_callback = change_fallback_state,
    };
    facade_set_button_callbacks(button_callbacks);

    at_cmd_core_init();
    at_cmd_core_set_device_role(AT_DEVICE_ROLE_NODE);
    at_cmd_core_register_pair_cb(at_start_pairing);
    at_cmd_core_register_connect_cb(at_start_connect);
    at_cmd_core_register_disconnect_cb(at_start_disconnect);
    at_cmd_core_register_shutdown_cb(at_start_shutdown);
    at_cmd_core_register_link_status_cb(at_get_link_status);
    at_cmd_core_register_link_margin_cb(at_get_link_margin);
    at_cmd_core_register_play_cb(at_play);
    at_cmd_core_register_stop_cb(at_stop);
    at_cmd_core_register_vol_cb(at_set_vol);
    at_cmd_core_register_next_track_cb(at_next_track);
    at_cmd_core_register_pre_track_cb(at_pre_track);
    at_cmd_core_register_battery_cb(facade_read_battery_level_pct);
    at_cmd_core_register_i2s_mux_cb(facade_set_i2s_mux);
    /* Boot banner, ahead of UWB_READY. Same timestamp as the crash-dump build line (both
     * expand AT_CMD_CORE_BUILD_ID / __DATE__ from this translation unit). It matters more
     * here than on the DG: a reconnect timeout puts the HS into Standby, so the host sees a
     * BUILD line only when something actually reset the MCU. */
    at_cmd_core_notify_build(AT_CMD_CORE_BUILD_ID);
    at_cmd_core_notify_uwb_ready();

    /* Audio process timer initialization. */
    facade_audio_process_main_channel_timer_init(audio_process_main_channel_callback);
    facade_audio_process_back_channel_timer_init(audio_process_back_channel_callback);

    /* Timer that updates statistics display every second and transmits button state to the Node every 10 ms. */
    facade_data_timer_init(DATA_TX_PERIOD_MS);
    facade_data_timer_set_callback(data_callback);

    certification_mode = facade_node_get_certification_mode();
    if (certification_mode != FACADE_CERTIF_NONE) {
        /* Init app in certification mode. */
        app_init();
        device_pairing_state = DEVICE_PAIRED;
        while (1) {
            /* Certification heartbeat: blink LED_USER_1 at ~1 Hz so it is visually
             * obvious the board is running in certification mode. Non-blocking (unlike
             * the button-selection blink), so it never stalls stats/link_watch or TX. */
            {
                static uint32_t cert_led_tick;
                uint32_t now = facade_get_tick_ms();
                if ((now - cert_led_tick) >= CERTIF_LED_TOGGLE_MS) {
                    cert_led_tick = now;
                    facade_certification_led_toggle();
                }
            }
            /* Statistics are displayed at intervals set by the timer when paired; timer stops if unpaired. */
            if (should_print_stats()) {
                print_stats();
            }
#if LINK_WATCH
            link_watch();
#endif
        }
    }

    device_pairing_state = DEVICE_UNPAIRED;

    /* Boot auto-reconnect: if a previous pairing was persisted to flash, try to
     * re-establish it silently. Only enter pairing when there is no usable record
     * (never paired) or the user asked to pair mid-attempt. A record that simply
     * could not reach its peer within RECONNECT_TIMEOUT_MS does NOT re-pair:
     * try_boot_reconnect() powers the module down into Standby (it does not return),
     * and a reset (SoC NRST / a WKUP button) wakes it back through here to retry. */
    if (try_boot_reconnect() == BOOT_RECONNECT_PAIR) {
        enter_pairing_mode();
    }

    while (1) {
        facade_button_handling();
        at_cmd_core_process();

        /* Read battery level every 10 seconds. */
        {
            static uint32_t battery_tick_start = 0;
            uint32_t now = facade_get_tick_ms();
            if ((now - battery_tick_start) >= 10000) {
                battery_tick_start = now;
                s_battery_pct = facade_read_battery_level_pct();
            }
        }

        /* Statistics are displayed at intervals set by the timer when paired; timer stops if unpaired. */
        if (should_print_stats()) {
            print_stats();
        }

#if LINK_WATCH
        link_watch();
#endif

#if CRASH_DUMP_PERIODIC_MS
        /* Periodic consolidated snapshot (link state + HW counters + last HardFault),
         * auto-emitted every CRASH_DUMP_PERIODIC_MS because the AT-UART RX is unusable here. */
        {
            static uint32_t crash_dump_tick_start = 0;
            uint32_t cd_now = facade_get_tick_ms();
            if ((cd_now - crash_dump_tick_start) >= CRASH_DUMP_PERIODIC_MS) {
                crash_dump_tick_start = cd_now;
                emit_crash_dump();
            }
        }
#endif

#if STALL_AUTO_RECOVER_MS
        stall_auto_recover();
#endif

        /* Wait for an interrupt event. */
        facade_wait_for_interrupt();
    }

    return 0;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize the Wireless Core.
 *
 *  @param[in]  app_pairing  Addresses received from the pairing process.
 *  @param[out] swc_err      Wireless Core error code.
 */
static void app_swc_core_init(pairing_assigned_address_t *app_pairing, swc_error_t *swc_err)
{
    uint16_t local_address = app_pairing->node_address;
    uint16_t remote_address = app_pairing->coordinator_address;
    uint8_t fallback_thresholds[] = {BACK_CHANNEL_FALLBACK_PAYLOAD_SIZE};
    uint8_t fallback_cca_try_count[] = {SWC_CCA_AUDIO_FBK_TRY_COUNT};

    if (certification_mode != FACADE_CERTIF_NONE) {
        app_pairing->coordinator_address = 0x1;
        app_pairing->node_address = 0x2;
        app_pairing->pan_id = 0xABC;
        remote_address = 0x2;
        local_address = 0x1;
    }

    /* Initialize Wireless Core. */
    swc_cfg_t core_cfg = {
        .timeslot_sequence = timeslot_us,
        .timeslot_sequence_length = ARRAY_SIZE(timeslot_us),
        .channel_sequence = channel_sequence,
        .channel_sequence_length = ARRAY_SIZE(channel_sequence),
        .concurrency_mode = SWC_CONCURRENCY_MODE_HIGH_PERFORMANCE,
        .memory_pool = swc_memory_pool,
        .memory_pool_size = SWC_MEM_POOL_SIZE,
    };

    /* Initialize Node. */
    swc_node_cfg_t node_cfg = {
        .role = SWC_ROLE_NODE,
        .pan_id = app_pairing->pan_id,
        .coordinator_address = remote_address,
        .local_address = local_address,
    };

    swc_init(core_cfg, node_cfg, facade_context_switch_trigger, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Initialize radio. */
    swc_radio_module_init(SWC_RADIO_ID_1, true, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

#if (SWC_RADIO_COUNT == 2)
    swc_radio_module_init(SWC_RADIO_ID_2, true, swc_err);
    ASSERT_SWC_STATUS(*swc_err);
#endif

    /* **** RX Connections **** */
    /* ** Main Channel: RX Audio Connection ** */
    swc_connection_cfg_t rx_audio_conn_cfg = {
        .name = "RX Audio Connection",
        .source_address = remote_address,
        .destination_address = local_address,
        .max_payload_size = MAIN_CHANNEL_SWC_PAYLOAD_SIZE + sizeof(sac_header_t),
        .queue_size = SWC_QUEUE_SIZE,
        .timeslot_id = rx_timeslots,
        .timeslot_count = ARRAY_SIZE(rx_timeslots),
    };

    rx_audio_conn = swc_connection_init(rx_audio_conn_cfg, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    swc_connection_set_modulation(rx_audio_conn, SWC_MOD_IOOK, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    swc_connection_set_fec_ratio(rx_audio_conn, SWC_FEC_1_2_5_0, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Audio connection concurrency settings. */
    swc_connection_concurrency_cfg_t rx_audio_concurrency_cfg = {
        .enabled = true,
        .try_count = MAIN_CHANNEL_SWC_CCA_FB_TRY_COUNT, /* Use maximum CCA try count on this connection. */
        .retry_time = MAIN_CHANNEL_SWC_CCA_AUDIO_RETRY_TIME,
        .fail_action = SWC_CCA_ABORT_TX,
    };

    swc_connection_set_concurrency_cfg(rx_audio_conn, &rx_audio_concurrency_cfg, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Audio connection RF channels settings. */
    swc_channel_cfg_t rx_audio_channel_cfg = {
        .tx_pulse_count = SR1100_PULSE_COUNT,
        .rx_pulse_count = SR1100_PULSE_COUNT,
    };
    uint8_t tx_ack_pulse_width[] = TX_ACK_PULSE_WIDTH;
    uint8_t tx_ack_pulse_gain[] = TX_ACK_PULSE_GAIN;

    for (uint8_t i = 0; i < ARRAY_SIZE(channel_frequency); i++) {
        rx_audio_channel_cfg.frequency = channel_frequency[i];
        rx_audio_channel_cfg.tx_pulse_width = tx_ack_pulse_width[i];
        rx_audio_channel_cfg.tx_pulse_gain = tx_ack_pulse_gain[i];
        swc_connection_add_channel(rx_audio_conn, rx_audio_channel_cfg, swc_err);
        ASSERT_SWC_STATUS(*swc_err);
    }

    /* Audio connection priority settings. */
    swc_connection_set_connection_priority(rx_audio_conn, AUDIO_CONNECTION_PRIORITY, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Audio connection callback settings. */
    swc_connection_set_rx_success_callback(rx_audio_conn, conn_rx_audio_success_callback, NULL, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* ** RX Data Connection ** */
    swc_connection_cfg_t rx_data_conn_cfg = {
        .name = "RX Data Connection",
        .source_address = remote_address,
        .destination_address = local_address,
        .max_payload_size = MAX_DATA_PAYLOAD_SIZE,
        .queue_size = SWC_QUEUE_SIZE,
        .timeslot_id = rx_timeslots,
        .timeslot_count = ARRAY_SIZE(rx_timeslots),
    };

    rx_data_conn = swc_connection_init(rx_data_conn_cfg, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    swc_connection_set_modulation(rx_data_conn, SWC_MOD_IOOK, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    swc_connection_set_fec_ratio(rx_data_conn, SWC_FEC_1_2_5_0, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Data connection concurrency settings. */
    swc_connection_concurrency_cfg_t rx_data_concurrency_cfg = {
        .enabled = true,
        .try_count = MAIN_CHANNEL_SWC_CCA_DATA_TRY_COUNT,
        .retry_time = MAIN_CHANNEL_SWC_CCA_DATA_RETRY_TIME,
        .fail_action = SWC_CCA_ABORT_TX,
    };

    swc_connection_set_concurrency_cfg(rx_data_conn, &rx_data_concurrency_cfg, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Data connection RF channels settings. */
    swc_channel_cfg_t rx_data_channel_cfg = {
        .tx_pulse_count = SR1100_PULSE_COUNT,
        .rx_pulse_count = SR1100_PULSE_COUNT,
    };
    uint8_t tx_data_ack_pulse_width[] = TX_DATA_ACK_PULSE_WIDTH;
    uint8_t tx_data_ack_pulse_gain[] = TX_DATA_ACK_PULSE_GAIN;

    for (uint8_t i = 0; i < ARRAY_SIZE(channel_frequency); i++) {
        rx_data_channel_cfg.frequency = channel_frequency[i];
        rx_data_channel_cfg.tx_pulse_width = tx_data_ack_pulse_width[i];
        rx_data_channel_cfg.tx_pulse_gain = tx_data_ack_pulse_gain[i];
        swc_connection_add_channel(rx_data_conn, rx_data_channel_cfg, swc_err);
        ASSERT_SWC_STATUS(*swc_err);
    }

    /* Data connection priority settings. */
    swc_connection_set_connection_priority(rx_data_conn, DATA_CONNECTION_PRIORITY, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Data connection callback settings. */
    swc_connection_set_rx_success_callback(rx_data_conn, conn_rx_data_success_callback, NULL, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* **** TX Connections **** */
    /* ** Back Channel: TX Audio Connection ** */
    swc_connection_cfg_t tx_audio_conn_cfg = {
        .name = "TX Audio Connection",
        .source_address = local_address,
        .destination_address = remote_address,
        .max_payload_size = BACK_CHANNEL_SWC_PAYLOAD_SIZE + sizeof(sac_header_t),
        .queue_size = SWC_QUEUE_SIZE + (USB_AUDIO_ENABLED ? BACK_CHANNEL_USB_FS_PRODUCER_BUFFERING : 0),
        .timeslot_id = tx_timeslots,
        .timeslot_count = ARRAY_SIZE(tx_timeslots),
    };

    /* ** TX Data Connection ** */
    swc_connection_cfg_t tx_data_conn_cfg = {
        .name = "TX Data Connection",
        .source_address = local_address,
        .destination_address = remote_address,
        .max_payload_size = MAX_DATA_PAYLOAD_SIZE,
        .queue_size = SWC_QUEUE_SIZE,
        .timeslot_id = tx_timeslots,
        .timeslot_count = ARRAY_SIZE(tx_timeslots),
    };

    if (certification_mode == FACADE_CERTIF_DATA) {
        /* Add data connection first to use it for certification mode. */
        tx_data_conn = swc_connection_init(tx_data_conn_cfg, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

        swc_connection_set_modulation(tx_data_conn, SWC_MOD_IOOK, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

        swc_connection_set_fec_ratio(tx_data_conn, SWC_FEC_1_2_5_0, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

        swc_connection_set_connection_priority(tx_data_conn, AUDIO_CONNECTION_PRIORITY, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

        tx_audio_conn = swc_connection_init(tx_audio_conn_cfg, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

        swc_connection_set_modulation(tx_audio_conn, SWC_MOD_IOOK, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

        swc_connection_set_fec_ratio(tx_audio_conn, SWC_FEC_1_2_5_0, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

        swc_connection_set_connection_priority(tx_audio_conn, DATA_CONNECTION_PRIORITY, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

    } else {
        if (certification_mode == FACADE_CERTIF_AUDIO_48k_ADPCM) {
            /* Change the connection's max payload size when certifying compressed audio. */
            tx_audio_conn_cfg.max_payload_size = fallback_thresholds[0];
        }
        tx_audio_conn = swc_connection_init(tx_audio_conn_cfg, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

        swc_connection_set_modulation(tx_audio_conn, SWC_MOD_IOOK, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

        swc_connection_set_fec_ratio(tx_audio_conn, SWC_FEC_1_2_5_0, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

        swc_connection_set_connection_priority(tx_audio_conn, AUDIO_CONNECTION_PRIORITY, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

        tx_data_conn = swc_connection_init(tx_data_conn_cfg, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

        swc_connection_set_modulation(tx_data_conn, SWC_MOD_IOOK, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

        swc_connection_set_fec_ratio(tx_data_conn, SWC_FEC_1_2_5_0, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

        swc_connection_set_connection_priority(tx_data_conn, DATA_CONNECTION_PRIORITY, swc_err);
        ASSERT_SWC_STATUS(*swc_err);
    }

    /* Audio connection concurrency settings. */
    swc_connection_concurrency_cfg_t tx_audio_concurrency_cfg = {
        .enabled = true,
        .try_count = BACK_CHANNEL_SWC_CCA_AUDIO_TRY_COUNT,
        .retry_time = BACK_CHANNEL_SWC_CCA_AUDIO_RETRY_TIME,
        .fail_action = SWC_CCA_ABORT_TX,
    };

    swc_connection_set_concurrency_cfg(tx_audio_conn, &tx_audio_concurrency_cfg, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Audio connection fallback settings. */
    swc_connection_fallback_cfg_t fallback_cfg = {
        .enabled = true,
        .fallback_mode_count = 1,
        .thresholds = fallback_thresholds,
        .cca_try_count = fallback_cca_try_count,
    };

    swc_connection_set_fallback_cfg(tx_audio_conn, &fallback_cfg, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Audio connection RF channels settings. */
    swc_channel_cfg_t tx_audio_channel_cfg = {
        .tx_pulse_count = SR1100_PULSE_COUNT,
        .rx_pulse_count = SR1100_PULSE_COUNT,
    };
    uint8_t tx_audio_pulse_width[] = TX_AUDIO_PULSE_WIDTH;
    uint8_t tx_audio_pulse_gain[] = TX_AUDIO_PULSE_GAIN;

    swc_fallback_channel_cfg_t tx_audio_fallback_channel_cfg = {
        .tx_pulse_count = SR1100_PULSE_COUNT,
    };
    uint8_t tx_audio_fb_pulse_width[] = TX_AUDIO_FB_PULSE_WIDTH;
    uint8_t tx_audio_fb_pulse_gain[] = TX_AUDIO_FB_PULSE_GAIN;

    for (uint8_t i = 0; i < ARRAY_SIZE(channel_frequency); i++) {
        tx_audio_channel_cfg.frequency = channel_frequency[i];
        tx_audio_channel_cfg.tx_pulse_width = tx_audio_pulse_width[i];
        tx_audio_channel_cfg.tx_pulse_gain = tx_audio_pulse_gain[i];
        swc_connection_add_channel(tx_audio_conn, tx_audio_channel_cfg, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

        tx_audio_fallback_channel_cfg.tx_pulse_width = tx_audio_fb_pulse_width[i];
        tx_audio_fallback_channel_cfg.tx_pulse_gain = tx_audio_fb_pulse_gain[i];
        swc_connection_add_fallback_channel(tx_audio_conn, tx_audio_channel_cfg, tx_audio_fallback_channel_cfg, i,
                                            FALLBACK_INDEX_0, swc_err);
        ASSERT_SWC_STATUS(*swc_err);
    }

    /* Audio connection callback settings. */
    swc_connection_set_tx_success_callback(tx_audio_conn, conn_tx_audio_success_callback, NULL, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Data connection concurrency settings. */
    swc_connection_concurrency_cfg_t tx_data_concurrency_cfg = {
        .enabled = true,
        .try_count = BACK_CHANNEL_SWC_CCA_DATA_TRY_COUNT,
        .retry_time = BACK_CHANNEL_SWC_CCA_DATA_RETRY_TIME,
        .fail_action = SWC_CCA_ABORT_TX,
    };

    swc_connection_set_concurrency_cfg(tx_data_conn, &tx_data_concurrency_cfg, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Data connection RF channels settings. */
    swc_channel_cfg_t tx_data_channel_cfg = {
        .tx_pulse_count = SR1100_PULSE_COUNT,
        .rx_pulse_count = SR1100_PULSE_COUNT,
    };
    uint8_t tx_data_pulse_width[] = TX_DATA_PULSE_WIDTH;
    uint8_t tx_data_pulse_gain[] = TX_DATA_PULSE_GAIN;

    for (uint8_t i = 0; i < ARRAY_SIZE(channel_frequency); i++) {
        tx_data_channel_cfg.frequency = channel_frequency[i];
        tx_data_channel_cfg.tx_pulse_width = tx_data_pulse_width[i];
        tx_data_channel_cfg.tx_pulse_gain = tx_data_pulse_gain[i];
        swc_connection_add_channel(tx_data_conn, tx_data_channel_cfg, swc_err);
        ASSERT_SWC_STATUS(*swc_err);
    }

    /* Data connection priority settings. */
    swc_connection_set_connection_priority(tx_data_conn, DATA_CONNECTION_PRIORITY, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Data connection callback settings. */
    swc_connection_set_tx_success_callback(tx_data_conn, conn_tx_data_success_callback, NULL, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Handle certification mode. */
    swc_set_certification_mode(certification_mode != FACADE_CERTIF_NONE, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Setup Wireless Core. */
    swc_setup(swc_err);
    ASSERT_SWC_STATUS(*swc_err);
}

/** @brief Callback function when an audio frame has been successfully received.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 *  @param[in] arg   Additional argument for the callback function.
 */
static void conn_rx_audio_success_callback(void *conn, void *arg)
{
    sac_status_t sac_status = SAC_OK;

    (void)conn;
    (void)arg;

    facade_rx_audio_conn_status();

    /* The SWC produces audio samples upon receiving them from the Coordinator. */
    sac_pipeline_produce(main_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Trigger main channel process. */
    facade_audio_process_main_channel_timer_trigger();
    main_channel_trigger_count++;
}

/** @brief Callback function when a previously sent data frame has been ACK'd.
 *
 *  @note This function is empty, but can be filled by users in any way they see fit.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 *  @param[in] arg   Additional argument for the callback function.
 */
static void conn_tx_data_success_callback(void *conn, void *arg)
{
    (void)conn;
    (void)arg;
}

/** @brief Callback function when a data frame has been successfully received on data connection.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 *  @param[in] arg   Additional argument for the callback function.
 */
static void conn_rx_data_success_callback(void *conn, void *arg)
{
    sac_status_t sac_status = SAC_OK;
    swc_error_t swc_err = SWC_ERR_NONE;
    user_data_t received_user_data = {0};
    uint16_t read_data_size;

    (void)conn;
    (void)arg;

    /* Get received payload. */
    read_data_size = wireless_read_data(&received_user_data, sizeof(received_user_data), &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    if (read_data_size > 0) {
        /* Depending on the requested button state from the Node, the specified LED turns on or off. */
        if (received_user_data.button_state == false) {
            facade_empty_payload_received_status();
        } else {
            facade_payload_received_status();
        }

        /* The fallback state is updated. */
        sac_fallback_set_rx_link_margin(&back_channel_fallback_instance, received_user_data.link_margin, &sac_status);
        ASSERT_SAC_STATUS(sac_status);
    }
}

/** @brief Callback function when a previously sent audio frame has been ACK'd.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 *  @param[in] arg   Additional argument for the callback function.
 */
static void conn_tx_audio_success_callback(void *conn, void *arg)
{
    (void)conn;
    (void)arg;

    facade_tx_audio_conn_status();

    /* Trigger back channel process. */
    facade_audio_process_back_channel_timer_trigger();
}

/** @brief Initialize the Audio Core.
 */
static void app_audio_core_init(void)
{
    sac_status_t sac_status = SAC_OK;

    /* ** Endpoint Interfaces ** */
    sac_endpoint_interface_t main_channel_consumer_iface = {0};
    sac_endpoint_interface_t main_channel_swc_producer_iface = {0};
    sac_endpoint_interface_t back_channel_producer_iface = {0};
    sac_endpoint_interface_t back_channel_swc_consumer_iface = {0};

    /* ** Processing Stages Interfaces ** */
    sac_processing_interface_t fallback_iface = {0};
    sac_processing_interface_t main_channel_upsampling_iface = {0};
    sac_processing_interface_t main_channel_mute_packet_iface = {0};
    sac_processing_interface_t main_channel_unpacking_iface = {0};
    sac_processing_interface_t main_channel_decompression_iface = {0};
    sac_processing_interface_t main_channel_volume_iface = {0};
    sac_processing_interface_t main_channel_mute_on_underflow_iface = {0};
    sac_processing_interface_t back_channel_packing_iface = {0};
    sac_processing_interface_t back_channel_compression_iface = {0};
    sac_processing_interface_t back_channel_compression_discard_iface = {0};
    sac_processing_interface_t back_channel_downsampling_iface = {0};

    sac_endpoint_swc_init(&main_channel_swc_producer_iface, &back_channel_swc_consumer_iface);
    sac_facade_audio_endpoint_init(&back_channel_producer_iface, &main_channel_consumer_iface);
    facade_set_audio_complete_callback(main_channel_audio_tx_complete_callback,
                                       back_channel_audio_rx_complete_callback);

    app_audio_core_fallback_interface_init(&fallback_iface);
    app_audio_core_upsampling_interface_init(&main_channel_upsampling_iface);
    app_audio_core_mute_packet_interface_init(&main_channel_mute_packet_iface);
    app_audio_core_unpacking_interface_init(&main_channel_unpacking_iface);
    app_audio_core_decompressing_interface_init(&main_channel_decompression_iface);
    app_audio_core_volume_interface_init(&main_channel_volume_iface);
    app_audio_core_mute_on_underflow_interface_init(&main_channel_mute_on_underflow_iface);

    app_audio_core_packing_interface_init(&back_channel_packing_iface);
    app_audio_core_compression_discard_interface_init(&back_channel_compression_discard_iface);
    app_audio_core_compressing_interface_init(&back_channel_compression_iface);
    app_audio_core_downsampling_interface_init(&back_channel_downsampling_iface);

    main_channel_swc_producer_instance.connection = rx_audio_conn;
    back_channel_swc_consumer_instance.connection = tx_audio_conn;

    /* Initialize Audio Core. */
    sac_cfg_t core_cfg = {
        .memory_pool = audio_memory_pool,
        .memory_pool_size = SAC_MEM_POOL_SIZE,
    };

    sac_init(core_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /*
     * Main Channel Audio Pipeline (RX)
     * ================================
     *
     * ***** NORMAL MODE USB (Fallback mode 0) *****
     * Input:       Stereo stream of 96kHz/24-bit depth samples is received over the air from the Coordinator.
     * Processing:  Mute packet process.
     * Output:      Stereo stream of 96kHz/24-bit.
     *
     * +-----+    +-------------+    +-----+
     * | SWC | -> | Mute Packet | -> | USB |
     * +-----+    +-------------+    +-----+
     *
     ***** NORMAL MODE I2S (Fallback mode 0) *****
     * Input:       Stereo stream of 96kHz/24-bit depth samples is received over the air from the Coordinator.
     * Processing:  Mute packet process.
     * Processing:  Unpacking from 24 bits to 24 bits encoded on 32 bits audio samples.
     * Processing:  Digital volume control followed by clock drift compensation and mute on glitch.
     * Output:      Stereo stream of 96kHz/24-bit.
     *
     * +-----+    +-------------+    +-----------+    +----------------+    +-----+    +----------------+    +-----+
     * | SWC | -> | Mute Packet | -> | Unpacking | -> | Digital Volume | -> | CDC | -> | Mute on Glitch | -> | I2S |
     * +-----+    +-------------+    +-----------+    +----------------+    +-----+    +----------------+    +-----+
     *
     * ***** FALLBACK MODE USB (Fallback mode 1) *****
     * Input:       Stereo stream of 48kHz/24-bit depth samples is received over the air from the Coordinator.
     * Processing:  Mute packet process.
     * Processing:  Audio sample accumulator 1.7x.
     * Output:      Stereo stream of 48kHz/24-bit.
     *
     * +-----+    +-------------+    +------------------+    +-----+
     * | SWC | -> | Mute Packet | -> | Accumulator 1.7x | -> | USB |
     * +-----+    +-------------+    +------------------+    +-----+
     *
     ***** FALLBACK MODE I2S (Fallback mode 1) *****
     * Input:       Stereo stream of 48kHz/24-bit depth samples is received over the air from the Coordinator.
     * Processing:  Mute packet process.
     * Processing:  Unpacking from 24 bits to 24 bits encoded on 32 bits audio samples.
     * Processing:  Upsampling audio samples from 48kHz to 96kHz.
     * Processing:  Audio sample accumulator 1.7x.
     * Processing:  Digital volume control followed by clock drift compensation and mute on glitch.
     * Output:      Stereo stream of 96kHz/24-bit.
     *
     * +-----+    +-------------+    +-----------+    +---------------+    +------------------+    +----------------+
     * | SWC | -> | Mute Packet | -> | Unpacking | -> | Upsampling 2x | -> | Accumulator 1.7x | -> | Digital Volume | --
     * +-----+    +-------------+    +-----------+    +---------------+    +------------------+    +----------------+  |
     *       -----------------------------------------------------------------------------------------------------------
     *       |    +-----+    +----------------+    +-----+
     *       ---> | CDC | -> | Mute on glitch | -> | I2S |
     *            +-----+    +----------------+    +-----+
     *
     ***** FALLBACK MODE USB (Fallback mode 2) *****
     * Input:       Stereo stream of 48kHz/16-bit depth samples is received over the air from the Coordinator.
     * Processing:  Mute packet process.
     * Processing:  Unpacking from 16 bits to 24 bits.
     * Processing:  Audio sample accumulator 1.7x.
     * Output:      Stereo stream of 48kHz/24-bit.
     *
     * +-----+    +-------------+    +-----------+    +------------------+    +-----+
     * | SWC | -> | Mute Packet | -> | Unpacking | -> | Accumulator 1.7x | -> | USB |
     * +-----+    +-------------+    +-----------+    +------------------+    +-----+
     *
     ***** FALLBACK MODE I2S (Fallback mode 2) *****
     * Input:       Stereo stream of 48kHz/16-bit depth samples is received over the air from the Coordinator.
     * Processing:  Mute packet process.
     * Processing:  Unpacking from 16 bits to 24 bits encoded on 32 bits audio samples.
     * Processing:  Upsampling audio samples from 48kHz to 96kHz.
     * Processing:  Audio sample accumulator 1.7x.
     * Processing:  Digital volume control followed by clock drift compensation and mute on glitch.
     * Output:      Stereo stream of 96kHz/24-bit.
     *
     * +-----+    +-------------+    +-----------+    +---------------+    +------------------+    +----------------+
     * | SWC | -> | Mute Packet | -> | Unpacking | -> | Upsampling 2x | -> | Accumulator 1.7x | -> | Digital Volume | --
     * +-----+    +-------------+    +-----------+    +---------------+    +------------------+    +----------------+  |
     *       -----------------------------------------------------------------------------------------------------------
     *       |    +-----+    +----------------+    +-----+
     *       ---> | CDC | -> | Mute on glitch | -> | I2S |
     *            +-----+    +----------------+    +-----+
     *
     ***** FALLBACK MODE USB (Fallback mode 3) *****
     * Input:       Stereo stream of 48kHz/24-bit depth samples is received over the air from the Coordinator.
     * Processing:  Mute packet process.
     * Processing:  Decompression of samples compressed with ADPCM.
     * Processing:  Audio sample accumulator 2.3x.
     * Output:      Stereo stream of 48kHz/24-bit.
     *
     * +-----+    +-------------+    +---------------------+    +------------------+    +-----+
     * | SWC | -> | Mute Packet | -> | ADPCM Decompression | -> | Accumulator 2.3x | -> | USB |
     * +-----+    +-------------+    +---------------------+    +------------------+    +-----+
     *
     ***** FALLBACK MODE I2S (Fallback mode 3) *****
     * Input:       Stereo stream of 48kHz/24-bit depth samples is received over the air from the Coordinator.
     * Processing:  Mute packet process.
     * Processing:  Decompression of samples compressed with ADPCM.
     * Processing:  Upsampling audio samples from 48kHz to 96kHz.
     * Processing:  Audio sample accumulator 2.3x.
     * Processing:  Digital volume control followed by clock drift compensation and mute on glitch.
     * Output:      Stereo stream of 96kHz/24-bit.
     *
     * +-----+    +-------------+    +---------------------+    +---------------+    +------------------+
     * | SWC | -> | Mute Packet | -> | ADPCM Decompression | -> | Upsampling 2x | -> | Accumulator 2.3x | ---
     * +-----+    +-------------+    +---------------------+    +---------------+    +------------------+   |
     *       ------------------------------------------------------------------------------------------------
     *       |    +----------------+    +-----+    +----------------+    +-----+
     *       ---> | Digital Volume | -> | CDC | -> | Mute on glitch | -> | I2S |
     *            +----------------+    +-----+    +----------------+    +-----+
     */

    /* Initialize SWC producer endpoint. */
    sac_endpoint_cfg_t main_channel_swc_producer_cfg = {
        .use_encapsulation = true,
        .delayed_action = false,
        .channel_count = MAIN_CHANNEL_CHANNEL_COUNT,
        .audio_payload_size = MAIN_CHANNEL_SWC_PAYLOAD_SIZE,
        .queue_size = (MAIN_CHANNEL_LATENCY_QUEUE_SIZE * MAIN_CHANNEL_MAX_ACC_DIV) / MAIN_CHANNEL_MAX_ACC_MUL,
    };
    main_channel_swc_producer_endpoint = sac_endpoint_init((void *)&main_channel_swc_producer_instance,
                                                           "SWC EP (Producer)", main_channel_swc_producer_iface,
                                                           main_channel_swc_producer_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    main_channel_fallback_instance.connection = rx_audio_conn;
    main_channel_fallback_instance.is_tx_device = false;
    main_channel_fallback_processing = sac_processing_stage_init(&main_channel_fallback_instance,
                                                                 "Main channel fallback RX", fallback_iface,
                                                                 &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    main_channel_upsampling_instance.cfg.multiply_ratio = SAC_SRC_TWO;
    main_channel_upsampling_instance.cfg.divide_ratio = SAC_SRC_ONE;
    /* 16-bit packed audio */
    main_channel_upsampling_instance.cfg.payload_size = MAIN_CHANNEL_SWC_PAYLOAD_SIZE *
                                                        main_channel_upsampling_instance.cfg.divide_ratio /
                                                        main_channel_upsampling_instance.cfg.multiply_ratio;
    main_channel_upsampling_instance.cfg.payload_size =
        (main_channel_upsampling_instance.cfg.payload_size * MAIN_CHANNEL_MAX_ACC_MUL) / MAIN_CHANNEL_MAX_ACC_DIV;

    /* Upsampling does not change the sample format. */
    main_channel_upsampling_instance.cfg.input_sample_format = MAIN_CHANNEL_CONSUMER_SAC_SAMPLE_FORMAT;
    main_channel_upsampling_instance.cfg.output_sample_format = MAIN_CHANNEL_CONSUMER_SAC_SAMPLE_FORMAT;
    main_channel_upsampling_instance.cfg.channel_count = MAIN_CHANNEL_CHANNEL_COUNT;
    main_channel_upsampling_processing = sac_processing_stage_init((void *)&main_channel_upsampling_instance,
                                                                   "Audio Upsampling", main_channel_upsampling_iface,
                                                                   &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Mute packet processing stage initialization. */
    main_channel_mute_packet_instance.is_tx = false;
    main_channel_mute_packet_processing = sac_processing_stage_init((void *)&main_channel_mute_packet_instance,
                                                                    "Mute packet", main_channel_mute_packet_iface,
                                                                    &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Processing stage packs into 24 bits before sending if fallback is deactivated. */
    main_channel_unpacking_instance.packing_mode = SAC_UNPACK_24BITS;
    main_channel_unpacking_processing = sac_processing_stage_init((void *)&main_channel_unpacking_instance,
                                                                  "Audio Packing", main_channel_unpacking_iface,
                                                                  &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Processing stage that unpacks the received audio samples from 16 bits to 24 bits. */
#if (USB_AUDIO_ENABLED)
    main_channel_fbk_unpacking_instance.packing_mode = SAC_SCALE_16BITS_24BITS;
#else
    main_channel_fbk_unpacking_instance.packing_mode = SAC_UNPACK_24BITS_16BITS;
#endif
    main_channel_fbk_unpacking_processing = sac_processing_stage_init((void *)&main_channel_fbk_unpacking_instance,
                                                                      "Audio Unpacking", main_channel_unpacking_iface,
                                                                      &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Processing stage that decompresses audio samples if fallback is activated. */
    main_channel_decompression_instance.compression_mode = SAC_COMPRESSION_UNPACK_STEREO;
    main_channel_decompression_instance.sample_format = MAIN_CHANNEL_CONSUMER_SAC_SAMPLE_FORMAT;
    main_channel_decompression_processing = sac_processing_stage_init((void *)&main_channel_decompression_instance,
                                                                      "Audio Decompressing",
                                                                      main_channel_decompression_iface, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Audio sample accumulator processing stage initialization. */
    sac_processing_interface_t main_channel_sample_accumulator_iface = {
        .init = sac_sample_accumulator_init,
        .process = sac_sample_accumulator_process,
    };
    /* Make consumer packets always the same size. */
    main_channel_sample_accumulator_instance.max_accumulator_size = USB_AUDIO_ENABLED ? MAIN_CHANNEL_SWC_PAYLOAD_SIZE :
                                                                                        MAIN_CHANNEL_I2S_PAYLOAD_SIZE;
    main_channel_sample_accumulator_processing =
        sac_processing_stage_init((void *)&main_channel_sample_accumulator_instance, "Audio Sample Accumulator",
                                  main_channel_sample_accumulator_iface, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Processing stage that handles the volume control. */
    main_channel_volume_instance.initial_volume_level = 100;
    main_channel_volume_instance.sample_format = MAIN_CHANNEL_CONSUMER_SAC_SAMPLE_FORMAT;
    main_channel_volume_processing = sac_processing_stage_init((void *)&main_channel_volume_instance,
                                                               "Digital Volume Control", main_channel_volume_iface,
                                                               &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Processing stage that compensates the clock drift. */
    main_channel_cdc_processing = sac_facade_cdc_processing_init(MAIN_CHANNEL_CONSUMER_SAC_SAMPLE_FORMAT, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Processing stage that handles the mute packet. */
    main_channel_mute_on_underflow_instance.reload_value =
        sac_get_nb_packets_in_x_ms(30, MAIN_CHANNEL_I2S_PAYLOAD_SIZE, MAIN_CHANNEL_CHANNEL_COUNT,
                                   MAIN_CHANNEL_CONSUMER_SAC_SAMPLE_FORMAT, I2S_SAMPLE_RATE_HZ);

    main_channel_mute_on_underflow_processing =
        sac_processing_stage_init((void *)&main_channel_mute_on_underflow_instance, "Mute on underflow",
                                  main_channel_mute_on_underflow_iface, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Audio consumer endpoint initialization. */
    sac_endpoint_cfg_t dummy_consumer_cfg = {
        .use_encapsulation = false,
        .delayed_action = false,
        .channel_count = MAIN_CHANNEL_CHANNEL_COUNT,
        .audio_payload_size = ((USB_AUDIO_ENABLED ? MAIN_CHANNEL_SWC_PAYLOAD_SIZE : MAIN_CHANNEL_I2S_PAYLOAD_SIZE) *
                               MAIN_CHANNEL_MAX_ACC_MUL) /
                              MAIN_CHANNEL_MAX_ACC_DIV,
        .queue_size = (MAIN_CHANNEL_MAX_ACC_MUL / MAIN_CHANNEL_MAX_ACC_DIV) + 1,
    };
    sac_endpoint_interface_t dummy_iface = {
        .action = ep_dummy_consume,
        .start = ep_dummy_start,
        .stop = ep_dummy_stop,
    };
    dummy_audio_consumer = sac_endpoint_init(NULL, "Audio EP (Consumer1)", dummy_iface, dummy_consumer_cfg,
                                             &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Audio pipeline initialization. */
    sac_pipeline_cfg_t main_channel_pipeline_cfg = {
        .do_initial_buffering = false,
    };
    main_channel_sac_pipeline = sac_pipeline_init("SWC -> Accumulator", main_channel_swc_producer_endpoint,
                                                  main_channel_pipeline_cfg, dummy_audio_consumer, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Add processing stage to the audio pipeline. */
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_mute_packet_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_fallback_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    /* Decompress. */
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_decompression_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    /* Unpack to 24-bit */
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_fbk_unpacking_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#if !USB_AUDIO_ENABLED
    /* Unpack to 32-bit for I2S DMA transfer. */
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_unpacking_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif
    /* SRC. */
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_upsampling_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Audio pipeline setup. */
    sac_pipeline_setup(main_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Fallback mode configuration. */
    sac_fallback_mode_cfg_t mode_cfg = sac_fallback_mode_get_defaults();
    uint8_t mode_index;

    /* Fallback mode 0 configuration. */
    mode_cfg.sample_count = MAIN_CHANNEL_SAMPLE_COUNT;
    mode_index = sac_fallback_add_mode(&main_channel_fallback_instance, "96kHz 24-bit", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#if !USB_AUDIO_ENABLED
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_unpacking_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif
    /* Fallback mode 1 configuration. */
    mode_cfg.sample_count = MAIN_CHANNEL_FBK_1_SAMPLE_COUNT;
    mode_index = sac_fallback_add_mode(&main_channel_fallback_instance, "48kHz 24-bit", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#if !USB_AUDIO_ENABLED
    /* When using I2S, packing is required to convert 24-bit audio aligned on 32-bit words. */
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_unpacking_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_upsampling_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Fallback mode 2 configuration. */
    mode_cfg.sample_count = MAIN_CHANNEL_FBK_2_SAMPLE_COUNT;
    mode_index = sac_fallback_add_mode(&main_channel_fallback_instance, "48kHz 16-bit", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_upsampling_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_fbk_unpacking_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Fallback mode 3 configuration. */
    mode_cfg.sample_count = MAIN_CHANNEL_FBK_3_SAMPLE_COUNT;
    mode_index = sac_fallback_add_mode(&main_channel_fallback_instance, "48kHz ADPCM", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_upsampling_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_decompression_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /** Start fallback in best quality.
     *
     *  When using USB dongle, fallback state will not update until USB audio playback starts. To avoid the user
     *  thinking the audio quality is bad on startup, the fallback is initialized to its best quality.
     */
    sac_fallback_set_current_mode(&main_channel_fallback_instance, 0, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    dummy_audio_producer = sac_endpoint_init(NULL, "ACC EP (Producer)", dummy_iface, dummy_consumer_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    sac_endpoint_cfg_t audio_consumer_cfg = {
        .use_encapsulation = false,
        .delayed_action = !USB_AUDIO_ENABLED,
        .channel_count = MAIN_CHANNEL_CHANNEL_COUNT,
        .audio_payload_size = USB_AUDIO_ENABLED ? MAIN_CHANNEL_SWC_PAYLOAD_SIZE : MAIN_CHANNEL_I2S_PAYLOAD_SIZE,
        .queue_size = MAIN_CHANNEL_LATENCY_QUEUE_SIZE,
    };
    main_channel_consumer_endpoint = sac_endpoint_init(NULL, "Audio EP (Consumer)", main_channel_consumer_iface,
                                                       audio_consumer_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    sac_pipeline_cfg_t main_channel_accumulator_pipeline_cfg = {
        .do_initial_buffering = false,
    };
    main_channel_accumulator_pipeline = sac_pipeline_init("Accumulator -> Audio", dummy_audio_producer,
                                                          main_channel_accumulator_pipeline_cfg,
                                                          main_channel_consumer_endpoint, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    sac_endpoint_link(dummy_audio_consumer, dummy_audio_producer, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    sac_pipeline_add_processing(main_channel_accumulator_pipeline, main_channel_sample_accumulator_processing,
                                &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#if !USB_AUDIO_ENABLED
    /* 24-bit packed audio volume not supported. */
    sac_pipeline_add_processing(main_channel_accumulator_pipeline, main_channel_volume_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_add_processing(main_channel_accumulator_pipeline, main_channel_cdc_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_add_processing(main_channel_accumulator_pipeline, main_channel_mute_on_underflow_processing,
                                &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif

    /* Audio pipeline setup. */
    sac_pipeline_setup(main_channel_accumulator_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /*
     * Back Channel Audio Pipeline (TX)
     * ================================
     *
     ***** NORMAL MODE USB (Fallback mode 0) *****
     * Input:       Mono stream of 48kHz/16-bit depth samples.
     * Processing:  Audio sample accumulator 1.7647x.
     * Output:      Mono stream of 48kHz/16-bit is sent over the air to the Coordinator.
     *
     * +-----+    +---------------------+    +-----+
     * | USB | -> | Accumulator 1.7674x | -> | SWC |
     * +-----+    +---------------------+    +-----+
     *
     ***** NORMAL MODE I2S (Fallback mode 0) *****
     * Input:       Mono stream of 96kHz/16-bit depth samples.
     * Processing:  Downsampling audio samples from 96kHz to 48kHz.
     * Processing:  Audio sample accumulator 1.7647x.
     * Output:      Mono stream of 48kHz/16-bit is sent over the air to the Coordinator.
     *
     * +-----+    +-----------------+    +----------------------+    +-----+
     * | I2S | -> | Downsampling 2x | -> | Accumulator 1.76447x | -> | SWC |
     * +-----+    +-----------------+    +----------------------+    +-----+
     *
     ***** FALLBACK MODE USB (Fallback mode 1) *****
     * Input:       Mono stream of 48kHz/16-bit depth samples.
     * Processing:  Audio sample accumulator 1.7647x.
     * Processing:  Audio compression using ADPCM.
     * Output:      ADPCM compressed Mono stream of 48kHz/16-bit is sent over the air to the Coordinator.
     *
     * +-----+    +---------------------+    +-------------------+    +-----+
     * | USB | -> | Accumulator 1.7647x | -> | ADPCM Compression | -> | SWC |
     * +-----+    +---------------------+    +-------------------+    +-----+
     *
     ***** FALLBACK MODE I2S (Fallback mode 1) *****
     * Input:       Mono stream of 96kHz/16-bit depth samples.
     * Processing:  Downsampling audio samples from 96kHz to 48kHz.
     * Processing:  Audio sample accumulator 1.7647x.
     * Processing:  Audio compression using ADPCM.
     * Output:      ADPCM compressed Mono stream of 48kHz/16-bit is sent over the air to the Coordinator.
     *
     * +-----+    +-----------------+    +---------------------+    +-------------------+    +-----+
     * | I2S | -> | Downsampling 2x | -> | Accumulator 1.7647x | -> | ADPCM Compression | -> | SWC |
     * +-----+    +-----------------+    +---------------------+    +-------------------+    +-----+
     */

    /* Initialize codec producer endpoint. */
    sac_endpoint_cfg_t back_channel_producer_cfg = {
        .use_encapsulation = false,
        .delayed_action = !USB_AUDIO_ENABLED,
        .channel_count = BACK_CHANNEL_CHANNEL_COUNT,
        .audio_payload_size = USB_AUDIO_ENABLED ? BACK_CHANNEL_SWC_PAYLOAD_SIZE : BACK_CHANNEL_I2S_PAYLOAD_SIZE,
        .queue_size = SAC_MIN_PRODUCER_QUEUE_SIZE + (USB_AUDIO_ENABLED ? BACK_CHANNEL_USB_FS_PRODUCER_BUFFERING : 0),
    };
    back_channel_producer_endpoint = sac_endpoint_init(NULL, "Audio EP (Producer)", back_channel_producer_iface,
                                                       back_channel_producer_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Processing stage that downsamples the audio samples from 96kHz to 48kHz. */
    back_channel_downsampling_instance.cfg.channel_count = BACK_CHANNEL_CHANNEL_COUNT;
    back_channel_downsampling_instance.cfg.divide_ratio = SAC_SRC_TWO;
    back_channel_downsampling_instance.cfg.multiply_ratio = SAC_SRC_ONE;
    back_channel_downsampling_instance.cfg.payload_size = BACK_CHANNEL_I2S_PAYLOAD_SIZE;
    back_channel_downsampling_instance.cfg.input_sample_format = I2S_SAC_SAMPLE_FORMAT;
    back_channel_downsampling_instance.cfg.output_sample_format = BACK_CHANNEL_SAC_SAMPLE_FORMAT;
    back_channel_downsampling_processing = sac_processing_stage_init((void *)&back_channel_downsampling_instance,
                                                                     "Audio Downsampling",
                                                                     back_channel_downsampling_iface, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* SWC consumer endpoint initialization. */
    sac_endpoint_cfg_t src_consumer_cfg = {
        .use_encapsulation = false,
        .delayed_action = false,
        .channel_count = BACK_CHANNEL_CHANNEL_COUNT,
        .audio_payload_size = BACK_CHANNEL_SWC_PAYLOAD_SIZE,
        .queue_size = BACK_CHANNEL_LATENCY_QUEUE_SIZE,
    };
    back_channel_src_consumer_endpoint = sac_endpoint_init(NULL, "SRC EP (Consumer1)", dummy_iface, src_consumer_cfg,
                                                           &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Audio pipeline initialization. */
    sac_pipeline_cfg_t back_channel_src_pipeline_cfg = {
        .do_initial_buffering = false,
    };
    back_channel_src_pipeline = sac_pipeline_init("Audio -> SRC", back_channel_producer_endpoint,
                                                  back_channel_src_pipeline_cfg, back_channel_src_consumer_endpoint,
                                                  &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#if (!USB_AUDIO_ENABLED)
    sac_pipeline_add_processing(back_channel_src_pipeline, back_channel_downsampling_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif
    /* Audio pipeline setup. */
    sac_pipeline_setup(back_channel_src_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* SWC pipeline */
    /* SRC SWC producer endpoint initialization. */
    sac_endpoint_cfg_t src_producer_cfg = src_consumer_cfg;

    back_channel_src_swc_producer = sac_endpoint_init(NULL, "SRC SWC EP (Producer)", dummy_iface, src_producer_cfg,
                                                      &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Processing stage that compresses using ADPCM algorithm before sending if fallback is activated. */
    back_channel_compression_instance.compression_mode = SAC_COMPRESSION_PACK_MONO;
    back_channel_compression_instance.sample_format = BACK_CHANNEL_SAC_SAMPLE_FORMAT;
    back_channel_compression_processing = sac_processing_stage_init((void *)&back_channel_compression_instance,
                                                                    "Audio Compression", back_channel_compression_iface,
                                                                    &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    back_channel_compression_discard_processing = sac_processing_stage_init((void *)&back_channel_compression_instance,
                                                                            "Audio Compression Discard",
                                                                            back_channel_compression_discard_iface,
                                                                            &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    back_channel_fallback_instance.connection = tx_audio_conn;
    back_channel_fallback_instance.is_tx_device = true;
    back_channel_fallback_instance.get_tick = facade_get_tick_ms;
    back_channel_fallback_instance.tick_frequency_hz = 1000;
    back_channel_fallback_processing = sac_processing_stage_init(&back_channel_fallback_instance,
                                                                 "Back channel fallback TX", fallback_iface,
                                                                 &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    sac_processing_interface_t back_channel_sample_accumulator_iface = {
        .init = sac_sample_accumulator_init,
        .process = sac_sample_accumulator_process,
        .gate = sac_fallback_gate_is_process_active,
    };
    /* Increase packet size in fallback to increase retx. */
    back_channel_sample_accumulator_instance.max_accumulator_size =
        (BACK_CHANNEL_SWC_PAYLOAD_SIZE * BACK_CHANNEL_MAX_ACC_MUL) / BACK_CHANNEL_MAX_ACC_DIV;
    back_channel_sample_accumulator_instance.get_accumulator_size = get_accumulator_size;
    back_channel_sample_accumulator_processing =
        sac_processing_stage_init((void *)&back_channel_sample_accumulator_instance, "Audio Sample Accumulator",
                                  back_channel_sample_accumulator_iface, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Initialize SWC consumer endpoint. */
    sac_endpoint_cfg_t back_channel_swc_consumer_cfg = {
        .use_encapsulation = true,
        .delayed_action = false,
        .channel_count = BACK_CHANNEL_CHANNEL_COUNT,
        .audio_payload_size = BACK_CHANNEL_SWC_PAYLOAD_SIZE,
        .queue_size = (BACK_CHANNEL_LATENCY_QUEUE_SIZE * BACK_CHANNEL_MAX_ACC_DIV) / BACK_CHANNEL_MAX_ACC_MUL,
    };
    back_channel_swc_consumer_endpoint = sac_endpoint_init((void *)&back_channel_swc_consumer_instance,
                                                           "SWC EP (Consumer)", back_channel_swc_consumer_iface,
                                                           back_channel_swc_consumer_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Initialize audio pipeline. */
    sac_pipeline_cfg_t back_channel_pipeline_cfg = {
        .do_initial_buffering = true,
        .max_payload_size = back_channel_sample_accumulator_instance.max_accumulator_size,
    };
    back_channel_sac_pipeline = sac_pipeline_init("SRC -> SWC", back_channel_src_swc_producer,
                                                  back_channel_pipeline_cfg, back_channel_swc_consumer_endpoint,
                                                  &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    sac_endpoint_link(back_channel_src_consumer_endpoint, back_channel_src_swc_producer, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Add processing stages to the audio pipeline. */
    sac_pipeline_add_processing(back_channel_sac_pipeline, back_channel_fallback_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_add_processing(back_channel_sac_pipeline, back_channel_sample_accumulator_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_add_processing(back_channel_sac_pipeline, back_channel_compression_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_add_processing(back_channel_sac_pipeline, back_channel_compression_discard_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Setup audio pipeline. */
    sac_pipeline_setup(back_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Fallback mode configuration. */
    mode_cfg = sac_fallback_mode_get_defaults();

    /* Fallback mode 0 configuration. */
    mode_cfg.cca_bad_fail_count_threshold_perc = 2;
    mode_cfg.cca_bad_time_sec = 0.1;
    mode_cfg.consumer_buffer_load_threshold_tenths = 18;
    mode_cfg.sample_count = BACK_CHANNEL_SAMPLE_COUNT;
    mode_index = sac_fallback_add_mode(&back_channel_fallback_instance, "48kHz 16-bit", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&back_channel_fallback_instance, mode_index,
                                     back_channel_sample_accumulator_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&back_channel_fallback_instance, mode_index,
                                     back_channel_compression_discard_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Fallback mode 1 configuration. */
    mode_cfg = sac_fallback_mode_get_defaults();
    mode_cfg.cca_good_fail_count_threshold_perc = 2;
    mode_cfg.cca_good_time_sec = 30;
    mode_cfg.link_margin_threshold = 50;
    mode_cfg.link_margin_good_time_sec = 5;
    mode_cfg.sample_count = BACK_CHANNEL_FBK_1_SAMPLE_COUNT;
    mode_index = sac_fallback_add_mode(&back_channel_fallback_instance, "48kHz ADPCM", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&back_channel_fallback_instance, mode_index,
                                     back_channel_sample_accumulator_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&back_channel_fallback_instance, mode_index, back_channel_compression_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    if (USB_AUDIO_ENABLED) {
        /** Start fallback in best quality.
         *
         *  When using USB dongle, fallback state will not update until USB audio playback starts. To avoid the user
         *  thinking the audio quality is bad on startup, the fallback is initialized to its best quality.
         */
        sac_fallback_set_current_mode(&back_channel_fallback_instance, 0, &sac_status);
        ASSERT_SAC_STATUS(sac_status);
    }
}

/** @brief Initialize the audio fallback processing stage interface.
 *
 *  @param[out] iface  Processing interface.
 */
static void app_audio_core_fallback_interface_init(sac_processing_interface_t *iface)
{
    iface->init = sac_fallback_init;
    iface->ctrl = NULL;
    iface->process = sac_fallback_process;
    iface->gate = NULL;
}

/** @brief Initialize the audio upsampling processing stage interface.
 *
 *  @param[out] iface  Processing interface.
 */
static void app_audio_core_upsampling_interface_init(sac_processing_interface_t *iface)
{
    iface->init = sac_src_cmsis_init;
    iface->ctrl = NULL;
    iface->process = sac_src_cmsis_process;
    iface->gate = sac_fallback_gate_is_process_active;
}

/** @brief Initialize the audio mute packet processing stage interface.
 *
 *  @param[out] iface  Processing interface.
 */
static void app_audio_core_mute_packet_interface_init(sac_processing_interface_t *iface)
{
    iface->init = NULL;
    iface->ctrl = NULL;
    iface->process = sac_mute_packet_process;
    iface->gate = NULL;
}

/** @brief Initialize the audio unpacking processing stage interface.
 *
 *  @param[out] iface  Processing interface.
 */
static void app_audio_core_unpacking_interface_init(sac_processing_interface_t *iface)
{
    iface->init = sac_packing_init;
    iface->ctrl = sac_packing_ctrl;
    iface->process = sac_packing_process;
    iface->gate = sac_fallback_gate_is_process_active;
}

/** @brief Initialize the audio decompressing processing stage interface.
 *
 *  @param[out] iface  Processing interface.
 */
static void app_audio_core_decompressing_interface_init(sac_processing_interface_t *iface)
{
    iface->init = sac_compression_init;
    iface->ctrl = sac_compression_ctrl;
    iface->process = sac_compression_process;
    iface->gate = sac_fallback_gate_is_process_active;
}

/** @brief Initialize the digital volume control audio processing stage interface.
 *
 *  @param[out] iface  Processing interface.
 */
static void app_audio_core_volume_interface_init(sac_processing_interface_t *iface)
{
    iface->init = sac_volume_init;
    iface->ctrl = sac_volume_ctrl;
    iface->process = sac_volume_process;
    iface->gate = NULL;
}

/** @brief Initialize the mute on underflow audio processing stage interface.
 *
 *  @param[out] iface  Processing interface.
 */
static void app_audio_core_mute_on_underflow_interface_init(sac_processing_interface_t *iface)
{
    iface->init = sac_mute_on_underflow_init;
    iface->ctrl = NULL;
    iface->process = sac_mute_on_underflow_process;
    iface->gate = NULL;
}

/** @brief Initialize the audio packing processing stage interface.
 *
 *  @param[out] iface  Processing interface.
 */
static void app_audio_core_packing_interface_init(sac_processing_interface_t *iface)
{
    iface->init = sac_packing_init;
    iface->ctrl = sac_packing_ctrl;
    iface->process = sac_packing_process;
    iface->gate = NULL;
}

/** @brief Initialize the sampling rate converter audio processing stage interface.
 *
 *  @param[out] iface  Processing interface.
 */
static void app_audio_core_downsampling_interface_init(sac_processing_interface_t *iface)
{
    iface->init = sac_src_cmsis_init;
    iface->ctrl = NULL;
    iface->process = sac_src_cmsis_process;
    iface->gate = NULL;
}

/** @brief Initialize the audio compressing processing stage interface.
 *
 *  @param[out] iface  Processing interface.
 */
static void app_audio_core_compressing_interface_init(sac_processing_interface_t *iface)
{
    iface->init = sac_compression_init;
    iface->ctrl = sac_compression_ctrl;
    iface->process = sac_compression_process;
    iface->gate = sac_fallback_gate_is_process_active;
}

/** @brief Initialize the compression discard audio processing stage interface.
 *
 *  @param[out] iface  Processing interface.
 */
static void app_audio_core_compression_discard_interface_init(sac_processing_interface_t *iface)
{
    iface->init = NULL;
    iface->ctrl = sac_compression_ctrl;
    iface->process = sac_compression_process_discard;
    iface->gate = sac_fallback_gate_is_process_active;
}

/** @brief Increase the audio output volume level.
 *
 *  This affects the audio pipeline that the digital volume processing stage is added to.
 */
static void volume_up(void)
{
    sac_status_t sac_status = SAC_OK;

    if (device_pairing_state != DEVICE_PAIRED) {
        return;
    }

    sac_processing_ctrl(main_channel_volume_processing, main_channel_sac_pipeline, SAC_VOLUME_INCREASE, SAC_NO_ARG,
                        &sac_status);
    ASSERT_SAC_STATUS(sac_status);
}

/** @brief Decrease the audio output volume level.
 *
 *  This affects the audio pipeline that the digital volume processing stage is added to.
 */
static void volume_down(void)
{
    sac_status_t sac_status = SAC_OK;

    if (device_pairing_state != DEVICE_PAIRED) {
        return;
    }

    sac_processing_ctrl(main_channel_volume_processing, main_channel_sac_pipeline, SAC_VOLUME_DECREASE, SAC_NO_ARG,
                        &sac_status);
    ASSERT_SAC_STATUS(sac_status);
}

#if LATCH_TEST_HOOKS
/** @brief Recovery test A: reset the MCU only, WITHOUT power-cycling the radio.
 *
 *  Boot re-inits the radio (toggles its reset/shutdown pins, re-calibrates). If this
 *  recovers the wedge, the SR1100 latch is cleared by a normal reboot -> in-firmware
 *  recoverable without pulling power. Non-trapping, so it works while wedged.
 */
static void dbg_soft_reset(void)
{
    facade_system_reset();
}

/** @brief Recovery test B: force a radio power-down (assert both shutdown pins), hold
 *         long enough to drain the SR1100's internal rails toward a true POR, then reset.
 *
 *  facade_uwb_shutdown() drives the radio SHUTDOWN pins (real on u5a5, no-op on u535).
 *  The hold is a tick busy-wait — valid here because this runs in the still-alive main
 *  loop. If B recovers but A does not, boot's normal bring-up isn't power-cycling the
 *  radio hard enough and an explicit shutdown belongs in the recovery/boot path. If
 *  neither A nor B recovers and only removing board power does, it is a deep POR-only
 *  latch (hardware / HQ territory).
 */
static void dbg_radio_por(void)
{
    facade_uwb_shutdown();

    uint32_t t0 = facade_get_tick_ms();
    while ((facade_get_tick_ms() - t0) < 300) {
        /* Hold the radios in shutdown ~300 ms before rebooting. */
    }

    facade_system_reset();
}
#endif

#if USB_AUDIO_ENABLED
/** @brief SAI DMA TX complete callback.
 *
 *  This feeds the codec with audio packets. It needs to be executed every time a DMA transfer to the codec is completed
 *  in order to keep the audio playing.
 */
static void main_channel_audio_tx_complete_callback(void)
{
#if SINE_INJECT_HS
    /* Free-running local sine: bypass the pipeline and feed the next sine chunk to SAI TX. */
    sac_facade_i2s_tx_sine((uint8_t *)main_channel_tx_sine_buf, sizeof(main_channel_tx_sine_buf));
    return;
#endif

    sac_status_t sac_status = SAC_OK;
    uint32_t target_fifo_size;
    uint32_t usb_buf_rem = facade_app_audio_usb_get_epin_fifo_remaining();

    target_fifo_size =
        main_channel_fbk_latency_fifo_size[sac_fallback_get_current_mode(&main_channel_fallback_instance, &sac_status)];
    facade_app_audio_usb_set_epin_target_fifo_size(target_fifo_size);

    if ((sac_pipeline_get_consumer_buffer_load(main_channel_accumulator_pipeline, &sac_status) > 0) &&
        ((usb_buf_rem / MAIN_CHANNEL_SWC_PAYLOAD_SIZE) > 1)) {
        sac_pipeline_consume(main_channel_accumulator_pipeline, &sac_status);
    }
}
#else
/** @brief Audio peripheral transfer complete callback.
 *
 *  This feeds the codec with audio packets. It needs to be executed every time a DMA transfer to the codec is completed
 *  in order to keep the audio playing.
 */
static void main_channel_audio_tx_complete_callback(void)
{
#if SINE_INJECT_HS
    /* Free-running local sine: bypass the pipeline and feed the next sine chunk to SAI TX. */
    sac_facade_i2s_tx_sine((uint8_t *)main_channel_tx_sine_buf, sizeof(main_channel_tx_sine_buf));
    return;
#endif

    sac_status_t sac_status = SAC_OK;

    uint32_t target_queue_size;

    /* Set audio latency based on the fallback mode. */
    target_queue_size =
        main_channel_fbk_latency_queue_size[sac_fallback_get_current_mode(&main_channel_fallback_instance,
                                                                          &sac_status)];
    facade_app_audio_cdc_set_target_queue_size(main_channel_accumulator_pipeline, target_queue_size, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    sac_pipeline_consume(main_channel_accumulator_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
}
#endif

/** @brief Audio peripheral receive complete callback.
 *
 *  This receives audio packets from the codec. It needs to be executed every time a DMA transfer from the codec is
 *  completed in order to keep recording audio.
 */
static void back_channel_audio_rx_complete_callback(void)
{
    sac_status_t sac_status = SAC_OK;

    /* The codec produces audio samples when it receives input audio. */
    sac_pipeline_produce(back_channel_src_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Trigger back channel process. */
    facade_audio_process_back_channel_timer_trigger();
}

/** @brief Callback handling the audio process that triggers with the app timer.
 */
static void audio_process_main_channel_callback(void)
{
    sac_status_t sac_status = SAC_OK;

    if (main_channel_trigger_count > 0) {
        main_channel_trigger_count--;
    }

    sac_pipeline_process(main_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    while (sac_pipeline_get_producer_buffer_load(main_channel_accumulator_pipeline, &sac_status) > 0) {
        sac_pipeline_process(main_channel_accumulator_pipeline, &sac_status);
        ASSERT_SAC_STATUS(sac_status);
    }

#if USB_AUDIO_ENABLED
    uint32_t buffer_load = 0;

    buffer_load = sac_pipeline_get_consumer_buffer_load(main_channel_accumulator_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Consume all nodes into the USB FIFO. */
    while (buffer_load > 0 && facade_app_audio_usb_get_epin_fifo_remaining() >= MAIN_CHANNEL_SWC_PAYLOAD_SIZE) {
        /* The USB audio consumes audio coming from the SWC rx audio connection. */
        sac_pipeline_consume(main_channel_accumulator_pipeline, &sac_status);
        ASSERT_SAC_STATUS(sac_status);
        buffer_load--;
    }
#endif

    if (main_channel_trigger_count > 0) {
        /* Retrigger the processing. */
        facade_audio_process_main_channel_timer_trigger();
    }
}

/** @brief Callback handling the audio process that triggers with the app timer.
 */
static void audio_process_back_channel_callback(void)
{
    sac_status_t sac_status = SAC_OK;
    uint32_t buffer_load = 0;

    buffer_load = sac_pipeline_get_producer_buffer_load(back_channel_src_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    if (buffer_load > 0) {
        /* Processing stages of the back channel pipeline are executed. */
        sac_pipeline_process(back_channel_src_pipeline, &sac_status);
        ASSERT_SAC_STATUS(sac_status);
        sac_pipeline_process(back_channel_sac_pipeline, &sac_status);
        ASSERT_SAC_STATUS(sac_status);
    }

    buffer_load = sac_pipeline_get_consumer_buffer_load(back_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    if (buffer_load > 0) {
        /* The SWC consumes audio samples produced by the codec. */
        sac_pipeline_consume(back_channel_sac_pipeline, &sac_status);
        ASSERT_SAC_STATUS(sac_status);
    }
}

/** @brief Check if stats should be printed.
 *
 *  @retval 0  Stats should not be printed.
 *  @retval 1  Stats should be printed.
 */
static bool should_print_stats(void)
{
    static uint32_t tick_start;
    uint32_t current_tick = facade_get_tick_ms();

    if (device_pairing_state != DEVICE_PAIRED) {
        tick_start = current_tick;
        return false;
    }

    if ((current_tick - tick_start) >= PRINT_INTERVAL_MS) {
        tick_start = current_tick;
        return true;
    }

    return false;
}

#if LINK_WATCH
/** @brief Lightweight link diagnostic for the HS RX-audio connection.
 *
 *  Prints a one-line report at LINK_WATCH_INTERVAL_MS and an immediate event line
 *  on every connect<->disconnect transition. Output goes via facade_stats_write
 *  (board-aware: ST-Link VCP / UART4 PC10-PC11 on u535, USB CDC elsewhere), the same
 *  channel as the crash-dump, so LW + crash-dump land on one port. The AT console
 *  (LPUART1) is left untouched. Reads are non-asserting on purpose, so the watch keeps
 *  running through a link drop instead of trapping.
 *
 *  Line format:
 *    [LW seq t=<ms>] <OK|LOST> lm=<link_margin> fb=<mode> swc=<RUN|STOP> cca_fail=<n>
 *        tx_drop=<n> rx_ok=<n> rx_miss=<n> miss/s=<n> rx_rej=<n> err=<connErr>/<statErr>
 *        send_err=<lastErr>(<count>)
 *
 *  How to read it:
 *    - lm (link margin): higher is better; collapsing toward 0 right before LOST
 *      points to RF range/interference.
 *    - fb (fallback mode): 0=96k/24b, 1=48k/24b, 2=48k/16b, 3=48k/ADPCM. Climbing
 *      0->3 before a drop is the link degrading down the fallback ladder.
 *    - OK->LOST edge prints the moment the link layer declares the conn down.
 *    - If the log FREEZES at/after the drop (seq stops advancing), the firmware
 *      trapped in swc_error_handler()'s while(1) -- that's a hang, not RF.
 *    - If it keeps printing LOST forever, the node never re-syncs (true RF loss);
 *      if it returns to OK on its own, auto-resync worked. */
static void link_watch(void)
{
    static uint32_t tick_start;
    static uint32_t seq;
    static bool initialized;
    static bool prev_connected;
    static uint32_t rxmiss_prev;
    static uint32_t rxmiss_prev_tick;
    static bool rxmiss_prev_valid;

    if (device_pairing_state != DEVICE_PAIRED || rx_audio_conn == NULL) {
        initialized = false;
        rxmiss_prev_valid = false;
        return;
    }

    uint32_t now = facade_get_tick_ms();
    if (initialized && (now - tick_start) < LINK_WATCH_INTERVAL_MS) {
        return;
    }
    tick_start = now;

    swc_error_t conn_err = SWC_ERR_NONE;
    swc_error_t stat_err = SWC_ERR_NONE;
    sac_status_t fb_status = SAC_OK;
    bool connected = swc_connection_get_connect_status(rx_audio_conn, &conn_err);
    swc_fallback_info_t info = swc_connection_get_fallback_info(rx_audio_conn, &conn_err);
    swc_statistics_t *rx_stats = swc_connection_update_stats(rx_audio_conn, &stat_err);
    swc_status_t swc_state = swc_get_status();
    /* Current main-channel fallback mode: 0=96k/24b, 1=48k/24b, 2=48k/16b, 3=48k/ADPCM.
     * Climbing 0->3 before a LOST is the link degrading down the fallback ladder. */
    uint8_t fb_mode = sac_fallback_get_current_mode(&main_channel_fallback_instance, &fb_status);

    uint32_t rx_ok = (rx_stats != NULL) ? rx_stats->packet_successfully_received_count : 0;
    uint32_t rx_miss = (rx_stats != NULL) ? rx_stats->no_packet_reception_count : 0;
    uint32_t rx_rej = (rx_stats != NULL) ? rx_stats->packet_rejected_count : 0;

    /* rx_miss per-second delta (empty/lost timeslots per second) — the live rate is far more
     * readable than the cumulative total. Compare against the DG's prod=<n>/s on the same time
     * axis: a high miss/s that tracks the DG being healthy (prod~2400) means the empties are the
     * fallback-mode slot occupancy, not starvation. Normalized to /s over the real interval. */
    uint32_t rxmiss_rate = 0;
    if (rxmiss_prev_valid) {
        uint32_t dms = now - rxmiss_prev_tick;
        if (dms > 0) {
            rxmiss_rate = (uint32_t)(((uint64_t)(rx_miss - rxmiss_prev) * 1000U) / dms);
        }
    }
    rxmiss_prev = rx_miss;
    rxmiss_prev_tick = now;
    rxmiss_prev_valid = true;

    /* Dual-radio HW liveness: if one of these freezes while the LW seq keeps
     * advancing, that radio's IRQ/DMA path stalled (the dual-only failure mode). */
    uint32_t r1_irq = 0, r2_irq = 0, r1_dma = 0, r2_dma = 0;
    bool have_hw = facade_get_radio_hw_counters(&r1_irq, &r2_irq, &r1_dma, &r2_dma);

    char line[256];

    /* Edge: announce connect<->disconnect transitions immediately. */
    if (!initialized) {
        prev_connected = connected;
        initialized = true;
    } else if (connected != prev_connected) {
        snprintf(line, sizeof(line), "\r\n[LW EVENT t=%lu] link %s\r\n",
                 (unsigned long)now, connected ? "RECOVERED" : "DROPPED");
        facade_stats_write(line);
        prev_connected = connected;
    }

    int n = snprintf(line, sizeof(line),
             "[LW %lu t=%lu] %s lm=%u fb=%u swc=%s cca_fail=%lu tx_drop=%lu "
             "rx_ok=%lu rx_miss=%lu miss/s=%lu rx_rej=%lu err=%d/%d send_err=%d(%lu)",
             (unsigned long)seq++, (unsigned long)now, connected ? "OK  " : "LOST",
             (unsigned)info.link_margin, (unsigned)fb_mode,
             (swc_state == SWC_STATUS_RUNNING) ? "RUN" : "STOP",
             (unsigned long)info.cca_fail_count, (unsigned long)info.tx_pkt_dropped,
             (unsigned long)rx_ok, (unsigned long)rx_miss, (unsigned long)rxmiss_rate,
             (unsigned long)rx_rej,
             (int)conn_err, (int)stat_err, (int)s_last_send_err, (unsigned long)s_send_err_count);
    if (have_hw && n > 0 && n < (int)sizeof(line)) {
        snprintf(line + n, sizeof(line) - n,
                 " r1_irq=%lu r2_irq=%lu r1_dma=%lu r2_dma=%lu\r\n",
                 (unsigned long)r1_irq, (unsigned long)r2_irq,
                 (unsigned long)r1_dma, (unsigned long)r2_dma);
    } else if (n > 0 && n < (int)sizeof(line)) {
        snprintf(line + n, sizeof(line) - n, "\r\n");
    }
    facade_stats_write(line);
}
#endif /* LINK_WATCH */

/** @brief Print the audio and wireless statistics.
 */
static void print_stats(void)
{
#if !PRINT_STATS_ENABLED
    /* Disabled on this debug branch to keep the CDC log quiet (see PRINT_STATS_ENABLED). */
    return;
#endif

    if (device_pairing_state != DEVICE_PAIRED) {
        return;
    }

    static char stats_string[STATS_ARRAY_LENGTH];
    int string_length = 0;
    sac_status_t sac_status = SAC_OK;
    swc_error_t swc_err = SWC_ERR_NONE;

    const char *device_str = "\n<   NODE   >\n\r";
    const char *audio_stats_str = "\n<<  Audio Core Statistics  >>\n\r";
    const char *fallback_stats_str = "\n<<  Fallback Statistics  >>\n\r";
    const char *wireless_stats_str = "\n<<  Wireless Core Statistics  >>\n\r";

    memset(stats_string, 0, sizeof(stats_string));

    /* ** Device Prelude ** */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, device_str);

    if (certification_mode != FACADE_CERTIF_NONE) {
        string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length,
                                  "Cert. Mode: %i\r\n", certification_mode);
    }

    /* ** Audio Statistics ** */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, audio_stats_str);
    sac_pipeline_update_stats(main_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    string_length += sac_pipeline_format_stats(main_channel_sac_pipeline, stats_string + string_length,
                                               sizeof(stats_string) - string_length, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_update_stats(main_channel_accumulator_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    string_length += sac_pipeline_format_stats(main_channel_accumulator_pipeline, stats_string + string_length,
                                               sizeof(stats_string) - string_length, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

#if USB_AUDIO_ENABLED
    /* ** USB Audio Statistics ** */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length,
                              "USB audio TX fifo sample count: %lu\r\n",
                              facade_get_node_usb_audio_tx_fifo_sample_count());
#else
    /* ** CDC Statistics ** */
    string_length += sac_facade_cdc_format_stats(stats_string + string_length, sizeof(stats_string) - string_length,
                                                 &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif

    sac_pipeline_update_stats(back_channel_src_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    string_length += sac_pipeline_format_stats(back_channel_src_pipeline, stats_string + string_length,
                                               sizeof(stats_string) - string_length, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_update_stats(back_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    string_length += sac_pipeline_format_stats(back_channel_sac_pipeline, stats_string + string_length,
                                               sizeof(stats_string) - string_length, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* ** Audio Fallback Statistics ** */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, fallback_stats_str);
    string_length += sac_fallback_format_stats(&main_channel_fallback_instance, stats_string + string_length,
                                               sizeof(stats_string) - string_length, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    string_length += sac_fallback_format_stats(&back_channel_fallback_instance, stats_string + string_length,
                                               sizeof(stats_string) - string_length, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* ** Wireless Statistics ** */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, wireless_stats_str);
    swc_connection_update_stats(rx_audio_conn, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    string_length += swc_connection_format_stats(rx_audio_conn, stats_string + string_length,
                                                 sizeof(stats_string) - string_length, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    swc_connection_update_stats(tx_audio_conn, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    string_length += swc_connection_format_stats(tx_audio_conn, stats_string + string_length,
                                                 sizeof(stats_string) - string_length, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    swc_connection_update_stats(tx_data_conn, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    string_length += swc_connection_format_stats(tx_data_conn, stats_string + string_length,
                                                 sizeof(stats_string) - string_length, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    swc_connection_update_stats(rx_data_conn, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    string_length += swc_connection_format_stats(rx_data_conn, stats_string + string_length,
                                                 sizeof(stats_string) - string_length, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    /* Board-dependent routing: u535 -> AT/expansion UART (LPUART1); u5a5 EVK & others -> USB CDC. */
    facade_stats_write(stats_string);

    /* ** APP Statistics ** */
    string_length = snprintf(stats_string, sizeof(stats_string), "\r\n<< Application Statistics >>\r\n");
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, "Fallback State:");
    if (fallback_state == FALLBACK_AUTO) {
        string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, " AUTO\r\n");
    } else if (fallback_state == FALLBACK_48K_UNCOMPRESSED) {
        string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length,
                                  " 48kHz 16-bit\r\n");
    } else if (fallback_state == FALLBACK_48K_ADPCM) {
        string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length,
                                  " 48kHz ADPCM\r\n");
    }

    facade_stats_write(stats_string);
}

#ifdef SINE_DEBUG_CAPTURE
/* Node-side observability, all updated every 10 ms in data_callback. Watch these instead of the
 * void* current_mode pointer.
 *   dbg_main_fb_mode    = current main-channel fallback mode index (0=96k/24b, 1=48k/24b,
 *                         2=48k/16b, 3=48k/ADPCM)
 *   dbg_main_fb_lm      = measured RX link margin
 *   dbg_main_queue_load = audio output (accumulator->SAI) consumer buffer load; 0 = starved/underflow,
 *                         climbing-to-full then dropping = overflow / producer faster than consumer
 *   dbg_rx_ok / miss / rej = cumulative SWC RX counts on rx_audio_conn: CRC-good / timeslot-empty
 *                         (lost) / corrupted. miss+rej rising in step with the audible glitches = RF loss. */
volatile uint8_t  dbg_main_fb_mode    = 0xFF;
volatile uint8_t  dbg_main_fb_lm      = 0;
volatile uint32_t dbg_main_queue_load = 0;
volatile uint32_t dbg_rx_ok           = 0;
volatile uint32_t dbg_rx_miss         = 0;
volatile uint32_t dbg_rx_rej          = 0;
#endif

/** @brief Callback sends the link margin and the button state every 10 ms.
 */
static void data_callback(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    swc_fallback_info_t fallback_info = {0};
    user_data_t transmitted_user_data = {0};

    /* Update the link margin and the button state. Do NOT assert here: this runs every
     * 10 ms and a transient error on link loss must not hang the device. Record and continue;
     * fallback_info stays zero-initialized on error so we just report a zero link margin. */
    fallback_info = swc_connection_get_fallback_info(rx_audio_conn, &swc_err);
    s_last_fb_info_err = swc_err;

#ifdef SINE_DEBUG_CAPTURE
    {
        sac_status_t fb_status = SAC_OK;
        swc_error_t stats_err = SWC_ERR_NONE;
        swc_statistics_t *rx_stats = swc_connection_update_stats(rx_audio_conn, &stats_err);

        dbg_main_fb_mode    = sac_fallback_get_current_mode(&main_channel_fallback_instance, &fb_status);
        dbg_main_fb_lm      = fallback_info.link_margin;
        dbg_main_queue_load = sac_pipeline_get_consumer_buffer_load(main_channel_accumulator_pipeline, &fb_status);
        if (rx_stats != NULL) {
            dbg_rx_ok   = rx_stats->packet_successfully_received_count;
            dbg_rx_miss = rx_stats->no_packet_reception_count;
            dbg_rx_rej  = rx_stats->packet_rejected_count;
        }
    }
#endif

    transmitted_user_data.link_margin = fallback_info.link_margin;
    transmitted_user_data.button_state = facade_read_button_state();
    transmitted_user_data.cmd_type = s_pending_cmd;
    s_pending_cmd = 0;
    transmitted_user_data.battery_pct = s_battery_pct;

    /* Send the button state to the Coordinator. */
    wireless_send_data(&transmitted_user_data, sizeof(transmitted_user_data), &swc_err);
}

/** @brief Handle pairing button callback.
 */
static void pairing_button_callback(void)
{
    /* Boot auto-reconnect in progress: the state says PAIRED but the link is only
     * half-open and try_boot_reconnect() still owns the connection handles. Do NOT
     * unpair from here -- just ask the reconnect to give up. The loop tears the link
     * down and main() drops into pairing mode, which is what the press meant anyway. */
    if (s_boot_reconnect_active) {
        s_boot_reconnect_abort = true;
        return;
    }

    switch (device_pairing_state) {
    case DEVICE_PAIRED:
        unpair_device();
        break;
    case DEVICE_PAIRING:
        abort_pairing_procedure();
        break;
    case DEVICE_UNPAIRED:
        enter_pairing_mode();
        break;
    default:
        break;
    }
}

/** @brief Enter Pairing Mode using the Pairing Module.
 */
static void enter_pairing_mode(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    pairing_error_t pairing_err = PAIRING_ERR_NONE;
    pairing_event_t pairing_event = PAIRING_EVENT_NONE;

    /* Set the pairing state. */
    device_pairing_state = DEVICE_PAIRING;
    at_cmd_core_notify_pairing_started();

    facade_notify_enter_pairing();

    /* The wireless core must be stopped before starting the pairing procedure. */
    if (swc_get_status() == SWC_STATUS_RUNNING) {
        swc_disconnect(&swc_err);
        ASSERT_SWC_STATUS(swc_err);
    }

    /* Give the information to the Pairing Module. */
    app_pairing_cfg.app_code = PAIRING_APP_CODE;
    app_pairing_cfg.timeout_sec = PAIRING_TIMEOUT_IN_SECONDS;
    app_pairing_cfg.application_callback = pairing_process_callback;
    app_pairing_cfg.memory_pool = swc_memory_pool;
    app_pairing_cfg.memory_pool_size = SWC_MEM_POOL_SIZE;
    app_pairing_cfg.context_switch_callback = facade_context_switch_trigger;
    pairing_event = pairing_node_start(&app_pairing_cfg, &pairing_assigned_address, PAIRING_DEVICE_ROLE_NODE,
                                       &pairing_err);
    if (pairing_err != PAIRING_ERR_NONE) {
        s_assert_file = __FILE__;
        s_assert_line = __LINE__;
        fatal_trap("PAIRING", (int)pairing_err);
    }

    /* Handle the pairing events. */
    switch (pairing_event) {
    case PAIRING_EVENT_SUCCESS:
        /* Indicate that the pairing process was successful. */
        facade_notify_pairing_successful();

        app_init();
        device_pairing_state = DEVICE_PAIRED;

        /* Persist the freshly assigned address so the next boot reconnects
         * automatically (boot auto-reconnect). Best-effort: a flash fault here
         * only costs the auto-reconnect on the next boot, not this session. */
        reconnect_store_save(&pairing_assigned_address);

        at_cmd_core_notify_pairing_result(true);
        break;
    case PAIRING_EVENT_TIMEOUT:
    case PAIRING_EVENT_INVALID_APP_CODE:
    case PAIRING_EVENT_ABORT:
    default:
        /* Indicate that the pairing process was unsuccessful. */
        facade_notify_not_paired();
        device_pairing_state = DEVICE_UNPAIRED;

        at_cmd_core_notify_pairing_result(false);
        break;
    }
}

/** @brief Attempt to reconnect to the persisted peer on boot.
 *
 *  If a valid pairing address was persisted to flash, restore it and bring the
 *  wireless core up (same path as at_start_connect / a fresh pairing success),
 *  then wait up to RECONNECT_TIMEOUT_MS for the real SWC link to come up. On
 *  success the device stays paired and streaming. On timeout it powers the module
 *  down into Standby (does not return); the flash record is KEPT (the peer being
 *  off is not a reason to forget the pair).
 *
 *  @return BOOT_RECONNECT_OK   link re-established (paired, streaming);
 *          BOOT_RECONNECT_PAIR no usable record (never paired), or the user
 *                              aborted the attempt to pair (button / AT+UWB_PAIR)
 *                              -- caller enters pairing.
 *          (A plain timeout does NOT return: the module enters Standby to save the
 *          battery and a reset wakes it back through boot auto-reconnect. The
 *          BOOT_RECONNECT_IDLE enumerator is therefore unreachable on the HS.)
 */
static boot_reconnect_result_t try_boot_reconnect(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    uint32_t start;
    bool connected = false;

    /* Blank / corrupt / wrong-version flash -> no record -> never paired -> pair. */
    if (!reconnect_store_load(&pairing_assigned_address)) {
        return BOOT_RECONNECT_PAIR;
    }

    /* A valid record should never carry a zero node address, but mirror the
     * at_start_connect() gate defensively -- treat it as no record and pair. */
    if (pairing_assigned_address.node_address == 0) {
        return BOOT_RECONNECT_PAIR;
    }

    /* Fast-blink the status LED to show a silent reconnect is in progress. */
    facade_notify_reconnecting();

    /* Build the wireless core from the stored addresses and connect. */
    at_cmd_core_set_uwb_conn_status(AT_UWB_CONN_STATUS_CONNECTING);
    app_init();
    device_pairing_state = DEVICE_PAIRED;

    /* Poll the real SWC link status (not just the local pairing state) until the
     * peer is actually reachable or the timeout elapses. Keep servicing buttons
     * and AT commands so the user can still abort / issue commands meanwhile;
     * those handlers defer their teardown through s_boot_reconnect_abort. */
    s_boot_reconnect_abort = false;
    s_boot_reconnect_active = true;

    start = facade_get_tick_ms();
    while ((facade_get_tick_ms() - start) < RECONNECT_TIMEOUT_MS) {
        swc_err = SWC_ERR_NONE;
        if (swc_connection_get_connect_status(rx_audio_conn, &swc_err)) {
            connected = true;
            break;
        }
        facade_button_handling();
        at_cmd_core_process();

        if (s_boot_reconnect_abort) {
            break;
        }
        /* Belt-and-braces: every teardown path NULLs the connection handles, and
         * swc_connection_get_connect_status() would dereference that on the next
         * pass. Never poll a handle the app has already released. */
        if (rx_audio_conn == NULL) {
            break;
        }
    }

    s_boot_reconnect_active = false;

    if (connected) {
        facade_notify_pairing_successful();
        return BOOT_RECONNECT_OK;
    }

    /* The loop ended without a link. Distinguish intent before tearing down. Either
     * way tear the half-open link down (resets device_pairing_state to UNPAIRED and
     * stops the pipelines). In-place teardown here, NOT the Standby that
     * AT+UWB_DISCONNECT performs. The flash record is intentionally kept. */
    if (s_boot_reconnect_abort) {
        /* The user pressed the pairing button / sent AT+UWB_PAIR during the window,
         * so pairing is what they meant. */
        app_teardown();
        return BOOT_RECONNECT_PAIR;
    }

    /* Plain timeout: the peer was simply unreachable. Blink the reconnect colour once
     * so the timeout is easy to observe, then power the module down into Standby to
     * save the battery -- the HS is battery-powered and there is nothing to do until
     * the peer is back. Do NOT re-pair. Waking (SoC NRST / a WKUP button) resets the
     * MCU back through boot auto-reconnect to try again. Same power-down path as
     * AT+UWB_DISCONNECT; the flash record is intentionally kept so the reset
     * reconnects to the same peer. */
    facade_notify_reconnect_failed();
    at_cmd_core_notify_standby();
    facade_enter_standby(); /* does not return */
    return BOOT_RECONNECT_IDLE; /* unreachable: keeps the non-void return type happy */
}

/** @brief Put the device in the unpaired state and disconnect it from the network.
 */
static void unpair_device(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    sac_status_t sac_status = SAC_OK;

    device_pairing_state = DEVICE_UNPAIRED;

    /* Stop timers. */
    facade_audio_process_main_channel_timer_stop();
    facade_audio_process_back_channel_timer_stop();
    facade_data_timer_stop();

    /* Disconnect the Wireless Core. */
    swc_disconnect(&swc_err);
    ASSERT_SWC_STATUS(swc_err);

    tx_audio_conn = NULL;
    rx_audio_conn = NULL;
    tx_data_conn = NULL;
    rx_data_conn = NULL;

    /* Stop the main channel audio pipeline. */
    sac_pipeline_stop(main_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_stop(main_channel_accumulator_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Stop the back channel audio pipeline. */
    sac_pipeline_stop(back_channel_src_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_stop(back_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    main_channel_sac_pipeline = NULL;
    main_channel_accumulator_pipeline = NULL;
    back_channel_src_pipeline = NULL;
    back_channel_sac_pipeline = NULL;

    facade_audio_deinit();

    /* Erase the persisted pairing address so the next boot does not reconnect
     * to the device the user just removed (boot auto-reconnect). */
    reconnect_store_clear();

    /* Indicate that the device is unpaired. */
    facade_led_all_off();
    facade_notify_not_paired();
}

/** @brief Pairing process callback called during pairing.
 */
static void pairing_process_callback(void)
{
    /*
     * Note: The button press will only be detected when the pairing module executes the registered pairing process
     *       callback, which might take a variable amount of time.
     */
    facade_button_handling();
    at_cmd_core_process();
}

/** @brief Abort the pairing procedure.
 */
static void abort_pairing_procedure(void)
{
    pairing_abort();
}

/** @brief Send data with a specific connection.
 *
 *  @param[in]  transmitted_data  Data to be sent over the air.
 *  @param[in]  size              Size of the data to be sent over the air.
 *  @param[out] swc_err           Wireless Core error code.
 */
static void wireless_send_data(void *transmitted_data, uint8_t size, swc_error_t *swc_err)
{
    uint8_t *buffer = NULL;

    /* Get buffer from queue to hold data. */
    swc_connection_get_payload_buffer(tx_data_conn, &buffer, swc_err);
    if ((*swc_err != SWC_ERR_NONE) || (buffer == NULL)) {
        return;
    }

    /* Format the new payload. */
    if (transmitted_data != NULL) {
        memcpy(buffer, transmitted_data, size);
    }

    /* Send the payload through the Wireless Core. Do NOT assert: when the link is down the
     * TX queue fills up and this returns an error every 10 ms. Trapping here was a prime cause
     * of the HS never recovering. Record the error and return; the queue drains once the link
     * is back. */
    swc_connection_send(tx_data_conn, buffer, size, swc_err);
    if (*swc_err != SWC_ERR_NONE) {
        s_last_send_err = *swc_err;
        s_send_err_count++;
    }
}

/** @brief Read data from a specific connection.
 *
 *  @param[out] received_data  Pointer to data buffer to write to.
 *  @param[in]  size           Size of the data buffer.
 *  @param[out] swc_err        Wireless Core error code.
 *
 *  @return Size of the data read.
 */
static uint16_t wireless_read_data(void *received_data, uint8_t size, swc_error_t *swc_err)
{
    uint8_t *payload = NULL;
    uint16_t payload_size = 0;

    /* Read received data. */
    payload_size = swc_connection_receive(rx_data_conn, &payload, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    if (payload_size > size) {
        return 0;
    }

    if (received_data != NULL) {
        memcpy(received_data, payload, payload_size);
    }

    /* Free the payload memory. */
    swc_connection_receive_complete(rx_data_conn, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    return payload_size;
}

/** @brief Initialize the application.
 */
static void app_init(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    sac_status_t sac_status = SAC_OK;

    /* Initialize Wireless Core. */
    app_swc_core_init(&pairing_assigned_address, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    /* Connect the Wireless Core. */
    swc_connect(&swc_err);
    ASSERT_SWC_STATUS(swc_err);

    at_cmd_core_set_device_address(pairing_assigned_address.node_address);
    at_cmd_core_set_uwb_conn_status(AT_UWB_CONN_STATUS_CONNECTED);

    /* Initialize Audio Core. */
    app_audio_core_init();
    /* Initialize GPIOs and peripherals for audio operations. */
#if (I2S_MASTER_MODE)
    facade_audio_node_init(true);
#else
    facade_audio_node_init(false);
#endif

    /* Start audio pipelines. */
    sac_pipeline_start(main_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_start(main_channel_accumulator_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_start(back_channel_src_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_start(back_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

#if SINE_INJECT_HS
    /* Kick the first SAI TX so the self-clocked sine loop starts. Without this the consumer
     * never starts (the pipeline queue stays empty with no OTA data), so no TX-complete would
     * ever fire. After this first write, each TX-complete re-arms the next sine chunk. */
    sac_facade_i2s_tx_sine((uint8_t *)main_channel_tx_sine_buf, sizeof(main_channel_tx_sine_buf));
#endif

    /* Start timers used for audio processes. */
    facade_audio_process_main_channel_timer_start();
    facade_audio_process_back_channel_timer_start();

    /* Start data and statistics timer. */
    facade_data_timer_start();
}

/** @brief AT+UWB_PAIR -- re-pair: drop the current pairing (if any), then enter pairing.
 *
 *  Unlike pairing_button_callback(), which is a three-way toggle (press once to unpair,
 *  press again to pair), the AT command is a single action: the host asks for "re-pair"
 *  once and expects the module to end up discoverable. Falling through from
 *  unpair_device() -- which also erases the persisted address via reconnect_store_clear()
 *  -- to enter_pairing_mode() is what makes that one command enough.
 *
 *  Note the link being down does NOT clear device_pairing_state: a device whose peer went
 *  away still reads DEVICE_PAIRED, so this fall-through is the common case, not the rare
 *  one. Re-sending while already DEVICE_PAIRING stays a no-op, so a host that repeats the
 *  command cannot restart the pairing window.
 */
static void at_start_pairing(void)
{
    /* See pairing_button_callback(): during boot auto-reconnect the teardown is
     * deferred to the polling loop, which then falls through to pairing mode. */
    if (s_boot_reconnect_active) {
        s_boot_reconnect_abort = true;
        return;
    }
    if (device_pairing_state == DEVICE_PAIRING) {
        return;
    }
    if (device_pairing_state == DEVICE_PAIRED) {
        unpair_device();
    }
    enter_pairing_mode();
}

/** @brief AT+UWB_CONNECT -- reconnect by resetting the MCU into boot auto-reconnect.
 *
 *  This used to re-run app_init() over the stack that at_start_disconnect() had just torn
 *  down. That crashed: swc_init()/sac_init() themselves are re-entrant (both reset their
 *  memory pool, and swc_disconnect() clears is_started), but the radio and SAI interrupts
 *  keep firing into handles being rebuilt, and the SAC pipelines cannot be restarted at
 *  all -- their lifecycle is start-once (a stopped consumer is never re-started because
 *  sac_pipeline_start() does not clear buffering_complete, and more state besides).
 *
 *  So reconnecting in place is not attempted. A reset boots into try_boot_reconnect(),
 *  which restores the link from the persisted pairing address -- the one reconnect path
 *  that is actually validated. The host sees:
 *      AT+UWB_CONNECT -> OK -> +EVENT: UWB_READY -> +EVENT: UWB_CONNECTED
 *  The OK is already on the wire when this runs: the AT core defers this callback until
 *  after the response is sent, and the expansion UART writes are blocking.
 */
static void at_start_connect(void)
{
    /* Already streaming -- nothing to reconnect, and a reset would drop a working link. */
    if (device_pairing_state == DEVICE_PAIRED) {
        return;
    }
    /* The boot-reconnect window is already bringing the link up; resetting would only
     * restart the attempt we are in the middle of. */
    if (s_boot_reconnect_active) {
        return;
    }

    facade_system_reset();
}

/** @brief Tear the wireless core and audio down in place, leaving the device UNPAIRED.
 *
 *  Only used by the boot auto-reconnect timeout, which must dismantle the half-open link
 *  before main() falls through to pairing. It is NOT what AT+UWB_DISCONNECT does: that
 *  powers the module down instead (see at_start_disconnect), precisely because this path
 *  is the unreliable one -- swc_disconnect() can report a timeout and the SAC pipelines
 *  cannot be restarted afterwards.
 */
static void app_teardown(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    sac_status_t sac_status = SAC_OK;

    /* Called from inside the boot-reconnect polling loop: defer, so the handles
     * stay valid until the loop has unwound. It calls us again right after. */
    if (s_boot_reconnect_active) {
        s_boot_reconnect_abort = true;
        return;
    }
    if (device_pairing_state == DEVICE_UNPAIRED) {
        return;
    }
    device_pairing_state = DEVICE_UNPAIRED;

    facade_audio_process_main_channel_timer_stop();
    facade_audio_process_back_channel_timer_stop();
    facade_data_timer_stop();

    /* Deliberately NOT asserted. swc_disconnect() reports SWC_ERR_DISCONNECT_TIMEOUT when
     * the scheduler does not stop in time, which is exactly the state a user reaches for
     * disconnect in. Trapping there would wedge the device instead of tearing it down --
     * the same reason stall_auto_recover() tolerates this error. */
    swc_disconnect(&swc_err);

    tx_audio_conn = NULL;
    rx_audio_conn = NULL;
    tx_data_conn = NULL;
    rx_data_conn = NULL;

    sac_pipeline_stop(main_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_stop(main_channel_accumulator_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_stop(back_channel_src_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_stop(back_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    main_channel_sac_pipeline = NULL;
    main_channel_accumulator_pipeline = NULL;
    back_channel_src_pipeline = NULL;
    back_channel_sac_pipeline = NULL;

    facade_audio_deinit();
    facade_led_all_off();
    at_cmd_core_set_uwb_conn_status(AT_UWB_CONN_STATUS_STANDBY);
}

/** @brief AT+UWB_DISCONNECT -- power the module down into Standby. Does not return.
 *
 *  No SDK teardown is attempted, deliberately. Stopping the wireless core and the audio
 *  pipelines in place is the unreliable path: swc_disconnect() can report a timeout (and
 *  asserting on it wedged the device), and a stopped SAC pipeline cannot be restarted at
 *  all. Powering the MCU off stops everything at once and makes all of that moot -- which
 *  also means there is nothing left that could quietly bring the link back up.
 *
 *  The module leaves Standby only through a reset, which runs main() from the top and so
 *  goes straight into boot auto-reconnect. On this hardware that is NRST (the SOC line or
 *  the reset button); AT+UWB_CONNECT reaches the same place via facade_system_reset().
 *  On the u535 product board the pairing button is PA4 = WKUP2, so it can be armed as a
 *  wake source later without changing any of this.
 */
static void at_start_disconnect(void)
{
    at_cmd_core_notify_standby();

    facade_enter_standby(); /* does not return */
}

static void at_start_shutdown(void)
{
    at_start_disconnect();
}

static bool at_get_link_status(void)
{
    return device_pairing_state == DEVICE_PAIRED;
}

static int32_t at_get_link_margin(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    swc_fallback_info_t info;

    if (device_pairing_state != DEVICE_PAIRED) {
        return 0;
    }
    info = swc_connection_get_fallback_info(rx_audio_conn, &swc_err);
    return (int32_t)info.link_margin;
}

/** @brief Emit a crash/stall snapshot to the AT UART (called periodically, every
 *         CRASH_DUMP_PERIODIC_MS, from the main loop).
 *
 *  Captures, in one block: the dual-radio HW liveness counters, the wireless link
 *  state (same fields as LINK_WATCH), and any HardFault register snapshot. All SWC
 *  reads use a local err and never assert, so this is safe to call in a stalled
 *  state and does not require the link to be up (covers the "hangs during sync,
 *  before pairing" case as long as the CPU is still running).
 */
#if STALL_AUTO_RECOVER_MS
/** @brief EXPERIMENTAL stall watchdog: force an SWC re-init if the radios wedge.
 *
 *  Samples the per-radio HW liveness counters every STALL_SAMPLE_MS. In normal operation
 *  they tick thousands/second even while out of range, so all four staying identical for
 *  STALL_AUTO_RECOVER_MS is the dual-radio scheduler-wedge signature. On that, force a
 *  scheduler/radio re-init: swc_disconnect() (which may return SWC_ERR_DISCONNECT_TIMEOUT
 *  from a frozen state — tolerated, NOT asserted) then swc_connect() (restarts TIM4, re-arms
 *  the radios, resets sync state). Mitigation only — see STALL_AUTO_RECOVER_MS. */
static void stall_auto_recover(void)
{
    static uint32_t last_sample_ms;
    static uint32_t last_r1_irq, last_r2_irq, last_r1_dma, last_r2_dma;
    static uint32_t frozen_ms;
    static bool have_baseline;

    if (device_pairing_state != DEVICE_PAIRED) {
        have_baseline = false;
        frozen_ms = 0;
        return;
    }

    uint32_t now = facade_get_tick_ms();
    if (have_baseline && (now - last_sample_ms) < STALL_SAMPLE_MS) {
        return; /* sample at a fixed cadence, not every loop iteration */
    }

    uint32_t r1_irq, r2_irq, r1_dma, r2_dma;
    if (!facade_get_radio_hw_counters(&r1_irq, &r2_irq, &r1_dma, &r2_dma)) {
        return; /* no HW counters on this board (single-radio) — nothing to watch */
    }

    if (have_baseline) {
        bool moved = (r1_irq != last_r1_irq) || (r2_irq != last_r2_irq) ||
                     (r1_dma != last_r1_dma) || (r2_dma != last_r2_dma);
        frozen_ms = moved ? 0 : (frozen_ms + (now - last_sample_ms));
    }
    last_sample_ms = now;
    last_r1_irq = r1_irq;
    last_r2_irq = r2_irq;
    last_r1_dma = r1_dma;
    last_r2_dma = r2_dma;
    have_baseline = true;

    if (frozen_ms >= STALL_AUTO_RECOVER_MS) {
        swc_error_t swc_err = SWC_ERR_NONE;

        facade_stats_write("\r\n+AUTO-RECOVER: radio stall -> swc reconnect\r\n");
        swc_disconnect(&swc_err); /* tolerate SWC_ERR_DISCONNECT_TIMEOUT — do NOT assert */
        swc_connect(&swc_err);    /* restarts TIM4 + radios + resets sync state */

        /* Re-baseline so we give the link time to come back before re-checking. */
        frozen_ms = 0;
        have_baseline = false;
    }
}
#endif

static void emit_crash_dump(void)
{
    char buf[288];

    /* Board-dependent routing (same channel as print_stats via facade_stats_write):
     * u535 -> AT/expansion UART (LPUART1); u5a5 EVK & others -> USB CDC. */
    facade_stats_write("\r\n+CRASH_DUMP:\r\n");

    /* Version + compile timestamp so the dump self-identifies the exact binary: __DATE__/
     * __TIME__ change on every rebuild, which is the quick way to confirm the board is
     * running the build you think it is (a hardcoded label cannot). */
    snprintf(buf, sizeof(buf), " build=" AT_CMD_CORE_BUILD_ID " role=HS paired=%d\r\n",
             (device_pairing_state == DEVICE_PAIRED) ? 1 : 0);
    facade_stats_write(buf);

    /* Wireless link state (non-asserting; rx_audio_conn may be NULL before pairing). */
    if (rx_audio_conn != NULL) {
        swc_error_t conn_err = SWC_ERR_NONE;
        swc_error_t stat_err = SWC_ERR_NONE;
        sac_status_t fb_status = SAC_OK;
        bool connected = swc_connection_get_connect_status(rx_audio_conn, &conn_err);
        swc_fallback_info_t info = swc_connection_get_fallback_info(rx_audio_conn, &conn_err);
        swc_statistics_t *rx_stats = swc_connection_update_stats(rx_audio_conn, &stat_err);
        swc_status_t swc_state = swc_get_status();
        uint8_t fb_mode = sac_fallback_get_current_mode(&main_channel_fallback_instance, &fb_status);
        uint32_t rx_ok = (rx_stats != NULL) ? rx_stats->packet_successfully_received_count : 0;
        uint32_t rx_miss = (rx_stats != NULL) ? rx_stats->no_packet_reception_count : 0;
        uint32_t rx_rej = (rx_stats != NULL) ? rx_stats->packet_rejected_count : 0;

        snprintf(buf, sizeof(buf),
                 " swc=%s conn=%s fb=%u lm=%u cca_fail=%lu tx_drop=%lu "
                 "rx_ok=%lu rx_miss=%lu rx_rej=%lu err=%d/%d send_err=%d(%lu)\r\n",
                 (swc_state == SWC_STATUS_RUNNING) ? "RUN" : "STOP",
                 connected ? "OK" : "LOST", (unsigned)fb_mode, (unsigned)info.link_margin,
                 (unsigned long)info.cca_fail_count, (unsigned long)info.tx_pkt_dropped,
                 (unsigned long)rx_ok, (unsigned long)rx_miss, (unsigned long)rx_rej,
                 (int)conn_err, (int)stat_err, (int)s_last_send_err, (unsigned long)s_send_err_count);
    } else {
        snprintf(buf, sizeof(buf), " swc=%s conn=N/A (no connection yet)\r\n",
                 (swc_get_status() == SWC_STATUS_RUNNING) ? "RUN" : "STOP");
    }
    facade_stats_write(buf);

    /* Dual-radio HW liveness counters (u535 & u5a5 dual-radio BSPs). */
    uint32_t r1_irq, r2_irq, r1_dma, r2_dma;
    if (facade_get_radio_hw_counters(&r1_irq, &r2_irq, &r1_dma, &r2_dma)) {
        snprintf(buf, sizeof(buf), " hw: r1_irq=%lu r2_irq=%lu r1_dma=%lu r2_dma=%lu\r\n",
                 (unsigned long)r1_irq, (unsigned long)r2_irq,
                 (unsigned long)r1_dma, (unsigned long)r2_dma);
    } else {
        snprintf(buf, sizeof(buf), " hw: N/A (single-radio board)\r\n");
    }
    facade_stats_write(buf);

    /* Scheduler liveness: mrt=multi-radio timer (TIM4) heartbeat, frt=free-running (TIM8) tick,
     * irq1/irq2=radio IRQ pin levels. Compare across two dumps: mrt frozen -> SWC scheduler died;
     * mrt ticking while hw r*_irq/dma frozen -> scheduler alive but radios not serviced. */
    uint32_t mrt, frt;
    bool irq1, irq2;
    if (facade_get_sched_liveness(&mrt, &frt, &irq1, &irq2)) {
        snprintf(buf, sizeof(buf), " sched: mrt=%lu frt=%lu irq1=%d irq2=%d\r\n",
                 (unsigned long)mrt, (unsigned long)frt, (int)irq1, (int)irq2);
    } else {
        snprintf(buf, sizeof(buf), " sched: N/A\r\n");
    }
    facade_stats_write(buf);

    /* Raw multi-radio scheduler timer (TIM4) state, to pin WHY mrt froze:
     *   cen=0            -> timer was stopped (disconnect/stop path)
     *   cen=1 arr=0      -> period programmed to 0 -> timer stalled (period-clamp hole)
     *   cen=1 arr=65534  -> clamped to max but still not firing -> NVIC/other
     *   cen=1 arr sane, uie=1, cnt advancing -> timer alive (not the wedge) */
    uint32_t t_cr1, t_arr, t_cnt, t_dier;
    if (facade_get_multi_radio_timer_regs(&t_cr1, &t_arr, &t_cnt, &t_dier)) {
        snprintf(buf, sizeof(buf), " tim4: cen=%lu arr=%lu cnt=%lu uie=%lu (cr1=0x%lX dier=0x%lX)\r\n",
                 (unsigned long)(t_cr1 & 0x1u), (unsigned long)t_arr, (unsigned long)t_cnt,
                 (unsigned long)(t_dier & 0x1u), (unsigned long)t_cr1, (unsigned long)t_dier);
    } else {
        snprintf(buf, sizeof(buf), " tim4: N/A\r\n");
    }
    facade_stats_write(buf);

    /* Last captured HardFault (all-zero = no fault seen; cross-ref pc/lr against the .map). */
    uint32_t cfsr, hfsr, pc, lr;
    if (facade_get_hardfault_snapshot(&cfsr, &hfsr, &pc, &lr)) {
        snprintf(buf, sizeof(buf), " fault: cfsr=0x%08lX hfsr=0x%08lX pc=0x%08lX lr=0x%08lX\r\n",
                 (unsigned long)cfsr, (unsigned long)hfsr, (unsigned long)pc, (unsigned long)lr);
    } else {
        snprintf(buf, sizeof(buf), " fault: N/A\r\n");
    }
    facade_stats_write(buf);
}

static void at_play(void)
{
    s_pending_cmd = 3;
}

static void at_stop(void)
{
    s_pending_cmd = 4;
}

static void at_set_vol(uint8_t vol)
{
    sac_status_t sac_status = SAC_OK;
    uint8_t steps;

    if (device_pairing_state != DEVICE_PAIRED) {
        return;
    }
    /* SAC_VOLUME_TICK = 0.1, so steps = vol/10 (rounded), max 10 steps = 100%. */
    steps = (vol + 5) / 10;
    sac_processing_ctrl(main_channel_volume_processing, main_channel_sac_pipeline,
                        SAC_VOLUME_MUTE, SAC_NO_ARG, &sac_status);
    for (uint8_t i = 0; i < steps; i++) {
        sac_processing_ctrl(main_channel_volume_processing, main_channel_sac_pipeline,
                            SAC_VOLUME_INCREASE, SAC_NO_ARG, &sac_status);
    }
}

static void at_next_track(void)
{
    s_pending_cmd = 1;
}

static void at_pre_track(void)
{
    s_pending_cmd = 2;
}

/** @brief Emit one self-diagnosing trap line to the CDC, then halt.
 *
 *  Prints "<tag> TRAP <file>:<line> code=<n>" naming the assert that tripped (captured
 *  by the ASSERT_SWC/SAC_STATUS wrappers into s_assert_file/s_assert_line), so a repro
 *  is diagnosable from the log alone. facade_print_error_string() lights the RGB red and
 *  raises the USB IRQ to priority 0 so the CDC IN transfer can drain even though we are
 *  about to spin; the short busy-wait below gives that ISR time to push the bytes out
 *  (tick-independent on purpose — SysTick may already be dead in a trap context).
 */
static void fatal_trap(const char *tag, int code)
{
    char buffer[ERROR_MESSAGE_BUFFER_SIZE];
    const char *file = (s_assert_file != NULL) ? (const char *)s_assert_file : "?";
    const char *base = file;

    /* Strip the directory so the log line stays short and readable. */
    for (const char *p = file; *p != '\0'; p++) {
        if (*p == '/' || *p == '\\') {
            base = p + 1;
        }
    }

    snprintf(buffer, sizeof(buffer), "\r\n%s TRAP %s:%lu code=%d\r\n",
             tag, base, (unsigned long)s_assert_line, code);
    facade_print_error_string(buffer);

    /* Let the priority-0 USB ISR drain the one-line message before we stop forever. */
    for (volatile uint32_t i = 0; i < 4000000u; i++) {
        __asm volatile("nop");
    }

    while (1);
}

void sac_error_handler(sac_status_t sac_status)
{
    fatal_trap("SAC", (int)sac_status);
}

void swc_error_handler(swc_error_t swc_status)
{
    fatal_trap("SWC", (int)swc_status);
}

/** @brief Return the accumulator size according to the current fallback mode.
 *
 *  @param[in] pipeline  Pipeline instance.
 *  @return  Accumulator size.
 */
static uint32_t get_accumulator_size(sac_pipeline_t *pipeline)
{
    uint32_t acc_size = 0;
    sac_status_t sac_status = SAC_OK;
    uint8_t curr_fbk_index = 0;

    curr_fbk_index = sac_fallback_get_current_mode(&back_channel_fallback_instance, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    acc_size = (pipeline->producer->cfg.audio_payload_size * back_channel_acc_mul[curr_fbk_index]) /
               back_channel_acc_div[curr_fbk_index];

    return acc_size;
}

/** @brief Iterate through fallback states.
 */
static void change_fallback_state(void)
{
    fallback_state = (fallback_state + 1) % FALLBACK_STATE_COUNT;

    sac_fallback_set_manual_mode(&back_channel_fallback_instance, (fallback_state > FALLBACK_AUTO), NULL);

    if (fallback_state > FALLBACK_AUTO) {
        sac_fallback_set_current_mode(&back_channel_fallback_instance, (fallback_state - 1), NULL);
    }
}
