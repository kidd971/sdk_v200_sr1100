/** @file  audio_unidirectional_coord.c
 *  @brief This application creates a unidirectional audio stream at 48kHz/24-bit depth from the I2S interface of the
 *         Coordinator to the I2S of the Node. It includes a fallback to 16 bits to ensure audio quality under varying
 *         conditions. Additionally, there is a bidirectional link for user data and link margin, which supports dynamic
 *         fallback updates.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES ******************************************************************/
#include <stdio.h>
#include "audio_unidirectional_facade.h"
#include "pairing_api.h"
#include "sac_api.h"
#include "sac_fallback.h"
#include "sac_fallback_gate.h"
#include "sac_endpoint_swc.h"
#include "sac_hal_facade.h"
#include "sac_packing.h"
#include "sac_stats.h"
#include "swc_api.h"
#include "swc_cfg_coord.h"
#include "swc_stats.h"

/* CONSTANTS ******************************************************************/
/* Total memory needed for the Audio Core. */
#define SAC_MEM_POOL_SIZE 6000
/* Audio sample size. */
#define SAC_AUDIO_SAMPLE_SIZE 3
/* Number of audio channels.. */
#define SAC_AUDIO_CHANNEL_COUNT 2
/* The SWC consumes audio samples and sends them over the air. */
#define SAC_CONSUMER_AUDIO_PAYLOAD_SIZE 78
/* The codec produces audio samples. */
#define SAC_PRODUCER_AUDIO_PAYLOAD_SIZE 104
/* Size of the latency queue used for the Audio Core. */
#define SAC_LATENCY_QUEUE_SIZE 11
/* Total memory needed for the Wireless Core. */
#define SWC_MEM_POOL_SIZE 10000
/* The data connection supports up to 16 bytes. */
#define MAX_DATA_PAYLOAD_SIZE 16
/* Length of the statistics array used for terminal display. */
#define STATS_ARRAY_LENGTH 3000
/* Period for audio processing timer in us. */
#define AUDIO_PROCESS_PERIOD_US 100
/* Period for data transmission timer in ms. */
#define DATA_TX_PERIOD_MS 10
/* Period for statistics print timer in ms. */
#define STATS_PRINT_PERIOD_MS 1000
/* **** Fallback **** */
/* Audio sample size in fallback. */
#define FALLBACK_AUDIO_SAMPLE_SIZE 2
/* During fallback mode transfers, audio samples are packed into 16 bits instead of 24 bits. */
#define FALLBACK_AUDIO_PAYLOAD_SIZE \
    ((SAC_CONSUMER_AUDIO_PAYLOAD_SIZE * FALLBACK_AUDIO_SAMPLE_SIZE) / SAC_AUDIO_SAMPLE_SIZE)
/* Number of attempts for CCA during fallback mode. */
#define FALLBACK_CCA_TRY_COUNT 3

/* TYPES **********************************************************************/
/** @brief Enumeration representing device pairing states.
 */
typedef enum device_pairing_state {
    /*! The device is unpaired with the Node. */
    DEVICE_UNPAIRED,
    /*! The device is paired with the Node. */
    DEVICE_PAIRED,
} device_pairing_state_t;

/** @brief Data used for sending and receiving link margin and button state.
 */
typedef struct user_data {
    /*! A boolean indicating the button's state, toggling with each press of SW2. */
    bool button_state;
    /*! The link margin to monitor link quality. */
    uint8_t link_margin;
} user_data_t;

/* PRIVATE GLOBALS ************************************************************/
/* **** Audio Core **** */
static uint8_t audio_memory_pool[SAC_MEM_POOL_SIZE];
static sac_hal_t sac_hal;
static sac_pipeline_t *sac_pipeline;

/* **** Processing Stages **** */
static sac_fallback_instance_t sac_fallback_instance;
static sac_processing_t *sac_fallback_processing;
static sac_packing_instance_t audio_packing_instance;
static sac_processing_t *sac_packing_processing;
static sac_packing_instance_t audio_packing_fallback_instance;
static sac_processing_t *sac_packing_fallback_processing;

