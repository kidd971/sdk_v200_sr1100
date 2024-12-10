/** @file  connection_priority_coord.c
 *  @brief This is a basic example of how to use the Wireless Core connection priority.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include <stdio.h>
#include "connection_priority_facade.h"
#include "pairing_api.h"
#include "swc_api.h"
#include "swc_cfg_coord.h"
#include "swc_stats.h"

/* CONSTANTS ******************************************************************/
#define SWC_MEM_POOL_SIZE           12000
#define MAX_SMALL_PAYLOAD_SIZE_BYTE 8
#define MAX_BIG_PAYLOAD_SIZE_BYTE   15
#define STATS_ARRAY_LENGTH          8000
#define TIMER1_PACKET_RATE_US       2000
#define TIMER2_PACKET_RATE_US       2500
#define TIMER_STAT_MS               1000
#define TX_CID0_PRIORITY            0
#define TX_CID1_PRIORITY            1
#define TX_CID2_PRIORITY            2
#define RX_PRIORITY                 0

/* The device roles are used for the pairing discovery list. */
#define DEVICE_ROLE_COORDINATOR 0
#define DEVICE_ROLE_NODE        1

/* The discovery list includes the coordinator and the node. */
#define PAIRING_DISCOVERY_LIST_SIZE 2
/* The application code prevents unwanted devices from pairing with this application. */
#define PAIRING_APP_CODE 0x0000000000000333
/* The timeout in second after which the pairing procedure will abort. */
#define PAIRING_TIMEOUT_IN_SECONDS 10

/* PRIVATE GLOBALS ************************************************************/
/* ** Wireless Core ** */
static uint8_t swc_memory_pool[SWC_MEM_POOL_SIZE];
static swc_node_t *node;
static swc_connection_t *tx_cid0;
static swc_connection_t *tx_cid1;
static swc_connection_t *tx_cid2;
static swc_connection_t *rx_cid3;
static swc_connection_t *rx_cid4;

static uint32_t timeslot_us[]       = SCHEDULE;
static uint32_t channel_sequence[]  = CHANNEL_SEQUENCE;
static uint32_t channel_frequency[] = CHANNEL_FREQ;
static int32_t tx_timeslots[]       = TX_TIMESLOTS;
static int32_t rx_timeslots[]       = RX_TIMESLOTS;

/* ** Application Specific ** */
static uint32_t cid0_sent_count;
static uint32_t cid1_sent_count;
static uint32_t cid2_sent_count;
static uint32_t cid0_dropped_count;
static uint32_t cid1_dropped_count;
static uint32_t cid2_dropped_count;

static volatile bool print_stats_now;
static volatile bool reset_stats_now;
static volatile bool stop_cid0;

static bool device_state_paired;
static pairing_cfg_t app_pairing_cfg;
static pairing_assigned_address_t pairing_assigned_address;
static pairing_discovery_list_t pairing_discovery_list[PAIRING_DISCOVERY_LIST_SIZE];

/* PRIVATE FUNCTION PROTOTYPE *************************************************/
static void app_swc_core_init(pairing_assigned_address_t *app_pairing, swc_error_t *err);
static void multi_conn_tx_send_callback(void);
static void rx_success_callback(void *conn);
static void single_conn_tx_send_callback(void);

static void print_stats(void);
static void reset_stats(void);
static void stats_callback(void);
static void stop_cid0_toggle(void);

static void enter_pairing_mode(void);
static void unpair_device(void);

static void pairing_application_callback(void);
static void abort_pairing_procedure(void);

