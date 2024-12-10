/** @file  star_network_node.c
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

#if defined(NODE1)
#include "swc_cfg_node1.h"
#elif defined(NODE2)
#include "swc_cfg_node2.h"
#endif

/* CONSTANTS ******************************************************************/
#define SWC_MEM_POOL_SIZE     6000
#define MAX_PAYLOAD_SIZE_BYTE 12
#define BUTTON_PRESSED        0x01
#define PRINTF_BUF_SIZE_BYTE  64

/* The timeout in second after which the pairing procedure will abort. */
#define PAIRING_TIMEOUT_IN_SECONDS 10
/* The application code prevents unwanted devices from pairing with this application. */
#define PAIRING_APP_CODE 0x0000000000000777

/* PRIVATE GLOBALS ************************************************************/
/* ** Wireless Core ** */
static uint8_t swc_memory_pool[SWC_MEM_POOL_SIZE];
static swc_node_t *node;

static swc_connection_t *rx_from_coord_conn;
static swc_connection_t *tx_to_coord_conn;

static uint32_t timeslot_us[] = SCHEDULE;
static uint32_t channel_sequence[] = CHANNEL_SEQUENCE;
static uint32_t channel_frequency[] = CHANNEL_FREQ;
static int32_t tx_to_coord_timeslots[] = TX_TO_COORD_TIMESLOTS;
static int32_t rx_from_coord_timeslots[] = RX_FROM_COORD_TIMESLOTS;

/* ** Application Specific ** */
static unsigned long coord_inc;

static bool device_state_paired;
static pairing_cfg_t app_pairing_cfg;
static pairing_assigned_address_t pairing_assigned_address;

/* PRIVATE FUNCTION PROTOTYPE *************************************************/
static void app_swc_core_init(pairing_assigned_address_t *app_pairing, swc_error_t *err);
static void conn_rx_success_callback(void *conn);

static void enter_pairing_mode(void);
static void unpair_device(void);

static void pairing_application_callback(void);
static void abort_pairing_procedure(void);