/* **** Endpoints **** */
static sac_endpoint_t *i2s_producer;
static ep_swc_instance_t swc_consumer_instance;
static sac_endpoint_t *swc_consumer;

/* **** Wireless Core **** */
static uint8_t swc_memory_pool[SWC_MEM_POOL_SIZE];
static swc_node_t *node;

static uint32_t timeslot_us[] = SCHEDULE;
static uint32_t channel_sequence[] = CHANNEL_SEQUENCE;
static uint32_t channel_frequency[] = CHANNEL_FREQ;

static int32_t tx_audio_timeslots[] = TX_AUDIO_TIMESLOTS;
static int32_t tx_data_timeslots[] = TX_DATA_TIMESLOTS;
static int32_t rx_data_timeslots[] = RX_DATA_TIMESLOTS;

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
/* Flag that allows display every second. */
static volatile bool print_stats_now;
/* Variable that stores the link margin and the button state received from the Node. */
static user_data_t received_user_data = {0};
/* Variable that stores the button state to be sent to the Node. (The Link margin is not used) */
static user_data_t transmitted_user_data = {0};

/* PRIVATE FUNCTION PROTOTYPE *************************************************/
static void app_init(void);
static void app_swc_core_init(pairing_assigned_address_t *pairing_assigned_address, swc_error_t *swc_err);
static void app_audio_core_init(void);

/* **** Callbacks **** */
static void conn_tx_audio_success_callback(void *conn);
static void conn_tx_data_success_callback(void *conn);
static void conn_rx_data_success_callback(void *conn);
static void i2s_rx_audio_complete_callback(void);
static void audio_process_callback(void);
static void data_callback(void);
static void pairing_process_callback(void);

/* **** Processing stages **** */
static void app_audio_core_fallback_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_packing_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_packing_fallback_interface_init(sac_processing_interface_t *iface);

/* **** Button actions **** */
static void toggle_button_state(void);
static void enter_pairing_mode(void);
static void unpair_device(void);
static void abort_pairing_procedure(void);

/* Fallback LED and terminal display. */
static void fallback_led_handler(void);
static void print_stats(void);

static void wireless_send_data(void *transmitted_data, uint8_t size, swc_error_t *swc_err);
static uint16_t wireless_read_data(void *received_data, uint8_t size, swc_error_t *swc_err);

/* **** Status Handling **** */
static void status_handler_sac(sac_status_t sac_status);

