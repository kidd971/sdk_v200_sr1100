/** @file  puretone_unidirectional_node.c
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
#include "reconnect_store.h"
#include "swc_cfg.h"
#include "swc_cfg_node.h"
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
#define SAC_MEM_POOL_SIZE 110000
/* Total memory needed for the Wireless Core. */
#define SWC_MEM_POOL_SIZE 10500
/* The data connection supports up to 16 bytes. */
#define MAX_DATA_PAYLOAD_SIZE 16

/** @brief How long to wait for the stored peer before giving up on a silent reconnect, ms. */
#define RECONNECT_TIMEOUT_MS 10000

/** @brief How long without a packet from the coordinator before the link counts as down.
 *
 *  The coordinator's data_callback() sends one every DATA_TX_PERIOD_MS unconditionally, so
 *  this is 20 consecutive misses.
 */
#define COORD_RX_TIMEOUT_MS 200
/* Length of the statistics array used for terminal display. */
#define STATS_ARRAY_LENGTH 3000
/* Period for data transmission timer in ms. */
#define DATA_TX_PERIOD_MS 10
/* Size of the buffer used to print errors. */
#define ERROR_MESSAGE_BUFFER_SIZE 120
/* Interval to print statistics in ms. */
/* Bring the receiver up on the calibration saved in the radio's NVM as well as the fresh one.
 * SPARK's audio demo does; the SDK example this app came from does not. */
/** @brief DIAGNOSTIC: pin multi-radio selection instead of letting the algorithm choose.
 *
 *  0 = leave it alone (SWC_MULTI_RADIO_SELECT_MODE_ALGO, the default), 1 = always radio 1,
 *  2 = always radio 2. Only has any effect on a dual-radio build; the call is not even
 *  compiled otherwise, so the default costs nothing and changes nothing.
 *
 *  It exists to cut one question in half. A dual-radio node receives almost nothing on this
 *  board while either radio ALONE works, and both radios are demonstrably alive -- the
 *  liveness counters run within 1% of each other. That leaves two candidates: the dual-radio
 *  schedule and initialisation, or the algorithm that picks which radio to believe. Pinning
 *  the selection keeps everything about the dual build except the choosing:
 *
 *    pinned and it works   -> the schedule and init are fine, the selection algorithm is not
 *    pinned and it fails   -> the fault is earlier than selection
 *
 *  Neither answer is available from a single-radio build, because that one also drops the
 *  second radio's initialisation, its timeslot behaviour and the multi-radio timer.
 */
#ifndef MULTI_RADIO_FORCE
#define MULTI_RADIO_FORCE 0
#endif

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
} user_data_t;

/* PRIVATE GLOBALS ************************************************************/
/* **** Audio Core **** */
/** Sample format of audio samples produced or received by the codec of the Node. */
static const sac_sample_format_t I2S_SAC_SAMPLE_FORMAT = {
    .bit_depth = SAC_24BITS,
    .sample_encoding = SAC_SAMPLE_UNPACKED,
};

/* Sample format of audio samples received by the SWC from the Coordinator. */
static const sac_sample_format_t MAIN_CHANNEL_SAC_SAMPLE_FORMAT = {
    .bit_depth = SAC_24BITS,
    .sample_encoding = SAC_SAMPLE_PACKED,
};

#define MAIN_CHANNEL_CONSUMER_SAC_SAMPLE_FORMAT \
    (USB_AUDIO_ENABLED ? MAIN_CHANNEL_SAC_SAMPLE_FORMAT : I2S_SAC_SAMPLE_FORMAT)

static uint8_t audio_memory_pool[SAC_MEM_POOL_SIZE];
static sac_pipeline_t *main_channel_sac_pipeline;
static sac_pipeline_t *main_channel_accumulator_pipeline;

/* **** Main Channel Processing Stages **** */
static sac_fallback_instance_t main_channel_fallback_instance;
static sac_processing_t *main_channel_fallback_processing;
static sac_packing_instance_t main_channel_unpacking_instance;
static sac_processing_t *main_channel_unpacking_processing;
static sac_packing_instance_t main_channel_fbk_unpacking_instance;
static sac_processing_t *main_channel_fbk_unpacking_processing;
static sac_compression_instance_t main_channel_decompression_instance;
static sac_processing_t *main_channel_decompression_processing;
static sac_processing_t *main_channel_cdc_processing;
static sac_mute_on_underflow_instance_t main_channel_mute_on_underflow_instance;
static sac_processing_t *main_channel_mute_on_underflow_processing;
static src_cmsis_instance_t main_channel_upsampling_instance;
static sac_processing_t *main_channel_upsampling_processing;
/* Second, independent SRC for the 24 kHz rung, mirroring the coordinator's 1:4 decimator. A ratio
 * is fixed at init, so the 48->96 kHz instance above cannot be reused, and the two have to mirror:
 * on the packet that ends a discard the decimator appends (FIR_NUMTAPS / ratio * channel_count) / 2
 * samples and this side expects exactly that many, which only agrees when multiply_ratio here
 * equals divide_ratio there. */
static src_cmsis_instance_t main_channel_upsampling4_instance;
static sac_processing_t *main_channel_upsampling4_processing;
static sac_mute_packet_instance_t main_channel_mute_packet_instance;
static sac_processing_t *main_channel_mute_packet_processing;
static sac_sample_accumulator_instance_t main_channel_sample_accumulator_instance;
static sac_processing_t *main_channel_sample_accumulator_processing;
static sac_volume_instance_t main_channel_volume_instance;
static sac_processing_t *main_channel_volume_processing;

/* **** Endpoints **** */
static sac_endpoint_t *main_channel_consumer_endpoint;
static ep_swc_instance_t main_channel_swc_producer_instance;
static sac_endpoint_t *main_channel_swc_producer_endpoint;
static sac_endpoint_t *dummy_audio_consumer;
static sac_endpoint_t *dummy_audio_producer;

