/** @file  quasar_radio.h
 *  @brief This module provides functions for the SPARK Radio module.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_RADIO_H_
#define QUASAR_RADIO_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include "quasar_adc.h"
#include "quasar_def.h"
#include "quasar_dma.h"
#include "quasar_error.h"
#include "quasar_qspi.h"
#include "quasar_spi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Quasar BSP radio configuration.
 */
typedef struct quasar_radio_config {
    /*! Selected GPIO configuration used for reset pin. */
    quasar_gpio_config_t reset_io;
    /*! Selected GPIO configuration used for shutdown pin. */
    quasar_gpio_config_t shutdown_io;
    /*! Selected GPIO configuration used for irq pin. */
    quasar_gpio_config_t irq_io;
    /*! Selected IRQ priority used for irq pin. */
    quasar_irq_priority_t irq_priority_of_irq_io;
    /*! Selected SPI configuration. */
    quasar_spi_config_t spi_config;
    /*! Selected DMA configuration. */
    quasar_dma_config_t dma_config;
    /*! Selected QSPI configuration. */
    quasar_qspi_config_t qspi_config;
} quasar_radio_config_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the radio 1's peripherals.
 *
 *  @li Reset pin
 *  @li Shutdown pin
 *  @li IRQ pin
 *  @li SPI1 SCK pin
 *  @li SPI1 MOSI pin
 *  @li SPI1 MISO pin
 *  @li SPI1 CS pin
 *
 *  @param[out] err  Pointer to store error status.
 */
void quasar_radio_1_init(quasar_bsp_status_t *err);

/** @brief Initialize the radio 2's peripherals.
 *
 *  @li Reset pin
 *  @li Shutdown pin
 *  @li IRQ pin
 *  @li SPI2 SCK pin
 *  @li SPI2 MOSI pin
 *  @li SPI2 MISO pin
 *  @li SPI2 CS pin
 *
 *  @note Depending on the board revision the MOSI pin GPIO differs.
 *
 *  @param[in]  board_revision  The board revision.
 *  @param[out] err             Pointer to store error status.
 */
void quasar_radio_2_init(quasar_revision_t board_revision, quasar_bsp_status_t *err);

/** @brief Deinitialize the radio 1's peripherals.
 *
 *  @param[out] err  Pointer to store error status.
 */
void quasar_radio_1_deinit(quasar_bsp_status_t *err);

/** @brief Deinitialize the radio 2's peripherals.
 *
 *  @note Depending on the board revision the MOSI pin GPIO differs.
 *
 *  @param[in]  board_revision  The board revision.
 *  @param[out] err             Pointer to store error status.
 */
void quasar_radio_2_deinit(quasar_revision_t board_revision, quasar_bsp_status_t *err);

/** @brief This function sets the function callback for the radio pin interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_radio_set_radio_1_irq_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the radio pin interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_radio_set_radio_2_irq_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the radio 1 DMA interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_radio_set_radio_1_non_blocking_transfer_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the radio 2 DMA interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_radio_set_radio_2_non_blocking_transfer_callback(void (*irq_callback)(void));

/** @brief Read the status of the on-board controller IRQ pin of the radio #1.
 *
 *  @retval True   Pin is high.
 *  @retval False  Pin is low.
 */
bool quasar_radio_1_read_irq_pin(void);

/** @brief Read the status of the on-board controller IRQ pin of the radio #2.
 *
 *  @retval True   Pin is high.
 *  @retval False  Pin is low.
 */
bool quasar_radio_2_read_irq_pin(void);

/** @brief Enable the on-board controller IRQ external interrupt of the radio #1.
 */
void quasar_radio_1_enable_irq_it(void);

/** @brief Enable the on-board controller IRQ external interrupt of the radio #2.
 */
void quasar_radio_2_enable_irq_it(void);

/** @brief Disable the on-board controller IRQ external interrupt of the radio #1.
 */
void quasar_radio_1_disable_irq_it(void);

/** @brief Disable the on-board controller IRQ external interrupt of the radio #2.
 */
void quasar_radio_2_disable_irq_it(void);

/** @brief Enable the DMA SPI interrupt of the radio #1.
 */
void quasar_radio_1_enable_non_blocking_transfer_irq_it(void);

/** @brief Enable the DMA SPI interrupt of the radio #2.
 */
void quasar_radio_2_enable_non_blocking_transfer_irq_it(void);

/** @brief Disable the DMA SPI interrupt of the radio #1.
 */
void quasar_radio_1_disable_non_blocking_transfer_irq_it(void);

/** @brief Disable the DMA SPI interrupt of the radio #2.
 */
void quasar_radio_2_disable_non_blocking_transfer_irq_it(void);

/** @brief Set the on-board controller shutdown pin of the radio #1.
 */
void quasar_radio_1_set_shutdown_pin(void);

/** @brief Set the on-board controller shutdown pin of the radio #2.
 */
void quasar_radio_2_set_shutdown_pin(void);

/** @brief Reset the on-board controller shutdown pin of the radio #1.
 */
void quasar_radio_1_reset_shutdown_pin(void);

