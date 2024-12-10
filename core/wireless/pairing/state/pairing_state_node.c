/** @file  pairing_state_node.c
 *  @brief This file handles the function related to the node's pairing states.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "pairing_state_node.h"
#include "pairing_address.h"
#include "pairing_error.h"
#include "pairing_event.h"
#include "pairing_message.h"
#include "pairing_security.h"
#include "pairing_state.h"
#include "pairing_state_machine.h"
#include "pairing_wireless.h"

/* MACROS *********************************************************************/
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))

/* PRIVATE GLOBALS ************************************************************/
static pairing_authentication_message_t pairing_authentication_message;
static pairing_authentication_response_t pairing_authentication_response;
static pairing_authentication_action_t pairing_authentication_action;

static pairing_identification_message_t pairing_identification_message;
static pairing_identification_response_t pairing_identification_response;
static pairing_identification_action_t pairing_identification_action;

static pairing_addressing_message_t pairing_addressing_message;
static pairing_addressing_response_t pairing_addressing_response;
static pairing_addressing_action_t pairing_addressing_action;

static uint8_t *received_payload[PAIRING_MAX_PAYLOAD_SIZE];
static pairing_command_t received_pairing_command;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void enter_pairing(void);
static void exit_pairing(void);

static void authentication_wait_for_message(void);
static void authentication_send_response(void);
static void authentication_wait_for_ack(void);
static void authentication_action(void);

static void identification_send_message(void);
static void identification_wait_for_ack(void);
static void identification_wait_for_response(void);
static void identification_action(void);

static void addressing_wait_for_message(void);
static void addressing_send_response(void);
static void addressing_wait_for_ack(void);
static void addressing_action(void);

/* TYPES **********************************************************************/
static pairing_state_machine_t pairing_state_machine[] = {
    {(uint8_t)PAIRING_STATE_ENTER, enter_pairing},
    {(uint8_t)PAIRING_STATE_EXIT, exit_pairing},

    {(uint8_t)PAIRING_STATE_AUTHENTICATION_WAIT_FOR_MESSAGE, authentication_wait_for_message},
    {(uint8_t)PAIRING_STATE_AUTHENTICATION_SEND_RESPONSE, authentication_send_response},
    {(uint8_t)PAIRING_STATE_AUTHENTICATION_WAIT_FOR_ACK, authentication_wait_for_ack},
    {(uint8_t)PAIRING_STATE_AUTHENTICATION_ACTION, authentication_action},

    {(uint8_t)PAIRING_STATE_IDENTIFICATION_SEND_MESSAGE, identification_send_message},
    {(uint8_t)PAIRING_STATE_IDENTIFICATION_WAIT_FOR_ACK, identification_wait_for_ack},
    {(uint8_t)PAIRING_STATE_IDENTIFICATION_WAIT_FOR_RESPONSE, identification_wait_for_response},
    {(uint8_t)PAIRING_STATE_IDENTIFICATION_ACTION, identification_action},

    {(uint8_t)PAIRING_STATE_ADDRESSING_WAIT_FOR_MESSAGE, addressing_wait_for_message},
    {(uint8_t)PAIRING_STATE_ADDRESSING_SEND_RESPONSE, addressing_send_response},
    {(uint8_t)PAIRING_STATE_ADDRESSING_WAIT_FOR_ACK, addressing_wait_for_ack},
    {(uint8_t)PAIRING_STATE_ADDRESSING_ACTION, addressing_action},
};

/* PUBLIC FUNCTIONS ***********************************************************/
void pairing_state_node_init(void)
{
    uint8_t state_machine_size = 0;

    state_machine_size = (ARRAY_SIZE(pairing_state_machine));
    pairing_state_machine_init(pairing_state_machine, state_machine_size);

    pairing_state_set_current_state(PAIRING_STATE_ENTER);
}