/* **** Wireless Core **** */
static uint8_t swc_memory_pool[SWC_MEM_POOL_SIZE];

/* ** RX Connections ** */
static swc_connection_t *rx_audio_conn;
static swc_connection_t *rx_data_conn;

/* ** TX Connections ** */
static swc_connection_t *tx_data_conn;

static const uint32_t timeslot_us[] = SCHEDULE;
static const uint32_t channel_sequence[] = CHANNEL_SEQUENCE;
static const uint32_t channel_frequency[] = CHANNEL_FREQ;

static const int32_t tx_timeslots[] = NODE_TIMESLOTS;
static const int32_t rx_timeslots[] = COORD_TIMESLOTS;

/* **** Application Specific **** */
static facade_certification_mode_t certification_mode;
/* Variables supporting pairing between the two devices. */
static device_pairing_state_t device_pairing_state;
static pairing_cfg_t app_pairing_cfg;
static pairing_assigned_address_t pairing_assigned_address;

/* When the coordinator's data packet last arrived, and whether one ever has since the
 * connections were built. Written from the RX callback, read from the main loop. */
static volatile uint32_t s_coord_rx_tick;
static volatile bool s_coord_rx_seen;

/* True while try_boot_reconnect() owns a half-open link and is polling it; the button
 * handler defers through s_boot_reconnect_abort so the loop unwinds before teardown. */
static bool s_boot_reconnect_active;
static bool s_boot_reconnect_abort;

/** @brief Outcome of a boot auto-reconnect attempt. */
typedef enum {
    BOOT_RECONNECT_OK,   /* The stored link was re-established. */
    BOOT_RECONNECT_PAIR, /* No usable record, or the user asked to pair mid-attempt. */
    BOOT_RECONNECT_IDLE, /* Had a record but the coordinator was not up yet: keep the core
                            running and let it sync whenever the peer appears. */
} boot_reconnect_result_t;

/* Fallback latency.
 *
 * uint16_t, not uint8_t: the fifo figure is the queue depth times the bytes per sample, so it
 * passes 255 once the bottom rungs carry 40 ms -- 90 packets x 3 bytes = 270, which as a uint8_t
 * wrapped to 14 and would have set a two-packet target instead of a deep one. Both consumers take
 * a wider type already (facade_app_audio_usb_set_epin_target_fifo_size is uint16_t, the queue size
 * goes into a uint32_t), so nothing downstream had to change. The assertions keep the next
 * latency increase from repeating it silently. */
uint16_t main_channel_fbk_latency_queue_size[] = MAIN_CHANNEL_FALLBACK_LATENCY_QUEUE_SIZE;
uint16_t main_channel_fbk_latency_fifo_size[] = MAIN_CHANNEL_FALLBACK_LATENCY_FIFO_SIZE;

_Static_assert(MAIN_CHANNEL_FBK_3_LATENCY_QUEUE_SIZE * ((MAIN_CHANNEL_BIT_DEPTH + 7) / SAC_BYTE_SIZE_BITS) <=
                   UINT16_MAX,
               "fallback latency fifo size no longer fits its type");
_Static_assert(MAIN_CHANNEL_FBK_4_LATENCY_QUEUE_SIZE * ((MAIN_CHANNEL_BIT_DEPTH + 7) / SAC_BYTE_SIZE_BITS) <=
                   UINT16_MAX,
               "fallback latency fifo size no longer fits its type");

static volatile uint32_t main_channel_trigger_count;

/* PRIVATE FUNCTION PROTOTYPE *************************************************/
static void app_init(void);
static void app_swc_core_init(pairing_assigned_address_t *app_pairing, swc_error_t *swc_err);
static void app_audio_core_init(void);

/* **** Callbacks **** */
/* Callbacks that are used for the main channel. */
static void conn_rx_audio_success_callback(void *conn, void *arg);
static void conn_tx_data_success_callback(void *conn, void *arg);
static void audio_tx_complete_callback(void);
static void audio_process_callback(void);
/* Callbacks that are used for data and pairing processes. */
static void conn_rx_data_success_callback(void *conn, void *arg);
static void data_callback(void);
static void pairing_process_callback(void);
static void pairing_button_callback(void);
static void volume_up(void);
static void volume_down(void);

/* **** Processing Stages **** */
static void app_audio_core_fallback_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_upsampling_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_mute_packet_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_unpacking_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_decompressing_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_volume_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_mute_on_underflow_interface_init(sac_processing_interface_t *iface);

/* **** Button Actions **** */
static void enter_pairing_mode(void);
static bool link_is_up(void);
static boot_reconnect_result_t try_boot_reconnect(void);
static void unpair_device(bool forget_peer);
static void abort_pairing_procedure(void);

/* **** Fallback LED and Terminal Display **** */
static void fallback_led_handler(void);
static bool should_print_stats(void);
static void print_stats(void);
static void print_diagnostics(void);
#if !STATS_VERBOSE
static void print_stats_compact(void);
static const char *fallback_mode_name(uint8_t mode);
#endif

static void wireless_send_data(const void *transmitted_data, uint8_t size, swc_error_t *swc_err);
static uint16_t wireless_read_data(void *received_data, uint8_t size, swc_error_t *swc_err);

