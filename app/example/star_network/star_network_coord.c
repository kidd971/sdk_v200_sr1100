/** @file  star_network_coord.c
 *  @brief This is a basic example on how to use a SPARK star network.
 *         This example uses the SPARK SPARK Wireless Core.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include <stdio.h>
#include "pairing_api.h"
#include "star_network_facade.h"
#include "swc_api.h"
#include "swc_cfg_coord.h"
#include "swc_stats.h"

/* CONSTANTS ******************************************************************/
#define SWC_MEM_POOL_SIZE     10000
#define MAX_PAYLOAD_SIZE_BYTE 12
#define BUTTON_PRESSED        0x01
#define PRINTF_BUF_SIZE_BYTE  64
#define PRINT_INTERVAL_MS     1000
#define STATS_ARRAY_LENGTH    2000

/* TYPES **********************************************************************/
/** @brief Enumeration representing device pairing states.
 */
typedef enum device_pairing_state {
    /*! The device is unpaired. */
    DEVICE_UNPAIRED,
    /*! The device is paired with at least one Node. */
    DEVICE_PAIRED,
} device_pairing_state_t;

/** @brief Data used for transmitting and receiving the button state and an incrementing counter.
 */
typedef struct user_data {
    /*! A boolean indicating the button's state. */
    bool button_state;
    /*! An incrementing counter. */
    uint32_t counter;
} user_data_t;

/* PRIVATE GLOBALS ************************************************************/
/* ** Wireless Core ** */
static uint8_t swc_memory_pool[SWC_MEM_POOL_SIZE];
static swc_node_t *node;
static swc_connection_t *tx_to_node1_conn;
static swc_connection_t *rx_from_node1_conn;
static swc_connection_t *tx_to_node2_conn;
static swc_connection_t *rx_from_node2_conn;

static uint32_t timeslot_us[] = SCHEDULE;
static uint32_t channel_sequence[] = CHANNEL_SEQUENCE;
static uint32_t channel_frequency[] = CHANNEL_FREQ;
static int32_t tx_to_node1_timeslots[] = TX_TO_NODE1_TIMESLOTS;
static int32_t rx_from_node1_timeslots[] = RX_FROM_NODE1_TIMESLOTS;
static int32_t tx_to_node2_timeslots[] = TX_TO_NODE2_TIMESLOTS;
static int32_t rx_from_node2_timeslots[] = RX_FROM_NODE2_TIMESLOTS;

/* Variables supporting pairing between the two devices. */
static bool continuous_pairing = true;
static device_pairing_state_t device_pairing_state;
static pairing_cfg_t app_pairing_cfg;
static pairing_assigned_address_t pairing_assigned_address;
static pairing_discovery_list_t pairing_discovery_list[PAIRING_DISCOVERY_LIST_SIZE];

/* Nodes counter values. */
static volatile uint32_t last_received_node1_counter_value;
static volatile uint32_t last_received_node2_counter_value;

/* PRIVATE FUNCTION PROTOTYPE *************************************************/
static void app_swc_core_init(pairing_assigned_address_t *app_pairing, swc_error_t *err);
static void conn_node1_rx_success_callback(void *conn);
static void conn_node2_rx_success_callback(void *conn);

static void enter_pairing_mode(void);
static void unpair_device(void);

static void pairing_application_callback(void);
static void abort_pairing_procedure(void);
static void app_init(void);

static void wireless_send_data(swc_connection_t *tx_conn, void *transmitted_data, uint8_t size, swc_error_t *swc_err);
static uint16_t wireless_read_data(swc_connection_t *rx_conn, void *received_data, uint8_t size, swc_error_t *swc_err);

static void send_user_data_to_node1(void);
static void send_user_data_to_node2(void);
static void print_stats(void);

