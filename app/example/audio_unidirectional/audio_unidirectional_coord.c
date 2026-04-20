/** @file  audio_unidirectional_coord.c
 *  @brief This application creates a unidirectional audio stream at 48kHz/24-bit depth from the audio interface of the
 *         Coordinator to the audio interface of the Node. It includes a fallback to 16 bits to ensure audio quality
 *         under varying conditions. Additionally, there is a bidirectional link for user data and link margin, which
 *         supports dynamic fallback updates.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES ******************************************************************/
#include <stdio.h>
#include "audio_unidirectional_facade.h"
#include "pairing_api.h"
#include "pairing_cfg.h"
#include "sac_api.h"
#include "sac_cfg.h"
#include "sac_endpoint_swc.h"
#include "sac_fallback.h"
#include "sac_fallback_gate.h"
#include "sac_hal_facade.h"
#include "sac_packing.h"
#include "sac_stats.h"
#include "swc_api.h"
#include "swc_cfg.h"
#include "swc_cfg_coord.h"
#include "swc_error.h"
#include "swc_stats.h"

/* CONSTANTS ******************************************************************/
/* Total memory needed for the Audio Core. */
#define SAC_MEM_POOL_SIZE 6000
/* Total memory needed for the Wireless Core. */
#define SWC_MEM_POOL_SIZE 10000
/* The data connection supports up to 16 bytes. */
#define MAX_DATA_PAYLOAD_SIZE 16
/* Length of the statistics array used for terminal display. */
#define STATS_ARRAY_LENGTH 3000
/* Period for data transmission timer in ms. */
#define DATA_TX_PERIOD_MS 10
/* Size of the buffer used to print errors. */
#define ERROR_MESSAGE_BUFFER_SIZE 50
/* Interval to print statistics in ms. */
#define PRINT_INTERVAL_MS 1000

/* **** Fallback **** */
/* During fallback mode transfers, audio samples are packed into 16 bits instead of 24 bits. */
#define FALLBACK_PAYLOAD_SIZE \
    SAC_CALCULATE_PAYLOAD_SIZE(MAIN_CHANNEL_SAMPLE_COUNT, MAIN_CHANNEL_CHANNEL_COUNT, SAC_16BITS)
/* Fallback channel index. */
#define FALLBACK_INDEX_0 0

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
} user_data_t;

/* PRIVATE GLOBALS ************************************************************/
/* **** Audio Core **** */
static uint8_t audio_memory_pool[SAC_MEM_POOL_SIZE];
static sac_pipeline_t *sac_pipeline;

/* **** Processing Stages **** */
static sac_fallback_instance_t sac_fallback_instance;
static sac_processing_t *sac_fallback_processing;
static sac_packing_instance_t audio_packing_instance;
static sac_processing_t *sac_packing_processing;
static sac_packing_instance_t audio_packing_fallback_instance;
static sac_processing_t *sac_packing_fallback_processing;

/* **** Endpoints **** */
static sac_endpoint_t *audio_producer;
static ep_swc_instance_t swc_consumer_instance;
static sac_endpoint_t *swc_consumer;

/* **** Wireless Core **** */
static uint8_t swc_memory_pool[SWC_MEM_POOL_SIZE];

static uint32_t timeslot_us[] = SCHEDULE;
static uint32_t channel_sequence[] = CHANNEL_SEQUENCE;
static uint32_t channel_frequency[] = CHANNEL_FREQ;

static int32_t tx_timeslots[] = COORD_TIMESLOTS;
static int32_t rx_timeslots[] = NODE_TIMESLOTS;

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

/* **** Processing stages **** */
static void app_audio_core_fallback_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_packing_interface_init(sac_processing_interface_t *iface);
static void app_audio_core_packing_fallback_interface_init(sac_processing_interface_t *iface);

/* **** Button actions **** */
static void enter_pairing_mode(void);
static void unpair_device(void);
static void abort_pairing_procedure(void);

/* Fallback LED and terminal display. */
static void fallback_led_handler(void);
static bool should_print_stats(void);
static void print_stats(void);

