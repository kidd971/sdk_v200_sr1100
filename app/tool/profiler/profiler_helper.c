/** @file  profiler_helper.c
 *  @brief The Helper plays a supporting role in enabling the DUT (Device Under Test) to measure its processing time
 * under different scenarios. It does not directly measure any processing time but facilitates the data transfer
 * required for the tool.
 *
 *  In each scenario, a set number of packets is sent and/or received at the smallest possible payload size supported by
 *  the radio. The average processing time is measured across all processing time samples for each transfer. The payload
 *  size is then increased, and the process repeats: sending and/or receiving the same number of packets, measuring the
 *  average processing time. This loop continues until the maximum payload size supported by the radio is reached.
 *
 *      Scenario 1: The Helper acts as the receiver, receiving payloads from the DUT for processing time measurement.
 *
 *      Scenario 2: The Helper acts as the transmitter, transferring a set of payloads to the DUT for processing time
 *                  measurement.
 *
 *      Scenario 3: The Helper operates in a bidirectional role, both receiving and transmitting payloads to and from
 *                  the DUT, enabling the measurement of processing time.
 *
 *  @copyright Copyright (C) 2025 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "profiler_facade.h"
#include "swc_api.h"
#include "swc_cfg.h"

/* CONSTANTS ******************************************************************/
#define SWC_MEM_POOL_SIZE 8000
#define PAYLOAD_TO_SEND   6

/* TYPES **********************************************************************/
/** @brief Represents the state of processing time measurements for a connection.
 */
typedef enum profiler_state {
    /* The link is idle, waiting for the next scenario to start. */
    PROFILER_STATE_IDLE,
    /* Currently transmitting or receiving AVERAGE_LENGTH payloads of the current payload size. */
    PROFILER_STATE_RUNNING,
    /* The last payload of the current size has been transmitted or received, and the payload size must increase. */
    PROFILER_STATE_INCREMENT_PAYLOAD_SIZE,
    /* The scenario is complete. */
    PROFILER_STATE_COMPLETE,
} profiler_state_t;

/* PRIVATE GLOBALS ************************************************************/
/* ** Wireless Core ** */
static uint8_t swc_memory_pool[SWC_MEM_POOL_SIZE];
static swc_connection_t *tx_conn;
static swc_connection_t *rx_conn;

/* ** Application Specific ** */
/* State of the processing time measurements for the transmission connection. */
static volatile profiler_state_t state_tx = PROFILER_STATE_IDLE;
/* State of the processing time measurements for the reception connection. */
static volatile profiler_state_t state_rx = PROFILER_STATE_IDLE;

/* Payload size used to measure the processing time. */
static volatile uint16_t current_profiling_payload = MIN_PAYLOAD_SIZE_BYTE;

static volatile uint32_t received_packet_count;
static volatile uint32_t generated_packet_count;
static volatile uint32_t transmitted_packet_count;

/* PRIVATE FUNCTION PROTOTYPE *************************************************/
static void conn_tx_success_callback(void *conn);
static void conn_tx_fail_callback(void *conn);
static void conn_rx_success_callback(void *conn);
static void packet_generation_timer_callback(void);

static void setup_tx_connection(void);
static void setup_rx_connection(void);
static void setup_bidir_connection(void);

static void run_unidir_rx_scenario(void);
static void run_unidir_tx_scenario(void);
static void run_bidir_bidir_scenario(void);

