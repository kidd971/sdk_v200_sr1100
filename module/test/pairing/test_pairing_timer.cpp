/** @file  test_pairing_timer.cpp
 *  @brief Unit tests for the Pairing Timer module.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team
 */

/* INCLUDES *******************************************************************/
#include "catch2/catch_all.hpp"
#include "pairing_timer.h"
#include "test_mock_helper.h"
#include "test_utils.h"

/* PRIVATE FUNCTIONS **********************************************************/

/* UNIT TESTS *****************************************************************/
SCENARIO( "The timer is initialized ", "[pairing]" ) {

    GIVEN( "A valid hal function and a timeout period" ) {

        RESET_FAKE(get_tick_impl);
        uint64_t time_tick[] = {0, 6000};
        SET_RETURN_SEQ(get_tick_impl, time_tick, sizeof(time_tick) / sizeof(uint64_t));

        pairing_start_timeout_counter(5);

        WHEN( "the timeout is reached" ) {
            pairing_timer_get_current_timer_tick_count();

            THEN( "the pairing timeout duration is set properly" ) {
                REQUIRE( pairing_timer_is_timeout() == true );
            }
        }
    }

    GIVEN( "A valid hal function and a timeout period" ) {

        RESET_FAKE(get_tick_impl);
        uint64_t time_tick[] = {0, 4000};
        SET_RETURN_SEQ(get_tick_impl, time_tick, sizeof(time_tick) / sizeof(uint64_t));

        pairing_start_timeout_counter(5);

        WHEN( "the timeout is not yet reached" ) {
            pairing_timer_get_current_timer_tick_count();

            THEN( "the pairing timeout duration is set properly" ) {
                REQUIRE( pairing_timer_is_timeout() == false );
            }
        }
    }
}