/** @brief Reset the on-board controller shutdown pin of the radio #2.
 */
void quasar_radio_2_reset_shutdown_pin(void);

/** @brief Set the on-board controller reset pin of the radio #1.
 */
void quasar_radio_1_set_reset_pin(void);

/** @brief Set the on-board controller reset pin of the radio #2.
 */
void quasar_radio_2_set_reset_pin(void);

/** @brief Reset the on-board controller reset pin of the radio #1.
 */
void quasar_radio_1_reset_reset_pin(void);

/** @brief Reset the on-board controller reset pin of the radio #2.
 */
void quasar_radio_2_reset_reset_pin(void);

/** @brief Set the radio 1`s debug enable pin.
 *
 *  @note With the SR11x0 this pin may be used as VDDIO for the radio on the demo board.
 */
void quasar_radio_1_set_debug_en(void);

/** @brief Reset the radio 1 debug enable pin.
 *
 *  @note With the SR11x0 this pin may be used as VDDIO for the radio on the demo board.
 */
void quasar_radio_1_reset_debug_en(void);

/** @brief Software interrupt trigger to force the cpu to get into the interrupt handler of the radio #1.
 */
void quasar_radio_1_context_switch(void);

/** @brief Software interrupt trigger to force the cpu to get into the interrupt handler of the radio #2.
 */
void quasar_radio_2_context_switch(void);

/** @brief Induce a context switch to the pendSV ISR.
 */
void quasar_radio_callback_context_switch(void);

/** @brief Change the radio 1 SPI BaudRate.
 *
 *  By default the SPI peripheral is initalized with a prescaler of 4.
 *
 *  @param[in]  prescaler  SPI BaudRate Prescaler
 *  @param[out] err        Pointer to quasar_bsp_status_t for error reporting.
 */
void quasar_radio_1_set_spi_baudrate(quasar_spi_prescaler_t prescaler, quasar_bsp_status_t *err);

/** @brief Change the radio 2 SPI BaudRate.
 *
 *  By default the SPI peripheral is initalized with a prescaler of 4.
 *
 *  @param[in]  prescaler  SPI BaudRate Prescaler
 *  @param[out] err        Pointer to quasar_bsp_status_t for error reporting.
 */
void quasar_radio_2_set_spi_baudrate(quasar_spi_prescaler_t prescaler, quasar_bsp_status_t *err);

/** @brief Set the on-board controller SPI chip-select pin of the radio #1.
 */
void quasar_radio_1_spi_set_cs(void);

/** @brief Set the on-board controller SPI chip-select pin of the radio #2.
 */
void quasar_radio_2_spi_set_cs(void);

/** @brief Reset the on-board controller SPI chip-select pin of the radio #1.
 */
void quasar_radio_1_spi_reset_cs(void);

/** @brief Reset the on-board controller SPI chip-select pin of the radio #2.
 */
void quasar_radio_2_spi_reset_cs(void);

/** @brief Read and Write data full duplex on the radio #1 in blocking mode.
 *
 *  @param[in]  tx_data  Data buffer to write.
 *  @param[out] rx_data  Data received.
 *  @param[in]  size     Size of the data.
 */
void quasar_radio_1_spi_transfer_full_duplex_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);

/** @brief Read and Write data full duplex on the radio #2 in blocking mode.
 *
 *  @param[in]  tx_data  Data buffer to write.
 *  @param[out] rx_data  Data received.
 *  @param[in]  size     Size of the data.
 */
void quasar_radio_2_spi_transfer_full_duplex_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);

/** @brief Read and Write data full duplex on the radio #1 in non-blocking mode.
 *
 *  @param[in]  tx_data  Data buffer to write.
 *  @param[out] rx_data  Data received.
 *  @param[in]  size     Size of the data.
 */
void quasar_radio_1_spi_transfer_full_duplex_non_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);

/** @brief Read and Write data full duplex on the radio #2 in non-blocking mode.
 *
 *  @param[in]  tx_data  Data buffer to write.
 *  @param[out] rx_data  Data received.
 *  @param[in]  size     Size of the data.
 */
void quasar_radio_2_spi_transfer_full_duplex_non_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);

/** @brief Read the status of the radio's SPI of the radio #1.
 *
 *  @retval true   SPI is busy.
 *  @retval false  SPI is not busy.
 */
bool quasar_radio_1_is_spi_busy(void);

/** @brief Read the status of the radio's SPI of the radio #2.
 *
 *  @retval true   SPI is busy.
 *  @retval false  SPI is not busy.
 */
bool quasar_radio_2_is_spi_busy(void);

/** @brief Initialize QSPI GPIOs as inputs while those pins are not used.
 */
void quasar_radio_init_unused_qspi_gpios(void);

/** @brief Initialize SPI GPIOs as inputs while those pins are not used.
 */
void quasar_radio_1_init_unused_spi_gpios(void);

/** @brief Initialize SPI GPIOs as inputs while those pins are not used.
 *
 *  @param[in] board_revision  The board revision.
 */
void quasar_radio_2_init_unused_spi_gpios(quasar_revision_t board_revision);

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_RADIO_H_ */
