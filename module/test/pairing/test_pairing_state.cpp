/** @file  test_pairing_state.cpp
 *  @brief Unit tests for the Pairing state.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team
 */

/* INCLUDES *******************************************************************/
#include "catch2/catch_all.hpp"
#include "pairing_state.h"
#include "swc_def.h"

/* UNIT TESTS *****************************************************************/
SCENARIO( "pairing_state_init() : validates initialization of the state", "[pairing]" ) {

    GIVEN( "valid role" ) {

        swc_role_t role = SWC_ROLE_COORDINATOR;
        pairing_state_t state = PAIRING_STATE_EXIT;

        WHEN( "calling the function for initializing" ) {
            pairing_state_init(role);
            state = pairing_state_get_current_state();
            THEN( "the state is set at the beginning" ) {
                REQUIRE( state == PAIRING_STATE_ENTER );
            }
        }
    }
}

SCENARIO( "pairing_state_get_current_state() : validates the configuration & de the recuperation of the state", "[pairing]" ) {

    GIVEN( "pairing state to set" ) {

        swc_role_t role = SWC_ROLE_COORDINATOR;
        pairing_state_t state = PAIRING_STATE_EXIT;

        pairing_state_init(role);

        WHEN( "calling the setter" ) {
            pairing_state_set_current_state(PAIRING_STATE_AUTHENTICATION_SEND_MESSAGE);
            state = pairing_state_get_current_state();
            THEN( "the state is set correctly" ) {
                REQUIRE( state == PAIRING_STATE_AUTHENTICATION_SEND_MESSAGE );
            }
        }
    }
}
