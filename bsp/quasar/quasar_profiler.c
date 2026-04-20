/** @file  quasar_profiler.c
 *  @brief Implementation of the Quasar profiler for measuring execution time.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_profiler.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void quasar_profiler_init(void)
{
    QUASAR_SET_BIT(CoreDebug->DEMCR, CoreDebug_DEMCR_TRCENA_Msk);
    QUASAR_SET_BIT(ITM->TCR, ITM_TCR_DWTENA_Msk);
    QUASAR_SET_BIT(DWT->CTRL, DWT_CTRL_CYCCNTENA_Msk);
}
