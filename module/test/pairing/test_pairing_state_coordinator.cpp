/** @file  test_pairing_state_coordinator.cpp
 *  @brief Unit tests for the coordinator's pairing state.
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
#include "pairing_state_coordinator.h"
#include "pairing_timer.h"
#include "pairing_wireless.h"
#include "test_mock_helper.h"

/* CONSTANTS ******************************************************************/
#define UNIQUE_ID        0x12345678
#define APP_CODE         0x12345678
#define TIMEOUT_SEC 10

/* PRIVATE GLOBALS ************************************************************/
static uint8_t *received_payload[PAIRING_MAX_PAYLOAD_SIZE];
static pairing_command_t received_pairing_command;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void init_pairing(void);

/* UNIT TESTS *****************************************************************/
SCENARIO( "The user started the pairing procedure with the coordinator", "[pairing]" ) {

    init_pairing();

    GIVEN( "The pairing is initialized" ) {

        WHEN( "the first state is called" ) {

            THEN( "the coordinator state is initialized" ) {
                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_ENTER );
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_NONE );
            }
        }
    }

    GIVEN( "The pairing is initialized" ) {
        /* Mock SPI bit AWAKE when trying to connect */
        uint8_t spi_read_byte = 0xFF;

        SET_RETURN_SEQ(spi_read_byte, &spi_read_byte, 1)
        WHEN( "the enter_pairing state is executed" ) {
            pairing_state_execute_current_state();

            THEN( "the next state is authentication send message" ) {
                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_AUTHENTICATION_SEND_MESSAGE );
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_NONE );
            }
        }
    }

    GIVEN( "The current state is authentication send message" ) {
        pairing_state_execute_current_state();

        WHEN( "the authentication send message state is executed" ) {
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
            sent_message_coordinator_callback();

            THEN( "the next state is authentication wait for response" ) {
                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_AUTHENTICATION_WAIT_FOR_RESPONSE );
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_NONE );
            }
        }
    }

    GIVEN( "The current state is authentication wait for response and the response arrives" ) {
        pairing_state_execute_current_state();
        pairing_state_set_current_state(PAIRING_STATE_AUTHENTICATION_WAIT_FOR_RESPONSE);

        /* Simulate a pairing response message from the node. */
        pairing_authentication_response_t authentication_response = {0};
        authentication_response.pairing_command = PAIRING_COMMAND_AUTHENTICATION_RESPONSE;
        authentication_response.pairing_authentication_action = PAIRING_AUTHENTICATION_ACTION_SUCCESS;

        /* Convert struct to byte array */
        uint8_t *message_array = (uint8_t *)&authentication_response;

        received_message_coordinator_callback(message_array, sizeof(authentication_response));

        WHEN( "the state is executed" ) {
            pairing_state_execute_current_state();

            THEN( "the next state is authentication action" ) {
                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_AUTHENTICATION_ACTION );
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_NONE );
            }
        }
    }

    GIVEN( "The current state is authentication action with a successful action" ) {
        pairing_state_execute_current_state();
        pairing_state_set_current_state(PAIRING_STATE_AUTHENTICATION_WAIT_FOR_RESPONSE);

        /* Simulate a pairing response message from the node. */
        pairing_authentication_response_t authentication_response = {0};
        authentication_response.pairing_command = PAIRING_COMMAND_AUTHENTICATION_RESPONSE;
        authentication_response.pairing_authentication_action = PAIRING_AUTHENTICATION_ACTION_SUCCESS;

        /* Convert struct to byte array */
        uint8_t *message_array = (uint8_t *)&authentication_response;

        received_message_coordinator_callback(message_array, sizeof(authentication_response));
        pairing_state_execute_current_state();

        WHEN( "the action state is executed and the action is a success" ) {
            pairing_state_execute_current_state();

            THEN( "the next state is identification wait for message" ) {
                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_IDENTIFICATION_WAIT_FOR_MESSAGE );
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_NONE );
            }
        }
    }

    GIVEN( "The current state is authentication action with a failed action" ) {
        pairing_state_execute_current_state();
        pairing_state_set_current_state(PAIRING_STATE_AUTHENTICATION_WAIT_FOR_RESPONSE);

        /* Simulate a pairing response message from the node. */
        pairing_authentication_response_t authentication_response = {0};
        authentication_response.pairing_command = PAIRING_COMMAND_AUTHENTICATION_RESPONSE;
        authentication_response.pairing_authentication_action = PAIRING_AUTHENTICATION_ACTION_FAIL;

        /* Convert struct to byte array */
        uint8_t *message_array = (uint8_t *)&authentication_response;

        received_message_coordinator_callback(message_array, sizeof(authentication_response));
        pairing_state_execute_current_state();

        WHEN( "the action state is executed and the action is a failure" ) {
            pairing_state_execute_current_state();

            THEN( "the pairing process is exited with the corresponding event" ) {
                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_EXIT );
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_INVALID_APP_CODE );
            }
        }
    }

    GIVEN( "The current state is identification wait for message" ) {
        pairing_state_execute_current_state();
        pairing_state_set_current_state(PAIRING_STATE_IDENTIFICATION_WAIT_FOR_MESSAGE);

        WHEN( "a message is received" ) {
            /* Simulate a pairing identification message from the node. */
            pairing_identification_message identification_message = {0};
            identification_message.pairing_command = PAIRING_COMMAND_IDENTIFICATION_MESSAGE;
            identification_message.device_role = 1;
            identification_message.unique_id = UNIQUE_ID;

            /* Convert struct to byte array */
            uint8_t *message_array = (uint8_t *)&identification_message;

            received_message_coordinator_callback(message_array, sizeof(identification_message));
            pairing_state_execute_current_state();

            THEN( "the next state is identification send response" ) {
                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_IDENTIFICATION_SEND_RESPONSE );
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_NONE );
            }
        }
    }

    GIVEN( "The current state is identification send response" ) {
        pairing_state_execute_current_state();
        pairing_state_set_current_state(PAIRING_STATE_IDENTIFICATION_SEND_RESPONSE);

        WHEN( "the response is sent" ) {
            pairing_state_execute_current_state();

            THEN( "the next state is identification send response" ) {
                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_IDENTIFICATION_WAIT_FOR_ACK );
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_NONE );
            }
        }
    }

    GIVEN( "The current state is identification action" ) {
        pairing_state_execute_current_state();
        pairing_state_set_current_state(PAIRING_STATE_IDENTIFICATION_WAIT_FOR_MESSAGE);

        /* Simulate a pairing identification message from the node. */
        pairing_identification_message identification_message = {0};
        identification_message.pairing_command = PAIRING_COMMAND_IDENTIFICATION_MESSAGE;
        identification_message.device_role = 1;
        identification_message.unique_id = UNIQUE_ID;

        /* Convert struct to byte array */
        uint8_t *message_array = (uint8_t *)&identification_message;

        received_message_coordinator_callback(message_array, sizeof(identification_message));
        pairing_state_execute_current_state();

        pairing_state_set_current_state(PAIRING_STATE_IDENTIFICATION_ACTION);

        WHEN( "the action is taken and the it is successful" ) {
            pairing_state_execute_current_state();

            THEN( "the next state is addressing send message" ) {
                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_ADDRESSING_SEND_MESSAGE );
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_NONE );
            }
        }
    }

    GIVEN( "The current state is addressing send message" ) {
        pairing_state_execute_current_state();
        pairing_state_set_current_state(PAIRING_STATE_ADDRESSING_SEND_MESSAGE);

        WHEN( "the message is sent" ) {
            pairing_state_execute_current_state();

            THEN( "the next state is identification wait for ack" ) {
                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_ADDRESSING_WAIT_FOR_ACK );
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_NONE );
            }
        }
    }

    GIVEN( "The current state is addressing wait for ack" ) {
        pairing_state_execute_current_state();
        pairing_state_set_current_state(PAIRING_STATE_ADDRESSING_WAIT_FOR_ACK);

        WHEN( "the sent message is acked" ) {
            sent_message_coordinator_callback();

            THEN( "the next state is identification wait for ack" ) {
                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_ADDRESSING_WAIT_FOR_RESPONSE );
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_NONE );
            }
        }
    }

    GIVEN( "The current state is addressing wait for response and the response arrived" ) {
        pairing_state_execute_current_state();
        pairing_state_set_current_state(PAIRING_STATE_ADDRESSING_WAIT_FOR_RESPONSE);

        /* Simulate a pairing response message from the node. */
        pairing_addressing_response_t addressing_response = {0};
        addressing_response.pairing_command = PAIRING_COMMAND_ADDRESSING_RESPONSE;
        addressing_response.pairing_addressing_action = PAIRING_ADDRESSING_ACTION_SUCCESS;

        /* Convert struct to byte array */
        uint8_t *message_array = (uint8_t *)&addressing_response;

        received_message_coordinator_callback(message_array, sizeof(addressing_response));

        WHEN( "the state is executed" ) {
            pairing_state_execute_current_state();

            THEN( "the next state is addressing action" ) {
                REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_ADDRESSING_ACTION );
                REQUIRE( pairing_event_get_event() == PAIRING_EVENT_NONE );
            }
        }
    }

    GIVEN( "The current state is addressing action and the response was successful" ) {
        pairing_state_execute_current_state();
        pairing_state_set_current_state(PAIRING_STATE_ADDRESSING_WAIT_FOR_RESPONSE);

        /* Simulate a pairing response message from the node. */
        pairing_addressing_response_t addressing_response = {0};
        addressing_response.pairing_command = PAIRING_COMMAND_ADDRESSING_RESPONSE;
        addressing_response.pairing_addressing_action = PAIRING_ADDRESSING_ACTION_SUCCESS;

        /* Convert struct to byte array */
        uint8_t *message_array = (uint8_t *)&addressing_response;

        received_message_coordinator_callback(message_array, sizeof(addressing_response));

        pairing_state_execute_current_state();

        REQUIRE( pairing_state_get_current_state() == PAIRING_STATE_ADDRESSING_ACTION );

    WHEN( "the action is successful" ) {
            pairing_state_execute_current_state();

            THEN( "the pairing procedure is successful" ) {
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
    constexpr uint8_t pairing_discovery_list_size = 2;
    static pairing_discovery_list_t pairing_discovery_list[pairing_discovery_list_size];

    pairing_discovery_list[0].node_address = 0x12;
    pairing_discovery_list[0].unique_id = 0x12345678;
    pairing_discovery_list[1].node_address = 0x34;
    pairing_discovery_list[1].unique_id = 0x87654321;

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

    /* Coordinator is always the device role 0. */
    pairing_address_set_device_role(PAIRING_DEVICE_ROLE_COORDINATOR);

    /* Initialize the discovery list. */
    pairing_address_discovery_list_init(pairing_discovery_list, pairing_discovery_list_size);

    /* Initialize the state machine. */
    pairing_state_init(SWC_ROLE_COORDINATOR);
}
