/** @file  bsp_validator_facade.h
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
#ifndef BSP_VALIDATOR_FACADE_H_
#define BSP_VALIDATOR_FACADE_H_

/* INCLUDES *******************************************************************/
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "swc_hal_facade.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Hardware Abstraction Layer for Spark Radio.
 *
 *  Provides an interface for controlling SPI communication, managing chip select (CS) pins,
 *  handling data transfer in both blocking and non-blocking modes, and managing IRQ and DMA
 *  interrupt sources for a Spark Radio device. This abstraction facilitates flexible integration
 *  with different hardware platforms and enhances portability by decoupling the radio operation
 *  specifics from the main application logic.
 *
 *  Functions:
 *  - set_cs: Set the CS pin high.
 *  - reset_cs: Set the CS pin low.
 *  - transfer_full_duplex_blocking: Perform SPI transfer in full duplex blocking mode.
 *  - transfer_full_duplex_non_blocking: Perform SPI transfer in full duplex non-blocking mode using DMA.
 *  - radio_context_switch: Trigger the radio's IRQ pin interrupt context.
 *  - disable_radio_irq: Disable the radio IRQ interrupt source.
 *  - enable_radio_irq: Enable the radio IRQ interrupt source.
 *  - disable_radio_dma_irq: Disable the SPI DMA interrupt source.
 *  - enable_radio_dma_irq: Enable the SPI DMA interrupt source.
 *
 *  This structure should be initialized statically, pointing to the appropriate subset of facade
 *  functions that implement the specified operations, allowing for tailored behavior based on
 *  the specific radio and platform in use.
 */
typedef struct {
    /*! Set reset pin HIGH */
    void (*set_reset_pin)(void);
    /*! Set reset pin LOW */
    void (*reset_reset_pin)(void);
    /*!< Set CS pin HIGH */
    void (*set_cs)(void);
    /*!< Set CS pin LOW */
    void (*reset_cs)(void);
    /*!< SPI Transfer full duplex in blocking mode */
    void (*transfer_full_duplex_blocking)(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);
    /*!< SPI Transfer full duplex in non-blocking mode using DMA */
    void (*transfer_full_duplex_non_blocking)(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);
    /*! Check if the status of the busy flag in the SPI Status Register */
    bool (*is_spi_busy)(void);
    /*! Return IRQ pin state. 0 (LOW), 1(HIGH) */
    bool (*read_irq_pin)(void);
    /*!< Trigger the radio IRQ context */
    void (*radio_context_switch)(void);
    /*!< Disable radio IRQ interrupt source */
    void (*disable_radio_irq)(void);
    /*!< Enable radio IRQ interrupt source */
    void (*enable_radio_irq)(void);
    /*!< Disable SPI DMA interrupt source */
    void (*disable_radio_dma_irq)(void);
    /*!< Enable SPI DMA interrupt source */
    void (*enable_radio_dma_irq)(void);
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
 *  @param[in] ms  miliseconds delay.
 */
void facade_time_delay(uint32_t ms);

/** @brief Serial output for the log feature.
 *
 *  User can provide serial IO such as UART.
 *
 *  @param[in] string  Message to be printed to the serial output.
 */
void facade_log_io(char *string);

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

#ifdef __cplusplus
}
#endif

#endif /* BSP_VALIDATOR_FACADE_H_ */