/* PUBLIC FUNCTIONS ***********************************************************/
int main(void)
{
    facade_board_init();

    /* Setup packet generation timer. */
    facade_packet_generation_timer_init(PACKET_GENERATION_FEQUENCY_US);
    facade_packet_generation_set_timer_callback(packet_generation_timer_callback);

    /* Initialize Wireless Core context switch handler. */
    facade_set_context_switch_handler(swc_connection_callbacks_processing_handler);

    /* SCENARIO 1: Helper is configured as a receiver. */
    run_unidir_rx_scenario();

    /* SCENARIO 2: Helper is configured as a transmitter. */
    run_unidir_tx_scenario();

    /* SCENARIO 3: Helper in a bidirectional setup. */
    run_bidir_bidir_scenario();

    while (1);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize the SWC and the connection for the unidirectional reception scenario. It also resets metrics
 *         used for synchronizing devices.
 */
static void setup_rx_connection(void)
{
    swc_node_t *node;
    swc_error_t swc_err = SWC_ERR_NONE;
    uint32_t timeslot_us[] = SCHEDULE;
    int32_t timeslots[] = TIMESLOTS;
    uint32_t channel_sequence[] = CHANNEL_SEQUENCE;
    uint32_t channel_frequency[] = CHANNEL_FREQ;

    /* Reset the synchronizing flags for the next scenario. */
    current_profiling_payload = MIN_PAYLOAD_SIZE_BYTE;
    received_packet_count = 0;
    transmitted_packet_count = 0;
    generated_packet_count = 0;

    swc_cfg_t core_cfg = {
        .timeslot_sequence = timeslot_us,
        .timeslot_sequence_length = ARRAY_SIZE(timeslot_us),
        .channel_sequence = channel_sequence,
        .channel_sequence_length = ARRAY_SIZE(channel_sequence),
        .concurrency_mode = SWC_CONCURRENCY_MODE_HIGH_PERFORMANCE,
        .memory_pool = swc_memory_pool,
        .memory_pool_size = SWC_MEM_POOL_SIZE,
    };
    swc_init(core_cfg, facade_context_switch_trigger, &swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1);
    }

    swc_node_cfg_t node_cfg = {
        .role = SWC_ROLE_NODE,
        .pan_id = PAN_ID,
        .coordinator_address = COORDINATOR_ADDRESS,
        .local_address = NODE_ADDRESS,
    };
    node = swc_node_init(node_cfg, &swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1);
    }

    swc_radio_module_init(node, SWC_RADIO_ID_1, true, &swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1);
    }

    /* ** RX Connection ** */
    swc_connection_cfg_t rx_conn_cfg = {
        .name = "RX Connection",
        .source_address = COORDINATOR_ADDRESS,
        .destination_address = NODE_ADDRESS,
        .max_payload_size = MAX_PAYLOAD_SIZE_BYTE,
        .queue_size = RX_DATA_QUEUE_SIZE,
        .timeslot_id = timeslots,
        .timeslot_count = ARRAY_SIZE(timeslots),
    };
    rx_conn = swc_connection_init(node, rx_conn_cfg, &swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1);
    }

    swc_channel_cfg_t rx_channel_cfg = {
        .tx_pulse_count = TX_ACK_PULSE_COUNT,
        .tx_pulse_width = TX_ACK_PULSE_WIDTH,
        .tx_pulse_gain = TX_ACK_PULSE_GAIN,
        .rx_pulse_count = RX_DATA_PULSE_COUNT,
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++) {
        rx_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(rx_conn, node, rx_channel_cfg, &swc_err);
        if (swc_err != SWC_ERR_NONE) {
            while (1);
        }
    }
    swc_connection_set_rx_success_callback(rx_conn, conn_rx_success_callback, &swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1);
    }

    swc_setup(node, &swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1);
    }
}

/** @brief Initialize the SWC and the connection for the unidirectional transmission scenario. It also resets metrics
 *         used for synchronizing devices.
 */
