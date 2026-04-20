/** @file  quasar_profiler.h
 *  @brief Implementation of the Quasar profiler for measuring execution time.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_PROFILER_H_
#define QUASAR_PROFILER_H_

/* INCLUDES *******************************************************************/
#include <stdint.h>
#include "quasar_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Initializes the profiler by enabling the necessary debugging and cycle counting features.
 *
 *  @note This function configures the DWT and ITM for cycle counting.
 */
void quasar_profiler_init(void);

/** @brief Get the current clock cycle count.
 *
 *  @return The current clock cycle count.
 */
static inline uint32_t quasar_profiler_get_cycle_count(void)
{
    return DWT->CYCCNT;
}

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_PROFILER_H_ */
