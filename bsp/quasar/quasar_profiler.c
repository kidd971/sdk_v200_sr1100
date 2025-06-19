/** @file  quasar_profiler.c
 *  @brief Implementation of the Quasar profiler for measuring execution time.
 *
 *  @copyright Copyright (C) 2025 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_profiler.h"
#include "quasar_clock.h"

/* MACROS *********************************************************************/
#define HZ_TO_GHZ(hz) (((double)(hz))/((double)1e9))

/* PRIVATE GLOBALS ************************************************************/
static uint32_t timestamp_begin;
static double clock_ns_frequency_factor;

/* PUBLIC FUNCTIONS ***********************************************************/
void quasar_profiler_init(void)
{
    QUASAR_SET_BIT(CoreDebug->DEMCR, CoreDebug_DEMCR_TRCENA_Msk);
    QUASAR_SET_BIT(ITM->TCR, ITM_TCR_DWTENA_Msk);
    QUASAR_SET_BIT(DWT->CTRL, DWT_CTRL_CYCCNTENA_Msk);

    clock_ns_frequency_factor = HZ_TO_GHZ(quasar_clock_get_system_clock_freq());
}

void quasar_profiler_start(void)
{
    timestamp_begin = DWT->CYCCNT;
}

uint64_t quasar_profiler_stop(void)
{
    uint32_t timestamp_end = DWT->CYCCNT;
    uint64_t elapsed_time_ns = (timestamp_end - timestamp_begin) / clock_ns_frequency_factor;

    return elapsed_time_ns;
}
