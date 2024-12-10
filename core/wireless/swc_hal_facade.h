/** @file  swc_hal_facade.h
 *
 *  @brief Facades for low-level platform-specific features required by SPARK Wireless Core.
 *
 *  @note This header defines the interfaces for various hardware features used by the
 *  SPARK Wireless Core library. These facades abstract the underlying
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
#ifndef SWC_HAL_FACADE_H_
#define SWC_HAL_FACADE_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/* Context Switching and Interrupt Management */

/** @defgroup ContextSwitchingAndInterruptManagement Context Switching and Interrupt Management
 *
 *  Functions related to managing context switches and setting interrupt callbacks,
 *  crucial for multitasking and efficient response to system events.
 *  @{
 */

/** @brief Manually triggers the radio #1's IRQ pin interrupt.
 *
 *  This function is specifically designed to manually activate the IRQ pin interrupt for radio #1,
 *  prompting the system to handle an interrupt event as if the radio hardware had issued it. The
 *  subsequent interrupt handling routine invokes an internal SWC library function tailored to
 *  process this event.
 *
 *  @note Unlike generic context switch mechanisms, this function does not require the user to set an
 *  application-level callback for the interrupt. However, users must implement a separate mechanism
 *  to configure a callback for the radio #1 IRQ to ensure proper handling of the triggered event
 *  within the SWC library's context.
 */
void swc_hal_radio_1_context_switch(void);

/** @brief Manually triggers the radio #2's IRQ pin interrupt.
 *
 *  @note This function is part of a dual-radio support system, allowing a single MCU to manage
 *  two separate radio ASICs. Users implementing support for a single radio ASIC do not have to provide
 *  an implementation for this function in their backend.
 *
 *  This function is specifically designed to manually activate the IRQ pin interrupt for radio #2,
 *  prompting the system to handle an interrupt event as if the radio hardware had issued it. The
 *  subsequent interrupt handling routine invokes an internal SWC library function tailored to
 *  process this event.
 *
 *  @note Unlike generic context switch mechanisms, this function does not require the user to set an
 *  application-level callback for the interrupt. However, users must implement a separate mechanism
 *  to configure a callback for the radio #1 IRQ to ensure proper handling of the triggered event
 *  within the SWC library's context.
 */
void swc_hal_radio_2_context_switch(void);

/** @brief Sets the callback function for radio #1's IRQ interrupt.
 *
 *  This function registers a internal SWC library function callback to be invoked when the IRQ pin
 *  interrupt for radio #1 is triggered.
 *
 * @param[in] callback A pointer to the callback function that will be executed upon
 *                     an interrupt from radio #1's IRQ pin.
 */
void swc_hal_set_radio_1_irq_callback(void (*callback)(void));

/** @brief Sets the callback function for radio #2's IRQ interrupt.
 *
 *  @note This function is part of a dual-radio support system, allowing a single MCU to manage
 *  two separate radio ASICs. Users implementing support for a single radio ASIC do not have to provide
 *  an implementation for this function in their backend.
 *
 *  This function registers a internal SWC library function callback to be invoked when the IRQ pin
 *  interrupt for radio #2 is triggered.
 *
 *  @param[in] callback A pointer to the callback function that will be executed upon
 *                      an interrupt from radio #2's IRQ pin.
 */
void swc_hal_set_radio_2_irq_callback(void (*callback)(void));

/** @brief Sets the callback function for the DMA receive (RX) interrupt for radio #1.
 *
 *  This function allows for the registration of a internal SWC library function callback
 *  to be invoked when the DMA RX operation for radio #1 completes,
 *
 *  @param[in] callback A pointer to the callback function that will be executed upon the
 *                      completion of a DMA RX operation for radio #1.
 */
void swc_hal_set_radio_1_dma_rx_callback(void (*callback)(void));

/** @brief Sets the callback function for the DMA receive (RX) interrupt for radio #2.
 *
 *  @note This function is part of a dual-radio support system, allowing a single MCU to manage
 *  two separate radio ASICs. Users implementing support for a single radio ASIC do not have to provide
 *  an implementation for this function in their backend.
 *
 *  This function allows for the registration of a internal SWC library function callback
 *  to be invoked when the DMA RX operation for radio #2 completes,
 *
 *  @param[in] callback A pointer to the callback function that will be executed upon the
 *                      completion of a DMA RX operation for radio #2.
 */
void swc_hal_set_radio_2_dma_rx_callback(void (*callback)(void));