/* PUBLIC FUNCTIONS ***********************************************************/
int main(void)
{
    /* Initialize the board and all GPIOs and peripherals for minimal operations. */
    facade_board_init();

    /* Timer that triggers the audio process every 100 us. */
    facade_audio_process_timer_init(AUDIO_PROCESS_PERIOD_US);
    facade_audio_process_set_timer_callback(audio_process_callback);

    /* Timer that updates statistics display every second and transmits button state to Node every 10 ms. */
    facade_data_timer_init(DATA_TX_PERIOD_MS);
    facade_data_timer_set_callback(data_callback);

    /* Initialize wireless core context switch handler before pairing is available */
    facade_set_context_switch_handler(swc_connection_callbacks_processing_handler);

    certification_mode = facade_get_coord_certification_mode();
    if (certification_mode > FACADE_CERTIF_NONE) {
        /* Init app in certification mode. */
        app_init();
        while (1) {
            /* Statistics are displayed at intervals set by the timer when paired; timer stops if unpaired. */
            if (print_stats_now) {
                print_stats();
                print_stats_now = false;
            }
        };
    }

    device_pairing_state = DEVICE_UNPAIRED;

    while (1) {
        if (device_pairing_state == DEVICE_UNPAIRED) {
            /* When the device is unpaired, the only action possible for the user is to enter pairing mode. */
            facade_button_handling(enter_pairing_mode, NULL, NULL, NULL);
        } else if (device_pairing_state == DEVICE_PAIRED) {
            /* When the device is paired, normal operations are executed. */
            fallback_led_handler();
            facade_button_handling(unpair_device, toggle_button_state, NULL, NULL);
        }

        /* Statistics are displayed at intervals set by the timer when paired; timer stops if unpaired. */
        if (print_stats_now) {
            print_stats();
            print_stats_now = false;
        }
    }

    return 0;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize the Wireless Core.
 *
 *  @param[in]  pairing_assigned_address  Addresses received from the pairing process.
 *  @param[out] swc_err                   Wireless Core error code.
 */
static void app_swc_core_init(pairing_assigned_address_t *pairing_assigned_address, swc_error_t *swc_err)
{
    uint8_t remote_address = pairing_discovery_list[DEVICE_ROLE_NODE].node_address;
    uint8_t local_address = pairing_discovery_list[DEVICE_ROLE_COORDINATOR].node_address;
    uint8_t fallback_thresholds[] = {FALLBACK_AUDIO_PAYLOAD_SIZE + sizeof(sac_header_t)};
    uint8_t fallback_cca_try_count[] = {FALLBACK_CCA_TRY_COUNT};

    if (certification_mode > FACADE_CERTIF_NONE) {
        pairing_assigned_address->coordinator_address = 0x1;
        pairing_assigned_address->node_address = 0x2;
        pairing_assigned_address->pan_id = 0xABC;
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
    swc_init(core_cfg, facade_context_switch_trigger, swc_err);
    if (*swc_err != SWC_ERR_NONE) {
        return;
    }

    /* Initialize Node. */
    swc_node_cfg_t node_cfg = {
        .role = NETWORK_ROLE,
        .pan_id = pairing_assigned_address->pan_id,
        .coordinator_address = pairing_assigned_address->coordinator_address,
        .local_address = local_address,
    };
    node = swc_node_init(node_cfg, swc_err);
    if (*swc_err != SWC_ERR_NONE) {
        return;
    }

    swc_radio_module_init(node, SWC_RADIO_ID_1, true, swc_err);
    if (*swc_err != SWC_ERR_NONE) {
        return;
    }

    /* ** TX AUDIO Connection ** */
    swc_connection_cfg_t tx_audio_conn_cfg = {
        .name = "TX Audio Connection",
        .source_address = local_address,
        .destination_address = remote_address,
        .max_payload_size = SAC_CONSUMER_AUDIO_PAYLOAD_SIZE + sizeof(sac_header_t),
        .queue_size = TX_DATA_QUEUE_SIZE,
        .timeslot_id = tx_audio_timeslots,
        .timeslot_count = ARRAY_SIZE(tx_audio_timeslots),
    };
    if (certification_mode == FACADE_CERTIF_AUDIO_16_BIT) {
        /* Change the connection's max payload size when certifying 16-bit audio. */
        tx_audio_conn_cfg.max_payload_size = fallback_thresholds[0];
    }
    tx_audio_conn = swc_connection_init(node, tx_audio_conn_cfg, swc_err);
    if (*swc_err != SWC_ERR_NONE) {
        return;
    }

    swc_connection_concurrency_cfg_t tx_audio_concurrency_cfg = {
        .enabled = true,
        .try_count = 3,
        .retry_time = 512, /* 25 us */
        .fail_action = SWC_CCA_ABORT_TX,
    };

    swc_connection_set_concurrency_cfg(tx_audio_conn, &tx_audio_concurrency_cfg, swc_err);
    if (*swc_err != SWC_ERR_NONE) {
        return;
    }

    swc_connection_fallback_cfg_t fallback_cfg = {
        .enabled = true,
        .fallback_mode_count = 1,
        .thresholds = fallback_thresholds,
        .cca_try_count = fallback_cca_try_count,
    };

    swc_connection_set_fallback_cfg(tx_audio_conn, &fallback_cfg, swc_err);
    if (*swc_err != SWC_ERR_NONE) {
        return;
    }

    swc_channel_cfg_t tx_audio_channel_cfg = {
        .tx_pulse_count = TX_DATA_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain = TX_DATA_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT,
    };

    swc_fallback_channel_cfg_t tx_audio_fallback_channel_cfg = {
        .tx_pulse_count = TX_DATA_FB_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_FB_PULSE_WIDTH,
        .tx_pulse_gain = TX_DATA_FB_PULSE_GAIN,
    };

    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++) {
        tx_audio_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(tx_audio_conn, node, tx_audio_channel_cfg, swc_err);
        if (*swc_err != SWC_ERR_NONE) {
            return;
        }

        swc_connection_add_fallback_channel(tx_audio_conn, node, tx_audio_channel_cfg, tx_audio_fallback_channel_cfg, i,
                                            0, swc_err);
        if (*swc_err != SWC_ERR_NONE) {
            return;
        }
    }
    swc_connection_set_tx_success_callback(tx_audio_conn, conn_tx_audio_success_callback, swc_err);
    if (*swc_err != SWC_ERR_NONE) {
        return;
    }

    /* ** TX Data Connection ** */
    swc_connection_cfg_t tx_data_conn_cfg = {
        .name = "TX Data Connection",
        .source_address = local_address,
        .destination_address = remote_address,
        .max_payload_size = MAX_DATA_PAYLOAD_SIZE,
        .queue_size = TX_DATA_QUEUE_SIZE,
        .timeslot_id = tx_data_timeslots,
        .timeslot_count = ARRAY_SIZE(tx_data_timeslots),
    };
    tx_data_conn = swc_connection_init(node, tx_data_conn_cfg, swc_err);
    if (*swc_err != SWC_ERR_NONE) {
        return;
    }

    swc_channel_cfg_t tx_data_channel_cfg = {
        .tx_pulse_count = TX_DATA_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain = TX_DATA_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT,
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++) {
        tx_data_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(tx_data_conn, node, tx_data_channel_cfg, swc_err);
        if (*swc_err != SWC_ERR_NONE) {
            return;
        }
    }
    swc_connection_set_tx_success_callback(tx_data_conn, conn_tx_data_success_callback, swc_err);
    if (*swc_err != SWC_ERR_NONE) {
        return;
    }

    /* ** RX Data Connection ** */
    swc_connection_cfg_t rx_data_conn_cfg = {
        .name = "RX Data Connection",
        .source_address = remote_address,
        .destination_address = local_address,
        .max_payload_size = MAX_DATA_PAYLOAD_SIZE,
        .queue_size = RX_DATA_QUEUE_SIZE,
        .timeslot_id = rx_data_timeslots,
        .timeslot_count = ARRAY_SIZE(rx_data_timeslots),
    };
    rx_data_conn = swc_connection_init(node, rx_data_conn_cfg, swc_err);
    if (*swc_err != SWC_ERR_NONE) {
        return;
    }

    swc_connection_set_acknowledgement(rx_data_conn, false, swc_err);
    if (*swc_err != SWC_ERR_NONE) {
        return;
    }

    swc_connection_set_credit_flow_ctrl(rx_data_conn, node, false, swc_err);
    if (*swc_err != SWC_ERR_NONE) {
        return;
    }

    swc_connection_set_retransmission(rx_data_conn, node, false, 0, 0, swc_err);
    if (*swc_err != SWC_ERR_NONE) {
        return;
    }

    swc_connection_set_rx_success_callback(rx_data_conn, conn_rx_data_success_callback, swc_err);
    if (*swc_err != SWC_ERR_NONE) {
        return;
    }

    swc_set_certification_mode(certification_mode > FACADE_CERTIF_NONE, swc_err);
    if (*swc_err != SWC_ERR_NONE) {
        return;
    }

    /* Wireless Core setup. */
    swc_setup(node, swc_err);
}

/** @brief Callback function when a previously sent audio frame has been ACK'd.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 */
static void conn_tx_audio_success_callback(void *conn)
{
    (void)conn;

    facade_tx_audio_conn_status();
}

/** @brief Callback function when a previously sent data frame has been ACK'd.
 *
 *  @note This function is empty, but can be filled by users in any way they see fit.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 */
static void conn_tx_data_success_callback(void *conn)
{
    (void)conn;
}

/** @brief Callback function when a data frame has been successfully received on data connection.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 */
static void conn_rx_data_success_callback(void *conn)
{
    (void)conn;

    sac_status_t sac_status = SAC_OK;
    swc_error_t swc_err = SWC_ERR_NONE;

    /* Get received payload. */
    if (wireless_read_data(&received_user_data, sizeof(received_user_data), &swc_err) > 0) {
        /* Depending on the requested button state from the Node, the specified LED turns on or off. */
        if (received_user_data.button_state == false) {
            facade_empty_payload_received_status();
        } else {
            facade_payload_received_status();
        }

        /* The fallback state is updated. */
        sac_fallback_set_rx_link_margin(&sac_fallback_instance, received_user_data.link_margin, &sac_status);
        status_handler_sac(sac_status);
    }
}

/** @brief Initialize the Audio Core.
 */
static void app_audio_core_init(void)
{
    sac_status_t sac_status = SAC_OK;

    sac_endpoint_interface_t i2s_producer_iface;
    sac_endpoint_interface_t swc_consumer_iface;

    sac_processing_interface_t fallback_iface;
    sac_processing_interface_t packing_iface;
    sac_processing_interface_t packing_fallback_iface;

    sac_facade_hal_init(&sac_hal);
    sac_endpoint_swc_init(NULL, &swc_consumer_iface);
    sac_facade_codec_endpoint_init(&i2s_producer_iface, NULL);
    facade_set_sai_complete_callback(NULL, i2s_rx_audio_complete_callback);

    app_audio_core_fallback_interface_init(&fallback_iface);
    app_audio_core_packing_interface_init(&packing_iface);
    app_audio_core_packing_fallback_interface_init(&packing_fallback_iface);

    swc_consumer_instance.connection = tx_audio_conn;

    /* Initialize Audio Core. */
    sac_cfg_t core_cfg = {
        .memory_pool = audio_memory_pool,
        .memory_pool_size = SAC_MEM_POOL_SIZE,
    };
    sac_init(core_cfg, &sac_hal, &sac_status);
    status_handler_sac(sac_status);

    /*
     * Audio Pipeline
     * ==============
     *
     ***** NORMAL MODE *****
     * Input:      Stereo stream of 48kHz/24-bit depth samples, encoded on 32 bits.
     * Processing: Packing from 32 bits to 24 bits audio samples.
     * Output:     Stereo stream at 48 kHz/24 bits is sent over the air to the Node.
     *
     * +-----+    +--------------------+    +-----+
     * | I2S | -> | Packing to 24 bits | -> | SWC |
     * +-----+    +--------------------+    +-----+
     *
     ***** FALLBACK MODE *****
     * Input:      Stereo stream of 48kHz/24-bit depth samples, encoded on 32 bits.
     * Processing: Packing from 32 bits to 16 bits audio samples.
     * Output:     Stereo stream at 48 kHz/16 bits is sent over the air to the Node.
     *
     * +-----+    +--------------------+    +-----+
     * | I2S | -> | Packing to 16 bits | -> | SWC |
     * +-----+    +--------------------+    +-----+
     */

    /* Initialize codec producer endpoint. */
    sac_endpoint_cfg_t i2s_producer_cfg = {
        .use_encapsulation = false,
        .delayed_action = true,
        .channel_count = SAC_AUDIO_CHANNEL_COUNT,
        .audio_payload_size = SAC_PRODUCER_AUDIO_PAYLOAD_SIZE,
        .queue_size = SAC_MIN_PRODUCER_QUEUE_SIZE,
    };
    i2s_producer = sac_endpoint_init(NULL, "I2S EP (Producer)", i2s_producer_iface, i2s_producer_cfg, &sac_status);
    status_handler_sac(sac_status);

    sac_fallback_instance = sac_fallback_get_defaults();
    sac_fallback_instance.connection = tx_audio_conn;
    sac_fallback_instance.is_tx_device = true;
    sac_fallback_instance.cca_max_try_count = FALLBACK_CCA_TRY_COUNT;
    sac_fallback_instance.get_tick = facade_get_tick_ms;
    sac_fallback_instance.tick_frequency_hz = 1000;
    sac_fallback_processing = sac_processing_stage_init(&sac_fallback_instance, "Fallback TX", fallback_iface,
                                                        &sac_status);
    status_handler_sac(sac_status);

    /* Processing stage that packs into 24 bits before sending if fallback is deactivated. */
    audio_packing_instance.packing_mode = SAC_PACK_24BITS;
    sac_packing_processing = sac_processing_stage_init((void *)&audio_packing_instance, "Audio Fallback Packing",
                                                       packing_iface, &sac_status);
    status_handler_sac(sac_status);

    /* Processing stage that packs into 16 bits before sending if fallback is activated. */
    audio_packing_fallback_instance.packing_mode = SAC_PACK_24BITS_16BITS;
    sac_packing_fallback_processing = sac_processing_stage_init((void *)&audio_packing_fallback_instance,
                                                                "Audio Packing", packing_fallback_iface, &sac_status);
    status_handler_sac(sac_status);

    /* Initialize SWC consumer endpoint. */
    sac_endpoint_cfg_t swc_consumer_cfg = {
        .use_encapsulation = true,
        .delayed_action = false,
        .channel_count = SAC_AUDIO_CHANNEL_COUNT,
        .audio_payload_size = SAC_CONSUMER_AUDIO_PAYLOAD_SIZE,
        .queue_size = SAC_LATENCY_QUEUE_SIZE,
    };
    swc_consumer = sac_endpoint_init((void *)&swc_consumer_instance, "SWC EP (Consumer)", swc_consumer_iface,
                                     swc_consumer_cfg, &sac_status);
    status_handler_sac(sac_status);

    /* Initialize audio pipeline. */
    sac_pipeline_cfg_t pipeline_cfg = {
        .do_initial_buffering = true,
    };
    sac_pipeline = sac_pipeline_init("I2S -> SWC", i2s_producer, pipeline_cfg, swc_consumer, &sac_status);
    status_handler_sac(sac_status);

    /* Add processing stages to the audio pipeline. */
    sac_pipeline_add_processing(sac_pipeline, sac_fallback_processing, &sac_status);
    status_handler_sac(sac_status);

    sac_pipeline_add_processing(sac_pipeline, sac_packing_processing, &sac_status);
    status_handler_sac(sac_status);

    sac_pipeline_add_processing(sac_pipeline, sac_packing_fallback_processing, &sac_status);
    status_handler_sac(sac_status);

    /* Setup audio pipeline. */
    sac_pipeline_setup(sac_pipeline, &sac_status);
    status_handler_sac(sac_status);
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
    iface->gate = sac_fallback_gate_is_fallback_off;
}

/** @brief Initialize the audio fallback packing processing stage interface.
 *
 *  @param[out] iface  Processing interface.
 */
static void app_audio_core_packing_fallback_interface_init(sac_processing_interface_t *iface)
{
    iface->init = sac_packing_init;
    iface->ctrl = sac_packing_ctrl;
    iface->process = sac_packing_process;
    iface->gate = sac_fallback_gate_is_fallback_on;
}

/** @brief Update the fallback LED indicator.
 */
static void fallback_led_handler(void)
{
    sac_status_t sac_status = SAC_OK;

    facade_fallback_status(sac_fallback_is_active(&sac_fallback_instance, &sac_status));
    status_handler_sac(sac_status);
}

/** @brief SAI DMA RX complete callback.
 *
 *  This receives audio packets from the codec. It needs to be executed every time a DMA transfer from the codec is
 *  completed in order to keep recording audio.
 */
static void i2s_rx_audio_complete_callback(void)
{
    sac_status_t sac_status = SAC_OK;

    /* The codec produces audio samples when it receives input audio. */
    sac_pipeline_produce(sac_pipeline, &sac_status);
    status_handler_sac(sac_status);
}

/** @brief Callback handling the audio process that triggers with the app timer.
 */
static void audio_process_callback(void)
{
    sac_status_t sac_status = SAC_OK;

    /* Processing stages of the pipeline are executed. */
    sac_pipeline_process(sac_pipeline, &sac_status);
    status_handler_sac(sac_status);
    /* The SWC consumes audio samples produced by the codec. */
    sac_pipeline_consume(sac_pipeline, &sac_status);
    status_handler_sac(sac_status);
}

/** @brief Print the audio and wireless statistics.
 */
static void print_stats(void)
{
    static char stats_string[STATS_ARRAY_LENGTH];
    int string_length = 0;
    sac_status_t sac_status = SAC_OK;

    const char *device_str = "\n<   COORDINATOR   >\n\r";
    const char *audio_stats_str = "\n<<  Audio Core Statistics  >>\n\r";
    const char *fallback_stats_str = "\n<<  Fallback Statistics  >>\n\r";
    const char *wireless_stats_str = "\n<<  Wireless Core Statistics  >>\n\r";

    /* ** Device Prelude ** */
    string_length = snprintf(stats_string, sizeof(stats_string), device_str);

    if (certification_mode > FACADE_CERTIF_NONE) {
        string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length,
                                  "Cert. Mode: %i\r\n", certification_mode);
    }

    /* ** Audio statistics ** */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, audio_stats_str);
    sac_pipeline_update_stats(sac_pipeline);
    string_length += sac_pipeline_format_stats(sac_pipeline, stats_string + string_length,
                                               sizeof(stats_string) - string_length);

    /* ** Audio fallback statistics ** */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, fallback_stats_str);
    string_length += sac_fallback_format_stats(&sac_fallback_instance, stats_string + string_length,
                                               sizeof(stats_string) - string_length, &sac_status);
    status_handler_sac(sac_status);

    /* ** Wireless statistics ** */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, wireless_stats_str);
    swc_connection_update_stats(tx_audio_conn);
    string_length += swc_connection_format_stats(tx_audio_conn, node, stats_string + string_length,
                                                 sizeof(stats_string) - string_length);

    swc_connection_update_stats(tx_data_conn);
    string_length += swc_connection_format_stats(tx_data_conn, node, stats_string + string_length,
                                                 sizeof(stats_string) - string_length);

    swc_connection_update_stats(rx_data_conn);
    string_length += swc_connection_format_stats(rx_data_conn, node, stats_string + string_length,
                                                 sizeof(stats_string) - string_length);

    facade_print_string(stats_string);
}

