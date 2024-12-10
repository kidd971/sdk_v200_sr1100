/** @file  pairing_state_coordinator.c
 *  @brief This file handles the functions related to the coordinator's pairing states.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "pairing_state_coordinator.h"
#include "pairing_address.h"
#include "pairing_error.h"
#include "pairing_event.h"
#include "pairing_message.h"
#include "pairing_security.h"
#include "pairing_state.h"
#include "pairing_state_machine.h"
#include "pairing_wireless.h"

/* MACROS *********************************************************************/
#define EXTRACT_PAN_ID(x) ((x >> (8 * 1)) & 0x0FFF)
#define EXTRACT_DEVICE_ADDRESS(x) ((x >> (8 * 0)) & 0xFF)
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

static void authentication_send_message(void);
static void authentication_wait_for_ack(void);
static void authentication_wait_for_response(void);
static void authentication_action(void);

static void identification_wait_for_message(void);
static void identification_send_response(void);
static void identification_wait_for_ack(void);
static void identification_action(void);

static void addressing_send_message(void);
static void addressing_wait_for_ack(void);
static void addressing_wait_for_response(void);
static void addressing_action(void);

/* TYPES **********************************************************************/
static pairing_state_machine_t pairing_state_machine[] = {
    {(uint8_t)PAIRING_STATE_ENTER, enter_pairing},
    {(uint8_t)PAIRING_STATE_EXIT, exit_pairing},

    {(uint8_t)PAIRING_STATE_AUTHENTICATION_SEND_MESSAGE, authentication_send_message},
    {(uint8_t)PAIRING_STATE_AUTHENTICATION_WAIT_FOR_ACK, authentication_wait_for_ack},
    {(uint8_t)PAIRING_STATE_AUTHENTICATION_WAIT_FOR_RESPONSE, authentication_wait_for_response},
    {(uint8_t)PAIRING_STATE_AUTHENTICATION_ACTION, authentication_action},

    {(uint8_t)PAIRING_STATE_IDENTIFICATION_WAIT_FOR_MESSAGE, identification_wait_for_message},
    {(uint8_t)PAIRING_STATE_IDENTIFICATION_SEND_RESPONSE, identification_send_response},
    {(uint8_t)PAIRING_STATE_IDENTIFICATION_WAIT_FOR_ACK, identification_wait_for_ack},
    {(uint8_t)PAIRING_STATE_IDENTIFICATION_ACTION, identification_action},

    {(uint8_t)PAIRING_STATE_ADDRESSING_SEND_MESSAGE, addressing_send_message},
    {(uint8_t)PAIRING_STATE_ADDRESSING_WAIT_FOR_ACK, addressing_wait_for_ack},
    {(uint8_t)PAIRING_STATE_ADDRESSING_WAIT_FOR_RESPONSE, addressing_wait_for_response},
    {(uint8_t)PAIRING_STATE_ADDRESSING_ACTION, addressing_action},
};

/* PUBLIC FUNCTIONS ***********************************************************/
void pairing_state_coordinator_init(void)
{
    uint8_t state_machine_size = 0;

    state_machine_size = (ARRAY_SIZE(pairing_state_machine));
    pairing_state_machine_init(pairing_state_machine, state_machine_size);

    pairing_state_set_current_state(PAIRING_STATE_ENTER);
}

void sent_message_coordinator_callback(void)
{
    pairing_state_t current_pairing_state = pairing_state_get_current_state();

    /* Used by the pairing procedure to ensure that the message was sent successfully. */
    if (current_pairing_state == PAIRING_STATE_AUTHENTICATION_WAIT_FOR_ACK) {
        pairing_state_set_current_state(PAIRING_STATE_AUTHENTICATION_WAIT_FOR_RESPONSE);
    } else if (current_pairing_state == PAIRING_STATE_IDENTIFICATION_WAIT_FOR_ACK) {
        pairing_state_set_current_state(PAIRING_STATE_IDENTIFICATION_ACTION);
    } else if (current_pairing_state == PAIRING_STATE_ADDRESSING_WAIT_FOR_ACK) {
        pairing_state_set_current_state(PAIRING_STATE_ADDRESSING_WAIT_FOR_RESPONSE);
    }
}

