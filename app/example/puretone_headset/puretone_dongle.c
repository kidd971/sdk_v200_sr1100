/** @file  puretone_headset_coord.c
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
#include "fw_version.h"
#include "at_cmd_core_facade.h"  /* facade_system_reset: AT+LE_UWB_CONNECT reboots into boot auto-reconnect */
#include "pairing_api.h"
#include "pairing_cfg.h"
#include "puretone_headset_facade.h"
#include "puretone_link_data.h"  /* user_data_t: the wire format shared with puretone_headset.c */
#include "reconnect_store.h"  /* boot auto-reconnect: persist/restore the pairing address */
#include "sac_api.h"
#include "sac_cdc.h"
#include "sac_cfg.h"
#include "sac_compression.h"
#include "sac_dummy_endpoint.h"
#include "sac_endpoint_swc.h"
#include "sac_fallback.h"
#include "sac_fallback_gate.h"
#include "sac_hal_facade.h"
#include "sac_mute_packet.h"
#include "sac_packing.h"
#include "sac_sample_accumulator.h"
#include "sac_src_cmsis.h"
#include "sac_stats.h"
#include "sac_volume.h"
#include "swc_api.h"
#include "swc_cfg.h"
#include "swc_cfg_coord.h"
#include "swc_error.h"
#include "swc_stats.h"
#include "swc_utils.h"

/* CONSTANTS ******************************************************************/
/* Total memory needed for the Audio Core. */
#define SAC_MEM_POOL_SIZE 50000
/* Total memory needed for the Wireless Core. */
#define SWC_MEM_POOL_SIZE 10500
/* MAX_DATA_PAYLOAD_SIZE comes from puretone_link_data.h, next to the struct it has to hold. */
/* Length of the statistics array used for terminal display. */
#define STATS_ARRAY_LENGTH 5000
/* Period for data transmission timer in ms.
 * With USB audio, the audio connection stops transmitting when the host is not streaming, making this periodic data
 * transmission the Node's only synchronization source (beacon). This period must not exceed 10 ms so that two data
 * frames fit within the Node's 21 ms sync-loss timeout, tolerating the loss of one frame.
 */
#define DATA_TX_PERIOD_MS 10
/* Boot auto-reconnect: max time to wait for the persisted peer before falling back to pairing. */
#define RECONNECT_TIMEOUT_MS 10000

/* How long without a packet from the node before link_is_up() calls the link down.
 *
 * The node sends one every DATA_TX_PERIOD_MS (10 ms), so this is 20 consecutive misses. Well
 * clear of ordinary loss even at the deepest fallback rung, and far less twitchy than the
 * Wireless Core's own 20 ms threshold, which exists for the audio pipeline rather than for a
 * host-facing event. AT_UWB_DISCONNECT_DEBOUNCE_MS (400 ms) sits on top of this in the AT
 * layer, so the host learns about a node that walked away in roughly 600 ms. */
#define NODE_RX_TIMEOUT_MS 200
/* Period for statistics print timer in ms. */
#define STATS_PRINT_PERIOD_MS 1000
/* Size of the buffer used to print errors. Must hold a full trap line:
 * "<TAG> TRAP <file>:<line> code=<n>". */
#define ERROR_MESSAGE_BUFFER_SIZE 96
/* Interval to print statistics in ms. */
#define PRINT_INTERVAL_MS 1000
/* Certification-mode heartbeat: status LED toggles every this many ms (~2 Hz blink,
 * deliberately faster than the pairing patterns so cert mode is visually distinct). */
#define CERTIF_LED_TOGGLE_MS 250

/* **** Link watch ****
 * DG-side diagnostic counterpart of the HS link_watch. Prints one line every
 * LINK_WATCH_INTERVAL_MS via facade_stats_write (board-aware: ST-Link VCP / UART4 PC10-PC11
 * on U535, USB CDC elsewhere), focused on the TX
 * side and on what the DG actually learns from the node:
 *   - node_lm : link margin the HS reported back over the data link (drives auto fallback).
 *               If this stays low/0 while the HS prints lm=255, the HS->DG back channel
 *               is not delivering, so the DG keeps audio pinned at the worst fallback.
 *   - bk_ok/bk_miss : back-channel (HS->DG) data RX counts -- health of that report path.
 *   - tx_slot/tx_noframe : audio TX timeslots vs slots where the DG had nothing to send.
 *               tx_noframe climbing in lockstep with the HS rx_miss == DG producer starved.
 * All SWC reads are non-asserting so the watch survives a link drop.
 * Set LINK_WATCH to 0 to compile it out. */
#ifndef LINK_WATCH
#define LINK_WATCH 1
#endif
/* Poll/print cadence for the link watch in ms. */
#define LINK_WATCH_INTERVAL_MS 1000
/* Temporarily silence the per-second statistics dump so the CDC port only shows the
 * link watch. Set back to 1 to restore the normal stats print. */
#define STATS_PRINT_ENABLED 0

/* **** CDC **** */
/* Maximum amount of drift to compensate. */
#define MAX_DRIFT_PPM 50

/* Number of SWC fallback modes. */
#define SWC_FALLBACK_MODE_COUNT 3

/* **** Standby test hook (bench only — off in product builds) ****
 * Set to 1 to bind USER_3 to at_start_disconnect(), i.e. the Standby power-down, so the
 * sleep path can be exercised from the board rather than only over AT+LE_UWB_DISCONNECT.
 * Mirrors the HS side. Left at 0 so USER_3 keeps the back-channel volume control. */
#ifndef STANDBY_TEST_HOOKS
#define STANDBY_TEST_HOOKS 0
#endif

/* **** Assert-site capture (no ST-Link needed) ****
 * The library ASSERT_SWC/SAC_STATUS macros only pass the error CODE to the fatal
 * handler, not where the assert tripped. Re-define them for THIS translation unit
 * so they stash __FILE__/__LINE__ into the globals below before trapping; the fatal
 * handler then prints "<TAG> TRAP <file>:<line> code=<n>". Mirrors the HS side.
 * Matters most on the app_init() re-entry path (AT+LE_UWB_CONNECT, boot auto-reconnect):
 * every step there is asserted, so without this a failed re-init is an anonymous wedge. */
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

/* TYPES **********************************************************************/
/** @brief Enumeration representing device pairing states.
 */
