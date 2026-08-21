/** @file  common_facade.h
 *  @brief Common facade functions for low-level platform-specific features required by the application example.
 *
 *  @note This header defines the common interfaces for various hardware features used by the all the examples.
 *
 *  These facades abstract the underlying platform-specific implementations of features like SPI/QSPI communication, IRQ
 *  handling, timer functions, and context switching mechanisms. The actual implementations are selected at compile time
 *  based on the target platform, allowing for flexibility and portability across different hardware. The facade is
 *  designed to be a compile-time dependency only, with no support for runtime polymorphism. This ensures tight
 *  integration with the build system and minimal overhead.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

#ifndef COMMON_FACADE_H_
#define COMMON_FACADE_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MACROS *********************************************************************/
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Triggers a software interrupt for context switching in a bare-metal environment.
 *
 *  @note This function is designed to be used as a callback for the wireless core's context switch mechanism. It
 *        configures and triggers a software interrupt specifically allocated for context switching purposes. The
 *        interrupt invoked by this function should be set with the lowest priority to ensure that it does not preempt
 *        more critical system operations.
 *
 *  @note In ARM Cortex-M systems, this function could triggers the PendSV interrupt, which is used to perform the
 *        context switch by setting the PendSV interrupt pending bit. The actual context switching logic, including
 *        saving and restoring of contexts, is handled by the interrupt service routine (ISR) associated with the
 *        software interrupt, which should invoke `swc_connection_callbacks_processing_handler` as part of its
 *        execution.
 *
 *  @note Usage: This function should be registered with `swc_init` as part of the initialization process for
 *        applications that require custom context switching mechanisms, allowing the wireless core to manage task
 *        priorities and execute less critical processes seamlessly.
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

/** @brief Initialize hardware drivers in the underlying board support package.
 */
void facade_board_init(void);

/** @brief Notify user of the wireless TX connection status.
 */
void facade_tx_conn_status(void);

/** @brief Notify user of the wireless RX connection status.
 */
void facade_rx_conn_status(void);

/** @brief Blocking delay with a 1ms resolution.
 *
 *  @param[in] ms_delay  Delay in milliseconds to wait.
 */
void facade_delay(uint32_t ms_delay);

/** @brief Print a string of characters.
 *
 *  @param[in] string  Null terminated string to print.
 */
void facade_print_string(char *string);

/** @brief Report whether the USB CDC connection is active.
 *
 *  @retval true   USB CDC is connected.
 *  @retval false  USB CDC is not connected.
 */
bool facade_is_usb_connected(void);

/** @brief Print error string.
 *
 *  @note The print mechanism must be able to work at the highest priority level where error checks are done.
 *
 *  @param[in] string  Null terminated string to print.
 */
void facade_print_error_string(char *string);

/** @brief Enter pairing notification LED pattern.
 */
void facade_notify_enter_pairing(void);

/** @brief Reconnecting notification LED pattern (boot auto-reconnect).
 *
 *  Fast blink x5 at 100 ms on the board status colour: blue on u535, green elsewhere.
 *
 *  The pattern has to be told apart from enter-pairing, which is a SLOW 250 ms x2 on the
 *  same LED in the same colour. That is the whole point of it: the two are the only things
 *  that happen at boot, they look similar from across a bench, and confusing them means
 *  believing a device is pairing when it is silently restoring a link it already had.
 *  Fast-and-five against slow-and-two is legible without counting.
 *
 *  Blocking, and deliberately so -- one second at the boot reconnect transition, never in
 *  the audio loop.
 */
void facade_notify_reconnecting(void);

/** @brief Not paired notification LED pattern.
 */
void facade_notify_not_paired(void);

/** @brief Successful pairing notification LED pattern.
 */
void facade_notify_pairing_successful(void);

/** @brief Display the certification mode indicator.
 *
 *  @note Shows a steady indicator to signal the device is running in certification mode. On boards with an RGB LED, the
 *        indicator is a steady yellow. On boards without an RGB LED, a dedicated status LED is used instead. While
 *        active, all activity status LEDs are suppressed so the certification indicator stays unambiguous.
 */
void facade_notify_certification_mode(void);

/** @brief Report whether the certification mode indicator is currently active.
 *
 *  @note Used by the board backends to suppress activity status LEDs while the certification indicator is shown.
 *
 *  @retval true   Certification mode is active.
 *  @retval false  Certification mode is not active.
 */
bool facade_is_certification_mode_active(void);

/** @brief Turn off all LEDs.
 */
void facade_led_all_off(void);

/** @brief Get the current system tick value in milliseconds.
 *
 *  @return The current millisecond system tick value.
 */
uint32_t facade_get_tick_ms(void);

/** @brief Suspend the CPU until an interrupt event occurs.
 */
void facade_wait_for_interrupt(void);

#ifdef __cplusplus
}
#endif

#endif /* COMMON_FACADE_H_ */
