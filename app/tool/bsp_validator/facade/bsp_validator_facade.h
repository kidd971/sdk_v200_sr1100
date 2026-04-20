/** @file  bsp_validator_facade.h
 *  @brief Facades for low-level platform-specific features required by the application example.
 *
 *  @note This header defines the interfaces for various hardware features used by the BSP Validator example.
 *
 *  These facades abstract the underlying platform-specific implementations of features like SPI communication, IRQ
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
#ifndef BSP_VALIDATOR_FACADE_H_
#define BSP_VALIDATOR_FACADE_H_

/* INCLUDES *******************************************************************/
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common_facade.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Hardware Abstraction Layer for Spark Radio.
 *
 *  Provides an interface for controlling communication with the radio, managing chip select (CS) pins,
 *  handling data transfer in both blocking and non-blocking modes, and managing IRQ and DMA
 *  interrupt sources for a Spark Radio device. This abstraction facilitates flexible integration
 *  with different hardware platforms and enhances portability by decoupling the radio operation
 *  specifics from the main application logic.
 *
 *  Functions:
 *  - end_transfer: Set the CS pin high.
 *  - begin_transfer: Set the CS pin low.
 *  - transfer_half_duplex_rx_blocking: Transfer half duplex from the radio in blocking mode.
 *  - transfer_half_duplex_rx_non_blocking: Transfer half duplex from the radio in non-blocking mode.
 *  - transfer_half_duplex_tx_blocking: Transfer half duplex to the radio in blocking mode.
 *  - transfer_half_duplex_tx_non_blocking: Transfer half duplex to the radio in non-blocking mode.
 *  - transfer_full_duplex_blocking: Perform transfer in full duplex blocking mode.
 *  - transfer_full_duplex_non_blocking: Perform transfer in full duplex non-blocking mode.
 *  - radio_context_switch: Trigger the radio's IRQ pin interrupt context.
 *  - disable_radio_irq: Disable the radio IRQ interrupt source.
 *  - enable_radio_irq: Enable the radio IRQ interrupt source.
 *  - disable_radio_non_blocking_transfer_irq: Disable the non blocking transfer complete interrupt source.
 *  - enable_radio_non_blocking_transfer_irq: Enable the non blocking transfer complete interrupt source.
 *
 *  This structure should be initialized statically, pointing to the appropriate subset of facade
 *  functions that implement the specified operations, allowing for tailored behavior based on
 *  the specific radio and platform in use.
 */
typedef struct {
    /*! Set reset pin HIGH. */
    void (*set_reset_pin)(void);
    /*! Set reset pin LOW. */
    void (*reset_reset_pin)(void);
    /*! Manually end the transfer to the radio. */
    void (*end_transfer)(void);
    /*! Manually begin the transfer to the radio. */
    void (*begin_transfer)(void);
    /*! Transfer half duplex RX in blocking mode. */
    void (*transfer_half_duplex_rx_blocking)(uint8_t command, uint8_t *rx_data, uint16_t size);
    /*! Transfer half duplex TX in blocking mode. */
    void (*transfer_half_duplex_tx_blocking)(uint8_t command, uint8_t *tx_data, uint16_t size);
    /*! Transfer half duplex RX in non-blocking mode. */
    void (*transfer_half_duplex_rx_non_blocking)(uint8_t command, uint8_t *rx_data, uint16_t size);
    /*! Transfer half duplex TX in non-blocking mode. */
    void (*transfer_half_duplex_tx_non_blocking)(uint8_t command, uint8_t *tx_data, uint16_t size);
    /*! Transfer full duplex to the radio in blocking mode. */
    void (*transfer_full_duplex_blocking)(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);
    /*! Transfer full duplex to the radio in non-blocking mode using DMA. */
    void (*transfer_full_duplex_non_blocking)(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);
    /*! Set the access mode to be SPI. */
    void (*set_access_mode_spi)(void);
    /*! Set the access mode to be QSPI. */
    void (*set_access_mode_qspi)(void);
    /*! Check if the status of the busy flag in the SPI Status Register. */
    bool (*is_transfer_busy)(void);
    /*! Return IRQ pin state. 0 (LOW), 1(HIGH). */
    bool (*read_irq_pin)(void);
    /*! Trigger the radio IRQ context. */
    void (*radio_context_switch)(void);
    /*! Disable radio IRQ interrupt source. */
    void (*disable_radio_irq)(void);
    /*! Enable radio IRQ interrupt source. */
    void (*enable_radio_irq)(void);
    /*! Disable DMA or peripheral interrupt source. */
    void (*disable_radio_non_blocking_transfer_irq)(void);
    /*! Enable DMA or peripheral interrupt source. */
    void (*enable_radio_non_blocking_transfer_irq)(void);
} swc_hal_validator_t;

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Initialize the BSP main peripherals.
 *
 *  @note This function initializes peripherals such as: System clock, SPI, GPIO, UART, NVIC and timers,
 *        which are required by the functions within this BSP validator application.
 */
void facade_bsp_init(void);

/** @brief Initialize the board UART peripherals.
 */
void facade_uart_init(void);

/** @brief Waiting delay.
 *
 *  @param[in] ms  Miliseconds delay.
 */
void facade_time_delay(uint32_t ms);

/** @brief Serial output for the log feature.
 *
 *  User can provide serial IO such as UART.
 *
 *  @param[in] string  Message to be printed to the serial output.
 */
void facade_log_io(char *string);

#ifdef __cplusplus
}
#endif

#endif /* BSP_VALIDATOR_FACADE_H_ */