static void setup_tx_connection(void)
{
    swc_node_t *node;
    swc_error_t swc_err = SWC_ERR_NONE;
    uint32_t timeslot_us[] = SCHEDULE;
    int32_t timeslots[] = TIMESLOTS;
    uint32_t channel_sequence[] = CHANNEL_SEQUENCE;
    uint32_t channel_frequency[] = CHANNEL_FREQ;

    /* Reset the synchronizing flags for the next scenario. */
    current_profiling_payload = MIN_PAYLOAD_SIZE_BYTE;
    received_packet_count = 0;
    transmitted_packet_count = 0;
    generated_packet_count = 0;

    swc_cfg_t core_cfg = {
        .timeslot_sequence = timeslot_us,
        .timeslot_sequence_length = ARRAY_SIZE(timeslot_us),
        .channel_sequence = channel_sequence,
        .channel_sequence_length = ARRAY_SIZE(channel_sequence),
        .concurrency_mode = SWC_CONCURRENCY_MODE_HIGH_PERFORMANCE,
        .memory_pool = swc_memory_pool,
        .memory_pool_size = SWC_MEM_POOL_SIZE,
    };
    swc_init(core_cfg, facade_context_switch_trigger, &swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1);
    }

    /* For the unidirectional TX connection (scenario 2), the Helper acts as the Coordinator. */
    swc_node_cfg_t node_cfg = {
        .role = SWC_ROLE_COORDINATOR,
        .pan_id = PAN_ID,
        .coordinator_address = COORDINATOR_ADDRESS,
        .local_address = NODE_ADDRESS,
    };
    node = swc_node_init(node_cfg, &swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1);
    }

    swc_radio_module_init(node, SWC_RADIO_ID_1, true, &swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1);
    }

    /* ** TX Connection ** */
    swc_connection_cfg_t tx_conn_cfg = {
        .name = "TX Connection",
        .source_address = NODE_ADDRESS,
        .destination_address = COORDINATOR_ADDRESS,
        .max_payload_size = MAX_PAYLOAD_SIZE_BYTE,
        .queue_size = TX_DATA_QUEUE_SIZE,
        .timeslot_id = timeslots,
        .timeslot_count = ARRAY_SIZE(timeslots),
    };
    tx_conn = swc_connection_init(node, tx_conn_cfg, &swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1);
    }

    swc_channel_cfg_t tx_channel_cfg = {
        .tx_pulse_count = TX_DATA_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain = TX_DATA_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT,
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++) {
        tx_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(tx_conn, node, tx_channel_cfg, &swc_err);
        if (swc_err != SWC_ERR_NONE) {
            while (1);
        }
    }
    swc_connection_set_tx_success_callback(tx_conn, conn_tx_success_callback, &swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1);
    }
    swc_connection_set_tx_fail_callback(tx_conn, conn_tx_fail_callback, &swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1);
    }

    swc_setup(node, &swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1);
    }
}

/** @brief Initialize the SWC and the connection for the bidirectional transmission-reception scenario. It also resets
 *         metrics used for synchronizing devices.
 */
static void setup_bidir_connection(void)
{
    swc_node_t *node;
    swc_error_t swc_err = SWC_ERR_NONE;
    uint32_t bidir_timeslot_us[] = BIDIR_SCHEDULE;
    int32_t bidir_tx_timeslots[] = BIDIR_TIMESLOTS_0;
    int32_t bidir_rx_timeslots[] = BIDIR_TIMESLOTS_1;
    uint32_t channel_sequence[] = CHANNEL_SEQUENCE;
    uint32_t channel_frequency[] = CHANNEL_FREQ;

    /* Reset the synchronizing flags for the next scenario. */
    current_profiling_payload = MIN_PAYLOAD_SIZE_BYTE;
    received_packet_count = 0;
    transmitted_packet_count = 0;
    generated_packet_count = 0;

    swc_cfg_t core_cfg = {
        .timeslot_sequence = bidir_timeslot_us,
        .timeslot_sequence_length = ARRAY_SIZE(bidir_timeslot_us),
        .channel_sequence = channel_sequence,
        .channel_sequence_length = ARRAY_SIZE(channel_sequence),
        .concurrency_mode = SWC_CONCURRENCY_MODE_HIGH_PERFORMANCE,
        .memory_pool = swc_memory_pool,
        .memory_pool_size = SWC_MEM_POOL_SIZE,
    };
    swc_init(core_cfg, facade_context_switch_trigger, &swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1);
    }

    swc_node_cfg_t node_cfg = {
        .role = SWC_ROLE_NODE,
        .pan_id = PAN_ID,
        .coordinator_address = COORDINATOR_ADDRESS,
        .local_address = NODE_ADDRESS,
    };
    node = swc_node_init(node_cfg, &swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1);
    }

    swc_radio_module_init(node, SWC_RADIO_ID_1, true, &swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1);
    }

    /* ** RX Connection ** */
    swc_connection_cfg_t rx_conn_cfg = {
        .name = "RX Connection",
        .source_address = COORDINATOR_ADDRESS,
        .destination_address = NODE_ADDRESS,
        .max_payload_size = MAX_PAYLOAD_SIZE_BYTE,
        .queue_size = RX_DATA_QUEUE_SIZE,
        .timeslot_id = bidir_rx_timeslots,
        .timeslot_count = ARRAY_SIZE(bidir_rx_timeslots),
    };
    rx_conn = swc_connection_init(node, rx_conn_cfg, &swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1);
    }

    swc_channel_cfg_t rx_channel_cfg = {
        .tx_pulse_count = TX_ACK_PULSE_COUNT,
        .tx_pulse_width = TX_ACK_PULSE_WIDTH,
        .tx_pulse_gain = TX_ACK_PULSE_GAIN,
        .rx_pulse_count = RX_DATA_PULSE_COUNT,
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++) {
        rx_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(rx_conn, node, rx_channel_cfg, &swc_err);
        if (swc_err != SWC_ERR_NONE) {
            while (1);
        }
    }
    swc_connection_set_rx_success_callback(rx_conn, conn_rx_success_callback, &swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1);
    }

    /* ** TX Connection ** */
    swc_connection_cfg_t tx_conn_cfg = {
        .name = "TX Connection",
        .source_address = NODE_ADDRESS,
        .destination_address = COORDINATOR_ADDRESS,
        .max_payload_size = MAX_PAYLOAD_SIZE_BYTE,
        .queue_size = TX_DATA_QUEUE_SIZE,
        .timeslot_id = bidir_tx_timeslots,
        .timeslot_count = ARRAY_SIZE(bidir_tx_timeslots),
    };
    tx_conn = swc_connection_init(node, tx_conn_cfg, &swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1);
    }

    swc_channel_cfg_t tx_channel_cfg = {
        .tx_pulse_count = TX_DATA_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain = TX_DATA_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT,
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_sequence); i++) {
        tx_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(tx_conn, node, tx_channel_cfg, &swc_err);
        if (swc_err != SWC_ERR_NONE) {
            while (1);
        }
    }
    swc_connection_set_tx_success_callback(tx_conn, conn_tx_success_callback, &swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1);
    }
    swc_connection_set_tx_fail_callback(tx_conn, conn_tx_fail_callback, &swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1);
    }

    swc_setup(node, &swc_err);
    if (swc_err != SWC_ERR_NONE) {
        while (1);
    }
}

