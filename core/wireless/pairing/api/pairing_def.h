/** @file  pairing_def.h
 *  @brief Pairing global definitions.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef PAIRING_DEF_H_
#define PAIRING_DEF_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/*! The coordinator always has the role 0 in the pairing discovery list. */
#define PAIRING_DEVICE_ROLE_COORDINATOR 0
/*! A minimum of two devices must be present in a network, including the coordinator. */
#define PAIRING_DISCOVERY_LIST_MINIMUM_SIZE 2

/* TYPES **********************************************************************/
/** @brief Pairing addresses discoverable during the pairing procedure.
 */
typedef struct pairing_assigned_address {
    /*! Coordinator's PAN ID. */
    uint16_t pan_id;
    /*! Coordinator's address. */
    uint8_t coordinator_address;
    /*! Node's assigned address. */
    uint8_t node_address;
} pairing_assigned_address_t;

/** @brief Pairing list of discovered devices used by the coordinator.
 */
typedef struct pairing_discovery_list {
    /*! Generated unique ID. */
    uint64_t unique_id;
    /*! Address of the node. */
    uint8_t node_address;
} pairing_discovery_list_t;

/** @brief Pairing event when exiting pairing procedure.
 */
typedef enum pairing_event {
    /*! No event occurred. */
    PAIRING_EVENT_NONE = 0,
    /*! The pairing procedure is successful. */
    PAIRING_EVENT_SUCCESS,
    /*! The timeout was reached. */
    PAIRING_EVENT_TIMEOUT,
    /*! The application code is not valid. */
    PAIRING_EVENT_INVALID_APP_CODE,
    /*! The pairing procedure was aborted from an external source. */
    PAIRING_EVENT_ABORT,
} pairing_event_t;

/** @brief Pairing API error structure.
 */
typedef enum pairing_error {
    /*! No error occurred. */
    PAIRING_ERR_NONE = 0,
    /*! A NULL pointer is passed as argument. */
    PAIRING_ERR_NULL_PTR,
    /*! Discovery list size must be 2 or more. */
    PAIRING_ERR_DISCOVERY_LIST_SIZE_TOO_SMALL,
    /*! The application code is not configured. */
    PAIRING_ERR_APP_CODE_NOT_CONFIGURED,
    /*! Timeout is shorter than the minimum timeout duration. */
    PAIRING_ERR_TIMEOUT,
    /*! HAL has not been initialized at the application level. */
    PAIRING_ERR_HAL_NOT_INITIALIZED,
    /*! The wireless regulation chosen is not supported. */
    PAIRING_ERR_REGULATION_OPTION_NOT_SUPPORTED,
    /*! The node's device role conflicts with the coordinator's reserved role. */
    PAIRING_ERR_DEVICE_ROLE,
    /*! A wireless error occurred. */
    PAIRING_ERR_WIRELESS_ERROR,
    /*! Wireless configurations can't be changed while the SWC is running. */
    PAIRING_ERR_CHANGING_WIRELESS_CONFIG_WHILE_RUNNING,
} pairing_error_t;

#ifdef __cplusplus
}
#endif

#endif /* PAIRING_DEF_H_ */
