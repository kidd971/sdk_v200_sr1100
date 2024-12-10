/** @file  hello_world_rtos_facade.h
 *  @brief Facades for low-level platform-specific features required by the application example.
 *
 *  @note This header defines the interfaces for various hardware features used by
 *  the hello-world-rtos example. These facades abstract the underlying
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
#ifndef HELLO_WORLD_RTOS_FACADE_H_
#define HELLO_WORLD_RTOS_FACADE_H_

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

#endif /* HELLO_WORLD_RTOS_FACADE_H_ */