/** @brief Callback function when a frame has been successfully received.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 */
static void conn_rx_success_callback(void *conn)
{
    (void)conn;

    swc_error_t err = SWC_ERR_NONE;
    uint8_t *payload = NULL;
    uint16_t received_payload_size;

    /* Retrieve the received packet from the SWC queue. */
    received_payload_size = swc_connection_receive(rx_conn, &payload, &err);
    swc_connection_receive_complete(rx_conn, &err);

    /* Only packets matching the expected size are counted. This ensures robustness against mismatched test steps
     * between devices in case of test procedure desynchronization.
     */
    if (received_payload_size == current_profiling_payload) {
        /* Count successfully received packets. */
        if (received_packet_count >= (AVERAGE_LENGTH - 1)) {
            state_rx = PROFILER_STATE_INCREMENT_PAYLOAD_SIZE;
        } else {
            received_packet_count++;
        }
    }

    facade_rx_conn_status();
}

/** @brief Callback function when a previously sent frame has been ACK'd.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 */
static void conn_tx_success_callback(void *conn)
{
    (void)conn;

    /* Count successfully transmitted packets. */
    if (transmitted_packet_count >= (AVERAGE_LENGTH - 1)) {
        state_tx = PROFILER_STATE_INCREMENT_PAYLOAD_SIZE;
    } else {
        transmitted_packet_count++;
    }

    facade_tx_conn_status();
}

/** @brief Callback function when a previously sent frame has been ACK'd.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 */
static void conn_tx_fail_callback(void *conn)
{
    (void)conn;

    static uint8_t last_packet_tx_fail_count;

    if (transmitted_packet_count >= (AVERAGE_LENGTH - 1)) {
        /* TX fail on last packet could be a missed ACK. */
        if (++last_packet_tx_fail_count > MAX_LAST_PKT_FAIL) {
            /* If the last packet can't get through, consider that the other device received and disconnected. */
            state_tx = PROFILER_STATE_INCREMENT_PAYLOAD_SIZE;
        }
    } else {
        last_packet_tx_fail_count = 0;
    }
}

