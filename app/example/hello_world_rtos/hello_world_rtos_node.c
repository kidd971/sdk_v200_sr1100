/** @file  hello_world_rtos_node.c
 *  @brief This is a basic example of how to use the SPARK Wireless Core
 *         In conjunction with a RTOS.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "hello_world_rtos_facade.h"
#include "pairing_api.h"
#include "pairing_cfg.h"
#include "swc_api.h"
#include "swc_cfg.h"
#include "swc_cfg_node.h"
#include "swc_stats.h"
#include "swc_utils.h"
#include "task.h"

/* CONSTANTS ******************************************************************/
#define SWC_MEM_POOL_SIZE                6100
#define MAX_PAYLOAD_SIZE_BYTE            30
#define ENDING_NULL_CHARACTER_SIZE       1
#define STATS_ARRAY_LENGTH               1000
#define SEMAPHORE_SWC_PROCESS_COUNT      1
#define SEMAPHORE_SWC_PROCESS_INIT_COUNT 0
#define HELLO_WORLD_SEND_PERIOD_MS       1
#define PRINT_STATS_PERIOD_MS            1000
#define BUTTON_POLLING_PERIOD_MS         200
/* Thread stack size. */
#define UI_THREAD_STACK_SIZE          2048
#define PAIRING_THREAD_STACK_SIZE     2048
#define SWC_THREAD_STACK_SIZE         2048
#define DATA_GEN_THREAD_STACK_SIZE    2048
#define PRINT_STATS_THREAD_STACK_SIZE 2048
/* Thread flags. */
enum thread_flags {
    NULL_THREAD_FLAG,
    DATA_GENERATION_THREAD_FLAG,
    BUTTON_EVENT_THREAD_FLAG,
    PAIRING_THREAD_FLAG,
    PRINT_STATS_THREAD_FLAG,
};
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

/* PRIVATE GLOBALS ************************************************************/
/* ** Wireless Core ** */
static uint8_t swc_memory_pool[SWC_MEM_POOL_SIZE];
static swc_connection_t *rx_conn;
static swc_connection_t *tx_conn;

static const uint32_t timeslot_us[] = SCHEDULE;
static const uint32_t channel_sequence[] = CHANNEL_SEQUENCE;
static const uint32_t channel_frequency[] = CHANNEL_FREQ;
static const int32_t tx_timeslots[] = NODE_TIMESLOTS;
static const int32_t rx_timeslots[] = COORD_TIMESLOTS;

/* ** Application Specific ** */
static uint32_t str_counter;
static char rx_payload[MAX_PAYLOAD_SIZE_BYTE];
static bool reset_stats_now;
static facade_certification_mode_t certification_mode;
/* Variables supporting pairing between the two devices. */
static volatile device_pairing_state_t device_pairing_state;
static pairing_cfg_t app_pairing_cfg;
static pairing_assigned_address_t pairing_assigned_address;

/* ** RTOS Specific ** */
osThreadId_t user_input_thread_id;
const osThreadAttr_t user_input_thread_attributes = {
    .name = "UI Thread",
    .priority = osPriorityNormal,
    .stack_size = UI_THREAD_STACK_SIZE,
};

osThreadId_t pairing_thread_id;
const osThreadAttr_t pairing_thread_attributes = {
    .name = "Pairing Thread",
    .priority = osPriorityLow,
    .stack_size = PAIRING_THREAD_STACK_SIZE,
};

osThreadId_t swc_callback_thread_id;
const osThreadAttr_t swc_callback_thread_attr = {
    .name = "SWC Callback Thread",
    .priority = osPriorityNormal1,
    .stack_size = SWC_THREAD_STACK_SIZE,
};

osThreadId_t data_gen_thread_id;
const osThreadAttr_t data_generation_thread_attr = {
    .name = "Data Generation Thread",
    .priority = osPriorityHigh,
    .stack_size = DATA_GEN_THREAD_STACK_SIZE,
};

osThreadId_t print_stats_thread_id;
const osThreadAttr_t print_stats_thread_attr = {
    .name = "Print Stats Thread",
    .priority = osPriorityLow,
    .stack_size = PRINT_STATS_THREAD_STACK_SIZE,
};

osSemaphoreId_t swc_process_sem;
static const osSemaphoreAttr_t swc_process_semAttr = {
    .name = "SWC process Sempahore",
};