void received_message_coordinator_callback(uint8_t *received_message, uint8_t message_size)
{
    received_pairing_command = (pairing_command_t)received_message[PAIRING_BYTE_COMMAND];

    /* Verify if the received message is valid. */
    if (message_size < PAIRING_MAX_PAYLOAD_SIZE) {
        memcpy(received_payload, received_message, message_size);
    } else {
        /* Something went wrong, pairing is aborted. */
        pairing_error_set_error(PAIRING_ERR_WIRELESS_ERROR);
    }
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief This state is the entry point for the coordinator states.
 */
static void enter_pairing(void)
{
    uint32_t generated_address = 0;
    uint16_t pan_id = 0;
    uint8_t coordinator_address = 0;
    uint64_t unique_id = 0;

    /* Set the wireless callbacks. */
    pairing_wireless_set_sent_message_callback(sent_message_coordinator_callback);
    pairing_wireless_set_received_message_callback(received_message_coordinator_callback);

    /* Generate an address for the coordinator. */
    unique_id = pairing_wireless_get_radio_serial_number();
    generated_address = pairing_address_generate_serialized_address(unique_id);
    pan_id = EXTRACT_PAN_ID(generated_address);
    coordinator_address = EXTRACT_DEVICE_ADDRESS(generated_address);
    pairing_address_set_pan_id(pan_id);
    pairing_address_set_coordinator_address(coordinator_address);

    /* Add the coordinator to the pairing discovery list. */
    pairing_address_add_node_to_device_discovery_list(0, coordinator_address, unique_id);

    /* Set the next state for the coordinator. */
    pairing_state_set_current_state(PAIRING_STATE_AUTHENTICATION_SEND_MESSAGE);

    /* The wireless core can start after the configuration. */
    pairing_wireless_connect();
}

/** @brief This state is transitory before exiting to the application.
 */
static void exit_pairing(void)
{
    return;
}

/** @brief Send the authentication message to the node.
 */
static void authentication_send_message(void)
{
    /* Prepare the authentication message. */
    pairing_authentication_message.pairing_command = PAIRING_COMMAND_AUTHENTICATION_MESSAGE;
    pairing_authentication_message.app_code        = pairing_security_get_app_code();

    pairing_wireless_send_message((uint8_t *)&pairing_authentication_message, sizeof(pairing_authentication_message));

    pairing_state_set_current_state(PAIRING_STATE_AUTHENTICATION_WAIT_FOR_ACK);
}

/** @brief Wait for the last sent message to be acknowledged.
 */
static void authentication_wait_for_ack(void)
{
    return;
}

/** @brief Wait for the node to verify the authentication.
 */
static void authentication_wait_for_response(void)
{
    if (received_pairing_command == PAIRING_COMMAND_AUTHENTICATION_RESPONSE) {
        received_pairing_command = PAIRING_COMMAND_NONE;

        /* Store the authentication action. */
        memcpy(&pairing_authentication_response, &received_payload, sizeof(pairing_authentication_response));
        pairing_authentication_action = pairing_authentication_response.pairing_authentication_action;

        pairing_state_set_current_state(PAIRING_STATE_AUTHENTICATION_ACTION);
    }
}

/** @brief An action is taken depending on the received response.
 */
static void authentication_action(void)
{
    if (pairing_authentication_action == PAIRING_AUTHENTICATION_ACTION_SUCCESS) {
        pairing_state_set_current_state(PAIRING_STATE_IDENTIFICATION_WAIT_FOR_MESSAGE);
    } else if (pairing_authentication_action == PAIRING_AUTHENTICATION_ACTION_FAIL) {
        pairing_event_set_event(PAIRING_EVENT_INVALID_APP_CODE);
        pairing_state_set_current_state(PAIRING_STATE_EXIT);
    }
    pairing_authentication_action = PAIRING_AUTHENTICATION_ACTION_NONE;
}

/** @brief Wait for the identification message from the node.
 */
static void identification_wait_for_message(void)
{
    uint32_t generated_address = 0;
    uint8_t node_address = 0;

    if (received_pairing_command == PAIRING_COMMAND_IDENTIFICATION_MESSAGE) {
        received_pairing_command = PAIRING_COMMAND_NONE;

        /* Store the received identification message. */
        memcpy(&pairing_identification_message, &received_payload, sizeof(pairing_identification_message));

        /* Generate an address for the node. */
        generated_address = pairing_address_generate_serialized_address(pairing_identification_message.unique_id);
        node_address = EXTRACT_DEVICE_ADDRESS(generated_address);

        /* Store an available node address based on serialized address. */
        pairing_address_set_node_address(pairing_address_get_available_node_id(node_address));

        /* Add the node to the pairing discovery list. */
        pairing_address_add_node_to_device_discovery_list(pairing_identification_message.device_role,
                                                          pairing_address_get_node_address(),
                                                          pairing_identification_message.unique_id);

        pairing_identification_action = PAIRING_IDENTIFICATION_ACTION_SUCCESS;

        pairing_state_set_current_state(PAIRING_STATE_IDENTIFICATION_SEND_RESPONSE);
    }
}

/** @brief A response is sent to the node with the action taken on the last received message.;
 */
static void identification_send_response(void)
{
    /* Prepare the identification response. */
    pairing_identification_response.pairing_command = PAIRING_COMMAND_IDENTIFICATION_RESPONSE;
    pairing_identification_response.pairing_identification_action = pairing_identification_action;

    pairing_wireless_send_message((uint8_t *)&pairing_identification_response, sizeof(pairing_identification_response));

    pairing_state_set_current_state(PAIRING_STATE_IDENTIFICATION_WAIT_FOR_ACK);
}

/** @brief Wait for the last sent message to be acknowledged.
 */
static void identification_wait_for_ack(void)
{
    return;
}

/** @brief The action sent in the response is applied for this device.
 */
static void identification_action(void)
{
    if (pairing_identification_action == PAIRING_IDENTIFICATION_ACTION_SUCCESS) {
        pairing_state_set_current_state(PAIRING_STATE_ADDRESSING_SEND_MESSAGE);
    } else if (pairing_identification_action == PAIRING_IDENTIFICATION_ACTION_FAIL) {
        pairing_state_set_current_state(PAIRING_STATE_EXIT);
    }

    pairing_identification_action = PAIRING_IDENTIFICATION_ACTION_NONE;
}

/** @brief Send the pairing addresses to the node.
 */
static void addressing_send_message(void)
{
    /* Prepare the addressing message. */
    pairing_addressing_message.pairing_command = PAIRING_COMMAND_ADDRESSING_MESSAGE;
    pairing_addressing_message.pan_id          = pairing_address_get_pan_id();
    pairing_addressing_message.coordinator_id  = pairing_address_get_coordinator_address();
    pairing_addressing_message.node_id         = pairing_address_get_node_address();

    pairing_wireless_send_message((uint8_t *)&pairing_addressing_message, sizeof(pairing_addressing_message));

    pairing_state_set_current_state(PAIRING_STATE_ADDRESSING_WAIT_FOR_ACK);
}

/** @brief Wait for the last sent message to be acknowledged.
 */
static void addressing_wait_for_ack(void)
{
    return;
}

/** @brief Wait for the node's addressing response  message.
 */
static void addressing_wait_for_response(void)
{
    if (received_pairing_command == PAIRING_COMMAND_ADDRESSING_RESPONSE) {
        received_pairing_command = PAIRING_COMMAND_NONE;

        /* Store the addressing action. */
        memcpy(&pairing_addressing_response, &received_payload, sizeof(pairing_addressing_response));
        pairing_addressing_action = pairing_addressing_response.pairing_addressing_action;

        pairing_state_set_current_state(PAIRING_STATE_ADDRESSING_ACTION);
    }
}

/** @brief An action is taken depending on the received response.
 */
static void addressing_action(void)
{
    if (pairing_addressing_action == PAIRING_ADDRESSING_ACTION_SUCCESS) {
        pairing_event_set_event(PAIRING_EVENT_SUCCESS);
        pairing_state_set_current_state(PAIRING_STATE_EXIT);
    }
    pairing_addressing_action = PAIRING_ADDRESSING_ACTION_NONE;
}