static void wireless_send_data(void *transmitted_data, uint8_t size, swc_error_t *swc_err);
static uint16_t wireless_read_data(void *received_data, uint8_t size, swc_error_t *swc_err);

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
    };
    facade_set_button_callbacks(button_callbacks);

    /* Audio process timer initialization. */
    facade_audio_process_timer_init(audio_process_callback);

    /* Timer that updates statistics display every second and transmits button state to Node every 10 ms. */
    facade_data_timer_init(DATA_TX_PERIOD_MS);
    facade_data_timer_set_callback(data_callback);

    certification_mode = facade_get_coord_certification_mode();
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
    uint8_t fallback_thresholds[] = {FALLBACK_PAYLOAD_SIZE + sizeof(sac_header_t)};
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

        swc_connection_set_connection_priority(tx_data_conn, AUDIO_CONNECTION_PRIORITY, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

        tx_audio_conn = swc_connection_init(tx_audio_conn_cfg, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

        swc_connection_set_connection_priority(tx_audio_conn, DATA_CONNECTION_PRIORITY, swc_err);
        ASSERT_SWC_STATUS(*swc_err);

    } else {
        if (certification_mode == FACADE_CERTIF_AUDIO_16_BIT) {
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
    sac_processing_interface_t packing_fallback_iface = {0};

    sac_endpoint_swc_init(NULL, &swc_consumer_iface);
    sac_facade_audio_endpoint_init(&producer_iface, NULL);
    facade_set_audio_complete_callback(NULL, audio_rx_complete_callback);

    app_audio_core_fallback_interface_init(&fallback_iface);
    app_audio_core_packing_interface_init(&packing_iface);
    app_audio_core_packing_fallback_interface_init(&packing_fallback_iface);

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
     * Input:      Stereo stream of 48kHz/24-bit depth samples.
     * Output:     Stereo stream at 48 kHz/24 bits is sent over the air to the Node.
     *
     * +-----+    +-----+
     * | USB | -> | SWC |
     * +-----+    +-----+
     *
     ***** NORMAL MODE I2S (Fallback mode 0) *****
     * Input:      Stereo stream of 48kHz/24-bit depth samples, encoded on 32 bits.
     * Processing: Packing from 32 bits to 24 bits audio samples.
     * Output:     Stereo stream at 48 kHz/24 bits is sent over the air to the Node.
     *
     * +-----+    +--------------------+    +-----+
     * | I2S | -> | Packing to 24 bits | -> | SWC |
     * +-----+    +--------------------+    +-----+
     *
     ***** FALLBACK MODE (Fallback mode 1) *****
     * Input(USB): Stereo stream of 48kHz/24-bit depth samples.
     * Input(I2S): Stereo stream of 48kHz/24-bit depth samples, encoded on 32 bits.
     * Processing: Packing from 32 bits to 16 bits audio samples.
     * Output:     Stereo stream at 48 kHz/16 bits is sent over the air to the Node.
     *
     * +-------+    +--------------------+    +-----+
     * | AUDIO | -> | Packing to 16 bits | -> | SWC |
     * +-------+    +--------------------+    +-----+
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
    sac_fallback_processing = sac_processing_stage_init(&sac_fallback_instance, "Fallback TX", fallback_iface,
                                                        &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Processing stage that packs into 24 bits before sending if fallback is deactivated. */
    audio_packing_instance.packing_mode = SAC_PACK_24BITS;
    sac_packing_processing = sac_processing_stage_init((void *)&audio_packing_instance, "Audio Fallback Packing",
                                                       packing_iface, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Processing stage that packs into 16 bits before sending if fallback is activated. */
    audio_packing_fallback_instance.packing_mode = USB_AUDIO_ENABLED ? SAC_SCALE_24BITS_16BITS : SAC_PACK_24BITS_16BITS;
    sac_packing_fallback_processing = sac_processing_stage_init((void *)&audio_packing_fallback_instance,
                                                                "Audio Packing", packing_fallback_iface, &sac_status);
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
    };
    sac_pipeline = sac_pipeline_init("Audio -> SWC", audio_producer, pipeline_cfg, swc_consumer, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Add processing stages to the audio pipeline. */
    sac_pipeline_add_processing(sac_pipeline, sac_fallback_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

#if !USB_AUDIO_ENABLED
    /* When using I2S, packing is required to convert 24-bit audio aligned on 32-bit words. */
    sac_pipeline_add_processing(sac_pipeline, sac_packing_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
#endif

    sac_pipeline_add_processing(sac_pipeline, sac_packing_fallback_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Setup audio pipeline. */
    sac_pipeline_setup(sac_pipeline, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Fallback mode configuration. */
    sac_fallback_mode_cfg_t mode_cfg = sac_fallback_mode_get_defaults();
    uint8_t mode_index;

    /* Fallback mode 0 configuration. */
    mode_index = sac_fallback_add_mode(&sac_fallback_instance, "48kHz 24-bit", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&sac_fallback_instance, mode_index, sac_packing_processing, &sac_status);
    ASSERT_SAC_STATUS(sac_status);

    /* Fallback mode 1 configuration. */
    mode_index = sac_fallback_add_mode(&sac_fallback_instance, "48kHz 16-bit", mode_cfg, &sac_status);
    ASSERT_SAC_STATUS(sac_status);
    sac_fallback_mode_assign_process(&sac_fallback_instance, mode_index, sac_packing_fallback_processing, &sac_status);
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

/** @brief Initialize the audio fallback packing processing stage interface.
 *
 *  @param[out] iface  Processing interface.
 */
static void app_audio_core_packing_fallback_interface_init(sac_processing_interface_t *iface)
{
    iface->init = sac_packing_init;
    iface->ctrl = sac_packing_ctrl;
    iface->process = sac_packing_process;
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
 *  This receives audio packets from the codec. It needs to be executed every time a DMA transfer from the codec is
 *  completed in order to keep recording audio.
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

/** @brief Callback sends the button state every 10 ms.
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
