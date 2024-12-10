/** @file  hello_world_node.c
 *  @brief This is a basic example of how to use the SPARK Wireless Core.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include <stdio.h>
#include "dataforge.h"
#include "datalink_facade.h"
#include "pairing_api.h"
#include "swc_api.h"
#include "swc_cfg_node.h"
#include "swc_stats.h"

/* CONSTANTS ******************************************************************/
/* More memory is needed when using dual radio. */
#if (SWC_RADIO_COUNT == 2)
#define SWC_MEM_POOL_SIZE 10000
#else
#define SWC_MEM_POOL_SIZE 6100
#endif

#define MAX_PAYLOAD_SIZE_BYTE 64
#define STATS_ARRAY_LENGTH    500

/* The timeout in second after which the pairing procedure will abort. */
#define PAIRING_TIMEOUT_IN_SECONDS 10
/* The pairing device role is used for the coordinator's pairing discovery list. */
#define PAIRING_DEVICE_ROLE 1
/* The application code prevents unwanted devices from pairing with this application. */
#define PAIRING_APP_CODE 0x0000000000000444

/* PRIVATE GLOBALS ************************************************************/
/* ** Wireless Core ** */
static uint8_t swc_memory_pool[SWC_MEM_POOL_SIZE];
static swc_node_t *node;
static swc_connection_t *rx_conn;

static uint32_t timeslot_us[] = SCHEDULE;
static uint32_t channel_sequence[] = CHANNEL_SEQUENCE;
static uint32_t channel_frequency[] = CHANNEL_FREQ;
static int32_t rx_timeslots[] = RX_TIMESLOTS;

/* ** Application Specific ** */
static uint32_t rx_count;
static bool print_stats_now;
static bool reset_stats_now;

static bool device_state_paired;
static pairing_cfg_t app_pairing_cfg;
static pairing_assigned_address_t pairing_assigned_address;

/*! Sequence number for pseudo random payload. */
uint8_t seq_num;

/* PRIVATE FUNCTION PROTOTYPE *************************************************/
/* **** Wireless Core **** */
static void app_swc_core_init(pairing_assigned_address_t *app_pairing, swc_error_t *err);
static void conn_rx_success_callback(void *conn);
/* **** Application **** */
static void print_stats(void);
static void reset_stats(void);

static void enter_pairing_mode(void);
static void unpair_device(void);

static void pairing_application_callback(void);
static void abort_pairing_procedure(void);

/* PUBLIC FUNCTIONS ***********************************************************/
int main(void)
{
    facade_board_init();

    /* Initialize wireless core context switch handler before pairing is available */
    facade_set_context_switch_handler(swc_connection_callbacks_processing_handler);

    while (1) {
        if (!device_state_paired) {
            /* When the device is not paired, the only action possible for the user is the pairing. */
            facade_button_handling(enter_pairing_mode, NULL, NULL, NULL);
        } else {
            /* When the device is paired, normal operations are executed. */
            facade_button_handling(unpair_device, reset_stats, NULL, NULL);
            /* Print stats every 1000 transmissions */
            if (print_stats_now) {
                if (reset_stats_now) {
                    swc_connection_reset_stats(rx_conn);
                    reset_stats_now = false;
                } else {
                    print_stats();
                }
                print_stats_now = false;
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

#if (SWC_RADIO_COUNT == 2)
    swc_radio_module_init(node, SWC_RADIO_ID_2, true, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }
#endif

    /* ** RX Connection ** */
    swc_connection_cfg_t rx_conn_cfg = {
        .name = "RX Connection",
        .source_address = remote_address,
        .destination_address = local_address,
        .max_payload_size = MAX_PAYLOAD_SIZE_BYTE,
        .queue_size = RX_DATA_QUEUE_SIZE,
        .timeslot_id = rx_timeslots,
        .timeslot_count = ARRAY_SIZE(rx_timeslots),
    };
    rx_conn = swc_connection_init(node, rx_conn_cfg, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    swc_channel_cfg_t rx_channel_cfg = {
        .tx_pulse_count = TX_ACK_PULSE_COUNT,
        .tx_pulse_width = TX_ACK_PULSE_WIDTH,
        .tx_pulse_gain = TX_ACK_PULSE_GAIN,
        .rx_pulse_count = RX_DATA_PULSE_COUNT,
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++) {
        rx_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(rx_conn, node, rx_channel_cfg, err);
        if (*err != SWC_ERR_NONE) {
            return;
        }
    }
    swc_connection_set_rx_success_callback(rx_conn, conn_rx_success_callback, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    swc_setup(node, err);
}

/** @brief Callback function when a frame has been successfully received.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 */
static void conn_rx_success_callback(void *conn)
{
    (void)conn;

    swc_error_t err;
    uint8_t *payload = NULL;
    bool payload_is_valid;

    /* Get new payload */
    size_t payload_size = swc_connection_receive(rx_conn, &payload, &err);

    if ((payload == NULL) || (err != SWC_ERR_NONE)) {
        return;
    }

    dataforge_seq_status_t seq_status = dataforge_validate_seq_num(payload, seq_num);

    seq_num = dataforge_extract_seq_num(payload);
    if (seq_status != DATAFORGE_MATCHING_SEQ) {
        payload_is_valid = false;
    } else {
        payload_is_valid = dataforge_validate_pseudo_crc(payload, payload_size);
    }

    /* Free the payload memory */
    swc_connection_receive_complete(rx_conn, &err);

    if (payload_is_valid) {
        facade_rx_conn_status();
    }

    /* Print stats every 1000 receptions */
    rx_count++;
    if ((rx_count % 1000) == 0) {
        print_stats_now = true;
    }
}

/** @brief Print the TX and RX statistics.
 */
static void print_stats(void)
{
    static char stats_string[STATS_ARRAY_LENGTH];

    swc_connection_update_stats(rx_conn);
    swc_connection_format_stats(rx_conn, node, stats_string, sizeof(stats_string));
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
