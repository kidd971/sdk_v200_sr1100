/** @file  puretone_unidirectional_coord.c
 *  @brief This application creates a puretone unidirectional audio stream at 96kHz/24-bit depth from the audio
 *         interface of the Coordinator to the audio interface of the Node. It utilizes multiple fallback modes to
 *         reduce audio quality down to 48 kHz ADPCM to preserve link quality under varying conditions. Additionally,
 *         there is a bidirectional link for user data and link margin, which supports dynamic fallback updates.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES ******************************************************************/
#include <stdio.h>

#include "pairing_api.h"
#include "pairing_cfg.h"
#include "puretone_unidirectional_facade.h"
#include "sac_api.h"
#include "sac_cfg.h"
#include "sac_compression.h"
#include "sac_endpoint_swc.h"
#include "sac_fallback.h"
#include "sac_fallback_gate.h"
#include "sac_hal_facade.h"
#include "sac_mute_packet.h"
#include "sac_packing.h"
#include "sac_sample_accumulator.h"
#include "sac_src_cmsis.h"
#include "sac_stats.h"
#include "swc_api.h"
#include "fw_version.h"
#include "at_cmd_core.h"
#include "at_cmd_core_facade.h"
#include "at_cmd_core.h"
#include "puretone_link_data.h"  /* user_data_t: the wire format, shared with the node */
#include "reconnect_store.h"
#include "swc_cfg.h"
#include "swc_cfg_coord.h"
#include "swc_error.h"
#include "swc_stats.h"
#include "swc_utils.h"

/* CONSTANTS ******************************************************************/
/* Total memory needed for the Audio Core.
 *
 * Raised with the bottom rungs' latency. The queues are sized in packets --
 * (sample_rate * (target_ms - codec_ms)) / (sample_count * 1000) -- so 15 -> 40 ms takes the
 * deepest queue from ~33 packets to ~93, and every one of them is a buffer out of this pool.
 * Deliberately generous: the pool is a fixed array, so running short is not a degradation but an
 * init failure, and print_stats() now reports what is actually allocated -- trim this to the
 * measured figure rather than to a calculation. */
#define SAC_MEM_POOL_SIZE 100000
/* Total memory needed for the Wireless Core. */
#define SWC_MEM_POOL_SIZE 10500
/* The data connection supports up to 16 bytes. */
/* MAX_DATA_PAYLOAD_SIZE comes from puretone_link_data.h, next to the struct it has to hold. */

/** @brief How long to wait for the stored peer to answer before giving up on a silent
 *         reconnect, in ms.
 *
 *  Ten seconds. Long enough for a node that is powering up at the same moment, short enough
 *  that a user who is actually waiting to pair does not conclude the device is dead.
 */
#define RECONNECT_TIMEOUT_MS 10000

/** @brief How long without a packet from the node before link_is_up() calls the link down.
 *
 *  The node sends one every DATA_TX_PERIOD_MS, so this is 20 consecutive misses -- clear of
 *  ordinary loss even at the deepest rung, and far less twitchy than the Wireless Core's own
 *  20 ms threshold, which exists for the audio pipeline rather than for a link decision.
 */
#define NODE_RX_TIMEOUT_MS 200

/* Build the data connection before audio and give it the top priority. See app_swc_core_init(). */
#ifndef DG_DATA_FIRST
#define DG_DATA_FIRST 0
#endif

/** @brief Silence long enough to mean the peer restarted rather than was momentarily blocked.
 *
 *  Three seconds. The two cases are indistinguishable at any single instant -- both are
 *  "nothing is arriving" -- but they are an order of magnitude apart in duration: a reboot
 *  takes seconds, an obstruction lasts hundreds of milliseconds. MD/link_dropout_arms_ledger.md
 *  measures obstruction in the hundreds of ms, which is what sets the floor here.
 *
 *  Used only to decide whether a ladder pin survives. A pin says "this link, as it stands,
 *  cannot hold a higher rung"; a peer that has since restarted is not that link any more, so
 *  the pin has nothing to say about it and is dropped. An obstruction is the same link and
 *  the pin stands.
 */
#define NODE_RESTART_SILENCE_MS 3000

/** @brief How long a link must hold before the ladder is allowed to judge it.
 *
 *  Two seconds. For the first moments of a link the transmit queue says nothing about radio
 *  quality: the audio producer is already filling while the connection is still coming up, so
 *  the queue is deep for reasons that have nothing to do with the air. is_link_bad() is
 *  queue-size-high OR cca-bad, so it reads that warm-up as a bad link and walks the ladder
 *  down -- 1 to 4 in one go, every time either end restarts.
 *
 *  Two seconds is two hundred node reports at DATA_TX_PERIOD_MS, which is far more than the
 *  queue average needs to converge, and it is short enough that a link which really is bad
 *  spends only that long at the top before the ladder starts working normally.
 */
#define LADDER_SETTLE_MS 2000

/* Length of the statistics array used for terminal display. */
#define STATS_ARRAY_LENGTH 3000
/* Period for data transmission timer in ms.
 * With USB audio, the audio connection stops transmitting when the host is not streaming, making this periodic data
 * transmission the Node's only synchronization source (beacon). This period must not exceed 10 ms so that two data
 * frames fit within the Node's 21 ms sync-loss timeout, tolerating the loss of one frame.
 */
#define DATA_TX_PERIOD_MS 10
/* Size of the buffer used to print errors. */
#define ERROR_MESSAGE_BUFFER_SIZE 50
/* Interval to print statistics in ms. */
/* Bring the receiver up on the calibration saved in the radio's NVM as well as the fresh one.
 * SPARK's audio demo does; the SDK example this app came from does not. */
#ifndef RADIO_USE_SAVED_CALIB
#define RADIO_USE_SAVED_CALIB false
#endif
#define PRINT_INTERVAL_MS 1000
/* The stock statistics block is about thirty lines a second, which is unreadable while listening
 * for a dropout that lasts a few tens of milliseconds. Set to 1 to get it back; the compact line
 * carries the counters that separate the failure modes and nothing else. */
#ifndef STATS_VERBOSE
#define STATS_VERBOSE 0
#endif

/* **** Fallback **** */
/* Number of SWC fallback modes. */
#define SWC_FALLBACK_MODE_COUNT 4

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
    /*! Forced fallback state to 24kHz compressed audio. */
    FALLBACK_24K_ADPCM_STEREO,
    /*! Total number of fallback states. */
    FALLBACK_STATE_COUNT,
} fallback_states_t;

/* user_data_t now lives in puretone_link_data.h, shared with the other role and with the
 * puretone_headset line. Both ends of a link must agree byte for byte, and this file and
 * its peer each used to carry their own copy of the definition -- editing one and not the
 * other compiles and links cleanly, then misreads every field past the divergence. */

/* PRIVATE GLOBALS ************************************************************/
/* **** Audio Core **** */
/** Sample format of audio samples produced by the codec of the Coordinator. */
static const sac_sample_format_t I2S_SAC_SAMPLE_FORMAT = {
    .bit_depth = SAC_24BITS,
    .sample_encoding = SAC_SAMPLE_UNPACKED,
};

/* Sample format of audio samples sent by the SWC of the Coordinator. */
static const sac_sample_format_t MAIN_CHANNEL_SAC_SAMPLE_FORMAT = {
    .bit_depth = SAC_24BITS,
    .sample_encoding = SAC_SAMPLE_PACKED,
};

#define MAIN_CHANNEL_PRODUCER_SAC_SAMPLE_FORMAT \
    (USB_AUDIO_ENABLED ? MAIN_CHANNEL_SAC_SAMPLE_FORMAT : I2S_SAC_SAMPLE_FORMAT)

static uint8_t audio_memory_pool[SAC_MEM_POOL_SIZE];
static sac_pipeline_t *sac_pipeline;

/* **** Processing Stages **** */
static sac_fallback_instance_t sac_fallback_instance;
static sac_processing_t *sac_fallback_processing;
static sac_packing_instance_t audio_packing_instance;
static sac_processing_t *sac_packing_processing;
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
/* Second, independent SRC for the 24 kHz rung. A ratio is fixed at init, so the 96->48 kHz
 * instance above cannot be reused, and the two ends' ratios have to mirror each other: on the
 * packet that ends a discard the decimator appends (FIR_NUMTAPS / ratio * channel_count) / 2
 * samples and the interpolator expects exactly that many, which only agrees when divide_ratio
 * here equals multiply_ratio on the node. */
static src_cmsis_instance_t main_channel_downsampling4_instance;
static sac_processing_t *main_channel_downsampling4_processing;
static sac_mute_packet_instance_t main_channel_mute_packet_instance;
static sac_processing_t *main_channel_mute_packet_processing;

/* **** Endpoints **** */
static sac_endpoint_t *audio_producer;
static ep_swc_instance_t swc_consumer_instance;
static sac_endpoint_t *swc_consumer;

/* **** Wireless Core **** */
static uint8_t swc_memory_pool[SWC_MEM_POOL_SIZE];

static const uint32_t timeslot_us[] = SCHEDULE;
static const uint32_t channel_sequence[] = CHANNEL_SEQUENCE;
static const uint32_t channel_frequency[] = CHANNEL_FREQ;

static const int32_t tx_timeslots[] = COORD_TIMESLOTS;
static const int32_t rx_timeslots[] = NODE_TIMESLOTS;

/* There is a unidirectional link for audio and a bidirectional link for data. */
static swc_connection_t *tx_audio_conn;
static swc_connection_t *tx_data_conn;
static swc_connection_t *rx_data_conn;

/* **** Application Specific **** */
static facade_certification_mode_t certification_mode;
/* Variables supporting pairing between the two devices. */
static device_pairing_state_t device_pairing_state;
static pairing_cfg_t app_pairing_cfg;
static pairing_assigned_address_t pairing_assigned_address;
static pairing_discovery_list_t pairing_discovery_list[PAIRING_DISCOVERY_LIST_SIZE];

/* When the node's data packet last arrived, and whether one ever has since the connections
 * were built. link_is_up() answers with these -- see there for why it does not ask the
 * Wireless Core. Written from the RX callback, read from the main loop; a 32-bit store is
 * atomic on this core. */
static volatile uint32_t s_node_rx_tick;
static volatile bool s_node_rx_seen;

/* True while try_boot_reconnect() owns a half-open link and is polling it. The button
 * handler must not tear down connections the reconnect loop is still dereferencing, so it
 * defers through s_boot_reconnect_abort and the loop unwinds first. */
static bool s_boot_reconnect_active;
static bool s_boot_reconnect_abort;

/* Why the fallback ladder is currently held in manual mode. Two separate reasons, tracked
 * separately on purpose: both are implemented with the same sac_fallback_set_manual_mode()
 * flag, so a single boolean would let releasing one silently release the other. */
static bool s_ladder_frozen; /* peer is absent -- descending would be meaningless */
static bool s_ladder_pinned; /* stepped down into the bottom rung -- stay there */

/* Last mode seen by fallback_hold_handler(), so it can tell a step DOWN into the bottom rung
 * from merely being parked there. Initialised on the first pass. Only that function reads
 * s_ladder_prev_mode, so it goes with it when the ladder is forced to one rung. */
#if FALLBACK_FORCE_MODE < 0
static uint8_t s_ladder_prev_mode;
#endif
static bool s_ladder_prev_valid;

/* Media key waiting to be sent to the node, an at_cmd_code_t. Edge triggered: set from the AT
 * handler in the main loop, read and cleared in data_callback() from the TIM16 ISR, so
 * volatile. See the note on cmd_type in puretone_link_data.h for why that is acceptable here
 * and would not be for an alarm. */
