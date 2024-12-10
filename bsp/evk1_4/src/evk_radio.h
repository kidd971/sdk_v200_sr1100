/** @file  evk_radio.h
 *  @brief This module controls the peripherals for the SR10x0 radio.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef EVK_RADIO_H_
#define EVK_RADIO_H_

/* INCLUDES *******************************************************************/
#include "evk_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize all peripherals related to the SR10x0 radio.
 *
 *  @param[in] pendsv_prio  PendSV priority.
 */
void evk_radio_peripherals_init(uint32_t pendsv_prio);

/** @brief Read the status of the on-board controller IRQ pin.
 *
 *  @retval True   Pin is high.
 *  @retval False  Pin is low.
 */
bool evk_radio_read_irq_pin(void);

/** @brief Enable the on-board controller IRQ external interrupt.
 */
void evk_radio_enable_irq_it(void);

/** @brief Disable the on-board controller IRQ external interrupt.
 */
void evk_radio_disable_irq_it(void);

/** @brief Enable the DMA SPI interrupt of the radio.
 */
void evk_radio_enable_dma_irq_it(void);

/** @brief Disable the DMA SPI interrupt of the radio.
 */
void evk_radio_disable_dma_irq_it(void);

/** @brief Set the on-board controller shutdown pin.
 */
void evk_radio_set_shutdown_pin(void);

/** @brief Reset the on-board controller shutdown pin.
 */
void evk_radio_reset_shutdown_pin(void);

/** @brief Set the on-board controller reset pin.
 */
void evk_radio_set_reset_pin(void);

/** @brief Reset the on-board controller reset pin.
 */
void evk_radio_reset_reset_pin(void);

/** @brief Set the on-board controller chip-select pin.
 */
void evk_radio_spi_set_cs(void);

/** @brief Reset the on-board controller chip-select pin.
 */
void evk_radio_spi_reset_cs(void);

/** @brief Set the on-board controller debug enable pin.
 */
void evk_radio_set_debug_en(void);

/** @brief Reset the on-board controller debug enable pin.
 */
void evk_radio_reset_debug_en(void);

/** @brief Write data on the SPI communication bus.
 *
 *  @param[in] data  Data buffer to write.
 *  @param[in] size  Size of the data to write.
 */
void evk_radio_spi_write_blocking(uint8_t *data, uint8_t size);

/** @brief Read data on the SPI communication bus.
 *
 *  @param[out] data  Data received.
 *  @param[in]  size  Size of the data to read.
 */
void evk_radio_spi_read_blocking(uint8_t *data, uint8_t size);

/** @brief Read and Write data full duplex on the radio in blocking mode.
 *
 *  @param tx_data  Data buffer to write.
 *  @param rx_data  Data received.
 *  @param size     Size of the data.
 */
void evk_radio_spi_transfer_full_duplex_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);

/** @brief Read and Write data full duplex on the radio in non-blocking mode.
 *
 *  @param tx_data  Data buffer to write.
 *  @param rx_data  Data received.
 *  @param size     Size of the data.
 */
void evk_radio_spi_transfer_full_duplex_non_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);

/** @brief Read the status of the radio's SPI.
 *
 *  @retval true   SPI is busy.
 *  @retval false  SPI is not busy.
 */
bool evk_radio_is_spi_busy(void);

/** @brief Software interrupt trigger to force the cpu to get into the interrupt handler.
 */
void evk_radio_context_switch(void);

/** @brief Induce a context switch to the pendSV ISR.
 */
void evk_radio_callback_context_switch(void);

/** @brief Change the radio's SPI BaudRate.
 *
 *  The evk_init() function does initialize by default the SPI peripheral with a prescaler of 4.
 *
 *  @param[in] prescaler  SPI BaudRate Prescaler
 */
void evk_radio_set_spi_baudrate(spi_prescaler_t prescaler);

#ifdef __cplusplus
}
#endif

#endif /* EVK_RADIO_H_ */

