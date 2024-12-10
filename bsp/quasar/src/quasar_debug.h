/** @file  quasar_debug.h
 *  @brief Debug module for Quasar BSP, providing debug IOs control and UART communication.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_DEBUG_IO_H_
#define QUASAR_DEBUG_IO_H_

/* INCLUDES *******************************************************************/
#include "quasar_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Quasar BSP debug IO peripherals selection.
 */
typedef enum quasar_debug_io_peripheral {
    /*! Expansion port IO 0 (PA4). */
    QUASAR_DEBUG_IO_1,
    /*! Expansion port IO 1 (PA5). */
    QUASAR_DEBUG_IO_2,
    /*! Expansion port IO 2 (PC3). */
    QUASAR_DEBUG_IO_3,
    /*! Expansion port IO 3 (PC5). */
    QUASAR_DEBUG_IO_4,
    /*! Expansion port IO 4 (PB1). */
    QUASAR_DEBUG_IO_5,
} quasar_debug_io_peripheral_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the debug UART (ST-Link) and debug IO peripherals.
 */
void quasar_debug_init(void);

/** @brief Deinitialize debug IO peripherals.
 */
void quasar_debug_deinit(void);

/** @brief Set debug IO peripheral.
 * .
 *  @param[in] quasar_debug_io_peripheral  Selected debug io peripheral.
 */
void quasar_debug_io_set(quasar_debug_io_peripheral_t quasar_debug_io_peripheral);

/** @brief Clear debug IO peripheral.
 *
 *  @param[in] quasar_debug_io_peripheral  Selected debug io peripheral.
 */
void quasar_debug_io_clear(quasar_debug_io_peripheral_t quasar_debug_io_peripheral);

/** @brief Toggle debug IO peripheral.
 *
 *  @param[in] quasar_debug_io_peripheral  Selected debug io peripheral.
 */
void quasar_debug_io_toggle(quasar_debug_io_peripheral_t quasar_debug_io_peripheral);

/** @brief Pulse debug IO peripheral.
 *
 *  @param[in] quasar_debug_io_peripheral  Selected debug io peripheral.
 */
void quasar_debug_io_pulse(quasar_debug_io_peripheral_t quasar_debug_io_peripheral);

/** @brief Transmit over debug UART using blocking method.
 *
 *  @note The UART protocol is set to 115200 baud, 8 data bits, no parity, and 1 stop bit (115200 8N1).
 *
 *  @param[in] data  Data to be transmitted.
 *  @param[in] size  Size of the data array to be transmitted.
 */
void quasar_debug_uart_transmit_blocking(uint8_t *data, uint16_t size);

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_DEBUG_IO_H_ */