/** @brief Disables the IRQ external interrupt for radio #1.
 *
 *  This function deactivates the external interrupt request (IRQ) for radio #1, preventing
 *  the interrupt handler from being invoked in response to radio #1's IRQ events.
 */
void swc_hal_radio_1_disable_irq_it(void);

/** @brief Disables the IRQ external interrupt for radio #2.
 *
 *  @note This function is part of a dual-radio support system, allowing a single MCU to manage
 *  two separate radio ASICs. Users implementing support for a single radio ASIC do not have to provide
 *  an implementation for this function in their backend.
 *
 *  This function deactivates the external interrupt request (IRQ) for radio #2, preventing
 *  the interrupt handler from being invoked in response to radio #2's IRQ events.
 */
void swc_hal_radio_2_disable_irq_it(void);

/** @brief Enables the IRQ external interrupt for radio #1.
 *
 *  This function enables the external interrupt request (IRQ) for radio #1, allowing
 *  the system to respond to IRQ signals from the radio.
 */
void swc_hal_radio_1_enable_irq_it(void);

/** @brief Enables the IRQ external interrupt for radio #2.
 *
 *  @note This function is part of a dual-radio support system, allowing a single MCU to manage
 *  two separate radio ASICs. Users implementing support for a single radio ASIC do not have to provide
 *  an implementation for this function in their backend.
 *
 *  This function enables the external interrupt request (IRQ) for radio #2, allowing
 *  the system to respond to IRQ signals from the radio.
 */
void swc_hal_radio_2_enable_irq_it(void);

/** @brief Disables the DMA SPI interrupt for radio #1.
 *
 *  This function deactivates the interrupt request (IRQ) associated with the DMA SPI
 *  operation for radio #1, preventing DMA related interrupt handling.
 */
void swc_hal_radio_1_disable_dma_irq_it(void);

/** @brief Disables the DMA SPI interrupt for radio #2.
 *
 *  @note This function is part of a dual-radio support system, allowing a single MCU to manage
 *  two separate radio ASICs. Users implementing support for a single radio ASIC do not have to provide
 *  an implementation for this function in their backend.
 *
 *  This function deactivates the interrupt request (IRQ) associated with the DMA SPI
 *  operation for radio #2, preventing DMA related interrupt handling.
 */
void swc_hal_radio_2_disable_dma_irq_it(void);

/** @brief Enables the DMA SPI interrupt for radio #1.
 *
 *  This function activates the interrupt request (IRQ) for the DMA SPI operation for radio #1,
 *  allowing DMA related interrupt handling.
 */
void swc_hal_radio_1_enable_dma_irq_it(void);

/** @brief Enables the DMA SPI interrupt for radio #2.
 *
 *  @note This function is part of a dual-radio support system, allowing a single MCU to manage
 *  two separate radio ASICs. Users implementing support for a single radio ASIC do not have to provide
 *  an implementation for this function in their backend.
 *
 *  This function activates the interrupt request (IRQ) for the DMA SPI operation for radio #2,
 *  allowing DMA related interrupt handling.
 */
void swc_hal_radio_2_enable_dma_irq_it(void);
/** @} */

/* GPIO Controls for Radios */

/** @defgroup GPIOControlsForRadios GPIO Controls for Radios
 *
 *  Functions for direct manipulation of GPIO pins connected to the radio ASIC,
 *  essential for radio module control and radios IRQ pin state monitoring.
 *  @{
 */

/** @brief Reads the status of radio #1's IRQ pin.
 * @retval True   If the pin is high.
 * @retval False  If the pin is low.
 */
bool swc_hal_radio_1_read_irq_pin(void);

/** @brief Reads the status of radio #2's IRQ pin.
 *
 *  @note This function is part of a dual-radio support system, allowing a single MCU to manage
 *  two separate radio ASICs. Users implementing support for a single radio ASIC do not have to provide
 *  an implementation for this function in their backend.
 *
 * @retval True   If the pin is high.
 * @retval False  If the pin is low.
 */
bool swc_hal_radio_2_read_irq_pin(void);

/** @brief Sets the reset pin of radio #1.
 */
void swc_hal_radio_1_set_reset_pin(void);

/** @brief Sets the reset pin of radio #2.
 *
 *  @note This function is part of a dual-radio support system, allowing a single MCU to manage
 *  two separate radio ASICs. Users implementing support for a single radio ASIC do not have to provide
 *  an implementation for this function in their backend.
 *
 */
void swc_hal_radio_2_set_reset_pin(void);

/** @brief Resets the reset pin of radio #1.
 */
void swc_hal_radio_1_reset_reset_pin(void);

