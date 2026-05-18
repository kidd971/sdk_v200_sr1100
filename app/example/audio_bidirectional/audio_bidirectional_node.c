/** @file  audio_bidirectional_node.c
 *  @brief This application creates a bidirectional audio stream at 48kHz/24-bit depth from the audio interface of the
 *         Coordinator to the audio interface of the Node, and at 32kHz/16-bit depth from the Node to the Coordinator.
 *         It utilizes a fallback mechanism that compresses audio using the ADPCM algorithm to preserve quality under
 *         varying conditions.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES ******************************************************************/
#include <stdio.h>
#include "at_cmd_core.h"
#include "audio_bidirectional_facade.h"
#include "pairing_api.h"
#include "pairing_cfg.h"
#include "sac_api.h"
#include "sac_cdc.h"
#include "sac_cfg.h"
#include "sac_compression.h"
#include "sac_endpoint_swc.h"
#include "sac_fallback.h"
#include "sac_fallback_gate.h"
#include "sac_hal_facade.h"
#include "sac_mute_on_underflow.h"
#include "sac_packing.h"
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
#define SAC_MEM_POOL_SIZE 10700
/* Total memory needed for the Wireless Core. */
#define SWC_MEM_POOL_SIZE 10500
/* The data connection supports up to 16 bytes. */
#define MAX_DATA_PAYLOAD_SIZE 16
/* Length of the statistics array used for terminal display. */
#define STATS_ARRAY_LENGTH 4096
/* Period for data transmission timer in ms. */
#define DATA_TX_PERIOD_MS 10
/* Size of the buffer used to print errors. */
#define ERROR_MESSAGE_BUFFER_SIZE 50
/* Interval to print statistics in ms. */
#define PRINT_INTERVAL_MS 1000

/* **** Fallback **** */
/* During fallback mode transfers, 16-bit audio samples are compressed to ADPCM (4-bit). */
#define BACK_CHANNEL_FALLBACK_PAYLOAD_SIZE \
    SAC_CALCULATE_PAYLOAD_SIZE(BACK_CHANNEL_SAMPLE_COUNT, BACK_CHANNEL_CHANNEL_COUNT, SAC_COMPRESSION_SAMPLE_RESOLUTION)
/* A header is added to compressed audio samples during fallback. */
#define BACK_CHANNEL_FALLBACK_HEADER_SIZE \
    (sizeof(sac_header_t) + SAC_COMPRESSION_HEADER_SIZE(BACK_CHANNEL_CHANNEL_COUNT))
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
    /*! Default priority for RX connection. */
    RX_CONNECTION_PRIORITY = 0,
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
    /*! Pending command from node: 0=none, 1=next_track, 2=pre_track. */
    uint8_t cmd_type;
    /*! Battery level of the node (0-100%). */
    uint8_t battery_pct;
} user_data_t;

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
static sac_pipeline_t *back_channel_sac_pipeline;

/* **** Processing Stages **** */
static sac_fallback_instance_t main_channel_fallback_instance;
static sac_processing_t *main_channel_fallback_processing;
static sac_fallback_instance_t back_channel_fallback_instance;
static sac_processing_t *back_channel_fallback_processing;
static sac_packing_instance_t main_channel_unpacking_instance;
static sac_processing_t *main_channel_unpacking_processing;
static sac_compression_instance_t main_channel_decompression_instance;
static sac_processing_t *main_channel_decompression_processing;
static sac_volume_instance_t main_channel_volume_instance;
static sac_processing_t *main_channel_volume_processing;
static sac_processing_t *main_channel_cdc_processing;
static sac_mute_on_underflow_instance_t main_channel_mute_on_underflow_instance;
static sac_processing_t *main_channel_mute_on_underflow_processing;
static sac_packing_instance_t back_channel_packing_instance;
static sac_processing_t *back_channel_packing_processing;
static sac_compression_instance_t back_channel_compression_instance;
static sac_processing_t *back_channel_compression_processing;
static sac_processing_t *back_channel_compression_discard_processing;
static src_cmsis_instance_t back_channel_downsampling_instance;
static sac_processing_t *back_channel_downsampling_processing;

/* **** Endpoints **** */
static sac_endpoint_t *main_channel_consumer_endpoint;
static ep_swc_instance_t main_channel_swc_producer_instance;
static sac_endpoint_t *main_channel_swc_producer_endpoint;
static sac_endpoint_t *back_channel_producer_endpoint;
static ep_swc_instance_t back_channel_swc_consumer_instance;
static sac_endpoint_t *back_channel_swc_consumer_endpoint;

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

