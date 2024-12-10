/** @file  pairing_wireless.c
 *  @brief This file handles the wireless management for pairing module.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "pairing_wireless.h"
#include "pairing_error.h"
#include "pairing_wireless_cfg.h"
#include "swc_utils.h"
#include "wps_config.h"

/* PRIVATE GLOBALS ************************************************************/
static swc_node_t *node;
static swc_connection_t *coord_to_node_conn;
static swc_connection_t *node_to_coord_conn;
static swc_role_t local_network_role;

static pairing_cfg_t *app_pairing_cfg;
static pairing_cfg_t local_pairing_cfg;

static uint64_t local_radio_serial_number;

static uint32_t timeslot_us[] = PAIRING_SCHEDULE;
static uint32_t channel_sequence[] = PAIRING_CHANNEL_SEQUENCE;
static uint32_t *channel_frequency;
static uint32_t channel_frequency_fcc_etsi[] = PAIRING_CHANNEL_FREQ_FCC_ETSI;
static uint32_t channel_frequency_arib[] = PAIRING_CHANNEL_FREQ_ARIB;
static int32_t coord_to_node_timeslots[] = COORD_TO_NODE_TIMESLOTS;
static int32_t node_to_coord_timeslots[] = NODE_TO_COORD_TIMESLOTS;

static void (*sent_message_callback)(void);
static void (*received_message_callback)(uint8_t *received_message, uint8_t payload_size);

/* PRIVATE FUNCTIONS PROTOTYPE ************************************************/
static void conn_tx_success_callback(void *conn);
static void conn_rx_success_callback(void *conn);
static void store_radio_serial_number(void);
static void handle_wireless_error(void);

