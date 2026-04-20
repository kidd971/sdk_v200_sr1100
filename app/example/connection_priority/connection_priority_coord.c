/** @file  connection_priority_coord.c
 *  @brief This is a basic example of how to use the Wireless Core connection priority.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include <stdio.h>
#include "connection_priority_facade.h"
#include "pairing_api.h"
#include "pairing_cfg.h"
#include "swc_api.h"
#include "swc_cfg.h"
#include "swc_cfg_coord.h"
#include "swc_error.h"
#include "swc_stats.h"

/* CONSTANTS ******************************************************************/
#define SWC_MEM_POOL_SIZE           12000
#define MAX_SMALL_PAYLOAD_SIZE_BYTE 8
#define MAX_BIG_PAYLOAD_SIZE_BYTE   15
#define STATS_ARRAY_LENGTH          8000
#define TIMER1_PACKET_RATE_US       2000
#define TIMER2_PACKET_RATE_US       2500
#define PRINT_INTERVAL_MS           1000
#define TX_CID0_PRIORITY            0
#define TX_CID1_PRIORITY            1
#define TX_CID2_PRIORITY            2
#define RX_PRIORITY                 0
/* Size of the buffer used to print errors. */
#define ERROR_MESSAGE_BUFFER_SIZE 50

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

/** @brief Enumeration representing the types of connection possible.
 */
typedef enum conn_type {
    /*! A transmittion connection. */
    TX_CONN,
    /*! A reception connection. */
    RX_CONN,
} conn_type_t;

/* PRIVATE GLOBALS ************************************************************/
/* ** Wireless Core ** */
static uint8_t swc_memory_pool[SWC_MEM_POOL_SIZE];
static swc_connection_t *tx_cid0;
static swc_connection_t *tx_cid1;
static swc_connection_t *tx_cid2;
static swc_connection_t *rx_cid3;
static swc_connection_t *rx_cid4;

static uint32_t timeslot_us[] = SCHEDULE;
static uint32_t channel_sequence[] = CHANNEL_SEQUENCE;
static uint32_t channel_frequency[] = CHANNEL_FREQ;
static int32_t tx_timeslots[] = COORD_TIMESLOTS;
static int32_t rx_timeslots[] = NODE_TIMESLOTS;

/* ** Application Specific ** */
static uint32_t cid0_sent_count;
static uint32_t cid1_sent_count;
static uint32_t cid2_sent_count;
static uint32_t cid0_dropped_count;
static uint32_t cid1_dropped_count;
static uint32_t cid2_dropped_count;

static volatile bool reset_stats_now;
static volatile bool stop_cid0;

/* **** Application Specific **** */
static facade_certification_mode_t certification_mode;
/* Variables supporting pairing between the two devices. */
static device_pairing_state_t device_pairing_state;
static pairing_cfg_t app_pairing_cfg;
static pairing_assigned_address_t pairing_assigned_address;
static pairing_discovery_list_t pairing_discovery_list[PAIRING_DISCOVERY_LIST_SIZE];

/* PRIVATE FUNCTION PROTOTYPE *************************************************/
static void app_init(void);
static void app_swc_core_init(pairing_assigned_address_t *app_pairing, swc_error_t *err);
static void multi_conn_tx_send_callback(void);
static void rx_success_callback(void *conn, void *arg);
static void tx_success_callback(void *conn, void *arg);
static void single_conn_tx_send_callback(void);
static swc_connection_t *setup_conn(uint8_t local_address, uint8_t remote_address, swc_channel_cfg_t *channel_cfg,
                                    conn_type_t conn_type, const char *conn_name, uint8_t prio,
                                    void (*cb)(void *conn, void *arg));
static void setup_connections(uint8_t local_address, uint8_t remote_address);

static bool should_print_stats(void);
static void print_stats(void);
static void reset_stats(void);
static void stop_cid0_toggle(void);

static void enter_pairing_mode(void);
static void unpair_device(void);

static void pairing_application_callback(void);
static void abort_pairing_procedure(void);
static void pairing_button_callback(void);

