/** @file  sac_error.c
 *  @brief SAC Error handling module.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sac_error.h"

/* PUBLIC FUNCTIONS ***********************************************************/
__attribute__((weak)) void sac_error_handler(sac_status_t sac_status)
{
    (void)sac_status;

    while (1);
}

__attribute__((weak)) void sac_warning_handler(sac_status_t sac_status)
{
    (void)sac_status;
}
