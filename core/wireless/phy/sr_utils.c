/** @file  sr_utils.c
 *  @brief File containing functions to link the free-running timer functions to the HAL utility instances used by the
 *         wireless core of the Spark chip.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sr_utils.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC GLOBALS *************************************************************/
sr_utils_hal_free_running_timer_t sr_utils_hal_free_running_timer;

/* PUBLIC FUNCTIONS ***********************************************************/
bool sr_utils_free_runing_timer_link(uint32_t (*get_free_running_timer_frequency_hz)(void),
                                     uint64_t (*get_tick_free_running_timer)(void))
{
    if ((get_free_running_timer_frequency_hz == NULL) || (get_tick_free_running_timer == NULL)) {
        return false;
    }

    /* Link the free running timer functions. */
    sr_utils_hal_free_running_timer.get_free_running_timer_frequency_hz = get_free_running_timer_frequency_hz;
    sr_utils_hal_free_running_timer.get_tick_free_running_timer = get_tick_free_running_timer;

    return true;
}
