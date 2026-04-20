/** @file  hello_world_node.c
 *  @brief This is a basic example of how to use the SPARK Wireless Core.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include <stdio.h>
#include "hello_world_facade.h"
#include "pairing_api.h"
#include "pairing_cfg.h"
#include "swc_api.h"
#include "swc_cfg.h"
#include "swc_cfg_node.h"
#include "swc_error.h"
#include "swc_stats.h"

/* CONSTANTS ******************************************************************/
/* More memory is needed when using dual radio. */
#if (SWC_RADIO_COUNT == 2)
#define SWC_MEM_POOL_SIZE 10000
#else
#define SWC_MEM_POOL_SIZE 6100
#endif

#define MAX_PAYLOAD_SIZE_BYTE      30
#define ENDING_NULL_CHARACTER_SIZE 1
#define STATS_ARRAY_LENGTH         1024
#define PRINT_INTERVAL_MS          1000
/* Size of the buffer used to print errors. */
#define ERROR_MESSAGE_BUFFER_SIZE 50

/* TYPES **********************************************************************/
/** @brief Enumeration representing device pairing states.
 */
typedef enum device_pairing_state {
    /*! The device is unpaired with the Coordinator. */
    DEVICE_UNPAIRED,
    /*! The device pairing is active. */
    DEVICE_PAIRING,
    /*! The device is paired with the Coordinator. */
    DEVICE_PAIRED,
} device_pairing_state_t;

/* PRIVATE GLOBALS ***********************************************************/
/* ** Wireless Core ** */
static uint8_t swc_memory_pool[SWC_MEM_POOL_SIZE];
static swc_connection_t *rx_conn;
static swc_connection_t *tx_conn;

static uint32_t timeslot_us[] = SCHEDULE;
static uint32_t channel_sequence[] = CHANNEL_SEQUENCE;
static uint32_t channel_frequency[] = CHANNEL_FREQ;
static int32_t tx_timeslots[] = NODE_TIMESLOTS;
static int32_t rx_timeslots[] = COORD_TIMESLOTS;

/* ** Application Specific ** */
static char rx_payload[MAX_PAYLOAD_SIZE_BYTE];
static bool reset_stats_now;
static uint32_t str_counter;
static facade_certification_mode_t certification_mode;
/* Variables supporting pairing between the two devices. */
static device_pairing_state_t device_pairing_state;
static pairing_cfg_t app_pairing_cfg;
static pairing_assigned_address_t pairing_assigned_address;

/* PRIVATE FUNCTION PROTOTYPE *************************************************/
static void app_init(void);
static void app_swc_core_init(pairing_assigned_address_t *app_pairing, swc_error_t *err);
static void conn_tx_success_callback(void *conn, void *arg);
static void conn_tx_fail_callback(void *conn, void *arg);
static void conn_rx_success_callback(void *conn, void *arg);

static bool should_print_stats(void);
static void print_stats(void);
static void reset_stats(void);

static void enter_pairing_mode(void);
static void unpair_device(void);

static void pairing_application_callback(void);
static void abort_pairing_procedure(void);
static void packet_generation_timer_interrupt_handler(void);
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
    };
    facade_set_button_callbacks(button_callbacks);

    /* Setup higher priority packet generation timer */
    facade_packet_generation_timer_init(timeslot_us[0]);
    facade_packet_generation_set_timer_callback(packet_generation_timer_interrupt_handler);

    certification_mode = facade_get_certification_mode();
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
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize the application.
 */