/** @brief Resets the reset pin of radio #2.
 *
 *  @note This function is part of a dual-radio support system, allowing a single MCU to manage
 *  two separate radio ASICs. Users implementing support for a single radio ASIC do not have to provide
 *  an implementation for this function in their backend.
 */
void swc_hal_radio_2_reset_reset_pin(void);
/** @} */

/* SPI Communication */

/** @defgroup SPICommunication SPI Communication
 *
 *  SPI bus communication functions for data exchange between the MCU and ASIC radios,
 *  underpinning wireless communication functionality.
 *  @{
 */

/** @brief Set the on-board controller SPI chip-select pin of the radio #1.
 */
void swc_hal_radio_1_spi_set_cs(void);

/** @brief Set the on-board controller SPI chip-select pin of the radio #2.
 *
 *  @note This function is part of a dual-radio support system, allowing a single MCU to manage
 *  two separate radio ASICs. Users implementing support for a single radio ASIC do not have to provide
 *  an implementation for this function in their backend.
 */
void swc_hal_radio_2_spi_set_cs(void);

/** @brief Reset the on-board controller SPI chip-select pin of the radio #1.
 */
void swc_hal_radio_1_spi_reset_cs(void);

/** @brief Reset the on-board controller SPI chip-select pin of the radio #2.
 *
 *  @note This function is part of a dual-radio support system, allowing a single MCU to manage
 *  two separate radio ASICs. Users implementing support for a single radio ASIC do not have to provide
 *  an implementation for this function in their backend.
 */
void swc_hal_radio_2_spi_reset_cs(void);

/** @brief Read and Write data full duplex on the radio #1 in blocking mode.
 *
 *  @param[in]  tx_data  Data buffer to write.
 *  @param[out] rx_data  Data received.
 *  @param[in]  size     Size of the data.
 */
void swc_hal_radio_1_spi_transfer_full_duplex_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);

/** @brief Read and Write data full duplex on the radio #2 in blocking mode.
 *
 *  @note This function is part of a dual-radio support system, allowing a single MCU to manage
 *  two separate radio ASICs. Users implementing support for a single radio ASIC do not have to provide
 *  an implementation for this function in their backend.
 *
 *  @param[in]  tx_data  Data buffer to write.
 *  @param[out] rx_data  Data received.
 *  @param[in]  size     Size of the data.
 */
void swc_hal_radio_2_spi_transfer_full_duplex_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);

/** @brief Read and Write data full duplex on the radio #1 in non-blocking mode.
 *
 *  @param[in]  tx_data  Data buffer to write.
 *  @param[out] rx_data  Data received.
 *  @param[in]  size     Size of the data.
 */
void swc_hal_radio_1_spi_transfer_full_duplex_non_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);

/** @brief Read and Write data full duplex on the radio #2 in non-blocking mode.
 *
 *  @note This function is part of a dual-radio support system, allowing a single MCU to manage
 *  two separate radio ASICs. Users implementing support for a single radio ASIC do not have to provide
 *  an implementation for this function in their backend.
 *
 *  @param[in]  tx_data  Data buffer to write.
 *  @param[out] rx_data  Data received.
 *  @param[in]  size     Size of the data.
 */
void swc_hal_radio_2_spi_transfer_full_duplex_non_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);

/** @brief Read the status of the radio's SPI of the radio #1.
 *
 *  @retval true   SPI is busy.
 *  @retval false  SPI is not busy.
 */
bool swc_hal_radio_1_is_spi_busy(void);

/** @brief Read the status of the radio's SPI of the radio #2.
 *
 *  @note This function is part of a dual-radio support system, allowing a single MCU to manage
 *  two separate radio ASICs. Users implementing support for a single radio ASIC do not have to provide
 *  an implementation for this function in their backend.
 *
 *  @retval true   SPI is busy.
 *  @retval false  SPI is not busy.
 */
bool swc_hal_radio_2_is_spi_busy(void);
/** @} */

/* Timer and Delay Management */
/** @brief Initializes the free running timer.
 *
 *  Sets up the timer used for obtaining a tick count, required for features like Stop and Wait.
 *  The timer resolution should be set to a tick frequency between 100Hz and 1MHz,
 *
 *  @note Users not using the Stop and Wait feature should implement a pseudo-version of
 *  this function that returns UINT64_MAX, ensuring backend compatibility with the facade.
 */
void swc_hal_free_running_timer_init(void);