/* **** Application Specific **** */
static facade_certification_mode_t certification_mode;
/* Variables supporting pairing between the two devices. */
static device_pairing_state_t device_pairing_state;
static pairing_cfg_t app_pairing_cfg;
static uint8_t s_pending_cmd = 0;
static uint8_t s_battery_pct = 0;
static pairing_assigned_address_t pairing_assigned_address;

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

/* **** Processing Stages **** */
/* Processing stages that are used for the main channel. */
static void app_audio_core_fallback_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_unpacking_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_decompressing_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_volume_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_mute_on_underflow_interface_init(sac_processing_interface_t *iface);
/* Processing stages that are used for the back channel. */
static void app_audio_core_packing_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_downsampling_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_compressing_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_compression_discard_interface_init(sac_processing_interface_t *iface);

/* **** Button Actions **** */
static void volume_up(void);
static void volume_down(void);
static void enter_pairing_mode(void);
static void unpair_device(void);
static void abort_pairing_procedure(void);

/* **** Fallback LED and Terminal Display **** */
static void fallback_led_handler(void);
static bool should_print_stats(void);
static void print_stats(void);

static void wireless_send_data(void *transmitted_data, uint8_t size, swc_error_t *swc_err);
static uint16_t wireless_read_data(void *received_data, uint8_t size, swc_error_t *swc_err);