/** @brief Callback handles the printing of statistics and also sends the button state every 10 ms.
 */
static void data_callback(void)
{
    static uint8_t counter;
    swc_error_t swc_err = SWC_ERR_NONE;

    /* Every second, the statistics are displayed. */
    if (counter >= STATS_PRINT_PERIOD_MS / DATA_TX_PERIOD_MS) {
        print_stats_now = true;
        counter = 0;
    }
    counter++;

    /* Send the state of the button to the Node. */
    wireless_send_data(&transmitted_user_data, sizeof(transmitted_user_data), &swc_err);
}

/** @brief Toggles the button state request.
 */
static void toggle_button_state(void)
{
    if (transmitted_user_data.button_state != false) {
        transmitted_user_data.button_state = false;
    } else {
        transmitted_user_data.button_state = true;
    }
}

/** @brief Enter Pairing Mode using the Pairing Module.
 */
static void enter_pairing_mode(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    pairing_error_t pairing_err = PAIRING_ERR_NONE;

    pairing_event_t pairing_event;

    facade_notify_enter_pairing();

    /* The wireless core must be stopped before starting the pairing procedure. */
    if (swc_get_status() == SWC_STATUS_RUNNING) {
        swc_disconnect(&swc_err);
        if ((swc_err != SWC_ERR_NONE) && (swc_err != SWC_ERR_NOT_CONNECTED)) {
            while (1) {};
        }
    }

    /* Give the information to the Pairing Module. */
    app_pairing_cfg.app_code = PAIRING_APP_CODE;
    app_pairing_cfg.timeout_sec = PAIRING_TIMEOUT_IN_SECONDS;
    app_pairing_cfg.application_callback = pairing_process_callback;
    app_pairing_cfg.memory_pool = swc_memory_pool;
    app_pairing_cfg.memory_pool_size = SWC_MEM_POOL_SIZE;
    app_pairing_cfg.uwb_regulation = SWC_REGULATION_FCC;
    app_pairing_cfg.context_switch_callback = facade_context_switch_trigger;
    pairing_event = pairing_coordinator_start(&app_pairing_cfg, &pairing_assigned_address, pairing_discovery_list,
                                              PAIRING_DISCOVERY_LIST_SIZE, &pairing_err);
    if (pairing_err != PAIRING_ERR_NONE) {
        while (1) {};
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
    if ((swc_err != SWC_ERR_NONE) && (swc_err != SWC_ERR_NOT_CONNECTED)) {
        while (1) {};
    }
    tx_audio_conn = NULL;
    tx_data_conn = NULL;
    rx_data_conn = NULL;

    /* Reset the pairing discovery list. */
    memset(pairing_discovery_list, 0, sizeof(pairing_discovery_list));

    /* Stop the audio pipeline. */
    sac_pipeline_stop(sac_pipeline, &sac_status);
    status_handler_sac(sac_status);
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
    facade_button_handling(abort_pairing_procedure, NULL, NULL, NULL);
}

/** @brief Abort the pairing procedure.
 */
static void abort_pairing_procedure(void)
{
    pairing_abort();
}

/** @brief Send data with a specific connection.
 *
 *  @param[in] transmitted_data  Data to be sent over the air.
 *  @param[in] size              Size of the data to be sent over the air.
 *  @param[out] swc_err          Wireless Core error code.
 */
static void wireless_send_data(void *transmitted_data, uint8_t size, swc_error_t *swc_err)
{
    uint8_t *buffer;

    /* Get buffer from queue to hold data. */
    swc_connection_allocate_payload_buffer(tx_data_conn, &buffer, MAX_DATA_PAYLOAD_SIZE, swc_err);
    if ((*swc_err != SWC_ERR_NONE) || (buffer == NULL)) {
        return;
    }

    /* Format the new payload. */
    if (transmitted_data != NULL) {
        memcpy(buffer, transmitted_data, size);
    }

    /* Send the payload through the Wireless Core. */
    swc_connection_send(tx_data_conn, buffer, size, swc_err);
}

/** @brief Read data from a specific connection.
 *
 *  @param[out] received_data   Pointer to data buffer to write to.
 *  @param[in]  size            Size of the data buffer.
 *  @param[out] swc_err         Wireless Core error code.
 *
 *  @return Size of the data read.
 */
static uint16_t wireless_read_data(void *received_data, uint8_t size, swc_error_t *swc_err)
{
    uint8_t *payload = NULL;
    uint16_t payload_size;

    /* Read received data. */
    payload_size = swc_connection_receive(rx_data_conn, &payload, swc_err);
    if (*swc_err != SWC_ERR_NONE) {
        return 0;
    }

    if (payload_size > size) {
        return 0;
    }

    if (received_data != NULL) {
        memcpy(received_data, payload, payload_size);
    }

    /* Free the payload memory. */
    swc_connection_receive_complete(rx_data_conn, swc_err);

    return payload_size;
}

/** @brief SAC Status handler.
 *
 *  @param[in] sac_status  Status code.
 */
static void status_handler_sac(sac_status_t sac_status)
{
    if (sac_status != SAC_OK) {
        if (sac_status > 0) {
            /* Handle warning */
        } else if (sac_status < 0) {
            /* Handle error */
            while (1) {};
        }
    }
}

/** @brief Initialize the application.
 */
static void app_init(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    sac_status_t sac_status = SAC_OK;

    /* Initialize Wireless Core. */
    app_swc_core_init(&pairing_assigned_address, &swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1) {};
    }
    /* Initialize Audio Core. */
    app_audio_core_init();
    /* Initialize GPIOs and peripherals for audio operations. */
    facade_audio_coord_init();
    /* Connect the Wireless Core. */
    swc_connect(&swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1) {};
    }
    /* Start the audio pipeline. */
    sac_pipeline_start(sac_pipeline, &sac_status);
    status_handler_sac(sac_status);
    /* Start timer used for audio process. */
    facade_audio_process_timer_start();
    /* Start data and statistics timer. */
    facade_data_timer_start();
}