/* PUBLIC FUNCTIONS ***********************************************************/
int main(void)
{
    facade_board_init();

    /* Initialize wireless core context switch handler before pairing is available */
    facade_set_context_switch_handler(swc_connection_callbacks_processing_handler);

    facade_button_callbacks_t button_callbacks = {
        .pairing_callback = pairing_button_callback,
        .reset_stats_callback = reset_stats,
        .stop_cid0_toggle_callback = stop_cid0_toggle,
    };
    facade_set_button_callbacks(button_callbacks);

    /* Connection ID 0 (CID0) and Connection ID 2 (CID2) send 500 pkt/s. */
    facade_packet_rate_timer1_init(TIMER1_PACKET_RATE_US);
    facade_packet_rate_set_timer1_callback(multi_conn_tx_send_callback);

    /* Connection ID 1 (CID1) sends 400 pkt/s. */
    facade_packet_rate_timer2_init(TIMER2_PACKET_RATE_US);
    facade_packet_rate_set_timer2_callback(single_conn_tx_send_callback);

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

        /* Print received string and stats every PRINT_INTERVAL_MS */
        if (should_print_stats()) {
            print_stats();
        }

        /* Wait for an interrupt event. */
        facade_wait_for_interrupt();
    }

    return 0;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize the application.
 */