/* **** AT Command Core Callbacks **** */
static void at_start_pairing(void);
static void at_start_connect(void);
static void at_start_disconnect(void);
static void at_start_shutdown(void);
static bool at_get_link_status(void);
static int32_t at_get_link_margin(void);
static void at_play(void);
static void at_stop(void);
static void at_set_vol(uint8_t vol);
static void at_next_track(void);
static void at_pre_track(void);
static void at_i2s_fmt_change(uint8_t fmt);

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
        .volume_up_callback = volume_up,
        .volume_down_callback = volume_down,
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
    at_cmd_core_register_i2s_fmt_cb(at_i2s_fmt_change);
    at_cmd_core_set_i2s_fmt(facade_get_i2s_fmt());
    at_cmd_core_notify_uwb_ready();

    /* Audio process timer initialization. */
    facade_audio_process_main_channel_timer_init(audio_process_main_channel_callback);
    facade_audio_process_back_channel_timer_init(audio_process_back_channel_callback);

    /* Timer that updates statistics display every second and transmits button state to the Node every 10 ms. */
    facade_data_timer_init(DATA_TX_PERIOD_MS);
    facade_data_timer_set_callback(data_callback);

    certification_mode = facade_get_certification_mode();
    if (certification_mode != FACADE_CERTIF_NONE) {
        /* Init app in certification mode. */
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

    /* Pairing occurs automatically when the device boots. */
    enter_pairing_mode();

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
    uint8_t fallback_thresholds[] = {BACK_CHANNEL_FALLBACK_PAYLOAD_SIZE + BACK_CHANNEL_FALLBACK_HEADER_SIZE};
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

    /* Audio connection concurrency settings. */
    swc_connection_concurrency_cfg_t rx_audio_concurrency_cfg = {
        .enabled = true,
        .try_count = SWC_CCA_AUDIO_FBK_TRY_COUNT, /* Use maximum CCA try count on this connection. */
        .retry_time = SWC_CCA_RETRY_TIME,
        .fail_action = SWC_CCA_ABORT_TX,
    };

    swc_connection_set_concurrency_cfg(rx_audio_conn, &rx_audio_concurrency_cfg, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Audio connection RF channels settings. */
    swc_channel_cfg_t rx_audio_channel_cfg = {
        .tx_pulse_count = TX_ACK_PULSE_COUNT,
        .tx_pulse_width = TX_ACK_PULSE_WIDTH,
        .tx_pulse_gain = TX_ACK_PULSE_GAIN,
        .rx_pulse_count = RX_DATA_PULSE_COUNT,
    };

    for (uint8_t i = 0; i < ARRAY_SIZE(channel_frequency); i++) {
        rx_audio_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(rx_audio_conn, rx_audio_channel_cfg, swc_err);
        ASSERT_SWC_STATUS(*swc_err);
    }

    /* Audio connection priority settings. */
    swc_connection_set_connection_priority(rx_audio_conn, RX_CONNECTION_PRIORITY, swc_err);
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

    /* Data connection concurrency settings. */
    swc_connection_concurrency_cfg_t rx_data_concurrency_cfg = {
        .enabled = true,
        .try_count = SWC_CCA_DATA_TRY_COUNT,
        .retry_time = SWC_CCA_RETRY_TIME,
        .fail_action = SWC_CCA_ABORT_TX,
    };

    swc_connection_set_concurrency_cfg(rx_data_conn, &rx_data_concurrency_cfg, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Data connection RF channels settings. */
    swc_channel_cfg_t rx_data_channel_cfg = {
        .tx_pulse_count = TX_ACK_PULSE_COUNT,
        .tx_pulse_width = TX_ACK_PULSE_WIDTH,
        .tx_pulse_gain = TX_ACK_PULSE_GAIN,
        .rx_pulse_count = RX_DATA_PULSE_COUNT,
    };

    for (uint8_t i = 0; i < ARRAY_SIZE(channel_frequency); i++) {
        rx_data_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(rx_data_conn, rx_data_channel_cfg, swc_err);
        ASSERT_SWC_STATUS(*swc_err);
    }

    /* Data connection priority settings. */
    swc_connection_set_connection_priority(rx_data_conn, RX_CONNECTION_PRIORITY, swc_err);
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

        swc_connection_set_connection_priority(tx_data_conn, AUDIO_CONNECTION_PRIORITY, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

        tx_audio_conn = swc_connection_init(tx_audio_conn_cfg, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

        swc_connection_set_connection_priority(tx_audio_conn, DATA_CONNECTION_PRIORITY, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

    } else {
        if (certification_mode == FACADE_CERTIF_AUDIO_ADPCM) {
            /* Change the connection's max payload size when certifying compressed audio. */
            tx_audio_conn_cfg.max_payload_size = fallback_thresholds[0];
        }
        tx_audio_conn = swc_connection_init(tx_audio_conn_cfg, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

        swc_connection_set_connection_priority(tx_audio_conn, AUDIO_CONNECTION_PRIORITY, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

        tx_data_conn = swc_connection_init(tx_data_conn_cfg, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

        swc_connection_set_connection_priority(tx_data_conn, DATA_CONNECTION_PRIORITY, swc_err);
        ASSERT_SWC_STATUS(*swc_err);
    }

    /* Audio connection concurrency settings. */
    swc_connection_concurrency_cfg_t tx_audio_concurrency_cfg = {
        .enabled = true,
        .try_count = SWC_CCA_AUDIO_TRY_COUNT,
        .retry_time = SWC_CCA_RETRY_TIME,
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
        .tx_pulse_count = TX_AUDIO_PULSE_COUNT,
        .tx_pulse_width = TX_AUDIO_PULSE_WIDTH,
        .tx_pulse_gain = TX_AUDIO_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT,
    };

    swc_fallback_channel_cfg_t tx_audio_fallback_channel_cfg = {
        .tx_pulse_count = TX_AUDIO_FB_PULSE_COUNT,
        .tx_pulse_width = TX_AUDIO_FB_PULSE_WIDTH,
        .tx_pulse_gain = TX_AUDIO_FB_PULSE_GAIN,
    };

    for (uint8_t i = 0; i < ARRAY_SIZE(channel_frequency); i++) {
        tx_audio_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(tx_audio_conn, tx_audio_channel_cfg, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

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
        .try_count = SWC_CCA_DATA_TRY_COUNT,
        .retry_time = SWC_CCA_RETRY_TIME,
        .fail_action = SWC_CCA_ABORT_TX,
    };

    swc_connection_set_concurrency_cfg(tx_data_conn, &tx_data_concurrency_cfg, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Data connection RF channels settings. */
    swc_channel_cfg_t tx_data_channel_cfg = {
        .tx_pulse_count = TX_DATA_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain = TX_DATA_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT,
    };

    for (uint8_t i = 0; i < ARRAY_SIZE(channel_frequency); i++) {
        tx_data_channel_cfg.frequency = channel_frequency[i];
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
     * Input:       Stereo stream of 48kHz/24-bit depth samples is received over the air from the Coordinator.
     * Output:      Stereo stream of 48 kHz/24 bits.
     *
     * +-----+    +-----+
     * | SWC | -> | USB |
     * +-----+    +-----+
     *
     ***** NORMAL MODE I2S (Fallback mode 0) *****
     * Input:       Stereo stream of 48kHz/24-bit depth samples is received over the air from the Coordinator.
     * Processing:  Unpacking from 24 bits to 24 bits encoded on 32 bits audio samples.
     * Processing:  Digital volume control followed by clock drift compensation and mute on glitch.
     * Output:      Stereo stream of 48 kHz/24 bits.
     *
     * +-----+    +-----------+    +----------------+    +-----+    +----------------+    +-----+
     * | SWC | -> | Unpacking | -> | Digital Volume | -> | CDC | -> | Mute on Glitch | -> | I2S |
     * +-----+    +-----------+    +----------------+    +-----+    +----------------+    +-----+
     *
     * ***** FALLBACK MODE USB (Fallback mode 1) *****
     * Input:       Stereo stream of 48kHz/24-bit depth samples is received over the air from the Coordinator.
     * Processing:  Decompression of samples compressed with ADPCM.
     * Output:      Stereo stream of 48 kHz/24 bits.
     *
     * +-----+    +---------------------+    +-----+
     * | SWC | -> | ADPCM Decompressing | -> | USB |
     * +-----+    +---------------------+    +-----+
     *
     ***** FALLBACK MODE I2S (Fallback mode 1) *****
     * Input:       Stereo stream of 48kHz/24-bit depth samples is received over the air from the Coordinator.
     * Processing:  Decompression of samples compressed with ADPCM.
     * Processing:  Digital volume control followed by clock drift compensation and mute on glitch.
     * Output:      Stereo stream of 48 kHz/24 bits.
     *
     * +-----+    +---------------------+    +----------------+    +-----+    +----------------+    +-----+
     * | SWC | -> | ADPCM Decompressing | -> | Digital Volume | -> | CDC | -> | Mute on Glitch | -> | I2S |
     * +-----+    +---------------------+    +----------------+    +-----+    +----------------+    +-----+
     */

    /* Initialize SWC producer endpoint. */
    sac_endpoint_cfg_t main_channel_swc_producer_cfg = {
        .use_encapsulation = true,
        .delayed_action = false,
        .channel_count = MAIN_CHANNEL_CHANNEL_COUNT,
        .audio_payload_size = MAIN_CHANNEL_SWC_PAYLOAD_SIZE,
        .queue_size = SAC_MIN_PRODUCER_QUEUE_SIZE,
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

    /* Processing stage that unpacks 24 bits to 24 bits encoded on 32 bits if fallback is deactivated. */
    main_channel_unpacking_instance.packing_mode = SAC_UNPACK_24BITS;
    main_channel_unpacking_processing = sac_processing_stage_init((void *)&main_channel_unpacking_instance,
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

    /* Initialize codec consumer endpoint. */
    sac_endpoint_cfg_t main_channel_consumer_cfg = {
        .use_encapsulation = false,
        .delayed_action = !USB_AUDIO_ENABLED,
        .channel_count = MAIN_CHANNEL_CHANNEL_COUNT,
        .audio_payload_size = USB_AUDIO_ENABLED ? MAIN_CHANNEL_SWC_PAYLOAD_SIZE : MAIN_CHANNEL_I2S_PAYLOAD_SIZE,
        .queue_size = MAIN_CHANNEL_LATENCY_QUEUE_SIZE,
    };
    main_channel_consumer_endpoint = sac_endpoint_init(NULL, "Audio EP (Consumer)", main_channel_consumer_iface,
                                                       main_channel_consumer_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Initialize audio pipeline. */
    sac_pipeline_cfg_t main_channel_pipeline_cfg = {
        .do_initial_buffering = false,
    };
    main_channel_sac_pipeline = sac_pipeline_init("SWC -> Audio", main_channel_swc_producer_endpoint,
                                                  main_channel_pipeline_cfg, main_channel_consumer_endpoint,
                                                  &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Add processing stages to the audio pipeline. */
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_fallback_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#if !USB_AUDIO_ENABLED
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_unpacking_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_decompression_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#if !USB_AUDIO_ENABLED
    /* 24-bit packed audio volume not supported. */
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_volume_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_cdc_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_mute_on_underflow_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif
    /* Audio pipeline setup. */
    sac_pipeline_setup(main_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Fallback mode configuration. */
    sac_fallback_mode_cfg_t mode_cfg = sac_fallback_mode_get_defaults();
    uint8_t mode_index;

    /* Fallback mode 0 configuration. */
    mode_index = sac_fallback_add_mode(&main_channel_fallback_instance, "48kHz 24-bit", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_unpacking_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Fallback mode 1 configuration. */
    mode_index = sac_fallback_add_mode(&main_channel_fallback_instance, "48kHz ADPCM", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_decompression_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /*
     * Back Channel Audio Pipeline (TX)
     * ================================
     *
     * ***** NORMAL MODE USB (Fallback mode 0) *****
     * Input:       Mono stream of 32kHz/16-bit depth samples.
     * Output:      Mono stream of 32kHz/16 bits is sent over the air to the Node.
     *
     * +-----+    +-----+
     * | USB | -> | SWC |
     * +-----+    +-----+
     *
     ***** NORMAL MODE I2S (Fallback mode 0) *****
     * Input:       Mono stream of 48kHz/24-bit depth samples, encoded on 32 bits.
     * Processing:  Packing from 32 bits to 16 bits audio samples.
     * Processing:  Downsampling audio samples from 48kHz to 32kHz.
     * Output:      Mono stream of 32 kHz/16 bits is sent over the air to the Node.
     *
     * +-----+    +--------------------+    +-------------------+    +-----+
     * | I2S | -> | Packing to 16 bits | -> | Downsampling 1.5x | -> | SWC |
     * +-----+    +--------------------+    +-------------------+    +-----+
     *
     * ***** FALLBACK MODE USB (Fallback mode 1) *****
     * Input:       Mono stream of 32kHz/16-bit depth samples.
     * Processing:  Audio compression using ADPCM.
     * Output:      ADPCM compressed Mono stream of 32 kHz/16 bits is sent over the air to the Node.
     *
     * +-----+    +-------------------+    +-----+
     * | USB | -> | ADPCM Compression | -> | SWC |
     * +-----+    +-------------------+    +-----+
     *
     ***** FALLBACK MODE I2S (Fallback mode 1) *****
     * Input:       Mono stream of 48kHz/24-bit depth samples, encoded on 32 bits.
     * Processing:  Packing from 32 bits to 16 bits audio samples.
     * Processing:  Downsampling audio samples from 48kHz to 32kHz.
     * Processing:  Audio compression using ADPCM.
     * Output:      ADPCM compressed Mono stream of 32 kHz/16 bits is sent over the air to the Node.
     *
     * +-----+    +--------------------+    +-------------------+    +-------------------+    +-----+
     * | I2S | -> | Packing to 16 bits | -> | Downsampling 1.5x | -> | ADPCM Compression | -> | SWC |
     * +-----+    +--------------------+    +-------------------+    +-------------------+    +-----+
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

    back_channel_fallback_instance.connection = tx_audio_conn;
    back_channel_fallback_instance.is_tx_device = true;
    back_channel_fallback_instance.get_tick = facade_get_tick_ms;
    back_channel_fallback_instance.tick_frequency_hz = 1000;
    back_channel_fallback_processing = sac_processing_stage_init(&back_channel_fallback_instance,
                                                                 "Back channel fallback TX", fallback_iface,
                                                                 &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Processing stage that packs 24-bit depth encoded on 32 bits into 16 bits. */
    back_channel_packing_instance.packing_mode = SAC_PACK_24BITS_16BITS;
    back_channel_packing_processing = sac_processing_stage_init((void *)&back_channel_packing_instance, "Audio Packing",
                                                                back_channel_packing_iface, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Processing stage that downsamples the audio samples from 48kHz to 32kHz. */
    back_channel_downsampling_instance.cfg.channel_count = BACK_CHANNEL_CHANNEL_COUNT;
    back_channel_downsampling_instance.cfg.divide_ratio = SAC_SRC_THREE;
    back_channel_downsampling_instance.cfg.multiply_ratio = SAC_SRC_TWO;
    back_channel_downsampling_instance.cfg.payload_size = BACK_CHANNEL_I2S_PAYLOAD_SIZE * BACK_CHANNEL_BIT_DEPTH /
                                                          I2S_DMA_BIT_DEPTH;
    back_channel_downsampling_instance.cfg.input_sample_format = BACK_CHANNEL_SAC_SAMPLE_FORMAT;
    back_channel_downsampling_instance.cfg.output_sample_format = BACK_CHANNEL_SAC_SAMPLE_FORMAT;
    back_channel_downsampling_processing = sac_processing_stage_init((void *)&back_channel_downsampling_instance,
                                                                     "Audio Downsampling",
                                                                     back_channel_downsampling_iface, &sac_status);
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

    /* Initialize SWC consumer endpoint. */
    sac_endpoint_cfg_t back_channel_swc_consumer_cfg = {
        .use_encapsulation = true,
        .delayed_action = false,
        .channel_count = BACK_CHANNEL_CHANNEL_COUNT,
        .audio_payload_size = BACK_CHANNEL_SWC_PAYLOAD_SIZE,
        .queue_size = BACK_CHANNEL_LATENCY_QUEUE_SIZE,
    };
    back_channel_swc_consumer_endpoint = sac_endpoint_init((void *)&back_channel_swc_consumer_instance,
                                                           "SWC EP (Consumer)", back_channel_swc_consumer_iface,
                                                           back_channel_swc_consumer_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Initialize audio pipeline. */
    sac_pipeline_cfg_t back_channel_pipeline_cfg = {
        .do_initial_buffering = true,
    };
    back_channel_sac_pipeline = sac_pipeline_init("Audio -> SWC", back_channel_producer_endpoint,
                                                  back_channel_pipeline_cfg, back_channel_swc_consumer_endpoint,
                                                  &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Add processing stages to the audio pipeline. */
    sac_pipeline_add_processing(back_channel_sac_pipeline, back_channel_fallback_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#if !USB_AUDIO_ENABLED
    sac_pipeline_add_processing(back_channel_sac_pipeline, back_channel_packing_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_add_processing(back_channel_sac_pipeline, back_channel_downsampling_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif
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
    mode_index = sac_fallback_add_mode(&back_channel_fallback_instance, "32kHz 16-bit", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&back_channel_fallback_instance, mode_index,
                                     back_channel_compression_discard_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Fallback mode 1 configuration. */
    mode_index = sac_fallback_add_mode(&back_channel_fallback_instance, "32kHz ADPCM", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&back_channel_fallback_instance, mode_index, back_channel_compression_processing,
                                     &sac_status);
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

/** @brief Update the fallback LED indicator.
 */
static void fallback_led_handler(void)
{
    sac_status_t sac_status = SAC_OK;

    facade_fallback_status(sac_fallback_get_current_mode(&main_channel_fallback_instance, &sac_status));
    ASSERT_SAC_STATUS(sac_status);
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

/** @brief Audio peripheral transfer complete callback.
 *
 *  This feeds the codec with audio packets. It needs to be executed every time a DMA transfer to the codec is completed
 *  in order to keep the audio playing.
 */
static void main_channel_audio_tx_complete_callback(void)
{
    sac_status_t sac_status = SAC_OK;

    if (main_channel_sac_pipeline == NULL) {
        return;
    }
    /* The codec consumes audio samples produced by the SWC (which receives them from the Coordinator). */
    sac_pipeline_consume(main_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
}

/** @brief Audio peripheral receive complete callback.
 *
 *  This receives audio packets from the codec. It needs to be executed every time a DMA transfer from the codec is
 *  completed in order to keep recording audio.
 */
static void back_channel_audio_rx_complete_callback(void)
{
    sac_status_t sac_status = SAC_OK;

    if (back_channel_sac_pipeline == NULL) {
        return;
    }
    /* The codec produces audio samples when it receives input audio. */
    sac_pipeline_produce(back_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Trigger back channel process. */
    facade_audio_process_back_channel_timer_trigger();
}

/** @brief Callback handling the audio process that triggers with the app timer.
 */
static void audio_process_main_channel_callback(void)
{
    sac_status_t sac_status = SAC_OK;
    uint32_t buffer_load = 0;

    if (main_channel_sac_pipeline == NULL) {
        return;
    }
    buffer_load = sac_pipeline_get_producer_buffer_load(main_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    if (buffer_load > 0) {
        /* Processing stages of the back channel pipeline are executed. */
        sac_pipeline_process(main_channel_sac_pipeline, &sac_status);
        ASSERT_SAC_STATUS(sac_status);
    }

#if USB_AUDIO_ENABLED
    buffer_load = sac_pipeline_get_consumer_buffer_load(main_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Consume all nodes into the USB FIFO. */
    while (buffer_load > 0) {
        /* The USB audio consumes audio coming from the SWC rx audio connection. */
        sac_pipeline_consume(main_channel_sac_pipeline, &sac_status);
        ASSERT_SAC_STATUS(sac_status);
        buffer_load--;
    }
#endif
}

/** @brief Callback handling the audio process that triggers with the app timer.
 */
static void audio_process_back_channel_callback(void)
{
    sac_status_t sac_status = SAC_OK;
    uint32_t buffer_load = 0;

    if (back_channel_sac_pipeline == NULL) {
        return;
    }
    buffer_load = sac_pipeline_get_producer_buffer_load(back_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    if (buffer_load > 0) {
        /* Processing stages of the back channel pipeline are executed. */
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

    facade_print_string(stats_string);
}

/** @brief Callback sends the link margin and the button state every 10 ms.
 */
static void data_callback(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    swc_fallback_info_t fallback_info = {0};
    user_data_t transmitted_user_data = {0};

    /* Update the link margin and the button state. */
    fallback_info = swc_connection_get_fallback_info(rx_audio_conn, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

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
        facade_print_error_string("An error occured during the pairing process.");
        while (1);
    }

    /* Handle the pairing events. */
    switch (pairing_event) {
    case PAIRING_EVENT_SUCCESS:
        /* Indicate that the pairing process was successful. */
        facade_notify_pairing_successful();

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

    /* Stop the back channel audio pipeline. */
    sac_pipeline_stop(main_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Stop the main channel audio pipeline. */
    sac_pipeline_stop(back_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    main_channel_sac_pipeline = NULL;
    back_channel_sac_pipeline = NULL;

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

    /* Send the payload through the Wireless Core. */
    swc_connection_send(tx_data_conn, buffer, size, swc_err);
    ASSERT_SWC_STATUS(*swc_err);
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
    facade_audio_node_init();

    /* Start audio pipelines. */
    sac_pipeline_start(main_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_start(back_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Start timers used for audio processes. */
    facade_audio_process_main_channel_timer_start();
    facade_audio_process_back_channel_timer_start();

    /* Start data and statistics timer. */
    facade_data_timer_start();
}

static void at_start_pairing(void)
{
    if (device_pairing_state == DEVICE_PAIRING) {
        return;
    }
    if (device_pairing_state == DEVICE_PAIRED) {
        unpair_device();
        return;
    }
    enter_pairing_mode();
}

static void at_start_connect(void)
{
    if (device_pairing_state == DEVICE_PAIRED) {
        return;
    }
    if (pairing_assigned_address.node_address == 0) {
        return;
    }
    at_cmd_core_set_uwb_conn_status(AT_UWB_CONN_STATUS_CONNECTING);
    app_init();
    device_pairing_state = DEVICE_PAIRED;
}

static void at_start_disconnect(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    sac_status_t sac_status = SAC_OK;

    if (device_pairing_state == DEVICE_UNPAIRED) {
        return;
    }
    device_pairing_state = DEVICE_UNPAIRED;

    facade_audio_process_main_channel_timer_stop();
    facade_audio_process_back_channel_timer_stop();
    facade_data_timer_stop();

    swc_disconnect(&swc_err);
    ASSERT_SWC_STATUS(swc_err);

    tx_audio_conn = NULL;
    rx_audio_conn = NULL;
    tx_data_conn = NULL;
    rx_data_conn = NULL;

    sac_pipeline_stop(main_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_stop(back_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    main_channel_sac_pipeline = NULL;
    back_channel_sac_pipeline = NULL;

    facade_audio_deinit();
    facade_led_all_off();
    at_cmd_core_set_uwb_conn_status(AT_UWB_CONN_STATUS_STANDBY);
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

static void at_i2s_fmt_change(uint8_t fmt)
{
    facade_set_i2s_fmt(fmt);
}

void sac_error_handler(sac_status_t sac_status)
{
    char buffer[ERROR_MESSAGE_BUFFER_SIZE];

    sprintf(buffer, "SAC Error! Code: %d\n\r", sac_status);
    facade_print_error_string(buffer);

    while (1);
}

void swc_error_handler(swc_error_t swc_status)
{
    char buffer[ERROR_MESSAGE_BUFFER_SIZE];

    sprintf(buffer, "SWC Error ! Code: %d\n\r", swc_status);
    facade_print_error_string(buffer);

    while (1);
}