/* PUBLIC FUNCTIONS ***********************************************************/
void pairing_wireless_init(pairing_cfg_t *pairing_cfg, swc_role_t network_role)
{
    swc_error_t err = SWC_ERR_NONE;
    uint16_t local_address;

    local_network_role = network_role;

    /* Copy the pairing configuration locally. */
    app_pairing_cfg = pairing_cfg;
    local_pairing_cfg = *app_pairing_cfg;

    /* Set pairing channels with the desired region. */
    switch (local_pairing_cfg.uwb_regulation) {
    case SWC_REGULATION_FCC:
    case SWC_REGULATION_ETSI:
        channel_frequency = channel_frequency_fcc_etsi;
        break;
    case SWC_REGULATION_ARIB:
        channel_frequency = channel_frequency_arib;
        break;
    default:
        break;
    }

    swc_cfg_t core_cfg = {
        .timeslot_sequence               = timeslot_us,
        .timeslot_sequence_length        = SWC_ARRAY_SIZE(timeslot_us),
        .channel_sequence                = channel_sequence,
        .channel_sequence_length         = SWC_ARRAY_SIZE(channel_sequence),
        .concurrency_mode                = SWC_CONCURRENCY_MODE_LOW_PERFORMANCE,
        .memory_pool                     = app_pairing_cfg->memory_pool,
        .memory_pool_size                = app_pairing_cfg->memory_pool_size,
    };

    swc_init(core_cfg, app_pairing_cfg->context_switch_callback, &err);
    if (err != SWC_ERR_NONE) {
        handle_wireless_error();
        return;
    }

    /* Update the addresses depending on the role. */
    if (network_role == SWC_ROLE_COORDINATOR) {
        local_address = PAIRING_COORD_ADDRESS;
    } else {
        local_address = PAIRING_NODE_ADDRESS;
    }

    swc_node_cfg_t node_cfg = {
        .role                = network_role,
        .pan_id              = PAIRING_PAN_ID,
        .coordinator_address = PAIRING_COORD_ADDRESS,
        .local_address       = local_address,
    };
    node = swc_node_init(node_cfg, &err);
    if (err != SWC_ERR_NONE) {
        handle_wireless_error();
        return;
    }

    swc_radio_module_init(node, SWC_RADIO_ID_1, true, &err);
    if (err != SWC_ERR_NONE) {
        handle_wireless_error();
        return;
    }
#if (WPS_RADIO_COUNT == 2)

    swc_radio_module_init(node, SWC_RADIO_ID_2, true, &err);
    if (err != SWC_ERR_NONE) {
        handle_wireless_error();
        return;
    }
#endif

    /* Coordinator to node connection. */
    swc_connection_cfg_t coord_to_node_conn_cfg = {
        .name                       = "Coord to Node Connection",
        .source_address             = PAIRING_COORD_ADDRESS,
        .destination_address        = PAIRING_NODE_ADDRESS,
        .max_payload_size           = PAIRING_MAX_PAYLOAD_SIZE,
        .queue_size                 = PAIRING_DATA_QUEUE_SIZE,
        .timeslot_id                = coord_to_node_timeslots,
        .timeslot_count             = SWC_ARRAY_SIZE(coord_to_node_timeslots),
    };
    coord_to_node_conn = swc_connection_init(node, coord_to_node_conn_cfg, &err);
    if (err != SWC_ERR_NONE) {
        handle_wireless_error();
        return;
    }
    swc_connection_set_auto_sync(coord_to_node_conn, true, &err);
    if (err != SWC_ERR_NONE) {
        handle_wireless_error();
        return;
    }

    /* Node to coordinator connection. */
    swc_connection_cfg_t node_to_coord_conn_cfg = {
        .name                       = "Node to Coord Connection",
        .source_address             = PAIRING_NODE_ADDRESS,
        .destination_address        = PAIRING_COORD_ADDRESS,
        .max_payload_size           = PAIRING_MAX_PAYLOAD_SIZE,
        .queue_size                 = PAIRING_DATA_QUEUE_SIZE,
        .timeslot_id                = node_to_coord_timeslots,
        .timeslot_count             = SWC_ARRAY_SIZE(node_to_coord_timeslots),
    };
    node_to_coord_conn = swc_connection_init(node, node_to_coord_conn_cfg, &err);
    if (err != SWC_ERR_NONE) {
        handle_wireless_error();
        return;
    }
    swc_connection_set_auto_sync(node_to_coord_conn, true, &err);
    if (err != SWC_ERR_NONE) {
        handle_wireless_error();
        return;
    }

    swc_channel_cfg_t tx_channel_cfg = {
        .tx_pulse_count = PAIRING_TX_DATA_PULSE_COUNT,
        .tx_pulse_width = PAIRING_TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain  = PAIRING_TX_DATA_PULSE_GAIN,
        .rx_pulse_count = PAIRING_RX_ACK_PULSE_COUNT,
    };

    swc_channel_cfg_t rx_channel_cfg = {
        .tx_pulse_count = PAIRING_TX_ACK_PULSE_COUNT,
        .tx_pulse_width = PAIRING_TX_ACK_PULSE_WIDTH,
        .tx_pulse_gain  = PAIRING_TX_ACK_PULSE_GAIN,
        .rx_pulse_count = PAIRING_RX_DATA_PULSE_COUNT,
    };

    /* Add the channels to the connections and set the connections callbacks. */
    if (local_network_role == SWC_ROLE_COORDINATOR) {
        for (uint8_t i = 0; i < SWC_ARRAY_SIZE(channel_sequence); i++) {
            tx_channel_cfg.frequency = channel_frequency[i];
            swc_connection_add_channel(coord_to_node_conn, node, tx_channel_cfg, &err);
            if (err != SWC_ERR_NONE) {
                handle_wireless_error();
                return;
            }

            rx_channel_cfg.frequency = channel_frequency[i];
            swc_connection_add_channel(node_to_coord_conn, node, rx_channel_cfg, &err);
            if (err != SWC_ERR_NONE) {
                handle_wireless_error();
                return;
            }
        }
        swc_connection_set_tx_success_callback(coord_to_node_conn, conn_tx_success_callback, &err);
        if (err != SWC_ERR_NONE) {
            handle_wireless_error();
            return;
        }
        swc_connection_set_rx_success_callback(node_to_coord_conn, conn_rx_success_callback, &err);
        if (err != SWC_ERR_NONE) {
            handle_wireless_error();
            return;
        }
    } else {
        for (uint8_t i = 0; i < SWC_ARRAY_SIZE(channel_sequence); i++) {
            tx_channel_cfg.frequency = channel_frequency[i];
            swc_connection_add_channel(node_to_coord_conn, node, tx_channel_cfg, &err);
            if (err != SWC_ERR_NONE) {
                handle_wireless_error();
                return;
            }

            rx_channel_cfg.frequency = channel_frequency[i];
            swc_connection_add_channel(coord_to_node_conn, node, rx_channel_cfg, &err);
            if (err != SWC_ERR_NONE) {
                handle_wireless_error();
                return;
            }
        }
        swc_connection_set_tx_success_callback(node_to_coord_conn, conn_tx_success_callback, &err);
        if (err != SWC_ERR_NONE) {
            handle_wireless_error();
            return;
        }
        swc_connection_set_rx_success_callback(coord_to_node_conn, conn_rx_success_callback, &err);
        if (err != SWC_ERR_NONE) {
            handle_wireless_error();
            return;
        }
    }

    swc_setup(node, &err);
    if (err != SWC_ERR_NONE) {
        handle_wireless_error();
        return;
    }

    /* Saves the radio serial number before connecting the device. */
    store_radio_serial_number();
}