/** @brief Get the free running timer tick count.
 *
 *  This function is ack as a watchdog timer for the Stop and Wait feature,
 *  indicating to the Wireless Core when to abort re-transmission efforts.
 *
 *  @note Users not using the Stop and Wait feature should implement a pseudo-version of
 *  this function that returns UINT64_MAX, ensuring backend compatibility with the facade.
 *
 * @return Tick count.
 */
uint64_t swc_hal_get_tick_free_running_timer(void);

/** @brief Returns the configured tick frequency of the free running timer.
 *
 *  Enables the Wireless Core to calculate accurate tick counts for delays and
 *  other timing-related operations by providing the free running timer's frequency in hertz.
 *
 *  @note Users not using the Stop and Wait feature should implement a pseudo-version of
 *  this function that returns 0, ensuring backend compatibility with the facade.
 *
 * @return The free running timer's configured frequency in Hz.
 */
uint32_t swc_hal_get_free_running_timer_frequency_hz(void);

/* Dual Radio Timer Management */

/** @defgroup DualRadioTimerManagement Dual Radio Timer Management
 *¬
 *  Functions for managing a hardware timer specifically designed for synchronizing dual radio operations.
 *  This includes initializing the timer, setting timer periods, handling callbacks, and managing timer interrupts.
 * @{
 */

/** @brief Initializes the timer for dual-radio support.
 *
 *  Configures the multi-radio timer with the following options:
 *  - Counter Up mode
 *  - Auto-reload preload disabled
 *  - Tick frequency set within this range: 18MHz and 22MHz
 *  - Interrupt generation at the end of each period
 *
 *  @note This function is part of a dual-radio support system, allowing a single MCU to manage
 *  two separate radio ASICs. Users implementing support for a single radio ASIC do not have to provide
 *  an implementation for this function in their backend.
 *
 */
void swc_hal_multi_radio_timer_init(void);

/** @brief Set multi-radio callback.
 *
 *  This function registers a internal SWC library function callback to be invoked when the IRQ pin
 *  interrupt for multi radio timer is triggered.
 *
 *  @note This function is part of a dual-radio support system, allowing a single MCU to manage
 *  two separate radio ASICs. Users implementing support for a single radio ASIC do not have to provide
 *  an implementation for this function in their backend.
 *
 * @param[in] callback A pointer to the callback function that will be executed.
 */
void swc_hal_set_multi_radio_timer_callback(void (*callback)(void));

/** @brief Starts the multi-radio timer.
 *
 *  Initiates the countdown for the multi-radio timer.
 *
 *  @note This function is part of a dual-radio support system, allowing a single MCU to manage
 *  two separate radio ASICs. Users implementing support for a single radio ASIC do not have to provide
 *  an implementation for this function in their backend.
 *
 */
void swc_hal_timer_multi_radio_timer_start(void);

/** @brief Starts the multi-radio timer.
 *
 *  Halt the countdown for the multi-radio timer.
 *
 *  @note This function is part of a dual-radio support system, allowing a single MCU to manage
 *  two separate radio ASICs. Users implementing support for a single radio ASIC do not have to provide
 *  an implementation for this function in their backend.
 *
 */
void swc_hal_timer_multi_radio_timer_stop(void);

/** @brief Sets the period of the multi-radio timer.
 *
 *  Defines the duration of the timer cycle for dual-radio operations, specified in ticks.
 *  Adjusting the period allows for the synchronization of actions across multiple radio
 *  units within a defined timeline.
 *
 *  @note This function is part of a dual-radio support system, allowing a single MCU to manage
 *  two separate radio ASICs. Users implementing support for a single radio ASIC do not have to provide
 *  an implementation for this function in their backend.
 *
 *
 * @param[in] period The timer period, in ticks.
 */
void swc_hal_timer_multi_radio_timer_set_period(uint16_t period);

/** @brief Set the period of the dual radio timer to the maximum value.
 *
 *  @note This function is part of a dual-radio support system, allowing a single MCU to manage
 *  two separate radio ASICs. Users implementing support for a single radio ASIC do not have to provide
 *  an implementation for this function in their backend.
 *
 */
void swc_hal_timer_multi_radio_timer_set_max_period(void);

/** @brief Returns the configured tick frequency of the multi radio timer.
 *
 *  Enables the Wireless Core to calculate accurate tick counts for delays and
 *  other timing-related operations by providing the multi radio timer's frequency in hertz.
 *
 * @return The multi radio timer's configured frequency in Hz.
 */
uint32_t swc_hal_get_timer_multi_frequency_hz(void);
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* SWC_HAL_FACADE_H_ */
