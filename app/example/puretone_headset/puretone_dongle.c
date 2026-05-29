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
#include "pairing_api.h"
#include "pairing_cfg.h"
#include "puretone_headset_facade.h"
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
#ifdef AUDIO_PRODUCER_SINE_WAVE
#include "sac_sinus_endpoint_96k.h"
#endif
#include "swc_api.h"
#include "swc_cfg.h"
#include "swc_cfg_coord.h"
#include "swc_error.h"
#include "swc_stats.h"

/* CONSTANTS ******************************************************************/
/* Total memory needed for the Audio Core. */
#define SAC_MEM_POOL_SIZE 50000
/* Total memory needed for the Wireless Core. */
#define SWC_MEM_POOL_SIZE 10500
/* The data connection supports up to 16 bytes. */
#define MAX_DATA_PAYLOAD_SIZE 16
/* Length of the statistics array used for terminal display. */
#define STATS_ARRAY_LENGTH 5000
/* Period for data transmission timer in ms. */
#define DATA_TX_PERIOD_MS 10
/* Period for statistics print timer in ms. */
#define STATS_PRINT_PERIOD_MS 1000
/* Size of the buffer used to print errors. */
#define ERROR_MESSAGE_BUFFER_SIZE 50
/* Interval to print statistics in ms. */
#define PRINT_INTERVAL_MS 1000

/* **** CDC **** */
/* Maximum amount of drift to compensate. */
#define MAX_DRIFT_PPM 50

/* Number of SWC fallback modes. */
#define SWC_FALLBACK_MODE_COUNT 3

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

#define MAIN_CHANNEL_PRODUCER_SAC_SAMPLE_FORMAT \
    (USB_AUDIO_ENABLED ? MAIN_CHANNEL_SAC_SAMPLE_FORMAT : I2S_SAC_SAMPLE_FORMAT)

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

static uint32_t timeslot_us[] = SCHEDULE;
static uint32_t channel_sequence[] = CHANNEL_SEQUENCE;
static uint32_t channel_frequency[] = CHANNEL_FREQ;

/* There is a bidirectional link for audio and a bidirectional link for data with a lower connection priority. */
static int32_t tx_timeslots[] = COORD_TIMESLOTS;
static int32_t rx_timeslots[] = NODE_TIMESLOTS;

/* **** Application Specific **** */
static facade_certification_mode_t certification_mode;
static fallback_states_t fallback_state;
/* Variables supporting pairing between the two devices. */
static device_pairing_state_t device_pairing_state;
static pairing_cfg_t app_pairing_cfg;
static pairing_assigned_address_t pairing_assigned_address;
static pairing_discovery_list_t pairing_discovery_list[PAIRING_DISCOVERY_LIST_SIZE];
static sac_cdc_instance_t back_channel_cdc_instance;

/* Fallback latency. */
uint8_t back_channel_fbk_latency_queue_size[] = BACK_CHANNEL_FALLBACK_LATENCY_QUEUE_SIZE;
uint8_t back_channel_fbk_latency_fifo_size[] = BACK_CHANNEL_FALLBACK_LATENCY_FIFO_SIZE;

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
static void volume_up(void);
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

/* **** Button Actions **** */
static void enter_pairing_mode(void);
static void unpair_device(void);
static void abort_pairing_procedure(void);

/* **** Fallback LED and Terminal Display **** */
static bool should_print_stats(void);
static void print_stats(void);

static void wireless_send_data(void *transmitted_data, uint8_t size, swc_error_t *swc_err);
static uint16_t wireless_read_data(void *received_data, uint8_t size, swc_error_t *swc_err);
static uint32_t get_accumulator_size(sac_pipeline_t *pipeline);

/* **** AT Command Core Callbacks **** */
static void at_start_pairing(void);
static void at_start_connect(void);
static void at_start_disconnect(void);
static void at_start_shutdown(void);
static bool at_get_link_status(void);
static int32_t at_get_link_margin(void);
static void at_set_vol(uint8_t vol);
static void at_i2s_fmt_change(uint8_t fmt);

