/** @file  pairing_security.c
 *  @brief This file handles the security features for the pairing procedure.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "pairing_security.h"

/* PRIVATE GLOBALS ************************************************************/
static uint64_t local_pairing_app_code;

/* PUBLIC FUNCTIONS ***********************************************************/
void pairing_security_init(void)
{
    local_pairing_app_code = PAIRING_APP_CODE_DEFAULT;
}

void pairing_security_set_app_code(uint64_t app_code)
{
    local_pairing_app_code = app_code;
}

uint64_t pairing_security_get_app_code(void)
{
    return local_pairing_app_code;
}

bool pairing_security_compare_app_code(uint64_t app_code)
{
    return (local_pairing_app_code == app_code ? true : false);
}
