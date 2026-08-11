/** @file  test_pairing_event.cpp
 *  @brief Unit tests for the Pairing event module.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team
 */

/* INCLUDES *******************************************************************/
#include "catch2/catch_all.hpp"
#include "pairing_event.h"

/* UNIT TESTS *****************************************************************/
SCENARIO( "pairing_event_init() : validates initialisation", "[pairing]" ) {
    pairing_event_t pairing_event_to_validate = PAIRING_EVENT_NONE;

    GIVEN( "nothing" ) {
        WHEN( "calling the function" ) {
            pairing_event_init();
            pairing_event_to_validate = pairing_event_get_event();
            THEN( "the local variable of event is initialized properly" ) {
                REQUIRE( pairing_event_to_validate == PAIRING_EVENT_NONE );
            }
        }
    }
}

SCENARIO( "pairing_event_set_event() : validates the configuration", "[pairing]" ) {
    pairing_event_t pairing_event_to_validate = PAIRING_EVENT_NONE;
    pairing_event_init();

    GIVEN( "the pairing event to set" ) {
        WHEN( "calling the setter" ) {
            pairing_event_set_event(PAIRING_EVENT_ABORT);
            pairing_event_to_validate = pairing_event_get_event();
            THEN( "the local variable of event is set properly" ) {
                REQUIRE( pairing_event_to_validate == PAIRING_EVENT_ABORT );
            }
        }
    }
}