/* PRIVATE FUNCTION PROTOTYPE *************************************************/
static void app_swc_core_init(pairing_assigned_address_t *app_pairing, swc_error_t *swc_err);
static void user_input_thread(void *argument);
static void swc_callback_thread(void *argument);
static void data_generation_thread(void *argument);
static void print_stats_thread(void *argument);
static void pairing_thread(void *argument);
static void callback_context_trigger(void);
static void conn_tx_success_callback(void *conn, void *arg);
static void conn_tx_fail_callback(void *conn, void *arg);
static void conn_rx_success_callback(void *conn, void *arg);
static void reset_stats(void);
static void request_pairing_mode(void);
static void unpair_device(void);
static void abort_pairing_procedure(void);
static void button_event_callback(void);
static void pairing_button_callback(void);
static void app_start(void);
static void app_stop(void);

/* PUBLIC FUNCTIONS ***********************************************************/
int main(void)
{
    str_counter = 0;

    facade_board_init();

    facade_button_init();

    facade_button_callbacks_t button_callbacks = {
        .pairing_callback = pairing_button_callback,
        .reset_stats_callback = reset_stats,
    };
    facade_set_button_callbacks(button_callbacks);

    osKernelInitialize();

    swc_process_sem = osSemaphoreNew(SEMAPHORE_SWC_PROCESS_COUNT, SEMAPHORE_SWC_PROCESS_INIT_COUNT,
                                     &swc_process_semAttr);
    if (swc_process_sem == NULL) {
        while (1);
    }

    user_input_thread_id = osThreadNew(user_input_thread, NULL, &user_input_thread_attributes);
    if (user_input_thread_id == NULL) {
        while (1);
    }

    facade_set_button_event_callback(button_event_callback);

    pairing_thread_id = osThreadNew(pairing_thread, NULL, &pairing_thread_attributes);
    if (pairing_thread_id == NULL) {
        while (1);
    }

    swc_callback_thread_id = osThreadNew(swc_callback_thread, NULL, &swc_callback_thread_attr);
    if (swc_callback_thread_id == NULL) {
        while (1);
    }

    data_gen_thread_id = osThreadNew(data_generation_thread, NULL, &data_generation_thread_attr);
    if (data_gen_thread_id == NULL) {
        while (1);
    }

    print_stats_thread_id = osThreadNew(print_stats_thread, NULL, &print_stats_thread_attr);
    if (print_stats_thread_id == NULL) {
        while (1);
    }

    osKernelStart();

    while (1);
}

