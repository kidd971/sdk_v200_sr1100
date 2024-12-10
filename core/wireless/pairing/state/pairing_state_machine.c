/** @file  pairing_state_machine.c
 *  @brief This file contains the functions to use a state machine.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "pairing_state_machine.h"

/* PRIVATE GLOBALS ************************************************************/
pairing_state_machine_t *local_state_machine;
uint8_t local_state_machine_size;

/* PUBLIC FUNCTIONS ***********************************************************/
void pairing_state_machine_init(pairing_state_machine_t *state_machine, uint8_t state_machine_size)
{
    local_state_machine = state_machine;
    local_state_machine_size = state_machine_size;
}

pairing_state_machine_t *pairing_state_machine_get_instance(void)
{
    return local_state_machine;
}

uint8_t pairing_state_machine_get_size(void)
{
    return local_state_machine_size;
}

void pairing_state_machine_execute_state(uint8_t state)
{
    uint8_t i = 0;
    bool found = false;

    do {
        if (local_state_machine[i].state == state) {
            found = true;
            if (local_state_machine[i].state_machine_callback != NULL) {
                local_state_machine[i].state_machine_callback();
            }
        }
        i++;
    } while (!found && i < local_state_machine_size);
}
