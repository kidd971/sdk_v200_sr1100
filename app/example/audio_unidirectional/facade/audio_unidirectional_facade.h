/** @file  audio_unidirectional_facade.h
 *  @brief Facades for low-level platform-specific features required by the application example.
 *
 *  @note This header defines the interfaces for various hardware features used by
 *  the audio unidirectional example. These facades abstract the underlying
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
#ifndef AUDIO_UNIDIRECTIONAL_FACADE_H_
#define AUDIO_UNIDIRECTIONAL_FACADE_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/** @brief Certifications modes.
 */
typedef enum facade_certification_mode {
    FACADE_CERTIF_NONE,
    FACADE_CERTIF_AUDIO_24_BIT,
    FACADE_CERTIF_AUDIO_16_BIT,
    FACADE_CERTIF_DATA,
} facade_certification_mode_t;

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

/** @brief Initialize the Coordinator's audio peripherals.
 *
 *  @note Configure the serial audio interface to MONO or Stereo.
 */
void facade_audio_coord_init(void);

/** @brief Initialize the Node's audio peripherals.
 *
 *  @note Configure the serial audio interface to MONO or Stereo.
 */
void facade_audio_node_init(void);

/** @brief Deinitialize the audio peripherals.
 */
void facade_audio_deinit(void);

/** @brief Set the serial audio interface transfer complete callbacks.
 *
 *  @note Set NULL in place of unused callback.
 *
 *  @param tx_callback  Audio i2s tx complete callback
 *  @param rx_callback  Audio i2s rx complete callback
 */
void facade_set_sai_complete_callback(void (*tx_callback)(void), void (*rx_callback)(void));

/** @brief Read button 2 state to define if certification mode is required for coord.
 *
 *  @return The certification mode to be applied.
 */
facade_certification_mode_t facade_get_coord_certification_mode(void);

/** @brief Read button 2 state to define if certification mode is required for node.
 *
 *  @return The certification mode to be applied.
 */
facade_certification_mode_t facade_get_node_certification_mode(void);

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

/** @brief Notify user of the wireless Audio TX connection status.
 *
 *  @note This function is intended only for the Coordinator, which is responsible for sending audio packets.
 */
void facade_tx_audio_conn_status(void);

/** @brief Notify user of the wireless Data TX connection status.
 */
void facade_tx_data_conn_status(void);

/** @brief Notify user of the wireless Audio RX connection status.
 *
 *  @note This function is intended only for the Node, which is responsible for receiving audio packets.
 */
void facade_rx_audio_conn_status(void);

/** @brief Notify user of the wireless Data RX connection status.
 */
void facade_rx_data_conn_status(void);

/** @brief Notify user of the fallback status.
 */
void facade_fallback_status(bool on);

/** @brief Initialize and set the audio process timer period.
 *
 *  @param[in] period_us  Timer period in us.
 */
void facade_audio_process_timer_init(uint32_t period_us);

/** @brief Set the audio process timer callback.
 *
 *  @param[in] callback  Callback when timer expires.
 */
void facade_audio_process_set_timer_callback(void (*callback)(void));

/** @brief Start the audio process timer.
 */
void facade_audio_process_timer_start(void);

/** @brief Stop the audio process timer.
 */
void facade_audio_process_timer_stop(void);

/** @brief Initialize and set the data timer period which include statistics and data transmitted to the other device.
 *
 *  @param[in] period_ms  Timer period in ms.
 */
void facade_data_timer_init(uint32_t period_ms);

/** @brief Set the data timer callback.
 *
 *  @param[in] callback  Callback when timer expires.
 */
void facade_data_timer_set_callback(void (*callback)(void));

/** @brief Start the data timer.
 */
void facade_data_timer_start(void);

/** @brief Stop the data timer.
 */
void facade_data_timer_stop(void);

/** @brief Print string.
 *
 *  @param[in] string  Null terminated string to print.
 */
void facade_print_string(char *string);

/** @brief Notify user of payload present in frame.
 */
void facade_payload_received_status(void);

/** @brief Notify user of no payload present in frame.
 */
void facade_empty_payload_received_status(void);

/** @brief Notify the user that the device is entering into pairing proces.
 */
void facade_notify_enter_pairing(void);

/** @brief Notify the user that the device is not paired.
 */
void facade_notify_not_paired(void);

/** @brief Notify the user that the pairing is successfully finished and the device is paired.
 */
void facade_notify_pairing_successful(void);

/** @brief Turn off all LEDs.
 */
void facade_led_all_off(void);

/** @brief Get the current system tick value in milliseconds.
 *
 *  @return The current millisecond system tick value.
 */
uint32_t facade_get_tick_ms(void);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_UNIDIRECTIONAL_FACADE_H_ */
