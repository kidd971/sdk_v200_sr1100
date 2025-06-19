/** @file  quasar_profiler.h
 *  @brief Implementation of the Quasar profiler for measuring execution time.
 *
 *  @copyright Copyright (C) 2025 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_PROFILER_H_
#define QUASAR_PROFILER_H_

/* INCLUDES *******************************************************************/
#include "quasar_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Initializes the profiler by enabling the necessary debugging and cycle counting features.
 *
 * This function configures the DWT and ITM for cycle counting and retrieves the system clock frequency factor
 * for time measurements.
 */
void quasar_profiler_init(void);

/** @brief Starts the profiler by capturing the current cycle count timestamp.
 */
void quasar_profiler_start(void);

/** @brief Stops the profiler and calculates the elapsed time in nanoseconds.
 *
 *  @return The elapsed time in nanoseconds.
 */
uint64_t quasar_profiler_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_PROFILER_H_ */