/* PUBLIC FUNCTIONS ***********************************************************/
int main(void)
{
    facade_board_init();

    /* Connection ID 0 (CID0) and Connection ID 2 (CID2) send 500 pkt/s. */
    facade_packet_rate_timer1_init(TIMER1_PACKET_RATE_US);
    facade_packet_rate_set_timer1_callback(multi_conn_tx_send_callback);

    /* Connection ID 1 (CID1) sends 400 pkt/s. */
    facade_packet_rate_timer2_init(TIMER2_PACKET_RATE_US);
    facade_packet_rate_set_timer2_callback(single_conn_tx_send_callback);

    facade_stats_timer_init(TIMER_STAT_MS);
    facade_stats_set_timer_callback(stats_callback);
    facade_stats_timer_start();

    /* Initialize wireless core context switch handler before pairing is available */
    facade_set_context_switch_handler(swc_connection_callbacks_processing_handler);

    while (1) {
        if (!device_state_paired) {
            /* When the device is not paired, the only action possible for the user is the pairing. */
            facade_button_handling(enter_pairing_mode, NULL, NULL, NULL);
        } else {
            /* When the device is paired, normal operations are executed. */
            facade_button_handling(unpair_device, reset_stats, stop_cid0_toggle, NULL);
            if (print_stats_now) {
                if (reset_stats_now) {
                    swc_connection_reset_stats(tx_cid0);
                    swc_connection_reset_stats(tx_cid1);
                    swc_connection_reset_stats(tx_cid2);
                    swc_connection_reset_stats(rx_cid3);
                    swc_connection_reset_stats(rx_cid4);
                    cid0_sent_count = 0;
                    cid1_sent_count = 0;
                    cid2_sent_count = 0;
                    cid0_dropped_count = 0;
                    cid1_dropped_count = 0;
                    cid2_dropped_count = 0;
                    reset_stats_now = false;
                } else {
                    print_stats();
                }
                print_stats_now = false;
            }
        }
    }

    return 0;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize the Wireless Core.
 *
 *  @param[in]  app_pairing  Configure the Wireless Core with the pairing values.
 *  @param[out] err          Wireless Core error code.
 */
static void app_swc_core_init(pairing_assigned_address_t *app_pairing, swc_error_t *err)
{
    uint8_t remote_address = pairing_discovery_list[DEVICE_ROLE_NODE].node_address;
    uint8_t local_address = pairing_discovery_list[DEVICE_ROLE_COORDINATOR].node_address;

    swc_cfg_t core_cfg = {
        .timeslot_sequence = timeslot_us,
        .timeslot_sequence_length = ARRAY_SIZE(timeslot_us),
        .channel_sequence = channel_sequence,
        .channel_sequence_length = ARRAY_SIZE(channel_sequence),
        .concurrency_mode = SWC_CONCURRENCY_MODE_HIGH_PERFORMANCE,
        .memory_pool = swc_memory_pool,
        .memory_pool_size = SWC_MEM_POOL_SIZE,
    };
    swc_init(core_cfg, facade_context_switch_trigger, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    swc_node_cfg_t node_cfg = {
        .role = NETWORK_ROLE,
        .pan_id = app_pairing->pan_id,
        .coordinator_address = app_pairing->coordinator_address,
        .local_address = local_address,
    };
    node = swc_node_init(node_cfg, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    swc_radio_module_init(node, SWC_RADIO_ID_1, true, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    swc_channel_cfg_t rx_channel_cfg = {
        .tx_pulse_count = TX_DATA_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain  = TX_DATA_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT,
    };
    swc_channel_cfg_t tx_channel_cfg = {
        .tx_pulse_count = TX_DATA_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain  = TX_DATA_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT,
    };

    /* ** Coordinator sending to Node Connection ID 0 ** */
    swc_connection_cfg_t tx_cfg_cid0 = {
        .name = "TX CID0 to Node",
        .source_address = local_address,
        .destination_address = remote_address,
        .max_payload_size = MAX_BIG_PAYLOAD_SIZE_BYTE,
        .queue_size = TX_DATA_QUEUE_SIZE,
        .timeslot_id = tx_timeslots,
        .timeslot_count = ARRAY_SIZE(tx_timeslots),
    };

    tx_cid0 = swc_connection_init(node, tx_cfg_cid0, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }
    swc_connection_set_connection_priority(node, tx_cid0, TX_CID0_PRIORITY, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++) {
        tx_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(tx_cid0, node, tx_channel_cfg, err);
        if (*err != SWC_ERR_NONE) {
            return;
        }
    }

    /* ** Coordinator sending to Node Connection ID 1 ** */
    swc_connection_cfg_t tx_cfg_cid1 = {
        .name = "TX CID1 to Node",
        .source_address = local_address,
        .destination_address = remote_address,
        .max_payload_size = MAX_SMALL_PAYLOAD_SIZE_BYTE,
        .queue_size = TX_DATA_QUEUE_SIZE,
        .timeslot_id = tx_timeslots,
        .timeslot_count = ARRAY_SIZE(tx_timeslots),
    };

    tx_cid1 = swc_connection_init(node, tx_cfg_cid1, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }
    swc_connection_set_connection_priority(node, tx_cid1, TX_CID1_PRIORITY, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++) {
        tx_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(tx_cid1, node, tx_channel_cfg, err);
        if (*err != SWC_ERR_NONE) {
            return;
        }
    }

    /* ** Coordinator sending to Node Connection ID 2 ** */
    swc_connection_cfg_t tx_cfg_cid2 = {
        .name = "TX CID2 to Node",
        .source_address = local_address,
        .destination_address = remote_address,
        .max_payload_size = MAX_BIG_PAYLOAD_SIZE_BYTE,
        .queue_size = TX_DATA_QUEUE_SIZE,
        .timeslot_id = tx_timeslots,
        .timeslot_count = ARRAY_SIZE(tx_timeslots),
    };

    tx_cid2 = swc_connection_init(node, tx_cfg_cid2, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }
    swc_connection_set_connection_priority(node, tx_cid2, TX_CID2_PRIORITY, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++) {
        tx_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(tx_cid2, node, tx_channel_cfg, err);
        if (*err != SWC_ERR_NONE) {
            return;
        }
    }

    /* ** Coordinator receiving from Node Connection ID 3 ** */
    swc_connection_cfg_t rx_cfg_cid3 = {
        .name = "RX CID3 from Node",
        .source_address = remote_address,
        .destination_address = local_address,
        .max_payload_size = MAX_BIG_PAYLOAD_SIZE_BYTE,
        .queue_size = RX_DATA_QUEUE_SIZE,
        .timeslot_id = rx_timeslots,
        .timeslot_count = ARRAY_SIZE(rx_timeslots),
    };

    rx_cid3 = swc_connection_init(node, rx_cfg_cid3, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }
    swc_connection_set_connection_priority(node, rx_cid3, RX_PRIORITY, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++) {
        rx_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(rx_cid3, node, rx_channel_cfg, err);
        if (*err != SWC_ERR_NONE) {
            return;
        }
    }

    swc_connection_set_rx_success_callback(rx_cid3, rx_success_callback, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }
    /* ** Coordinator receiving from Node Connection ID 4 ** */
    swc_connection_cfg_t rx_cfg_cid4 = {
        .name = "RX CID4 from Node",
        .source_address = remote_address,
        .destination_address = local_address,
        .max_payload_size = MAX_SMALL_PAYLOAD_SIZE_BYTE,
        .queue_size = RX_DATA_QUEUE_SIZE,
        .timeslot_id = rx_timeslots,
        .timeslot_count = ARRAY_SIZE(rx_timeslots),
    };

    rx_cid4 = swc_connection_init(node, rx_cfg_cid4, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }
    swc_connection_set_connection_priority(node, rx_cid4, RX_PRIORITY, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++) {
        rx_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(rx_cid4, node, rx_channel_cfg, err);
        if (*err != SWC_ERR_NONE) {
            return;
        }
    }

    swc_connection_set_rx_success_callback(rx_cid4, rx_success_callback, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }
    swc_setup(node, err);
}

/** @brief Callback function when it is time to send payloads from multiple connections.
 *
 *  Payloads are sent on Connection ID 0 (CID0) and Connection ID 2 (CID2).
 */
static void multi_conn_tx_send_callback(void)
{
    swc_error_t swc_err;
    uint8_t *payload_buf;

    if (!stop_cid0) {
        swc_connection_allocate_payload_buffer(tx_cid0, &payload_buf, MAX_BIG_PAYLOAD_SIZE_BYTE, &swc_err);
        if (payload_buf != NULL) {
            snprintf((char *)payload_buf, MAX_BIG_PAYLOAD_SIZE_BYTE, "%s", "CID0");
            swc_connection_send(tx_cid0, payload_buf, MAX_BIG_PAYLOAD_SIZE_BYTE, &swc_err);
            cid0_sent_count++;
        } else {
            cid0_dropped_count++;
        }
    }

    swc_connection_allocate_payload_buffer(tx_cid2, &payload_buf, MAX_BIG_PAYLOAD_SIZE_BYTE, &swc_err);
    if (payload_buf != NULL) {
        snprintf((char *)payload_buf, MAX_BIG_PAYLOAD_SIZE_BYTE, "%s", "CID2");
        swc_connection_send(tx_cid2, payload_buf, MAX_BIG_PAYLOAD_SIZE_BYTE, &swc_err);
        cid2_sent_count++;
    } else {
        cid2_dropped_count++;
    }
}

/** @brief Callback function when a frame has been successfully received.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 */
static void rx_success_callback(void *conn)
{
    swc_error_t err;

    swc_connection_receive_complete(conn, &err);
}

/** @brief Callback function when it is time to send a payload from a single connection.
 *
 *  Payload is sent on Connection ID 1 (CID1).
 */
static void single_conn_tx_send_callback(void)
{
    swc_error_t swc_err;
    uint8_t *payload_buf;

    swc_connection_allocate_payload_buffer(tx_cid1, &payload_buf, MAX_SMALL_PAYLOAD_SIZE_BYTE, &swc_err);
    if (payload_buf != NULL) {
        snprintf((char *)payload_buf, MAX_SMALL_PAYLOAD_SIZE_BYTE, "%s", "CID1");
        swc_connection_send(tx_cid1, payload_buf, MAX_SMALL_PAYLOAD_SIZE_BYTE, &swc_err);
        cid1_sent_count++;
    } else {
        cid1_dropped_count++;
    }
}

/** @brief Print the available statistics.
 */
static void print_stats(void)
{
    char stats_string[STATS_ARRAY_LENGTH];
    uint32_t total_payload_count;
    int string_length = 0;

    const char *device_str          = "\n\r<  COORDINATOR  >\n\r";
    const char *app_stats_str       = "<<  Connection Priority App Statistics  >>\n\r";
    const char *data_rate_str       = "<<< Connections Transmission Rate >>>\n\r";
    const char *total_cid0_str      = "Payload Generated on CID0:\t%10lu\n\r";
    const char *total_cid1_str      = "Payload Generated on CID1:\t%10lu\n\r";
    const char *total_cid2_str      = "Payload Generated on CID2:\t%10lu\n\r";
    const char *payload_sent_str    = "  Payload Sent:\t\t\t%10lu (%05.2f%%)\n\r";
    const char *payload_dropped_str = "  Payload Dropped:\t\t%10lu (%05.2f%%)\n\r";
    const char *overview_str        = "<<< Connections Transmission Overview >>>\n\r";
    const char *cid0_sent_str       = "Payload Sent on CID0:\t\t%10lu (%05.2f%%)\n\r";
    const char *cid1_sent_str       = "Payload Sent on CID1:\t\t%10lu (%05.2f%%)\n\r";
    const char *cid2_sent_str       = "Payload Sent on CID2:\t\t%10lu (%05.2f%%)\n\r";
    const char *wireless_stats_str  = "<<  Wireless Core Statistics  >>\n\r";

    swc_connection_update_stats(tx_cid0);
    swc_connection_update_stats(tx_cid1);
    swc_connection_update_stats(tx_cid2);
    swc_connection_update_stats(rx_cid3);
    swc_connection_update_stats(rx_cid4);

    total_payload_count = (cid0_sent_count + cid1_sent_count + cid2_sent_count);

    /* Device role */
    string_length = snprintf(stats_string, sizeof(stats_string), device_str);

    /* Application statistics */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, app_stats_str);
    /* Connection transmission rate */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, data_rate_str);
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, total_cid0_str,
                              cid0_sent_count + cid0_dropped_count);
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, payload_sent_str, cid0_sent_count,
                              (double)cid0_sent_count * 100 / (cid0_sent_count + cid0_dropped_count));
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, payload_dropped_str, cid0_dropped_count,
                              (double)cid0_dropped_count * 100 / (cid0_sent_count + cid0_dropped_count));
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, total_cid1_str,
                              cid1_sent_count + cid1_dropped_count);
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, payload_sent_str, cid1_sent_count,
                              (double)cid1_sent_count * 100 / (cid1_sent_count + cid1_dropped_count));
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, payload_dropped_str, cid1_dropped_count,
                              (double)cid1_dropped_count * 100 / (cid1_sent_count + cid1_dropped_count));
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, total_cid2_str,
                              cid2_sent_count + cid2_dropped_count);
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, payload_sent_str, cid2_sent_count,
                              (double)cid2_sent_count * 100 / (cid2_sent_count + cid2_dropped_count));
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, payload_dropped_str, cid2_dropped_count,
                              (double)cid2_dropped_count * 100 / (cid2_sent_count + cid2_dropped_count));
    /* Link capacity utilization */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, overview_str);
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, cid0_sent_str, cid0_sent_count,
                              (double)cid0_sent_count * 100 / total_payload_count);
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, cid1_sent_str, cid1_sent_count,
                              (double)cid1_sent_count * 100 / total_payload_count);
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, cid2_sent_str, cid2_sent_count,
                              (double)cid2_sent_count * 100 / total_payload_count);

    /* Wireless statistics */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, wireless_stats_str);
    string_length += swc_connection_format_stats(tx_cid0, node, stats_string + string_length, sizeof(stats_string) - string_length);
    string_length += swc_connection_format_stats(tx_cid1, node, stats_string + string_length, sizeof(stats_string) - string_length);
    string_length += swc_connection_format_stats(tx_cid2, node, stats_string + string_length, sizeof(stats_string) - string_length);
    string_length += swc_connection_format_stats(rx_cid3, node, stats_string + string_length, sizeof(stats_string) - string_length);
    string_length += swc_connection_format_stats(rx_cid4, node, stats_string + string_length, sizeof(stats_string) - string_length);

    facade_print_string(stats_string);
}