static volatile uint8_t s_pending_cmd;

/* Times data_callback() found the data connection's queue full and dropped its packet -- dfull
 * in the [DG] line. Incremented from the TIM16 ISR, read from the main loop. */
static volatile uint32_t s_data_queue_full_count;

/* When the peer most recently became reachable, and whether the ladder has been let go since.
 * See LADDER_SETTLE_MS. s_link_up_tick has no reader once the ladder is forced to one rung. */
#if FALLBACK_FORCE_MODE < 0
static uint32_t s_link_up_tick;
#endif
static bool s_link_up_valid;
static bool s_link_settled;

/** @brief Outcome of a boot auto-reconnect attempt. */
typedef enum {
    BOOT_RECONNECT_OK,   /* The stored link was re-established; stay paired and stream. */
    BOOT_RECONNECT_PAIR, /* No usable record, or the user asked to pair mid-attempt. */
    BOOT_RECONNECT_IDLE, /* Had a record but the node was not up yet: keep the coordinator's
                            core running -- it is the timebase master -- so the node syncs
                            whenever it does boot. */
} boot_reconnect_result_t;

/* Forced fallback state, starts in automatic mode. */
static fallback_states_t fallback_state;

/* Main channel audio sample accumulator settings. */
uint8_t main_channel_acc_mul[] = MAIN_CHANNEL_ACC_MUL;
uint8_t main_channel_acc_div[] = MAIN_CHANNEL_ACC_DIV;

/* PRIVATE FUNCTION PROTOTYPE *************************************************/
static void app_init(void);
static void app_swc_core_init(pairing_assigned_address_t *app_pairing, swc_error_t *swc_err);
static void app_audio_core_init(void);

/* **** Callbacks **** */
static void conn_tx_audio_success_callback(void *conn, void *arg);
static void conn_tx_data_success_callback(void *conn, void *arg);
static void conn_rx_data_success_callback(void *conn, void *arg);
static void audio_rx_complete_callback(void);
static void audio_process_callback(void);
static void data_callback(void);
static void pairing_process_callback(void);
static void pairing_button_callback(void);
static void change_fallback_state(void);

/* **** Processing stages **** */
static void app_audio_core_fallback_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_packing_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_downsampling_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_downsampling_discard_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_mute_packet_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_compressing_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_compression_discard_interface_init(sac_processing_interface_t *iface);

/* **** Button actions **** */
static void enter_pairing_mode(void);
static bool link_is_up(void);
static boot_reconnect_result_t try_boot_reconnect(void);
static void unpair_device(bool forget_peer);
static void abort_pairing_procedure(void);

/* Fallback LED and terminal display. */
static void fallback_hold_handler(void);
static bool should_print_stats(void);
static void print_stats(void);
static void print_diagnostics(void);
#if !STATS_VERBOSE
static void print_stats_compact(void);
static const char *fallback_mode_name(uint8_t mode);
#endif


/* **** AT command core callbacks **** */
static void at_start_pairing(void);
static void at_start_connect(void);
static void at_start_disconnect(void);
static void at_start_shutdown(void);
static bool at_get_link_status(void);
static int32_t at_get_link_margin(void);
static uint8_t at_get_fb_rung(void);
static void at_cmd_tx(uint8_t cmd_type, uint8_t value);


static void wireless_send_data(const void *transmitted_data, uint8_t size, swc_error_t *swc_err);
static uint16_t wireless_read_data(void *received_data, uint8_t size, swc_error_t *swc_err);
static uint32_t get_accumulator_size(sac_pipeline_t *pipeline);

