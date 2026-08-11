/** @file  test_pairing_security.cpp
 *  @brief Unit tests for the Pairing security module.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team
 */

/* INCLUDES *******************************************************************/
#include "catch2/catch_all.hpp"
#include "pairing_security.h"

/* CONSTANTS ******************************************************************/
#define PAIRING_APP_CODE_TEST_1 5
#define PAIRING_APP_CODE_TEST_2 10

/* UNIT TESTS *****************************************************************/
SCENARIO( "pairing_security_init() : validates initialisation", "[pairing]" ) {
    uint64_t app_code_to_validate = PAIRING_APP_CODE_TEST_1;

    GIVEN( "nothing" ) {
        WHEN( "calling the function" ) {
            pairing_security_init();
            app_code_to_validate = pairing_security_get_app_code();
            THEN( "the local variable of app code is initialise properly" ) {
                REQUIRE( app_code_to_validate == PAIRING_APP_CODE_DEFAULT);
            }
        }
    }
}

SCENARIO( "pairing_security_set_app_code() : validates the configuration", "[pairing]" ) {
    uint64_t app_code_to_validate = PAIRING_APP_CODE_TEST_1;
    pairing_security_init();

    GIVEN( "the app code to set" ) {
        WHEN( "calling the setter" ) {
            pairing_security_set_app_code(PAIRING_APP_CODE_TEST_2);
            app_code_to_validate = pairing_security_get_app_code();
            THEN( "the local variable of app code is set properly" ) {
                REQUIRE( app_code_to_validate == PAIRING_APP_CODE_TEST_2 );
            }
        }
    }
}

SCENARIO( "pairing_security_compare_app_code() : validates the comparison", "[pairing]" ) {
    pairing_security_init();

    GIVEN( "the app code that is currently configured" ) {
        pairing_security_set_app_code(PAIRING_APP_CODE_TEST_1);
        WHEN( "calling the function" ) {
            THEN( "the comparator return true" ) {
                REQUIRE( pairing_security_compare_app_code(PAIRING_APP_CODE_TEST_1) );
            }
        }
    }

    GIVEN( "the app code that is not currently configured" ) {
        pairing_security_set_app_code(PAIRING_APP_CODE_TEST_1);
        WHEN( "calling the function" ) {
            THEN( "the comparator return false" ) {
                REQUIRE( !pairing_security_compare_app_code(PAIRING_APP_CODE_TEST_2) );
            }
        }
    }
}
