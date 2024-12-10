/** @file  datalink_coord.c
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
#include "swc_cfg_coord.h"
#include "swc_stats.h"

/* CONSTANTS ******************************************************************/
#define SWC_MEM_POOL_SIZE     6000
#define MAX_PAYLOAD_SIZE_BYTE 64
#define STATS_ARRAY_LENGTH    1024

/* The device roles are used for the pairing discovery list. */
#define DEVICE_ROLE_COORDINATOR 0
#define DEVICE_ROLE_NODE        1

/* The discovery list includes the coordinator and the node. */
#define PAIRING_DISCOVERY_LIST_SIZE 2
/* The application code prevents unwanted devices from pairing with this application. */
#define PAIRING_APP_CODE 0x0000000000000444
/* The timeout in second after which the pairing procedure will abort. */
#define PAIRING_TIMEOUT_IN_SECONDS 10

/* PRIVATE GLOBALS ************************************************************/
/* ** Wireless Core ** */
static uint8_t swc_memory_pool[SWC_MEM_POOL_SIZE];
static swc_node_t *node;
static swc_connection_t *tx_conn;

static uint32_t timeslot_us[] = SCHEDULE;
static uint32_t channel_sequence[] = CHANNEL_SEQUENCE;
static uint32_t channel_frequency[] = CHANNEL_FREQ;
static int32_t tx_timeslots[] = TX_TIMESLOTS;

/* ** Application Specific ** */
static uint32_t tx_count;
static bool print_stats_now;
static bool reset_stats_now;

static bool device_state_paired;
static pairing_cfg_t app_pairing_cfg;
static pairing_assigned_address_t pairing_assigned_address;
static pairing_discovery_list_t pairing_discovery_list[PAIRING_DISCOVERY_LIST_SIZE];

/*! Sequence number for pseudo random payload. */
uint8_t seq_num;

/* PRIVATE FUNCTION PROTOTYPE *************************************************/
/* **** Wireless Core **** */
static void app_swc_core_init(pairing_assigned_address_t *app_pairing, swc_error_t *err);
static void conn_tx_success_callback(void *conn);
static void conn_tx_fail_callback(void *conn);
/* **** Application **** */
static void print_stats(void);
static void reset_stats(void);

static void enter_pairing_mode(void);
static void unpair_device(void);

static void pairing_application_callback(void);
static void abort_pairing_procedure(void);
static void packet_generation_timer_interrupt_handler(void);

/* PUBLIC FUNCTIONS ***********************************************************/
int main(void)
{
    facade_board_init();

    /* Initialize wireless core context switch handler before pairing is available */
    facade_set_context_switch_handler(swc_connection_callbacks_processing_handler);

    /* Setup higher priority packet generation timer */
    facade_packet_generation_timer_init(timeslot_us[0]);
    facade_packet_generation_set_timer_callback(packet_generation_timer_interrupt_handler);

    while (1) {
        if (!device_state_paired) {
            /* When the device is not paired, the only action possible for the user is the pairing. */
            facade_button_handling(enter_pairing_mode, NULL, NULL, NULL);
        } else {
            /* When the device is paired, normal operations are executed. */
            facade_button_handling(unpair_device, reset_stats, NULL, NULL);
            /* Print received string and stats every 1000 transmissions */
            if (print_stats_now) {
                if (reset_stats_now) {
                    swc_connection_reset_stats(tx_conn);
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

    /* ** TX Connection ** */
    swc_connection_cfg_t tx_conn_cfg = {
        .name = "TX Connection",
        .source_address = local_address,
        .destination_address = remote_address,
        .max_payload_size = MAX_PAYLOAD_SIZE_BYTE,
        .queue_size = TX_DATA_QUEUE_SIZE,
        .timeslot_id = tx_timeslots,
        .timeslot_count = ARRAY_SIZE(tx_timeslots),
    };
    tx_conn = swc_connection_init(node, tx_conn_cfg, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    swc_channel_cfg_t tx_channel_cfg = {
        .tx_pulse_count = TX_DATA_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain = TX_DATA_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT,
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++) {
        tx_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(tx_conn, node, tx_channel_cfg, err);
        if (*err != SWC_ERR_NONE) {
            return;
        }
    }
    swc_connection_set_tx_success_callback(tx_conn, conn_tx_success_callback, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }
    swc_connection_set_tx_fail_callback(tx_conn, conn_tx_fail_callback, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    swc_setup(node, err);
}

/** @brief Callback function when a previously sent frame has been ACK'd.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 */
static void conn_tx_success_callback(void *conn)
{
    (void)conn;

    facade_tx_conn_status();

    /* Print stats every 1000 transmissions */
    tx_count++;
    if ((tx_count % 1000) == 0) {
        print_stats_now = true;
    }
}

/** @brief Callback function when a previously sent frame has not been ACK'd.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 */
static void conn_tx_fail_callback(void *conn)
{
    (void)conn;

    /* Print stats every 1000 transmissions */
    tx_count++;
    if ((tx_count % 1000) == 0) {
        print_stats_now = true;
    }
}

/** @brief Print the TX and RX statistics.
 */
static void print_stats(void)
{
    static char stats_string[STATS_ARRAY_LENGTH];

    swc_connection_update_stats(tx_conn);
    swc_connection_format_stats(tx_conn, node, stats_string, sizeof(stats_string));
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

        /* Configure the Coordinator and Node addresses. */
        app_swc_core_init(&pairing_assigned_address, &swc_err);
        if (swc_err != SWC_ERR_NONE) {
            while (1);
        }

        swc_connect(&swc_err);
        if (swc_err != SWC_ERR_NONE) {
            while (1);
        }

        device_state_paired = true;
        facade_packet_generation_timer_start();

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

    facade_packet_generation_timer_stop();

    /* Indicate that the device is unpaired and turn off all LEDs */
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

/** @brief Packet generation interrupt handler.
 */
static void packet_generation_timer_interrupt_handler(void)
{
    swc_error_t swc_err;
    uint8_t *tx_buf = NULL;

    swc_connection_allocate_payload_buffer(tx_conn, &tx_buf, MAX_PAYLOAD_SIZE_BYTE, &swc_err);
    if ((tx_buf != NULL) && (swc_err == SWC_ERR_NONE)) {
        seq_num = dataforge_increment_seq_num(seq_num);
        dataforge_generate_pseudo(tx_buf, MAX_PAYLOAD_SIZE_BYTE, seq_num);
        swc_connection_send(tx_conn, tx_buf, MAX_PAYLOAD_SIZE_BYTE, &swc_err);
    }
}
