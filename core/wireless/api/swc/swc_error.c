/** @file  swc_error.c
 *  @brief SWC Error handling module.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "swc_error.h"

/* PUBLIC FUNCTIONS ***********************************************************/
__attribute__((weak)) void swc_error_handler(swc_error_t swc_status)
{
    (void)swc_status;

    while (1);
}

__attribute__((weak)) void swc_warning_handler(swc_error_t swc_status)
{
    (void)swc_status;
}
