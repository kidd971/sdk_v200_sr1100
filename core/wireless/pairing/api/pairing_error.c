/** @file  pairing_error.c
 *  @brief This file provides helper functions to manage the error state of the pairing module.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "pairing_error.h"

/* PRIVATE GLOBALS ************************************************************/
static pairing_error_t *local_pairing_error;

/* PUBLIC FUNCTIONS ***********************************************************/
void pairing_error_init(pairing_error_t *pairing_error)
{
    local_pairing_error = pairing_error;
}

void pairing_error_set_error(pairing_error_t pairing_error)
{
    if (local_pairing_error != NULL) {
        *local_pairing_error = pairing_error;
    }
}

pairing_error_t pairing_error_get_error(void)
{
    return *local_pairing_error;
}
