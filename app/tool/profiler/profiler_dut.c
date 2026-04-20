/** @file  profiler_dut.c
 *  @brief This tool measures the processing time of the wireless core under different scenarios. It evaluates the
 *  average processing time for various payload sizes, ranging from the smallest to the maximum possible, in a sweep
 *  across these sizes. The processing time is measured per timeslot for each payload size.
 *
 *  In each scenario, a set number of packets is sent and/or received at the smallest possible payload size supported by
 *  the radio. The average processing time is measured across all processing time samples for each transfer. The payload
 *  size is then increased, and the process repeats: sending and/or receiving the same number of packets, measuring the
 *  average processing time. This loop continues until the maximum payload size supported by the radio is reached.
 *
 *      Scenario 1: The DUT (Device Under Test) is configured as the transmitter and the Helper as the receiver,
 *                  transferring a set of payloads with varying sizes.
 *
 *      Scenario 2: The DUT is configured as the receiver and the Helper as the transmitter, receiving a similar set of
 *                  payloads with varying sizes.
 *
 *      Scenario 3: Both the DUT and Helper are set up in a bidirectional configuration, transferring and receiving
 *                  payloads of different sizes.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "profiler_facade.h"
#include "swc_api.h"
#include "swc_cfg.h"
#include "swc_error.h"
#include "swc_hal_facade.h"

/* CONSTANTS ******************************************************************/
#define SWC_MEM_POOL_SIZE 8000
/* Payload value to send. */
#define PAYLOAD_TO_SEND 6
/* Bytes overhead of a TX SPI transfer: 1 byte CMD + 3 bytes MAC. */
#define TX_PAYLOAD_SPI_OVERHEAD 4
/* Define whether or not to print all measurement statistics.  */
#define PRINT_ALL_STATS false
/* Buffer size for terminal logs. */
#define LOG_BUFFER_SIZE      ((PRINT_ALL_STATS) ? 3000 : 1024)
#define LOG_LINE_BUFFER_SIZE 64
/* Size of the buffer used to print errors. */
#define ERROR_MESSAGE_BUFFER_SIZE 50

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

/** @brief Statistics of the processing time for a given payload size.
 */
typedef struct processing_time_stats {
    /* Payload size of the measurement. */
    uint32_t payload_size;
    /* Sum of all measurements. */
    double sum;
    /* Number of measurements summed. */
    uint32_t sum_count;
    /* Average measurement based on the sum of all measurements. */
    double average;
    /* Maximum value measured. */
    double max;
    /* Index of the maximum value measured. */
    uint32_t max_idx;
    /* Minimum value measured. */
    double min;
    /* Index of the minimum value measured. */
    uint32_t min_idx;
} processing_time_stats_t;

/** @brief Statistics of a scenario.
 */
typedef volatile struct scenario_stats {
    processing_time_stats_t processing_time_stats[PAYLOAD_SIZE_COUNT];
    uint32_t current_index;
} scenario_stats_t;

/** @brief Statistics for the summary of all the profiler measurements.
 */
typedef struct profiler_stats {
    /* Time required to send 1 byte via SPI. */
    double spi_us_per_byte;
    /* Constant processing time for each transmission, including configuration and CPU cycles. */
    double tx_base_proc_time_us;
    /* Constant processing time for each reception, including configuration and CPU cycles. */
    double rx_base_proc_time_us;
    /* Constant processing time for each transfer, including configuration and CPU cycles. */
    double rxtx_base_proc_time_us;
    /* The RX payload size for which the SPI transfer exceeds the CPU cycles needed to process the received frame. */
    double rx_payload_threshold;
    /* Time not included in the constant processing time for each transfer. */
    double rxtx_overhead_time_us;
} profiler_stats_t;

/* PRIVATE GLOBALS ************************************************************/
/* ** Wireless Core ** */
static uint8_t swc_memory_pool[SWC_MEM_POOL_SIZE];
static swc_connection_t *tx_conn;
static swc_connection_t *rx_conn;

/* ** Application Specific ** */
/* Character buffer for logs. */
static char log_buffer[LOG_BUFFER_SIZE];
/* State of the processing time measurements for the transmission connection. */
static volatile profiler_state_t state_tx = PROFILER_STATE_IDLE;
/* State of the processing time measurements for the reception connection. */
static volatile profiler_state_t state_rx = PROFILER_STATE_IDLE;

/* Payload size used to measure the processing time. */
static volatile uint16_t current_profiling_payload = MIN_PAYLOAD_SIZE_BYTE;
/* Number of received packets in the current measurement. */
static volatile uint32_t received_packet_count;
/* Number of generated packets in the current measurement. */
static volatile uint32_t generated_packet_count;
/* Number of transmitted packets in the current measurement. */
static volatile uint32_t transmitted_packet_count;
/* Processing time of the transfer that has just completed. */
static volatile double processing_time_sample_us;
/* Timestamp handle to pass to the profiler. */
static uint32_t timestamp_handle;

/* Current scenario statistics. */
static scenario_stats_t *current_tx_stats;
static scenario_stats_t *current_rx_stats;

/* PRIVATE FUNCTION PROTOTYPE *************************************************/
static void conn_tx_success_callback(void *conn, void *arg);
static void conn_tx_fail_callback(void *conn, void *arg);
static void conn_rx_success_callback(void *conn, void *arg);
#if (SWC_RADIO_COUNT == 1)
static void radio_callback(void);
#elif (SWC_RADIO_COUNT == 2)
static void radio1_callback(void);
static void radio2_callback(void);
#endif
static void context_switch_callback(void);
static void packet_generation_timer_callback(void);