void pairing_wireless_connect(void)
{
    swc_error_t err;

    swc_connect(&err);
    if (err != SWC_ERR_NONE) {
        handle_wireless_error();
        return;
    }
}

void pairing_wireless_disconnect(void)
{
    swc_error_t err;

    swc_disconnect(&err);
    if (err != SWC_ERR_NONE) {
        handle_wireless_error();
        return;
    }
}

void pairing_wireless_free_memory(void)
{
    swc_free_memory();
}

void pairing_wireless_send_message(uint8_t *payload_buffer, uint16_t size)
{
    swc_error_t err;

    if (local_network_role == SWC_ROLE_COORDINATOR) {
        swc_connection_send(coord_to_node_conn, payload_buffer, size, &err);
    } else if (local_network_role == SWC_ROLE_NODE) {
        swc_connection_send(node_to_coord_conn, payload_buffer, size, &err);
    }

    if (err != SWC_ERR_NONE) {
        handle_wireless_error();
        return;
    }
}

uint64_t pairing_wireless_get_radio_serial_number(void)
{
    return local_radio_serial_number;
}

swc_role_t pairing_wireless_get_network_role(void)
{
    return local_network_role;
}

void pairing_wireless_set_sent_message_callback(void (*callback)(void))
{
    sent_message_callback = callback;
}

void pairing_wireless_set_received_message_callback(void (*callback)(uint8_t *received_message, uint8_t message_size))
{
    received_message_callback = callback;
}

swc_status_t pairing_wireless_get_status(void)
{
    return swc_get_status();
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Callback function when a previously sent frame has been ACK'd.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 */
static void conn_tx_success_callback(void *conn)
{
    (void)conn;

    sent_message_callback();
}

/** @brief Callback function when a frame has been successfully received.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 */
static void conn_rx_success_callback(void *conn)
{
    swc_error_t err = SWC_ERR_NONE;
    uint8_t *payload = NULL;
    uint8_t payload_size = 0;

    payload_size = swc_connection_receive(conn, &payload, &err);
    if (err != SWC_ERR_NONE) {
        handle_wireless_error();
        return;
    }

    /*
     * The received payload is passed to the callback.
     * The callback must copy the payload since it will
     * be discarded when the swc_connection_receive_complete()
     * is called afterward.
     */
    received_message_callback(payload, payload_size);

    /* Notify the SWC that the new payload has been read. */
    swc_connection_receive_complete(conn, &err);
    if (err != SWC_ERR_NONE) {
        handle_wireless_error();
        return;
    }
}

/** @brief Store the radio serial number, can be used as a Unique ID.
 */
static void store_radio_serial_number(void)
{
    swc_error_t err;

    local_radio_serial_number = swc_node_get_radio_serial_number(&err);
    if (err != SWC_ERR_NONE) {
        handle_wireless_error();
        return;
    }
}

/** @brief Handle a wireless error.
 */
static void handle_wireless_error(void)
{
    pairing_error_set_error(PAIRING_ERR_WIRELESS_ERROR);
}
