/** @file  test_pairing_machine_state.cpp
 *  @brief Unit tests for the Pairing Machine State module.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team
 */

/* INCLUDES *******************************************************************/
#include "catch2/catch_all.hpp"
#include "pairing_state_machine.h"

/* TYPES **********************************************************************/
typedef enum pairing_state {
    PAIRING_STATE_1,
    PAIRING_STATE_2,
    PAIRING_STATE_3,
} pairing_state_t;

/* PRIVATE GLOBALS ************************************************************/
static bool state_1_called;
static bool state_2_called;
static bool state_3_called;

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief First state function to test the machine state.
 */
void state_function_1(void)
{
    state_1_called = true;
}

/** @brief Second state function to test the machine state.
 */
void state_function_2(void)
{
    state_2_called = true;
}

/** @brief Third state function to test the machine state.
 */
void state_function_3(void)
{
    state_3_called = true;
}

pairing_state_machine_t state_machine[] = {
    {(uint8_t)PAIRING_STATE_1, state_function_1},
    {(uint8_t)PAIRING_STATE_2, state_function_2},
    {(uint8_t)PAIRING_STATE_3, state_function_3},
};

/* UNIT TESTS *****************************************************************/
SCENARIO( "Pairing state machine initialization", "[pairing]" ) {

    GIVEN( "The state machine is initialized" ) {

        uint8_t state_machine_size = 0;

        state_machine_size = (sizeof(state_machine) / sizeof(pairing_state_machine_t));

        WHEN( "the application state is passed as a parameter" ) {
            pairing_state_machine_init(state_machine, state_machine_size);

            THEN( "the local state machine is assigned" ) {
                REQUIRE( state_machine == pairing_state_machine_get_instance() );
            }
            THEN( "the size is correctly updated" ) {
                REQUIRE( pairing_state_machine_get_size() == state_machine_size );
                REQUIRE( pairing_state_machine_get_size() == 3 );
            }
        }
    }
}

SCENARIO( "Pairing state machine state function execution", "[pairing]" ) {

    GIVEN( "an initialized state machine" ) {

        uint8_t state_machine_size = 0;

        state_machine_size = (sizeof(state_machine) / sizeof(pairing_state_machine_t));
        pairing_state_machine_init(state_machine, state_machine_size);

        WHEN( "passing the first state to execute its associated function" ) {
            pairing_state_machine_execute_state((uint8_t)PAIRING_STATE_1);

            THEN( "the function is called successfully" ) {
                REQUIRE( state_1_called == true );
            }
        }
        WHEN( "passing the second state to execute its associated function" ) {
            pairing_state_machine_execute_state((uint8_t)PAIRING_STATE_2);

            THEN( "the function is called successfully" ) {
                REQUIRE( state_2_called == true );
            }
        }
        WHEN( "passing the third state to execute its associated function" ) {
            pairing_state_machine_execute_state((uint8_t)PAIRING_STATE_3);

            THEN( "the function is called successfully" ) {
                REQUIRE( state_3_called == true );
            }
        }
    }
}