/* PUBLIC FUNCTIONS ***********************************************************/
int main(void)
{
    swc_error_t swc_err;
    uint8_t *star_network_buf;

    facade_board_init();

    /* Initialize wireless core context switch handler before pairing is available */
    facade_set_context_switch_handler(swc_connection_callbacks_processing_handler);

    while (1) {
        if (!device_state_paired) {
            /* When the device is not paired, the only action possible for the user is the pairing. */
            facade_button_handling(enter_pairing_mode, NULL, NULL, NULL);
        } else {
            /* When the device is paired, normal operations are executed. */
            facade_button_handling(unpair_device, NULL, NULL, NULL);
            swc_connection_allocate_payload_buffer(tx_to_coord_conn, &star_network_buf, MAX_PAYLOAD_SIZE_BYTE,
                                                   &swc_err);
            if (star_network_buf != NULL) {
                /* Send the payload through the Wireless Core. */
                snprintf((char *)star_network_buf, MAX_PAYLOAD_SIZE_BYTE, "x%lu", coord_inc++);
                star_network_buf[0] = facade_read_button_status(BUTTON_B);
                /* Payload must include the terminating NULL. */
                swc_connection_send(tx_to_coord_conn, star_network_buf, MAX_PAYLOAD_SIZE_BYTE, &swc_err);
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
    uint16_t local_address = app_pairing->node_address;
    uint16_t remote_address = app_pairing->coordinator_address;

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
    swc_set_fast_sync(true, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    swc_node_cfg_t node_cfg = {
        .role = NETWORK_ROLE,
        .pan_id = app_pairing->pan_id,
        .coordinator_address = remote_address,
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

    /* ** Node sending to Coordinator connection ** */
    swc_connection_cfg_t tx_to_coord_conn_cfg = {
        .name = "TX to Coord Connection",
        .source_address = local_address,
        .destination_address = remote_address,
        .max_payload_size = MAX_PAYLOAD_SIZE_BYTE,
        .queue_size = TX_DATA_QUEUE_SIZE,
        .timeslot_id = tx_to_coord_timeslots,
        .timeslot_count = ARRAY_SIZE(tx_to_coord_timeslots),
    };
    tx_to_coord_conn = swc_connection_init(node, tx_to_coord_conn_cfg, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    swc_channel_cfg_t tx_to_coord_channel_cfg = {
        .tx_pulse_count = TX_DATA_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain = TX_DATA_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT,
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++) {
        tx_to_coord_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(tx_to_coord_conn, node, tx_to_coord_channel_cfg, err);
        if (*err != SWC_ERR_NONE) {
            return;
        }
    }

    /* ** Node receiving from Coordinator connection ** */
    swc_connection_cfg_t rx_from_coord_conn_cfg = {
        .name = "RX from Coordinator Connection",
        .source_address = remote_address,
        .destination_address = local_address,
        .max_payload_size = MAX_PAYLOAD_SIZE_BYTE,
        .queue_size = RX_DATA_QUEUE_SIZE,
        .timeslot_id = rx_from_coord_timeslots,
        .timeslot_count = ARRAY_SIZE(rx_from_coord_timeslots),
    };
    rx_from_coord_conn = swc_connection_init(node, rx_from_coord_conn_cfg, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    swc_channel_cfg_t rx_from_coord_channel_cfg = {
        .tx_pulse_count = TX_ACK_PULSE_COUNT,
        .tx_pulse_width = TX_ACK_PULSE_WIDTH,
        .tx_pulse_gain = TX_ACK_PULSE_GAIN,
        .rx_pulse_count = RX_DATA_PULSE_COUNT,
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++) {
        rx_from_coord_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(rx_from_coord_conn, node, rx_from_coord_channel_cfg, err);
        if (*err != SWC_ERR_NONE) {
            return;
        }
    }
    swc_connection_set_rx_success_callback(rx_from_coord_conn, conn_rx_success_callback, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    swc_setup(node, err);
}

/** @brief Callback function when a frame has been successfully received from the Coordinator.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 */
static void conn_rx_success_callback(void *conn)
{
    (void)conn;

    swc_error_t err;
    uint8_t *payload = NULL;

    /* Get new payload. */
    swc_connection_receive(rx_from_coord_conn, &payload, &err);

    if (payload[0] == BUTTON_PRESSED) {
        facade_payload_received_status();
    } else {
        facade_empty_payload_received_status();
    }

    facade_usb_printf("Received from Coordinator : %s\n\r", &payload[1]);

    /* Notify the Wireless Core that the new payload has been read. */
    swc_connection_receive_complete(rx_from_coord_conn, &err);
}

/** @brief Enter in Pairing Mode using the Pairing Module.
 */
static void enter_pairing_mode(void)
{
    swc_error_t swc_err;
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

    /* Give the information to the Pairing Module. */
    app_pairing_cfg.app_code = PAIRING_APP_CODE;
    app_pairing_cfg.timeout_sec = PAIRING_TIMEOUT_IN_SECONDS;
    app_pairing_cfg.context_switch_callback = facade_context_switch_trigger;
    app_pairing_cfg.application_callback = pairing_application_callback;
    app_pairing_cfg.memory_pool = swc_memory_pool;
    app_pairing_cfg.memory_pool_size = SWC_MEM_POOL_SIZE;
    app_pairing_cfg.uwb_regulation = SWC_REGULATION_FCC;
    pairing_event = pairing_node_start(&app_pairing_cfg, &pairing_assigned_address, PAIRING_DEVICE_ROLE, &pairing_err);
    if (pairing_err != PAIRING_ERR_NONE) {
        while (1);
    }

    /* Handle the pairing events. */
    switch (pairing_event) {
    case PAIRING_EVENT_SUCCESS:
        facade_notify_pairing_successful();

        app_swc_core_init(&pairing_assigned_address, &swc_err);
        if (swc_err != SWC_ERR_NONE) {
            while (1);
        }

        swc_connect(&swc_err);
        if (swc_err != SWC_ERR_NONE) {
            while (1);
        }

        device_state_paired = true;

        break;
    case PAIRING_EVENT_TIMEOUT:
    case PAIRING_EVENT_INVALID_APP_CODE:
    case PAIRING_EVENT_ABORT:
    default:
        /* Indicate that the pairing process was unsuccessful */
        facade_notify_not_paired();
        device_state_paired = false;
        break;
    }
}

/** @brief Put the device in the unpaired state and disconnect it from the network.
 */
static void unpair_device(void)
{
    swc_error_t swc_err;

    device_state_paired = false;

    swc_disconnect(&swc_err);
    if ((swc_err != SWC_ERR_NONE) && (swc_err != SWC_ERR_NOT_CONNECTED)) {
        while (1);
    }

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

/** @brief Abort the pairing procedure once started.
 */
static void abort_pairing_procedure(void)
{
    pairing_abort();
}
