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
 *  @copyright Copyright (C) 2025 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef PROFILER_FACADE_H_
#define PROFILER_FACADE_H_

/* INCLUDES *******************************************************************/
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MACROS *********************************************************************/
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Initialize hardware drivers in the underlying board support package.
 */
void facade_board_init(void);

/** @brief Blocking delay with a 1ms resolution.
 *
 *  @param[in] ms_delay  Delay in milliseconds.
 */
void facade_delay(uint32_t ms_delay);

/** @brief Initializes the logging interface.
 *
 *  This function configures the communication interface used for logging, which may be UART, USB, or another medium.
 */
void facade_log_init(void);

 /** @brief Transmits a log message using the configured communication interface.
  *
  *  This function sends a string over the selected logging interface. The entire log message must be handled within
  *  this function’s context. In other words, the function must ensure that the string is either fully printed
  *  (e.g., using blocking transmission) or safely stored (e.g., using an intermediate buffer).
  *
  *   @param[in] string  String to be transmitted.
  */
void facade_log_write(char *string);

/** @brief Triggers a software interrupt for context switching in a bare-metal environment.
 *
 *  @note This function is designed to be used as a callback for the wireless core's context switch mechanism. It
 *  configures and triggers a software interrupt specifically allocated for context switching purposes. The interrupt
 *  invoked by this function should be set with the lowest priority to ensure that it does not preempt more critical
 *  system operations.
 *
 *  In ARM Cortex-M systems, this function could triggers the PendSV interrupt, which is used to perform the context
 *  switch by setting the PendSV interrupt pending bit. The actual context switching logic, including saving and
 *  restoring of contexts, is handled by the interrupt service routine (ISR) associated with the software interrupt,
 *  which should invoke `swc_connection_callbacks_processing_handler` as part of its execution.
 *
 *  Usage:
 *  This function should be registered with `swc_init` as part of the initialization process for applications that
 *  require custom context switching mechanisms, allowing the wireless core to manage task priorities and execute less
 *  critical processes seamlessly.
 */
void facade_context_switch_trigger(void);

/** @brief Registers a callback function to be invoked by the context switch IRQ handler.
 *
 *  @note The primary use case involves registering the `swc_connection_callbacks_processing_handler` provided by the
 *  SWC API. This handler is then called within the context switch IRQ handler.
 *
 *  Example usage:
 *  @code
 *  int main(void) {
 *      // Register SWC API function to be invoked within the context switch associated IRQ handler
 *      facade_set_context_switch_handler(swc_connection_callbacks_processing_handler);
 *      // Further initialization and application code follows
 *  }
 *  @endcode
 *
 *  @param[in] callback  Function pointer to the user-defined callback.
 */
void facade_set_context_switch_handler(void (*callback)(void));

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
  *  This function sets up the profiling mechanism, allowing time measurements between `facade_profiler_start` and
  *  `facade_profiler_stop`.
  */
void facade_profiler_init(void);

 /** @brief Starts a profiling measurement.
  *
  *  This function marks the beginning of a time measurement. The elapsed time can be retrieved by calling
  *  `facade_profiler_stop`.
  */
void facade_profiler_start(void);

/** @brief Stops a profiling measurement and returns the elapsed time in nanoseconds.
 *
 *  This function marks the end of a time measurement and returns the elapsed time in nanoseconds since the last call to
 *  `facade_profiler_start`.
 *
 *  @return Elapsed time in nanoseconds since the last `facade_profiler_start` call.
 */
uint64_t facade_profiler_stop(void);

/** @brief Notify user of the wireless TX connection status.
 */
void facade_tx_conn_status(void);

/** @brief Notify user of the wireless RX connection status.
 */
void facade_rx_conn_status(void);

#ifdef __cplusplus
}
#endif

#endif /* PROFILER_FACADE_H_ */
