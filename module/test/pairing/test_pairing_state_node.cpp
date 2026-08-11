/** @file  test_pairing_state_node.cpp
 *  @brief Unit tests for the node's pairing state.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team
 */

/* INCLUDES *******************************************************************/
#include "catch2/catch_all.hpp"
#include "pairing_address.h"
#include "pairing_def.h"
#include "pairing_error.h"
#include "pairing_event.h"
#include "pairing_message.h"
#include "pairing_security.h"
#include "pairing_state.h"
#include "pairing_state_node.h"
#include "pairing_timer.h"
#include "pairing_wireless.h"

/* CONSTANTS ******************************************************************/
#define APP_CODE            0x12345678
#define APP_CODE_INVERTED   0x87654321
#define TIMEOUT_SEC    10
#define PAN_ID              0x01
#define COORDINATOR_ADDRESS 0x02
#define NODE_ADDRESS        0x03

/* PRIVATE GLOBALS ************************************************************/
static uint8_t *received_payload[PAIRING_MAX_PAYLOAD_SIZE];
static pairing_command_t received_pairing_command;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void init_pairing(void);

/* UNIT TESTS *****************************************************************/
SCENARIO( "The user started the pairing procedure with the node", "[pairing]" ) {

    init_pairing();

    GIVEN( "The pairing is initialized" ) {

        WHEN( "the first state is called" ) {

            THEN( "the node state is initialized" ) {
                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_ENTER);
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_NONE );
            }
        }
    }

    GIVEN( "The pairing is initialized" ) {

        WHEN( "the enter_pairing state is executed" ) {
            pairing_state_execute_current_state();

            THEN( "the next state is authentication wait for message" ) {
                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_AUTHENTICATION_WAIT_FOR_MESSAGE);
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_NONE );
            }
        }
    }

    GIVEN( "The current state is authentication wait for message" ) {
        pairing_state_execute_current_state();

        WHEN( "a message is received" ) {
            /* Simulate a pairing identification message from the node. */
            pairing_authentication_message authentication_message = {0};
            authentication_message.pairing_command = PAIRING_COMMAND_AUTHENTICATION_MESSAGE;
            authentication_message.app_code = APP_CODE;

            /* Convert struct to byte array */
            uint8_t *message_array = (uint8_t *)&authentication_message;

            received_message_node_callback(message_array, sizeof(authentication_message));
            pairing_state_execute_current_state();

            THEN( "the next state is authentication send response" ) {
                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_AUTHENTICATION_SEND_RESPONSE );
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_NONE );
            }
        }
    }

    GIVEN( "The current state is authentication send response" ) {
        pairing_state_execute_current_state();
        pairing_state_set_current_state(PAIRING_STATE_AUTHENTICATION_SEND_RESPONSE);

        WHEN( "the response is sent" ) {
            pairing_state_execute_current_state();

            THEN( "the next state is authentication wait for ack" ) {
                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_AUTHENTICATION_WAIT_FOR_ACK );
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_NONE );
            }
        }
    }

    GIVEN( "The current state is authentication wait for ack" ) {
        pairing_state_execute_current_state();
        pairing_state_set_current_state(PAIRING_STATE_AUTHENTICATION_WAIT_FOR_ACK);

        WHEN( "an ACK is received" ) {
            sent_message_node_callback();

            THEN( "the next state is authentication action" ) {
                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_AUTHENTICATION_ACTION );
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_NONE );
            }
        }
    }

    GIVEN( "The current state is authentication action and it was a success" ) {
        pairing_state_execute_current_state();
        pairing_state_set_current_state(PAIRING_STATE_AUTHENTICATION_WAIT_FOR_MESSAGE);

        /* Simulate a pairing authentication message from the coordinator. */
        pairing_authentication_message authentication_message = {0};
        authentication_message.pairing_command = PAIRING_COMMAND_AUTHENTICATION_MESSAGE;
        authentication_message.app_code = APP_CODE;

        /* Convert struct to byte array. */
        uint8_t *message_array = (uint8_t *)&authentication_message;

        received_message_node_callback(message_array, sizeof(authentication_message));
        pairing_state_execute_current_state();

        pairing_state_set_current_state(PAIRING_STATE_AUTHENTICATION_ACTION);

        WHEN( "the authentication action state is executed" ) {
            pairing_state_execute_current_state();

            THEN( "the next state is identification send message" ) {
                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_IDENTIFICATION_SEND_MESSAGE );
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_NONE );
            }
        }
    }

    GIVEN( "The current state is authentication action and it was a failure" ) {
        pairing_state_execute_current_state();
        pairing_state_set_current_state(PAIRING_STATE_AUTHENTICATION_WAIT_FOR_MESSAGE);

        /* Simulate a pairing authentication message from the coordinator. */
        pairing_authentication_message authentication_message = {0};
        authentication_message.pairing_command = PAIRING_COMMAND_AUTHENTICATION_MESSAGE;
        authentication_message.app_code = APP_CODE_INVERTED;

        /* Convert struct to byte array. */
        uint8_t *message_array = (uint8_t *)&authentication_message;

        received_message_node_callback(message_array, sizeof(authentication_message));
        pairing_state_execute_current_state();

        pairing_state_set_current_state(PAIRING_STATE_AUTHENTICATION_ACTION);

        WHEN( "the authentication action state is executed" ) {
            pairing_state_execute_current_state();

            THEN( "the pairing process is exited with the corresponding event" ) {
                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_EXIT );
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_INVALID_APP_CODE );
            }
        }
    }

    GIVEN( "The current state is identification send message" ) {
        pairing_state_execute_current_state();
        pairing_state_set_current_state(PAIRING_STATE_IDENTIFICATION_SEND_MESSAGE);

        WHEN( "the identification send message state is executed" ) {
            pairing_state_execute_current_state();

            THEN( "the next state is identification wait for ack" ) {

                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_IDENTIFICATION_WAIT_FOR_ACK );
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_NONE );
            }
        }
    }

    GIVEN( "The current state is identification wait for ack" ) {
        pairing_state_execute_current_state();
        pairing_state_set_current_state(PAIRING_STATE_IDENTIFICATION_WAIT_FOR_ACK);

        WHEN( "an ACK is received" ) {
            sent_message_node_callback();

            THEN( "the next state is identification wait for response" ) {

                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_IDENTIFICATION_WAIT_FOR_RESPONSE );
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_NONE );
            }
        }
    }

    GIVEN( "The current state is identification wait for response and the response arrives" ) {
        pairing_state_execute_current_state();
        pairing_state_set_current_state(PAIRING_STATE_IDENTIFICATION_WAIT_FOR_RESPONSE);

        /* Simulate a pairing response message from the coordinator. */
        pairing_identification_response_t identification_response = {0};
        identification_response.pairing_command = PAIRING_COMMAND_IDENTIFICATION_RESPONSE;
        identification_response.pairing_identification_action = PAIRING_IDENTIFICATION_ACTION_SUCCESS;

        /* Convert struct to byte array */
        uint8_t *message_array = (uint8_t *)&identification_response;

        received_message_node_callback(message_array, sizeof(identification_response));

        WHEN( "the state is executed" ) {
            pairing_state_execute_current_state();

            THEN( "the next state is identification action" ) {
                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_IDENTIFICATION_ACTION );
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_NONE );
            }
        }
    }

    GIVEN( "The current state is authentication action with a successful action" ) {
        pairing_state_execute_current_state();
        pairing_state_set_current_state(PAIRING_STATE_IDENTIFICATION_WAIT_FOR_RESPONSE);

        /* Simulate a pairing response message from the coordinator. */
        pairing_identification_response_t identification_response = {0};
        identification_response.pairing_command = PAIRING_COMMAND_IDENTIFICATION_RESPONSE;
        identification_response.pairing_identification_action = PAIRING_IDENTIFICATION_ACTION_SUCCESS;

        /* Convert struct to byte array */
        uint8_t *message_array = (uint8_t *)&identification_response;

        received_message_node_callback(message_array, sizeof(identification_response));
        pairing_state_execute_current_state();

        WHEN( "the action state is executed and the action is a success" ) {
            pairing_state_execute_current_state();

            THEN( "the next state is addressing wait for message" ) {
                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_ADDRESSING_WAIT_FOR_MESSAGE );
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_NONE );
            }
        }
    }

    GIVEN( "The current state is addressing wait for message" ) {
        pairing_state_execute_current_state();
        pairing_state_set_current_state(PAIRING_STATE_ADDRESSING_WAIT_FOR_MESSAGE);

        WHEN( "a message is received" ) {
            /* Simulate a pairing addressing message from the node. */
            pairing_addressing_message_t addressing_message = {0};
            addressing_message.pairing_command = PAIRING_COMMAND_ADDRESSING_MESSAGE;
            addressing_message.pan_id = PAN_ID;
            addressing_message.coordinator_id = COORDINATOR_ADDRESS;
            addressing_message.node_id = NODE_ADDRESS;

            /* Convert struct to byte array */
            uint8_t *message_array = (uint8_t *)&addressing_message;

            received_message_node_callback(message_array, sizeof(addressing_message));
            pairing_state_execute_current_state();

            THEN( "the next state is addressing send response" ) {
                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_ADDRESSING_SEND_RESPONSE );
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_NONE );
            }
        }
    }

    GIVEN( "The current state is addressing send response" ) {
        pairing_state_execute_current_state();
        pairing_state_set_current_state(PAIRING_STATE_ADDRESSING_SEND_RESPONSE);

        WHEN( "the response is sent" ) {
            pairing_state_execute_current_state();

            THEN( "the next state is addressing wait for ack" ) {
                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_ADDRESSING_WAIT_FOR_ACK );
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_NONE );
            }
        }
    }

    GIVEN( "The current state is addressing action and the addressing was successful" ) {
        pairing_state_execute_current_state();
        pairing_state_set_current_state(PAIRING_STATE_ADDRESSING_WAIT_FOR_MESSAGE);

        /* Simulate a pairing addressing message from the coordinator. */
        pairing_addressing_message_t addressing_message = {0};
        addressing_message.pairing_command = PAIRING_COMMAND_ADDRESSING_MESSAGE;
            addressing_message.pan_id = PAN_ID;
            addressing_message.coordinator_id = COORDINATOR_ADDRESS;
            addressing_message.node_id = NODE_ADDRESS;

        /* Convert struct to byte array */
        uint8_t *message_array = (uint8_t *)&addressing_message;
        received_message_node_callback(message_array, sizeof(addressing_message));

        pairing_state_execute_current_state();

        pairing_state_set_current_state(PAIRING_STATE_ADDRESSING_ACTION);

        WHEN( "the state is executed" ) {
            pairing_state_execute_current_state();

            THEN( "the next state is authentication action" ) {
                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_EXIT );
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_SUCCESS );
            }
        }
    }
}

/* PRIVATE FUNCTIONS **********************************************************/
static void init_pairing(void)
{
    static pairing_cfg_t pairing_cfg = {
        .app_code = APP_CODE,
        .timeout_sec = TIMEOUT_SEC,
    };

    static pairing_assigned_address_t pairing_assigned_address = {};
    static pairing_error_t pairing_err;

    /* Initialize the pairing error instance. */
    pairing_error_init(&pairing_err);

    /* Initialize the pairing module. */
    /* Enable pairing specific feature in the SWC API. */
    swc_reserved_address_unlock();

    /* Get the pairing address handle from the application and create a local pairing instance. */
    pairing_address_init(&pairing_assigned_address);

    /* Initialize security related features. */
    pairing_security_init();
    pairing_security_set_app_code(pairing_cfg.app_code);

    /* Initialize the pairing events. */
    pairing_event_init();

    /* Set the timeout duration and begin counting the ticks to monitor the timeout. */
    pairing_start_timeout_counter(pairing_cfg.timeout_sec);

    /* Initialize the state machine. */
    pairing_state_init(SWC_ROLE_NODE);
}