/* PUBLIC FUNCTIONS ***********************************************************/
int main(void)
{
#if USB_AUDIO_ENABLED
    /* Configure usb audio before board initialization. */
    facade_configure_node_usb_audio();
#endif
    /* Initialize the board and all GPIOs and peripherals for minimal operations. */
    facade_board_init();

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
        char banner[96];

        snprintf(banner, sizeof(banner), "\r\n[BOOT] puretone_unidirectional node " BOARD_NAME " " RADIO_TAG " "
                                         __DATE__ " " __TIME__ "\r\n");
        facade_print_string(banner);
    }

    /* Initialize wireless core context switch handler before pairing is available. */
    facade_set_context_switch_handler(swc_connection_callbacks_processing_handler);

    facade_button_callbacks_t button_callbacks = {
        .pairing_callback = pairing_button_callback,
        .volume_up_callback = volume_up,
        .volume_down_callback = volume_down,
    };
    facade_set_button_callbacks(button_callbacks);

    /* Audio process timer initialization. */
    facade_audio_process_timer_init(audio_process_callback);

    /* Timer that updates statistics display every second and transmits button state to the Coordinator at the
     * DATA_TX_PERIOD_MS interval.
     */
    facade_data_timer_init(DATA_TX_PERIOD_MS);
    facade_data_timer_set_callback(data_callback);

    certification_mode = facade_get_node_certification_mode();
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
     * rather than pairing again. Only pair when there is no usable record -- a factory
     * device, or one the user unpaired -- or when the user asks for it mid-attempt. */
    if (try_boot_reconnect() == BOOT_RECONNECT_PAIR) {
        enter_pairing_mode();
    }

    while (1) {
        facade_button_handling();

        if (device_pairing_state == DEVICE_PAIRED) {
            fallback_led_handler();
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
    uint16_t local_address = app_pairing->node_address;
    uint16_t remote_address = app_pairing->coordinator_address;
    swc_radio_handle_t *radio_handle = NULL;
#if (SWC_RADIO_COUNT == 2)
    swc_radio_handle_t *radio_handle_2 = NULL;
#endif

    /* In cert mode, pairing has not run -- override addresses directly. */
    if (certification_mode != FACADE_CERTIF_NONE) {
        app_pairing->pan_id = 0xABC;
        remote_address = 0x1;
        local_address = 0x2;
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
        .role = SWC_ROLE_NODE,
        .coordinator_address = remote_address,
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

#if (SWC_RADIO_COUNT == 2)
    radio_handle_2 = swc_radio_module_calib(SWC_RADIO_ID_2, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Initialize the radio. */
    swc_radio_module_init(radio_handle_2, RADIO_USE_SAVED_CALIB, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

#if (MULTI_RADIO_FORCE != 0)
    /* Diagnostic only -- see MULTI_RADIO_FORCE. Deliberately after both radios are
     * initialised, so that everything about the dual build is still in place and the only
     * thing removed is the choosing. */
    swc_set_multi_radio_select_mode((MULTI_RADIO_FORCE == 1) ? SWC_MULTI_RADIO_SELECT_MODE_RADIO1
                                                             : SWC_MULTI_RADIO_SELECT_MODE_RADIO2,
                                    swc_err);
    ASSERT_SWC_STATUS(*swc_err);
#endif
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
    const swc_connection_concurrency_cfg_t rx_audio_concurrency_cfg = {
        .enabled = true,
        .try_count = MAIN_CHANNEL_SWC_CCA_FB_TRY_COUNT, /* Use maximum CCA try count on this connection. */
        .retry_time = MAIN_CHANNEL_SWC_CCA_AUDIO_RETRY_TIME,
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

    /* **** TX Connections **** */
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
    tx_data_conn = swc_connection_init(tx_data_conn_cfg, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    swc_connection_set_modulation(tx_data_conn, SWC_MOD_IOOK, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    swc_connection_set_fec_ratio(tx_data_conn, SWC_FEC_1_2_5_0, swc_err);
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
            .tx_pulse_count = TX_DATA_PULSE_COUNT,
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

    /* Data connection priority settings. */
    swc_connection_set_connection_priority(tx_data_conn, DATA_CONNECTION_PRIORITY, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Data connection callback settings. */
    swc_connection_set_tx_success_callback(tx_data_conn, conn_tx_data_success_callback, NULL, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Handle certification mode. */
    swc_set_certification_mode(certification_mode != FACADE_CERTIF_NONE, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Wireless Core setup. */
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
    facade_audio_process_timer_trigger();
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
    swc_error_t swc_err = SWC_ERR_NONE;
    user_data_t received_user_data = {0};
    uint16_t read_data_size;

    (void)conn;
    (void)arg;

    /* Get received payload. */
    read_data_size = wireless_read_data(&received_user_data, sizeof(received_user_data), &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    if (read_data_size > 0) {
        /* Depending on the requested button state from the Coordinator, the specified LED turns on or off. */
        if (received_user_data.button_state == false) {
            facade_empty_payload_received_status();
        } else {
            facade_payload_received_status();
        }

        /* Peer heartbeat for link_is_up(). The data connection rather than the audio one:
         * the coordinator sends on this every DATA_TX_PERIOD_MS whatever the audio is
         * doing, so it stays a heartbeat when the ladder is deep and the audio slots are
         * mostly idle. */
        s_coord_rx_tick = facade_get_tick_ms();
        s_coord_rx_seen = true;
    }
}

/** @brief Audio peripheral transmit complete callback.
 *
 *  @note This feeds audio packets to the codec. It needs to be executed every time a DMA transfer to the codec is
 *        completed in order to keep playing audio.
 */
static void audio_tx_complete_callback(void)
{
    sac_status_t sac_status = SAC_OK;

#if USB_AUDIO_ENABLED
    uint32_t usb_buf_rem = facade_app_audio_usb_get_epin_fifo_remaining();
    uint32_t target_fifo_size;

    target_fifo_size =
        main_channel_fbk_latency_fifo_size[sac_fallback_get_current_mode(&main_channel_fallback_instance, &sac_status)];
    facade_app_audio_usb_set_epin_target_fifo_size(target_fifo_size);

    if ((sac_pipeline_get_consumer_buffer_load(main_channel_accumulator_pipeline, &sac_status) > 0) &&
        ((usb_buf_rem / MAIN_CHANNEL_SWC_PAYLOAD_SIZE) > 1)) {
        sac_pipeline_consume(main_channel_accumulator_pipeline, &sac_status);
    }
#else
    uint32_t target_queue_size;

    /* Set audio latency based on the fallback mode. */
    target_queue_size =
        main_channel_fbk_latency_queue_size[sac_fallback_get_current_mode(&main_channel_fallback_instance,
                                                                          &sac_status)];
    facade_app_audio_cdc_set_target_queue_size(main_channel_accumulator_pipeline, target_queue_size, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    sac_pipeline_consume(main_channel_accumulator_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif
}

/** @brief Callback handling the audio process that triggers with the app timer.
 */
static void audio_process_callback(void)
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
    {
        uint32_t buffer_load = 0;

        buffer_load = sac_pipeline_get_consumer_buffer_load(main_channel_accumulator_pipeline, &sac_status);
        ASSERT_SAC_STATUS(sac_status);

        /* Consume all nodes into the USB FIFO. */
        while (buffer_load > 0 && facade_app_audio_usb_get_epin_fifo_remaining() >= MAIN_CHANNEL_SWC_PAYLOAD_SIZE) {
            sac_pipeline_consume(main_channel_accumulator_pipeline, &sac_status);
            ASSERT_SAC_STATUS(sac_status);
            buffer_load--;
        }
    }
#endif

    if (main_channel_trigger_count > 0) {
        /* Retrigger the processing. */
        facade_audio_process_timer_trigger();
    }
}

/** @brief Initialize the Audio Core.
 */
static void app_audio_core_init(void)
{
    sac_status_t sac_status = SAC_OK;

    /* ** Endpoint Interfaces ** */
    sac_endpoint_interface_t main_channel_consumer_iface = {0};
    sac_endpoint_interface_t main_channel_swc_producer_iface = {0};

    /* ** Processing Stages Interfaces ** */
    sac_processing_interface_t fallback_iface = {0};
    sac_processing_interface_t main_channel_upsampling_iface = {0};
    sac_processing_interface_t main_channel_mute_packet_iface = {0};
    sac_processing_interface_t main_channel_unpacking_iface = {0};
    sac_processing_interface_t main_channel_decompression_iface = {0};
    sac_processing_interface_t main_channel_volume_iface = {0};
    sac_processing_interface_t main_channel_mute_on_underflow_iface = {0};

    sac_endpoint_swc_init(&main_channel_swc_producer_iface, NULL);
    sac_facade_audio_endpoint_init(NULL, &main_channel_consumer_iface);
    facade_set_audio_complete_callback(audio_tx_complete_callback, NULL);

    app_audio_core_fallback_interface_init(&fallback_iface);
    app_audio_core_upsampling_interface_init(&main_channel_upsampling_iface);
    app_audio_core_mute_packet_interface_init(&main_channel_mute_packet_iface);
    app_audio_core_unpacking_interface_init(&main_channel_unpacking_iface);
    app_audio_core_decompressing_interface_init(&main_channel_decompression_iface);
    app_audio_core_volume_interface_init(&main_channel_volume_iface);
    app_audio_core_mute_on_underflow_interface_init(&main_channel_mute_on_underflow_iface);

    main_channel_swc_producer_instance.connection = rx_audio_conn;

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
     * Input:      Stereo stream of 96kHz/24-bit depth samples is received over the air from the Coordinator.
     * Processing: Mute packet process.
     * Output:     Stereo stream of 96kHz/24-bit.
     *
     * +-----+    +-------------+    +-----+
     * | SWC | -> | Mute Packet | -> | USB |
     * +-----+    +-------------+    +-----+
     *
     * ***** NORMAL MODE I2S (Fallback mode 0) *****
     * Input:      Stereo stream of 96kHz/24-bit depth samples is received over the air from the Coordinator.
     * Processing: Mute packet process.
     * Processing: Unpacking from 24 bits to 24 bits encoded on 32 bits audio samples.
     * Processing: Digital volume control followed by clock drift compensation and mute on glitch.
     * Output:     Stereo stream of 96kHz/24-bit.
     *
     * +-----+    +-------------+    +-----------+    +----------------+    +-----+    +----------------+    +-----+
     * | SWC | -> | Mute Packet | -> | Unpacking | -> | Digital Volume | -> | CDC | -> | Mute on Glitch | -> | I2S |
     * +-----+    +-------------+    +-----------+    +----------------+    +-----+    +----------------+    +-----+
     *
     * ***** FALLBACK MODE USB (Fallback mode 1) *****
     * Input:      Stereo stream of 48kHz/24-bit depth samples is received over the air from the Coordinator.
     * Processing: Mute packet process.
     * Processing: Upsampling audio samples from 48kHz to 96kHz.
     * Processing: Audio sample accumulator.
     * Output:     Stereo stream of 96kHz/24-bit.
     *
     * +-----+    +-------------+    +---------------+    +------------------+    +-----+
     * | SWC | -> | Mute Packet | -> | Upsampling 2x | -> | Accumulator 1.7x | -> | USB |
     * +-----+    +-------------+    +---------------+    +------------------+    +-----+
     *
     * ***** FALLBACK MODE I2S (Fallback mode 1) *****
     * Input:      Stereo stream of 48kHz/24-bit depth samples is received over the air from the Coordinator.
     * Processing: Mute packet process.
     * Processing: Unpacking from 24 bits to 24 bits encoded on 32 bits audio samples.
     * Processing: Upsampling audio samples from 48kHz to 96kHz.
     * Processing: Audio sample accumulator.
     * Processing: Digital volume control followed by clock drift compensation and mute on glitch.
     * Output:     Stereo stream of 96kHz/24-bit.
     *
     * +-----+    +-------------+    +-----------+    +---------------+    +------------------+    +----------------+
     * | SWC | -> | Mute Packet | -> | Unpacking | -> | Upsampling 2x | -> | Accumulator 1.7x | -> | Digital Volume | --
     * +-----+    +-------------+    +-----------+    +---------------+    +------------------+    +----------------+  |
     *       -----------------------------------------------------------------------------------------------------------
     *       |    +-----+    +----------------+    +-----+
     *       ---> | CDC | -> | Mute on glitch | -> | I2S |
     *            +-----+    +----------------+    +-----+
     *
     * ***** FALLBACK MODE USB (Fallback mode 2) *****
     * Input:      Stereo stream of 48kHz/16-bit depth samples is received over the air from the Coordinator.
     * Processing: Mute packet process.
     * Processing: Unpacking from 16 bits to 24 bits.
     * Processing: Upsampling audio samples from 48kHz to 96kHz.
     * Processing: Audio sample accumulator.
     * Output:     Stereo stream of 96kHz/24-bit.
     *
     * +-----+    +-------------+    +-----------+    +---------------+    +------------------+    +-----+
     * | SWC | -> | Mute Packet | -> | Unpacking | -> | Upsampling 2x | -> | Accumulator 1.7x | -> | USB |
     * +-----+    +-------------+    +-----------+    +---------------+    +------------------+    +-----+
     *
     * ***** FALLBACK MODE I2S (Fallback mode 2) *****
     * Input:      Stereo stream of 48kHz/16-bit depth samples is received over the air from the Coordinator.
     * Processing: Mute packet process.
     * Processing: Unpacking from 16 bits to 24 bits encoded on 32 bits audio samples.
     * Processing: Upsampling audio samples from 48kHz to 96kHz.
     * Processing: Audio sample accumulator.
     * Processing: Digital volume control followed by clock drift compensation and mute on glitch.
     * Output:     Stereo stream of 96kHz/24-bit.
     *
     * +-----+    +-------------+    +-----------+    +---------------+    +------------------+    +----------------+
     * | SWC | -> | Mute Packet | -> | Unpacking | -> | Upsampling 2x | -> | Accumulator 1.7x | -> | Digital Volume | --
     * +-----+    +-------------+    +-----------+    +---------------+    +------------------+    +----------------+  |
     *       -----------------------------------------------------------------------------------------------------------
     *       |    +-----+    +----------------+    +-----+
     *       ---> | CDC | -> | Mute on glitch | -> | I2S |
     *            +-----+    +----------------+    +-----+
     *
     * ***** FALLBACK MODE USB (Fallback mode 3) *****
     * Input:      Stereo stream of 48kHz/24-bit depth samples is received over the air from the Coordinator.
     * Processing: Mute packet process.
     * Processing: Decompression of samples compressed with ADPCM.
     * Processing: Upsampling audio samples from 48kHz to 96kHz.
     * Processing: Audio sample accumulator 2.3x.
     * Output:     Stereo stream of 96kHz/24-bit.
     *
     * +-----+    +-------------+    +---------------------+    +---------------+    +------------------+    +-----+
     * | SWC | -> | Mute Packet | -> | ADPCM Decompression | -> | Upsampling 2x | -> | Accumulator 2.3x | -> | USB |
     * +-----+    +-------------+    +---------------------+    +---------------+    +------------------+    +-----+
     *
     * ***** FALLBACK MODE I2S (Fallback mode 3) *****
     * Input:      Stereo stream of 48kHz/24-bit depth samples is received over the air from the Coordinator.
     * Processing: Mute packet process.
     * Processing: Decompression of samples compressed with ADPCM.
     * Processing: Upsampling audio samples from 48kHz to 96kHz.
     * Processing: Audio sample accumulator 2.3x.
     * Processing: Digital volume control followed by clock drift compensation and mute on glitch.
     * Output:     Stereo stream of 96kHz/24-bit.
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
    /* 24-bit packed audio from SWC at half sample rate. */
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

    /* 24 -> 96 kHz for the bottom rung, mirroring the coordinator's 1:4. A single 4:1, not two
     * chained 2:1: the interpolation path validates its input against
     * pipeline->_internal.current_sample_count, which the fallback stage writes once per packet
     * and never updates between stages, so a chained second stage would always see half what the
     * check expects and be rejected. No discard variant, matching the coordinator -- see the note
     * on its 1:4 instance. */
    main_channel_upsampling4_instance.cfg.multiply_ratio = SAC_SRC_FOUR;
    main_channel_upsampling4_instance.cfg.divide_ratio = SAC_SRC_ONE;
    main_channel_upsampling4_instance.cfg.payload_size = MAIN_CHANNEL_SWC_PAYLOAD_SIZE *
                                                         main_channel_upsampling4_instance.cfg.divide_ratio /
                                                         main_channel_upsampling4_instance.cfg.multiply_ratio;
    main_channel_upsampling4_instance.cfg.payload_size =
        (main_channel_upsampling4_instance.cfg.payload_size * MAIN_CHANNEL_MAX_ACC_MUL) / MAIN_CHANNEL_MAX_ACC_DIV;

    /* Upsampling does not change the sample format. */
    main_channel_upsampling4_instance.cfg.input_sample_format = MAIN_CHANNEL_CONSUMER_SAC_SAMPLE_FORMAT;
    main_channel_upsampling4_instance.cfg.output_sample_format = MAIN_CHANNEL_CONSUMER_SAC_SAMPLE_FORMAT;
    main_channel_upsampling4_instance.cfg.channel_count = MAIN_CHANNEL_CHANNEL_COUNT;
    main_channel_upsampling4_processing = sac_processing_stage_init((void *)&main_channel_upsampling4_instance,
                                                                    "Audio Upsampling 4:1",
                                                                    main_channel_upsampling_iface, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Mute packet processing stage initialization. */
    main_channel_mute_packet_instance.is_tx = false;
    main_channel_mute_packet_processing = sac_processing_stage_init((void *)&main_channel_mute_packet_instance,
                                                                    "Mute packet", main_channel_mute_packet_iface,
                                                                    &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Processing stage unpacks into 24 bits on 32-bit words for I2S. */
    main_channel_unpacking_instance.packing_mode = SAC_UNPACK_24BITS;
    main_channel_unpacking_processing = sac_processing_stage_init((void *)&main_channel_unpacking_instance,
                                                                  "Audio Unpacking", main_channel_unpacking_iface,
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

    /* Processing stage that handles the mute on underflow. */
    main_channel_mute_on_underflow_instance.reload_value =
        sac_get_nb_packets_in_x_ms(30, MAIN_CHANNEL_I2S_PAYLOAD_SIZE, MAIN_CHANNEL_CHANNEL_COUNT,
                                   MAIN_CHANNEL_CONSUMER_SAC_SAMPLE_FORMAT, I2S_SAMPLE_RATE_HZ);

    main_channel_mute_on_underflow_processing =
        sac_processing_stage_init((void *)&main_channel_mute_on_underflow_instance, "Mute on underflow",
                                  main_channel_mute_on_underflow_iface, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Dummy consumer endpoint for the first pipeline stage. */
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

    /* Audio pipeline initialization (SWC -> Accumulator). */
    sac_pipeline_cfg_t main_channel_pipeline_cfg = {
        .do_initial_buffering = false,
    };
    main_channel_sac_pipeline = sac_pipeline_init("SWC -> Accumulator", main_channel_swc_producer_endpoint,
                                                  main_channel_pipeline_cfg, dummy_audio_consumer, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Add processing stages. */
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_mute_packet_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_fallback_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    /* Decompress. */
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_decompression_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    /* Unpack to 24-bit. */
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

    /* Fallback mode 0 configuration: 96kHz 24-bit. */
    mode_cfg.sample_count = MAIN_CHANNEL_SAMPLE_COUNT;
    uint8_t mode_index_96k = sac_fallback_add_mode(&main_channel_fallback_instance, "96kHz 24-bit", mode_cfg,
                                                   &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#if !USB_AUDIO_ENABLED
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index_96k, main_channel_unpacking_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#else
    (void)mode_index_96k;
#endif

    /* Fallback mode 1 configuration: 48kHz 24-bit. */
    mode_cfg.sample_count = MAIN_CHANNEL_FBK_1_SAMPLE_COUNT;
    mode_index = sac_fallback_add_mode(&main_channel_fallback_instance, "48kHz 24-bit", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#if !USB_AUDIO_ENABLED
    /* When using I2S, unpacking is required to convert 24-bit audio aligned on 32-bit words. */
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_unpacking_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_upsampling_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Fallback mode 2 configuration: 48kHz 16-bit. */
    mode_cfg.sample_count = MAIN_CHANNEL_FBK_2_SAMPLE_COUNT;
    mode_index = sac_fallback_add_mode(&main_channel_fallback_instance, "48kHz 16-bit", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_upsampling_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_fbk_unpacking_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Fallback mode 3 configuration: 48kHz ADPCM. */
    mode_cfg.sample_count = MAIN_CHANNEL_FBK_3_SAMPLE_COUNT;
    mode_index = sac_fallback_add_mode(&main_channel_fallback_instance, "48kHz ADPCM", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_upsampling_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_decompression_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Fallback mode 4 configuration: 24kHz ADPCM. Mirrors the coordinator's mode 4 -- same chain
     * as mode 3 with the 4:1 interpolator in place of the 2:1. The mode index is a wire-level
     * contract, so both sides must be flashed together. */
    mode_cfg.sample_count = MAIN_CHANNEL_FBK_4_SAMPLE_COUNT;
    mode_index = sac_fallback_add_mode(&main_channel_fallback_instance, "24kHz ADPCM", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_upsampling4_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_decompression_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /** Start fallback in best quality. */
    sac_fallback_set_current_mode(&main_channel_fallback_instance, 0, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Second pipeline stage: Accumulator -> Audio output. */
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

/** @brief Update the fallback LED indicator.
 */
static void fallback_led_handler(void)
{
    sac_status_t sac_status = SAC_OK;

    facade_fallback_status(sac_fallback_get_current_mode(&main_channel_fallback_instance, &sac_status));
    ASSERT_SAC_STATUS(sac_status);
}

/** @brief Volume up button callback.
 */
static void volume_up(void)
{
    sac_status_t sac_status = SAC_OK;

    sac_processing_ctrl(main_channel_volume_processing, main_channel_accumulator_pipeline, SAC_VOLUME_INCREASE,
                        SAC_NO_ARG, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
}

/** @brief Volume down button callback.
 */
static void volume_down(void)
{
    sac_status_t sac_status = SAC_OK;

    sac_processing_ctrl(main_channel_volume_processing, main_channel_accumulator_pipeline, SAC_VOLUME_DECREASE,
                        SAC_NO_ARG, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
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

    /* ** Audio statistics ** */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, audio_stats_str);
    /* What the pool actually cost, so SAC_MEM_POOL_SIZE can be set from a measurement. Allocation
     * happens once at init and never grows, so this figure is final by the first print. */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length,
                              "Mem Pool: %lu/%u bytes\r\n",
                              (unsigned long)sac_get_allocated_bytes(&sac_status), (unsigned)SAC_MEM_POOL_SIZE);
    ASSERT_SAC_STATUS(sac_status);
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

    /* ** Audio fallback statistics ** */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, fallback_stats_str);
    string_length += sac_fallback_format_stats(&main_channel_fallback_instance, stats_string + string_length,
                                               sizeof(stats_string) - string_length, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* ** Wireless statistics ** */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, wireless_stats_str);
    swc_connection_t *connections[] = {rx_audio_conn, tx_data_conn, rx_data_conn};

    for (uint8_t i = 0; i < ARRAY_SIZE(connections); i++) {
        swc_connection_update_stats(connections[i], &swc_err);
        ASSERT_SWC_STATUS(swc_err);
        string_length += swc_connection_format_stats(connections[i], stats_string + string_length,
                                                     sizeof(stats_string) - string_length, &swc_err);
        ASSERT_SWC_STATUS(swc_err);
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

/** @brief One line a second: which rung, and the counters that belong to this role.
 *
 *  The node receives, so rejected packets are its business. rej/s counts packets that arrived and
 *  could not be decoded -- which is what close-range obstruction is expected to produce, since the
 *  direct path is blocked and what reaches the antenna is reflections at different delays. That is
 *  inter-symbol interference, and it is answered by ISI mitigation, not by retransmission: every
 *  retry of a smeared packet is smeared the same way.
 *
 *  Read against the coordinator's cca_fail/s, which counts packets that were never sent at all.
 *  rej climbing while rx holds is the multipath signature; cca_fail climbing is the busy-channel
 *  one. They need different fixes and sound identical.
 *
 *  Rates rather than totals, because a dropout lasts tens of milliseconds and a free-running
 *  counter cannot show one without differencing two lines by eye.
 */
static void print_stats_compact(void)
{
    static uint32_t prev_rx_ok, prev_rej, prev_miss, prev_tick;
    static bool prev_valid;

    char line[144];
    swc_error_t swc_err = SWC_ERR_NONE;
    sac_status_t sac_status = SAC_OK;
    uint32_t now = facade_get_tick_ms();
    swc_fallback_info_t info = swc_connection_get_fallback_info(rx_audio_conn, &swc_err);
    swc_statistics_t *rx = swc_connection_update_stats(rx_audio_conn, &swc_err);
    uint8_t fb_mode = sac_fallback_get_current_mode(&main_channel_fallback_instance, &sac_status);
    uint32_t rx_ok = (rx != NULL) ? rx->packet_successfully_received_count : 0;
    uint32_t rej = (rx != NULL) ? rx->packet_rejected_count : 0;
    uint32_t miss = (rx != NULL) ? rx->no_packet_reception_count : 0;
    uint32_t rx_rate = 0;
    uint32_t rej_rate = 0;
    uint32_t miss_rate = 0;

    if (prev_valid) {
        uint32_t dms = now - prev_tick;

        if (dms > 0) {
            rx_rate = (uint32_t)(((uint64_t)(rx_ok - prev_rx_ok) * 1000U) / dms);
            rej_rate = (uint32_t)(((uint64_t)(rej - prev_rej) * 1000U) / dms);
            miss_rate = (uint32_t)(((uint64_t)(miss - prev_miss) * 1000U) / dms);
        }
    }
    prev_rx_ok = rx_ok;
    prev_rej = rej;
    prev_miss = miss;
    prev_tick = now;
    prev_valid = true;

    uint32_t slots = rx_rate + miss_rate;
    uint32_t fill_pct = (slots > 0) ? (uint32_t)(((uint64_t)rx_rate * 100U) / slots) : 0;

    /* Fraction of this connection's scheduled receive slots that carried a packet.
     *
     * rx + miss is the number of slots the schedule gave this connection, and it was measured
     * equal to the coordinator's transmitted packet count -- 834 + 1964 against tx=2798,
     * exact -- so at a rung where the coordinator fills every slot this IS the arrival rate,
     * and it is available here without correlating two consoles.
     *
     * It is NOT the arrival rate at a rung where the coordinator has slots with nothing to
     * send. Those idle slots are counted as misses here, so the figure reads low: at mode 4
     * the coordinator idles around 1200 slots a second, which turns a real 34% into 19%. Read
     * it at a rung where the DG reports idle=0 -- modes 0 to 2 measured exactly 0 -- or read
     * it only as a relative number between two distances.
     *
     * Integer percent on purpose: this gets compared between distances by eye, and a decimal
     * would imply a precision a one-second window does not have.
     */
    snprintf(line, sizeof(line), "[HS t=%lu] fb=%u %-13s rx=%lu/s rej=%lu/s miss=%lu/s fill=%lu%% lm=%u\r\n", (unsigned long)now,
             (unsigned)fb_mode, fallback_mode_name(fb_mode), (unsigned long)rx_rate, (unsigned long)rej_rate,
             (unsigned long)miss_rate, (unsigned long)fill_pct, (unsigned)info.link_margin);
    facade_print_string(line);
}
#endif /* !STATS_VERBOSE */

/** @brief Print the liveness counters, and the HardFault snapshot if there is one.
 *
 *  This is the role that can be built dual-radio, so the per-radio split earns its place here: one
 *  radio's interrupt and DMA counters freezing while the other keeps ticking is the wedge
 *  signature, and the packet statistics cannot show it -- the connection goes quiet either way.
 *  mrt separates a dead scheduler from radios that are simply not being serviced.
 *
 *  The fault line appears only when there has been a fault. Its absence proves nothing, since a
 *  board stuck in a while(1) faults nothing; its presence proves the failure was a fault rather
 *  than a hang, which is the first thing worth knowing and is invisible from outside.
 */
static void print_diagnostics(void)
{
    char line[160];
    uint32_t r1_irq = 0, r2_irq = 0, r1_dma = 0, r2_dma = 0;
    uint32_t mrt = 0, frt_unused = 0;
    bool irq1 = false, irq2 = false;

    (void)facade_get_radio_hw_counters(&r1_irq, &r2_irq, &r1_dma, &r2_dma);
    /* frt is discarded: it is the same counter facade_get_tick_ms() returns. */
    (void)facade_get_sched_liveness(&mrt, &frt_unused, &irq1, &irq2);

#if STATS_VERBOSE
    snprintf(line, sizeof(line), "Liveness: irq=%lu/%lu dma=%lu/%lu mrt=%lu irq_pin=%d/%d\r\n",
             (unsigned long)r1_irq, (unsigned long)r2_irq, (unsigned long)r1_dma, (unsigned long)r2_dma,
             (unsigned long)mrt, (int)irq1, (int)irq2);
    facade_print_string(line);
#endif


    uint32_t cfsr = 0, hfsr = 0, pc = 0, lr = 0;

    if (facade_get_hardfault_snapshot(&cfsr, &hfsr, &pc, &lr) && ((cfsr | hfsr | pc | lr) != 0)) {
        snprintf(line, sizeof(line), "Fault: cfsr=0x%08lX hfsr=0x%08lX pc=0x%08lX lr=0x%08lX\r\n",
                 (unsigned long)cfsr, (unsigned long)hfsr, (unsigned long)pc, (unsigned long)lr);
        facade_print_string(line);
    }

    /* Raw scheduler-timer state, only when it is not the all-zero a single-radio board always
     * reads: cen=0 means stopped, cen=1 with arr=0 means the period was programmed to zero and
     * the timer stalled, a sane arr with a moving cnt means the timer is not the problem. */
    uint32_t t_cr1 = 0, t_arr = 0, t_cnt = 0, t_dier = 0;

    if (facade_get_multi_radio_timer_regs(&t_cr1, &t_arr, &t_cnt, &t_dier) &&
        ((t_cr1 | t_arr | t_cnt | t_dier) != 0)) {
        snprintf(line, sizeof(line), "Tim4: cen=%lu arr=%lu cnt=%lu uie=%lu (cr1=0x%lX dier=0x%lX)\r\n",
                 (unsigned long)(t_cr1 & 0x1u), (unsigned long)t_arr, (unsigned long)t_cnt,
                 (unsigned long)(t_dier & 0x1u), (unsigned long)t_cr1, (unsigned long)t_dier);
        facade_print_string(line);
    }
}

/** @brief Callback sends the button state and link margin at the DATA_TX_PERIOD_MS interval.
 */
static void data_callback(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    swc_fallback_info_t fallback_info = {0};
    user_data_t transmitted_user_data = {0};

    /* Send the state of the button and link margin to the Coordinator. */
    fallback_info = swc_connection_get_fallback_info(rx_audio_conn, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    transmitted_user_data.link_margin = fallback_info.link_margin;
    transmitted_user_data.button_state = facade_read_button_state();
    wireless_send_data(&transmitted_user_data, sizeof(transmitted_user_data), &swc_err);
}

/** @brief Handle pairing button callback.
 */
static void pairing_button_callback(void)
{
    /* Called from inside the boot-reconnect polling loop: defer, so the connection handles
     * stay valid until the loop has unwound. */
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
        facade_print_error_string("An error occurred during the pairing process.");
        while (1);
    }

    /* Handle the pairing events. */
    switch (pairing_event) {
    case PAIRING_EVENT_SUCCESS:
        /* Indicate that the pairing process was successful. */
        facade_notify_pairing_successful();

        /* Persist before connecting, so a power cut between the two does not lose a pairing
         * the user has already been told succeeded. */
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
        device_pairing_state = DEVICE_UNPAIRED;
        break;
    }
}

/** @brief Has the coordinator been heard from recently?
 *
 *  Measured directly rather than asked of the Wireless Core. A node CAN answer this from
 *  connection status -- it loses sync when the coordinator goes away, unlike a coordinator,
 *  which is handed synced == true unconditionally -- but doing it the same way on both roles
 *  means one mechanism to reason about instead of two that fail differently.
 *
 *  @return true if a packet arrived within COORD_RX_TIMEOUT_MS.
 */
static bool link_is_up(void)
{
    if (!s_coord_rx_seen) {
        return false;
    }

    return (facade_get_tick_ms() - s_coord_rx_tick) < COORD_RX_TIMEOUT_MS;
}

/** @brief Re-establish a persisted pairing without running the pairing procedure.
 *
 *  @return BOOT_RECONNECT_OK    link re-established;
 *          BOOT_RECONNECT_PAIR  no usable record, or the user aborted -- caller pairs;
 *          BOOT_RECONNECT_IDLE  a record existed but the coordinator was not up in time.
 */
static boot_reconnect_result_t try_boot_reconnect(void)
{
    uint32_t start;
    bool connected = false;

    if (!reconnect_store_load(&pairing_assigned_address)) {
        return BOOT_RECONNECT_PAIR;
    }

    if (pairing_assigned_address.node_address == 0) {
        return BOOT_RECONNECT_PAIR;
    }

    /* No discovery list to rebuild on this role: app_swc_core_init() takes both addresses
     * straight out of pairing_assigned_address, which reconnect_store_load() has just
     * filled. The coordinator needs the extra step; this side does not. */

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
        unpair_device(false);
        return BOOT_RECONNECT_PAIR;
    }

    /* Timeout with the core still up: the coordinator just is not on yet. Leave the wireless
     * core running and keep waiting, exactly as the coordinator does.
     *
     * This side could instead fall through to pairing, and that would be wrong in a way that
     * is not obvious: a coordinator that timed out is sitting with its core running and
     * transmitting a schedule -- it is NOT in pairing mode. A node that answered a timeout by
     * entering pairing would therefore never meet it. Whichever device is switched on second
     * has to be able to join the first, and both waiting is what makes that true. Pairing
     * stays reachable through the button. */
    facade_print_string("[BOOT] stored pair did not answer; core left running\r\n");
    return BOOT_RECONNECT_IDLE;
}

/** @brief Unpair the device. This will reset its internal state.
 *
 *  @param[in] forget_peer  true to also erase the persisted pairing address. false tears the
 *                          link down but KEEPS the record, which is what an aborted
 *                          reconnect wants: the user asked to pair now, not to forget who
 *                          they were paired with.
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

    rx_audio_conn = NULL;
    rx_data_conn = NULL;
    tx_data_conn = NULL;

    /* The heartbeat belongs to the connections that just went away. */
    s_coord_rx_seen = false;

    /* Forget the peer only when asked. */
    if (forget_peer) {
        (void)reconnect_store_clear();
    }

    /* Stop the audio pipelines. */
    sac_pipeline_stop(main_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_stop(main_channel_accumulator_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    main_channel_sac_pipeline = NULL;
    main_channel_accumulator_pipeline = NULL;

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

    /* Initialize Audio Core. */
    app_audio_core_init();

    /* Initialize GPIOs and peripherals for audio operations. */
    facade_audio_node_init();

    /* Start the audio pipelines. */
    sac_pipeline_start(main_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_start(main_channel_accumulator_pipeline, &sac_status);
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
