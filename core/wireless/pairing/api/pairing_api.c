/** @file  pairing_api.c
 *  @brief The pairing module is used to exchange network information between two unconnected devices and establish network parameters for further exchanges.
 *
 *  @note Pairing only supports little endian.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "pairing_api.h"
#include "pairing_address.h"
#include "pairing_error.h"
#include "pairing_event.h"
#include "pairing_security.h"
#include "pairing_state.h"
#include "pairing_timer.h"
#include "pairing_wireless.h"

/* CONSTANTS ******************************************************************/
/*! Minimum timeout duration in seconds. */
#define PAIRING_MINIMUM_TIMEOUT_SEC 5
/*!
 * Delay in millisecond applied before disconnecting the wireless core to ensure
 * all packets have been acked before exiting.
 */
#define PAIRING_EXIT_DELAY_MS            100

/* MACROS *********************************************************************/
#define CHECK_ERROR(cond, err_code, ret)       \
    do {                                       \
        if (cond) {                            \
            pairing_error_set_error(err_code); \
            ret;                               \
        }                                      \
    } while (0)

/* PRIVATE GLOBALS ************************************************************/
static void (*application_callback)(void);

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void initialize_pairing_module(pairing_cfg_t *pairing_cfg, pairing_assigned_address_t *pairing_assigned_address);
static void pairing_process(void);
static void pairing_deinit(void);

/* PUBLIC FUNCTIONS ***********************************************************/
pairing_event_t pairing_coordinator_start(pairing_cfg_t *pairing_cfg, pairing_assigned_address_t *pairing_assigned_address,
                                          pairing_discovery_list_t *discovery_list, uint8_t discovery_list_size,
                                          pairing_error_t *pairing_err)
{
    swc_status_t swc_status;

    *pairing_err = PAIRING_ERR_NONE;
    pairing_error_init(pairing_err);

    /* Avoid changing wireless configuration while the wireless core is running. */
    swc_status = pairing_wireless_get_status();
    CHECK_ERROR(swc_status == SWC_STATUS_RUNNING, PAIRING_ERR_CHANGING_WIRELESS_CONFIG_WHILE_RUNNING, return PAIRING_EVENT_NONE);

    /* Checking for parameter errors. */
    CHECK_ERROR(pairing_cfg == NULL, PAIRING_ERR_NULL_PTR, return PAIRING_EVENT_NONE);
    CHECK_ERROR(pairing_assigned_address == NULL, PAIRING_ERR_NULL_PTR, return PAIRING_EVENT_NONE);
    CHECK_ERROR(discovery_list == NULL, PAIRING_ERR_NULL_PTR, return PAIRING_EVENT_NONE);
    CHECK_ERROR(discovery_list_size < PAIRING_DISCOVERY_LIST_MINIMUM_SIZE,
                PAIRING_ERR_DISCOVERY_LIST_SIZE_TOO_SMALL, return PAIRING_EVENT_NONE);
    CHECK_ERROR(pairing_cfg->app_code == 0, PAIRING_ERR_APP_CODE_NOT_CONFIGURED, return PAIRING_EVENT_NONE);
    CHECK_ERROR(pairing_cfg->timeout_sec < PAIRING_MINIMUM_TIMEOUT_SEC,
                PAIRING_ERR_TIMEOUT, return PAIRING_EVENT_NONE);
    CHECK_ERROR(pairing_cfg->uwb_regulation >= _SWC_REGULATION_COUNT,
                PAIRING_ERR_REGULATION_OPTION_NOT_SUPPORTED, return PAIRING_EVENT_NONE);
    CHECK_ERROR(pairing_cfg->context_switch_callback == NULL, PAIRING_ERR_NULL_PTR, return PAIRING_EVENT_NONE);

    /* Initialize the pairing module. */
    initialize_pairing_module(pairing_cfg, pairing_assigned_address);

    /* Coordinator is always the device role 0. */
    pairing_address_set_device_role(PAIRING_DEVICE_ROLE_COORDINATOR);

    /* Initialize the discovery list. */
    pairing_address_discovery_list_init(discovery_list, discovery_list_size);

    /* Reconfigure the Wireless Core for the pairing module. */
    pairing_wireless_init(pairing_cfg, SWC_ROLE_COORDINATOR);
    if (pairing_error_get_error() != PAIRING_ERR_NONE) {
        pairing_deinit();
        return pairing_event_get_event();
    }

    /* Initialize the state machine. */
    pairing_state_init(SWC_ROLE_COORDINATOR);

    /* Start the pairing process loop. */
    pairing_process();

    /* Deinitialize the pairing module before returning to the application. */
    pairing_deinit();

    return pairing_event_get_event();
}

