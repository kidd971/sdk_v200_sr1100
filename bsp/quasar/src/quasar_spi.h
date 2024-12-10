/** @file  quasar_spi.h
 *  @brief This module configure SPI and provides functions to transmit and receive.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_SPI_H_
#define QUASAR_SPI_H_

/* INCLUDES *******************************************************************/
#include "quasar_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief List of all available SPI instances.
 */
typedef enum quasar_spi_selection {
    /*! Select the full feature SPI 1. */
    QUASAR_SPI_SELECTION_SPI1 = 0,
    /*! Select the full feature SPI 2. */
    QUASAR_SPI_SELECTION_SPI2 = 1,
    /*! Select the limited feature SPI 3. */
    QUASAR_SPI_SELECTION_SPI3 = 2,
    /*! Indicate the number of possible SPI selection. */
    _QUASAR_SPI_SELECTION_COUNT = 3
} quasar_spi_selection_t;

/** @brief List of all available clock sources for the SPI instances.
 *
 *  @note From the reference manual
 *      - `0b00` : PCLK1    -> set 0
 *      - `0b01` : SYSCLK   -> set 1
 *      - `0b10` : HSI16    -> set 2
 *      - `0b11` : MSIK     -> set 3
 *
 *  The selected clock source must be initialized and activated before.
 */
typedef enum quasar_spi_clk_source {
    /* Select PCLK1 as clock source. */
    QUASAR_SPI_CLK_SOURCE_PCLK1  = 0,
    /* Select SYSCLK as clock source. */
    QUASAR_SPI_CLK_SOURCE_SYSCLK = 1,
    /* Select HSI16 as clock source. */
    QUASAR_SPI_CLK_SOURCE_HSI16  = 2,
    /* Select MSIK as clock source. */
    QUASAR_SPI_CLK_SOURCE_MSIK   = 3,
} quasar_spi_clk_source_t;

/** @brief List of available SPI prescalers.
 */
typedef enum quasar_spi_prescaler {
    /* Set the SPI prescaler to 2. */
    QUASAR_SPI_PRESCALER_2   = SPI_BAUDRATEPRESCALER_2,
    /* Set the SPI prescaler to 4. */
    QUASAR_SPI_PRESCALER_4   = SPI_BAUDRATEPRESCALER_4,
    /* Set the SPI prescaler to 8. */
    QUASAR_SPI_PRESCALER_8   = SPI_BAUDRATEPRESCALER_8,
    /* Set the SPI prescaler to 16. */
    QUASAR_SPI_PRESCALER_16  = SPI_BAUDRATEPRESCALER_16,
    /* Set the SPI prescaler to 32. */
    QUASAR_SPI_PRESCALER_32  = SPI_BAUDRATEPRESCALER_32,
    /* Set the SPI prescaler to 64. */
    QUASAR_SPI_PRESCALER_64  = SPI_BAUDRATEPRESCALER_64,
    /* Set the SPI prescaler to 128. */
    QUASAR_SPI_PRESCALER_128 = SPI_BAUDRATEPRESCALER_128,
    /* Set the SPI prescaler to 256. */
    QUASAR_SPI_PRESCALER_256 = SPI_BAUDRATEPRESCALER_256,
} quasar_spi_prescaler_t;

/** @brief Quasar BSP SPI configuration.
 */
typedef struct quasar_spi_config {
    /*! Selected SPI instance. */
    quasar_spi_selection_t spi_selection;
    /*! Selected SPI prescaler. */
    quasar_spi_prescaler_t spi_prescaler;
    /*! Selected GPIO configuration used for SCK. */
    quasar_gpio_config_t gpio_config_sck;
    /*! Selected GPIO configuration used for MOSI. */
    quasar_gpio_config_t gpio_config_mosi;
    /*! Selected GPIO configuration used for MISO. */
    quasar_gpio_config_t gpio_config_miso;
    /*! Selected GPIO configuration used for chip select. */
    quasar_gpio_config_t gpio_config_ncs;
    /*! Selected clock source. */
    quasar_spi_clk_source_t clk_source;
} quasar_spi_config_t;

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Initialize the SPI peripheral.
 *
 *  @param[in] spi_config  Configuration of the SPI peripheral.
 */