/* PUBLIC FUNCTIONS ***********************************************************/
int main(void)
{
#if USB_AUDIO_ENABLED
    /* Configure usb audio before board initialization. */
    facade_configure_coord_usb_audio();
#endif
    /* Initialize the board and all GPIOs and peripherals for minimal operations. */
    facade_board_init();

    /* AT command channel, first, because on some boards it IS the console: u5a5 has no
     * other output at all, so a banner printed before this runs is a banner nobody sees.
     * Everything it needs is already up -- facade_board_init() has configured the clocks
     * and GPIO -- and putting it here also means a failure anywhere later, including in
     * audio setup, is still reportable. */
    at_cmd_core_init();
    at_cmd_core_set_device_role(AT_DEVICE_ROLE_COORDINATOR);
    at_cmd_core_register_pair_cb(at_start_pairing);
    at_cmd_core_register_connect_cb(at_start_connect);
    at_cmd_core_register_disconnect_cb(at_start_disconnect);
    at_cmd_core_register_shutdown_cb(at_start_shutdown);
    at_cmd_core_register_link_status_cb(at_get_link_status);
    at_cmd_core_register_link_edge_cb(facade_link_status);
    at_cmd_core_register_link_margin_cb(at_get_link_margin);
    at_cmd_core_register_fb_rung_cb(at_get_fb_rung);
    at_cmd_core_register_cmd_tx_cb(at_cmd_tx);
    at_cmd_core_register_i2s_mux_cb(facade_set_i2s_mux);
    at_cmd_core_notify_build(AT_CMD_CORE_BUILD_ID);
    at_cmd_core_notify_uwb_ready();

    /* First thing on the console, before anything can fail. Without it "nothing came out of
     * the serial port" has two meanings that cannot be told apart: the console is broken, or
     * the console is fine and the application died before printing anything. On a board whose
     * only failure indication is a blinking LED, establishing that distinction costs a whole
     * test cycle, every time.
     *
     * __DATE__ / __TIME__ expand HERE, in the application translation unit, so an incremental
     * build that recompiles only this file still reports its own timestamp -- which is the
     * point of printing them. Naming the role matters too: the two binaries come from
     * different presets with opposite I2S clock roles, and flashing the wrong one fails as
     * silence, which looks like something else entirely.
     */
    {
        char banner[112];

        snprintf(banner, sizeof(banner), "\r\n[BOOT] puretone_unidirectional coordinator " BOARD_NAME " " RADIO_TAG " " FW_VERSION_STRING " "
                                         __DATE__ " " __TIME__ "\r\n");
        facade_print_string(banner);
    }

#if DEBUG_IO_TXEN
    /* Scope marker for correlating this side's frame timing against the radio module's own
     * SYNC_TXEN pin (SR1120 pin 5, module test point TP1). Off unless -DDEBUG_IO_TXEN=1. */
    facade_debug_txen_io_init();
#endif

    /* Initialize wireless core context switch handler before pairing is available. */
    facade_set_context_switch_handler(swc_connection_callbacks_processing_handler);

    facade_button_callbacks_t button_callbacks = {
        .pairing_callback = pairing_button_callback,
        .fallback_callback = change_fallback_state,
    };
    facade_set_button_callbacks(button_callbacks);

    /* Audio process timer initialization. */
    facade_audio_process_timer_init(audio_process_callback);

    /* Timer that updates statistics display every second and transmits button state to Node at the DATA_TX_PERIOD_MS
     * interval.
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
            /* Statistics are displayed at intervals set by the timer when paired; timer stops if unpaired. */
            if (should_print_stats()) {
                print_stats();
            }
        }
    }

    device_pairing_state = DEVICE_UNPAIRED;

    /* Boot auto-reconnect: if a previous pairing was persisted, re-establish it silently
     * rather than pairing again. Only enter pairing when there is no usable record -- a
     * factory device, or one the user unpaired -- or when the user asks for it mid-attempt.
     *
     * A record that simply could not reach its node in time does NOT re-pair. This side is
     * the timebase master, so its wireless core is left running and the node syncs whenever
     * it boots; tearing down and pairing again would throw away a working record because the
     * peer happened to be switched off. */
    if (try_boot_reconnect() == BOOT_RECONNECT_PAIR) {
        enter_pairing_mode();
    }

    while (1) {
        facade_button_handling();
        at_cmd_core_process();

        if (device_pairing_state == DEVICE_PAIRED) {
            fallback_hold_handler();
        }

        /* Statistics are displayed at intervals set by the timer when paired; timer stops if unpaired. */
        if (should_print_stats()) {
            print_stats();
        }

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
    const uint8_t fallback_cca_try_count[] = {SWC_CCA_AUDIO_FBK_1_TRY_COUNT, SWC_CCA_AUDIO_FBK_2_TRY_COUNT,
                                              SWC_CCA_AUDIO_FBK_3_TRY_COUNT, SWC_CCA_AUDIO_FBK_4_TRY_COUNT};

    /* swc_connection_set_fallback_cfg() requires the thresholds in descending order and asserts
     * if they are not -- a red LED at init, well after the edit that caused it. The bottom rung's
     * size follows from its accumulator ratio and resampler ratio, so the ladder can be inverted
     * by changing one number in sac_cfg.h; catch it here instead. */
    _Static_assert(sizeof(fallback_thresholds) == SWC_FALLBACK_MODE_COUNT,
                   "fallback threshold count must match SWC_FALLBACK_MODE_COUNT");
    _Static_assert(sizeof(fallback_cca_try_count) == SWC_FALLBACK_MODE_COUNT,
                   "CCA try count array must match SWC_FALLBACK_MODE_COUNT");
    _Static_assert(MAIN_CHANNEL_FBK_4_PAYLOAD_SIZE < MAIN_CHANNEL_FBK_3_PAYLOAD_SIZE &&
                       MAIN_CHANNEL_FBK_3_PAYLOAD_SIZE < MAIN_CHANNEL_FBK_2_PAYLOAD_SIZE &&
                       MAIN_CHANNEL_FBK_2_PAYLOAD_SIZE < MAIN_CHANNEL_FBK_1_PAYLOAD_SIZE,
                   "SWC fallback thresholds must stay in descending payload order");
    _Static_assert(MAIN_CHANNEL_FBK_1_PAYLOAD_SIZE <= UINT8_MAX,
                   "fallback thresholds are uint8_t; a larger payload wraps silently");
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

    /* Initialize the radio.
     *
     * The flag is pwr_cycle_saved_calib -- whether to bring up the receiver using the calibration
     * saved in the radio's NVM rather than only what swc_radio_module_calib() just produced. The
     * SDK example passes false and this app inherited it; SPARK's own audio demo passes true on
     * the same EVK hardware.
     *
     * It is a candidate for the close-range dropouts because calibration is what the demodulator
     * runs on, and the measured failure is decode failure -- rx_rej climbing while cca_fail does
     * not. Everything else comparable between the two firmwares has been checked and matches or
     * favours this one, on the same board, so what is left is either this, the schedule, or a
     * difference inside the prebuilt wireless core between SDK versions.
     */
    swc_radio_module_init(radio_handle, RADIO_USE_SAVED_CALIB, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* **** TX Connections **** */
    /* ** TX Audio Connection ** */
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

    /* DG_DATA_FIRST takes the certification branch's order in normal operation: data built
     * first and given the top priority, audio second. An experiment arm, off by default.
     *
     * Why it exists: on the walk back from an obstruction the node reported
     * LE_UWB_DISCONNECTED while audio was still flowing, and the [DG] line showed the Wireless
     * Core giving this data connection no timeslots at all -- dslot=0 with dfull=100, dcca=0 --
     * for 5 s in one capture and 12 s and counting in another (2026-09-29, u5a5, ISI 1), even
     * through seconds when audio left 2000+ slots idle. The node's only heartbeat is this
     * connection, so its silence alone is the disconnect. The core is prebuilt and the
     * reason is not visible from here; this tests whether it is tied to the connection being
     * the lower-priority, second-built one on shared slots. Data costs ~100 slots/s of 3810,
     * so letting it win costs audio little. Coordinator only: the node's connections are
     * receive-side and it does not read this. */
    if ((certification_mode == FACADE_CERTIF_DATA) || DG_DATA_FIRST) {
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
    const swc_connection_concurrency_cfg_t rx_data_concurrency_cfg = {
        .enabled = true,
        .try_count = MAIN_CHANNEL_SWC_CCA_DATA_TRY_COUNT,
        .retry_time = MAIN_CHANNEL_SWC_CCA_DATA_RETRY_TIME,
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

    /* Wireless Core setup. */
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

#if DEBUG_IO_TXEN
    /* High for the width of this callback: one pulse per audio frame the Coordinator got ACK'd.
     * Overlay it with the module's TP1 (SYNC_TXEN) to check the two are 1:1 and to measure the
     * offset. The rising edge is the reference -- this runs in the deferred SWC callback context,
     * so the absolute delay to the RF burst is fixed-ish but not zero. */
    facade_debug_txen_io_set();
#endif

    facade_tx_audio_conn_status();

    /* Trigger audio process. */
    facade_audio_process_timer_trigger();

#if DEBUG_IO_TXEN
    facade_debug_txen_io_clear();
#endif
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
    (void)conn;
    (void)arg;

    sac_status_t sac_status = SAC_OK;
    swc_error_t swc_err = SWC_ERR_NONE;
    user_data_t received_user_data = {0};
    uint16_t read_data_size;

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
        sac_fallback_set_rx_link_margin(&sac_fallback_instance, received_user_data.link_margin, &sac_status);
        ASSERT_SAC_STATUS(sac_status);

        /* Media keys from the node -- someone pressing play on the headset rather than on the
         * source. Edge triggered, so a lost packet loses the press; acceptable only because a
         * person is in the loop to press it again. AT_CMD_VOL never arrives here: the node
         * owns its own output level and filters it out before it reaches the air.
         *
         * notify_*_received() raises +EVENT for this side's SOC. It also calls the per-key
         * hardware callbacks, which this side deliberately leaves unregistered -- registering
         * them to forward would echo every key back to the node at the packet rate. */
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

        /* Vendor pass-through, unconditionally: a packet carrying no command still carries
         * the node's acknowledgement of ours. De-duplication and the +EVENT line happen in
         * the AT core. */
        user_data_deliver_vendor(&received_user_data);

        /* Peer heartbeat for link_is_up(). Stamped here because reaching this line is the
         * only direct evidence this side ever gets that the node is alive. */
        s_node_rx_tick = facade_get_tick_ms();
        s_node_rx_seen = true;
    }
}

/** @brief Initialize the Audio Core.
 */
static void app_audio_core_init(void)
{
    sac_status_t sac_status = SAC_OK;

    sac_endpoint_interface_t producer_iface = {0};
    sac_endpoint_interface_t swc_consumer_iface = {0};

    sac_processing_interface_t fallback_iface = {0};
    sac_processing_interface_t packing_iface = {0};
    sac_processing_interface_t downsampling_iface = {0};
    sac_processing_interface_t downsampling_discard_iface = {0};
    sac_processing_interface_t mute_packet_iface = {0};
    sac_processing_interface_t main_channel_compression_iface = {0};
    sac_processing_interface_t main_channel_compression_discard_iface = {0};

    sac_endpoint_swc_init(NULL, &swc_consumer_iface);
    sac_facade_audio_endpoint_init(&producer_iface, NULL);
    facade_set_audio_complete_callback(NULL, audio_rx_complete_callback);

    app_audio_core_fallback_interface_init(&fallback_iface);
    app_audio_core_packing_interface_init(&packing_iface);
    app_audio_core_downsampling_interface_init(&downsampling_iface);
    app_audio_core_downsampling_discard_interface_init(&downsampling_discard_iface);
    app_audio_core_mute_packet_interface_init(&mute_packet_iface);
    app_audio_core_compression_discard_interface_init(&main_channel_compression_discard_iface);
    app_audio_core_compressing_interface_init(&main_channel_compression_iface);

    swc_consumer_instance.connection = tx_audio_conn;

    /* Initialize Audio Core. */
    sac_cfg_t core_cfg = {
        .memory_pool = audio_memory_pool,
        .memory_pool_size = SAC_MEM_POOL_SIZE,
    };
    sac_init(core_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /*
     * Audio Pipeline
     * ==============
     *
     * ***** NORMAL MODE USB (Fallback mode 0) *****
     * Input:      Stereo stream of 96kHz/24-bit depth samples.
     * Output:     Stereo stream of 96kHz/24-bit is sent over the air to the Node.
     *
     * +-----+    +-----+
     * | USB | -> | SWC |
     * +-----+    +-----+
     *
     * **** NORMAL MODE I2S (Fallback mode 0) *****
     * Input:      Stereo stream of 96kHz/24-bit depth samples, encoded on 32 bits.
     * Processing: Packing from 32 bits to 24 bits audio samples.
     * Output:     Stereo stream of 96kHz/24-bit is sent over the air to the Node.
     *
     * +-----+    +--------------------+    +-----+
     * | I2S | -> | Packing to 24 bits | -> | SWC |
     * +-----+    +--------------------+    +-----+
     *
     * **** FALLBACK MODE USB (Fallback mode 1) *****
     * Input:      Stereo stream of 96kHz/24-bit packed depth samples.
     * Processing: Audio sample accumulator 1.7x.
     * Processing: Downsampling audio samples from 96kHz to 48kHz.
     * Output:     Stereo stream of 48kHz/24-bit is sent over the air to the Node.
     *
     * +-----+    +------------------+    +-----------------+    +-----+
     * | USB | -> | Accumulator 1.7x | -> | Downsampling 2x | -> | SWC |
     * +-----+    +------------------+    +-----------------+    +-----+
     *
     * **** FALLBACK MODE I2S (Fallback mode 1) *****
     * Input:      Stereo stream of 96kHz/24-bit depth samples, encoded on 32 bits.
     * Processing: Audio sample accumulator 1.7x.
     * Processing: Downsampling audio samples from 96kHz to 48kHz.
     * Processing: Packing from 32 bits to 24 bits audio samples.
     * Output:     Stereo stream of 48kHz/24-bit is sent over the air to the Node.
     *
     * +-----+    +------------------+    +-----------------+    +--------------------+    +-----+
     * | I2S | -> | Accumulator 1.7x | -> | Downsampling 2x | -> | Packing to 24 bits | -> | SWC |
     * +-----+    +------------------+    +-----------------+    +--------------------+    +-----+
     *
     * **** FALLBACK MODE USB (Fallback mode 2) *****
     * Input:      Stereo stream of 96kHz/24-bit depth samples.
     * Processing: Audio sample accumulator 1.7x.
     * Processing: Downsampling audio samples from 96kHz to 48kHz.
     * Processing: Packing from 24 bits to 16 bits audio samples.
     * Output:     Stereo stream of 48kHz/16-bit is sent over the air to the Node.
     *
     * +-----+    +------------------+    +-----------------+    +--------------------+    +-----+
     * | USB | -> | Accumulator 1.7x | -> | Downsampling 2x | -> | Packing to 16 bits | -> | SWC |
     * +-----+    +------------------+    +-----------------+    +--------------------+    +-----+
     *
     * **** FALLBACK MODE I2S (Fallback mode 2) *****
     * Input:      Stereo stream of 96kHz/24-bit depth samples, encoded on 32 bits.
     * Processing: Audio sample accumulator 1.7x.
     * Processing: Downsampling audio samples from 96kHz to 48kHz.
     * Processing: Packing from 32 bits to 16 bits audio samples.
     * Output:     Stereo stream of 48kHz/16-bit is sent over the air to the Node.
     *
     * +-----+    +------------------+    +-----------------+    +--------------------+    +-----+
     * | I2S | -> | Accumulator 1.7x | -> | Downsampling 2x | -> | Packing to 16 bits | -> | SWC |
     * +-----+    +------------------+    +-----------------+    +--------------------+    +-----+
     *
     * **** FALLBACK MODE USB (Fallback mode 3) *****
     * Input:      Stereo stream of 96kHz/24-bit depth samples.
     * Processing: Audio sample accumulator 2.3x.
     * Processing: Downsampling audio samples from 96kHz to 48kHz.
     * Processing: Audio compression using ADPCM.
     * Output:     ADPCM compressed stereo stream of 48 kHz/24-bit is sent over the air to the Node.
     *
     * +-----+    +------------------+    +-----------------+    +-------------------+    +-----+
     * | USB | -> | Accumulator 2.3x | -> | Downsampling 2x | -> | ADPCM Compression | -> | SWC |
     * +-----+    +------------------+    +-----------------+    +-------------------+    +-----+
     *
     * **** FALLBACK MODE I2S (Fallback mode 3) *****
     * Input:      Stereo stream of 96kHz/24-bit depth samples, encoded on 32 bits.
     * Processing: Audio sample accumulator 2.3x.
     * Processing: Downsampling audio samples from 96kHz to 48kHz.
     * Processing: Audio compression using ADPCM.
     * Output:     ADPCM compressed stereo stream of 48 kHz/24-bit is sent over the air to the Node.
     *
     * +-----+    +------------------+    +-----------------+    +-------------------+    +-----+
     * | I2S | -> | Accumulator 2.3x | -> | Downsampling 2x | -> | ADPCM Compression | -> | SWC |
     * +-----+    +------------------+    +-----------------+    +-------------------+    +-----+
     */

    /* Initialize codec producer endpoint. */
    sac_endpoint_cfg_t producer_cfg = {
        .use_encapsulation = false,
        .delayed_action = !USB_AUDIO_ENABLED,
        .channel_count = MAIN_CHANNEL_CHANNEL_COUNT,
        .audio_payload_size = USB_AUDIO_ENABLED ? MAIN_CHANNEL_SWC_PAYLOAD_SIZE : MAIN_CHANNEL_I2S_PAYLOAD_SIZE,
        .queue_size = SAC_MIN_PRODUCER_QUEUE_SIZE + (USB_AUDIO_ENABLED ? MAIN_CHANNEL_USB_FS_PRODUCER_BUFFERING : 0),
    };
    audio_producer = sac_endpoint_init(NULL, "Audio EP (Producer)", producer_iface, producer_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    sac_fallback_instance.connection = tx_audio_conn;
    sac_fallback_instance.is_tx_device = true;
    sac_fallback_instance.get_tick = facade_get_tick_ms;
    sac_fallback_instance.tick_frequency_hz = 1000;
    /* Report the rung as it changes instead of letting the AT layer poll for it. The whole
     * warning this link can give a host is a couple of hundred milliseconds (see
     * AT_UWB_QUALITY_WEAK_RUNG), and a polling interval would be most of it. This fires from
     * the ladder itself; the notify only marks state dirty. */
    sac_fallback_instance.fallback_state_change_callback = at_cmd_core_notify_fb_rung_change;
    sac_fallback_processing = sac_processing_stage_init(&sac_fallback_instance, "Main channel fallback TX",
                                                        fallback_iface, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Audio sample accumulator processing stage initialization. */
    sac_processing_interface_t sample_accumulator_iface = {
        .init = sac_sample_accumulator_init,
        .process = sac_sample_accumulator_process,
        .gate = sac_fallback_gate_is_process_active,
    };
    /* Increase packet size in fallback to increase retx. */
    main_channel_sample_accumulator_instance.max_accumulator_size =
        (producer_cfg.audio_payload_size * MAIN_CHANNEL_MAX_ACC_MUL) / MAIN_CHANNEL_MAX_ACC_DIV;
    main_channel_sample_accumulator_instance.get_accumulator_size = get_accumulator_size;
    main_channel_sample_accumulator_processing =
        sac_processing_stage_init((void *)&main_channel_sample_accumulator_instance, "Audio Sample Accumulator",
                                  sample_accumulator_iface, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Processing stage that downsamples the audio samples from 96kHz to 48kHz. */
    main_channel_downsampling_instance.cfg.multiply_ratio = SAC_SRC_ONE;
    main_channel_downsampling_instance.cfg.divide_ratio = SAC_SRC_TWO;
    main_channel_downsampling_instance.cfg.payload_size = USB_AUDIO_ENABLED ? MAIN_CHANNEL_SWC_PAYLOAD_SIZE :
                                                                              MAIN_CHANNEL_I2S_PAYLOAD_SIZE;
    main_channel_downsampling_instance.cfg.payload_size =
        (main_channel_downsampling_instance.cfg.payload_size * MAIN_CHANNEL_MAX_ACC_MUL) / MAIN_CHANNEL_MAX_ACC_DIV;
    /* Downsampling does not change the sample format. */
    main_channel_downsampling_instance.cfg.input_sample_format = MAIN_CHANNEL_PRODUCER_SAC_SAMPLE_FORMAT;
    main_channel_downsampling_instance.cfg.output_sample_format = MAIN_CHANNEL_PRODUCER_SAC_SAMPLE_FORMAT;
    main_channel_downsampling_instance.cfg.channel_count = MAIN_CHANNEL_CHANNEL_COUNT;
    main_channel_downsampling_processing = sac_processing_stage_init((void *)&main_channel_downsampling_instance,
                                                                     "Audio Downsampling", downsampling_iface,
                                                                     &sac_status);
    main_channel_downsampling_discard_processing =
        sac_processing_stage_init((void *)&main_channel_downsampling_instance, "Audio Downsampling Discard",
                                  downsampling_discard_iface, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* 96 -> 24 kHz for the bottom rung. A single 1:4, not two chained 1:2: the interpolation path
     * validates its input against pipeline->_internal.current_sample_count, which the fallback
     * stage writes once per packet and never updates between stages, so a second chained stage
     * would always see twice what the check expects and be rejected.
     *
     * No discard variant is registered, deliberately. discard_active then never becomes true on
     * this instance, so the coordinator always emits exactly sample_count_out and the node always
     * sees the size it expects -- which keeps this rung clear of the oversized-transition-packet
     * path, the one part of this area the SDK has never exercised. The cost is a cold-FIR
     * transient when the ladder enters mode 4, audible as a soft pop at the 3<->4 boundary. */
    main_channel_downsampling4_instance.cfg.multiply_ratio = SAC_SRC_ONE;
    main_channel_downsampling4_instance.cfg.divide_ratio = SAC_SRC_FOUR;
    main_channel_downsampling4_instance.cfg.payload_size = USB_AUDIO_ENABLED ? MAIN_CHANNEL_SWC_PAYLOAD_SIZE :
                                                                               MAIN_CHANNEL_I2S_PAYLOAD_SIZE;
    main_channel_downsampling4_instance.cfg.payload_size =
        (main_channel_downsampling4_instance.cfg.payload_size * MAIN_CHANNEL_MAX_ACC_MUL) / MAIN_CHANNEL_MAX_ACC_DIV;
    /* Downsampling does not change the sample format. */
    main_channel_downsampling4_instance.cfg.input_sample_format = MAIN_CHANNEL_PRODUCER_SAC_SAMPLE_FORMAT;
    main_channel_downsampling4_instance.cfg.output_sample_format = MAIN_CHANNEL_PRODUCER_SAC_SAMPLE_FORMAT;
    main_channel_downsampling4_instance.cfg.channel_count = MAIN_CHANNEL_CHANNEL_COUNT;
    main_channel_downsampling4_processing = sac_processing_stage_init((void *)&main_channel_downsampling4_instance,
                                                                      "Audio Downsampling 1:4", downsampling_iface,
                                                                      &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Processing stage that packs into 24 bits before sending if fallback is deactivated. */
    audio_packing_instance.packing_mode = SAC_PACK_24BITS;
    sac_packing_processing = sac_processing_stage_init((void *)&audio_packing_instance, "Audio Packing", packing_iface,
                                                       &sac_status);
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

    /* Processing stage that packs the audio samples from 24 bits to 16 bits. */
    if (USB_AUDIO_ENABLED) {
        main_channel_fbk_packing_instance.packing_mode = SAC_SCALE_24BITS_16BITS;
    } else {
        main_channel_fbk_packing_instance.packing_mode = SAC_PACK_24BITS_16BITS;
    }
    main_channel_fbk_packing_processing = sac_processing_stage_init((void *)&main_channel_fbk_packing_instance,
                                                                    "Audio Packing", packing_iface, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Mute packet processing stage initialization. */
    main_channel_mute_packet_instance.is_tx = true;
    main_channel_mute_packet_processing = sac_processing_stage_init((void *)&main_channel_mute_packet_instance,
                                                                    "Mute packet", mute_packet_iface, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Initialize SWC consumer endpoint. */
    sac_endpoint_cfg_t swc_consumer_cfg = {
        .use_encapsulation = true,
        .delayed_action = false,
        .channel_count = MAIN_CHANNEL_CHANNEL_COUNT,
        .audio_payload_size = MAIN_CHANNEL_SWC_PAYLOAD_SIZE,
        .queue_size = MAIN_CHANNEL_LATENCY_QUEUE_SIZE,
    };
    swc_consumer = sac_endpoint_init((void *)&swc_consumer_instance, "SWC EP (Consumer)", swc_consumer_iface,
                                     swc_consumer_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Initialize audio pipeline. */
    sac_pipeline_cfg_t pipeline_cfg = {
        .do_initial_buffering = true,
        .max_payload_size = main_channel_sample_accumulator_instance.max_accumulator_size,
    };
    sac_pipeline = sac_pipeline_init("Audio -> SWC", audio_producer, pipeline_cfg, swc_consumer, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Add processing stages to the audio pipeline. */
    sac_pipeline_add_processing(sac_pipeline, sac_fallback_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_add_processing(sac_pipeline, main_channel_sample_accumulator_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_add_processing(sac_pipeline, main_channel_downsampling_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_add_processing(sac_pipeline, main_channel_downsampling_discard_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#if !USB_AUDIO_ENABLED
    /* When using I2S, packing is required to convert 24-bit audio aligned on 32-bit words. */
    sac_pipeline_add_processing(sac_pipeline, sac_packing_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif
    sac_pipeline_add_processing(sac_pipeline, main_channel_compression_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_add_processing(sac_pipeline, main_channel_compression_discard_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_add_processing(sac_pipeline, main_channel_fbk_packing_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_add_processing(sac_pipeline, main_channel_mute_packet_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Setup audio pipeline. */
    sac_pipeline_setup(sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Fallback mode configuration. */
    sac_fallback_mode_cfg_t mode_cfg = sac_fallback_mode_get_defaults();
    uint8_t mode_index;

    /* Fallback mode 0 configuration: 96kHz 24-bit. */
    mode_cfg.cca_bad_fail_count_threshold_perc = 2;
    mode_cfg.cca_bad_time_sec = 0.1;
    mode_cfg.consumer_buffer_load_threshold_tenths = 33;
    mode_cfg.sample_count = MAIN_CHANNEL_SAMPLE_COUNT;
    mode_index = sac_fallback_add_mode(&sac_fallback_instance, "96kHz 24-bit", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&sac_fallback_instance, mode_index, main_channel_sample_accumulator_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&sac_fallback_instance, mode_index, main_channel_downsampling_discard_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#if !USB_AUDIO_ENABLED
    /* When using I2S, packing is required to convert 24-bit audio aligned on 32-bit words. */
    sac_fallback_mode_assign_process(&sac_fallback_instance, mode_index, sac_packing_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif

    /* Fallback mode 1 configuration: 48kHz 24-bit. */
    mode_cfg = sac_fallback_mode_get_defaults();
    mode_cfg.cca_bad_fail_count_threshold_perc = 5;
    mode_cfg.cca_bad_time_sec = 0.1;
    mode_cfg.consumer_buffer_load_threshold_tenths = 40;
    mode_cfg.cca_good_fail_count_threshold_perc = 5;
    mode_cfg.cca_good_time_sec = 30;
    mode_cfg.link_margin_threshold = 60;
    mode_cfg.link_margin_good_time_sec = 5;
    mode_cfg.sample_count = MAIN_CHANNEL_FBK_1_SAMPLE_COUNT;
    mode_index = sac_fallback_add_mode(&sac_fallback_instance, "48kHz 24-bit", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&sac_fallback_instance, mode_index, main_channel_sample_accumulator_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&sac_fallback_instance, mode_index, main_channel_downsampling_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#if !USB_AUDIO_ENABLED
    /* When using I2S, packing is required to convert 24-bit audio aligned on 32-bit words. */
    sac_fallback_mode_assign_process(&sac_fallback_instance, mode_index, sac_packing_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif

    /* Fallback mode 2 configuration: 48kHz 16-bit. */
    mode_cfg = sac_fallback_mode_get_defaults();
    mode_cfg.cca_bad_fail_count_threshold_perc = 60;
    mode_cfg.cca_bad_time_sec = 0.1;
    mode_cfg.consumer_buffer_load_threshold_tenths = 48;
    mode_cfg.cca_good_fail_count_threshold_perc = 60;
    mode_cfg.cca_good_time_sec = 30;
    mode_cfg.link_margin_threshold = 40;
    mode_cfg.link_margin_good_time_sec = 4;
    mode_cfg.sample_count = MAIN_CHANNEL_FBK_2_SAMPLE_COUNT;
    mode_index = sac_fallback_add_mode(&sac_fallback_instance, "48kHz 16-bit", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&sac_fallback_instance, mode_index, main_channel_sample_accumulator_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&sac_fallback_instance, mode_index, main_channel_downsampling_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&sac_fallback_instance, mode_index, main_channel_fbk_packing_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&sac_fallback_instance, mode_index, main_channel_compression_discard_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Fallback mode 3 configuration: 48kHz ADPCM. */
    mode_cfg = sac_fallback_mode_get_defaults();
    mode_cfg.cca_good_fail_count_threshold_perc = 60;
    mode_cfg.cca_good_time_sec = 10;
    mode_cfg.link_margin_threshold = 40;
    mode_cfg.link_margin_good_time_sec = 2;
    mode_cfg.sample_count = MAIN_CHANNEL_FBK_3_SAMPLE_COUNT;
    mode_index = sac_fallback_add_mode(&sac_fallback_instance, "48kHz ADPCM", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&sac_fallback_instance, mode_index, main_channel_sample_accumulator_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&sac_fallback_instance, mode_index, main_channel_downsampling_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&sac_fallback_instance, mode_index, main_channel_compression_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Fallback mode 4 configuration: 24kHz ADPCM.
     *
     * Same processing chain as mode 3 with the 1:4 resampler in place of the 1:2, which halves
     * what goes on the air: 23 samples/channel instead of 46, so 31 B instead of 54 B.
     *
     * What this rung buys is bitrate -- 192 kbps against 384 -- which is a range argument. It does
     * NOT buy retransmission headroom: slots are spent per packet, not per byte, so a smaller
     * payload does not change how many attempts a packet gets. Only the accumulator does that,
     * and this rung shares mode 3's ratio.
     *
     * Entry thresholds are mode 3's, loosened one step: by the time the ladder is here the link
     * has already failed everything above, so the question is whether 24 kHz holds, not whether to
     * be cautious about arriving. Recovery is deliberately slower than mode 3's -- 15 s of good
     * CCA against 10 -- because the 3<->4 boundary swaps resamplers and each crossing costs a
     * cold-FIR pop, so oscillating across it is worse than sitting on the lower rung a while. */
    mode_cfg = sac_fallback_mode_get_defaults();
    mode_cfg.cca_good_fail_count_threshold_perc = 60;
    mode_cfg.cca_good_time_sec = 15;
    mode_cfg.link_margin_threshold = 30;
    mode_cfg.link_margin_good_time_sec = 3;
    mode_cfg.sample_count = MAIN_CHANNEL_FBK_4_SAMPLE_COUNT;
    mode_index = sac_fallback_add_mode(&sac_fallback_instance, "24kHz ADPCM", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&sac_fallback_instance, mode_index, main_channel_sample_accumulator_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&sac_fallback_instance, mode_index, main_channel_downsampling4_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&sac_fallback_instance, mode_index, main_channel_compression_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /** Start fallback at the highest ACTIVE mode.
     *
     *  sac_fallback_add_mode sets current_mode to the last added mode, so an explicit set is
     *  required either way.
     */
#if MAIN_CHANNEL_ALLOW_96K
    sac_fallback_set_current_mode(&sac_fallback_instance, 0, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#else
    /* Ceiling capped at mode 1 (48 kHz 24-bit): deactivate mode 0 while leaving automatic
     * fallback enabled. Downward degradation still works -- trigger_next_mode() drops to mode
     * 2/3/4 on a bad link -- and recover_to_previous_mode() skips the inactive mode 0, so the
     * ladder degrades and recovers up to 48 kHz and never climbs to 96 kHz.
     *
     * Only the coordinator needs this. The node's main channel is RX and follows the
     * transmitted header, so capping this side caps the whole main-channel path.
     *
     * Ported from puretone_dongle.c, where the same cap has been the shipping configuration
     * since d531b16 -- the one handed to the ODM as puretone_gen2_231_rc04. Its history is
     * worth knowing before anyone removes this again: the cap was added, removed on the
     * expectation that the SDK v2.3.1 TDMA re-sync fix had cured the dual-radio park, and
     * then re-applied when hardware showed it had not. The vendor fix addresses unsynced RX
     * overrun, which is a different failure.
     *
     * The failure this prevents is specific and recognisable: at mode 0 on a dual-radio
     * build, TIM4's ARR wedges at 0xFFFD, the radio IRQ and DMA counters freeze, swc goes to
     * STOP, and stall_auto_recover() cannot revive it -- see MD/radio_stall_wedge_open_issue.md,
     * which recorded that signature before it was connected to this cap. A unidirectional
     * node is affected more directly than the headset ever was, because this ladder STARTS at
     * mode 0: a dual-radio node boots straight into the parking condition.
     *
     * Re-enable with -DMAIN_CHANNEL_ALLOW_96K=1 once the park is fixed, and only then. */
    sac_fallback_mode_set_active_state(&sac_fallback_instance, 0, false, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_set_current_mode(&sac_fallback_instance, 1, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif

#if FALLBACK_FORCE_MODE >= 0
    /* Overrides the ceiling above. The requested rung is activated whatever it is -- forcing a
     * mode that cannot be reached would be a silent no-op, which is worse than the override --
     * then selected, then the module is put in manual mode so nothing moves it again.
     *
     * The #error in sac_cfg.h is what stops this re-enabling mode 0 by accident. */
    sac_fallback_mode_set_active_state(&sac_fallback_instance, FALLBACK_FORCE_MODE, true, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_set_current_mode(&sac_fallback_instance, FALLBACK_FORCE_MODE, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_set_manual_mode(&sac_fallback_instance, true, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif

#if SINE_INJECT_DG
    /* Pin whatever mode the two branches above just selected -- mode 1 (48 kHz 24-bit) in the
     * shipping configuration, mode 0 if someone built with MAIN_CHANNEL_ALLOW_96K.
     *
     * Automatic fallback has to stop for a pure-tone test. Left running, a weak link drops the
     * ladder to mode 2/3/4, whose resampling and ADPCM introduce phase discontinuities at buffer
     * boundaries; on a tone that is plainly audible as noise, and it gets misread as a fault in
     * the audio path rather than the ladder doing its job. Manual mode stops automatic changes
     * and the node follows the mode carried in the transmitted header.
     *
     * Note this is one rung below what puretone_dongle.c pins for the same test. It pinned mode
     * 0, which is uncompressed and not resampled; mode 1 is uncompressed but IS resampled
     * 96k->48k and back, so a small amount of resampler artefact is expected here and is not a
     * fault. Mode 0 is unavailable by default on this line because 96 kHz parks a dual-radio
     * node -- for the cleanest possible tone on a single-radio pair, build with
     * -DMAIN_CHANNEL_ALLOW_96K=1 and this pins mode 0 instead. */
    sac_fallback_set_manual_mode(&sac_fallback_instance, true, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif
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

/** @brief Initialize the audio downsampling processing stage interface.
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

/** @brief Initialize the audio compression discard processing stage interface.
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

/** @brief Update the fallback LED indicator.
 */
/** @brief Freeze the ladder while the peer is away, and pin it when it steps down to the floor.
 *
 *  FREEZE. The ladder's trigger is queue depth -- is_link_bad() is queue-size-high OR
 *  cca-bad -- so a peer that is not there backs the transmit queue up and reads as a bad
 *  link. It steps down, the queue stays exactly as full, and it steps again until it reaches
 *  the bottom. Every one of those steps is a judgement about a link with nobody on the other
 *  end. While link_is_up() says the peer is gone, the ladder holds where it is.
 *
 *  SETTLE. The same queue argument applies for a while after the peer comes BACK, and that
 *  is not obvious: the peer is reachable, so the freeze has lifted, but the transmit queue is
 *  still full of warm-up rather than evidence. Left alone the ladder reads that as a bad link
 *  and walks 1 to 4 in one go -- which is what a coordinator restart did, with the node
 *  present throughout, so nothing about it looked like an absent peer. The ladder stays held
 *  until the link has run for LADDER_SETTLE_MS.
 *
 *  PIN, and this is the part that has to be exact. Pinning happens on the TRANSITION -- a
 *  step DOWN into the bottom rung -- not on being at the bottom rung. Those are different
 *  conditions and the difference is the whole behaviour:
 *
 *    - "at the bottom" pins the moment a freeze thaws with the ladder already down there,
 *      which is precisely the peer-reboot path this is supposed to survive. That was the
 *      first version, and it locked at 24 kHz constantly.
 *    - "stepped down into the bottom" only fires when the ladder was somewhere higher and
 *      the link pushed it the rest of the way, which is the situation worth latching.
 *
 *  A step down is prev < bottom && current == bottom rather than prev == bottom - 1, so a
 *  descent that crosses two rungs between two polls still counts. Missing it would leave the
 *  ladder unpinned, which is the safe direction to be wrong in.
 *
 *  The previous mode is recorded on every pass INCLUDING frozen ones. That is what makes a
 *  thaw not look like a step: if the ladder reached the bottom while the peer was away, prev
 *  is already the bottom by the time it comes back.
 *
 *  A pin does not survive the peer restarting. It is a statement about one link -- that it
 *  could not hold a higher rung -- and a peer that has been silent for
 *  NODE_RESTART_SILENCE_MS is not that link any more. An obstruction, an order of magnitude
 *  shorter, is the same link and keeps its pin.
 *
 *  A manual selection from the button beats all of it. fallback_state leaves FALLBACK_AUTO
 *  only when a person has chosen a rung, and overriding that would make the button
 *  unreliable in exactly the situation someone reaches for it.
 */
static void fallback_hold_handler(void)
{
#if FALLBACK_FORCE_MODE >= 0
    /* The rung is nailed down for the whole run, so there is no ladder to hold -- and
     * the thaw path below would hand it straight back by calling set_manual_mode()
     * with s_ladder_pinned, which is false. Compile the whole thing out instead. */
#else
    sac_status_t sac_status = SAC_OK;
    /* Five modes are added to this instance and the last index is the bottom rung. Derived
     * from the state enum so that adding a rung does not leave this behind. */
    const uint8_t bottom_mode = (uint8_t)(FALLBACK_STATE_COUNT - 2);
    uint8_t mode;
    bool peer_present;

    /* A person has taken the ladder; leave it alone. */
    if (fallback_state != FALLBACK_AUTO) {
        return;
    }

    mode = sac_fallback_get_current_mode(&sac_fallback_instance, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    peer_present = link_is_up();

    if (!peer_present) {
        /* Whatever comes back has to earn the ladder again from scratch. */
        s_link_up_valid = false;
        s_link_settled = false;

        if (!s_ladder_frozen) {
            sac_fallback_set_manual_mode(&sac_fallback_instance, true, &sac_status);
            ASSERT_SAC_STATUS(sac_status);
            s_ladder_frozen = true;
        }

        /* Gone long enough to be a restart rather than something in the way: drop the pin,
         * so whatever comes back is judged on its own link. */
        if (s_ladder_pinned && s_node_rx_seen &&
            ((facade_get_tick_ms() - s_node_rx_tick) >= NODE_RESTART_SILENCE_MS)) {
            s_ladder_pinned = false;
            facade_print_string("[FB] peer restarted; pin released\r\n");
        }

        /* Still tracked while frozen -- see the note about thaws above. */
        s_ladder_prev_mode = mode;
        s_ladder_prev_valid = true;
        return;
    }

    /* Peer is reachable, but a link that has only just come up has a transmit queue full of
     * warm-up rather than evidence. Keep holding until it has run long enough to mean
     * something -- and keep recording the mode while holding, so a descent that happened
     * during the hold is the baseline afterwards rather than a step down to be latched. */
    if (!s_link_settled) {
        if (!s_link_up_valid) {
            s_link_up_tick = facade_get_tick_ms();
            s_link_up_valid = true;
        }

        if ((facade_get_tick_ms() - s_link_up_tick) < LADDER_SETTLE_MS) {
            if (!s_ladder_frozen) {
                sac_fallback_set_manual_mode(&sac_fallback_instance, true, &sac_status);
                ASSERT_SAC_STATUS(sac_status);
                s_ladder_frozen = true;
            }

            s_ladder_prev_mode = mode;
            s_ladder_prev_valid = true;
            return;
        }

        s_link_settled = true;
    }

    if (s_ladder_frozen) {
        /* Release. If the pin was dropped while the peer was away, the ladder is free again;
         * if it was not, this leaves it held. */
        sac_fallback_set_manual_mode(&sac_fallback_instance, s_ladder_pinned, &sac_status);
        ASSERT_SAC_STATUS(sac_status);
        s_ladder_frozen = false;
    }

#if FALLBACK_PIN_AT_BOTTOM
    if (!s_ladder_pinned && s_ladder_prev_valid && (s_ladder_prev_mode < bottom_mode) &&
        (mode >= bottom_mode)) {
        sac_fallback_set_manual_mode(&sac_fallback_instance, true, &sac_status);
        ASSERT_SAC_STATUS(sac_status);
        s_ladder_pinned = true;
        facade_print_string("[FB] stepped down to the bottom rung; pinned\r\n");
    }
#else
    (void)bottom_mode;
#endif

    s_ladder_prev_mode = mode;
    s_ladder_prev_valid = true;
#endif
}

/** @brief Audio peripheral receive complete callback.
 *
 *  @note This receives audio packets from the codec. It needs to be executed every time a DMA transfer from the codec
 *        is completed in order to keep recording audio.
 */
static void audio_rx_complete_callback(void)
{
    sac_status_t sac_status = SAC_OK;

#if SINE_INJECT_DG
    /* External I2S clocks the DMA and so still supplies the 96 kHz timing; the buffer it just
     * filled is overwritten with sine before it enters the pipeline. Same path, same format and
     * same cadence as real audio -- only the samples differ. */
    sac_facade_i2s_inject_sine();
#endif

    /* The codec produces audio samples when it receives input audio. */
    sac_pipeline_produce(sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Trigger audio process. */
    facade_audio_process_timer_trigger();
}

/** @brief Callback handling the audio process that triggers with the app timer.
 */
static void audio_process_callback(void)
{
    sac_status_t sac_status = SAC_OK;
    uint32_t buffer_load = 0;

    buffer_load = sac_pipeline_get_producer_buffer_load(sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    while (buffer_load > 0) {
        /* Processing stages of the pipeline are executed. */
        sac_pipeline_process(sac_pipeline, &sac_status);
        ASSERT_SAC_STATUS(sac_status);
        buffer_load--;
        /* The SWC consumes audio samples produced by the codec. */
        sac_pipeline_consume(sac_pipeline, &sac_status);
        ASSERT_SAC_STATUS(sac_status);
    }

    buffer_load = sac_pipeline_get_consumer_buffer_load(sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    if (buffer_load > 0) {
        /* The SWC consumes audio samples produced by the codec. */
        sac_pipeline_consume(sac_pipeline, &sac_status);
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

/** @brief Print the audio and wireless statistics.
 */
static void print_stats(void)
{
    if (device_pairing_state != DEVICE_PAIRED) {
        return;
    }

#if !STATS_VERBOSE
    print_stats_compact();
#else

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

    /* ** Audio statistics ** */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, audio_stats_str);
    /* What the pool actually cost, so SAC_MEM_POOL_SIZE can be set from a measurement. Allocation
     * happens once at init and never grows, so this figure is final by the first print. */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length,
                              "Mem Pool: %lu/%u bytes\r\n",
                              (unsigned long)sac_get_allocated_bytes(&sac_status), (unsigned)SAC_MEM_POOL_SIZE);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_update_stats(sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    string_length += sac_pipeline_format_stats(sac_pipeline, stats_string + string_length,
                                               sizeof(stats_string) - string_length, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* ** Audio fallback statistics ** */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, fallback_stats_str);
    string_length += sac_fallback_format_stats(&sac_fallback_instance, stats_string + string_length,
                                               sizeof(stats_string) - string_length, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* ** Wireless statistics ** */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, wireless_stats_str);
    swc_connection_t *connections[] = {tx_audio_conn, tx_data_conn, rx_data_conn};

    for (uint8_t i = 0; i < ARRAY_SIZE(connections); i++) {
        swc_connection_update_stats(connections[i], &swc_err);
        ASSERT_SWC_STATUS(swc_err);
        string_length += swc_connection_format_stats(connections[i], stats_string + string_length,
                                                     sizeof(stats_string) - string_length, &swc_err);
        ASSERT_SWC_STATUS(swc_err);
    }

    facade_print_string(stats_string);

    /* ** APP Statistics ** */
    string_length = snprintf(stats_string, sizeof(stats_string), "\r\n<< Application Statistics >>\r\n");
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, "Fallback State:");
    if (fallback_state == FALLBACK_AUTO) {
        snprintf(stats_string + string_length, sizeof(stats_string) - string_length, " AUTO\r\n");
    } else if (fallback_state == FALLBACK_96K_24BIT_UNCOMPRESSED) {
        snprintf(stats_string + string_length, sizeof(stats_string) - string_length, " 96kHz 24-bit\r\n");
    } else if (fallback_state == FALLBACK_48K_24BIT_UNCOMPRESSED) {
        snprintf(stats_string + string_length, sizeof(stats_string) - string_length, " 48kHz 24-bit\r\n");
    } else if (fallback_state == FALLBACK_48K_16BIT_UNCOMPRESSED) {
        snprintf(stats_string + string_length, sizeof(stats_string) - string_length, " 48kHz 16-bit\r\n");
    } else if (fallback_state == FALLBACK_48K_ADPCM_STEREO) {
        snprintf(stats_string + string_length, sizeof(stats_string) - string_length, " 48kHz ADPCM\r\n");
    } else if (fallback_state == FALLBACK_24K_ADPCM_STEREO) {
        snprintf(stats_string + string_length, sizeof(stats_string) - string_length, " 24kHz ADPCM\r\n");
    } else {
        /* Every enumerator above supplies its own newline, so a state with no branch here does not
         * merely lose its name -- it runs the next section onto this line. */
        snprintf(stats_string + string_length, sizeof(stats_string) - string_length, " (unnamed state %u)\r\n",
                 (unsigned)fallback_state);
    }

    facade_print_string(stats_string);
#endif /* !STATS_VERBOSE */

    print_diagnostics();
}

#if !STATS_VERBOSE
/** @brief Map a fallback mode index to the rung's name. */
static const char *fallback_mode_name(uint8_t mode)
{
    static const char *const names[] = {
        "96kHz 24-bit", "48kHz 24-bit", "48kHz 16-bit", "48kHz ADPCM", "24kHz ADPCM",
    };

    return (mode < ARRAY_SIZE(names)) ? names[mode] : "?";
}

/** @brief One line a second: which rung, and the counter that belongs to this role.
 *
 *  The coordinator transmits, so clear-channel assessment is its business. cca_fail counts
 *  packets this side never put on air because the channel never looked clear -- the connection's
 *  fail action is SWC_CCA_ABORT_TX, so a failed assessment loses the packet rather than delaying
 *  it.
 *
 *  Read it against the node's rej/s, which counts packets that were sent, arrived, and could not
 *  be decoded. The two failures need different fixes and are indistinguishable from the listening
 *  position: cca_fail is answered by more CCA tries, rej by ISI mitigation, and neither helps the
 *  other.
 *
 *  Rates rather than totals, because a dropout lasts tens of milliseconds and a free-running
 *  counter cannot show one without differencing two lines by eye.
 */
static void print_stats_compact(void)
{
    static uint32_t prev_cca_fail, prev_tx_drop, prev_tx, prev_idle, prev_dtx, prev_dack, prev_tick;
    static uint32_t prev_dslot, prev_dcca, prev_dfull;
    static bool prev_valid;

    char line[240];
    swc_error_t swc_err = SWC_ERR_NONE;
    sac_status_t sac_status = SAC_OK;
    uint32_t now = facade_get_tick_ms();
    swc_fallback_info_t info = swc_connection_get_fallback_info(tx_audio_conn, &swc_err);
    swc_statistics_t *tx = swc_connection_update_stats(tx_audio_conn, &swc_err);
    uint8_t fb_mode = sac_fallback_get_current_mode(&sac_fallback_instance, &sac_status);
    /* Everything that actually went on the air, whether or not this connection uses acks --
     * summing both is what makes the number independent of that setting. */
    uint32_t tx_sent = (tx != NULL) ? (tx->packet_sent_and_acked_count + tx->packet_sent_and_not_acked_count) : 0;
    /* Scheduled transmit slots that carried nothing, because there was nothing to send. This
     * is the coordinator's mirror of the node's miss/s, and the pair of them is what tells
     * "the air was empty because nothing was produced" apart from "packets were sent and did
     * not arrive". Those two look identical from either end alone. */
    uint32_t tx_idle = (tx != NULL) ? tx->no_packet_tranmission_count : 0;
    /* The data connection, which shares the audio connection's slots at the lower priority.
     * Nominal is 100/s, one per DATA_TX_PERIOD_MS, and dack is how many of those the node
     * acknowledged. It is also the node's only heartbeat, so dtx falling while idle= reads 0
     * is audio holding every slot and starving it -- the candidate for a node that reports
     * LE_UWB_DISCONNECTED while audio is still flowing; see dgap in the node's line.
     *
     * Measured 2026-09-29 on a u5a5 pair: that is NOT what happens. dtx went to 0 for ~5 s
     * around a walk-back disconnect, and for three of those seconds audio had 2000-3000 idle
     * slots/s and was barely retrying -- the data connection stopped on its own. The three
     * counters after dack say why it stopped, and the Wireless Core is prebuilt, so they are
     * the only view in:
     *   dslot  TX timeslots the core gave the data connection. 0 = the scheduler stopped
     *          offering it slots at all.
     *   dcca   packets the core abandoned on a failed clear-channel assessment. This
     *          connection's fail action is SWC_CCA_ABORT_TX with 15 tries, so a channel that
     *          reads busy costs the packet without it ever reaching the air -- and cca_fail
     *          earlier in the line counts the AUDIO connection only.
     *   dfull  data_callback() found the queue (SWC_QUEUE_SIZE) full and dropped the packet,
     *          i.e. the core was holding packets it was not sending. */
    swc_statistics_t *dt = swc_connection_update_stats(tx_data_conn, &swc_err);
    uint32_t dtx_sent = (dt != NULL) ? (dt->packet_sent_and_acked_count + dt->packet_sent_and_not_acked_count) : 0;
    uint32_t dtx_acked = (dt != NULL) ? dt->packet_sent_and_acked_count : 0;
    uint32_t dslot = (dt != NULL) ? dt->tx_timeslot_occurrence : 0;
    uint32_t dcca = (dt != NULL) ? dt->cca_fail_count : 0;
    uint32_t dfull = s_data_queue_full_count;
    uint32_t dslot_rate = 0;
    uint32_t dcca_rate = 0;
    uint32_t dfull_rate = 0;
    uint32_t cca_rate = 0;
    uint32_t drop_rate = 0;
    uint32_t tx_rate = 0;
    uint32_t idle_rate = 0;
    uint32_t dtx_rate = 0;
    uint32_t dack_rate = 0;

    if (prev_valid) {
        uint32_t dms = now - prev_tick;

        if (dms > 0) {
            cca_rate = (uint32_t)(((uint64_t)(info.cca_fail_count - prev_cca_fail) * 1000U) / dms);
            drop_rate = (uint32_t)(((uint64_t)(info.tx_pkt_dropped - prev_tx_drop) * 1000U) / dms);
            tx_rate = (uint32_t)(((uint64_t)(tx_sent - prev_tx) * 1000U) / dms);
            idle_rate = (uint32_t)(((uint64_t)(tx_idle - prev_idle) * 1000U) / dms);
            dtx_rate = (uint32_t)(((uint64_t)(dtx_sent - prev_dtx) * 1000U) / dms);
            dack_rate = (uint32_t)(((uint64_t)(dtx_acked - prev_dack) * 1000U) / dms);
            dslot_rate = (uint32_t)(((uint64_t)(dslot - prev_dslot) * 1000U) / dms);
            dcca_rate = (uint32_t)(((uint64_t)(dcca - prev_dcca) * 1000U) / dms);
            dfull_rate = (uint32_t)(((uint64_t)(dfull - prev_dfull) * 1000U) / dms);
        }
    }
    prev_dslot = dslot;
    prev_dcca = dcca;
    prev_dfull = dfull;
    prev_cca_fail = info.cca_fail_count;
    prev_tx_drop = info.tx_pkt_dropped;
    prev_tx = tx_sent;
    prev_idle = tx_idle;
    prev_dtx = dtx_sent;
    prev_dack = dtx_acked;
    prev_tick = now;
    prev_valid = true;

    /* dtx/dack go last so that anything already splitting this line on its first fields reads
     * them where it always has. */
    snprintf(line, sizeof(line),
             "[DG] " FW_VERSION_COMPACT " %lu fb=%u %-13s tx=%lu/s idle=%lu/s cca_fail=%lu/s tx_drop=%lu/s"
             " dtx=%lu/s dack=%lu/s dslot=%lu/s dcca=%lu/s dfull=%lu/s\r\n",
             (unsigned long)now, (unsigned)fb_mode, fallback_mode_name(fb_mode), (unsigned long)tx_rate,
             (unsigned long)idle_rate, (unsigned long)cca_rate, (unsigned long)drop_rate, (unsigned long)dtx_rate,
             (unsigned long)dack_rate, (unsigned long)dslot_rate, (unsigned long)dcca_rate,
             (unsigned long)dfull_rate);
    facade_print_string(line);
}
#endif /* !STATS_VERBOSE */


/** @brief Print the liveness counters, and the HardFault snapshot if there is one.
 *
 *  Liveness is read as a delta across two prints: the radio interrupt and DMA counters tick
 *  thousands of times a second in normal operation, even out of range, so all of them holding
 *  still while the log keeps printing means the radios stopped being serviced rather than the
 *  link being bad. mrt is the wireless core's scheduler tick and separates the two cases -- it
 *  frozen means the scheduler died, it moving beside frozen radio counters means the scheduler
 *  is alive and the radios are not.
 *
 *  The fault line is printed only when there has been a fault, because all-zero is the normal
 *  reading and this block is verbose enough already. Its absence is not proof of health, mind:
 *  a board stuck in a while(1) faults nothing and prints nothing here. What it does prove, when
 *  present, is that the failure was a fault and not a hang -- which is the first fork in
 *  diagnosing one, and is otherwise indistinguishable from the outside.
 */
static void print_diagnostics(void)
{
    char line[160];
    uint32_t r1_irq = 0, r2_irq = 0, r1_dma = 0, r2_dma = 0;
    uint32_t mrt = 0, frt_unused = 0;
    bool irq1 = false, irq2 = false;

    (void)facade_get_radio_hw_counters(&r1_irq, &r2_irq, &r1_dma, &r2_dma);
    /* frt is discarded: facade_get_sched_liveness fills it from the same counter
     * facade_get_tick_ms() returns, so printing it would only repeat a number already available. */
    (void)facade_get_sched_liveness(&mrt, &frt_unused, &irq1, &irq2);

#if STATS_VERBOSE
    snprintf(line, sizeof(line), "Liveness: irq=%lu/%lu dma=%lu/%lu mrt=%lu\r\n", (unsigned long)r1_irq,
             (unsigned long)r2_irq, (unsigned long)r1_dma, (unsigned long)r2_dma, (unsigned long)mrt);
    facade_print_string(line);
#endif


    uint32_t cfsr = 0, hfsr = 0, pc = 0, lr = 0;

    if (facade_get_hardfault_snapshot(&cfsr, &hfsr, &pc, &lr) && ((cfsr | hfsr | pc | lr) != 0)) {
        snprintf(line, sizeof(line), "Fault: cfsr=0x%08lX hfsr=0x%08lX pc=0x%08lX lr=0x%08lX\r\n",
                 (unsigned long)cfsr, (unsigned long)hfsr, (unsigned long)pc, (unsigned long)lr);
        facade_print_string(line);
    }
}

/** @brief Callback sends the button state at the DATA_TX_PERIOD_MS interval.
 */
static void data_callback(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    user_data_t transmitted_user_data = {0};

    /* Send the state of the button to the Node (The Link margin is not used). */
    transmitted_user_data.button_state = facade_read_button_state();

    /* Media key, if one is waiting. Cleared as it is packed: it is an edge, and the sender
     * has no way to learn whether this packet arrived. */
    transmitted_user_data.cmd_type = s_pending_cmd;
    s_pending_cmd = 0;

    user_data_pack_vendor(&transmitted_user_data);

    /* user_data_tx_size(), not sizeof(). The struct now reserves room for the vendor block,
     * which is empty in almost every packet; sending the whole thing would put that reserved
     * space on the air a hundred times a second to carry nothing. See the header. */
    wireless_send_data(&transmitted_user_data, user_data_tx_size(&transmitted_user_data), &swc_err);
}

/** @brief Handle pairing button callback.
 */
static void pairing_button_callback(void)
{
    /* Called from inside the boot-reconnect polling loop: defer, so the connection handles
     * stay valid until the loop has unwound. The loop tears down and the caller enters
     * pairing, which is what the press meant anyway. */
    if (s_boot_reconnect_active) {
        s_boot_reconnect_abort = true;
        return;
    }

    switch (device_pairing_state) {
    case DEVICE_PAIRED:
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

    facade_notify_enter_pairing();
    at_cmd_core_notify_pairing_started();

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
        at_cmd_core_notify_pairing_result(true);

        /* Persist before connecting, so a power cut between the two does not lose a pairing
         * the user has already been told succeeded. A record whose link never came up is
         * harmless: the next boot simply reconnects to it. */
        (void)reconnect_store_save(&pairing_assigned_address);

        app_init();
        device_pairing_state = DEVICE_PAIRED;

        break;
    case PAIRING_EVENT_TIMEOUT:
    case PAIRING_EVENT_INVALID_APP_CODE:
    case PAIRING_EVENT_ABORT:
    default:
        /* Indicate that the pairing process was unsuccessful. */
        facade_notify_not_paired();

        /* Every unsuccessful outcome reports the same way -- timeout, invalid app code and
         * abort all reach here. A host that asked for pairing needs to learn that it ended
         * far more than it needs to learn which of the three ended it, and silence is the one
         * answer it cannot act on. */
        at_cmd_core_notify_pairing_result(false);
        device_pairing_state = DEVICE_UNPAIRED;
        break;
    }
}

/** @brief Has the node been heard from recently?
 *
 *  Measured directly rather than asked of the Wireless Core, and the reason is specific to
 *  this role: link_update_connect_status() hands a COORDINATOR synced == true
 *  unconditionally, so a coordinator's connection status can only fall through accumulated
 *  frame outcomes. On the sibling application that path was observed not to fire at all --
 *  the coordinator went on reporting CONNECTED with the node powered off. Asking a transmit
 *  connection is worse still: this side is the timebase master and keeps transmitting into
 *  its own timeslots whether or not anything is listening.
 *
 *  The node's data_callback() sends a packet every DATA_TX_PERIOD_MS unconditionally, so an
 *  arriving packet is the peer's heartbeat and the RX callback stamps it.
 *
 *  s_node_rx_seen guards the boot case. Without it a tick count still below the timeout
 *  reads as "heard recently" for the first NODE_RX_TIMEOUT_MS after reset, and
 *  try_boot_reconnect() polls this -- so it would declare success against a node that was
 *  never powered on, which is the failure it exists to avoid.
 *
 *  @return true if a packet arrived within NODE_RX_TIMEOUT_MS.
 */
static bool link_is_up(void)
{
    if (!s_node_rx_seen) {
        return false;
    }

    return (facade_get_tick_ms() - s_node_rx_tick) < NODE_RX_TIMEOUT_MS;
}

/** @brief Re-establish a persisted pairing without running the pairing procedure.
 *
 *  Restores the discovery list from flash, brings the wireless core up on those addresses,
 *  and polls for the node for up to RECONNECT_TIMEOUT_MS. Buttons keep being serviced
 *  throughout; a press defers through s_boot_reconnect_abort so the loop can unwind before
 *  anything releases the handles it is reading.
 *
 *  @return BOOT_RECONNECT_OK    link re-established, paired and streaming;
 *          BOOT_RECONNECT_PAIR  no usable record, or the user aborted -- caller pairs;
 *          BOOT_RECONNECT_IDLE  a record existed but the node was not up in time. The core
 *                               is LEFT RUNNING; do not tear down and do not re-pair.
 */
static boot_reconnect_result_t try_boot_reconnect(void)
{
    uint32_t start;
    bool connected = false;

    /* Blank, corrupt or wrong-version flash reads as no record, which is what a factory
     * device looks like, so it falls through to pairing with no special case. */
    if (!reconnect_store_load(&pairing_assigned_address)) {
        return BOOT_RECONNECT_PAIR;
    }

    /* A valid record should never carry a zero node address. Guard anyway and treat it as
     * no record rather than building a core around it. */
    if (pairing_assigned_address.node_address == 0) {
        return BOOT_RECONNECT_PAIR;
    }

    /* app_swc_core_init() reads both addresses out of the discovery list, so rebuild it from
     * the persisted pair. */
    pairing_discovery_list[PAIRING_DEVICE_ROLE_COORDINATOR].node_address =
        pairing_assigned_address.coordinator_address;
    pairing_discovery_list[PAIRING_DEVICE_ROLE_NODE].node_address =
        pairing_assigned_address.node_address;

    /* Say so on the LED as well as the console. The console is the bench's instrument; the
     * LED is what someone standing in front of the product can read, and on a board with no
     * display it is the only thing separating "restoring your pairing" from "pairing now".
     */
    facade_notify_reconnecting();
    facade_print_string("[BOOT] reconnecting to stored pair\r\n");

    app_init();
    device_pairing_state = DEVICE_PAIRED;

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
    }

    s_boot_reconnect_active = false;

    if (connected) {
        facade_notify_pairing_successful();
        facade_print_string("[BOOT] reconnected\r\n");
        return BOOT_RECONNECT_OK;
    }

    if (s_boot_reconnect_abort) {
        /* The user asked to pair while this was running. Dismantle what was built -- that
         * also returns device_pairing_state to UNPAIRED -- and let the caller pair. The
         * flash record is deliberately left intact; the user has not said to forget the
         * peer, only that they want to pair now. */
        unpair_device(false);
        return BOOT_RECONNECT_PAIR;
    }

    /* Plain timeout with the core still up: the node just is not on yet. This side is mains
     * powered and IS the timebase master, so leave the wireless core running -- it keeps
     * transmitting the schedule and the node syncs whenever it boots. Do not tear down and
     * do not re-pair. */
    facade_print_string("[BOOT] stored pair did not answer; core left running\r\n");
    return BOOT_RECONNECT_IDLE;
}

/** @brief Unpair the device. This will reset its discovery list.
 *
 *  @param[in] forget_peer  true to also erase the persisted pairing address, so the next boot
 *                          pairs instead of reconnecting. false tears the link down but KEEPS
 *                          the record, which is what an aborted reconnect wants: the user
 *                          asked to pair now, not to forget who they were paired with, and
 *                          those are different instructions.
 */
static void unpair_device(bool forget_peer)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    sac_status_t sac_status = SAC_OK;

    device_pairing_state = DEVICE_UNPAIRED;

    /* Stop timers. */
    facade_audio_process_timer_stop();
    facade_data_timer_stop();

    /* Disconnect the Wireless Core. */
    swc_disconnect(&swc_err);
    ASSERT_SWC_STATUS(swc_err);

    tx_audio_conn = NULL;
    tx_data_conn = NULL;
    rx_data_conn = NULL;

    /* Reset the pairing discovery list. */
    memset(pairing_discovery_list, 0, sizeof(pairing_discovery_list));

    /* The heartbeat belongs to the connections that just went away, and so do both ladder
     * holds -- a pin earned by one link says nothing about the next one. */
    s_node_rx_seen = false;
    s_ladder_frozen = false;
    s_ladder_pinned = false;
    s_ladder_prev_valid = false;
    s_link_up_valid = false;
    s_link_settled = false;

    /* Forget the peer only when asked. The press that removes a device should not leave a
     * record behind; an aborted reconnect should not throw one away. */
    if (forget_peer) {
        (void)reconnect_store_clear();
        /* Only when the record is actually erased. Tearing the link down and forgetting who
         * the peer was call for opposite responses from a host -- one is reconnectable, the
         * other needs pairing -- so reporting the first as the second would send it looking
         * for a person to press a button that nothing was waiting for. */
        at_cmd_core_notify_unpaired();
    }

    /* Stop the audio pipeline. */
    sac_pipeline_stop(sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline = NULL;

    facade_audio_deinit();

    /* Indicate that the device is unpaired. */
    facade_led_all_off();
    facade_notify_not_paired();
}

/** @brief Pairing process callback called during pairing.
 */
static void pairing_process_callback(void)
{
    /*
     * Note: The button press will only be detected when the pairing executes the registered pairing process callback,
     *       which might take a variable amount of time.
     */
    facade_button_handling();

    /* And the AT channel, for the same reason. pairing_coordinator_start() does not return
     * until the procedure succeeds, times out or is aborted, so this callback is the only
     * thing running for the whole pairing window -- without it the module stops answering AT
     * for PAIRING_TIMEOUT_IN_SECONDS, which is precisely when a host has just sent
     * AT+LE_UWB_PAIR and is waiting to hear how it went. */
    at_cmd_core_process();
}

/** @brief Abort the pairing procedure.
 */
static void abort_pairing_procedure(void)
{
    pairing_abort();
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

    sac_fallback_set_manual_mode(&sac_fallback_instance, (fallback_state > FALLBACK_AUTO), &sac_status);

    if (fallback_state > FALLBACK_AUTO) {
        sac_fallback_set_current_mode(&sac_fallback_instance, (fallback_state - 1), &sac_status);
    }
}

/** @brief Send data with a specific connection.
 *
 *  @param[in]  transmitted_data  Data to be sent over the air.
 *  @param[in]  size              Size of the data to be sent over the air.
 *  @param[out] swc_err           Wireless Core error code.
 */
/** @brief AT+LE_UWB_PAIR -- start pairing.
 *
 *  Mirrors the pairing button rather than adding a second way to reach the same state
 *  machine, including the deferral while boot reconnect is polling: the connection handles
 *  have to stay valid until that loop unwinds.
 *
 *  Unpairs WITHOUT forgetting the stored peer. A pairing attempt that is started and then
 *  abandoned would otherwise cost the device the record it already had, leaving it unable to
 *  reconnect to a peer it was perfectly able to reach a moment earlier. The record is
 *  replaced when a new pairing succeeds, which is the only point at which the old one is
 *  genuinely obsolete.
 */
static void at_start_pairing(void)
{
    if (s_boot_reconnect_active) {
        s_boot_reconnect_abort = true;
        return;
    }

    if (device_pairing_state == DEVICE_PAIRING) {
        return;
    }

    if (device_pairing_state == DEVICE_PAIRED) {
        unpair_device(false);
    }

    enter_pairing_mode();
}

/** @brief AT+LE_UWB_CONNECT -- re-establish the stored link.
 *
 *  A reset, not a connect. Re-establishing a link means rebuilding the wireless core from the
 *  stored record, and main() already does exactly that on the way up, through
 *  try_boot_reconnect(). Doing it again in place would mean a second implementation of the
 *  same sequence, kept in step with the first by hand.
 *
 *  Does nothing when already paired, and nothing while boot reconnect is still running --
 *  resetting a device that is a second away from connecting by itself would restart the wait
 *  rather than shorten it.
 */
static void at_start_connect(void)
{
    if (device_pairing_state == DEVICE_PAIRED) {
        return;
    }

    if (s_boot_reconnect_active) {
        return;
    }

    facade_expansion_uart_flush();
    facade_system_reset();
}

/** @brief AT+LE_UWB_DISCONNECT -- take the link down, keep the pairing.
 *
 *  Tears the connections down and stays idle. The peer record survives, so AT+LE_UWB_CONNECT
 *  or a power cycle brings the link back without pairing again -- "disconnect" means the link,
 *  not the relationship.
 *
 *  The headset line answers this command by entering Standby instead, which is a deeper
 *  power-down. This application has no standby facade, and inventing one here to match a
 *  command name would be a power-management decision taken by accident. If the ODM needs the
 *  radios down as well, that is AT+LE_UWB_SHUTDOWN below, which says so.
 */
static void at_start_disconnect(void)
{
    if (device_pairing_state != DEVICE_PAIRED) {
        return;
    }

    unpair_device(false);
}

/** @brief AT+LE_UWB_SHUTDOWN -- take the link down and power the radios off.
 *
 *  The SR1100 sits on its own supply, so tearing down the connections leaves it drawing
 *  current. This is the command that stops that, and the only difference from disconnect.
 *
 *  Not a reset: the MCU keeps running and keeps answering AT, which is what makes the state
 *  reportable afterwards. Coming back needs AT+LE_UWB_CONNECT, which resets.
 */
static void at_start_shutdown(void)
{
    at_start_disconnect();
    facade_uwb_shutdown();
}

/** @brief AT+LE_UWB_CONN_STATUS? -- is there a link right now.
 *
 *  Paired is not connected. The pairing state says a peer is known; link_is_up() says packets
 *  are arriving from it. Reporting the first as the second would tell a host the link is fine
 *  while the other end is switched off.
 */
static bool at_get_link_status(void)
{
    if (device_pairing_state != DEVICE_PAIRED) {
        return false;
    }

    return link_is_up();
}

/** @brief AT+CONN_LM? -- link margin in dB, measured here on the data connection.
 *
 *  Measured locally now, where it used to be whatever the node last reported. Two things were
 *  wrong with the report: it was a raw transceiver code being printed with a "dB" suffix, and
 *  it stopped updating the moment the node did -- so the coordinator answered most confidently
 *  exactly when it knew least.
 *
 *  The direction is the honest catch, and it is the reverse one: this is the margin of the
 *  node's packets arriving here, not of the audio going out. It is a proxy for the path rather
 *  than a measurement of the audio link. A dB figure for the forward direction would have to
 *  come from the node, which means a new user_data_t field and both ends reflashed; see
 *  MD/uwb_quality_indicator_decision_spec.md 3.4. The quality report no longer depends on
 *  either, which is what makes the proxy acceptable here.
 *
 *  Block average when the platform fills it, running average otherwise; see the node's copy.
 */
static int32_t at_get_link_margin(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    swc_statistics_t *stats = NULL;
    uint32_t tenth_db;

    if (device_pairing_state != DEVICE_PAIRED) {
        return 0;
    }

    stats = swc_connection_update_stats(rx_data_conn, &swc_err);
    if ((stats == NULL) || (swc_err != SWC_ERR_NONE)) {
        return 0;
    }

    tenth_db = (stats->link_margin_block_avg_tenth_db != 0) ? stats->link_margin_block_avg_tenth_db
                                                            : stats->link_margin_avg;

    return (int32_t)(tenth_db / 10);
}

/** @brief The rung this side is currently sending at.
 *
 *  This is the only quality input the coordinator has, and it is deliberately not paired with
 *  a dropout count: the gaps happen at the node's speaker, and this side cannot hear them. So
 *  a coordinator's LE_UWB_QUALITY is a statement about how much headroom the ladder has left,
 *  never about what anyone heard, and it will not report CRITICAL for a dropout the way the
 *  node does. MD/uwb_quality_indicator_decision_spec.md 3.4 has the reasoning and what it
 *  would take to change it.
 *
 *  Returns 0 rather than asserting when there is nothing to ask; see the node's copy.
 */
static uint8_t at_get_fb_rung(void)
{
    sac_status_t sac_status = SAC_OK;
    uint8_t rung;

    if (device_pairing_state != DEVICE_PAIRED) {
        return 0;
    }

    rung = sac_fallback_get_current_mode(&sac_fallback_instance, &sac_status);

    return (sac_status == SAC_OK) ? rung : 0;
}

/** @brief AT+PLAY / STOP / NEXT_TRACK / PRE_TRACK -- forward a media key to the node.
 *
 *  Queued, not sent: the data packet goes out on its own 10 ms timer, and sending here would
 *  mean a second transmit path with its own failure modes.
 *
 *  AT_CMD_VOL is dropped rather than queued. Volume on this link is the node's own output
 *  level and is applied there by its local handler; forwarding it from this side would give
 *  one setting two owners.
 */
static void at_cmd_tx(uint8_t cmd_type, uint8_t value)
{
    (void)value;

    if (cmd_type == AT_CMD_VOL) {
        return;
    }

    s_pending_cmd = cmd_type;
}

static void wireless_send_data(const void *transmitted_data, uint8_t size, swc_error_t *swc_err)
{
    uint8_t *buffer = NULL;

    /* Get buffer from queue to hold data. */
    swc_connection_get_payload_buffer(tx_data_conn, &buffer, swc_err);
    if ((*swc_err != SWC_ERR_NONE) || (buffer == NULL)) {
        s_data_queue_full_count++;
        return;
    }

    /* Format the new payload. */
    if (transmitted_data != NULL) {
        memcpy(buffer, transmitted_data, size);
    }

    /* Send the payload through the Wireless Core. */
    swc_connection_send(tx_data_conn, buffer, size, swc_err);
    ASSERT_SWC_STATUS(*swc_err);
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
     * This used to return 0 on an oversized payload WITHOUT calling
     * swc_connection_receive_complete(), which leaks the receive buffer. Unreachable while
     * both ends run identical firmware, which is why it survived: user_data_t was two bytes
     * on both sides and nothing could ever arrive longer.
     *
     * That stops being true the moment the struct grows -- the AT vendor pass-through takes
     * it to five bytes in the common case -- because then an older build receives a longer
     * packet, which is an ordinary condition during a rollout rather than an error. And the
     * failure is not the mild one it looks like. A leaked buffer every 10 ms fills the
     * receive queue and kills the data connection for good: link margin stops updating and
     * the fallback ladder, which reads queue depth, wanders on stale information.
     *
     * Truncating is safe because user_data_t is append-only, so the prefix a shorter struct
     * understands sits at the same offsets in the longer one. The opposite case -- a payload
     * shorter than the struct -- already worked, because callers zero-initialize and every
     * field's zero means "absent". Both directions therefore degrade to "the fields this
     * build knows about", which is the whole intent of the layout. */
    copy_size = (payload_size > size) ? size : payload_size;

    if ((received_data != NULL) && (payload != NULL) && (copy_size > 0)) {
        memcpy(received_data, payload, copy_size);
    }

    /* Free the payload memory. Must happen on every path that took a payload. */
    swc_connection_receive_complete(rx_data_conn, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    return copy_size;
}

/** @brief Get the accumulator size based on the current fallback mode.
 *
 *  @param[in] pipeline  Audio pipeline.
 *  @return Accumulator size for the current fallback mode.
 */
static uint32_t get_accumulator_size(sac_pipeline_t *pipeline)
{
    sac_status_t sac_status = SAC_OK;
    uint8_t current_mode = sac_fallback_get_current_mode(&sac_fallback_instance, &sac_status);

    uint32_t acc_size = (pipeline->producer->cfg.audio_payload_size * main_channel_acc_mul[current_mode]) /
                        main_channel_acc_div[current_mode];

    return acc_size;
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

    /* Initialize Audio Core. */
    app_audio_core_init();

    /* Initialize GPIOs and peripherals for audio operations. */
    facade_audio_coord_init();

    /* Start the audio pipeline. */
    sac_pipeline_start(sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Start timer used for audio process. */
    facade_audio_process_timer_start();

    /* Start data and statistics timer. */
    facade_data_timer_start();
}

void sac_error_handler(sac_status_t sac_status)
{
    char buffer[ERROR_MESSAGE_BUFFER_SIZE];

    snprintf(buffer, sizeof(buffer), "SAC Error! Code: %d\n\r", sac_status);
    facade_print_error_string(buffer);

    while (1);
}

void swc_error_handler(swc_error_t swc_status)
{
    char buffer[ERROR_MESSAGE_BUFFER_SIZE];

    snprintf(buffer, sizeof(buffer), "SWC Error ! Code: %d\n\r", swc_status);
    facade_print_error_string(buffer);

    while (1);
}