static void setup_tx_connection(void);
static void setup_rx_connection(void);
static void setup_bidir_connection(void);

static void run_unidir_tx_scenario(profiler_stats_t *profiler_stats);
static void run_unidir_rx_scenario(profiler_stats_t *profiler_stats);
static void run_bidir_bidir_scenario(profiler_stats_t *profiler_stats);

static void profiler_add_processing_time_tx_sample(void);
static void profiler_add_processing_time_rx_sample(void);

static void init_scenario_stats(scenario_stats_t *scenario_stats);
static void calculate_processing_time_average(scenario_stats_t *scenario_stats);
static double calculate_tx_base_proc(double total_tx_time_1, double nb_bytes_1, double total_tx_time_2,
                                     double nb_bytes_2);
static double calculate_spi_us_per_byte(double total_tx_time_1, double nb_bytes_1, double total_tx_time_2,
                                        double nb_bytes_2);

static double calculate_rx_payload_threshold(double first_total_rx_time, double last_total_rx_time,
                                             double spi_us_per_byte);

static double calculate_spi_transfer_time_us(double spi_us_per_byte, uint32_t nb_byte);
static void save_current_payload_size(volatile processing_time_stats_t *current_stats);

static void log_clean_line(void);
static void log_current_payload_size(void);
static void log_processing_time_average(scenario_stats_t *scenario_tx_stats, scenario_stats_t *scenario_rx_stats);
static void log_profiler_summary(profiler_stats_t *profiler_stats);
static uint32_t log_advanced_stats(char *buffer, uint32_t buffer_size, volatile processing_time_stats_t *stats);