pairing_event_t pairing_node_start(pairing_cfg_t *pairing_cfg, pairing_assigned_address_t *pairing_assigned_address,
                                   uint8_t device_role, pairing_error_t *pairing_err)
{
    swc_status_t swc_status;

    *pairing_err = PAIRING_ERR_NONE;
    pairing_error_init(pairing_err);

    /* Avoid changing wireless configuration while the wireless core is running. */
    swc_status = pairing_wireless_get_status();
    CHECK_ERROR(swc_status == SWC_STATUS_RUNNING, PAIRING_ERR_CHANGING_WIRELESS_CONFIG_WHILE_RUNNING, return PAIRING_EVENT_NONE);

    /* Checking for parameter errors. */
    CHECK_ERROR(pairing_cfg == NULL, PAIRING_ERR_NULL_PTR, return PAIRING_EVENT_NONE);
    CHECK_ERROR(pairing_assigned_address == NULL, PAIRING_ERR_NULL_PTR, return PAIRING_EVENT_NONE);
    CHECK_ERROR(pairing_cfg->app_code == 0, PAIRING_ERR_APP_CODE_NOT_CONFIGURED, return PAIRING_EVENT_NONE);
    CHECK_ERROR(pairing_cfg->timeout_sec < PAIRING_MINIMUM_TIMEOUT_SEC,
                PAIRING_ERR_TIMEOUT, return PAIRING_EVENT_NONE);
    CHECK_ERROR(pairing_cfg->uwb_regulation >= _SWC_REGULATION_COUNT,
                PAIRING_ERR_REGULATION_OPTION_NOT_SUPPORTED, return PAIRING_EVENT_NONE);
    CHECK_ERROR(device_role == PAIRING_DEVICE_ROLE_COORDINATOR, PAIRING_ERR_DEVICE_ROLE, return PAIRING_EVENT_NONE);
    CHECK_ERROR(pairing_cfg->context_switch_callback == NULL, PAIRING_ERR_NULL_PTR, return PAIRING_EVENT_NONE);

    /* Initialize the pairing module. */
    initialize_pairing_module(pairing_cfg, pairing_assigned_address);

    /* Assign the device pairing role for the discovery list. */
    pairing_address_set_device_role(device_role);

    /* Reconfigure the Wireless Core for the pairing module. */
    pairing_wireless_init(pairing_cfg, SWC_ROLE_NODE);
    if (pairing_error_get_error() != PAIRING_ERR_NONE) {
        pairing_deinit();
        return pairing_event_get_event();
    }

    /* Initialize the state machine with the device's role. */
    pairing_state_init(SWC_ROLE_NODE);

    /* Start the pairing process loop. */
    pairing_process();

    /* Deinitialize the pairing module before returning to the application. */
    pairing_deinit();

    return pairing_event_get_event();
}

void pairing_abort(void)
{
    pairing_event_set_event(PAIRING_EVENT_ABORT);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize the pairing module.
 */
static void initialize_pairing_module(pairing_cfg_t *pairing_cfg, pairing_assigned_address_t *pairing_assigned_address)
{
    /* Enable pairing specific feature in the SWC API. */
    swc_reserved_address_unlock();

    /* Get the pairing address handle from the application and create a local pairing instance. */
    pairing_address_init(pairing_assigned_address);

    /* Initialize security related features. */
    pairing_security_init();
    pairing_security_set_app_code(pairing_cfg->app_code);

    /* Initialize the pairing events. */
    pairing_event_init();

    /* Initialize the timer manager module. */
    pairing_timer_init(pairing_cfg->timeout_sec);

    /* Application level callback to perform application tasks. */
    if (pairing_cfg->application_callback != NULL) {
        application_callback = pairing_cfg->application_callback;
    }
}

/** @brief Main pairing process loop.
 */
static void pairing_process(void)
{
    do {
        /* Execute the function associated with the current state. */
        pairing_state_execute_current_state();

        /* Verify if the application defined timeout is reached. */
        if (pairing_timer_is_timeout()) {
            pairing_event_set_event(PAIRING_EVENT_TIMEOUT);
        }

        /* Application level callback to perform application tasks. */
        if (application_callback != NULL) {
            application_callback();
        }

    } while ((pairing_event_get_event() == PAIRING_EVENT_NONE) && (pairing_error_get_error() == PAIRING_ERR_NONE));
}

/** @brief Deinitialize the pairing process and its Wireless Core instance.
 */
static void pairing_deinit(void)
{
    /* Delay wireless deinit to allow radio to ACK remaining packets. */
    pairing_timer_blocking_delay_ms(PAIRING_EXIT_DELAY_MS);

    /* Free the memory before returning to the application. */
    pairing_wireless_disconnect();
    pairing_wireless_free_memory();

    swc_reserved_address_lock();
}
