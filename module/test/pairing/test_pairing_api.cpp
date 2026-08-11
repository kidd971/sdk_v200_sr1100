/** @file  test_pairing_api.cpp
 *  @brief Unit tests for the Pairing API module.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team
 */

/* INCLUDES *******************************************************************/
#include "catch2/catch_all.hpp"
#include "pairing_api.h"
#include "test_utils.h"

/* CONSTANTS ******************************************************************/
#define APP_CODE    0x12345678
#define TIMEOUT_SEC 10

/* UNIT TESTS *****************************************************************/
static void context_switch_callback(void)
{
    return;
}

SCENARIO("pairing_coordinator_start() is called from an application", "[pairing]")
{
    GIVEN("All pairing configuration are initialized")
    {
        pairing_event_t pairing_event = PAIRING_EVENT_NONE;

        pairing_cfg_t pairing_cfg = {
            .app_code = APP_CODE,
            .timeout_sec = TIMEOUT_SEC,
            .context_switch_callback = context_switch_callback,
        };

        pairing_assigned_address_t pairing_assigned_address = {};
        uint8_t pairing_discovery_list_size = 2;

        pairing_discovery_list_t pairing_discovery_list[pairing_discovery_list_size];

        pairing_error_t pairing_err = PAIRING_ERR_NONE;

        WHEN("starting the pairing coordinator with pairing_cfg set to NULL")
        {
            pairing_event = pairing_coordinator_start(nullptr, &pairing_assigned_address, pairing_discovery_list,
                                                      pairing_discovery_list_size, &pairing_err);

            THEN("a PAIRING_ERR_NULL_PTR error is returned")
            {
                REQUIRE(pairing_err == PAIRING_ERR_NULL_PTR);
            }
        }

        WHEN("starting the pairing coordinator with app_code set to 0")
        {
            pairing_cfg.app_code = 0;
            pairing_event = pairing_coordinator_start(&pairing_cfg, &pairing_assigned_address, pairing_discovery_list,
                                                      pairing_discovery_list_size, &pairing_err);

            THEN("a PAIRING_ERR_APP_CODE_NOT_CONFIGURED error is returned")
            {
                REQUIRE(pairing_err == PAIRING_ERR_APP_CODE_NOT_CONFIGURED);
            }
        }

        WHEN("starting the pairing coordinator with timeout_sec set to 0")
        {
            pairing_cfg.timeout_sec = 0;
            pairing_event = pairing_coordinator_start(&pairing_cfg, &pairing_assigned_address, pairing_discovery_list,
                                                      pairing_discovery_list_size, &pairing_err);

            THEN("a PAIRING_ERR_TIMEOUT error is returned")
            {
                REQUIRE(pairing_err == PAIRING_ERR_TIMEOUT);
            }
        }

        WHEN("starting the pairing coordinator with pairing_assigned_address set to NULL")
        {
            pairing_event = pairing_coordinator_start(&pairing_cfg, nullptr, pairing_discovery_list,
                                                      pairing_discovery_list_size, &pairing_err);

            THEN("a PAIRING_ERR_NULL_PTR error is returned")
            {
                REQUIRE(pairing_err == PAIRING_ERR_NULL_PTR);
            }
        }

        WHEN("starting the pairing coordinator with pairing_discovery_list set to NULL")
        {
            pairing_event = pairing_coordinator_start(&pairing_cfg, &pairing_assigned_address, nullptr,
                                                      pairing_discovery_list_size, &pairing_err);

            THEN("a PAIRING_ERR_NULL_PTR error is returned")
            {
                REQUIRE(pairing_err == PAIRING_ERR_NULL_PTR);
            }
        }

        WHEN("starting the pairing coordinator with pairing_discovery_list_size set to 1")
        {
            pairing_discovery_list_size = 1;
            pairing_event = pairing_coordinator_start(&pairing_cfg, &pairing_assigned_address, pairing_discovery_list,
                                                      pairing_discovery_list_size, &pairing_err);

            THEN("a PAIRING_ERR_DISCOVERY_LIST_SIZE_TOO_SMALL error is returned")
            {
                REQUIRE(pairing_err == PAIRING_ERR_DISCOVERY_LIST_SIZE_TOO_SMALL);
            }
        }

        WHEN("starting the pairing coordinator with context_switch_callback set to NULL")
        {
            pairing_cfg.context_switch_callback = NULL;
            pairing_event = pairing_coordinator_start(&pairing_cfg, &pairing_assigned_address, pairing_discovery_list,
                                                      pairing_discovery_list_size, &pairing_err);

            THEN("a PAIRING_ERR_NULL_PTR error is returned")
            {
                REQUIRE(pairing_err == PAIRING_ERR_NULL_PTR);
            }
        }
    }
}

SCENARIO("pairing_node_start() is called from an application", "[pairing]")
{

    GIVEN("All pairing configuration are initialized")
    {
        pairing_event_t pairing_event = PAIRING_EVENT_NONE;
        pairing_assigned_address_t pairing_assigned_address = {};
        uint8_t device_role = 1;
        pairing_error_t pairing_err = PAIRING_ERR_NONE;

        pairing_cfg_t pairing_cfg = {
            .app_code = APP_CODE,
            .timeout_sec = TIMEOUT_SEC,
            .context_switch_callback = context_switch_callback,
        };

        WHEN("starting the pairing node with pairing_cfg set to NULL")
        {
            pairing_event = pairing_node_start(nullptr, &pairing_assigned_address, device_role, &pairing_err);

            THEN("a PAIRING_ERR_NULL_PTR error is returned")
            {
                REQUIRE(pairing_err == PAIRING_ERR_NULL_PTR);
            }
        }

        WHEN("starting the pairing node with app_code set to 0")
        {
            pairing_cfg.app_code = 0;
            pairing_event = pairing_node_start(&pairing_cfg, &pairing_assigned_address, device_role, &pairing_err);

            THEN("a PAIRING_ERR_APP_CODE_NOT_CONFIGURED error is returned")
            {
                REQUIRE(pairing_err == PAIRING_ERR_APP_CODE_NOT_CONFIGURED);
            }
        }

        WHEN("starting the pairing node with timeout_sec set to 0")
        {
            pairing_cfg.timeout_sec = 0;
            pairing_event = pairing_node_start(&pairing_cfg, &pairing_assigned_address, device_role, &pairing_err);

            THEN("a PAIRING_ERR_TIMEOUT error is returned")
            {
                REQUIRE(pairing_err == PAIRING_ERR_TIMEOUT);
            }
        }

        WHEN("starting the pairing node with pairing_assigned_address set to NULL")
        {
            pairing_event = pairing_node_start(&pairing_cfg, nullptr, device_role, &pairing_err);

            THEN("a PAIRING_ERR_NULL_PTR error is returned")
            {
                REQUIRE(pairing_err == PAIRING_ERR_NULL_PTR);
            }
        }

        WHEN("starting the pairing node with device_role set to 0")
        {
            device_role = 0;
            pairing_event = pairing_node_start(&pairing_cfg, &pairing_assigned_address, device_role, &pairing_err);

            THEN("a PAIRING_ERR_DEVICE_ROLE error is returned")
            {
                REQUIRE(pairing_err == PAIRING_ERR_DEVICE_ROLE);
            }
        }
    }
}