/* PUBLIC FUNCTIONS ***********************************************************/
int main(void)
{
    profiler_stats_t profiler_stats = {0};

    facade_board_init();
    facade_profiler_init();
    facade_log_init();

    /* Setup packet generation timer. */
    facade_packet_generation_timer_init(PACKET_GENERATION_FEQUENCY_US);
    facade_packet_generation_set_timer_callback(packet_generation_timer_callback);

    /* Initialize Wireless Core context switch handler. */
    facade_set_context_switch_handler(swc_connection_callbacks_processing_handler);

    facade_log_write("\r\n\n=== Running SWC Profiler ===\r\n");
#if (SWC_RADIO_COUNT == 2)
    facade_log_write("\n**** Warning: Dual radio DUT ****\r\n");
    facade_log_write(" 1. Make sure that there are two radios modules on the DUT device.\r\n");
    facade_log_write(" 2. Make sure that both radios modules are within range of the helper device.\r\n\n");
#endif

    /* SCENARIO 1: DUT is configured as a transmitter. */
    run_unidir_tx_scenario(&profiler_stats);

    /* SCENARIO 2: DUT is configured as a Receiver. */
    run_unidir_rx_scenario(&profiler_stats);

    /* SCENARIO 3: DUT in a bidirectional setup. */
    run_bidir_bidir_scenario(&profiler_stats);

    log_profiler_summary(&profiler_stats);

    while (1);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize the SWC and the connection for the unidirectional transmission scenario. It also resets flags and
 *         metrics used for processing time measurments.
 */
static void setup_tx_connection(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    uint32_t timeslot_us[] = SCHEDULE;
    int32_t timeslots[] = TIMESLOTS;
    uint32_t channel_sequence[] = CHANNEL_SEQUENCE;
    uint32_t channel_frequency[] = CHANNEL_FREQ;

    /* Reset the synchronization flags for the next scenario. */
    current_profiling_payload = MIN_PAYLOAD_SIZE_BYTE;
    received_packet_count = 0;
    transmitted_packet_count = 0;
    generated_packet_count = 0;
    rx_conn = NULL;

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
        .pan_id = PAN_ID,
        .coordinator_address = COORDINATOR_ADDRESS,
        .local_address = COORDINATOR_ADDRESS,
    };

    swc_init(core_cfg, node_cfg, context_switch_callback, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    swc_radio_module_init(SWC_RADIO_ID_1, true, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

#if (SWC_RADIO_COUNT == 1)
    /* Set another callback to insert the profiler start in the Radio IRQ callback. */
    swc_hal_set_radio_1_irq_callback(radio_callback);
#elif (SWC_RADIO_COUNT == 2)
    /* Set another callback to insert the profiler start in the Radio IRQ callback. */
    swc_hal_set_radio_1_irq_callback(radio1_callback);

    /* Initialize second radio. */
    swc_radio_module_init(SWC_RADIO_ID_2, true, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    /* Set another callback to insert the profiler start in the Radio IRQ callback. */
    swc_hal_set_radio_2_irq_callback(radio2_callback);
#endif

    /* ** TX Connection ** */
    swc_connection_cfg_t tx_conn_cfg = {
        .name = "TX Connection",
        .source_address = COORDINATOR_ADDRESS,
        .destination_address = NODE_ADDRESS,
        .max_payload_size = MAX_PAYLOAD_SIZE_BYTE,
        .queue_size = SWC_QUEUE_SIZE,
        .timeslot_id = timeslots,
        .timeslot_count = ARRAY_SIZE(timeslots),
    };
    tx_conn = swc_connection_init(tx_conn_cfg, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    swc_channel_cfg_t tx_channel_cfg = {
        .tx_pulse_count = TX_DATA_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain = TX_DATA_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT,
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_frequency); i++) {
        tx_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(tx_conn, tx_channel_cfg, &swc_err);
        ASSERT_SWC_STATUS(swc_err);
    }
    swc_connection_set_tx_success_callback(tx_conn, conn_tx_success_callback, NULL, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    swc_connection_set_tx_fail_callback(tx_conn, conn_tx_fail_callback, NULL, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    swc_setup(&swc_err);
    ASSERT_SWC_STATUS(swc_err);
}

/** @brief Initialize the SWC and the connection for the unidirectional reception scenario. It also resets flags and
 *         metrics used for processing time measurements.
 */
static void setup_rx_connection(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    uint32_t timeslot_us[] = SCHEDULE;
    int32_t timeslots[] = TIMESLOTS;
    uint32_t channel_sequence[] = CHANNEL_SEQUENCE;
    uint32_t channel_frequency[] = CHANNEL_FREQ;

    /* Reset the synchronization flags for the next scenario. */
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

    /* For the unidirectional RX connection (Scenario 2), the DUT acts as the Node. */
    swc_node_cfg_t node_cfg = {
        .role = SWC_ROLE_NODE,
        .pan_id = PAN_ID,
        .coordinator_address = NODE_ADDRESS,
        .local_address = COORDINATOR_ADDRESS,
    };

    swc_init(core_cfg, node_cfg, context_switch_callback, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    swc_radio_module_init(SWC_RADIO_ID_1, true, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

#if (SWC_RADIO_COUNT == 1)
    /* Set another callback to insert the profiler start in the Radio IRQ callback. */
    swc_hal_set_radio_1_irq_callback(radio_callback);
#elif (SWC_RADIO_COUNT == 2)
    /* Set another callback to insert the profiler start in the Radio IRQ callback. */
    swc_hal_set_radio_1_irq_callback(radio1_callback);

    /* Initialize second radio. */
    swc_radio_module_init(SWC_RADIO_ID_2, true, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    /* Set another callback to insert the profiler start in the Radio IRQ callback. */
    swc_hal_set_radio_2_irq_callback(radio2_callback);
#endif

    /* ** RX Connection ** */
    swc_connection_cfg_t rx_conn_cfg = {
        .name = "RX Connection",
        .source_address = NODE_ADDRESS,
        .destination_address = COORDINATOR_ADDRESS,
        .max_payload_size = MAX_PAYLOAD_SIZE_BYTE,
        .queue_size = SWC_QUEUE_SIZE,
        .timeslot_id = timeslots,
        .timeslot_count = ARRAY_SIZE(timeslots),
    };
    rx_conn = swc_connection_init(rx_conn_cfg, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    swc_channel_cfg_t rx_channel_cfg = {
        .tx_pulse_count = TX_ACK_PULSE_COUNT,
        .tx_pulse_width = TX_ACK_PULSE_WIDTH,
        .tx_pulse_gain = TX_ACK_PULSE_GAIN,
        .rx_pulse_count = RX_DATA_PULSE_COUNT,
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_frequency); i++) {
        rx_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(rx_conn, rx_channel_cfg, &swc_err);
        ASSERT_SWC_STATUS(swc_err);
    }
    swc_connection_set_rx_success_callback(rx_conn, conn_rx_success_callback, NULL, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    swc_setup(&swc_err);
    ASSERT_SWC_STATUS(swc_err);
}

/** @brief Initialize the SWC and the connection for the bidirectional transmission-reception scenario. It also resets
 *         flags and metrics used for processing time measurements.
 */
static void setup_bidir_connection(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    uint32_t bidir_timeslot_us[] = BIDIR_SCHEDULE;
    int32_t bidir_tx_timeslots[] = BIDIR_TIMESLOTS_1;
    int32_t bidir_rx_timeslots[] = BIDIR_TIMESLOTS_0;
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

    swc_node_cfg_t node_cfg = {
        .role = SWC_ROLE_COORDINATOR,
        .pan_id = PAN_ID,
        .coordinator_address = COORDINATOR_ADDRESS,
        .local_address = COORDINATOR_ADDRESS,
    };

    swc_init(core_cfg, node_cfg, context_switch_callback, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    swc_radio_module_init(SWC_RADIO_ID_1, true, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

#if (SWC_RADIO_COUNT == 1)
    /* Set another callback to insert the profiler start in the Radio IRQ callback. */
    swc_hal_set_radio_1_irq_callback(radio_callback);
#elif (SWC_RADIO_COUNT == 2)
    /* Set another callback to insert the profiler start in the Radio IRQ callback. */
    swc_hal_set_radio_1_irq_callback(radio1_callback);

    /* Initialize second radio. */
    swc_radio_module_init(SWC_RADIO_ID_2, true, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    /* Set another callback to insert the profiler start in the Radio IRQ callback. */
    swc_hal_set_radio_2_irq_callback(radio2_callback);
#endif

    /* ** TX Connection ** */
    swc_connection_cfg_t tx_conn_cfg = {
        .name = "TX Connection",
        .source_address = COORDINATOR_ADDRESS,
        .destination_address = NODE_ADDRESS,
        .max_payload_size = MAX_PAYLOAD_SIZE_BYTE,
        .queue_size = SWC_QUEUE_SIZE,
        .timeslot_id = bidir_tx_timeslots,
        .timeslot_count = ARRAY_SIZE(bidir_tx_timeslots),
    };
    tx_conn = swc_connection_init(tx_conn_cfg, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    swc_channel_cfg_t tx_channel_cfg = {
        .tx_pulse_count = TX_DATA_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain = TX_DATA_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT,
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_frequency); i++) {
        tx_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(tx_conn, tx_channel_cfg, &swc_err);
        ASSERT_SWC_STATUS(swc_err);
    }
    swc_connection_set_tx_success_callback(tx_conn, conn_tx_success_callback, NULL, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    swc_connection_set_tx_fail_callback(tx_conn, conn_tx_fail_callback, NULL, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    /* ** RX Connection ** */
    swc_connection_cfg_t rx_conn_cfg = {
        .name = "RX Connection",
        .source_address = NODE_ADDRESS,
        .destination_address = COORDINATOR_ADDRESS,
        .max_payload_size = MAX_PAYLOAD_SIZE_BYTE,
        .queue_size = SWC_QUEUE_SIZE,
        .timeslot_id = bidir_rx_timeslots,
        .timeslot_count = ARRAY_SIZE(bidir_rx_timeslots),
    };
    rx_conn = swc_connection_init(rx_conn_cfg, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    swc_channel_cfg_t rx_channel_cfg = {
        .tx_pulse_count = TX_ACK_PULSE_COUNT,
        .tx_pulse_width = TX_ACK_PULSE_WIDTH,
        .tx_pulse_gain = TX_ACK_PULSE_GAIN,
        .rx_pulse_count = RX_DATA_PULSE_COUNT,
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_frequency); i++) {
        rx_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(rx_conn, rx_channel_cfg, &swc_err);
        ASSERT_SWC_STATUS(swc_err);
    }
    swc_connection_set_rx_success_callback(rx_conn, conn_rx_success_callback, NULL, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    swc_setup(&swc_err);
    ASSERT_SWC_STATUS(swc_err);
}

/** @brief Callback function when a previously sent frame has been ACK'd.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 *  @param[in] arg   Additional argument for the callback function.
 */
static void conn_tx_success_callback(void *conn, void *arg)
{
    (void)conn;
    (void)arg;

    /* Count successfully transmitted packets. */
    if (transmitted_packet_count >= (AVERAGE_LENGTH - 1)) {
        state_tx = PROFILER_STATE_INCREMENT_PAYLOAD_SIZE;
    } else {
        /* Save the processing time measurement following a transmission.
         *  - The first packet is not measured since it can be an outlier (ex: instruction cache miss, event, etc.)
         *  - The last packet is not measured since no other packet will be loaded into the radio's FIFO.
         */
        if (transmitted_packet_count++ > 1) {
            profiler_add_processing_time_tx_sample();
        }
    }

    facade_tx_conn_status();
}

/** @brief Callback function when a previously sent frame has been ACK'd.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 *  @param[in] arg   Additional argument for the callback function.
 */
static void conn_tx_fail_callback(void *conn, void *arg)
{
    static uint8_t last_packet_tx_fail_count;

    (void)conn;
    (void)arg;

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

/** @brief Callback function when a frame has been successfully received.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 *  @param[in] arg   Additional argument for the callback function.
 */
static void conn_rx_success_callback(void *conn, void *arg)
{
    (void)conn;
    (void)arg;

    swc_error_t err = SWC_ERR_NONE;
    uint8_t *payload = NULL;
    uint16_t received_payload_size;

    /* Retrieve the received packet from the SWC queue. */
    received_payload_size = swc_connection_receive(rx_conn, &payload, &err);
    ASSERT_SWC_STATUS(err);

    swc_connection_receive_complete(rx_conn, &err);
    ASSERT_SWC_STATUS(err);

    /* Only packets matching the expected size are counted. This ensures robustness against mismatched test steps
     * between devices in case of test procedure desynchronization.
     */
    if (received_payload_size == current_profiling_payload) {
        /* Count successfully received packets. */
        if (received_packet_count >= (AVERAGE_LENGTH - 1)) {
            state_rx = PROFILER_STATE_INCREMENT_PAYLOAD_SIZE;
        } else {
            /* Save the processing time measurement following a reception.
             *  - The first packet is not measured since it can be an outlier (ex: instruction cache miss, event, etc.)
             */
            if (received_packet_count++ > 1) {
                profiler_add_processing_time_rx_sample();
            }
        }
    }

    facade_rx_conn_status();
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
            ASSERT_SWC_STATUS(swc_err);

            /* The count of generated packets is incremented. */
            generated_packet_count++;
        }
    }
}

#if (SWC_RADIO_COUNT == 1)
/** @brief Handles the radio interrupt and starts the profiler.
 */
static void radio_callback(void)
{
    /* The profiler starts counting from here. */
    facade_profiler_start(&timestamp_handle);
    swc_radio_irq_handler();
}
#elif (SWC_RADIO_COUNT == 2)

/** @brief Handles the radio interrupt and starts the profiler.
 */
static void radio1_callback(void)
{
    /* The profiler starts counting from here. */
    facade_profiler_start(&timestamp_handle);
    swc_radio1_irq_handler();
}
/** @brief Handles the radio interrupt and starts the profiler.
 */
static void radio2_callback(void)
{
    /* The profiler starts counting from here. */
    facade_profiler_start(&timestamp_handle);
    swc_radio2_irq_handler();
}
#endif

/** @brief Handles a context switch event and stops the profiler.
 */
static void context_switch_callback(void)
{
    /* The profiler stops counting, and the elapsed time between the start and stop is retrieved. */
    uint32_t elapsed_cycles = facade_profiler_stop(&timestamp_handle);

    processing_time_sample_us = facade_profiler_get_elapsed_us(elapsed_cycles);
    facade_context_switch_trigger();
}

/** @brief Run the unidirectional transmission scenario.
 */
static void run_unidir_tx_scenario(profiler_stats_t *profiler_stats)
{
    /** Scenario 1: The DUT (Device Under Test) is configured as the transmitter and the Helper as the receiver,
     *              transferring a set of payloads with varying sizes.
     */

    swc_error_t swc_err = SWC_ERR_NONE;
    scenario_stats_t scenario_tx_stats = {0};

    init_scenario_stats(&scenario_tx_stats);

    current_rx_stats = NULL;
    current_tx_stats = &scenario_tx_stats;

    facade_log_write("\r\n\nSCENARIO 1: Measuring processing time for unidirectional transmission link.\r\n");

    /* Setup the SWC and the TX connection. */
    setup_tx_connection();

    /* Only the processing time of the transmission will be measured in this scenario. */
    state_tx = PROFILER_STATE_RUNNING;
    state_rx = PROFILER_STATE_IDLE;

    while (state_tx != PROFILER_STATE_COMPLETE) {
        log_current_payload_size();

        /* Connect the wireless core. */
        swc_connect(&swc_err);
        ASSERT_SWC_STATUS(swc_err);

        /* Start the generation of packets. */
        facade_packet_generation_timer_start();

        /** Wait until all profiling size payloads are sent.
         *  The state transitions to PROFILER_STATE_INCREMENT_PAYLOAD_SIZE in the conn_tx_success_callback when the
         *  count is reached.
         */
        while (state_tx != PROFILER_STATE_INCREMENT_PAYLOAD_SIZE);

        /* Stop the timer used for the generation of new packets. */
        facade_packet_generation_timer_stop();

        save_current_payload_size(&scenario_tx_stats.processing_time_stats[scenario_tx_stats.current_index++]);

        /* If the payload size was the max value, all processing times are measured, and the scenario is complete. */
        if (current_profiling_payload >= MAX_PAYLOAD_SIZE_BYTE) {
            state_tx = PROFILER_STATE_COMPLETE;
        } else {
            /* Disconnect the Wireless Core. */
            swc_disconnect(&swc_err);
            ASSERT_SWC_STATUS(swc_err);

            /* Flush packets from TX queue. */
            swc_connection_flush_queue(tx_conn, &swc_err);
            ASSERT_SWC_STATUS(swc_err);

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
    log_clean_line();

    /* Disconnect the Wireless Core. */
    swc_disconnect(&swc_err);
    ASSERT_SWC_STATUS(swc_err);

    /* Compute and log the average processing time. */
    calculate_processing_time_average(&scenario_tx_stats);

    profiler_stats->tx_base_proc_time_us =
        calculate_tx_base_proc(scenario_tx_stats.processing_time_stats[0].max,
                               MIN_PAYLOAD_SIZE_BYTE + TX_PAYLOAD_SPI_OVERHEAD,
                               scenario_tx_stats.processing_time_stats[PAYLOAD_SIZE_COUNT - 1].max,
                               MAX_PAYLOAD_SIZE_BYTE + TX_PAYLOAD_SPI_OVERHEAD);
    profiler_stats->spi_us_per_byte =
        calculate_spi_us_per_byte(scenario_tx_stats.processing_time_stats[0].max,
                                  MIN_PAYLOAD_SIZE_BYTE + TX_PAYLOAD_SPI_OVERHEAD,
                                  scenario_tx_stats.processing_time_stats[PAYLOAD_SIZE_COUNT - 1].max,
                                  MAX_PAYLOAD_SIZE_BYTE + TX_PAYLOAD_SPI_OVERHEAD);

    log_processing_time_average(&scenario_tx_stats, NULL);
}

/** @brief Run the unidirectional reception scenario.
 */
static void run_unidir_rx_scenario(profiler_stats_t *profiler_stats)
{
    /** Scenario 2: The DUT is configured as the receiver and the Helper as the transmitter, receiving a similar set of
     *              payloads with varying sizes.
     */

    swc_error_t swc_err = SWC_ERR_NONE;
    scenario_stats_t scenario_rx_stats = {0};

    init_scenario_stats(&scenario_rx_stats);

    current_rx_stats = &scenario_rx_stats;
    current_tx_stats = NULL;

    facade_log_write("\r\n\nSCENARIO 2: Measuring processing time for unidirectional reception link.\r\n");

    /* Setup the SWC and the RX connection. */
    setup_rx_connection();

    /* Only the processing time of the reception will be measured in this scenario. */
    state_tx = PROFILER_STATE_IDLE;
    state_rx = PROFILER_STATE_RUNNING;

    while (state_rx != PROFILER_STATE_COMPLETE) {
        log_current_payload_size();

        /* Connect the wireless core. */
        swc_connect(&swc_err);
        ASSERT_SWC_STATUS(swc_err);

        /** Wait until all profiling size payloads are received.
         *  The state transitions to PROFILER_STATE_INCREMENT_PAYLOAD_SIZE in the conn_rx_success_callback when the
         *  count is reached.
         */
        while (state_rx != PROFILER_STATE_INCREMENT_PAYLOAD_SIZE);

        save_current_payload_size(&scenario_rx_stats.processing_time_stats[scenario_rx_stats.current_index++]);

        /* If the payload size was the max value, all processing times are measured, and the scenario is complete. */
        if (current_profiling_payload >= MAX_PAYLOAD_SIZE_BYTE) {
            state_rx = PROFILER_STATE_COMPLETE;
        } else {
            /* Disconnect the Wireless Core. */
            swc_disconnect(&swc_err);
            ASSERT_SWC_STATUS(swc_err);

            /* Increase the payload size of the packet. */
            current_profiling_payload <<= 1;
            if (current_profiling_payload > MAX_PAYLOAD_SIZE_BYTE) {
                current_profiling_payload = MAX_PAYLOAD_SIZE_BYTE;
            }
            /* Restart the transfers. */
            state_rx = PROFILER_STATE_RUNNING;
            received_packet_count = 0;
        }
    }
    log_clean_line();

    /* Disconnect the Wireless Core. */
    swc_disconnect(&swc_err);
    ASSERT_SWC_STATUS(swc_err);

    /* Compute and log the average processing time. */
    calculate_processing_time_average(&scenario_rx_stats);

    /* The RX payload SPI transfer runs in parallel and doesn’t impact total RX processing time for short payloads. */
    profiler_stats->rx_base_proc_time_us = scenario_rx_stats.processing_time_stats[0].max;
    profiler_stats->rx_payload_threshold =
        calculate_rx_payload_threshold(scenario_rx_stats.processing_time_stats[0].max,
                                       scenario_rx_stats.processing_time_stats[PAYLOAD_SIZE_COUNT - 1].max,
                                       profiler_stats->spi_us_per_byte);

    log_processing_time_average(NULL, &scenario_rx_stats);
}

/** @brief Run the bidirectional transmission-reception scenario.
 */
static void run_bidir_bidir_scenario(profiler_stats_t *profiler_stats)
{
    /** Scenario 3: Both the DUT and Helper are set up in a bidirectional configuration, transferring and receiving
     *              payloads of different sizes.
     */

    swc_error_t swc_err = SWC_ERR_NONE;
    scenario_stats_t scenario_rx_stats = {0};
    scenario_stats_t scenario_tx_stats = {0};

    init_scenario_stats(&scenario_rx_stats);
    init_scenario_stats(&scenario_tx_stats);

    current_rx_stats = &scenario_rx_stats;
    current_tx_stats = &scenario_tx_stats;

    facade_log_write("\r\n\nSCENARIO 3: Measuring processing time for bidirectional link.\r\n");

    /* Setup the SWC and the TX and RX connections. */
    setup_bidir_connection();

    /* The processing time for both transmission and reception will be measured in this scenario. */
    state_tx = PROFILER_STATE_RUNNING;
    state_rx = PROFILER_STATE_RUNNING;

    while (state_rx != PROFILER_STATE_COMPLETE || state_tx != PROFILER_STATE_COMPLETE) {
        log_current_payload_size();

        /* Connect the wireless core. */
        swc_connect(&swc_err);
        ASSERT_SWC_STATUS(swc_err);

        /* Start the generation of packets. */
        facade_packet_generation_timer_start();

        /* Wait until both sequence are finished. (Packets with the right payload size has been sent and received) */
        while (state_rx != PROFILER_STATE_INCREMENT_PAYLOAD_SIZE || state_tx != PROFILER_STATE_INCREMENT_PAYLOAD_SIZE);

        /* Stop the timer used for the generation of new packets. */
        facade_packet_generation_timer_stop();

        /* Disconnect the Wireless Core. */
        swc_disconnect(&swc_err);
        ASSERT_SWC_STATUS(swc_err);

        /* Flush packets from TX queue. */
        swc_connection_flush_queue(tx_conn, &swc_err);
        ASSERT_SWC_STATUS(swc_err);

        save_current_payload_size(&scenario_rx_stats.processing_time_stats[scenario_rx_stats.current_index++]);
        save_current_payload_size(&scenario_tx_stats.processing_time_stats[scenario_tx_stats.current_index++]);

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
    log_clean_line();

    /* Compute and log the average processing time. */
    calculate_processing_time_average(&scenario_rx_stats);
    calculate_processing_time_average(&scenario_tx_stats);

    /* Difference between the current base processing time and the expected measurement using rx_base_proc_time_us and a
     * TX SPI transfer.
     */
    double expected_proc = (profiler_stats->rx_base_proc_time_us +
                            calculate_spi_transfer_time_us(profiler_stats->spi_us_per_byte,
                                                           MIN_PAYLOAD_SIZE_BYTE + TX_PAYLOAD_SPI_OVERHEAD));
    profiler_stats->rxtx_overhead_time_us = scenario_rx_stats.processing_time_stats[0].max - expected_proc;
    /* Add rxtx overhead to the rx_base_proc_time_us. */
    profiler_stats->rxtx_base_proc_time_us = profiler_stats->rx_base_proc_time_us +
                                             profiler_stats->rxtx_overhead_time_us;

    log_processing_time_average(&scenario_tx_stats, &scenario_rx_stats);
}

/** @brief Adds the latest measured processing time sample to the total transmission processing time.
 */
static void profiler_add_processing_time_tx_sample(void)
{
    volatile processing_time_stats_t *stats = &current_tx_stats->processing_time_stats[current_tx_stats->current_index];

    if (processing_time_sample_us) {
        if (processing_time_sample_us > stats->max) {
            stats->max = processing_time_sample_us;
            stats->max_idx = stats->sum_count;
        }
        if (processing_time_sample_us < stats->min) {
            stats->min = processing_time_sample_us;
            stats->min_idx = stats->sum_count;
        }
        stats->sum += processing_time_sample_us;
        stats->sum_count++;
        processing_time_sample_us = 0;
    }
}

/** @brief Adds the latest measured processing time sample to the total reception processing time.
 */
static void profiler_add_processing_time_rx_sample(void)
{
    volatile processing_time_stats_t *stats = &current_rx_stats->processing_time_stats[current_rx_stats->current_index];

    if (processing_time_sample_us) {
        if (processing_time_sample_us > stats->max) {
            stats->max = processing_time_sample_us;
            stats->max_idx = stats->sum_count;
        }
        if (processing_time_sample_us < stats->min) {
            stats->min = processing_time_sample_us;
            stats->min_idx = stats->sum_count;
        }
        stats->sum += processing_time_sample_us;
        stats->sum_count++;
        processing_time_sample_us = 0;
    }
}

/** @brief Computes the average processing time for all scenario measurements.
 */
static void calculate_processing_time_average(scenario_stats_t *scenario_stats)
{
    volatile processing_time_stats_t *stats = scenario_stats->processing_time_stats;

    for (uint8_t i = 0; i < scenario_stats->current_index; i++) {
        stats[i].average = stats[i].sum / stats[i].sum_count;
    }
}

/** @brief Calculate the base TX processing time without the SPI transfer.
 *
 *  @param[in] total_tx_time_1    Total processing time of the measurement no 1.
 *  @param[in] nb_tx_spi_bytes_1  Number of bytes in the TX SPI transfer of the measurement no 1.
 *  @param[in] total_tx_time_2    Total processing time of the measurement no 2.
 *  @param[in] nb_tx_spi_bytes_2  Number of bytes in the TX SPI transfer of the measurement no 2.
 *  @return The base TX processing time without the SPI transfer.
 */
static double calculate_tx_base_proc(double total_tx_time_1, double nb_tx_spi_bytes_1, double total_tx_time_2,
                                     double nb_tx_spi_bytes_2)
{
    return ((total_tx_time_1 * (nb_tx_spi_bytes_2)) - (total_tx_time_2 * (nb_tx_spi_bytes_1))) /
           (nb_tx_spi_bytes_2 - nb_tx_spi_bytes_1);
}

/** @brief Calculate the number of microseconds for each byte transfered on the SPI.
 *
 *  @param[in] total_tx_time_1    Total processing time of the measurement no 1.
 *  @param[in] nb_tx_spi_bytes_1  Number of bytes in the TX SPI transfer of the measurement no 1.
 *  @param[in] total_tx_time_2    Total processing time of the measurement no 2.
 *  @param[in] nb_tx_spi_bytes_2  Number of bytes in the TX SPI transfer of the measurement no 2.
 *  @return The number of microseconds for each byte transfered on the SPI.
 */
static double calculate_spi_us_per_byte(double total_tx_time_1, double nb_tx_spi_bytes_1, double total_tx_time_2,
                                        double nb_tx_spi_bytes_2)
{
    return (total_tx_time_2 - total_tx_time_1) / (nb_tx_spi_bytes_2 - nb_tx_spi_bytes_1);
}

/** @brief Calculate the RX payload threshold where the SPI transfer becomes longer than the MCU processing happening in
 *         the background.
 *
 *  @param[in] first_total_rx_time  Total processing time of the first measurement in the RX scenario.
 *  @param[in] last_total_rx_time   Total processing time of the last measurement in the RX scenario.
 *  @param[in] spi_us_per_byte      The number of microseconds for each byte transfered on the SPI.
 *  @return The RX payload threshold.
 */
static double calculate_rx_payload_threshold(double first_total_rx_time, double last_total_rx_time,
                                             double spi_us_per_byte)
{
    return MAX_PAYLOAD_SIZE_BYTE - (last_total_rx_time - first_total_rx_time) / spi_us_per_byte;
}

/** @brief Calculate the time of a SPI transfer.
 *
 *  @param[in] spi_us_per_byte  The number of microseconds for each byte transfered on the SPI.
 *  @param[in] nb_byte          Number of bytes to transfer.
 *  @return The time of a SPI transfer.
 */
static double calculate_spi_transfer_time_us(double spi_us_per_byte, uint32_t nb_byte)
{
    return nb_byte * spi_us_per_byte;
}

/** @brief Save the payload size of the current measurement.
 *
 *  @param[in] current_stats  Pointer to the statistics structure of the current measurement.
 */
static void save_current_payload_size(volatile processing_time_stats_t *current_stats)
{
    current_stats->payload_size = current_profiling_payload;
}

/** @brief Init scenario stats structure to default values.
 *
 *  @param[in] scenario_stats  Pointer to scenario stats structure.
 */
static void init_scenario_stats(scenario_stats_t *scenario_stats)
{
    scenario_stats->current_index = 0;
    for (uint8_t i = 0; i < PAYLOAD_SIZE_COUNT; i++) {
        /* Reset all parameters to 0. */
        memset((void *)&scenario_stats->processing_time_stats[i], 0, sizeof(processing_time_stats_t));
        /* Initialize the min parameter to maximum value possible. */
        scenario_stats->processing_time_stats[i].min = __DBL_MAX__;
    }
}

/** @brief Clean current log line.
 */
static void log_clean_line(void)
{
    /* Clear log buffer. */
    memset(log_buffer, 0, LOG_BUFFER_SIZE);
    /* Make an empty line. */
    memset(log_buffer, ' ', LOG_LINE_BUFFER_SIZE);
    log_buffer[0] = '\r';
    log_buffer[LOG_LINE_BUFFER_SIZE - 1] = '\r';
    facade_log_write(log_buffer);
}

/** @brief Print current payload size.
 */
static void log_current_payload_size(void)
{
    /* Clear log buffer. */
    memset(log_buffer, 0, LOG_BUFFER_SIZE);
    snprintf(log_buffer, sizeof(log_buffer), "\rCurrent Payload size: %3u bytes", current_profiling_payload);
    facade_log_write(log_buffer);
}

/** @brief Logs the average processing time and the associated payload size.
 *
 *  @param[in] scenario_tx_stats  Pointer the scenario's TX statistics.
 *  @param[in] scenario_rx_stats  Pointer the scenario's RX statistics.
 */
static void log_processing_time_average(scenario_stats_t *scenario_tx_stats, scenario_stats_t *scenario_rx_stats)
{
    int string_length = 0;
    uint32_t payload_size = 0;

    /* Clear log buffer. */
    memset(log_buffer, 0, LOG_BUFFER_SIZE);
    for (int i = 0; i < PAYLOAD_SIZE_COUNT; i++) {
        payload_size = (scenario_tx_stats != NULL) ? scenario_tx_stats->processing_time_stats[i].payload_size :
                                                     scenario_rx_stats->processing_time_stats[i].payload_size;
        string_length += snprintf(log_buffer + string_length, sizeof(log_buffer) - string_length,
                                  "\r\n< Payload size: %lu bytes >", payload_size);
        if (scenario_tx_stats) {
            volatile processing_time_stats_t *tx_stats = scenario_tx_stats->processing_time_stats;

            if (PRINT_ALL_STATS) {
                string_length += snprintf(log_buffer + string_length, sizeof(log_buffer) - string_length,
                                          "\r\n<< TX Processing Time >>");
                string_length += log_advanced_stats(log_buffer + string_length, sizeof(log_buffer) - string_length,
                                                    &tx_stats[i]);
            } else {
                string_length += snprintf(log_buffer + string_length, sizeof(log_buffer) - string_length,
                                          "\r\n\tTX Processing Time: %7.3f us", tx_stats[i].max);
            }
        }
        if (scenario_rx_stats) {
            volatile processing_time_stats_t *rx_stats = scenario_rx_stats->processing_time_stats;

            if (PRINT_ALL_STATS) {
                string_length += snprintf(log_buffer + string_length, sizeof(log_buffer) - string_length,
                                          "\r\n<< RX Processing Time >>");
                string_length += log_advanced_stats(log_buffer + string_length, sizeof(log_buffer) - string_length,
                                                    &rx_stats[i]);
            } else {
                string_length += snprintf(log_buffer + string_length, sizeof(log_buffer) - string_length,
                                          "\r\n\tRX Processing Time: %7.3f us", rx_stats[i].max);
            }
        }
    }

    facade_log_write(log_buffer);
}

/** @brief Log advanced statistics to the user.
 *
 *  @param[in] buffer       Buffer to write to.
 *  @param[in] buffer_size  Size of the buffer.
 *  @param[in] stats        Statistics to log.
 */
static uint32_t log_advanced_stats(char *buffer, uint32_t buffer_size, volatile processing_time_stats_t *stats)
{
    uint32_t string_length = 0;

    string_length += snprintf(buffer + string_length, buffer_size - string_length, "\r\n\t%-7s: %7.3f us, count: %lu",
                              "Average", stats->average, stats->sum_count);
    string_length += snprintf(buffer + string_length, buffer_size - string_length, "\r\n\t%-7s: %7.3f us, index: %lu",
                              "Max", stats->max, stats->max_idx);
    string_length += snprintf(buffer + string_length, buffer_size - string_length, "\r\n\t%-7s: %7.3f us, index: %lu",
                              "Min", stats->min, stats->min_idx);
    string_length += snprintf(buffer + string_length, buffer_size - string_length, "\r\n\t%-7s: %7.3f us", "Range",
                              stats->max - stats->min);

    return string_length;
}

/** @brief Log the SPI speed and processing time formulas.
 */
static void log_profiler_summary(profiler_stats_t *profiler_stats)
{
    uint32_t string_length = 0;

    /* Clear log buffer. */
    memset(log_buffer, 0, LOG_BUFFER_SIZE);
    string_length += snprintf(log_buffer + string_length, sizeof(log_buffer) - string_length,
                              "\r\n\n=== SWC Profiler Summary ===\r\n");
    string_length += snprintf(log_buffer + string_length, sizeof(log_buffer) - string_length,
                              "\n<<< Constants >>>\r\n"
                              " TX Base processing time: %.3f us\r\n"
                              " RX Base processing time: %.3f us\r\n"
                              " RX Payload processing threshold: %.3f Bytes\r\n"
                              " RX-TX Base processing time: %.3f us\r\n",
                              profiler_stats->tx_base_proc_time_us, profiler_stats->rx_base_proc_time_us,
                              profiler_stats->rx_payload_threshold, profiler_stats->rxtx_base_proc_time_us);
    string_length += snprintf(log_buffer + string_length, sizeof(log_buffer) - string_length,
                              " SPI Time per Byte: %.6f us / Byte\r\n"
                              " SPI Speed: %.3f Mbps\r\n",
                              profiler_stats->spi_us_per_byte, 8 / profiler_stats->spi_us_per_byte);
    string_length += snprintf(
        log_buffer + string_length, sizeof(log_buffer) - string_length,
        "\n< Processing Formulas >\r\n"
        "<< TX processing formula >>\r\n"
        "   proc = %.3f us + (CMD_BYTE + MAC_HDR + nb_tx_byte) * %.3f us/byte\r\n"
        "<< RX processing formula >>\r\n"
        "   proc = %.3f us + max((nb_rx_byte - %.3f), 0) * %.3f us/byte\r\n"
        "<< RX-TX processing formula >>\r\n"
        "   proc = %.3f us + (max((nb_rx_byte - %.3f), 0) + (CMD_BYTE + MAC_HDR + nb_tx_byte)) * %.3f us/byte\r\n",
        profiler_stats->tx_base_proc_time_us, profiler_stats->spi_us_per_byte, profiler_stats->rx_base_proc_time_us,
        profiler_stats->rx_payload_threshold, profiler_stats->spi_us_per_byte, profiler_stats->rxtx_base_proc_time_us,
        profiler_stats->rx_payload_threshold, profiler_stats->spi_us_per_byte);

    facade_log_write(log_buffer);
}

void swc_error_handler(swc_error_t swc_status)
{
    char buffer[ERROR_MESSAGE_BUFFER_SIZE];

    sprintf(buffer, "SWC Error ! Code: %d\n\r", swc_status);
    facade_log_error_string(buffer);

    while (1);
}