/* PUBLIC FUNCTIONS ***********************************************************/
int main(void)
{
    uint32_t tick_start = 0;

    facade_board_init();

    /* Initialize wireless core context switch handler before pairing is available */
    facade_set_context_switch_handler(swc_connection_callbacks_processing_handler);

    device_pairing_state = DEVICE_UNPAIRED;

    /* Pairing occurs automatically when the device boots. */
    while (continuous_pairing) {
        enter_pairing_mode();
    }

    while (1) {
        if (device_pairing_state == DEVICE_UNPAIRED) {
            /* The Coordinator is UNPAIRED if no Node is paired. It is only possible to pair a node. */
            facade_button_handling(enter_pairing_mode, NULL, NULL, NULL);
        } else if (device_pairing_state == DEVICE_PAIRED) {
            /* The Coordinator is PAIRED if there is at least one Node paired. Data streaming is active. */
            facade_button_handling(enter_pairing_mode, unpair_device, NULL, NULL);
            /* Send an incrementing counter and the button state of the SW3 and SW4 to Nodes. */
            send_user_data_to_node1();
            send_user_data_to_node2();
            /* Print received string and stats every PRINT_INTERVAL_MS */
            if ((facade_get_tick_ms() - tick_start) > PRINT_INTERVAL_MS) {
                tick_start += PRINT_INTERVAL_MS;
                print_stats();
            }
        }
    }
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize the Wireless Core.
 *
 *  @param[in]  app_pairing  Configure the Wireless Core with the pairing values.
 *  @param[out] err          Wireless Core error code.
 */
static void app_swc_core_init(pairing_assigned_address_t *app_pairing, swc_error_t *err)
{
    uint8_t remote_address_node_1 = pairing_discovery_list[DEVICE_ROLE_NODE_1].node_address;
    uint8_t remote_address_node_2 = pairing_discovery_list[DEVICE_ROLE_NODE_2].node_address;
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

    /* ** Coordinator sending to Node1 connection ** */
    swc_connection_cfg_t tx_to_node1_conn_cfg = {
        .name = "Coordinator to Node1 connection",
        .source_address = local_address,
        .destination_address = remote_address_node_1,
        .max_payload_size = MAX_PAYLOAD_SIZE_BYTE,
        .queue_size = TX_DATA_QUEUE_SIZE,
        .timeslot_id = tx_to_node1_timeslots,
        .timeslot_count = ARRAY_SIZE(tx_to_node1_timeslots),
    };
    tx_to_node1_conn = swc_connection_init(node, tx_to_node1_conn_cfg, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    swc_channel_cfg_t tx_to_node1_channel_cfg = {
        .tx_pulse_count = TX_DATA_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain = TX_DATA_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT,
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++) {
        tx_to_node1_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(tx_to_node1_conn, node, tx_to_node1_channel_cfg, err);
        if (*err != SWC_ERR_NONE) {
            return;
        }
    }

    /* ** Coordinator receiving from Node1 connection ** */
    swc_connection_cfg_t rx_from_node1_conn_cfg = {
        .name = "Node1 to Coordinator connection",
        .source_address = remote_address_node_1,
        .destination_address = local_address,
        .max_payload_size = MAX_PAYLOAD_SIZE_BYTE,
        .queue_size = RX_DATA_QUEUE_SIZE,
        .timeslot_id = rx_from_node1_timeslots,
        .timeslot_count = ARRAY_SIZE(rx_from_node1_timeslots),
    };
    rx_from_node1_conn = swc_connection_init(node, rx_from_node1_conn_cfg, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    swc_channel_cfg_t rx_from_node1_channel_cfg = {
        .tx_pulse_count = TX_DATA_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain = TX_DATA_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT,
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++) {
        rx_from_node1_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(rx_from_node1_conn, node, rx_from_node1_channel_cfg, err);
        if (*err != SWC_ERR_NONE) {
            return;
        }
    }
    swc_connection_set_rx_success_callback(rx_from_node1_conn, conn_node1_rx_success_callback, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    /* ** Coordinator sending to Node2 connection ** */
    swc_connection_cfg_t tx_to_node2_conn_cfg = {
        .name = "Coordinator to Node2 connection",
        .source_address = local_address,
        .destination_address = remote_address_node_2,
        .max_payload_size = MAX_PAYLOAD_SIZE_BYTE,
        .queue_size = TX_DATA_QUEUE_SIZE,
        .timeslot_id = tx_to_node2_timeslots,
        .timeslot_count = ARRAY_SIZE(tx_to_node2_timeslots),
    };
    tx_to_node2_conn = swc_connection_init(node, tx_to_node2_conn_cfg, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    swc_channel_cfg_t tx_to_node2_channel_cfg = {
        .tx_pulse_count = TX_DATA_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain = TX_DATA_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT,
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++) {
        tx_to_node2_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(tx_to_node2_conn, node, tx_to_node2_channel_cfg, err);
        if (*err != SWC_ERR_NONE) {
            return;
        }
    }

    /* ** Coordinator receiving from Node2 connection ** */
    swc_connection_cfg_t rx_from_node2_conn_cfg = {
        .name = "Node2 to Coordinator connection",
        .source_address = remote_address_node_2,
        .destination_address = local_address,
        .max_payload_size = MAX_PAYLOAD_SIZE_BYTE,
        .queue_size = RX_DATA_QUEUE_SIZE,
        .timeslot_id = rx_from_node2_timeslots,
        .timeslot_count = ARRAY_SIZE(rx_from_node2_timeslots),
    };
    rx_from_node2_conn = swc_connection_init(node, rx_from_node2_conn_cfg, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    swc_channel_cfg_t rx_from_node2_channel_cfg = {
        .tx_pulse_count = TX_DATA_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain = TX_DATA_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT,
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++) {
        rx_from_node2_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(rx_from_node2_conn, node, rx_from_node2_channel_cfg, err);
        if (*err != SWC_ERR_NONE) {
            return;
        }
    }
    swc_connection_set_rx_success_callback(rx_from_node2_conn, conn_node2_rx_success_callback, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    swc_setup(node, err);
}

/** @brief Callback function when a frame has been successfully received from Node1.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 */
static void conn_node1_rx_success_callback(void *conn)
{
    (void)conn;

    swc_error_t swc_err = SWC_ERR_NONE;
    user_data_t received_user_data = {0};

    /* Get received payload. */
    if (wireless_read_data(rx_from_node1_conn, &received_user_data, sizeof(received_user_data), &swc_err) > 0) {
        /* Depending on the requested button state from the Node, the specified LED turns on or off. */
        if (received_user_data.button_state == false) {
            facade_node1_empty_payload_coord_received_status();
        } else {
            facade_node1_payload_coord_received_status();
        }
    }

    last_received_node1_counter_value = received_user_data.counter;
}

/** @brief Callback function when a frame has been successfully received from Node2.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 */
static void conn_node2_rx_success_callback(void *conn)
{
    (void)conn;

    swc_error_t swc_err = SWC_ERR_NONE;
    user_data_t received_user_data = {0};

    /* Get received payload. */
    if (wireless_read_data(rx_from_node2_conn, &received_user_data, sizeof(received_user_data), &swc_err) > 0) {
        /* Depending on the requested button state from the Node, the specified LED turns on or off. */
        if (received_user_data.button_state == false) {
            facade_node2_empty_payload_coord_received_status();
        } else {
            facade_node2_payload_coord_received_status();
        }
    }

    last_received_node2_counter_value = received_user_data.counter;
}

/** @brief Enter in Pairing Mode using the Pairing Module.
 */
static void enter_pairing_mode(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    pairing_error_t pairing_err = PAIRING_ERR_NONE;

    pairing_event_t pairing_event = PAIRING_EVENT_NONE;

    facade_notify_enter_pairing();

    /* The wireless core must be stopped before starting the pairing procedure. */
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
        if (device_pairing_state == DEVICE_PAIRED) {
            /* The two devices are paired. */
            continuous_pairing = false;
        }
        app_init();
        device_pairing_state = DEVICE_PAIRED;

        break;
    case PAIRING_EVENT_TIMEOUT:
    case PAIRING_EVENT_INVALID_APP_CODE:
    case PAIRING_EVENT_ABORT:
    default:
        /* Indicate that the pairing process was unsuccessful. */
        facade_notify_not_paired();
        /* If at least one device is paired, reinitialize the app to ensure proper functionality. */
        if (device_pairing_state == DEVICE_PAIRED) {
            app_init();
        }
        /* There is no device ready for pairing, so the app stops trying to pair. */
        continuous_pairing = false;
        break;
    }
}

/** @brief Unpair the device, this will erase the pairing configuration and stop communication.
 */
static void unpair_device(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;

    memset(pairing_discovery_list, 0, sizeof(pairing_discovery_list));

    /* Disconnect the Wireless Core. */
    swc_disconnect(&swc_err);
    if ((swc_err != SWC_ERR_NONE) && (swc_err != SWC_ERR_NOT_CONNECTED)) {
        while (1);
    }

    device_pairing_state = DEVICE_UNPAIRED;

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
    facade_button_handling(NULL, abort_pairing_procedure, NULL, NULL);
}

/** @brief Abort the pairing procedure.
 */
static void abort_pairing_procedure(void)
{
    pairing_abort();
}

/** @brief Application initialization procedure.
 */
static void app_init(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;

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
}

/** @brief Send data with a specific connection.
 *
 *  @param[in]  tx_conn           Pointer to the connection used for data transmission.
 *  @param[in]  transmitted_data  Data to be sent over the air.
 *  @param[in]  size              Size of the data to be sent over the air.
 *  @param[out] swc_err           Wireless Core error code.
 */
static void wireless_send_data(swc_connection_t *tx_conn, void *transmitted_data, uint8_t size, swc_error_t *swc_err)
{
    uint8_t *buffer = NULL;

    /* Get buffer from queue to hold data. */
    swc_connection_get_payload_buffer(tx_conn, &buffer, swc_err);
    if ((*swc_err != SWC_ERR_NONE) || (buffer == NULL)) {
        return;
    }

    /* Format the new payload. */
    if (transmitted_data != NULL) {
        memcpy(buffer, transmitted_data, size);
    }

    /* Send the payload through the Wireless Core. */
    swc_connection_send(tx_conn, buffer, size, swc_err);
}

/** @brief Read data from a specific connection.
 *
 *  @param[in]  rx_conn        Pointer to the connection used for data reception.
 *  @param[out] received_data  Pointer to data buffer to write to.
 *  @param[in]  size           Size of the data buffer.
 *  @param[out] swc_err        Wireless Core error code.
 *  @return Size of the data read.
 */
static uint16_t wireless_read_data(swc_connection_t *rx_conn, void *received_data, uint8_t size, swc_error_t *swc_err)
{
    uint8_t *payload = NULL;
    uint16_t payload_size = 0;

    /* Read received data. */
    payload_size = swc_connection_receive(rx_conn, &payload, swc_err);
    if (*swc_err != SWC_ERR_NONE || payload_size > size) {
        return 0;
    }

    if (received_data != NULL) {
        memcpy(received_data, payload, payload_size);
    }

    /* Free the payload memory. */
    swc_connection_receive_complete(rx_conn, swc_err);

    return payload_size;
}

/** @brief Transmit user data to Node 1.
 *
 *  This function sends a data packet containing the state of button SW2
 *  and an incrementing counter to Node 1.
 */
static void send_user_data_to_node1(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    user_data_t data_to_transmit = {0};
    static unsigned long inc_coord;

    data_to_transmit.button_state = facade_coord_read_user_button_1_state();
    data_to_transmit.counter = inc_coord++;

    wireless_send_data(tx_to_node1_conn, &data_to_transmit, sizeof(data_to_transmit), &swc_err);
}

/** @brief Transmit user data to Node 2.
 *
 *  This function sends a data packet containing the state of button SW4
 *  and an incrementing counter to Node 2.
 */
static void send_user_data_to_node2(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    user_data_t data_to_transmit = {0};
    static unsigned long inc_coord;

    data_to_transmit.button_state = facade_coord_read_user_button_2_state();
    data_to_transmit.counter = inc_coord++;

    wireless_send_data(tx_to_node2_conn, &data_to_transmit, sizeof(data_to_transmit), &swc_err);
}

/** @brief Print the available statistics.
 */
static void print_stats(void)
{
    static char stats_string[STATS_ARRAY_LENGTH];
    int string_length = 0;

    const char *device_str = "\n\r<  COORDINATOR  >\n\r";
    const char *app_stats_str = "<<  Star Network App Statistics  >>\n\r";
    const char *node1_counter_str = "Node 1 counter value:\t%10lu\n\r";
    const char *node2_counter_str = "Node 2 counter value:\t%10lu\n\r";
    const char *wireless_stats_str = "<<  Wireless Core Statistics  >>\n\r";

    memset(stats_string, 0, sizeof(stats_string));

    swc_connection_update_stats(tx_to_node1_conn);
    swc_connection_update_stats(tx_to_node2_conn);
    swc_connection_update_stats(rx_from_node1_conn);
    swc_connection_update_stats(rx_from_node2_conn);

    /* Device role */
    string_length = snprintf(stats_string, sizeof(stats_string), device_str);

    /* Application statistics */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, app_stats_str);
    /* Connection transmission rate */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, node1_counter_str,
                              last_received_node1_counter_value);
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, node2_counter_str,
                              last_received_node2_counter_value);

    /* Wireless statistics */
    string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, wireless_stats_str);
    string_length += swc_connection_format_stats(tx_to_node1_conn, node, stats_string + string_length,
                                                 sizeof(stats_string) - string_length);
    string_length += swc_connection_format_stats(tx_to_node2_conn, node, stats_string + string_length,
                                                 sizeof(stats_string) - string_length);
    string_length += swc_connection_format_stats(rx_from_node1_conn, node, stats_string + string_length,
                                                 sizeof(stats_string) - string_length);
    string_length += swc_connection_format_stats(rx_from_node2_conn, node, stats_string + string_length,
                                                 sizeof(stats_string) - string_length);

    facade_print_string(stats_string);
}
