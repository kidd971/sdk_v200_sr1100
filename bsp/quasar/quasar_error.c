/** @file  quasar_error.c
 *  @brief Quasar BSP Error handling module.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_error.h"

/* PUBLIC FUNCTIONS ***********************************************************/
__attribute__((weak)) void quasar_bsp_error_handler(quasar_bsp_status_t quasar_status)
{
    (void)quasar_status;

    while (1);
}

__attribute__((weak)) void quasar_bsp_warning_handler(quasar_bsp_status_t quasar_status)
{
    (void)quasar_status;
}
