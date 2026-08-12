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
#include "swc_cfg.h"
#include "swc_cfg_coord.h"
#include "swc_error.h"
#include "swc_stats.h"
#include "swc_utils.h"

/* CONSTANTS ******************************************************************/
/* Total memory needed for the Audio Core. */
#define SAC_MEM_POOL_SIZE 35000
/* Total memory needed for the Wireless Core. */
#define SWC_MEM_POOL_SIZE 10500
/* The data connection supports up to 16 bytes. */
#define MAX_DATA_PAYLOAD_SIZE 16
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
#define PRINT_INTERVAL_MS 1000

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
static void unpair_device(void);
static void abort_pairing_procedure(void);

/* Fallback LED and terminal display. */
static void fallback_led_handler(void);
static bool should_print_stats(void);
static void print_stats(void);

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

    /* Pairing occurs automatically when the device boots. */
    enter_pairing_mode();

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

    /* Initialize the radio. */
    swc_radio_module_init(radio_handle, false, swc_err);
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

    facade_tx_audio_conn_status();

    /* Trigger audio process. */
    facade_audio_process_timer_trigger();
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

    /** Start fallback in best quality.
     *
     *  sac_fallback_add_mode sets current_mode to the last added mode, so an explicit
     *  set to mode 0 is required to ensure we start at the highest quality.
     */
    sac_fallback_set_current_mode(&sac_fallback_instance, 0, &sac_status);
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
static void fallback_led_handler(void)
{
    sac_status_t sac_status = SAC_OK;

    facade_fallback_status(sac_fallback_get_current_mode(&sac_fallback_instance, &sac_status));
    ASSERT_SAC_STATUS(sac_status);
}

/** @brief Audio peripheral receive complete callback.
 *
 *  @note This receives audio packets from the codec. It needs to be executed every time a DMA transfer from the codec
 *        is completed in order to keep recording audio.
 */
static void audio_rx_complete_callback(void)
{
    sac_status_t sac_status = SAC_OK;

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
}

/** @brief Callback sends the button state at the DATA_TX_PERIOD_MS interval.
 */
static void data_callback(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    user_data_t transmitted_user_data = {0};

    /* Send the state of the button to the Node (The Link margin is not used). */
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
