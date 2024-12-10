/** @file  pairing_state.c
 *  @brief This file handles the functions related to the pairing states.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "pairing_state.h"
#include "pairing_state_coordinator.h"
#include "pairing_state_machine.h"
#include "pairing_state_node.h"

/* PRIVATE GLOBALS ************************************************************/
static pairing_state_t current_pairing_state;

/* PUBLIC FUNCTIONS ***********************************************************/
void pairing_state_init(swc_role_t swc_role)
{
    if (swc_role == SWC_ROLE_COORDINATOR) {
        pairing_state_coordinator_init();
    } else {
        pairing_state_node_init();
    }
}

void pairing_state_execute_current_state(void)
{
    pairing_state_machine_execute_state(current_pairing_state);
}

void pairing_state_set_current_state(pairing_state_t pairing_state)
{
    current_pairing_state = pairing_state;
}

pairing_state_t pairing_state_get_current_state(void)
{
    return current_pairing_state;
}