/** @brief Reset the TX and RX statistics.
 */
static void reset_stats(void)
{
    if (reset_stats_now == false) {
        reset_stats_now = true;
    }
}

/** @brief Callback handling when the stats have to be printed.
 */
static void stats_callback(void)
{
    print_stats_now = true;
}

/** @brief Toggles whether payloads are generated on Connection ID 0 by the application or not.
 */
static void stop_cid0_toggle(void)
{
    stop_cid0 = !stop_cid0;
}

/** @brief Enter in Pairing Mode using the Pairing Module.
 */
static void enter_pairing_mode(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    pairing_error_t pairing_err = PAIRING_ERR_NONE;

    pairing_event_t pairing_event;

    facade_notify_enter_pairing();

    /* The Wireless Core must be stopped before starting the pairing procedure. */
    if (swc_get_status() == SWC_STATUS_RUNNING) {
        swc_disconnect(&swc_err);
        if ((swc_err != SWC_ERR_NONE) && (swc_err != SWC_ERR_NOT_CONNECTED)) {
            while (1);
        }
    }

    /* Give the information to the Pairing Application. */
    app_pairing_cfg.app_code = PAIRING_APP_CODE;
    app_pairing_cfg.timeout_sec = PAIRING_TIMEOUT_IN_SECONDS;
    app_pairing_cfg.context_switch_callback = facade_context_switch_trigger;
    app_pairing_cfg.application_callback = pairing_application_callback;
    app_pairing_cfg.memory_pool = swc_memory_pool;
    app_pairing_cfg.memory_pool_size = SWC_MEM_POOL_SIZE;
    app_pairing_cfg.uwb_regulation = SWC_REGULATION_FCC;
    pairing_event = pairing_coordinator_start(&app_pairing_cfg, &pairing_assigned_address, pairing_discovery_list,
                                              PAIRING_DISCOVERY_LIST_SIZE, &pairing_err);
    if (pairing_err != PAIRING_ERR_NONE) {
        while (1);
    }

    /* Handle the pairing events. */
    switch (pairing_event) {
    case PAIRING_EVENT_SUCCESS:
        /* Indicate that the pairing process was successful. */
        facade_notify_pairing_successful();

        /* Reconfigure the Coordinator and Node addresses. */
        app_swc_core_init(&pairing_assigned_address, &swc_err);
        if (swc_err != SWC_ERR_NONE) {
            while (1);
        }

        swc_connect(&swc_err);
        if (swc_err != SWC_ERR_NONE) {
            while (1);
        }

        /* Connection ID 0 (CID0) and Connection ID 2 (CID2) start sending packet. */
        facade_packet_rate_timer1_start();

        /* Connection ID 1 (CID1) sends 400 pkt/s start sending packet. */
        facade_packet_rate_timer2_start();

        device_state_paired = true;

        break;
    case PAIRING_EVENT_TIMEOUT:
    case PAIRING_EVENT_INVALID_APP_CODE:
    case PAIRING_EVENT_ABORT:
    default:
        /* Indicate that the pairing process was unsuccessful. */
        facade_notify_not_paired();
        device_state_paired = false;
        break;
    }
}

/** @brief Unpair the device, this will erase the pairing configuration and stop communication.
 */
static void unpair_device(void)
{
    swc_error_t swc_err;

    device_state_paired = false;

    memset(pairing_discovery_list, 0, sizeof(pairing_discovery_list));

    /* Disconnect the Wireless Core. */
    swc_disconnect(&swc_err);
    if ((swc_err != SWC_ERR_NONE) && (swc_err != SWC_ERR_NOT_CONNECTED)) {
        while (1);
    }

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
    facade_button_handling(abort_pairing_procedure, NULL, NULL, NULL);
}

/** @brief Abort the pairing procedure.
 */
static void abort_pairing_procedure(void)
{
    pairing_abort();
}