/* PUBLIC FUNCTIONS ***********************************************************/
int main(void)
{
#if USB_AUDIO_ENABLED
    /* Configure usb audio before board initialization. */
    facade_configure_coord_usb_audio();
#endif
    /* Initialize the board and all GPIOs and peripherals for minimal operations. */
    facade_board_init();

    /* Initialize wireless core context switch handler before pairing is available */
    facade_set_context_switch_handler(swc_connection_callbacks_processing_handler);

    facade_button_callbacks_t button_callbacks = {
        .pairing_callback = pairing_button_callback,
        .volume_up_callback = volume_up,
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
    at_cmd_core_register_vol_cb(at_set_vol);
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

    certification_mode = facade_coord_get_certification_mode();
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
    uint8_t fallback_thresholds[] = MAIN_CHANNEL_FALLBACK_PAYLOAD_SIZE;
    uint8_t fallback_cca_try_count[] = {SWC_CCA_AUDIO_FBK_1_TRY_COUNT, SWC_CCA_AUDIO_FBK_2_TRY_COUNT,
                                        SWC_CCA_AUDIO_FBK_3_TRY_COUNT};

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
        .role = SWC_ROLE_COORDINATOR,
        .pan_id = app_pairing->pan_id,
        .coordinator_address = app_pairing->coordinator_address,
        .local_address = local_address,
    };
    swc_init(core_cfg, node_cfg, facade_context_switch_trigger, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Initialize radio. */
    swc_radio_module_init(SWC_RADIO_ID_1, true, swc_err);
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
    swc_connection_concurrency_cfg_t tx_audio_concurrency_cfg = {
        .enabled = true,
        .try_count = MAIN_CHANNEL_SWC_CCA_AUDIO_TRY_COUNT,
        .retry_time = MAIN_CHANNEL_SWC_CCA_AUDIO_RETRY_TIME,
        .fail_action = SWC_CCA_ABORT_TX,
    };

    swc_connection_set_concurrency_cfg(tx_audio_conn, &tx_audio_concurrency_cfg, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Audio connection fallback settings. */
    swc_connection_fallback_cfg_t fallback_cfg = {
        .enabled = true,
        .fallback_mode_count = SWC_FALLBACK_MODE_COUNT,
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
    uint8_t tx_audio_fb_pulse_width[][MAX_CHANNEL_NUMBER] = {
        TX_AUDIO_FB_BAND_1_PULSE_WIDTH,
        TX_AUDIO_FB_BAND_2_PULSE_WIDTH,
        TX_AUDIO_FB_BAND_3_PULSE_WIDTH,
        TX_AUDIO_FB_BAND_4_PULSE_WIDTH,
    };

    uint8_t tx_audio_fb_pulse_gain[][MAX_CHANNEL_NUMBER] = {
        TX_AUDIO_FB_BAND_1_PULSE_GAIN,
        TX_AUDIO_FB_BAND_2_PULSE_GAIN,
        TX_AUDIO_FB_BAND_3_PULSE_GAIN,
        TX_AUDIO_FB_BAND_4_PULSE_GAIN,
    };

    for (uint8_t i = 0; i < ARRAY_SIZE(channel_frequency); i++) {
        tx_audio_channel_cfg.frequency = channel_frequency[i];
        tx_audio_channel_cfg.tx_pulse_width = tx_audio_pulse_width[i];
        tx_audio_channel_cfg.tx_pulse_gain = tx_audio_pulse_gain[i];
        swc_connection_add_channel(tx_audio_conn, tx_audio_channel_cfg, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

        for (uint8_t j = 0; j < SWC_FALLBACK_MODE_COUNT; j++) {
            tx_audio_fallback_channel_cfg.tx_pulse_width = tx_audio_fb_pulse_width[i][j];
            tx_audio_fallback_channel_cfg.tx_pulse_gain = tx_audio_fb_pulse_gain[i][j];
            swc_connection_add_fallback_channel(tx_audio_conn, tx_audio_channel_cfg, tx_audio_fallback_channel_cfg, i,
                                                j, swc_err);
        }

        ASSERT_SWC_STATUS(*swc_err);
    }

    /* Audio connection callback settings. */
    swc_connection_set_tx_success_callback(tx_audio_conn, conn_tx_audio_success_callback, NULL, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Data connection concurrency settings. */
    swc_connection_concurrency_cfg_t tx_data_concurrency_cfg = {
        .enabled = true,
        .try_count = MAIN_CHANNEL_SWC_CCA_DATA_TRY_COUNT,
        .retry_time = MAIN_CHANNEL_SWC_CCA_DATA_RETRY_TIME,
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

    /* Data connection callback settings. */
    swc_connection_set_tx_success_callback(tx_data_conn, conn_tx_data_success_callback, NULL, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* **** RX Connections **** */
    /* ** Back Channel: RX Audio Connection ** */
    swc_connection_cfg_t rx_audio_conn_cfg = {
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
    swc_connection_concurrency_cfg_t rx_audio_concurrency_cfg = {
        .enabled = true,
        .try_count = BACK_CHANNEL_SWC_CCA_FB_TRY_COUNT, /* Use maximum CCA try count on this connection. */
        .retry_time = BACK_CHANNEL_SWC_CCA_AUDIO_RETRY_TIME,
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
        .try_count = BACK_CHANNEL_SWC_CCA_DATA_TRY_COUNT,
        .retry_time = BACK_CHANNEL_SWC_CCA_DATA_RETRY_TIME,
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

        /* Forward commands from node to SOC via UART. */
        if (received_user_data.cmd_type == 1) {
            at_cmd_core_notify_next_track_received();
        } else if (received_user_data.cmd_type == 2) {
            at_cmd_core_notify_pre_track_received();
        } else if (received_user_data.cmd_type == 3) {
            at_cmd_core_notify_play_received();
        } else if (received_user_data.cmd_type == 4) {
            at_cmd_core_notify_stop_received();
        }

        /* Cache battery level reported by node. */
        at_cmd_core_set_battery_level(received_user_data.battery_pct);
    }
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
#ifdef AUDIO_PRODUCER_SINE_WAVE
    /* Sine wave mode: back channel consumer (headphone output) still uses I2S; producer is internal. */
    sac_facade_audio_endpoint_init(NULL, &back_channel_consumer_iface);
    main_channel_producer_iface.action = ep_sinus_96k_produce;
    main_channel_producer_iface.start  = ep_sinus_96k_start;
    main_channel_producer_iface.stop   = ep_sinus_96k_stop;
    facade_set_audio_complete_callback(back_channel_audio_tx_complete_callback, NULL);
#else
    sac_facade_audio_endpoint_init(&main_channel_producer_iface, &back_channel_consumer_iface);
    facade_set_audio_complete_callback(back_channel_audio_tx_complete_callback,
                                       main_channel_audio_rx_complete_callback);
#endif

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
     ***** NORMAL MODE USB (Fallback mode 0) *****
     * Input:       Stereo stream of 96kHz/24-bit depth samples.
     * Output:      Stereo stream of 96kHz/24-bit is sent over the air to the Node.
     * +-----+    +-----+
     * | USB | -> | SWC |
     * +-----+    +-----+
     *
     ***** NORMAL MODE I2S (Fallback mode 0) *****
     * Input:       Stereo stream of 96kHz/24-bit depth samples, encoded on 32 bits.
     * Processing:  Packing from 32 bits to 24 bits audio samples.
     * Output:      Stereo stream of 96kHz/24-bit is sent over the air to the Node.
     *
     * +-----+    +--------------------+    +-----+
     * | I2S | -> | Packing to 24 bits | -> | SWC |
     * +-----+    +--------------------+    +-----+
     *
     ***** FALLBACK MODE USB (Fallback mode 1) *****
     * Input:       Stereo stream of 96kHz/24-bit packed depth samples.
     * Processing:  Audio sample accumulator 1.7x.
     * Processing:  Downsampling audio samples from 96kHz to 48kHz.
     * Output:      Stereo stream of 48kHz/24-bit is sent over the air to the Node.
     *
     * +-----+    +------------------+    +-----------------+    +-----+
     * | USB | -> | Accumulator 1.7x | -> | Downsampling 2x | -> | SWC |
     * +-----+    +------------------+    +-----------------+    +-----+
     *
     ***** FALLBACK MODE I2S (Fallback mode 1) *****
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
     ***** FALLBACK MODE USB (Fallback mode 2) *****
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
     ***** FALLBACK MODE I2S (Fallback mode 2) *****
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
     ***** FALLBACK MODE USB (Fallback mode 3) *****
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
     ***** FALLBACK MODE I2S (Fallback mode 3) *****
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
#ifdef AUDIO_PRODUCER_SINE_WAVE
        .delayed_action = false,
#else
        .delayed_action = !USB_AUDIO_ENABLED,
#endif
        .channel_count = MAIN_CHANNEL_CHANNEL_COUNT,
        .audio_payload_size = USB_AUDIO_ENABLED ? MAIN_CHANNEL_SWC_PAYLOAD_SIZE : MAIN_CHANNEL_I2S_PAYLOAD_SIZE,
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
    main_channel_downsampling_instance.cfg.payload_size = USB_AUDIO_ENABLED ? MAIN_CHANNEL_SWC_PAYLOAD_SIZE :
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

    /* Processing stage packs into 24 bits before sending if fallback is deactivated. */
    main_channel_packing_instance.packing_mode = SAC_PACK_24BITS;
    main_channel_packing_processing = sac_processing_stage_init((void *)&main_channel_packing_instance, "Audio Packing",
                                                                main_channel_packing_iface, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Processing stage that unpacks the received audio samples from 16 bits to 24 bits. */
    if (USB_AUDIO_ENABLED) {
        main_channel_fbk_packing_instance.packing_mode = SAC_SCALE_24BITS_16BITS;
    } else {
        main_channel_fbk_packing_instance.packing_mode = SAC_PACK_24BITS_16BITS;
    }
    main_channel_fbk_packing_processing = sac_processing_stage_init((void *)&main_channel_fbk_packing_instance,
                                                                    "Audio Unpacking", main_channel_packing_iface,
                                                                    &sac_status);
    ASSERT_SAC_STATUS(sac_status);

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
#if !USB_AUDIO_ENABLED
    /* When using I2S, packing is required to convert 24-bit audio aligned on 32-bit words. */
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_packing_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_compression_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_compression_discard_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_pipeline_add_processing(main_channel_sac_pipeline, main_channel_fbk_packing_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
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
#if !USB_AUDIO_ENABLED
    /* When using I2S, packing is required to convert 24-bit audio aligned on 32-bit words. */
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_packing_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif

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
#if !USB_AUDIO_ENABLED
    /* When using I2S, packing is required to convert 24-bit audio aligned on 32-bit words. */
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_packing_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif

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
    sac_fallback_mode_assign_process(&main_channel_fallback_instance, mode_index, main_channel_fbk_packing_processing,
                                     &sac_status);
    ASSERT_SAC_STATUS(sac_status);
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

    /*
     * Back Channel Audio Pipeline (RX)
     * ================================
     *
     ****** NORMAL MODE USB (Fallback mode 0) *****
     * Input:       Mono stream of 48kHz/16-bit depth samples is received over the air from the Node.
     * Processing:  Audio sample accumulator 1.7647x.
     * Processing:  Digital volume control.
     * Output:      Mono stream of 48kHz/16-bit.
     *
     * +-----+    +---------------------+    +----------------+    +-----+
     * | SWC | -> | Accumulator 1.7647x | -> | Digital Volume | -> | USB |
     * +-----+    +---------------------+    +----------------+    +-----+
     *
     ***** NORMAL MODE I2S (Fallback mode 0) *****
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
     ***** FALLBACK MODE (Fallback mode 1) *****
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
    mode_index = sac_fallback_add_mode(&back_channel_fallback_instance, "48kHz 16-bit", mode_cfg, &sac_status);
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
 *  This affects the audio pipeline that the digital volume processing stage is added to.
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
 *  This affects the audio pipeline that the digital volume processing stage is added to.
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
 *  This receives audio packets from the codec. It needs to be executed every time a DMA transfer from the codec is
 *  completed in order to keep recording audio.
 */
static void main_channel_audio_rx_complete_callback(void)
{
    sac_status_t sac_status = SAC_OK;

    /* The codec produces audio samples when it receives input audio. */
    sac_pipeline_produce(main_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Trigger main channel process. */
    facade_audio_process_main_channel_timer_trigger();
}

#if USB_AUDIO_ENABLED
/** @brief SAI DMA TX complete callback.
 *
 *  This feeds the codec with audio packets. It needs to be executed every time a DMA transfer to the codec is completed
 *  in order to keep the audio playing.
 */
static void back_channel_audio_tx_complete_callback(void)
{
    sac_status_t sac_status = SAC_OK;
    uint32_t target_fifo_size;
    uint32_t usb_buf_rem = facade_app_audio_usb_get_epin_fifo_remaining();

    target_fifo_size =
        back_channel_fbk_latency_fifo_size[sac_fallback_get_current_mode(&back_channel_fallback_instance, &sac_status)];
    facade_app_audio_usb_set_epin_target_fifo_size(target_fifo_size);

    if ((sac_pipeline_get_consumer_buffer_load(back_channel_accumulator_pipeline, &sac_status) > 0) &&
        ((usb_buf_rem / BACK_CHANNEL_SWC_PAYLOAD_SIZE) > 1)) {
        sac_pipeline_consume(back_channel_accumulator_pipeline, &sac_status);
    }
}
#else
/** @brief Back channel audio TX complete callback.
 *
 *  This feeds the codec with audio packets. It needs to be executed every time a DMA transfer to the codec is
 *  completed in order to keep the audio playing.
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

#ifdef AUDIO_PRODUCER_SINE_WAVE
    /* No DMA callback drives production; generate one packet on each timer invocation. */
    sac_pipeline_produce(main_channel_sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif

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
    swc_connection_update_stats(tx_audio_conn, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    string_length += swc_connection_format_stats(tx_audio_conn, stats_string + string_length,
                                                 sizeof(stats_string) - string_length, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    swc_connection_update_stats(rx_audio_conn, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    string_length += swc_connection_format_stats(rx_audio_conn, stats_string + string_length,
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

    /* ** APP Statistics ** */
    string_length = snprintf(stats_string, sizeof(stats_string), "\r\n<< Application Statistics >>\r\n");
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, "Fallback State:");
    if (fallback_state == FALLBACK_AUTO) {
        string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, " AUTO\r\n");
    } else if (fallback_state == FALLBACK_96K_24BIT_UNCOMPRESSED) {
        string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length,
                                  " 96kHz 24-bit\r\n");
    } else if (fallback_state == FALLBACK_48K_24BIT_UNCOMPRESSED) {
        string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length,
                                  " 48kHz 24-bit\r\n");
    } else if (fallback_state == FALLBACK_48K_16BIT_UNCOMPRESSED) {
        string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length,
                                  " 48kHz 16-bit\r\n");
    } else if (fallback_state == FALLBACK_48K_ADPCM_STEREO) {
        string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length,
                                  " 48kHz ADPCM\r\n");
    }

    facade_print_string(stats_string);
}

/** @brief Callback sends the button state every 10 ms.
 */
static void data_callback(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    swc_fallback_info_t fallback_info = {0};
    user_data_t transmitted_user_data = {0};

    /* Update the link margin. */
    fallback_info = swc_connection_get_fallback_info(rx_audio_conn, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    /* Send the button state and the link margin to the Node. */
    transmitted_user_data.link_margin = fallback_info.link_margin;
    transmitted_user_data.button_state = facade_read_button_state();
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
    pairing_event = pairing_coordinator_start(&app_pairing_cfg, &pairing_assigned_address, pairing_discovery_list,
                                              PAIRING_DISCOVERY_LIST_SIZE, &pairing_err);
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

/** @brief Unpair the device. This will reset its discovery list.
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

    /* Reset the pairing discovery list. */
    memset(pairing_discovery_list, 0, sizeof(pairing_discovery_list));

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

    at_cmd_core_set_device_address(pairing_discovery_list[PAIRING_DEVICE_ROLE_COORDINATOR].node_address);
    at_cmd_core_set_uwb_conn_status(AT_UWB_CONN_STATUS_CONNECTED);

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
#ifdef AUDIO_PRODUCER_SINE_WAVE
    /* Reconfigure main channel timer to the audio packet period before starting. */
    facade_audio_sine_wave_timer_reconfig();
#endif
    facade_audio_process_main_channel_timer_start();
    facade_audio_process_back_channel_timer_start();
#ifdef AUDIO_PRODUCER_SINE_WAVE
    /* Sine wave mode has no DMA RX callback to fire the first produce; trigger manually. */
    facade_audio_process_main_channel_timer_trigger();
#endif

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
    if (pairing_discovery_list[PAIRING_DEVICE_ROLE_COORDINATOR].node_address == 0) {
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
    sac_pipeline_stop(back_channel_accumulator_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    main_channel_sac_pipeline = NULL;
    back_channel_sac_pipeline = NULL;
    back_channel_accumulator_pipeline = NULL;

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
    fallback_state = (fallback_state + 1) % FALLBACK_STATE_COUNT;

    sac_fallback_set_manual_mode(&main_channel_fallback_instance, (fallback_state > FALLBACK_AUTO), NULL);

    if (fallback_state > FALLBACK_AUTO) {
        sac_fallback_set_current_mode(&main_channel_fallback_instance, (fallback_state - 1), NULL);
    }
}