static void app_init(void)
{
    swc_error_t swc_err;

    app_swc_core_init(&pairing_assigned_address, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    swc_connect(&swc_err);
    ASSERT_SWC_STATUS(swc_err);

    facade_packet_generation_timer_start();
}

/** @brief Initialize the Wireless Core.
 *
 *  @param[in]  app_pairing  Configure the Wireless Core with the pairing values.
 *  @param[out] err          Wireless Core error code.
 */
static void app_swc_core_init(pairing_assigned_address_t *app_pairing, swc_error_t *err)
{
    uint16_t local_address = app_pairing->node_address;
    uint16_t remote_address = app_pairing->coordinator_address;

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
        .role = SWC_ROLE_NODE,
        .pan_id = app_pairing->pan_id,
        .coordinator_address = remote_address,
        .local_address = local_address,
    };

    swc_init(core_cfg, node_cfg, facade_context_switch_trigger, err);
    ASSERT_SWC_STATUS(*err);

    swc_radio_module_init(SWC_RADIO_ID_1, true, err);
    ASSERT_SWC_STATUS(*err);

#if (SWC_RADIO_COUNT == 2)
    swc_radio_module_init(SWC_RADIO_ID_2, true, err);
    ASSERT_SWC_STATUS(*err);
#endif

    /* ** TX Connection ** */
    swc_connection_cfg_t tx_conn_cfg = {
        .name = "TX Connection",
        .source_address = local_address,
        .destination_address = remote_address,
        .max_payload_size = MAX_PAYLOAD_SIZE_BYTE + ENDING_NULL_CHARACTER_SIZE,
        .queue_size = SWC_QUEUE_SIZE,
        .timeslot_id = tx_timeslots,
        .timeslot_count = ARRAY_SIZE(tx_timeslots),
    };
    tx_conn = swc_connection_init(tx_conn_cfg, err);
    ASSERT_SWC_STATUS(*err);

    swc_channel_cfg_t tx_channel_cfg = {
        .tx_pulse_count = TX_DATA_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain = TX_DATA_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT,
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_frequency); i++) {
        tx_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(tx_conn, tx_channel_cfg, err);
        ASSERT_SWC_STATUS(*err);
    }
    swc_connection_set_tx_success_callback(tx_conn, conn_tx_success_callback, NULL, err);
    ASSERT_SWC_STATUS(*err);

    swc_connection_set_tx_fail_callback(tx_conn, conn_tx_fail_callback, NULL, err);
    ASSERT_SWC_STATUS(*err);

    /* ** RX Connection ** */
    swc_connection_cfg_t rx_conn_cfg = {
        .name = "RX Connection",
        .source_address = remote_address,
        .destination_address = local_address,
        .max_payload_size = MAX_PAYLOAD_SIZE_BYTE + ENDING_NULL_CHARACTER_SIZE,
        .queue_size = SWC_QUEUE_SIZE,
        .timeslot_id = rx_timeslots,
        .timeslot_count = ARRAY_SIZE(rx_timeslots),
    };
    rx_conn = swc_connection_init(rx_conn_cfg, err);
    ASSERT_SWC_STATUS(*err);

    swc_channel_cfg_t rx_channel_cfg = {
        .tx_pulse_count = TX_ACK_PULSE_COUNT,
        .tx_pulse_width = TX_ACK_PULSE_WIDTH,
        .tx_pulse_gain = TX_ACK_PULSE_GAIN,
        .rx_pulse_count = RX_DATA_PULSE_COUNT,
    };
    for (uint8_t i = 0; i < ARRAY_SIZE(channel_frequency); i++) {
        rx_channel_cfg.frequency = channel_frequency[i];
        swc_connection_add_channel(rx_conn, rx_channel_cfg, err);
        ASSERT_SWC_STATUS(*err);
    }
    swc_connection_set_rx_success_callback(rx_conn, conn_rx_success_callback, NULL, err);
    ASSERT_SWC_STATUS(*err);

    /* Handle certification mode. */
    swc_set_certification_mode(certification_mode != FACADE_CERTIF_NONE, err);
    ASSERT_SWC_STATUS(*err);

    swc_setup(err);
    ASSERT_SWC_STATUS(*err);
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

    facade_tx_conn_status();
}

/** @brief Callback function when a previously sent frame has not been ACK'd.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 *  @param[in] arg   Additional argument for the callback function.
 */
static void conn_tx_fail_callback(void *conn, void *arg)
{
    (void)conn;
    (void)arg;
}

/** @brief Callback function when a frame has been successfully received.
 *
 *  @param[in] conn  Connection the callback function has been linked to.
 *  @param[in] arg   Additional argument passed to the callback function.
 */