static void app_init(void)
{
    swc_error_t swc_err;

    /* Reconfigure the Coordinator and Node addresses. */
    app_swc_core_init(&pairing_assigned_address, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    swc_connect(&swc_err);
    ASSERT_SWC_STATUS(swc_err);

    /* Connection ID 0 (CID0) and Connection ID 2 (CID2) start sending packet. */
    facade_packet_rate_timer1_start();

    /* Connection ID 1 (CID1) sends 400 pkt/s start sending packet. */
    facade_packet_rate_timer2_start();
}

/** @brief Initialize the Wireless Core.
 *
 *  @param[in]  app_pairing  Configure the Wireless Core with the pairing values.
 *  @param[out] err          Wireless Core error code.
 */
static void app_swc_core_init(pairing_assigned_address_t *app_pairing, swc_error_t *err)
{
    uint8_t remote_address = pairing_discovery_list[PAIRING_DEVICE_ROLE_NODE].node_address;
    uint8_t local_address = pairing_discovery_list[PAIRING_DEVICE_ROLE_COORDINATOR].node_address;

    if (certification_mode != FACADE_CERTIF_NONE) {
        app_pairing->coordinator_address = 0x1;
        app_pairing->node_address = 0x2;
        app_pairing->pan_id = 0xABC;
        remote_address = 0x2;
        local_address = 0x1;
    }

    swc_cfg_t core_cfg = {
        .timeslot_sequence = timeslot_us,
        .timeslot_sequence_length = ARRAY_SIZE(timeslot_us),
        .channel_sequence = channel_sequence,
        .channel_sequence_length = ARRAY_SIZE(channel_sequence),
        .concurrency_mode = SWC_CONCURRENCY_MODE_HIGH_PERFORMANCE,
        .memory_pool = swc_memory_pool,
        .memory_pool_size = SWC_MEM_POOL_SIZE,
    };

    swc_node_cfg_t node_cfg = {
        .role = SWC_ROLE_COORDINATOR,
        .pan_id = app_pairing->pan_id,
        .coordinator_address = app_pairing->coordinator_address,
        .local_address = local_address,
    };

    swc_init(core_cfg, node_cfg, facade_context_switch_trigger, err);
    ASSERT_SWC_STATUS(*err);

    swc_radio_module_init(SWC_RADIO_ID_1, true, err);
    ASSERT_SWC_STATUS(*err);

    setup_connections(local_address, remote_address);

    /* Handle certification mode. */
    swc_set_certification_mode(certification_mode != FACADE_CERTIF_NONE, err);
    ASSERT_SWC_STATUS(*err);

    swc_setup(err);
    ASSERT_SWC_STATUS(*err);
}

/** @brief Callback function when it is time to send payloads from multiple connections.
 *
 *  Payloads are sent on Connection ID 0 (CID0) and Connection ID 2 (CID2).
 */
static void multi_conn_tx_send_callback(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    uint8_t *payload_buf = NULL;

    if (!stop_cid0) {
        swc_connection_get_payload_buffer(tx_cid0, &payload_buf, &swc_err);

        if (payload_buf != NULL) {
            snprintf((char *)payload_buf, MAX_BIG_PAYLOAD_SIZE_BYTE, "%s", "CID0");
            swc_connection_send(tx_cid0, payload_buf, MAX_BIG_PAYLOAD_SIZE_BYTE, &swc_err);
            ASSERT_SWC_STATUS(swc_err);

            cid0_sent_count++;
        } else {
            cid0_dropped_count++;
        }
    }

    swc_connection_get_payload_buffer(tx_cid2, &payload_buf, &swc_err);

    if (payload_buf != NULL) {
        snprintf((char *)payload_buf, MAX_BIG_PAYLOAD_SIZE_BYTE, "%s", "CID2");
        swc_connection_send(tx_cid2, payload_buf, MAX_BIG_PAYLOAD_SIZE_BYTE, &swc_err);
        ASSERT_SWC_STATUS(swc_err);

        cid2_sent_count++;
    } else {
        cid2_dropped_count++;
    }
}

/** @brief Callback function when a frame has been successfully received.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 *  @param[in] arg   Additional argument passed to the callback function.
 */
static void rx_success_callback(void *conn, void *arg)
{
    swc_error_t err = SWC_ERR_NONE;

    (void)arg;

    swc_connection_receive_complete(conn, &err);
    ASSERT_SWC_STATUS(err);

    facade_rx_conn_status();
}

/** @brief Callback function when a frame has been successfully transmitted.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 */
static void tx_success_callback(void *conn, void *arg)
{
    (void)conn;
    (void)arg;

    facade_tx_conn_status();
}

/** @brief Callback function when it is time to send a payload from a single connection.
 *
 *  Payload is sent on Connection ID 1 (CID1).
 */
static void single_conn_tx_send_callback(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    uint8_t *payload_buf = NULL;

    swc_connection_get_payload_buffer(tx_cid1, &payload_buf, &swc_err);

    if (payload_buf != NULL) {
        snprintf((char *)payload_buf, MAX_SMALL_PAYLOAD_SIZE_BYTE, "%s", "CID1");
        swc_connection_send(tx_cid1, payload_buf, MAX_SMALL_PAYLOAD_SIZE_BYTE, &swc_err);
        ASSERT_SWC_STATUS(swc_err);

        cid1_sent_count++;
    } else {
        cid1_dropped_count++;
    }
}

static swc_connection_t *setup_conn(uint8_t local_address, uint8_t remote_address, swc_channel_cfg_t *channel_cfg,
                                    conn_type_t conn_type, const char *conn_name, uint8_t prio,
                                    void (*cb)(void *conn, void *arg))
{
    if (channel_cfg == NULL) {
        facade_print_error_string("An invalid channel config was given to setup a connection.");
        while (1);
    }

    swc_error_t err = SWC_ERR_NONE;
    swc_connection_t *conn = NULL;

    /* Connection config. */
    swc_connection_cfg_t conn_cfg = {
        .name = conn_name,
        .source_address = conn_type == TX_CONN ? local_address : remote_address,
        .destination_address = conn_type == TX_CONN ? remote_address : local_address,
        .max_payload_size = MAX_BIG_PAYLOAD_SIZE_BYTE,
        .queue_size = SWC_QUEUE_SIZE,
        .timeslot_id = conn_type == TX_CONN ? tx_timeslots : rx_timeslots,
        .timeslot_count = conn_type == TX_CONN ? ARRAY_SIZE(tx_timeslots) : ARRAY_SIZE(rx_timeslots),
    };

    conn = swc_connection_init(conn_cfg, &err);
    ASSERT_SWC_STATUS(err);

    swc_connection_set_connection_priority(conn, prio, &err);
    ASSERT_SWC_STATUS(err);

    switch (conn_type) {
    case TX_CONN:
        swc_connection_set_tx_success_callback(conn, cb, NULL, &err);
        break;
    case RX_CONN:
        swc_connection_set_rx_success_callback(conn, cb, NULL, &err);
        break;
    default:
        facade_print_error_string("An invalid connection type was provided to setup a connection.");
        while (1);
    }
    ASSERT_SWC_STATUS(err);

    for (uint8_t i = 0; i < ARRAY_SIZE(channel_frequency); i++) {
        channel_cfg->frequency = channel_frequency[i];
        swc_connection_add_channel(conn, *channel_cfg, &err);
        ASSERT_SWC_STATUS(err);
    }

    return conn;
}

static void setup_connections(uint8_t local_address, uint8_t remote_address)
{
    swc_channel_cfg_t rx_ch_cfg = {
        .tx_pulse_count = TX_DATA_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain = TX_DATA_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT,
    };
    swc_channel_cfg_t tx_ch_cfg = {
        .tx_pulse_count = TX_DATA_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain = TX_DATA_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT,
    };

    const char *cid0_name = "TX CID0 to Node";
    const char *cid1_name = "TX CID1 to Node";
    const char *cid2_name = "TX CID2 to Node";
    const char *cid3_name = "RX CID3 from Node";
    const char *cid4_name = "RX CID4 from Node";

    switch (certification_mode) {
    case FACADE_CERTIF_NONE:
        /* clang-format off */
        tx_cid0 = setup_conn(local_address, remote_address, &tx_ch_cfg, TX_CONN, cid0_name, TX_CID0_PRIORITY, tx_success_callback);
        tx_cid1 = setup_conn(local_address, remote_address, &tx_ch_cfg, TX_CONN, cid1_name, TX_CID1_PRIORITY, tx_success_callback);
        tx_cid2 = setup_conn(local_address, remote_address, &tx_ch_cfg, TX_CONN, cid2_name, TX_CID2_PRIORITY, tx_success_callback);
        rx_cid3 = setup_conn(local_address, remote_address, &rx_ch_cfg, RX_CONN, cid3_name, RX_PRIORITY, rx_success_callback);
        rx_cid4 = setup_conn(local_address, remote_address, &rx_ch_cfg, RX_CONN, cid4_name, RX_PRIORITY, rx_success_callback);
        /* clang-format on */
        break;
    case FACADE_CERTIF_CONNECTION_ID_0:
        tx_cid0 = setup_conn(local_address, remote_address, &tx_ch_cfg, TX_CONN, cid0_name, 0, tx_success_callback);
        tx_cid1 = setup_conn(local_address, remote_address, &tx_ch_cfg, TX_CONN, cid1_name, 1, tx_success_callback);
        tx_cid2 = setup_conn(local_address, remote_address, &tx_ch_cfg, TX_CONN, cid2_name, 1, tx_success_callback);
        rx_cid3 = setup_conn(local_address, remote_address, &rx_ch_cfg, RX_CONN, cid3_name, 1, rx_success_callback);
        rx_cid4 = setup_conn(local_address, remote_address, &rx_ch_cfg, RX_CONN, cid4_name, 1, rx_success_callback);
        break;
    case FACADE_CERTIF_CONNECTION_ID_1:
        tx_cid1 = setup_conn(local_address, remote_address, &tx_ch_cfg, TX_CONN, cid1_name, 0, tx_success_callback);
        tx_cid0 = setup_conn(local_address, remote_address, &tx_ch_cfg, TX_CONN, cid0_name, 1, tx_success_callback);
        tx_cid2 = setup_conn(local_address, remote_address, &tx_ch_cfg, TX_CONN, cid2_name, 1, tx_success_callback);
        rx_cid3 = setup_conn(local_address, remote_address, &rx_ch_cfg, RX_CONN, cid3_name, 1, rx_success_callback);
        rx_cid4 = setup_conn(local_address, remote_address, &rx_ch_cfg, RX_CONN, cid4_name, 1, rx_success_callback);
        break;
    case FACADE_CERTIF_CONNECTION_ID_2:
        tx_cid2 = setup_conn(local_address, remote_address, &tx_ch_cfg, TX_CONN, cid2_name, 0, tx_success_callback);
        tx_cid0 = setup_conn(local_address, remote_address, &tx_ch_cfg, TX_CONN, cid0_name, 1, tx_success_callback);
        tx_cid1 = setup_conn(local_address, remote_address, &tx_ch_cfg, TX_CONN, cid1_name, 1, tx_success_callback);
        rx_cid3 = setup_conn(local_address, remote_address, &rx_ch_cfg, RX_CONN, cid3_name, 1, rx_success_callback);
        rx_cid4 = setup_conn(local_address, remote_address, &rx_ch_cfg, RX_CONN, cid4_name, 1, rx_success_callback);
        break;
    default:
        facade_print_error_string("Encountered unknown certification mode while setting up connections.");
        while (1);
        break;
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

/** @brief Print the available statistics.
 */
static void print_stats(void)
{
    if (device_pairing_state != DEVICE_PAIRED) {
        return;
    }

    static char stats_string[STATS_ARRAY_LENGTH];
    uint32_t total_payload_count = 0;
    int string_length = 0;
    swc_error_t swc_err = SWC_ERR_NONE;

    const char *device_str = "\n\r<  COORDINATOR  >\n\r";
    const char *certification_mode_str = "Cert. Mode %i\r\n";
    const char *app_stats_str = "<<  Connection Priority App Statistics  >>\n\r";
    const char *data_rate_str = "<<< Connections Transmission Rate >>>\n\r";
    const char *total_cid0_str = "Payload Generated on CID0:\t%10lu\n\r";
    const char *total_cid1_str = "Payload Generated on CID1:\t%10lu\n\r";
    const char *total_cid2_str = "Payload Generated on CID2:\t%10lu\n\r";
    const char *payload_sent_str = "  Payload Sent:\t\t\t%10lu (%05.2f%%)\n\r";
    const char *payload_dropped_str = "  Payload Dropped:\t\t%10lu (%05.2f%%)\n\r";
    const char *overview_str = "<<< Connections Transmission Overview >>>\n\r";
    const char *cid0_sent_str = "Payload Sent on CID0:\t\t%10lu (%05.2f%%)\n\r";
    const char *cid1_sent_str = "Payload Sent on CID1:\t\t%10lu (%05.2f%%)\n\r";
    const char *cid2_sent_str = "Payload Sent on CID2:\t\t%10lu (%05.2f%%)\n\r";
    const char *wireless_stats_str = "<<  Wireless Core Statistics  >>\n\r";

    memset(stats_string, 0, sizeof(stats_string));

    swc_connection_update_stats(tx_cid0, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    swc_connection_update_stats(tx_cid1, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    swc_connection_update_stats(tx_cid2, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    swc_connection_update_stats(rx_cid3, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    swc_connection_update_stats(rx_cid4, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    total_payload_count = (cid0_sent_count + cid1_sent_count + cid2_sent_count);

    if (certification_mode != FACADE_CERTIF_NONE) {
        string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length,
                                  certification_mode_str, certification_mode);
    }

    /* Device role */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, device_str);

    /* Application statistics */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, app_stats_str);
    /* Connection transmission rate */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, data_rate_str);
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, total_cid0_str,
                              cid0_sent_count + cid0_dropped_count);
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, payload_sent_str,
                              cid0_sent_count, (double)cid0_sent_count * 100 / (cid0_sent_count + cid0_dropped_count));
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, payload_dropped_str,
                              cid0_dropped_count,
                              (double)cid0_dropped_count * 100 / (cid0_sent_count + cid0_dropped_count));
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, total_cid1_str,
                              cid1_sent_count + cid1_dropped_count);
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, payload_sent_str,
                              cid1_sent_count, (double)cid1_sent_count * 100 / (cid1_sent_count + cid1_dropped_count));
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, payload_dropped_str,
                              cid1_dropped_count,
                              (double)cid1_dropped_count * 100 / (cid1_sent_count + cid1_dropped_count));
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, total_cid2_str,
                              cid2_sent_count + cid2_dropped_count);
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, payload_sent_str,
                              cid2_sent_count, (double)cid2_sent_count * 100 / (cid2_sent_count + cid2_dropped_count));
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, payload_dropped_str,
                              cid2_dropped_count,
                              (double)cid2_dropped_count * 100 / (cid2_sent_count + cid2_dropped_count));
    /* Link capacity utilization */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, overview_str);
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, cid0_sent_str,
                              cid0_sent_count, (double)cid0_sent_count * 100 / total_payload_count);
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, cid1_sent_str,
                              cid1_sent_count, (double)cid1_sent_count * 100 / total_payload_count);
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, cid2_sent_str,
                              cid2_sent_count, (double)cid2_sent_count * 100 / total_payload_count);

    /* Wireless statistics */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, wireless_stats_str);
    string_length += swc_connection_format_stats(tx_cid0, stats_string + string_length,
                                                 sizeof(stats_string) - string_length, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    string_length += swc_connection_format_stats(tx_cid1, stats_string + string_length,
                                                 sizeof(stats_string) - string_length, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    string_length += swc_connection_format_stats(tx_cid2, stats_string + string_length,
                                                 sizeof(stats_string) - string_length, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    string_length += swc_connection_format_stats(rx_cid3, stats_string + string_length,
                                                 sizeof(stats_string) - string_length, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    string_length += swc_connection_format_stats(rx_cid4, stats_string + string_length,
                                                 sizeof(stats_string) - string_length, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    facade_print_string(stats_string);

    if (reset_stats_now) {
        swc_connection_reset_stats(tx_cid0, &swc_err);
        ASSERT_SWC_STATUS(swc_err);

        swc_connection_reset_stats(tx_cid1, &swc_err);
        ASSERT_SWC_STATUS(swc_err);

        swc_connection_reset_stats(tx_cid2, &swc_err);
        ASSERT_SWC_STATUS(swc_err);

        swc_connection_reset_stats(rx_cid3, &swc_err);
        ASSERT_SWC_STATUS(swc_err);

        swc_connection_reset_stats(rx_cid4, &swc_err);
        ASSERT_SWC_STATUS(swc_err);

        cid0_sent_count = 0;
        cid1_sent_count = 0;
        cid2_sent_count = 0;
        cid0_dropped_count = 0;
        cid1_dropped_count = 0;
        cid2_dropped_count = 0;
        reset_stats_now = false;
    }
}

/** @brief Reset the TX and RX statistics.
 */
static void reset_stats(void)
{
    if (device_pairing_state != DEVICE_PAIRED) {
        return;
    }

    if (reset_stats_now == false) {
        reset_stats_now = true;
    }
}

/** @brief Toggles whether payloads are generated on Connection ID 0 by the application or not.
 */
static void stop_cid0_toggle(void)
{
    if (device_pairing_state != DEVICE_PAIRED) {
        return;
    }

    stop_cid0 = !stop_cid0;
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

/** @brief Enter in Pairing Mode using the Pairing Module.
 */
static void enter_pairing_mode(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    pairing_error_t pairing_err = PAIRING_ERR_NONE;
    pairing_event_t pairing_event = PAIRING_EVENT_NONE;

    /* Set the application's state. */
    device_pairing_state = DEVICE_PAIRING;

    facade_notify_enter_pairing();

    /* The Wireless Core must be stopped before starting the pairing procedure. */
    if (swc_get_status() == SWC_STATUS_RUNNING) {
        swc_disconnect(&swc_err);
        ASSERT_SWC_STATUS(swc_err);
    }

    /* Give the information to the Pairing Application. */
    app_pairing_cfg.app_code = PAIRING_APP_CODE;
    app_pairing_cfg.timeout_sec = PAIRING_TIMEOUT_IN_SECONDS;
    app_pairing_cfg.context_switch_callback = facade_context_switch_trigger;
    app_pairing_cfg.application_callback = pairing_application_callback;
    app_pairing_cfg.memory_pool = swc_memory_pool;
    app_pairing_cfg.memory_pool_size = SWC_MEM_POOL_SIZE;
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

/** @brief Unpair the device, this will erase the pairing configuration and stop communication.
 */
static void unpair_device(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;

    device_pairing_state = DEVICE_UNPAIRED;

    memset(pairing_discovery_list, 0, sizeof(pairing_discovery_list));

    /* Disconnect the Wireless Core. */
    swc_disconnect(&swc_err);
    ASSERT_SWC_STATUS(swc_err);

    /* Connection ID 3 (CID3) stop sending packets. */
    facade_packet_rate_timer1_stop();

    /* Connection ID 4 (CID4) stop sending packets. */
    facade_packet_rate_timer2_stop();

    /* Indicate that the device is unpaired and turn off all LEDs. */
    facade_notify_not_paired();
}

/** @brief Application callback called during pairing.
 */
static void pairing_application_callback(void)
{
    /*
     * Note: The button press will only be detected when the pairing module
     *       executes the registered application callback, which might take
     *       a variable amount of time.
     */
    facade_button_handling();
}

/** @brief Abort the pairing procedure.
 */
static void abort_pairing_procedure(void)
{
    pairing_abort();
}

void swc_error_handler(swc_error_t swc_status)
{
    char buffer[ERROR_MESSAGE_BUFFER_SIZE];

    sprintf(buffer, "SWC Error ! Code: %d\n\r", swc_status);
    facade_print_error_string(buffer);

    while (1);
}