/** @brief Timer callback for generating packets.
 *
 *  This function is triggered by a timer to periodically send packets. If the required number of packets has not been
 *  sent, it allocates a buffer, fills it with test data, and queues it for transmission.
 */
static void packet_generation_timer_callback(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    uint8_t *profiler_buf = NULL;

    /* If the number of generated or received packets is below the required amount, the timer continues sending packets
     * to maintain synchronization.
     */
    if ((generated_packet_count <= (AVERAGE_LENGTH - 1)) ||
        ((rx_conn != NULL) && (received_packet_count < (AVERAGE_LENGTH - 1)))) {

        swc_connection_get_payload_buffer(tx_conn, &profiler_buf, &swc_err);

        /* If the SWC queue is not full, a new packets is inserted into the queue */
        if (profiler_buf != NULL) {
            memset(profiler_buf, PAYLOAD_TO_SEND, current_profiling_payload);
            swc_connection_send(tx_conn, profiler_buf, current_profiling_payload, &swc_err);
            /* The count of generated packets is incremented. */
            generated_packet_count++;
        }
    }
}

/** @brief Run the unidirectional reception scenario.
 */
static void run_unidir_rx_scenario(void)
{
    /* SCENARIO 1 : The Helper acts as the receiver, receiving payloads from the DUT for processing time measurement. */

    swc_error_t swc_err = SWC_ERR_NONE;

    /* Setup the SWC and the RX connection. */
    setup_rx_connection();

    /* The Helper receives the packets from the DUT. */
    state_tx = PROFILER_STATE_IDLE;
    state_rx = PROFILER_STATE_RUNNING;

    while (state_rx != PROFILER_STATE_COMPLETE) {
        /* Connect the wireless core. */
        swc_connect(&swc_err);
        if (swc_err != SWC_ERR_NONE) {
            while (1);
        }

        /** Wait until all profiling size payloads are received.
         *  The state transitions to PROFILER_STATE_INCREMENT_PAYLOAD_SIZE in the conn_rx_success_callback when the
         *  count is reached.
         */
        while (state_rx != PROFILER_STATE_INCREMENT_PAYLOAD_SIZE);

        /* If the payload size was the max value, all processing times are measured, and the scenario is complete. */
        if (current_profiling_payload >= MAX_PAYLOAD_SIZE_BYTE) {
            state_rx = PROFILER_STATE_COMPLETE;
        } else {
            /* Disconnect the Wireless Core. */
            swc_disconnect(&swc_err);
            if ((swc_err != SWC_ERR_NONE) && (swc_err != SWC_ERR_NOT_CONNECTED)) {
                while (1);
            }
            /* Increase the payload size of the packet. */
            current_profiling_payload <<= 1;
            if (current_profiling_payload > MAX_PAYLOAD_SIZE_BYTE) {
                current_profiling_payload = MAX_PAYLOAD_SIZE_BYTE;
            }
            /* Restart the transfers. */
            received_packet_count = 0;
            state_rx = PROFILER_STATE_RUNNING;
        }
    }

    /* Disconnect the Wireless Core. */
    swc_disconnect(&swc_err);
    if ((swc_err != SWC_ERR_NONE) && (swc_err != SWC_ERR_NOT_CONNECTED)) {
        while (1);
    }
}

/** @brief Run the unidirectional transmission scenario.
 */