void quasar_spi_init(quasar_spi_config_t spi_config);

/** @brief Deinitialize the SPI peripheral and it's associated GPIOs.
 *
 *  @param[in] spi_config  Configuration of the SPI peripheral.
 */
void quasar_spi_deinit(quasar_spi_config_t spi_config);

/** @brief Initiate a blocking SPI transmission for a specified SPI instance.
 *
 *  @param[in] spi_selection  Selected SPI peripheral.
 *  @param[in] data           Data to be transmitted.
 *  @return HAL status.
 */
uint8_t quasar_spi_transmit(quasar_spi_selection_t spi_selection, uint8_t data);

/** @brief Initiate a blocking SPI reception for a specified SPI instance.
 *
 *  @param[in]  spi_selection  Selected SPI peripheral.
 *  @param[out] data           Data that have been received.
 *  @return HAL status.
 */
uint8_t quasar_spi_receive(quasar_spi_selection_t spi_selection, uint8_t *data);

/** @brief Initiate a blocking SPI full duplex transfer for a specified SPI instance.
 *
 *  @param[in]  spi_selection  Selected SPI peripheral.
 *  @param[in]  tx_data        Data that have to be transmit.
 *  @param[out] rx_data        Data that have been received.
 *  @param[in]  data_size      Size of the data, both received and transmitted.
 */
void quasar_spi_transfer_full_duplex_blocking(quasar_spi_selection_t spi_selection, uint8_t *tx_data, uint8_t *rx_data, uint16_t data_size);

/** @brief Initiate a non blocking SPI full duplex transfer for a specified SPI instance.
 *
 *  @param[in]  spi_selection  Selected SPI peripheral.
 *  @param[in]  tx_data        Data that have to be transmit.
 *  @param[out] rx_data        Data that have been received.
 *  @param[in]  data_size      Size of the data, both received and transmitted.
 */
void quasar_spi_transfer_full_duplex_non_blocking(quasar_spi_selection_t spi_selection, uint8_t *tx_data, uint8_t *rx_data, uint16_t data_size);

/** @brief Start a SPI transaction for a specified chip select.
 *
 *  @param[in] gpio_port_cs  GPIO port of the chip select.
 *  @param[in] gpio_pin_cs   GPIO pin of the chip select.
 */
void quasar_spi_clear_cs(quasar_gpio_port_t gpio_port_cs, quasar_gpio_pin_t gpio_pin_cs);

/** @brief Stop a SPI transaction for a specified chip select.
 *
 *  @param[in] gpio_port_cs  GPIO port of the chip select.
 *  @param[in] gpio_pin_cs   GPIO pin of the chip select.
 */
void quasar_spi_set_cs(quasar_gpio_port_t gpio_port_cs, quasar_gpio_pin_t gpio_pin_cs);

/** @brief Set the baudrate for a specified SPI.
 *
 *  @param[in] spi_selection  Selected SPI.
 *  @param[in] prescaler      Prescaler value that has to be set.
 */
void quasar_spi_set_baudrate(quasar_spi_selection_t spi_selection, quasar_spi_prescaler_t prescaler);

/** @brief Read the status of the SPI.
 *
 *  @param[in] spi_selection  Available SPI selection.
 *  @retval true   SPI is busy.
 *  @retval false  SPI is not busy.
 */
bool quasar_spi_is_busy(quasar_spi_selection_t spi_selection);

/** @brief Return the handle from the selected SPI.
 *
 *  @param[in] spi_selection  Available SPI selection.
 *  @return Selected SPI handle.
 */
SPI_HandleTypeDef *quasar_spi_get_selected_handle(quasar_spi_selection_t spi_selection);

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_SPI_H_ */
