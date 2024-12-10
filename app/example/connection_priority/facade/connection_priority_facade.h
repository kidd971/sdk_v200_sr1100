/** @file  connection_priority_facade.h
 *  @brief Facades for low-level platform-specific features required by the application example.
 *
 *  @note This header defines the interfaces for various hardware features used by
 *  the connection priority example. These facades abstract the underlying
 *  platform-specific implementations of features like SPI communication,
 *  IRQ handling, timer functions, and context switching mechanisms. The actual
 *  implementations are selected at compile time based on the target platform,
 *  allowing for flexibility and portability across different hardware.
 *
 *  The facade is designed to be a compile-time dependency only, with no
 *  support for runtime polymorphism. This ensures tight integration with the
 *  build system and minimal overhead.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef CONNECTION_PRIORITY_H_
#define CONNECTION_PRIORITY_H_

/* INCLUDES *******************************************************************/
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MACROS *********************************************************************/
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))

/* PUBLIC FUNCTIONS ***********************************************************/

/** @brief Triggers a software interrupt for context switching in a bare-metal environment.
 *
 *  @note This function is designed to be used as a callback for the wireless core's context switch mechanism.
 *  It configures and triggers a software interrupt specifically allocated for context switching purposes.
 *  The interrupt invoked by this function should be set with the lowest priority to ensure that it does
 *  not preempt more critical system operations.
 *
 *  In ARM Cortex-M systems, this function could triggers the PendSV interrupt, which is used
 *  to perform the context switch by setting the PendSV interrupt pending bit. The actual context
 *  switching logic, including saving and restoring of contexts, is handled by the interrupt service
 *  routine (ISR) associated with the software interrupt, which should invoke
 *  `swc_connection_callbacks_processing_handler` as part of its execution.
 *
 *  Usage:
 *  This function should be registered with `swc_register_context_switch_trigger` as part of the
 *  initialization process for applications that require custom context switching mechanisms,
 *  allowing the wireless core to manage task priorities and execute less critical processes seamlessly.
 *
 *  @see swc_register_context_switch_trigger
 */
void facade_context_switch_trigger(void);

/**
 *  @brief Registers a callback function to be invoked by the context switch IRQ handler.
 *
 *  @note The primary use case involves registering the `swc_connection_callbacks_processing_handler`
 *  provided by the SWC API. This handler is then called within the context switch IRQ handler.
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

/** @brief Initialize hardware drivers in the underlying board support package.
 */
void facade_board_init(void);

/** @brief Poll for button presses.
 *
 *  @note Set NULL in place of unused callback.
 *
 *  @param[in] button1_callback  Function to execute when pressing button #1.
 *  @param[in] button2_callback  Function to execute when pressing button #2.
 *  @param[in] button3_callback  Function to execute when pressing button #3.
 *  @param[in] button4_callback  Function to execute when pressing button #4.
 */
void facade_button_handling(void (*button1_callback)(void), void (*button2_callback)(void),
                            void (*button3_callback)(void), void (*button4_callback)(void));

/** @brief Initialize and set the timer 1 period.
 *
 *  @param[in] period_us  Timer period in microseconds.
 */
void facade_packet_rate_timer1_init(uint32_t period_us);

/** @brief Set the timer 1 callback.
 *
 *  @param[in] callback  Callback when timer expires.
 */
void facade_packet_rate_set_timer1_callback(void (*callback)(void));

/** @brief Start the timer 1.
 */
void facade_packet_rate_timer1_start(void);

/** @brief Stop the timer 1.
 */
void facade_packet_rate_timer1_stop(void);

/** @brief Initialize and set the timer 2 period.
 *
 *  @param[in] period_us  Timer period in microseconds.
 */
void facade_packet_rate_timer2_init(uint32_t period_us);

/** @brief Set the timer 2 callback.
 *
 *  @param[in] callback  Callback when timer expires.
 */
void facade_packet_rate_set_timer2_callback(void (*callback)(void));

/** @brief Start the timer 2.
 */
void facade_packet_rate_timer2_start(void);

/** @brief Stop the timer 2.
 */
void facade_packet_rate_timer2_stop(void);

/** @brief Initialize and set the stats timer period.
 *
 *  @param[in] period_ms  Timer period in milliseconds.
 */
void facade_stats_timer_init(uint32_t period_ms);

/** @brief Set the stats timer callback.
 *
 *  @param[in] callback  Callback when timer expires.
 */
void facade_stats_set_timer_callback(void (*callback)(void));

/** @brief Start the stats timer.
 */
void facade_stats_timer_start(void);

/** @brief Print string.
 *
 *  @param[in] string  Null terminated string to print.
 */
void facade_print_string(char *string);

/** @brief Enter pairing notification LED pattern.
 */
void facade_notify_enter_pairing(void);

/** @brief Not paired notification LED pattern.
 */
void facade_notify_not_paired(void);

/** @brief Successful pairing notification LED pattern.
 */
void facade_notify_pairing_successful(void);

#ifdef __cplusplus
}
#endif

#endif /* CONNECTION_PRIORITY_H_ */