void swc_error_handler(swc_error_t swc_status)
{
    char buffer[ERROR_MESSAGE_BUFFER_SIZE];

    snprintf(buffer, sizeof(buffer), "SWC Error ! Code: %d\n\r", swc_status);
    facade_print_error_string(buffer);

    while (1);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize the Wireless Core.
 *
 *  @param[in]  app_pairing  Configure the Wireless Core with the pairing values.
 *  @param[out] swc_err      Wireless Core error code.
 */
static void app_swc_core_init(pairing_assigned_address_t *app_pairing, swc_error_t *swc_err)
{
    uint16_t local_address = app_pairing->node_address;
    uint16_t remote_address = app_pairing->coordinator_address;
    swc_radio_handle_t *radio_handle = NULL;
#if (SWC_RADIO_COUNT == 2)
    swc_radio_handle_t *radio_handle_2 = NULL;
#endif

    /* In cert mode, pairing has not run -- override addresses directly. */
    if (certification_mode != FACADE_CERTIF_NONE) {
        app_pairing->pan_id = 0xABC;
        remote_address = 0x1;
        local_address = 0x2;
    }

    const swc_cfg_t core_cfg = {
        .timeslot_sequence = timeslot_us,
        .timeslot_sequence_length = ARRAY_SIZE(timeslot_us),
        .channel_sequence = channel_sequence,
        .channel_sequence_length = ARRAY_SIZE(channel_sequence),
        .concurrency_mode = SWC_CONCURRENCY_MODE_HIGH_PERFORMANCE,
        .memory_pool = swc_memory_pool,
        .memory_pool_size = SWC_MEM_POOL_SIZE,
        .pan_id = app_pairing->pan_id,
    };

    const swc_node_cfg_t node_cfg = {
        .role = SWC_ROLE_NODE,
        .coordinator_address = remote_address,
        .local_address = local_address,
    };

    swc_init(core_cfg, node_cfg, callback_context_trigger, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Calibrate the radio. */
    radio_handle = swc_radio_module_calib(SWC_RADIO_ID_1, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Initialize the radio. */
    swc_radio_module_init(radio_handle, false, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

#if (SWC_RADIO_COUNT == 2)
    /* Calibrate the radio. */
    radio_handle_2 = swc_radio_module_calib(SWC_RADIO_ID_2, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Initialize the radio. */
    swc_radio_module_init(radio_handle_2, false, swc_err);
    ASSERT_SWC_STATUS(*swc_err);
#endif

    /* ** TX Connection ** */
    const swc_connection_cfg_t tx_conn_cfg = {
        .name = "TX Connection",
        .source_address = local_address,
        .destination_address = remote_address,
        .max_payload_size = MAX_PAYLOAD_SIZE_BYTE + ENDING_NULL_CHARACTER_SIZE,
        .queue_size = SWC_QUEUE_SIZE,
        .timeslot_id = tx_timeslots,
        .timeslot_count = ARRAY_SIZE(tx_timeslots),
    };
    tx_conn = swc_connection_init(tx_conn_cfg, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    const swc_channel_cfg_t tx_channel_cfg = {
        .tx_pulse_count = TX_DATA_PULSE_COUNT,
        .tx_pulse_width = TX_DATA_PULSE_WIDTH,
        .tx_pulse_gain = TX_DATA_PULSE_GAIN,
        .rx_pulse_count = RX_ACK_PULSE_COUNT,
    };
    swc_channel_t *tx_channels = swc_channel_list_init_from_base(tx_channel_cfg, channel_frequency,
                                                                 ARRAY_SIZE(channel_frequency), swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    swc_connection_set_channels(tx_conn, tx_channels, ARRAY_SIZE(channel_frequency), swc_err);
    ASSERT_SWC_STATUS(*swc_err);
    swc_connection_set_tx_success_callback(tx_conn, conn_tx_success_callback, NULL, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    swc_connection_set_tx_fail_callback(tx_conn, conn_tx_fail_callback, NULL, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* ** RX Connection ** */
    const swc_connection_cfg_t rx_conn_cfg = {
        .name = "RX Connection",
        .source_address = remote_address,
        .destination_address = local_address,
        .max_payload_size = MAX_PAYLOAD_SIZE_BYTE + ENDING_NULL_CHARACTER_SIZE,
        .queue_size = SWC_QUEUE_SIZE,
        .timeslot_id = rx_timeslots,
        .timeslot_count = ARRAY_SIZE(rx_timeslots),
    };
    rx_conn = swc_connection_init(rx_conn_cfg, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    const swc_channel_cfg_t rx_channel_cfg = {
        .tx_pulse_count = TX_ACK_PULSE_COUNT,
        .tx_pulse_width = TX_ACK_PULSE_WIDTH,
        .tx_pulse_gain = TX_ACK_PULSE_GAIN,
        .rx_pulse_count = RX_DATA_PULSE_COUNT,
    };
    swc_channel_t *rx_channels = swc_channel_list_init_from_base(rx_channel_cfg, channel_frequency,
                                                                 ARRAY_SIZE(channel_frequency), swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    swc_connection_set_channels(rx_conn, rx_channels, ARRAY_SIZE(channel_frequency), swc_err);
    ASSERT_SWC_STATUS(*swc_err);
    swc_connection_set_rx_success_callback(rx_conn, conn_rx_success_callback, NULL, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    /* Handle certification mode. */
    swc_set_certification_mode(certification_mode != FACADE_CERTIF_NONE, swc_err);
    ASSERT_SWC_STATUS(*swc_err);

    swc_setup(swc_err);
    ASSERT_SWC_STATUS(*swc_err);
}

/** @brief The user input thread.
 *
 *  @param[in] argument  Unused.
 */
static void user_input_thread(void *argument)
{
    (void)argument;

    device_pairing_state = DEVICE_UNPAIRED;

    certification_mode = facade_get_certification_mode();
    if (certification_mode != FACADE_CERTIF_NONE) {
        /* Init app in certification mode. */
        facade_notify_certification_mode();
        device_pairing_state = DEVICE_PAIRED;
        app_start();
    } else {
        /* Pairing occurs automatically when the device boots. */
        request_pairing_mode();
    }

    while (1) {
        osThreadFlagsWait(BUTTON_EVENT_THREAD_FLAG, osFlagsWaitAny, BUTTON_POLLING_PERIOD_MS);
        facade_button_handling();
    }
}

/** @brief Listen for incoming radio interrupts and call swc_process.
 *
 *  @param[in] argument  Unused.
 */
static void swc_callback_thread(void *argument)
{
    (void)argument;

    while (1) {
        if (osSemaphoreAcquire(swc_process_sem, osWaitForever) == osOK) {
            swc_connection_callbacks_processing_handler();
        }
    }
}

/** @brief Thread for periodic data generation.
 *
 *  @param[in] argument  Unused.
 */
static void data_generation_thread(void *argument)
{
    (void)argument;

    uint32_t send_period_ticks = (HELLO_WORLD_SEND_PERIOD_MS * osKernelGetTickFreq()) / 1000U;

    if (send_period_ticks == 0U) {
        send_period_ticks = 1U;
    }

    while (1) {
        /* Wait until the Wireless Core application is started. */
        osThreadFlagsWait(DATA_GENERATION_THREAD_FLAG, osFlagsWaitAny, osWaitForever);

        uint32_t next_wakeup_tick = osKernelGetTickCount();

        while (device_pairing_state == DEVICE_PAIRED) {
            next_wakeup_tick += send_period_ticks;
            if (osDelayUntil(next_wakeup_tick) != osOK) {
                next_wakeup_tick = osKernelGetTickCount();
            }

            if (device_pairing_state != DEVICE_PAIRED) {
                break;
            }

            swc_error_t swc_err = SWC_ERR_NONE;
            uint8_t *hello_world_buf = NULL;

            swc_connection_get_payload_buffer(tx_conn, &hello_world_buf, &swc_err);
            if (hello_world_buf != NULL) {
                size_t tx_payload_size = snprintf((char *)hello_world_buf, MAX_PAYLOAD_SIZE_BYTE,
                                                  "Hello, World! %" PRIu32 "\n\r", str_counter++);

                swc_connection_send(tx_conn, hello_world_buf, tx_payload_size + ENDING_NULL_CHARACTER_SIZE, &swc_err);
                ASSERT_SWC_STATUS(swc_err);
            }
        }
    }
}

/** @brief Thread to enter pairing mode using the pairing module.
 *
 *  @param[in] argument  Unused.
 */
static void pairing_thread(void *argument)
{
    (void)argument;

    swc_error_t swc_err = SWC_ERR_NONE;
    pairing_error_t pairing_err = PAIRING_ERR_NONE;

    pairing_event_t pairing_event;

    while (1) {
        /* Wait for pairing request. */
        osThreadFlagsWait(PAIRING_THREAD_FLAG, osFlagsWaitAny, osWaitForever);

        /* Set the application's state. */
        device_pairing_state = DEVICE_PAIRING;

        facade_notify_enter_pairing();

        /* The wireless core must be stopped before starting the pairing procedure. */
        if (swc_get_status() == SWC_STATUS_RUNNING) {
            swc_disconnect(&swc_err);
            ASSERT_SWC_STATUS(swc_err);
        }

        /* Give the information to the Pairing Application. */
        app_pairing_cfg.app_code = PAIRING_APP_CODE;
        app_pairing_cfg.timeout_sec = PAIRING_TIMEOUT_IN_SECONDS;
        app_pairing_cfg.context_switch_callback = callback_context_trigger;
        app_pairing_cfg.application_callback = NULL;
        app_pairing_cfg.memory_pool = swc_memory_pool;
        app_pairing_cfg.memory_pool_size = SWC_MEM_POOL_SIZE;
        pairing_event = pairing_node_start(&app_pairing_cfg, &pairing_assigned_address, PAIRING_DEVICE_ROLE_NODE,
                                           &pairing_err);
        if (pairing_err != PAIRING_ERR_NONE) {
            while (1);
        }

        /* Handle the pairing events. */
        switch (pairing_event) {
        case PAIRING_EVENT_SUCCESS:
            /* Indicate that the pairing process was successful. */
            facade_notify_pairing_successful();
            /* Set the application's state. */
            device_pairing_state = DEVICE_PAIRED;
            /* Run the application. */
            app_start();
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
}

/** @brief Thread responsible for formatting and printing the Wireless Core statistics.
 *
 *  @param[in] argument  Unused.
 */
static void print_stats_thread(void *argument)
{
    (void)argument;

    uint32_t print_period_ticks = (PRINT_STATS_PERIOD_MS * osKernelGetTickFreq()) / 1000U;

    if (print_period_ticks == 0U) {
        print_period_ticks = 1U;
    }

    while (1) {
        /* Wait until the Wireless Core application is started. */
        osThreadFlagsWait(PRINT_STATS_THREAD_FLAG, osFlagsWaitAny, osWaitForever);

        uint32_t next_wakeup_tick = osKernelGetTickCount();

        while (device_pairing_state == DEVICE_PAIRED) {
            next_wakeup_tick += print_period_ticks;
            if (osDelayUntil(next_wakeup_tick) != osOK) {
                next_wakeup_tick = osKernelGetTickCount();
            }

            if (device_pairing_state != DEVICE_PAIRED) {
                break;
            }

            static char stats_string[STATS_ARRAY_LENGTH];
            int string_length = 0;
            swc_error_t swc_err = SWC_ERR_NONE;

            const char *certification_mode_str = "\r\nCert. Mode %i";

            memset(stats_string, 0, sizeof(stats_string));

            /* Print received string and stats every PRINT_STATS_PERIOD ms. */
            swc_connection_t *connections[] = {tx_conn, rx_conn};

            for (uint8_t i = 0; i < ARRAY_SIZE(connections); i++) {
                swc_connection_update_stats(connections[i], &swc_err);
                ASSERT_SWC_STATUS(swc_err);
            }

            if (certification_mode != FACADE_CERTIF_NONE) {
                string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length,
                                          certification_mode_str, certification_mode);
            }

            /* Put rx_payload at the start of the print. */
            string_length += snprintf(stats_string + string_length, sizeof(stats_string) - string_length, "\r\n%s",
                                      rx_payload);
            for (uint8_t i = 0; i < ARRAY_SIZE(connections); i++) {
                string_length += swc_connection_format_stats(connections[i], stats_string + string_length,
                                                             sizeof(stats_string) - string_length, &swc_err);
                ASSERT_SWC_STATUS(swc_err);
            }

            facade_print_string(stats_string);

            if (reset_stats_now) {
                swc_connection_reset_stats(tx_conn, &swc_err);
                ASSERT_SWC_STATUS(swc_err);

                swc_connection_reset_stats(rx_conn, &swc_err);
                ASSERT_SWC_STATUS(swc_err);

                reset_stats_now = false;
            }
        }
    }
}

/** @brief Callback context switch implementation for the SWC interface.
 */
static void callback_context_trigger(void)
{
    osSemaphoreRelease(swc_process_sem);
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
 *  @param[in] arg   Additional argument for the callback function.
 */
static void conn_rx_success_callback(void *conn, void *arg)
{
    (void)conn;
    (void)arg;

    swc_error_t swc_err = SWC_ERR_NONE;
    uint8_t *payload = NULL;

    /* Get new payload. */
    swc_connection_receive(rx_conn, &payload, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    memcpy(rx_payload, payload, sizeof(rx_payload));

    /* Free the payload memory. */
    swc_connection_receive_complete(rx_conn, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    facade_rx_conn_status();
}

/** @brief Reset the TX and RX statistics.
 */
static void reset_stats(void)
{
    if (device_pairing_state != DEVICE_PAIRED) {
        return;
    }

    reset_stats_now = true;
}

/** @brief Request to enter pairing mode.
 */
static void request_pairing_mode(void)
{
    osThreadFlagsSet(pairing_thread_id, PAIRING_THREAD_FLAG);
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
        request_pairing_mode();
        break;
    default:
        break;
    }
}

/** @brief Start the application.
 */
static void app_start(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;

    /* Configure the wireless core. */
    app_swc_core_init(&pairing_assigned_address, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    /* Connect the wireless core. */
    swc_connect(&swc_err);
    ASSERT_SWC_STATUS(swc_err);

    /* Start the periodic application threads. */
    osThreadFlagsSet(data_gen_thread_id, DATA_GENERATION_THREAD_FLAG);
    osThreadFlagsSet(print_stats_thread_id, PRINT_STATS_THREAD_FLAG);
}

/** @brief Stop the application.
 */
static void app_stop(void)
{
    swc_error_t swc_err = SWC_ERR_NONE;

    /* Disconnect the Wireless Core. */
    swc_disconnect(&swc_err);
    ASSERT_SWC_STATUS(swc_err);
}

/** @brief Unpair the device, this will stop communication.
 */
static void unpair_device(void)
{
    device_pairing_state = DEVICE_UNPAIRED;

    /* Stop the application. */
    app_stop();

    /* Indicate that the device is unpaired and turn off all LEDs. */
    facade_notify_not_paired();
}

/** @brief Abort the pairing procedure.
 */
static void abort_pairing_procedure(void)
{
    pairing_abort();
}

/** @brief Callback function triggered on button state change event.
 */
static void button_event_callback(void)
{
    /* Set the button event thread flag. */
    osThreadFlagsSet(user_input_thread_id, BUTTON_EVENT_THREAD_FLAG);
}