static void conn_rx_success_callback(void *conn, void *arg)
{
    (void)conn;
    (void)arg;

    swc_error_t err = SWC_ERR_NONE;
    uint8_t *payload = NULL;

    /* Get new payload */
    swc_connection_receive(rx_conn, &payload, &err);
    ASSERT_SWC_STATUS(err);

    memcpy(rx_payload, payload, sizeof(rx_payload));

    /* Free the payload memory */
    swc_connection_receive_complete(rx_conn, &err);
    ASSERT_SWC_STATUS(err);

    facade_rx_conn_status();
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

/** @brief Print the TX and RX statistics.
 */
static void print_stats(void)
{
    if (device_pairing_state != DEVICE_PAIRED) {
        return;
    }

    static char stats_string[STATS_ARRAY_LENGTH];
    int string_length = 0;
    swc_error_t swc_err = SWC_ERR_NONE;

    const char *certification_mode_str = "\r\nCert. Mode %i";

    memset(stats_string, 0, sizeof(stats_string));

    /* Print received string and stats every PRINT_STATS_PERIOD ms. */
    swc_connection_update_stats(tx_conn, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    swc_connection_update_stats(rx_conn, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    if (certification_mode != FACADE_CERTIF_NONE) {
        string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length,
                                  certification_mode_str, certification_mode);
    }

    /* Put rx_payload at the start of the print. */
    string_length = snprintf(stats_string, sizeof(stats_string), "\r\n%s", rx_payload);
    string_length += swc_connection_format_stats(tx_conn, stats_string + string_length,
                                                 sizeof(stats_string) - string_length, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    string_length += swc_connection_format_stats(rx_conn, stats_string + string_length,
                                                 sizeof(stats_string) - string_length, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    facade_print_string(stats_string);

    if (reset_stats_now) {
        swc_connection_reset_stats(tx_conn, &swc_err);
        ASSERT_SWC_STATUS(swc_err);

        swc_connection_reset_stats(rx_conn, &swc_err);
        ASSERT_SWC_STATUS(swc_err);

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

    /* Set the pairing state. */
    device_pairing_state = DEVICE_PAIRING;

    facade_notify_enter_pairing();

    /* The Wireless Core must be stopped before starting the pairing procedure. */
    if (swc_get_status() == SWC_STATUS_RUNNING) {
        swc_disconnect(&swc_err);
        ASSERT_SWC_STATUS(swc_err);
    }

    /* Give the information to the Pairing Module. */
    app_pairing_cfg.app_code = PAIRING_APP_CODE;
    app_pairing_cfg.timeout_sec = PAIRING_TIMEOUT_IN_SECONDS;
    app_pairing_cfg.context_switch_callback = facade_context_switch_trigger;
    app_pairing_cfg.application_callback = pairing_application_callback;
    app_pairing_cfg.memory_pool = swc_memory_pool;
    app_pairing_cfg.memory_pool_size = SWC_MEM_POOL_SIZE;
    pairing_event = pairing_node_start(&app_pairing_cfg, &pairing_assigned_address, PAIRING_DEVICE_ROLE_NODE,
                                       &pairing_err);
    if (pairing_err != PAIRING_ERR_NONE) {
        facade_print_error_string("An error occured during the pairing process.");
        while (1);
    }

    /* Handle the pairing events. */
    switch (pairing_event) {
    case PAIRING_EVENT_SUCCESS:
        facade_notify_pairing_successful();

        app_init();
        device_pairing_state = DEVICE_PAIRED;

        break;
    case PAIRING_EVENT_TIMEOUT:
    case PAIRING_EVENT_INVALID_APP_CODE:
    case PAIRING_EVENT_ABORT:
    default:
        /* Indicate that the pairing process was unsuccessful */
        facade_notify_not_paired();
        device_pairing_state = DEVICE_UNPAIRED;
        break;
    }
}

/** @brief Put the device in the unpaired state and disconnect it from the network.
 */
static void unpair_device(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;

    device_pairing_state = DEVICE_UNPAIRED;

    swc_disconnect(&swc_err);
    ASSERT_SWC_STATUS(swc_err);

    facade_packet_generation_timer_stop();

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

/** @brief Abort the pairing procedure once started.
 */
static void abort_pairing_procedure(void)
{
    pairing_abort();
}

/** @brief Packet generation interrupt handler.
 */
static void packet_generation_timer_interrupt_handler(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;
    uint8_t *hello_world_buf = NULL;

    swc_connection_get_payload_buffer(tx_conn, &hello_world_buf, &swc_err);

    if (hello_world_buf != NULL) {
        size_t tx_payload_size = snprintf((char *)hello_world_buf, MAX_PAYLOAD_SIZE_BYTE, "Hello, World! %lu\n\r",
                                          str_counter++);

        swc_connection_send(tx_conn, hello_world_buf, tx_payload_size + ENDING_NULL_CHARACTER_SIZE, &swc_err);
        ASSERT_SWC_STATUS(swc_err);
    }
}

void swc_error_handler(swc_error_t swc_status)
{
    char buffer[ERROR_MESSAGE_BUFFER_SIZE];

    sprintf(buffer, "SWC Error ! Code: %d\n\r", swc_status);
    facade_print_error_string(buffer);

    while (1);
}