static void run_unidir_tx_scenario(void)
{
    /** SCENARIO 2 : The Helper acts as the transmitter, transferring a set of payloads to the DUT for processing
     *               time measurement.
     */

    swc_error_t swc_err = SWC_ERR_NONE;

    /* Setup the SWC and the TX connection. */
    setup_tx_connection();

    /* The Helper transmits the packets to the DUT. */
    state_tx = PROFILER_STATE_RUNNING;
    state_rx = PROFILER_STATE_IDLE;

    while (state_tx != PROFILER_STATE_COMPLETE) {
        /* Connect to the wireless core. */
        swc_connect(&swc_err);
        if (swc_err != SWC_ERR_NONE) {
            while (1);
        }

        /* Start the generation of packets. */
        facade_packet_generation_timer_start();

        /** Wait until all profiling size payloads are sent.
         *  The state transitions to PROFILER_STATE_INCREMENT_PAYLOAD_SIZE in the conn_tx_success_callback when the
         *  count is reached.
         */
        while (state_tx != PROFILER_STATE_INCREMENT_PAYLOAD_SIZE);

        /* Stop the timer used for the generation of new packets. */
        facade_packet_generation_timer_stop();

        /* If the payload size was the max value, all processing times are measured, and the scenario is complete. */
        if (current_profiling_payload >= MAX_PAYLOAD_SIZE_BYTE) {
            state_tx = PROFILER_STATE_COMPLETE;
        } else {
            /* Disconnect the Wireless Core. */
            swc_disconnect(&swc_err);
            if ((swc_err != SWC_ERR_NONE) && (swc_err != SWC_ERR_NOT_CONNECTED)) {
                while (1);
            }
            /* Flush packets from TX queue. */
            swc_connection_flush_queue(tx_conn, &swc_err);
            if (swc_err != SWC_ERR_NONE) {
                while (1);
            }

            /* Increase the payload size of the packet. */
            current_profiling_payload <<= 1;
            if (current_profiling_payload > MAX_PAYLOAD_SIZE_BYTE) {
                current_profiling_payload = MAX_PAYLOAD_SIZE_BYTE;
            }
            /* Restart the transfers. */
            generated_packet_count = 0;
            transmitted_packet_count = 0;
            state_tx = PROFILER_STATE_RUNNING;
        }
    }

    /* Disconnect the Wireless Core. */
    swc_disconnect(&swc_err);
    if ((swc_err != SWC_ERR_NONE) && (swc_err != SWC_ERR_NOT_CONNECTED)) {
        while (1);
    }
}

/** @brief Run the bidirectional transmission-reception scenario.
 */
static void run_bidir_bidir_scenario(void)
{
    /** SCENARIO 3 : The Helper operates in a bidirectional role, both receiving and transmitting payloads to and from
     *               the DUT, enabling the measurement of processing time.
     */

    swc_error_t swc_err = SWC_ERR_NONE;

    /* Setup the SWC and the TX and RX connections. */
    setup_bidir_connection();

    /* The Helper receives and transmits packets to and from the DUT. */
    state_tx = PROFILER_STATE_RUNNING;
    state_rx = PROFILER_STATE_RUNNING;

    while (state_rx != PROFILER_STATE_COMPLETE || state_tx != PROFILER_STATE_COMPLETE) {
        /* Connect to the wireless core. */
        swc_connect(&swc_err);
        if (swc_err != SWC_ERR_NONE) {
            while (1);
        }

        /* Start the generation of packets. */
        facade_packet_generation_timer_start();

        /* Wait until both sequence are finished. (Packets with the right payload size has been sent and received) */
        while (state_rx != PROFILER_STATE_INCREMENT_PAYLOAD_SIZE || state_tx != PROFILER_STATE_INCREMENT_PAYLOAD_SIZE);

        /* Stop the timer used for the generation of new packets. */
        facade_packet_generation_timer_stop();

        /* Disconnect the Wireless Core. */
        swc_disconnect(&swc_err);
        if ((swc_err != SWC_ERR_NONE) && (swc_err != SWC_ERR_NOT_CONNECTED)) {
            while (1);
        }
        /* Flush packets from TX queue. */
        swc_connection_flush_queue(tx_conn, &swc_err);
        if (swc_err != SWC_ERR_NONE) {
            while (1);
        }

        /* If the payload size was the max value, all processing times are measured, and the scenario is complete. */
        if (current_profiling_payload >= MAX_PAYLOAD_SIZE_BYTE) {
            state_tx = PROFILER_STATE_COMPLETE;
            state_rx = PROFILER_STATE_COMPLETE;
        } else {
            /* Increase the payload size of the packet. */
            current_profiling_payload <<= 1;
            if (current_profiling_payload > MAX_PAYLOAD_SIZE_BYTE) {
                current_profiling_payload = MAX_PAYLOAD_SIZE_BYTE;
            }

            /* Restart the sequence. */
            generated_packet_count = 0;
            transmitted_packet_count = 0;
            received_packet_count = 0;
            state_tx = PROFILER_STATE_RUNNING;
            state_rx = PROFILER_STATE_RUNNING;
        }
    }
}
