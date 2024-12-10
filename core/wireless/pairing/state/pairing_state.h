/** @file  pairing_state.h
 *  @brief This file handles the functions related to the pairing states.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef PAIRING_STATE_H_
#define PAIRING_STATE_H_

/* INCLUDES *******************************************************************/
#include "swc_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Pairing states available for both the coordinator and node devices.
 */
typedef enum pairing_state {
    /*! The entry state for the pairing states. */
    PAIRING_STATE_ENTER,
    /*! This state is used to disconnect and free wireless memory before returning to the application. */
    PAIRING_STATE_EXIT,
    /*! This state is used by the coordinator to send the authentication message to the node. */
    PAIRING_STATE_AUTHENTICATION_SEND_MESSAGE,
    /*! This state is used by the coordinator to wait for the node's authentication response message. */
    PAIRING_STATE_AUTHENTICATION_WAIT_FOR_RESPONSE,
    /*! This state is used by the node to wait for the coordinator's authentication message. */
    PAIRING_STATE_AUTHENTICATION_WAIT_FOR_MESSAGE,
    /*! This state is used by the node to send the authentication response message to the coordinator. */
    PAIRING_STATE_AUTHENTICATION_SEND_RESPONSE,
    /*! This state is used to wait for acknowledgment after sending a message. */
    PAIRING_STATE_AUTHENTICATION_WAIT_FOR_ACK,
    /*! This state is used to take an action depending on various conditions. */
    PAIRING_STATE_AUTHENTICATION_ACTION,
    /*! This state is used by the node to send the identification message to the coordinator. */
    PAIRING_STATE_IDENTIFICATION_SEND_MESSAGE,
    /*! This state is used by the node to wait for the coordinator's identification response message. */
    PAIRING_STATE_IDENTIFICATION_WAIT_FOR_RESPONSE,
    /*! This state is used by the coordinator to wait for the node's identification message. */
    PAIRING_STATE_IDENTIFICATION_WAIT_FOR_MESSAGE,
    /*! This state is used by the coordinator to send the identification response message to the node. */
    PAIRING_STATE_IDENTIFICATION_SEND_RESPONSE,
    /*! This state is used to wait for acknowledgment after sending a message. */
    PAIRING_STATE_IDENTIFICATION_WAIT_FOR_ACK,
    /*! This state is used to take an action depending on various conditions. */
    PAIRING_STATE_IDENTIFICATION_ACTION,
    /*! This state is used by the coordinator to send the addressing message to the node. */
    PAIRING_STATE_ADDRESSING_SEND_MESSAGE,
    /*! This state is used by the coordinator to wait for the node's addressing response message. */
    PAIRING_STATE_ADDRESSING_WAIT_FOR_RESPONSE,
    /*! This state is used by the node to wait for the coordinator's addressing message. */
    PAIRING_STATE_ADDRESSING_WAIT_FOR_MESSAGE,
    /*! This state is used by the node to send the addressing response message to the coordinator. */
    PAIRING_STATE_ADDRESSING_SEND_RESPONSE,
    /*! This state is used to wait for acknowledgment after sending a message. */
    PAIRING_STATE_ADDRESSING_WAIT_FOR_ACK,
    /*! This state is used to take an action depending on various conditions. */
    PAIRING_STATE_ADDRESSING_ACTION,
} pairing_state_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the pairing states of the coordinator device.
 *
 *  @param[in] swc_role  The pairing device's network role.
 */
void pairing_state_init(swc_role_t swc_role);

/** @brief Execute the function that is associated with the current state.
 */
void pairing_state_execute_current_state(void);

/** @brief Apply the next state to be executed.
 *
 *  @param[in] pairing_state  The new effective pairing state.
 */
void pairing_state_set_current_state(pairing_state_t pairing_state);

/** @brief Get the current pairing state.
 *
 *  @return Current pairing state.
 */
pairing_state_t pairing_state_get_current_state(void);

#ifdef __cplusplus
}
#endif

#endif /* PAIRING_STATE_H_ */
