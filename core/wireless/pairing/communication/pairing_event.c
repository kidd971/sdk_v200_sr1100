/** @file  pairing_event.c
 *  @brief This file provides helper functions to manage the event state of the pairing module.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "pairing_event.h"

/* PRIVATE GLOBALS ************************************************************/
static pairing_event_t local_pairing_event;

/* PUBLIC FUNCTIONS ***********************************************************/
void pairing_event_init(void)
{
    local_pairing_event = PAIRING_EVENT_NONE;
}

void pairing_event_set_event(pairing_event_t pairing_event)
{
    local_pairing_event = pairing_event;
}

pairing_event_t pairing_event_get_event(void)
{
    return local_pairing_event;
}