void sent_message_node_callback(void)
{
    pairing_state_t current_pairing_state = pairing_state_get_current_state();

    /* Used by the pairing procedure to ensure that the message was sent successfully. */
    if (current_pairing_state == PAIRING_STATE_AUTHENTICATION_WAIT_FOR_ACK) {
        pairing_state_set_current_state(PAIRING_STATE_AUTHENTICATION_ACTION);
    } else if (current_pairing_state == PAIRING_STATE_IDENTIFICATION_WAIT_FOR_ACK) {
        pairing_state_set_current_state(PAIRING_STATE_IDENTIFICATION_WAIT_FOR_RESPONSE);
    } else if (current_pairing_state == PAIRING_STATE_ADDRESSING_WAIT_FOR_ACK) {
        pairing_state_set_current_state(PAIRING_STATE_ADDRESSING_ACTION);
    }
}

void received_message_node_callback(uint8_t *received_message, uint8_t message_size)
{
    received_pairing_command = received_message[PAIRING_BYTE_COMMAND];

    /* Verify if the received message is valid. */
    if (message_size < PAIRING_MAX_PAYLOAD_SIZE) {
        memcpy(received_payload, received_message, message_size);
    } else {
        /* Something went wrong, pairing is aborted. */
        pairing_error_set_error(PAIRING_ERR_WIRELESS_ERROR);
    }
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief This state is the entry point for the node states.
 */
static void enter_pairing(void)
{
    /* Set the wireless callbacks. */
    pairing_wireless_set_sent_message_callback(sent_message_node_callback);
    pairing_wireless_set_received_message_callback(received_message_node_callback);

    /* Set the next state for the node device. */
    pairing_state_set_current_state(PAIRING_STATE_AUTHENTICATION_WAIT_FOR_MESSAGE);

    /* The wireless core can start after the configuration. */
    pairing_wireless_connect();
}

/** @brief This state is transitory before exiting to the application.
 */
static void exit_pairing(void)
{
    return;
}

/** @brief Wait for the authentication message from the coordinator.
 */
static void authentication_wait_for_message(void)
{
    bool is_app_code_valid = false;

    if (received_pairing_command == PAIRING_COMMAND_AUTHENTICATION_MESSAGE) {
        received_pairing_command = PAIRING_COMMAND_NONE;
        memcpy(&pairing_authentication_message, &received_payload, sizeof(pairing_authentication_message));

        /* Store the device application code . */
        is_app_code_valid = pairing_security_compare_app_code(pairing_authentication_message.app_code);

        /* Verify if the application code is valid. */
        if (is_app_code_valid) {
            pairing_authentication_action = PAIRING_AUTHENTICATION_ACTION_SUCCESS;
        } else {
            pairing_authentication_action = PAIRING_AUTHENTICATION_ACTION_FAIL;
        }

        pairing_state_set_current_state(PAIRING_STATE_AUTHENTICATION_SEND_RESPONSE);
    }
}

/** @brief A response is sent to the coordinator with the action taken on the last received message.
 */
static void authentication_send_response(void)
{
    /* Prepare the authentication response. */
    pairing_authentication_response.pairing_command  = PAIRING_COMMAND_AUTHENTICATION_RESPONSE;
    pairing_authentication_response.pairing_authentication_action  = pairing_authentication_action;

    pairing_wireless_send_message((uint8_t *)&pairing_authentication_response, sizeof(pairing_authentication_response));

    pairing_state_set_current_state(PAIRING_STATE_AUTHENTICATION_WAIT_FOR_ACK);
}

/** @brief Wait for the last sent message to be acknowledged.
 */
static void authentication_wait_for_ack(void)
{
    return;
}

/** @brief The action sent in the response message is applied for this device.
 */
static void authentication_action(void)
{
    if (pairing_authentication_action == PAIRING_AUTHENTICATION_ACTION_SUCCESS) {
        pairing_state_set_current_state(PAIRING_STATE_IDENTIFICATION_SEND_MESSAGE);
    } else if (pairing_authentication_action == PAIRING_AUTHENTICATION_ACTION_FAIL) {
        pairing_event_set_event(PAIRING_EVENT_INVALID_APP_CODE);
        pairing_state_set_current_state(PAIRING_STATE_EXIT);
    }

    pairing_authentication_action = PAIRING_AUTHENTICATION_ACTION_NONE;
}

/** @brief Send the identification message to the coordinator.
 */
static void identification_send_message(void)
{
    /* Prepare the identification message. */
    pairing_identification_message.pairing_command = PAIRING_COMMAND_IDENTIFICATION_MESSAGE;
    pairing_identification_message.device_role     = pairing_address_get_device_role();
    pairing_identification_message.unique_id       = pairing_wireless_get_radio_serial_number();

    pairing_wireless_send_message((uint8_t *)&pairing_identification_message, sizeof(pairing_identification_message));

    pairing_state_set_current_state(PAIRING_STATE_IDENTIFICATION_WAIT_FOR_ACK);
}

/** @brief Wait for the last sent message to be acknowledged.
 */
static void identification_wait_for_ack(void)
{
    return;
}

/** @brief Wait for the coordinator's identification response message.
 */
static void identification_wait_for_response(void)
{
    if (received_pairing_command == PAIRING_COMMAND_IDENTIFICATION_RESPONSE) {
        received_pairing_command = PAIRING_COMMAND_NONE;

        /* Store the identification action. */
        memcpy(&pairing_identification_response, &received_payload, sizeof(pairing_identification_response));
        pairing_identification_action = pairing_identification_response.pairing_identification_action;

        pairing_state_set_current_state(PAIRING_STATE_IDENTIFICATION_ACTION);
    }
}

/** @brief An action is taken depending on the received response.
 */
static void identification_action(void)
{
    if (pairing_identification_action == PAIRING_IDENTIFICATION_ACTION_SUCCESS) {
        pairing_state_set_current_state(PAIRING_STATE_ADDRESSING_WAIT_FOR_MESSAGE);
    } else if (pairing_identification_action == PAIRING_IDENTIFICATION_ACTION_FAIL) {
        pairing_state_set_current_state(PAIRING_STATE_EXIT);
    }

    pairing_identification_action = PAIRING_IDENTIFICATION_ACTION_NONE;
}

/** @brief Wait for the addressing message from the coordinator.
 */
static void addressing_wait_for_message(void)
{
    if (received_pairing_command == PAIRING_COMMAND_ADDRESSING_MESSAGE) {
        received_pairing_command = PAIRING_COMMAND_NONE;

        /* Store the received addressing message. */
        memcpy(&pairing_addressing_message, &received_payload, sizeof(pairing_addressing_message));

        /* Reconfigure the coordinator and node addresses once they have been learned. */
        pairing_address_set_pan_id(pairing_addressing_message.pan_id);
        pairing_address_set_coordinator_address(pairing_addressing_message.coordinator_id);
        pairing_address_set_node_address(pairing_addressing_message.node_id);

        pairing_addressing_action = PAIRING_ADDRESSING_ACTION_SUCCESS;

        pairing_state_set_current_state(PAIRING_STATE_ADDRESSING_SEND_RESPONSE);
    }
}

/** @brief A response is sent to the coordinator with the action taken on the last received message.
 */
static void addressing_send_response(void)
{
    /* Prepare the addressing response. */
    pairing_addressing_response.pairing_command  = PAIRING_COMMAND_ADDRESSING_RESPONSE;
    pairing_addressing_response.pairing_addressing_action = pairing_addressing_action;

    pairing_wireless_send_message((uint8_t *)&pairing_addressing_response, sizeof(pairing_addressing_response));

    pairing_state_set_current_state(PAIRING_STATE_ADDRESSING_WAIT_FOR_ACK);
}

/** @brief Wait for the last sent message to be acknowledged.
 */
static void addressing_wait_for_ack(void)
{
    return;
}

/** @brief The action sent in the response is applied for this device.
 */
static void addressing_action(void)
{
    if (pairing_addressing_action == PAIRING_ADDRESSING_ACTION_SUCCESS) {
        pairing_event_set_event(PAIRING_EVENT_SUCCESS);
        pairing_state_set_current_state(PAIRING_STATE_EXIT);
    } else if (pairing_addressing_action == PAIRING_ADDRESSING_ACTION_FAIL) {
        pairing_state_set_current_state(PAIRING_STATE_EXIT);
    }

    pairing_addressing_action = PAIRING_ADDRESSING_ACTION_NONE;
}
