/** @file  profiler_facade.h
 *  @brief Facades for low-level platform-specific features required by the profiler tool.
 *
 *  @note This header defines the interfaces for various hardware features used by the profiler tool. These facades
 *  abstract the underlying platform-specific implementations of features like SPI communication, IRQ handling, timer
 *  functions, and context switching mechanisms. The actual implementations are selected at compile time based on the
 *  target platform, allowing for flexibility and portability across different hardware.
 *
 *  The facade is designed to be a compile-time dependency only, with no support for runtime polymorphism. This ensures
 *  tight integration with the build system and minimal overhead.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef PROFILER_FACADE_H_
#define PROFILER_FACADE_H_

/* INCLUDES *******************************************************************/
#include <stdint.h>
#include "common_facade.h"

#ifdef __cplusplus
extern "C" {
#endif

/* MACROS *********************************************************************/
/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Initializes the logging interface.
 *
 *  This function configures the communication interface used for logging, which may be UART, USB, or another medium.
 */
void facade_log_init(void);

/** @brief Transmits a log message using the configured communication interface.
 *
 *  @note This function sends a string over the selected logging interface. The entire log message must be handled
 *        within this function’s context. In other words, the function must ensure that the string is either fully
 *        printed (e.g., using blocking transmission) or safely stored (e.g., using an intermediate buffer).
 *
 *  @param[in] string  String to be transmitted.
 */
void facade_log_write(char *string);

/** @brief Print error string.
 *
 *  @note The log mechanism must be able to work at the highest priority level where error checks are done.
 *
 *  @param[in] string  Null terminated string to log.
 */
void facade_log_error_string(char *string);

/** @brief Initialize packet generation timer peripheral.
 *
 *  @note The packet generation timer priority should be lower than the wireless core context switch priority to avoid
 *        interruptions during measurement.
 *
 *  @param[in] period  Timer interrupt period.
 */
void facade_packet_generation_timer_init(uint32_t period);

/** @brief This function sets the function callback for the packet generation timer IRQ.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void facade_packet_generation_set_timer_callback(void (*irq_callback)(void));

/** @brief Start packet generation timer peripheral.
 */
void facade_packet_generation_timer_start(void);

/** @brief Stop packet generation timer peripheral.
 */
void facade_packet_generation_timer_stop(void);

/** @brief Initializes the profiling system.
 *
 *  @note This function sets up the profiling mechanism, allowing time measurements between
 *        `facade_profiler_start` and `facade_profiler_stop`.
 */
void facade_profiler_init(void);

/** @brief Starts a profiling measurement.
 *
 *  @note This function marks the beginning of a time measurement. The elapsed number of cycles can be
 *        retrieved by calling `facade_profiler_stop`.
 *
 *  @param[out] timestamp_start_handle  An optional pointer to a variable to save the start cycle count value to.
 */
void facade_profiler_start(uint32_t *timestamp_start_handle);

/** @brief Stops a profiling measurement and returns the elapsed time in nanoseconds.
 *
 *  @note This function marks the end of a time measurement and returns the number of cycles since
 *        the call to `facade_profiler_start`.
 *
 *  @param[in] timestamp_start_handle  An optional pointer to a variable containing the start cycle count value.
 *  @return Elapsed number of cycles since the last `facade_profiler_start` call.
 */
uint32_t facade_profiler_stop(uint32_t *timestamp_start_handle);

/** @brief Convert a given cycle count to microseconds.
 *
 *  @param[in] cycle_count  The cycle count to convert.
 *  @return The elapsed time in microseconds.
 */
double facade_profiler_get_elapsed_us(uint32_t cycle_count);

#ifdef __cplusplus
}
#endif

#endif /* PROFILER_FACADE_H_ */