typedef enum device_pairing_state {
    /*! The device is unpaired with the Node. */
    DEVICE_UNPAIRED,
    /*! The device pairing is active. */
    DEVICE_PAIRING,
    /*! The device is paired with the Node. */
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
/* user_data_t now lives in puretone_link_data.h -- see that file for why the two hand-copied
 * definitions were merged and what the rules are for extending it. */

/** @brief Enumeration representing the fallback states.
 */
typedef enum fallback_states {
    /*! Default fallback state with automatic algorithm. */
    FALLBACK_AUTO,
    /*! Forced fallback state to 96kHz/24-bit uncompressed audio. */
    FALLBACK_96K_24BIT_UNCOMPRESSED,
    /*! Forced fallback state to 48kHz/24-bit uncompressed audio. */
    FALLBACK_48K_24BIT_UNCOMPRESSED,
    /*! Forced fallback state to 48kHz/16-bit uncompressed audio. */
    FALLBACK_48K_16BIT_UNCOMPRESSED,
    /*! Forced fallback state to 48kHz compressed audio. */
    FALLBACK_48K_ADPCM_STEREO,
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

/* Sample format of main channel audio samples received via USB. */
static const sac_sample_format_t USB_SAC_SAMPLE_FORMAT = {
    .bit_depth = SAC_24BITS,
    .sample_encoding = SAC_SAMPLE_PACKED,
};

/* Sample format of audio samples received by the SWC of the Coordinator. */
static const sac_sample_format_t BACK_CHANNEL_SAC_SAMPLE_FORMAT = {
    .bit_depth = SAC_16BITS,
    .sample_encoding = SAC_SAMPLE_PACKED,
};

/* Sample format of uncompressed audio samples sent over the air (normal operation, fallback modes 0-1). */
static const sac_sample_format_t MAIN_CHANNEL_OTA_UNCOMPRESSED_SAC_SAMPLE_FORMAT = {
    .bit_depth = MAIN_CHANNEL_OTA_UNCOMPRESSED_BIT_DEPTH,
    .sample_encoding = SAC_SAMPLE_PACKED,
};

/* Sample format of packed audio samples sent over the air in fallback mode 2. */
static const sac_sample_format_t MAIN_CHANNEL_OTA_PACKED_FBK_SAC_SAMPLE_FORMAT = {
    .bit_depth = MAIN_CHANNEL_OTA_PACKED_BIT_DEPTH,
    .sample_encoding = SAC_SAMPLE_PACKED,
};

#define MAIN_CHANNEL_PRODUCER_SAC_SAMPLE_FORMAT (USB_AUDIO_ENABLED ? USB_SAC_SAMPLE_FORMAT : I2S_SAC_SAMPLE_FORMAT)

#define BACK_CHANNEL_CONSUMER_SAC_SAMPLE_FORMAT \
    (USB_AUDIO_ENABLED ? BACK_CHANNEL_SAC_SAMPLE_FORMAT : I2S_SAC_SAMPLE_FORMAT)

static uint8_t audio_memory_pool[SAC_MEM_POOL_SIZE];
static sac_pipeline_t *main_channel_sac_pipeline;
static sac_pipeline_t *back_channel_sac_pipeline;
static sac_pipeline_t *back_channel_accumulator_pipeline;

/* **** Main Channel Processing Stages **** */
static sac_fallback_instance_t main_channel_fallback_instance;
static sac_processing_t *main_channel_fallback_processing;
static sac_packing_instance_t main_channel_packing_instance;
static sac_processing_t *main_channel_packing_processing;
static sac_packing_instance_t main_channel_fbk_packing_instance;
static sac_processing_t *main_channel_fbk_packing_processing;
static sac_compression_instance_t main_channel_compression_instance;
static sac_processing_t *main_channel_compression_processing;
static sac_processing_t *main_channel_compression_discard_processing;
static sac_sample_accumulator_instance_t main_channel_sample_accumulator_instance;
static sac_processing_t *main_channel_sample_accumulator_processing;
static src_cmsis_instance_t main_channel_downsampling_instance;
static sac_processing_t *main_channel_downsampling_processing;
static sac_processing_t *main_channel_downsampling_discard_processing;
static sac_mute_packet_instance_t main_channel_mute_packet_instance;
static sac_processing_t *main_channel_mute_packet_processing;

/* **** Back Channel Processing Stages **** */
static sac_fallback_instance_t back_channel_fallback_instance;
static sac_processing_t *back_channel_fallback_processing;
static sac_compression_instance_t back_channel_decompression_instance;
static sac_processing_t *back_channel_decompression_processing;
static sac_packing_instance_t back_channel_unpacking_instance;
static sac_processing_t *back_channel_unpacking_processing;
static sac_processing_t *back_channel_cdc_processing;
static sac_sample_accumulator_instance_t back_channel_sample_accumulator_instance;
static sac_processing_t *back_channel_sample_accumulator_processing;
static src_cmsis_instance_t back_channel_upsampling_instance;
static sac_processing_t *back_channel_upsampling_processing;
static sac_volume_instance_t back_channel_volume_instance;
static sac_processing_t *back_channel_volume_processing;

/* **** Endpoints **** */
static sac_endpoint_t *main_channel_producer_endpoint;
static ep_swc_instance_t main_channel_swc_consumer_instance;
static sac_endpoint_t *main_channel_swc_consumer_endpoint;
static sac_endpoint_t *back_channel_consumer_endpoint;
static ep_swc_instance_t back_channel_swc_producer_instance;
static sac_endpoint_t *back_channel_swc_producer_endpoint;
static sac_endpoint_t *dummy_audio_consumer;
static sac_endpoint_t *dummy_audio_producer;

/* **** Wireless Core **** */
static uint8_t swc_memory_pool[SWC_MEM_POOL_SIZE];

/* ** TX Connections ** */
static swc_connection_t *tx_audio_conn;
static swc_connection_t *tx_data_conn;

/* ** RX Connections ** */
static swc_connection_t *rx_audio_conn;
static swc_connection_t *rx_data_conn;

static const uint32_t timeslot_us[] = SCHEDULE;
static const uint32_t channel_sequence[] = CHANNEL_SEQUENCE;
static const uint32_t channel_frequency[] = CHANNEL_FREQ;

/* There is a bidirectional link for audio and a bidirectional link for data with a lower connection priority. */
static const int32_t tx_timeslots[] = COORD_TIMESLOTS;
static const int32_t rx_timeslots[] = NODE_TIMESLOTS;

/* Non-fatal error tracking for the 10 ms data path (was ASSERT_SWC_STATUS -> while(1)). */
static volatile swc_error_t s_last_fb_info_err;  /* last err from swc_connection_get_fallback_info */
static volatile swc_error_t s_last_send_err;     /* last err from swc_connection_send */
static volatile uint32_t s_send_err_count;       /* cumulative data-send failures */
/* Last link margin the node reported over the back channel (fed to the audio fallback). */
static volatile uint8_t s_node_rx_lm;
/* When the node's 10 ms data packet last arrived, and whether one ever has since the
 * connections were built. This is what link_is_up() answers with -- see there. Written from
 * the wireless RX callback, read from the main loop; a 32-bit store is atomic on this core. */
static volatile uint32_t s_node_rx_tick;
static volatile bool     s_node_rx_seen;
/* Media command queued for the next 10 ms data packet, an at_cmd_code_t. Edge triggered:
 * data_callback() clears it right after packing, so two AT commands issued inside the same
 * 10 ms window mean the first one never reaches the air. Written from AT command context and
 * read from the data timer, hence volatile. */
static volatile uint8_t s_pending_cmd = AT_CMD_NONE;

/* **** Application Specific **** */
static facade_certification_mode_t certification_mode;
static fallback_states_t fallback_state;
/* Variables supporting pairing between the two devices. */
static device_pairing_state_t device_pairing_state;
static pairing_cfg_t app_pairing_cfg;
static pairing_assigned_address_t pairing_assigned_address;
static pairing_discovery_list_t pairing_discovery_list[PAIRING_DISCOVERY_LIST_SIZE];
/* True while try_boot_reconnect() owns the half-open link and is polling it. During that
 * window device_pairing_state is already DEVICE_PAIRED, so any teardown reached from the
 * polled button / AT handlers (unpair_device(), at_start_disconnect()) would NULL the
 * connection handles under the polling loop -- and swc_connection_get_connect_status()
 * dereferences its argument with no NULL check. Teardown is therefore deferred: the
 * handlers only raise s_boot_reconnect_abort and the loop unwinds itself. */
static volatile bool s_boot_reconnect_active;
static volatile bool s_boot_reconnect_abort;
static sac_cdc_instance_t back_channel_cdc_instance;

/* Fallback latency. */
uint8_t back_channel_fbk_latency_queue_size[] = BACK_CHANNEL_FALLBACK_LATENCY_QUEUE_SIZE;
uint8_t back_channel_fbk_latency_usb_fifo_size[] = BACK_CHANNEL_FALLBACK_LATENCY_USB_FIFO_SIZE;

/* Main channel audio sample accumulator settings. */
uint8_t main_channel_acc_mul[] = MAIN_CHANNEL_ACC_MUL;
uint8_t main_channel_acc_div[] = MAIN_CHANNEL_ACC_DIV;

static volatile uint32_t back_channel_trigger_count;

/* PRIVATE FUNCTION PROTOTYPE *************************************************/
static void app_init(void);
static void app_swc_core_init(pairing_assigned_address_t *app_pairing, swc_error_t *swc_err);
static void app_audio_core_init(void);

/* **** Callbacks **** */
/* Callbacks that are used for the main channel. */
static void conn_tx_audio_success_callback(void *conn, void *arg);
static void conn_rx_data_success_callback(void *conn, void *arg);
static void main_channel_audio_rx_complete_callback(void);
static void audio_process_main_channel_callback(void);
/* Callbacks that are used for the back channel. */
static void conn_tx_data_success_callback(void *conn, void *arg);
static void conn_rx_audio_success_callback(void *conn, void *arg);
static void back_channel_audio_tx_complete_callback(void);
static void audio_process_back_channel_callback(void);
/* Callbacks that are used for data and pairing processes. */
static void data_callback(void);
static void pairing_process_callback(void);
static void pairing_button_callback(void);
/* Unreferenced while STANDBY_TEST_HOOKS owns USER_3; kept so setting that flag to 0
 * restores the back-channel volume control unchanged. */
static void volume_up(void) __attribute__((unused));
static void volume_down(void);
static void change_fallback_state(void);

/* **** Processing Stages **** */
/* Processing stages that are used for the main channel. */
static void app_audio_core_fallback_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_downsampling_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_downsampling_discard_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_packing_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_mute_packet_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_compressing_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_compression_discard_interface_init(sac_processing_interface_t *iface);
/* Processing stages that are used for the back channel. */
static void app_audio_core_upsampling_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_cdc_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_decompressing_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_unpacking_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_volume_interface_init(sac_processing_interface_t *iface);

/* Outcome of a boot auto-reconnect attempt. */
typedef enum {
    BOOT_RECONNECT_OK,   /* The stored link was re-established; stay paired and stream. */
    BOOT_RECONNECT_PAIR, /* No usable record, or the user asked to pair mid-attempt: enter pairing. */
    BOOT_RECONNECT_IDLE, /* Had a record but the node was not up yet: keep the coordinator's core
                            running (it is the timebase master) so the node syncs whenever it boots. */
} boot_reconnect_result_t;

/* **** Button Actions **** */
static void enter_pairing_mode(void);
static void unpair_device(bool forget_peer);
static boot_reconnect_result_t try_boot_reconnect(void);
static void abort_pairing_procedure(void);

/* **** Fallback LED and Terminal Display **** */
static bool should_print_stats(void);
static void print_stats(void);
#if LINK_WATCH
static void link_watch(void);
#endif

static void wireless_send_data(const void *transmitted_data, uint8_t size, swc_error_t *swc_err);
static uint16_t wireless_read_data(void *received_data, uint8_t size, swc_error_t *swc_err);
static void fatal_trap(const char *tag, int code);
static uint32_t get_accumulator_size(sac_pipeline_t *pipeline);

/* **** AT Command Core Callbacks **** */
static void at_start_pairing(void);
static void at_start_connect(void);
static void at_start_disconnect(void);
static void app_teardown(void);
static void at_start_shutdown(void);
static bool link_is_up(void);
static bool at_get_link_status(void);
static int32_t at_get_link_margin(void);
static void at_set_vol(uint8_t vol);
static void at_cmd_tx(uint8_t cmd_type, uint8_t value);

/* PUBLIC FUNCTIONS ***********************************************************/
int main(void)
{
#if USB_AUDIO_ENABLED
    /* Configure usb audio before board initialization. */
    facade_configure_coord_usb_audio();
#endif
    /* Initialize the board and all GPIOs and peripherals for minimal operations. */
    facade_board_init();

    /* Initialize wireless core context switch handler before pairing is available. */
    facade_set_context_switch_handler(swc_connection_callbacks_processing_handler);

    facade_button_callbacks_t button_callbacks = {
        .pairing_callback = pairing_button_callback,
#if STANDBY_TEST_HOOKS
        /* USER_3 drives the Standby power-down (see STANDBY_TEST_HOOKS). */
        .volume_up_callback = at_start_disconnect,
#else
        .volume_up_callback = volume_up,
#endif
        .volume_down_callback = volume_down,
        .fallback_callback = change_fallback_state,
    };
    facade_set_button_callbacks(button_callbacks);

    at_cmd_core_init();
    at_cmd_core_set_device_role(AT_DEVICE_ROLE_COORDINATOR);
    at_cmd_core_register_pair_cb(at_start_pairing);
    at_cmd_core_register_connect_cb(at_start_connect);
    at_cmd_core_register_disconnect_cb(at_start_disconnect);
    at_cmd_core_register_shutdown_cb(at_start_shutdown);
    at_cmd_core_register_link_status_cb(at_get_link_status);
    at_cmd_core_register_link_margin_cb(at_get_link_margin);
    /* AT+VOL on this side means "my back channel", applied locally -- see at_set_vol() and
     * at_cmd_core_register_vol_cb(). The media keys are the opposite: they mean "tell the
     * headset", so they go through the forwarding hook. Registering it is what finally makes
     * AT+PLAY / STOP / NEXT_TRACK / PRE_TRACK do something here; until now they returned OK
     * with no callback registered at either end, which is the worst kind of broken because
     * the host cannot tell it from success. */
    at_cmd_core_register_vol_cb(at_set_vol);
    at_cmd_core_register_cmd_tx_cb(at_cmd_tx);
    at_cmd_core_register_i2s_mux_cb(facade_set_i2s_mux);
    /* Version on the debug console, once. It used to prefix every statistics line at 2 Hz,
     * which is a great deal of repetition to answer a question -- "which binary produced
     * this log?" -- that needs answering once per boot, and that the timestamp printed
     * beside it answers more precisely anyway.
     *
     * facade_stats_write, not facade_print_string: the two are different channels here.
     * print_string is the weak USB CDC implementation on this application, while the
     * statistics go out over the ST-Link UART on u535 and only fall back to CDC on u5a5.
     * A banner whose entire job is to label a log has to arrive on the same channel as
     * the log, or it labels nothing. */
    {
        char banner[80];

        snprintf(banner, sizeof(banner), "\r\n[BOOT] puretone_dongle " FW_VERSION_STRING " " __DATE__ " " __TIME__ "\r\n");
        facade_stats_write(banner);
    }

    /* Boot banner, ahead of UWB_READY. The DG has no periodic crash dump and its LINK_WATCH
     * output goes to the ST-Link VCP (UART4), which a customer board does not necessarily
     * wire out -- so on the AT port this pair of lines is the only evidence of a boot. A
     * BUILD line that reappears every ~10 s means the module is resetting; a single one
     * followed by silence means boot reconnect timed out and the coordinator is sitting in
     * BOOT_RECONNECT_IDLE, which is otherwise indistinguishable over this port. */
    at_cmd_core_notify_build(AT_CMD_CORE_BUILD_ID);
    at_cmd_core_notify_uwb_ready();

    /* Audio process timer initialization. */
    facade_audio_process_main_channel_timer_init(audio_process_main_channel_callback);
    facade_audio_process_back_channel_timer_init(audio_process_back_channel_callback);

    /* Timer that updates statistics display every second and transmits button state to the Node at the
     * DATA_TX_PERIOD_MS interval.
     */
    facade_data_timer_init(DATA_TX_PERIOD_MS);
    facade_data_timer_set_callback(data_callback);

    certification_mode = facade_get_coord_certification_mode();
    if (certification_mode != FACADE_CERTIF_NONE) {
        /* Init app in certification mode. */
        facade_notify_certification_mode();
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
            if (STATS_PRINT_ENABLED && should_print_stats()) {
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
     * could not reach its node in time does NOT re-pair: the coordinator keeps its
     * wireless core running (it is the timebase master) and the node syncs whenever
     * it boots -- the main loop's AT status machine emits UWB_CONNECTED then. */
    if (try_boot_reconnect() == BOOT_RECONNECT_PAIR) {
        enter_pairing_mode();
    }

    while (1) {
        facade_button_handling();
        at_cmd_core_process();

        /* Statistics are displayed at intervals set by the timer when paired; timer stops if unpaired. */
        if (STATS_PRINT_ENABLED && should_print_stats()) {
            print_stats();
        }

#if LINK_WATCH
        link_watch();
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
    uint8_t remote_address = pairing_discovery_list[PAIRING_DEVICE_ROLE_NODE].node_address;
    uint8_t local_address = pairing_discovery_list[PAIRING_DEVICE_ROLE_COORDINATOR].node_address;
    const uint8_t fallback_thresholds[] = MAIN_CHANNEL_FALLBACK_PAYLOAD_SIZE;

    /* swc_connection_set_fallback_cfg() requires these in descending order and asserts if they
     * are not -- a red LED at init, well after the edit that caused it. The accumulator ratio
     * feeds the bottom rung's size, so the ladder can be inverted by changing one number in
     * sac_cfg.h; catch it here instead. The uint8_t bound matters for the same reason: a large
     * enough ratio silently wraps the threshold rather than overflowing it. */
    _Static_assert(sizeof(fallback_thresholds) == SWC_FALLBACK_MODE_COUNT,
                   "fallback threshold count must match SWC_FALLBACK_MODE_COUNT");
    _Static_assert(MAIN_CHANNEL_FBK_3_PAYLOAD_SIZE < MAIN_CHANNEL_FBK_2_PAYLOAD_SIZE &&
                       MAIN_CHANNEL_FBK_2_PAYLOAD_SIZE < MAIN_CHANNEL_FBK_1_PAYLOAD_SIZE,
                   "SWC fallback thresholds must stay in descending payload order");
    _Static_assert(MAIN_CHANNEL_FBK_1_PAYLOAD_SIZE <= UINT8_MAX,
                   "fallback thresholds are uint8_t; a larger payload wraps silently");

    const uint8_t fallback_cca_try_count[] = {
        SWC_CCA_AUDIO_FBK_1_TRY_COUNT,
        SWC_CCA_AUDIO_FBK_2_TRY_COUNT,
        SWC_CCA_AUDIO_FBK_3_TRY_COUNT,
    };
    swc_radio_handle_t *radio_handle = NULL;

    if (certification_mode != FACADE_CERTIF_NONE) {
        app_pairing->pan_id = 0xABC;
        remote_address = 0x2;
        local_address = 0x1;
    }

    /* Initialize Wireless Core. */
    const swc_cfg_t core_cfg = {
        .timeslot_sequence = timeslot_us,
        .timeslot_sequence_length = ARRAY_SIZE(timeslot_us),
        .channel_sequence = channel_sequence,
        .channel_sequence_length = ARRAY_SIZE(channel_sequence),
        .concurrency_mode = SWC_CONCURRENCY_MODE_HIGH_PERFORMANCE,
        .memory_pool = swc_memory_pool,
        .memory_pool_size = SWC_MEM_POOL_SIZE,
        .pan_id = app_pairing->pan_id,
    };

    /* Initialize Node. */
    const swc_node_cfg_t node_cfg = {
        .role = SWC_ROLE_COORDINATOR,
        .coordinator_address = local_address,
        .local_address = local_address,
        .isi_mitig = NODE_ISI_MITIG,
    };
    swc_init(core_cfg, node_cfg, facade_context_switch_trigger, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Calibrate the radio. */
    radio_handle = swc_radio_module_calib(SWC_RADIO_ID_1, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Initialize the radio. */
    swc_radio_module_init(radio_handle, false, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* **** TX Connections **** */
    /* ** Main Channel: TX Audio Connection ** */
    swc_connection_cfg_t tx_audio_conn_cfg = {
        .name = "TX Audio Connection",
        .source_address = local_address,
        .destination_address = remote_address,
        .max_payload_size = MAIN_CHANNEL_SWC_PAYLOAD_SIZE + sizeof(sac_header_t),
        .queue_size = SWC_QUEUE_SIZE + (USB_AUDIO_ENABLED ? MAIN_CHANNEL_USB_FS_PRODUCER_BUFFERING : 0),
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
        /* Change the connection's max payload size to match the certification mode. */
        if (certification_mode == FACADE_CERTIF_AUDIO_48k_24_BIT) {
            tx_audio_conn_cfg.max_payload_size = fallback_thresholds[0];
        } else if (certification_mode == FACADE_CERTIF_AUDIO_48k_16_BIT) {
            tx_audio_conn_cfg.max_payload_size = fallback_thresholds[1];
        } else if (certification_mode == FACADE_CERTIF_AUDIO_48k_ADPCM) {
            tx_audio_conn_cfg.max_payload_size = fallback_thresholds[2];
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
    const swc_connection_concurrency_cfg_t tx_audio_concurrency_cfg = {
        .enabled = true,
        .try_count = MAIN_CHANNEL_SWC_CCA_AUDIO_TRY_COUNT,
        .retry_time = MAIN_CHANNEL_SWC_CCA_AUDIO_RETRY_TIME,
        .fail_action = SWC_CCA_ABORT_TX,
    };

    swc_connection_set_concurrency_cfg(tx_audio_conn, &tx_audio_concurrency_cfg, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Audio connection fallback settings. */
    const swc_connection_fallback_cfg_t fallback_cfg = {
        .enabled = true,
        .fallback_mode_count = SWC_FALLBACK_MODE_COUNT,
        .thresholds = fallback_thresholds,
        .cca_try_count = fallback_cca_try_count,
    };

    swc_connection_set_fallback_cfg(tx_audio_conn, &fallback_cfg, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Audio connection RF channels settings. */
    const uint8_t tx_audio_pulse_width[] = TX_AUDIO_PULSE_WIDTH;
    const uint8_t tx_audio_pulse_gain[] = TX_AUDIO_PULSE_GAIN;

    const uint8_t tx_audio_fb_pulse_width[][MAX_CHANNEL_NUMBER] = {
        TX_AUDIO_FB_BAND_1_PULSE_WIDTH,
        TX_AUDIO_FB_BAND_2_PULSE_WIDTH,
        TX_AUDIO_FB_BAND_3_PULSE_WIDTH,
        TX_AUDIO_FB_BAND_4_PULSE_WIDTH,
    };

    const uint8_t tx_audio_fb_pulse_gain[][MAX_CHANNEL_NUMBER] = {
        TX_AUDIO_FB_BAND_1_PULSE_GAIN,
        TX_AUDIO_FB_BAND_2_PULSE_GAIN,
        TX_AUDIO_FB_BAND_3_PULSE_GAIN,
        TX_AUDIO_FB_BAND_4_PULSE_GAIN,
    };

    swc_channel_cfg_t tx_audio_channel_cfgs[ARRAY_SIZE(channel_frequency)];

    for (uint8_t i = 0; i < ARRAY_SIZE(channel_frequency); i++) {
        tx_audio_channel_cfgs[i] = (swc_channel_cfg_t){
            .tx_pulse_count = SR1100_PULSE_COUNT,
            .rx_pulse_count = SR1100_PULSE_COUNT,
            .frequency = channel_frequency[i],
            .tx_pulse_width = tx_audio_pulse_width[i],
            .tx_pulse_gain = tx_audio_pulse_gain[i],
        };
    }

    swc_channel_t *tx_audio_channels = swc_channel_list_init(tx_audio_channel_cfgs, ARRAY_SIZE(channel_frequency),
                                                             swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    swc_connection_set_channels(tx_audio_conn, tx_audio_channels, ARRAY_SIZE(channel_frequency), swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    for (uint8_t j = 0; j < SWC_FALLBACK_MODE_COUNT; j++) {
        swc_channel_cfg_t tx_audio_fb_channel_cfgs[ARRAY_SIZE(channel_frequency)];

        for (uint8_t i = 0; i < ARRAY_SIZE(channel_frequency); i++) {
            tx_audio_fb_channel_cfgs[i] = (swc_channel_cfg_t){
                .tx_pulse_count = SR1100_PULSE_COUNT,
                .rx_pulse_count = SR1100_PULSE_COUNT,
                .frequency = channel_frequency[i],
                .tx_pulse_width = tx_audio_fb_pulse_width[i][j],
                .tx_pulse_gain = tx_audio_fb_pulse_gain[i][j],
            };
        }

        swc_channel_t *tx_audio_fb_channels = swc_channel_list_init(tx_audio_fb_channel_cfgs,
                                                                    ARRAY_SIZE(channel_frequency), swc_err);
        ASSERT_SWC_STATUS(*swc_err);

        swc_connection_set_fallback_channels(tx_audio_conn, tx_audio_fb_channels, ARRAY_SIZE(channel_frequency), j,
                                             swc_err);
        ASSERT_SWC_STATUS(*swc_err);
    }

    /* Audio connection callback settings. */
    swc_connection_set_tx_success_callback(tx_audio_conn, conn_tx_audio_success_callback, NULL, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Data connection concurrency settings. */
    const swc_connection_concurrency_cfg_t tx_data_concurrency_cfg = {
        .enabled = true,
        .try_count = MAIN_CHANNEL_SWC_CCA_DATA_TRY_COUNT,
        .retry_time = MAIN_CHANNEL_SWC_CCA_DATA_RETRY_TIME,
        .fail_action = SWC_CCA_ABORT_TX,
    };

    swc_connection_set_concurrency_cfg(tx_data_conn, &tx_data_concurrency_cfg, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Data connection RF channels settings. */
    const uint8_t tx_data_pulse_width[] = TX_DATA_PULSE_WIDTH;
    const uint8_t tx_data_pulse_gain[] = TX_DATA_PULSE_GAIN;

    swc_channel_cfg_t tx_data_channel_cfgs[ARRAY_SIZE(channel_frequency)];

    for (uint8_t i = 0; i < ARRAY_SIZE(channel_frequency); i++) {
        tx_data_channel_cfgs[i] = (swc_channel_cfg_t){
            .tx_pulse_count = SR1100_PULSE_COUNT,
            .rx_pulse_count = SR1100_PULSE_COUNT,
            .frequency = channel_frequency[i],
            .tx_pulse_width = tx_data_pulse_width[i],
            .tx_pulse_gain = tx_data_pulse_gain[i],
        };
    }

    swc_channel_t *tx_data_channels = swc_channel_list_init(tx_data_channel_cfgs, ARRAY_SIZE(channel_frequency),
                                                            swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    swc_connection_set_channels(tx_data_conn, tx_data_channels, ARRAY_SIZE(channel_frequency), swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Data connection callback settings. */
    swc_connection_set_tx_success_callback(tx_data_conn, conn_tx_data_success_callback, NULL, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* **** RX Connections **** */
    /* ** Back Channel: RX Audio Connection ** */
    const swc_connection_cfg_t rx_audio_conn_cfg = {
        .name = "RX Audio Connection",
        .source_address = remote_address,
        .destination_address = local_address,
        .max_payload_size = BACK_CHANNEL_SWC_PAYLOAD_SIZE + sizeof(sac_header_t),
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
    const swc_connection_concurrency_cfg_t rx_audio_concurrency_cfg = {
        .enabled = true,
        .try_count = BACK_CHANNEL_SWC_CCA_FB_TRY_COUNT, /* Use maximum CCA try count on this connection. */
        .retry_time = BACK_CHANNEL_SWC_CCA_AUDIO_RETRY_TIME,
        .fail_action = SWC_CCA_ABORT_TX,
    };

    swc_connection_set_concurrency_cfg(rx_audio_conn, &rx_audio_concurrency_cfg, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Audio connection RF channels settings. */
    const uint8_t tx_ack_pulse_width[] = TX_ACK_PULSE_WIDTH;
    const uint8_t tx_ack_pulse_gain[] = TX_ACK_PULSE_GAIN;

    swc_channel_cfg_t rx_audio_channel_cfgs[ARRAY_SIZE(channel_frequency)];

    for (uint8_t i = 0; i < ARRAY_SIZE(channel_frequency); i++) {
        rx_audio_channel_cfgs[i] = (swc_channel_cfg_t){
            .tx_pulse_count = SR1100_PULSE_COUNT,
            .rx_pulse_count = SR1100_PULSE_COUNT,
            .frequency = channel_frequency[i],
            .tx_pulse_width = tx_ack_pulse_width[i],
            .tx_pulse_gain = tx_ack_pulse_gain[i],
        };
    }

    swc_channel_t *rx_audio_channels = swc_channel_list_init(rx_audio_channel_cfgs, ARRAY_SIZE(channel_frequency),
                                                             swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    swc_connection_set_channels(rx_audio_conn, rx_audio_channels, ARRAY_SIZE(channel_frequency), swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Audio connection priority settings. */
    swc_connection_set_connection_priority(rx_audio_conn, AUDIO_CONNECTION_PRIORITY, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Audio connection callback settings. */
    swc_connection_set_rx_success_callback(rx_audio_conn, conn_rx_audio_success_callback, NULL, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* ** RX Data Connection ** */
    const swc_connection_cfg_t rx_data_conn_cfg = {
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
    const swc_connection_concurrency_cfg_t rx_data_concurrency_cfg = {
        .enabled = true,
        .try_count = BACK_CHANNEL_SWC_CCA_DATA_TRY_COUNT,
        .retry_time = BACK_CHANNEL_SWC_CCA_DATA_RETRY_TIME,
        .fail_action = SWC_CCA_ABORT_TX,
    };

    swc_connection_set_concurrency_cfg(rx_data_conn, &rx_data_concurrency_cfg, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Data connection RF channels settings. */
    const uint8_t tx_data_ack_pulse_width[] = TX_DATA_ACK_PULSE_WIDTH;
    const uint8_t tx_data_ack_pulse_gain[] = TX_DATA_ACK_PULSE_GAIN;

    swc_channel_cfg_t rx_data_channel_cfgs[ARRAY_SIZE(channel_frequency)];

    for (uint8_t i = 0; i < ARRAY_SIZE(channel_frequency); i++) {
        rx_data_channel_cfgs[i] = (swc_channel_cfg_t){
            .tx_pulse_count = SR1100_PULSE_COUNT,
            .rx_pulse_count = SR1100_PULSE_COUNT,
            .frequency = channel_frequency[i],
            .tx_pulse_width = tx_data_ack_pulse_width[i],
            .tx_pulse_gain = tx_data_ack_pulse_gain[i],
        };
    }

    swc_channel_t *rx_data_channels = swc_channel_list_init(rx_data_channel_cfgs, ARRAY_SIZE(channel_frequency),
                                                            swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    swc_connection_set_channels(rx_data_conn, rx_data_channels, ARRAY_SIZE(channel_frequency), swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Data connection priority settings. */
    swc_connection_set_connection_priority(rx_data_conn, DATA_CONNECTION_PRIORITY, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Data connection callback settings. */
    swc_connection_set_rx_success_callback(rx_data_conn, conn_rx_data_success_callback, NULL, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Handle certification mode. */
    swc_set_certification_mode(certification_mode != FACADE_CERTIF_NONE, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Setup Wireless Core. */
    swc_setup(swc_err);
    ASSERT_SWC_STATUS(*swc_err);
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

    /* Trigger main channel process. */
    facade_audio_process_main_channel_timer_trigger();
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

    /* The SWC produces audio samples upon receiving them from the Node. */
    sac_pipeline_produce(back_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Trigger back channel process. */
    facade_audio_process_back_channel_timer_trigger();
    back_channel_trigger_count++;
}

/* DG audio production counter — ALWAYS compiled (not gated on SINE_DEBUG_CAPTURE) so the produce
 * rate can be read over UART on no-SINE builds: LINK_WATCH prints its per-second delta as prod=<n>/s.
 * Incremented once per main-channel audio buffer produced (SAI RX-DMA complete). Expect 2400/s
 * (96000/40); a sustained value below that = producer starved = the "dead air" the node sees. */
volatile uint32_t dbg_dg_produce_cnt = 0;

#ifdef SINE_DEBUG_CAPTURE
/* DG-side TX observability. The rest are refreshed every ~10 ms in
 * conn_rx_data_success_callback. Compare the per-second deltas:
 *   dbg_tx_queue_load   = DG TX (radio) consumer buffer load; climbing/full => produce faster than
 *                         the radio can send => buffers dropped before TX.
 *   dbg_tx_acked        = packets actually sent+acked/sec.
 *   dbg_tx_dropped      = Wireless Core timeout drops (produce faster than TX).
 *   dbg_tx_notx         = TX timeslots with nothing to send (producer starved).
 *   dbg_tx_cca_fail     = CCA fails (channel busy). */
volatile uint32_t dbg_tx_queue_load  = 0;
volatile uint32_t dbg_tx_acked       = 0;
volatile uint32_t dbg_tx_dropped     = 0;
volatile uint32_t dbg_tx_notx        = 0;
volatile uint32_t dbg_tx_cca_fail    = 0;
#endif

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
        sac_fallback_set_rx_link_margin(&main_channel_fallback_instance, received_user_data.link_margin, &sac_status);
        ASSERT_SAC_STATUS(sac_status);
        /* Snapshot for link_watch: this is the margin the DG actually uses to pick the
         * fallback mode. Compare against the lm the HS prints to spot a dead back channel. */
        s_node_rx_lm = received_user_data.link_margin;

        /* Peer heartbeat for link_is_up(). Stamped here because reaching this line is the
         * only direct evidence this side ever gets that the node is alive. */
        s_node_rx_tick = facade_get_tick_ms();
        s_node_rx_seen = true;

        /* Forward commands from node to SOC via UART. AT_CMD_NONE falls through silently:
         * it is what every packet without a pending command carries. */
        switch (received_user_data.cmd_type) {
        case AT_CMD_NEXT_TRACK:
            at_cmd_core_notify_next_track_received();
            break;
        case AT_CMD_PRE_TRACK:
            at_cmd_core_notify_pre_track_received();
            break;
        case AT_CMD_PLAY:
            at_cmd_core_notify_play_received();
            break;
        case AT_CMD_STOP:
            at_cmd_core_notify_stop_received();
            break;
        default:
            break;
        }

        /* Vendor pass-through: de-duplication and the +EVENT line happen in the AT core, so
         * this side never needs to know what the command means. */
        user_data_deliver_vendor(&received_user_data);

        /* Cache battery level reported by node. */
        at_cmd_core_set_battery_level(received_user_data.battery_pct);
    }

#ifdef SINE_DEBUG_CAPTURE
    {
        swc_statistics_t *tx_stats = swc_connection_update_stats(tx_audio_conn, &swc_err);

        dbg_tx_queue_load = sac_pipeline_get_consumer_buffer_load(main_channel_sac_pipeline, &sac_status);
        if (tx_stats != NULL) {
            dbg_tx_acked    = tx_stats->packet_sent_and_acked_count;
            dbg_tx_dropped  = tx_stats->packet_dropped_count;
            dbg_tx_notx     = tx_stats->no_packet_tranmission_count;
            dbg_tx_cca_fail = tx_stats->cca_fail_count;
        }
    }
#endif
}

/** @brief Initialize the Audio Core.
 */
static void app_audio_core_init(void)
{
    sac_status_t sac_status = SAC_OK;

    /* ** Endpoint Interfaces ** */
    sac_endpoint_interface_t main_channel_producer_iface = {0};
    sac_endpoint_interface_t main_channel_swc_consumer_iface = {0};
    sac_endpoint_interface_t back_channel_consumer_iface = {0};
    sac_endpoint_interface_t back_channel_swc_producer_iface = {0};

    /* ** Processing Stage Interfaces ** */
    sac_processing_interface_t fallback_iface = {0};
    sac_processing_interface_t main_channel_downsampling_iface = {0};
    sac_processing_interface_t main_channel_downsampling_discard_iface = {0};
    sac_processing_interface_t main_channel_packing_iface = {0};
    sac_processing_interface_t main_channel_mute_packet_iface = {0};
    sac_processing_interface_t main_channel_compression_iface = {0};
    sac_processing_interface_t main_channel_compression_discard_iface = {0};
    sac_processing_interface_t back_channel_upsampling_iface = {0};
    sac_processing_interface_t back_channel_cdc_iface = {0};
    sac_processing_interface_t back_channel_decompression_iface = {0};
    sac_processing_interface_t back_channel_unpacking_iface = {0};
    sac_processing_interface_t back_channel_volume_iface = {0};

    sac_endpoint_swc_init(&back_channel_swc_producer_iface, &main_channel_swc_consumer_iface);
    sac_facade_audio_endpoint_init(&main_channel_producer_iface, &back_channel_consumer_iface);
    facade_set_audio_complete_callback(back_channel_audio_tx_complete_callback,
                                       main_channel_audio_rx_complete_callback);

    app_audio_core_fallback_interface_init(&fallback_iface);
    app_audio_core_downsampling_interface_init(&main_channel_downsampling_iface);
    app_audio_core_downsampling_discard_interface_init(&main_channel_downsampling_discard_iface);
    app_audio_core_packing_interface_init(&main_channel_packing_iface);
    app_audio_core_mute_packet_interface_init(&main_channel_mute_packet_iface);
    app_audio_core_compression_discard_interface_init(&main_channel_compression_discard_iface);
    app_audio_core_compressing_interface_init(&main_channel_compression_iface);

    app_audio_core_upsampling_interface_init(&back_channel_upsampling_iface);
    app_audio_core_cdc_interface_init(&back_channel_cdc_iface);
    app_audio_core_decompressing_interface_init(&back_channel_decompression_iface);
    app_audio_core_unpacking_interface_init(&back_channel_unpacking_iface);
    app_audio_core_volume_interface_init(&back_channel_volume_iface);

    main_channel_swc_consumer_instance.connection = tx_audio_conn;
    back_channel_swc_producer_instance.connection = rx_audio_conn;

    /* Initialize Audio Core. */
    sac_cfg_t core_cfg = {
        .memory_pool = audio_memory_pool,
        .memory_pool_size = SAC_MEM_POOL_SIZE,
    };
    sac_init(core_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /*
     * Main Channel Audio Pipeline (TX)
     * ================================
     *
     * **** NORMAL MODE USB (Fallback mode 0) *****
     * Input:       Stereo stream of 96kHz/24-bit depth samples.
     * Output:      Stereo stream of 96kHz/24-bit is sent over the air to the Node.
     * +-----+    +-----+
     * | USB | -> | SWC |
     * +-----+    +-----+
     *
     * **** NORMAL MODE I2S (Fallback mode 0) *****
     * Input:       Stereo stream of 96kHz/24-bit depth samples, encoded on 32 bits.
     * Processing:  Packing from 32 bits to 24 bits audio samples.
     * Output:      Stereo stream of 96kHz/24-bit is sent over the air to the Node.
     *
     * +-----+    +--------------------+    +-----+
     * | I2S | -> | Packing to 24 bits | -> | SWC |
     * +-----+    +--------------------+    +-----+
     *
     * **** FALLBACK MODE USB (Fallback mode 1) *****
     * Input:       Stereo stream of 96kHz/24-bit packed depth samples.
     * Processing:  Audio sample accumulator 1.7x.
     * Processing:  Downsampling audio samples from 96kHz to 48kHz.
     * Output:      Stereo stream of 48kHz/24-bit is sent over the air to the Node.
     *
     * +-----+    +------------------+    +-----------------+    +-----+
     * | USB | -> | Accumulator 1.7x | -> | Downsampling 2x | -> | SWC |
     * +-----+    +------------------+    +-----------------+    +-----+
     *
     * **** FALLBACK MODE I2S (Fallback mode 1) *****
     * Input:       Stereo stream of 96kHz/24-bit depth samples, encoded on 32 bits.
     * Processing:  Audio sample accumulator 1.7x.
     * Processing:  Downsampling audio samples from 96kHz to 48kHz.
     * Processing:  Packing from 32 bits to 24 bits audio samples.
     * Output:      Stereo stream of 48kHz/24-bit is sent over the air to the Node.
     *
     * +-----+    +------------------+    +-----------------+    +--------------------+    +-----+
     * | I2S | -> | Accumulator 1.7x | -> | Downsampling 2x | -> | Packing to 24 bits | -> | SWC |
     * +-----+    +------------------+    +-----------------+    +--------------------+    +-----+
     *
     * **** FALLBACK MODE USB (Fallback mode 2) *****
     * Input:       Stereo stream of 96kHz/24-bit depth samples.
     * Processing:  Audio sample accumulator 1.7x.
     * Processing:  Downsampling audio samples from 96kHz to 48kHz.
     * Processing:  Packing from 32 bits to 16 bits audio samples.
     * Output:      Stereo stream of 48kHz/16-bit is sent over the air to the Node.
     *
     * +-----+    +------------------+    +-----------------+    +--------------------+    +-----+
     * | USB | -> | Accumulator 1.7x | -> | Downsampling 2x | -> | Packing to 16 bits | -> | SWC |
     * +-----+    +------------------+    +-----------------+    +--------------------+    +-----+
     *
     * **** FALLBACK MODE I2S (Fallback mode 2) *****
     * Input:       Stereo stream of 96kHz/24-bit depth samples, encoded on 32 bits.
     * Processing:  Audio sample accumulator 1.7x.
     * Processing:  Downsampling audio samples from 96kHz to 48kHz.
     * Processing:  Packing from 32 bits to 16 bits audio samples.
     * Output:      Stereo stream of 48kHz/16-bit is sent over the air to the Node.
     *
     * +-----+    +------------------+    +-----------------+    +--------------------+    +-----+
     * | I2S | -> | Accumulator 1.7x | -> | Downsampling 2x | -> | Packing to 16 bits | -> | SWC |
     * +-----+    +------------------+    +-----------------+    +--------------------+    +-----+
     *
     * **** FALLBACK MODE USB (Fallback mode 3) *****
     * Input:       Stereo stream of 96kHz/24-bit depth samples.
     * Processing:  Audio sample accumulator 2.3x.
     * Processing:  Downsampling audio samples from 96kHz to 48kHz.
     * Processing:  Audio compression using ADPCM.
     * Output:      ADPCM compressed stereo stream of 48 kHz/24-bit is sent over the air to the Node.
     *
     * +-----+    +------------------+    +-----------------+    +-------------------+    +-----+
     * | USB | -> | Accumulator 2.3x | -> | Downsampling 2x | -> | ADPCM Compression | -> | SWC |
     * +-----+    +------------------+    +-----------------+    +-------------------+    +-----+
     *
     * **** FALLBACK MODE I2S (Fallback mode 3) *****
     * Input:       Stereo stream of 96kHz/24-bit depth samples, encoded on 32 bits.
     * Processing:  Audio sample accumulator 2.3x.
     * Processing:  Downsampling audio samples from 96kHz to 48kHz.
     * Processing:  Audio compression using ADPCM.
     * Output:      ADPCM compressed stereo stream of 48 kHz/24-bit is sent over the air to the Node.
     *
     * +-----+    +------------------+    +-----------------+    +-------------------+    +-----+
     * | I2S | -> | Accumulator 2.3x | -> | Downsampling 2x | -> | ADPCM Compression | -> | SWC |
     * +-----+    +------------------+    +-----------------+    +-------------------+    +-----+
     */

    /* Initialize codec producer endpoint. */
    sac_endpoint_cfg_t main_channel_producer_cfg = {
        .use_encapsulation = false,
        .delayed_action = !USB_AUDIO_ENABLED,
        .channel_count = MAIN_CHANNEL_CHANNEL_COUNT,
        .audio_payload_size = USB_AUDIO_ENABLED ? MAIN_CHANNEL_USB_PAYLOAD_SIZE : MAIN_CHANNEL_I2S_PAYLOAD_SIZE,
        .queue_size = SAC_MIN_PRODUCER_QUEUE_SIZE + (USB_AUDIO_ENABLED ? MAIN_CHANNEL_USB_FS_PRODUCER_BUFFERING : 0),
    };
    main_channel_producer_endpoint = sac_endpoint_init(NULL, "Audio EP (Producer)", main_channel_producer_iface,
                                                       main_channel_producer_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    main_channel_fallback_instance.connection = tx_audio_conn;
    main_channel_fallback_instance.is_tx_device = true;
    main_channel_fallback_instance.get_tick = facade_get_tick_ms;
    main_channel_fallback_instance.tick_frequency_hz = 1000;
    main_channel_fallback_processing = sac_processing_stage_init(&main_channel_fallback_instance,
                                                                 "Main channel fallback TX", fallback_iface,
                                                                 &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    sac_processing_interface_t main_channel_sample_accumulator_iface = {
        .init = sac_sample_accumulator_init,
        .process = sac_sample_accumulator_process,
        .gate = sac_fallback_gate_is_process_active,
    };
    /* Increase packet size in fallback to increase retx. */
    main_channel_sample_accumulator_instance.max_accumulator_size =
        (main_channel_producer_cfg.audio_payload_size * MAIN_CHANNEL_MAX_ACC_MUL) / MAIN_CHANNEL_MAX_ACC_DIV;
    main_channel_sample_accumulator_instance.get_accumulator_size = get_accumulator_size;
    main_channel_sample_accumulator_processing =
        sac_processing_stage_init((void *)&main_channel_sample_accumulator_instance, "Audio Sample Accumulator",
                                  main_channel_sample_accumulator_iface, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Processing stage that downsamples the audio samples from 96kHz to 48kHz. */
    main_channel_downsampling_instance.cfg.multiply_ratio = SAC_SRC_ONE;
    main_channel_downsampling_instance.cfg.divide_ratio = SAC_SRC_TWO;
    main_channel_downsampling_instance.cfg.payload_size = USB_AUDIO_ENABLED ? MAIN_CHANNEL_USB_PAYLOAD_SIZE :
                                                                              MAIN_CHANNEL_I2S_PAYLOAD_SIZE;
    main_channel_downsampling_instance.cfg.payload_size =
        (main_channel_downsampling_instance.cfg.payload_size * MAIN_CHANNEL_MAX_ACC_MUL) / MAIN_CHANNEL_MAX_ACC_DIV;
    /* Upsampling does not change the sample format. */
    main_channel_downsampling_instance.cfg.input_sample_format = MAIN_CHANNEL_PRODUCER_SAC_SAMPLE_FORMAT;
    main_channel_downsampling_instance.cfg.output_sample_format = MAIN_CHANNEL_PRODUCER_SAC_SAMPLE_FORMAT;
    main_channel_downsampling_instance.cfg.channel_count = MAIN_CHANNEL_CHANNEL_COUNT;
    main_channel_downsampling_processing = sac_processing_stage_init((void *)&main_channel_downsampling_instance,
                                                                     "Audio Downsampling",
                                                                     main_channel_downsampling_iface, &sac_status);
    main_channel_downsampling_discard_processing =
        sac_processing_stage_init((void *)&main_channel_downsampling_instance, "Audio Downsampling Discard",
                                  main_channel_downsampling_discard_iface, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Processing stage compresses using ADPCM before sending if fallback is activated. */
    main_channel_compression_instance.compression_mode = SAC_COMPRESSION_PACK_STEREO;
    main_channel_compression_instance.sample_format = main_channel_downsampling_instance.cfg.output_sample_format;
    main_channel_compression_processing = sac_processing_stage_init((void *)&main_channel_compression_instance,
                                                                    "Audio Compression", main_channel_compression_iface,
                                                                    &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Processing stage removes compressed audio samples, applicable when returning from fallback mode. */
    main_channel_compression_discard_processing = sac_processing_stage_init((void *)&main_channel_compression_instance,
                                                                            "Audio Compression Discard",
                                                                            main_channel_compression_discard_iface,
                                                                            &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Processing stage packs audio before sending over the air. Packing now follows the producer
     * and OTA sample formats declared in sac_cfg.h. The producer must stay right-justified 24-bit,
     * which is what both real audio (master or slave, RJF) and SINE_INJECT_DG deliver. */
    bool packing_en = sac_configure_packing(&main_channel_packing_instance, MAIN_CHANNEL_PRODUCER_SAC_SAMPLE_FORMAT,
                                            MAIN_CHANNEL_OTA_UNCOMPRESSED_SAC_SAMPLE_FORMAT, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    if (packing_en) {
        main_channel_packing_processing = sac_processing_stage_init((void *)&main_channel_packing_instance,
                                                                    "Audio Packing", main_channel_packing_iface,
                                                                    &sac_status);
        ASSERT_SAC_STATUS(sac_status);
    }

    /* Processing stage that packs audio samples to 16 bits for fallback mode 2. */
    bool packing_fbk_en = sac_configure_packing(&main_channel_fbk_packing_instance,
                                                MAIN_CHANNEL_PRODUCER_SAC_SAMPLE_FORMAT,
                                                MAIN_CHANNEL_OTA_PACKED_FBK_SAC_SAMPLE_FORMAT, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    if (packing_fbk_en) {
        main_channel_fbk_packing_processing = sac_processing_stage_init((void *)&main_channel_fbk_packing_instance,
                                                                        "Audio Packing FBK", main_channel_packing_iface,
                                                                        &sac_status);
        ASSERT_SAC_STATUS(sac_status);
    }

    /* Mute packet processing stage initialization. */
    main_channel_mute_packet_instance.is_tx = true;
    main_channel_mute_packet_processing = sac_processing_stage_init((void *)&main_channel_mute_packet_instance,
                                                                    "Mute packet", main_channel_mute_packet_iface,
                                                                    &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Initialize SWC consumer endpoint. */
    sac_endpoint_cfg_t main_channel_swc_consumer_cfg = {
        .use_encapsulation = true,
        .delayed_action = false,
        .channel_count = MAIN_CHANNEL_CHANNEL_COUNT,
        .audio_payload_size = MAIN_CHANNEL_SWC_PAYLOAD_SIZE,
        .queue_size = MAIN_CHANNEL_LATENCY_QUEUE_SIZE,
    };
    main_channel_swc_consumer_endpoint = sac_endpoint_init((void *)&main_channel_swc_consumer_instance,
                                                           "SWC EP (Consumer)", main_channel_swc_consumer_iface,
                                                           main_channel_swc_consumer_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Initialize audio pipeline. */
    sac_pipeline_cfg_t main_channel_pipeline_cfg = {
        .do_initial_buffering = true,
        .max_payload_size = main_channel_sample_accumulator_instance.max_accumulator_size,
    };
    main_channel_sac_pipeline = sac_pipeline_init("Audio -> SWC", main_channel_producer_endpoint,
                                                  main_channel_pipeline_cfg, main_channel_swc_consumer_endpoint,
                                                  &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Add processing stages to the audio pipeline. */
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_fallback_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_sample_accumulator_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_downsampling_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_downsampling_discard_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    if (packing_en) {
        sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_packing_processing, &sac_status);
        ASSERT_SAC_STATUS(sac_status);
    }
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_compression_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_compression_discard_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    if (packing_fbk_en) {
        sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_fbk_packing_processing, &sac_status);
        ASSERT_SAC_STATUS(sac_status);
    }
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_mute_packet_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Setup audio pipeline. */
    sac_pipeline_setup(main_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Fallback mode configuration. */
    sac_fallback_mode_cfg_t mode_cfg = sac_fallback_mode_get_defaults();
    uint8_t mode_index;

    /* Fallback mode 0 configuration. */
    mode_cfg.cca_bad_fail_count_threshold_perc = 2;
    mode_cfg.cca_bad_time_sec = 0.1;
    mode_cfg.consumer_buffer_load_threshold_tenths = 33;
    mode_cfg.sample_count = MAIN_CHANNEL_SAMPLE_COUNT;
    mode_index = sac_fallback_add_mode(&main_channel_fallback_instance, "96kHz 24-bit", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index,
                                     main_channel_sample_accumulator_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index,
                                     main_channel_downsampling_discard_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    if (packing_en) {
        sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_packing_processing,
                                         &sac_status);
        ASSERT_SAC_STATUS(sac_status);
    }

    /* Fallback mode 1 configuration. */
    mode_cfg = sac_fallback_mode_get_defaults();
    mode_cfg.cca_bad_fail_count_threshold_perc = 5;
    mode_cfg.cca_bad_time_sec = 0.1;
    mode_cfg.consumer_buffer_load_threshold_tenths = 40;
    mode_cfg.cca_good_fail_count_threshold_perc = 5;
    mode_cfg.cca_good_time_sec = 30;
    mode_cfg.link_margin_threshold = 60;
    mode_cfg.link_margin_good_time_sec = 5;
    mode_cfg.sample_count = MAIN_CHANNEL_FBK_1_SAMPLE_COUNT;
    mode_index = sac_fallback_add_mode(&main_channel_fallback_instance, "48kHz 24-bit", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index,
                                     main_channel_sample_accumulator_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_downsampling_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    if (packing_en) {
        sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_packing_processing,
                                         &sac_status);
        ASSERT_SAC_STATUS(sac_status);
    }

    /* Fallback mode 2 configuration. */
    mode_cfg = sac_fallback_mode_get_defaults();
    mode_cfg.cca_bad_fail_count_threshold_perc = 60;
    mode_cfg.cca_bad_time_sec = 0.1;
    mode_cfg.consumer_buffer_load_threshold_tenths = 48;
    mode_cfg.cca_good_fail_count_threshold_perc = 60;
    mode_cfg.cca_good_time_sec = 30;
    mode_cfg.link_margin_threshold = 40;
    mode_cfg.link_margin_good_time_sec = 4;
    mode_cfg.sample_count = MAIN_CHANNEL_FBK_2_SAMPLE_COUNT;
    mode_index = sac_fallback_add_mode(&main_channel_fallback_instance, "48kHz 16-bit", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index,
                                     main_channel_sample_accumulator_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_downsampling_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    if (packing_fbk_en) {
        sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index,
                                         main_channel_fbk_packing_processing, &sac_status);
        ASSERT_SAC_STATUS(sac_status);
    }
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index,
                                     main_channel_compression_discard_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Fallback mode 3 configuration. */
    mode_cfg = sac_fallback_mode_get_defaults();
    mode_cfg.cca_good_fail_count_threshold_perc = 60;
    mode_cfg.cca_good_time_sec = 10;
    mode_cfg.link_margin_threshold = 40;
    mode_cfg.link_margin_good_time_sec = 2;
    mode_cfg.sample_count = MAIN_CHANNEL_FBK_3_SAMPLE_COUNT;
    mode_index = sac_fallback_add_mode(&main_channel_fallback_instance, "48kHz ADPCM", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index,
                                     main_channel_sample_accumulator_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_downsampling_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_compression_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    if (USB_AUDIO_ENABLED) {
        /** Start fallback in best quality.
         *
         *  When using USB dongle, fallback state will not update until USB audio playback starts. To avoid the user
         *  thinking the audio quality is bad on startup, the fallback is initialized to its best quality.
         */
        sac_fallback_set_current_mode(&main_channel_fallback_instance, 0, &sac_status);
        ASSERT_SAC_STATUS(sac_status);
    }

#if SINE_INJECT_DG
    /* SINE test: pin the main channel to mode 0 (96 kHz 24-bit, no resample/compression) and
     * disable automatic fallback. The link-margin auto-fallback otherwise drops to mode 2/3
     * (48 kHz 16-bit / ADPCM), whose resampling + lossy compression introduce phase
     * discontinuities at buffer boundaries that sound like noise on a pure tone. Manual mode
     * stops automatic mode change; the node follows the mode 0 we transmit. */
    sac_fallback_set_current_mode(&main_channel_fallback_instance, 0, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_set_manual_mode(&main_channel_fallback_instance, true, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#else
    /* Main-channel ceiling capped at mode 1 (48 kHz 24-bit uncompressed): deactivate mode 0
     * (96 kHz) while leaving automatic fallback enabled. Downward degradation still works
     * (trigger_next_mode() drops to mode 2/3 on a bad link) and recover_to_previous_mode()
     * skips the inactive mode 0, so the link degrades and recovers up to 48 kHz but never
     * climbs to 96 kHz. Only the DG needs this: the node's main channel is RX (follows the
     * transmitted header), so capping the coordinator caps the whole main-channel path.
     *
     * The cap is kept because 96 kHz still parks: the SDK v2.3.1 TDMA re-sync fix did NOT
     * resolve the dual-radio TIM4-arr-max wedge at fb=0 (confirmed on u535 with the fixed
     * build). Re-enable mode 0 only once that park is fixed. Mutually exclusive with
     * SINE_INJECT_DG, which pins mode 0 for pure-tone testing. */
    sac_fallback_mode_set_active_state(&main_channel_fallback_instance, 0, false, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_set_current_mode(&main_channel_fallback_instance, 1, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif

    /*
     * Back Channel Audio Pipeline (RX)
     * ================================
     *
     * ***** NORMAL MODE USB (Fallback mode 0) *****
     * Input:       Mono stream of 48kHz/16-bit depth samples is received over the air from the Node.
     * Processing:  Audio sample accumulator 1.7647x.
     * Processing:  Digital volume control.
     * Output:      Mono stream of 48kHz/16-bit.
     *
     * +-----+    +---------------------+    +----------------+    +-----+
     * | SWC | -> | Accumulator 1.7647x | -> | Digital Volume | -> | USB |
     * +-----+    +---------------------+    +----------------+    +-----+
     *
     * **** NORMAL MODE I2S (Fallback mode 0) *****
     * Input:       Mono stream of 32kHz/16-bit depth samples is received over the air from the Node.
     * Processing:  Audio sample accumulator 1.7647x.
     * Processing:  Upsampling audio samples from 48kHz to 96kHz.
     * Processing:  Unpacking from 16 to 24 bits encoded on 32 bits audio samples.
     * Processing:  Digital volume control followed by clock drift compensation.
     * Output:      Mono stream of 48kHz/24-bit.
     *
     * +-----+    +---------------------+    +---------------+    +-----------+    +----------------+    +-----+
     * | SWC | -> | Accumulator 1.7647x | -> | Upsampling 2x | -> | Unpacking | -> | Digital Volume | -> | CDC | ---
     * +-----+    +---------------------+    +---------------+    +-----------+    +----------------+    +-----+   |
     *       -------------------------------------------------------------------------------------------------------
     *       |    +-----+
     *       ---> | I2S |
     *            +-----+
     *
     * ****** FALLBACK MODE USB (Fallback mode 1) *****
     * Input:       Mono stream of 48kHz/16-bit depth samples is received over the air from the Node.
     * Processing:  Decompression of samples compressed with ADPCM.
     * Processing:  Audio sample accumulator 1.7647x.
     * Processing:  Digital volume control.
     * Output:      Mono stream of 48kHz/16-bit.
     *
     * +-----+    +---------------------+    +---------------------+    +----------------+    +-----+
     * | SWC | -> | ADPCM Decompression | -> | Accumulator 1.7647x | -> | Digital Volume | -> | USB |
     * +-----+    +---------------------+    +---------------------+    +----------------+    +-----+
     *
     * **** FALLBACK MODE (Fallback mode 1) *****
     * Input:       Mono stream of 48kHz/16-bit depth samples is received over the air from the Node.
     * Processing:  Decompression of samples compressed with ADPCM.
     * Processing:  Audio sample accumulator 1.7647x.
     * Processing:  Upsampling audio samples from 48kHz to 96kHz.
     * Processing:  Unpacking from 16 to 24 bits encoded on 32 bits audio samples.
     * Processing:  Digital volume control followed by clock drift compensation and mute on glitch.
     * Output:      Mono stream of 96kHz/24-bit.
     *
     * +-----+    +---------------------+    +--------------------+    +------------+    +-----------+
     * | SWC | -> | ADPCM Decompressing | -> | Accumulator 1.7647 | -> | Upsampling | -> | Unpacking | ---
     * +-----+    +---------------------+    +--------------------+    +------------+    +-----------+   |
     *       ---------------------------------------------------------------------------------------------
     *       |    +----------------+    +-----+    +-----+
     *       ---> | Digital Volume | -> | CDC | -> | I2S |
     *            +----------------+    +-----+    +-----+
     */

    /* Initialize SWC producer endpoint. */
    sac_endpoint_cfg_t back_channel_swc_producer_cfg = {
        .use_encapsulation = true,
        .delayed_action = false,
        .channel_count = BACK_CHANNEL_CHANNEL_COUNT,
        .audio_payload_size = BACK_CHANNEL_SWC_PAYLOAD_SIZE,
        .queue_size = (SAC_MIN_PRODUCER_QUEUE_SIZE * BACK_CHANNEL_MAX_ACC_DIV) / BACK_CHANNEL_MAX_ACC_MUL,
    };
    back_channel_swc_producer_endpoint = sac_endpoint_init((void *)&back_channel_swc_producer_instance,
                                                           "SWC EP (Producer)", back_channel_swc_producer_iface,
                                                           back_channel_swc_producer_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    back_channel_fallback_instance.connection = rx_audio_conn;
    back_channel_fallback_instance.is_tx_device = false;
    back_channel_fallback_processing = sac_processing_stage_init(&back_channel_fallback_instance,
                                                                 "Back channel fallback RX", fallback_iface,
                                                                 &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Processing stage that decompresses audio samples if fallback is activated. */
    back_channel_decompression_instance.compression_mode = SAC_COMPRESSION_UNPACK_MONO;
    back_channel_decompression_instance.sample_format = BACK_CHANNEL_SAC_SAMPLE_FORMAT;
    back_channel_decompression_processing = sac_processing_stage_init((void *)&back_channel_decompression_instance,
                                                                      "Audio Decompressing",
                                                                      back_channel_decompression_iface, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Audio consumer endpoint initialization. */
    sac_endpoint_cfg_t dummy_consumer_cfg = {
        .use_encapsulation = false,
        .delayed_action = false,
        .channel_count = BACK_CHANNEL_CHANNEL_COUNT,
        .audio_payload_size = (BACK_CHANNEL_SWC_PAYLOAD_SIZE * BACK_CHANNEL_MAX_ACC_MUL) / BACK_CHANNEL_MAX_ACC_DIV,
        .queue_size = 3,
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
    sac_pipeline_cfg_t swc_pipeline_cfg = {
        .do_initial_buffering = false,
    };
    back_channel_sac_pipeline = sac_pipeline_init("SWC -> Accumulator", back_channel_swc_producer_endpoint,
                                                  swc_pipeline_cfg, dummy_audio_consumer, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Add processing stage to the audio pipeline. */
    sac_pipeline_add_processing(back_channel_sac_pipeline, back_channel_fallback_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_add_processing(back_channel_sac_pipeline, back_channel_decompression_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Audio pipeline setup. */
    sac_pipeline_setup(back_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Fallback mode configuration. */
    mode_cfg = sac_fallback_mode_get_defaults();

    /* Fallback mode 0 configuration. */
    mode_cfg.sample_count = BACK_CHANNEL_SAMPLE_COUNT;
    sac_fallback_add_mode(&back_channel_fallback_instance, "48kHz 16-bit", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Fallback mode 1 configuration. */
    mode_cfg.sample_count = BACK_CHANNEL_FBK_1_SAMPLE_COUNT;
    mode_index = sac_fallback_add_mode(&back_channel_fallback_instance, "48kHz ADPCM", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&back_channel_fallback_instance, mode_index, back_channel_decompression_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /** Start fallback in best quality.
     *
     *  When using USB dongle, fallback state will not update until USB audio playback starts. To avoid the user
     *  thinking the audio quality is bad on startup, the fallback is initialized to its best quality.
     */
    sac_fallback_set_current_mode(&back_channel_fallback_instance, 0, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* **** Accumulator pipeline **** */
    dummy_audio_producer = sac_endpoint_init(NULL, "ACC EP (Producer)", dummy_iface, dummy_consumer_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Audio sample accumulator processing stage initialization. */
    sac_processing_interface_t back_channel_sample_accumulator_iface = {
        .init = sac_sample_accumulator_init,
        .process = sac_sample_accumulator_process,
    };
    /* Make consumer packets always the same size. */
    back_channel_sample_accumulator_instance.max_accumulator_size = BACK_CHANNEL_SWC_PAYLOAD_SIZE;
    back_channel_sample_accumulator_processing =
        sac_processing_stage_init((void *)&back_channel_sample_accumulator_instance, "Audio Sample Accumulator",
                                  back_channel_sample_accumulator_iface, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Processing stage that upsamples the audio samples from 48 kHz to 96 kHz. */
    back_channel_upsampling_instance.cfg.multiply_ratio = SAC_SRC_TWO;
    back_channel_upsampling_instance.cfg.divide_ratio = SAC_SRC_ONE;
    back_channel_upsampling_instance.cfg.payload_size = BACK_CHANNEL_SWC_PAYLOAD_SIZE;
    back_channel_upsampling_instance.cfg.input_sample_format = BACK_CHANNEL_SAC_SAMPLE_FORMAT;
    back_channel_upsampling_instance.cfg.output_sample_format = BACK_CHANNEL_SAC_SAMPLE_FORMAT;
    back_channel_upsampling_instance.cfg.channel_count = BACK_CHANNEL_CHANNEL_COUNT;
    back_channel_upsampling_processing = sac_processing_stage_init((void *)&back_channel_upsampling_instance,
                                                                   "Audio Upsampling", back_channel_upsampling_iface,
                                                                   &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Processing stage that unpacks the received audio samples from 16 bits to 24 bits. */
    back_channel_unpacking_instance.packing_mode = SAC_UNPACK_24BITS_16BITS;
    back_channel_unpacking_processing = sac_processing_stage_init((void *)&back_channel_unpacking_instance,
                                                                  "Audio Unpacking", back_channel_unpacking_iface,
                                                                  &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Processing stage that handles the volume control. */
    back_channel_volume_instance.initial_volume_level = 100;
    back_channel_volume_instance.sample_format = BACK_CHANNEL_CONSUMER_SAC_SAMPLE_FORMAT;
    back_channel_volume_processing = sac_processing_stage_init((void *)&back_channel_volume_instance,
                                                               "Digital Volume Control", back_channel_volume_iface,
                                                               &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Processing stage that compensates the clock drift using CDC resampling. */
    back_channel_cdc_instance.cdc_resampling_length = CDC_DEFAULT_RESAMPLING_LENGTH * 2;
    /* Calculate queue averaging size based on sampling rate on the consumer. */
    back_channel_cdc_instance.cdc_queue_avg_size =
        sac_cdc_calculate_queue_average_size(MAX_DRIFT_PPM, I2S_SAMPLE_RATE_HZ,
                                             BACK_CHANNEL_I2S_PAYLOAD_SIZE / sizeof(uint32_t),
                                             CDC_DEFAULT_RESAMPLING_LENGTH * 2);
    back_channel_cdc_instance.sample_format = BACK_CHANNEL_CONSUMER_SAC_SAMPLE_FORMAT;
    back_channel_cdc_processing = sac_processing_stage_init((void *)&back_channel_cdc_instance, "CDC",
                                                            back_channel_cdc_iface, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Initialize codec consumer endpoint. */
    sac_endpoint_cfg_t back_channel_consumer_cfg = {
        .use_encapsulation = false,
        .delayed_action = !USB_AUDIO_ENABLED,
        .channel_count = BACK_CHANNEL_CHANNEL_COUNT,
        .audio_payload_size = USB_AUDIO_ENABLED ? BACK_CHANNEL_SWC_PAYLOAD_SIZE : BACK_CHANNEL_I2S_PAYLOAD_SIZE,
        .queue_size = BACK_CHANNEL_LATENCY_QUEUE_SIZE,
    };
    back_channel_consumer_endpoint = sac_endpoint_init(NULL, "Audio EP (Consumer)", back_channel_consumer_iface,
                                                       back_channel_consumer_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Initialize audio pipeline. */
    sac_pipeline_cfg_t back_channel_accumulator_pipeline_cfg = {
        .do_initial_buffering = false,
    };
    back_channel_accumulator_pipeline = sac_pipeline_init("Accumulator -> Audio", dummy_audio_producer,
                                                          back_channel_accumulator_pipeline_cfg,
                                                          back_channel_consumer_endpoint, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    sac_endpoint_link(dummy_audio_consumer, dummy_audio_producer, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Add processing stages to the audio pipeline. */
    sac_pipeline_add_processing(back_channel_accumulator_pipeline, back_channel_sample_accumulator_processing,
                                &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#if !USB_AUDIO_ENABLED
    sac_pipeline_add_processing(back_channel_accumulator_pipeline, back_channel_upsampling_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_add_processing(back_channel_accumulator_pipeline, back_channel_unpacking_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif
    sac_pipeline_add_processing(back_channel_accumulator_pipeline, back_channel_volume_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#if !USB_AUDIO_ENABLED
    sac_pipeline_add_processing(back_channel_accumulator_pipeline, back_channel_cdc_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif

    /* Setup audio pipeline. */
    sac_pipeline_setup(back_channel_accumulator_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
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

/** @brief Initialize the audio dowsampling processing stage interface.
 *
 *  @param[out] iface  Processing interface.
 */
static void app_audio_core_downsampling_interface_init(sac_processing_interface_t *iface)
{
    iface->init = sac_src_cmsis_init;
    iface->ctrl = NULL;
    iface->process = sac_src_cmsis_process;
    iface->gate = sac_fallback_gate_is_process_active;
}

/** @brief Initialize the audio downsampling discard processing stage interface.
 *
 *  @param[out] iface  Processing interface.
 */
static void app_audio_core_downsampling_discard_interface_init(sac_processing_interface_t *iface)
{
    iface->init = NULL;
    iface->ctrl = NULL;
    iface->process = sac_src_cmsis_process_discard;
    iface->gate = sac_fallback_gate_is_process_active;
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

/** @brief Initialize the audio upsampling processing stage interface.
 *
 *  @param[out] iface  Processing interface.
 */
static void app_audio_core_upsampling_interface_init(sac_processing_interface_t *iface)
{
    iface->init = sac_src_cmsis_init;
    iface->ctrl = NULL;
    iface->process = sac_src_cmsis_process;
    iface->gate = NULL;
}

/** @brief Initialize the audio cdc processing stage interface.
 *
 *  @param[out] iface  Processing interface.
 */
static void app_audio_core_cdc_interface_init(sac_processing_interface_t *iface)
{
    iface->init = sac_cdc_init;
    iface->ctrl = sac_cdc_ctrl;
    iface->process = sac_cdc_process;
    iface->gate = NULL;
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

/** @brief Initialize the unpacking processing stage interface.
 *
 *  @param[out] iface  Processing interface.
 */
static void app_audio_core_unpacking_interface_init(sac_processing_interface_t *iface)
{
    iface->init = sac_packing_init;
    iface->ctrl = sac_packing_ctrl;
    iface->process = sac_packing_process;
    iface->gate = NULL;
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

/** @brief Increase the audio output volume level.
 *
 *  @note This affects the audio pipeline that the digital volume processing stage is added to.
 */
static void volume_up(void)
{
    sac_status_t sac_status = SAC_OK;

    if (device_pairing_state != DEVICE_PAIRED) {
        return;
    }

    sac_processing_ctrl(back_channel_volume_processing, back_channel_sac_pipeline, SAC_VOLUME_INCREASE, SAC_NO_ARG,
                        &sac_status);
    ASSERT_SAC_STATUS(sac_status);
}

/** @brief Decrease the audio output volume level.
 *
 *  @note This affects the audio pipeline that the digital volume processing stage is added to.
 */
static void volume_down(void)
{
    sac_status_t sac_status = SAC_OK;

    if (device_pairing_state != DEVICE_PAIRED) {
        return;
    }

    sac_processing_ctrl(back_channel_volume_processing, back_channel_sac_pipeline, SAC_VOLUME_DECREASE, SAC_NO_ARG,
                        &sac_status);
    ASSERT_SAC_STATUS(sac_status);
}

/** @brief Main channel audio RX complete callback.
 *
 *  @note This receives audio packets from the codec. It needs to be executed every time a DMA transfer from the codec
 *        is completed in order to keep recording audio.
 */
static void main_channel_audio_rx_complete_callback(void)
{
    sac_status_t sac_status = SAC_OK;

#if SINE_INJECT_DG
    /* External I2S clocks the DMA (precise 96 kHz timing); overwrite the just-read buffer
     * with a 1 kHz sine before it enters the pipeline. Same path/format as real audio. */
    sac_facade_i2s_inject_sine();
#endif

    /* One increment per produced audio buffer: delta/sec = DG produce rate (expect 2400/s).
     * Always compiled (not just SINE_DEBUG_CAPTURE) so LINK_WATCH reports it on no-SINE builds. */
    dbg_dg_produce_cnt++;

    /* The codec produces audio samples when it receives input audio. */
    sac_pipeline_produce(main_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Trigger main channel process. */
    facade_audio_process_main_channel_timer_trigger();
}

#if USB_AUDIO_ENABLED
/** @brief SAI DMA TX complete callback.
 *
 *  @note This feeds the codec with audio packets. It needs to be executed every time a DMA transfer to the codec is
 *        completed in order to keep the audio playing.
 */
static void back_channel_audio_tx_complete_callback(void)
{
    sac_status_t sac_status = SAC_OK;
    uint32_t target_fifo_size;
    uint32_t usb_buf_rem = facade_app_audio_usb_get_epin_fifo_remaining();

    target_fifo_size =
        back_channel_fbk_latency_usb_fifo_size[sac_fallback_get_current_mode(&back_channel_fallback_instance,
                                                                             &sac_status)];
    facade_app_audio_usb_set_epin_target_fifo_size(target_fifo_size);

    if ((sac_pipeline_get_consumer_buffer_load(back_channel_accumulator_pipeline, &sac_status) > 0) &&
        ((usb_buf_rem / BACK_CHANNEL_SWC_PAYLOAD_SIZE) > 1)) {
        sac_pipeline_consume(back_channel_accumulator_pipeline, &sac_status);
    }
}
#else
/** @brief Back channel audio TX complete callback.
 *
 *  @note This feeds the codec with audio packets. It needs to be executed every time a DMA transfer to the codec is
 *        completed in order to keep the audio playing.
 */
static void back_channel_audio_tx_complete_callback(void)
{
    sac_status_t sac_status = SAC_OK;

    uint32_t target_queue_size;

    /* Set audio latency based on the fallback mode. */
    target_queue_size =
        back_channel_fbk_latency_queue_size[sac_fallback_get_current_mode(&back_channel_fallback_instance,
                                                                          &sac_status)];
    sac_cdc_ctrl(&back_channel_cdc_instance, back_channel_accumulator_pipeline, SAC_CDC_SET_TARGET_QUEUE_SIZE,
                 target_queue_size, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* The codec consumes audio samples produced by the SWC (which receives them from the Node). */
    sac_pipeline_consume(back_channel_accumulator_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
}
#endif

/** @brief Callback handling the audio process triggered by the app timer.
 */
static void audio_process_main_channel_callback(void)
{
    sac_status_t sac_status = SAC_OK;
    uint32_t buffer_load = 0;

    buffer_load = sac_pipeline_get_producer_buffer_load(main_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    while (buffer_load > 0) {
        /* Processing stages of the pipeline are executed. */
        sac_pipeline_process(main_channel_sac_pipeline, &sac_status);
        ASSERT_SAC_STATUS(sac_status);
        buffer_load--;
        /* The SWC consumes audio samples produced by the codec. */
        sac_pipeline_consume(main_channel_sac_pipeline, &sac_status);
        ASSERT_SAC_STATUS(sac_status);
    }

    buffer_load = sac_pipeline_get_consumer_buffer_load(main_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    if (buffer_load > 0) {
        /* The SWC consumes audio samples produced by the codec. */
        sac_pipeline_consume(main_channel_sac_pipeline, &sac_status);
        ASSERT_SAC_STATUS(sac_status);
    }
}

/** @brief Callback handling the audio process that triggers with the app timer.
 */
static void audio_process_back_channel_callback(void)
{
    sac_status_t sac_status = SAC_OK;

    if (back_channel_trigger_count > 0) {
        back_channel_trigger_count--;
    }

    sac_pipeline_process(back_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    while (sac_pipeline_get_producer_buffer_load(back_channel_accumulator_pipeline, &sac_status) > 0) {
        sac_pipeline_process(back_channel_accumulator_pipeline, &sac_status);
        ASSERT_SAC_STATUS(sac_status);
    }

#if USB_AUDIO_ENABLED
    uint32_t buffer_load = 0;

    buffer_load = sac_pipeline_get_consumer_buffer_load(back_channel_accumulator_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Consume all nodes into the USB FIFO. */
    while (buffer_load > 0 && facade_app_audio_usb_get_epin_fifo_remaining() >= BACK_CHANNEL_SWC_PAYLOAD_SIZE) {
        /* The USB audio consumes audio coming from the SWC rx audio connection. */
        sac_pipeline_consume(back_channel_accumulator_pipeline, &sac_status);
        ASSERT_SAC_STATUS(sac_status);
        buffer_load--;
    }
#endif

    if (back_channel_trigger_count > 0) {
        /* Retrigger the processing. */
        facade_audio_process_back_channel_timer_trigger();
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
/** @brief DG-side link diagnostic. Prints one line every LINK_WATCH_INTERVAL_MS via
 *         facade_stats_write (board-aware: ST-Link VCP / UART4 PC10-PC11 on U535, USB CDC
 *         elsewhere). Non-asserting reads, so it keeps running through a link drop.
 *
 *  Line format:
 *    [DG t=<ms>] <Connected|Disconnected> fb=<mode> node_lm=<n> prod=<n>/s send=<n>/s
 *        bk=<ok>/<miss> tx_drop=<n>
 *  followed, only when they say something, by swc=STOP and send_err=<e>(<n>).
 *
 *  Rates, not raw counters, for the two that matter: tx_slot/tx_noframe were free-running,
 *  so answering "is the coordinator still putting frames on air" meant diffing two lines by
 *  hand. send/s is the same information already differenced.
 *
 *  How to read it against the HS log:
 *    - node_lm low/0 while the HS prints lm=255  => HS->DG back channel is dead; the DG
 *      never learns the link is good and keeps audio at the worst fallback. Watch bk=ok/miss:
 *      if miss climbs and ok stalls, the node's reports are not arriving.
 *    - prod/s below 2400 (96000/40) => the DG audio producer is starved (nothing queued to
 *      send), so the HS sees empty slots and records them as rx_miss. Not an RF problem.
 *    - send/s against prod/s is the retransmission read: the audio packet rate is prod/s
 *      divided by the current accumulator ratio, so send/s well above that is spare slots
 *      being spent on retries, which is what widening the accumulator buys.
 *    - fb mode here is what the DG decided and transmits; the node follows it. */
static void link_watch(void)
{
    static uint32_t tick_start;
    static bool initialized;
    static bool prev_connected;
    static uint32_t produce_prev;
    static uint32_t sent_prev;
    static uint32_t rate_prev_tick;
    static bool rate_prev_valid;

    if (device_pairing_state != DEVICE_PAIRED || tx_audio_conn == NULL) {
        initialized = false;
        rate_prev_valid = false;
        return;
    }

    uint32_t now = facade_get_tick_ms();
    if (initialized && (now - tick_start) < LINK_WATCH_INTERVAL_MS) {
        return;
    }
    tick_start = now;

    swc_error_t conn_err = SWC_ERR_NONE;
    swc_error_t tx_err = SWC_ERR_NONE;
    swc_error_t bk_err = SWC_ERR_NONE;
    sac_status_t fb_status = SAC_OK;
    bool connected = swc_connection_get_connect_status(tx_audio_conn, &conn_err);
    swc_statistics_t *tx_stats = swc_connection_update_stats(tx_audio_conn, &tx_err);
    swc_statistics_t *bk_stats = swc_connection_update_stats(rx_data_conn, &bk_err);
    swc_status_t swc_state = swc_get_status();
    uint8_t fb_mode = sac_fallback_get_current_mode(&main_channel_fallback_instance, &fb_status);

    uint32_t tx_slot = (tx_stats != NULL) ? tx_stats->tx_timeslot_occurrence : 0;
    uint32_t tx_noframe = (tx_stats != NULL) ? tx_stats->no_packet_tranmission_count : 0;
    uint32_t tx_drop = (tx_stats != NULL) ? tx_stats->packet_dropped_count : 0;
    uint32_t bk_ok = (bk_stats != NULL) ? bk_stats->packet_successfully_received_count : 0;
    uint32_t bk_miss = (bk_stats != NULL) ? bk_stats->no_packet_reception_count : 0;

    /* Both rates over the real elapsed interval, normalized to per-second:
     *   prod/s -- DG audio production (buffers/s). Expect 2400 (96000/40); a sustained value
     *             below that is a starved producer, i.e. dead air the node records as rx_miss.
     *   send/s -- timeslots that actually carried a frame (occurrence minus no-transmission).
     * Sharing one timestamp keeps the two directly comparable, which is the whole point of
     * printing them side by side. */
    uint32_t produce_now = dbg_dg_produce_cnt;
    uint32_t sent_now = tx_slot - tx_noframe;
    uint32_t prod_rate = 0;
    uint32_t send_rate = 0;
    if (rate_prev_valid) {
        uint32_t dms = now - rate_prev_tick;
        if (dms > 0) {
            prod_rate = (uint32_t)(((uint64_t)(produce_now - produce_prev) * 1000U) / dms);
            send_rate = (uint32_t)(((uint64_t)(sent_now - sent_prev) * 1000U) / dms);
        }
    }
    produce_prev = produce_now;
    sent_prev = sent_now;
    rate_prev_tick = now;
    rate_prev_valid = true;

    /* Sized for the worst case, not the typical one: bk_ok/tx_slot are free-running counters
     * that reach 10 digits on a long soak, and the prefix now carries the version. Too small
     * and snprintf drops the trailing \r\n, which runs the next line into this one. */
    char line[256];

    /* Edge: announce connect<->disconnect transitions immediately. */
    if (!initialized) {
        prev_connected = connected;
        initialized = true;
    } else if (connected != prev_connected) {
        snprintf(line, sizeof(line), "\r\n[DG t=%lu] link %s\r\n",
                 (unsigned long)now, connected ? "RECOVERED" : "DROPPED");
        facade_stats_write(line);
        prev_connected = connected;
    }

    int n = snprintf(line, sizeof(line),
                     "[DG t=%lu] %s fb=%u node_lm=%u "
                     "prod=%lu/s send=%lu/s bk=%lu/%lu tx_drop=%lu",
                     (unsigned long)now, connected ? "Connected   " : "Disconnected",
                     (unsigned)fb_mode, (unsigned)s_node_rx_lm,
                     (unsigned long)prod_rate, (unsigned long)send_rate,
                     (unsigned long)bk_ok, (unsigned long)bk_miss, (unsigned long)tx_drop);

    /* Appended only when they carry information, the same rule the HS crash dump follows:
     * swc is RUNNING and send_err is 0(0) for every line of a healthy run, and a field that
     * never changes is one more thing to read past on every line for a year. */
    if (n > 0 && n < (int)sizeof(line) && swc_state != SWC_STATUS_RUNNING) {
        n += snprintf(line + n, sizeof(line) - n, " swc=STOP");
    }
    if (n > 0 && n < (int)sizeof(line) && (s_last_send_err != SWC_ERR_NONE || s_send_err_count != 0)) {
        n += snprintf(line + n, sizeof(line) - n, " send_err=%d(%lu)",
                      (int)s_last_send_err, (unsigned long)s_send_err_count);
    }
    if (n > 0 && n < (int)sizeof(line)) {
        snprintf(line + n, sizeof(line) - n, "\r\n");
    }
    facade_stats_write(line);
}
#endif /* LINK_WATCH */

/** @brief Print the audio and wireless statistics.
 */
static void print_stats(void)
{
    if (device_pairing_state != DEVICE_PAIRED) {
        return;
    }

    static char stats_string[STATS_ARRAY_LENGTH];
    int string_length = 0;
    sac_status_t sac_status = SAC_OK;
    swc_error_t swc_err = SWC_ERR_NONE;

    const char *device_str = "\n<   COORDINATOR   >\n\r";
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

    sac_pipeline_update_stats(back_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    string_length += sac_pipeline_format_stats(back_channel_sac_pipeline, stats_string + string_length,
                                               sizeof(stats_string) - string_length, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    sac_pipeline_update_stats(back_channel_accumulator_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    string_length += sac_pipeline_format_stats(back_channel_accumulator_pipeline, stats_string + string_length,
                                               sizeof(stats_string) - string_length, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

#if USB_AUDIO_ENABLED
    /* ** USB Audio Statistics ** */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length,
                              "USB audio TX fifo sample count: %lu\r\n",
                              facade_get_coord_usb_audio_tx_fifo_sample_count());
#else
    /* ** CDC Statistics ** */
    string_length += sac_cdc_format_stats(&back_channel_cdc_instance, stats_string + string_length,
                                          sizeof(stats_string) - string_length, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif

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
    swc_connection_t *connections[] = {tx_audio_conn, rx_audio_conn, tx_data_conn, rx_data_conn};

    for (uint8_t i = 0; i < ARRAY_SIZE(connections); i++) {
        swc_connection_update_stats(connections[i], &swc_err);
        ASSERT_SWC_STATUS(swc_err);
        string_length += swc_connection_format_stats(connections[i], stats_string + string_length,
                                                     sizeof(stats_string) - string_length, &swc_err);
        ASSERT_SWC_STATUS(swc_err);
    }

    facade_stats_write(stats_string);

    /* ** APP Statistics ** */
    string_length = snprintf(stats_string, sizeof(stats_string), "\r\n<< Application Statistics >>\r\n");
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, "Fallback State:");
    if (fallback_state == FALLBACK_AUTO) {
        snprintf(stats_string + string_length, sizeof(stats_string) - string_length, " AUTO\r\n");
    } else if (fallback_state == FALLBACK_96K_24BIT_UNCOMPRESSED) {
        snprintf(stats_string + string_length, sizeof(stats_string) - string_length, " 96kHz %d-bit\r\n",
                 MAIN_CHANNEL_OTA_UNCOMPRESSED_BIT_DEPTH);
    } else if (fallback_state == FALLBACK_48K_24BIT_UNCOMPRESSED) {
        snprintf(stats_string + string_length, sizeof(stats_string) - string_length, " 48kHz %d-bit\r\n",
                 MAIN_CHANNEL_OTA_UNCOMPRESSED_BIT_DEPTH);
    } else if (fallback_state == FALLBACK_48K_16BIT_UNCOMPRESSED) {
        snprintf(stats_string + string_length, sizeof(stats_string) - string_length, " 48kHz %d-bit\r\n",
                 MAIN_CHANNEL_OTA_PACKED_BIT_DEPTH);
    } else if (fallback_state == FALLBACK_48K_ADPCM_STEREO) {
        snprintf(stats_string + string_length, sizeof(stats_string) - string_length, " 48kHz ADPCM\r\n");
    }

    facade_stats_write(stats_string);
}

/** @brief Callback sends the button state at the DATA_TX_PERIOD_MS interval.
 */
static void data_callback(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    swc_fallback_info_t fallback_info = {0};
    user_data_t transmitted_user_data = {0};

    /* Update the link margin. Do NOT assert: this runs every 10 ms and a transient error on
     * link loss must not hang the device. Record and continue. */
    fallback_info = swc_connection_get_fallback_info(rx_audio_conn, &swc_err);
    s_last_fb_info_err = swc_err;

    /* Send the button state and the link margin to the Node. */
    transmitted_user_data.link_margin = fallback_info.link_margin;
    transmitted_user_data.button_state = facade_read_button_state();

    /* Edge triggered: clear the slot as soon as it is packed, exactly as the node does.
     * Nothing checks whether the send below succeeded, so a media command issued while the
     * link is down is lost rather than retried -- acceptable for a key press a user can
     * repeat, and the reason MD/at_cmd_bidir_decision_spec.md keeps state-carrying fields off
     * this mechanism. */
    transmitted_user_data.cmd_type = s_pending_cmd;
    s_pending_cmd = AT_CMD_NONE;

    /* Vendor pass-through. Unlike cmd_type this is not a single slot the sender clears: the
     * AT core hands out the same frame for several consecutive packets and only then moves
     * on, which is what stands in for an acknowledgement. */
    user_data_pack_vendor(&transmitted_user_data);

    /* Length, not sizeof: the vendor payload is empty in almost every packet and must not be
     * put on the air when it is. See user_data_tx_size(). */
    wireless_send_data(&transmitted_user_data, user_data_tx_size(&transmitted_user_data), &swc_err);
}

/** @brief Handle pairing button callback.
 */
static void pairing_button_callback(void)
{
    /* Boot auto-reconnect in progress: the state says PAIRED but the link is only
     * half-open and try_boot_reconnect() still owns the connection handles. Do NOT
     * unpair from here -- that would also wipe the discovery list and the flash
     * record. Just ask the reconnect to give up; main() then enters pairing mode,
     * which is what the press meant anyway. */
    if (s_boot_reconnect_active) {
        s_boot_reconnect_abort = true;
        return;
    }

    switch (device_pairing_state) {
    case DEVICE_PAIRED:
        /* A deliberate "forget this peer": the press means the user is removing the
         * device, so the record goes with it. Contrast at_start_pairing(). */
        unpair_device(true);
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
    pairing_event = pairing_coordinator_start(&app_pairing_cfg, &pairing_assigned_address, pairing_discovery_list,
                                              PAIRING_DISCOVERY_LIST_SIZE, &pairing_err);
    if (pairing_err != PAIRING_ERR_NONE) {
        facade_print_error_string("An error occurred during the pairing process.");
        while (1);
    }

    /* Handle the pairing events. */
    switch (pairing_event) {
    case PAIRING_EVENT_SUCCESS:
        /* Indicate that the pairing process was successful. */
        facade_notify_pairing_successful();

        app_init();
        device_pairing_state = DEVICE_PAIRED;

        /* Persist the assigned address so the next boot reconnects automatically
         * (boot auto-reconnect). Best-effort: a flash fault here only costs the
         * auto-reconnect on the next boot, not this session. */
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
 *  If a valid pairing address was persisted to flash, rebuild the coordinator's
 *  discovery list from it and bring the wireless core up (same path as a fresh
 *  pairing success / at_start_connect), then wait up to RECONNECT_TIMEOUT_MS for
 *  the real SWC link to come up. On success the device stays paired and
 *  streaming. On timeout the wireless core is LEFT RUNNING (see below); the flash
 *  record is KEPT (the peer being off is not a reason to forget the pair).
 *
 *  @return BOOT_RECONNECT_OK   link re-established (paired, streaming);
 *          BOOT_RECONNECT_PAIR no usable record (never paired), or the user
 *                              aborted the attempt to pair (button / AT+LE_UWB_PAIR)
 *                              -- caller enters pairing;
 *          BOOT_RECONNECT_IDLE a record existed but the node was not up in time --
 *                              the coordinator's core is left running so the node
 *                              can sync whenever it boots (do NOT teardown / re-pair).
 */
static boot_reconnect_result_t try_boot_reconnect(void)
{
    uint32_t start;
    bool connected = false;

    /* Blank / corrupt / wrong-version flash -> no record -> never paired -> pair. */
    if (!reconnect_store_load(&pairing_assigned_address)) {
        return BOOT_RECONNECT_PAIR;
    }

    /* A valid record should never carry a zero node address, but guard defensively
     * -- treat it as no record and pair. */
    if (pairing_assigned_address.node_address == 0) {
        return BOOT_RECONNECT_PAIR;
    }

    /* Rebuild the coordinator's discovery list from the persisted addresses:
     * app_swc_core_init() reads the local (coordinator) and remote (node) node
     * addresses from it. These equal the assigned coordinator/node addresses. */
    pairing_discovery_list[PAIRING_DEVICE_ROLE_COORDINATOR].node_address =
        pairing_assigned_address.coordinator_address;
    pairing_discovery_list[PAIRING_DEVICE_ROLE_NODE].node_address =
        pairing_assigned_address.node_address;

    /* Fast-blink the status LED to show a silent reconnect is in progress. */
    facade_notify_reconnecting();

    /* Build the wireless core from the restored addresses and connect. */
    at_cmd_core_set_uwb_conn_status(AT_UWB_CONN_STATUS_CONNECTING);
    app_init();
    device_pairing_state = DEVICE_PAIRED;

    /* Poll the real SWC link status until the node is reachable or the timeout
     * elapses. Keep servicing buttons and AT commands meanwhile; those handlers
     * defer their teardown through s_boot_reconnect_abort.
     *
     * link_is_up(), not tx_audio_conn: this runs before the host has necessarily
     * started feeding audio, and waiting on the audio connection made a reconnect
     * to a perfectly reachable node report CONNECT_FAIL and sit in IDLE until
     * someone pressed play. */
    s_boot_reconnect_abort = false;
    s_boot_reconnect_active = true;

    start = facade_get_tick_ms();
    while ((facade_get_tick_ms() - start) < RECONNECT_TIMEOUT_MS) {
        if (link_is_up()) {
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
        if (tx_audio_conn == NULL) {
            break;
        }
    }

    s_boot_reconnect_active = false;

    if (connected) {
        facade_notify_pairing_successful();
        return BOOT_RECONNECT_OK;
    }

    /* The user aborted with the pairing button / AT+LE_UWB_PAIR, or a deferred teardown
     * already released the core handles: dismantle whatever is left (also resets
     * device_pairing_state to UNPAIRED and stops the pipelines) and let the caller
     * enter pairing. The flash record is intentionally left intact. */
    if (s_boot_reconnect_abort || tx_audio_conn == NULL) {
        app_teardown();
        return BOOT_RECONNECT_PAIR;
    }

    /* Plain timeout with the core still up: the node just is not on yet. Unlike the
     * HS (battery -- it powers down and waits for an SoC-driven retry), the
     * coordinator is mains-powered and IS the timebase master, so LEAVE the wireless
     * core running. It keeps transmitting the schedule; the node syncs whenever it
     * boots and the main loop's AT status machine emits UWB_CONNECTED. Do NOT
     * teardown, do NOT re-pair. */
    return BOOT_RECONNECT_IDLE;
}

/** @brief Unpair the device. This will reset its discovery list.
 *
 *  @param[in] forget_peer  true to also erase the persisted pairing address, so the next
 *                          boot does not reconnect to the peer that was just dropped.
 *                          false to tear the link down but keep the record, which is what
 *                          re-pairing wants: see at_start_pairing().
 */
static void unpair_device(bool forget_peer)
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

    /* Reset the pairing discovery list. */
    memset(pairing_discovery_list, 0, sizeof(pairing_discovery_list));

    if (forget_peer) {
        /* Erase the persisted pairing address so the next boot does not reconnect
         * to the device the user just removed (boot auto-reconnect). */
        reconnect_store_clear();
    }

    /* Stop the main channel audio pipeline. */
    sac_pipeline_stop(main_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Stop the back channel audio pipeline. */
    sac_pipeline_stop(back_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_stop(back_channel_accumulator_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    main_channel_sac_pipeline = NULL;
    back_channel_sac_pipeline = NULL;
    back_channel_accumulator_pipeline = NULL;

    facade_audio_deinit();

    if (forget_peer) {
        /* Tell the host the record is gone, not just the link -- the two call for opposite
         * handling. Reported from here so it tracks the erase itself: a teardown that keeps
         * the record must not claim the device was unpaired. */
        at_cmd_core_notify_unpaired();
    }

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
static void wireless_send_data(const void *transmitted_data, uint8_t size, swc_error_t *swc_err)
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

    /* Send the payload through the Wireless Core. Do NOT assert: when the link is down the TX
     * queue fills up and this returns an error every 10 ms. Record and return; the queue drains
     * once the link is back. */
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
 *  @return Size of the data read.
 */
static uint16_t wireless_read_data(void *received_data, uint8_t size, swc_error_t *swc_err)
{
    uint8_t *payload = NULL;
    uint16_t payload_size = 0;
    uint16_t copy_size = 0;

    /* Read received data. */
    payload_size = swc_connection_receive(rx_data_conn, &payload, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Copy only what the caller's struct can hold, and copy it whatever the sizes are.
     *
     * This used to "return 0" on an oversized payload WITHOUT calling
     * swc_connection_receive_complete(), which leaks the RX buffer. That is unreachable while
     * both ends run the same firmware, but user_data_t grows over releases (face_state, the
     * vendor pass-through fields), so an old build receiving a newer, larger packet becomes a
     * real field condition during a rollout. The failure mode was not "the new field is
     * missing": a leaked buffer every 10 ms fills the RX queue and kills the data connection
     * permanently -- link margin stops updating and fallback wanders.
     *
     * Truncating instead is safe because user_data_t is append-only: the prefix a shorter
     * struct understands sits at the same offsets in the longer one. The reverse case
     * (payload shorter than the struct) already worked, because callers zero-initialize and
     * every field's 0 means "absent". Both directions therefore degrade to "the fields this
     * build knows about", which is the whole point. */
    copy_size = (payload_size > size) ? size : payload_size;

    if (received_data != NULL && payload != NULL && copy_size > 0) {
        memcpy(received_data, payload, copy_size);
    }

    /* Free the payload memory. Must happen on every path that took a payload. */
    swc_connection_receive_complete(rx_data_conn, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    return copy_size;
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

    at_cmd_core_set_device_address(pairing_discovery_list[PAIRING_DEVICE_ROLE_COORDINATOR].node_address);
    /* Deliberately NOT set to CONNECTED here: swc_connect() only arms the link, it does
     * not mean the node answered. Declaring CONNECTED at this point overwrote the
     * CONNECTING state its callers had just set, so the AT_UWB_CONNECT_TIMEOUT_MS window
     * and +EVENT: LE_UWB_CONNECT_FAIL could never fire and AT+LE_UWB_CONN_STATUS? reported a
     * live link even when the node was absent. The status is now driven by the link poll
     * in at_cmd_core_process() via at_get_link_status(). */

    /* Initialize Audio Core. */
    app_audio_core_init();
    /* Initialize GPIOs and peripherals for audio operations. */
#if (I2S_MASTER_MODE)
    facade_audio_coord_init(true);
#else
    facade_audio_coord_init(false);
#endif

    /* Start audio pipelines. */
    sac_pipeline_start(main_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_start(back_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_start(back_channel_accumulator_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Start timers used for audio processes. */
    facade_audio_process_main_channel_timer_start();
    facade_audio_process_back_channel_timer_start();

    /* Start data and statistics timer. */
    facade_data_timer_start();
}

/** @brief AT+LE_UWB_PAIR -- re-pair: drop the current pairing (if any), then enter pairing.
 *
 *  Unlike pairing_button_callback(), which is a three-way toggle (press once to unpair,
 *  press again to pair), the AT command is a single action: the host asks for "re-pair"
 *  once and expects the module to end up discoverable. Falling through from
 *  unpair_device() to enter_pairing_mode() is what makes that one command enough.
 *
 *  Note the link being down does NOT clear device_pairing_state: a device whose peer went
 *  away still reads DEVICE_PAIRED, so this fall-through is the common case, not the rare
 *  one. Re-sending while already DEVICE_PAIRING stays a no-op, so a host that repeats the
 *  command cannot restart the pairing window.
 *
 *  The teardown passes forget_peer=false, so re-pairing is a replace rather than an
 *  erase-then-hope: the old record survives until enter_pairing_mode() overwrites it on
 *  success. Pairing needs BOTH ends inside the same window (UWB is the only channel
 *  between them, so with the link down neither can tell the other to re-pair), which makes
 *  a mistimed attempt the normal kind of failure, not an exotic one -- and erasing up front
 *  would turn every one of them into a permanently lost pairing. On PAIR_FAIL the module is
 *  simply back where it started, and AT+LE_UWB_CONNECT resets it into boot auto-reconnect,
 *  which reloads the kept record. Same policy as try_boot_reconnect(): a peer that cannot
 *  be reached is not a reason to forget it.
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
        unpair_device(false); /* keep the record until the new pairing replaces it */
    }
    enter_pairing_mode();
}

/** @brief AT+LE_UWB_CONNECT -- reconnect by resetting the MCU into boot auto-reconnect.
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
 *      AT+LE_UWB_CONNECT -> OK -> +EVENT: LE_UWB_READY -> +EVENT: LE_UWB_CONNECTED
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
 *  before main() falls through to pairing. It is NOT what AT+LE_UWB_DISCONNECT does: that
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

    /* The heartbeat belongs to the connections that just went away. */
    s_node_rx_seen = false;

    sac_pipeline_stop(main_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_stop(back_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_stop(back_channel_accumulator_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    main_channel_sac_pipeline = NULL;
    back_channel_sac_pipeline = NULL;
    back_channel_accumulator_pipeline = NULL;

    facade_audio_deinit();
    facade_led_all_off();
    at_cmd_core_set_uwb_conn_status(AT_UWB_CONN_STATUS_STANDBY);
}

/** @brief AT+LE_UWB_DISCONNECT -- power the module down into Standby. Does not return.
 *
 *  No SDK teardown is attempted, deliberately. Stopping the wireless core and the audio
 *  pipelines in place is the unreliable path: swc_disconnect() can report a timeout (and
 *  asserting on it wedged the device), and a stopped SAC pipeline cannot be restarted at
 *  all. Powering the MCU off stops everything at once and makes all of that moot -- which
 *  also means there is nothing left that could quietly bring the link back up.
 *
 *  The module leaves Standby only through a reset, which runs main() from the top and so
 *  goes straight into boot auto-reconnect. On this hardware that is NRST (the SOC line or
 *  the reset button); AT+LE_UWB_CONNECT reaches the same place via facade_system_reset().
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
    /* device_pairing_state alone only says "the app believes it is paired" — it is set
     * unconditionally right after app_init(), so it reported a link that may never have
     * come up. Ask the Wireless Core instead, through link_is_up(). */
    if (device_pairing_state != DEVICE_PAIRED) {
        return false;
    }
    return link_is_up();
}

/** @brief True while the node is still answering this coordinator.
 *
 *  The data connections, not tx_audio_conn. On the coordinator the audio connection's status
 *  is only a statement about audio: with an idle source there is nothing queued, auto-sync is
 *  off on every connection, so the audio timeslots go out empty and the frame outcome is
 *  FRAME_WAIT -- which link_update_connect_status() ignores. The status therefore freezes at
 *  whatever it was when the music stopped and stays there until playback resumes. Frozen is
 *  worse than wrong here: OR-ing it in would mask a node that switched off mid-pause.
 *
 *  Both data connections are exercised every 10 ms whatever the audio source is doing --
 *  tx_data_conn by data_callback() (and ACK'd by the node), rx_data_conn by the node's own
 *  report -- so their status is live evidence that the peer is there. Either one up is
 *  enough; a node that really goes away takes both down inside the same 20 ms.
 *
 *  Non-asserting reads, like link_watch(), so a status poll during a drop cannot itself trap.
 *  Both handles are NULL-checked: every teardown path releases them.
 */
static bool link_is_up(void)
{
    /* "Have I heard the node recently", measured directly, rather than asking the Wireless
     * Core for a connection status.
     *
     * Two versions of this were wrong before it. The first OR'd tx_data_conn in and returned
     * true on it -- the wrong question on this side, because the coordinator is the timebase
     * master and keeps transmitting into its own timeslots whether or not anything is
     * listening (wps_mac.c even pins a TX connection to CONNECTED outright when it carries no
     * ACK). The second asked rx_data_conn alone, which is the right connection to ask, and it
     * STILL reported CONNECTED with the node powered off: on the coordinator link_update_
     * connect_status() gets synced == true unconditionally, so the status can only fall
     * through frame outcomes, and that path did not fire here. Rather than keep guessing at
     * the core's bookkeeping for a host-facing event, measure the thing the event is about.
     *
     * The node's data_callback() sends a packet every 10 ms unconditionally, so an arriving
     * packet is the peer's heartbeat and the RX callback stamps it. No packet for
     * NODE_RX_TIMEOUT_MS means the node is not there.
     *
     * s_node_rx_seen guards the boot case: without it, a tick count still below the timeout
     * would read as "heard recently" for the first NODE_RX_TIMEOUT_MS after reset -- and
     * try_boot_reconnect() polls this, so it would have declared success against a node that
     * was never powered on, which is exactly the failure this replaces.
     *
     * Both flags are cleared in app_teardown(), so a rebuilt link starts from "not heard". */
    if (!s_node_rx_seen) {
        return false;
    }

    return (facade_get_tick_ms() - s_node_rx_tick) < NODE_RX_TIMEOUT_MS;
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

static void at_set_vol(uint8_t vol)
{
    sac_status_t sac_status = SAC_OK;
    uint8_t steps;

    if (device_pairing_state != DEVICE_PAIRED) {
        return;
    }
    /* SAC_VOLUME_TICK = 0.1, so steps = vol/10 (rounded), max 10 steps = 100%. */
    steps = (vol + 5) / 10;
    sac_processing_ctrl(back_channel_volume_processing, back_channel_sac_pipeline,
                        SAC_VOLUME_MUTE, SAC_NO_ARG, &sac_status);
    for (uint8_t i = 0; i < steps; i++) {
        sac_processing_ctrl(back_channel_volume_processing, back_channel_sac_pipeline,
                            SAC_VOLUME_INCREASE, SAC_NO_ARG, &sac_status);
    }
}

/** @brief Queue a media command for the node. Registered as at_cmd_core's forwarding hook.
 *
 *  Runs in AT command context, so it only sets the pending slot; data_callback() puts it on
 *  the air on the next 10 ms tick.
 *
 *  Volume is filtered out rather than forwarded. AT+VOL here has always adjusted this
 *  device's own back channel (at_set_vol()), and that is what ODM hosts are integrated
 *  against; forwarding it as well would make one AT command change two different speakers.
 *  AT_CMD_VOL therefore stays reserved and never reaches the air.
 */
static void at_cmd_tx(uint8_t cmd_type, uint8_t value)
{
    (void)value;

    if (cmd_type == AT_CMD_VOL) {
        return;
    }

    s_pending_cmd = cmd_type;
}

/** @brief Emit one self-diagnosing trap line, then halt.
 *
 *  Prints "<tag> TRAP <file>:<line> code=<n>" naming the assert that tripped (captured
 *  by the ASSERT_SWC/SAC_STATUS wrappers at the top of this file), so a repro is
 *  diagnosable from the log alone. The line goes to both sinks because the DG's debug
 *  channel is board-dependent: facade_stats_write() is the ST-Link VCP / UART4 path used
 *  by LINK_WATCH on U535, facade_print_error_string() is the USB CDC path elsewhere (and
 *  lights the RGB red). Re-emitted on a slow loop rather than printed once, because the
 *  operator usually attaches the terminal only after noticing the box has wedged.
 *  The delay is a nop spin on purpose — SysTick may already be dead in a trap context.
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

    while (1) {
        facade_stats_write(buffer);
        for (volatile uint32_t i = 0; i < 8000000u; i++) {
            __asm volatile("nop");
        }
    }
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
 *  @return Accumulator size.
 */
static uint32_t get_accumulator_size(sac_pipeline_t *pipeline)
{
    uint32_t acc_size = 0;
    sac_status_t sac_status = SAC_OK;
    uint8_t curr_fbk_index = 0;

    curr_fbk_index = sac_fallback_get_current_mode(&main_channel_fallback_instance, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    acc_size = (pipeline->producer->cfg.audio_payload_size * main_channel_acc_mul[curr_fbk_index]) /
               main_channel_acc_div[curr_fbk_index];

    return acc_size;
}

/** @brief Iterate through fallback states.
 */
static void change_fallback_state(void)
{
    sac_status_t sac_status;

    fallback_state = (fallback_state + 1) % FALLBACK_STATE_COUNT;

    if (device_pairing_state != DEVICE_PAIRED) {
        return;
    }

    sac_fallback_set_manual_mode(&main_channel_fallback_instance, (fallback_state > FALLBACK_AUTO), &sac_status);

    if (fallback_state > FALLBACK_AUTO) {
        sac_fallback_set_current_mode(&main_channel_fallback_instance, (fallback_state - 1), &sac_status);
    }
}
