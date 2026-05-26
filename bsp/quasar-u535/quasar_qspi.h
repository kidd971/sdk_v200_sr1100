/** @file  quasar_qspi.h
 *  @brief This module configure QSPI and provides functions to transmit and receive.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_QSPI_H_
#define QUASAR_QSPI_H_

/* INCLUDES *******************************************************************/
#include "quasar_dma.h"
#include "quasar_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* EXTERNS ********************************************************************/
extern OSPI_HandleTypeDef *const qspi_handle_table[];

/* Return the global interrupt of the selected QSPI peripheral from quasar_qspi_selection_t. */
#if defined(STM32U535xx)
#define QUASAR_QSPI_GET_SELECTED_IRQ(qspi_selection) ((IRQn_Type)((void)(qspi_selection), OCTOSPI1_IRQn))
#else
#define QUASAR_QSPI_GET_SELECTED_IRQ(qspi_selection) \
    ((IRQn_Type)((qspi_selection == QUASAR_QSPI_SELECTION_OSPI1) ? OCTOSPI1_IRQn : OCTOSPI2_IRQn))
#endif

/* MACROS *********************************************************************/
/*! Return the handle from the selected QSPI from quasar_qspi_selection_t. */
#define QUASAR_QSPI_GET_SELECTED_HANDLE(qspi_selection) \
    ((OSPI_HandleTypeDef *)((qspi_selection < _QUASAR_QSPI_SELECTION_COUNT) ? qspi_handle_table[qspi_selection] : NULL))

/*! Return true is the QSPI is busy, false if not. */
#define QUASAR_QSPI_IS_BUSY(qspi_selection) \
    ((QUASAR_QSPI_GET_SELECTED_HANDLE(qspi_selection)->Instance->SR) & OCTOSPI_SR_BUSY)

/* TYPES **********************************************************************/
/** @brief List of all available QSPI instances.
 */
typedef enum quasar_qspi_selection {
    /*! Select the full feature OSPI 1. */
    QUASAR_QSPI_SELECTION_OSPI1 = 0,
    /*! Select the full feature OSPI 2. */
    QUASAR_QSPI_SELECTION_OSPI2,
    /*! Indicate the number of possible SPI selection. */
    _QUASAR_QSPI_SELECTION_COUNT,
} quasar_qspi_selection_t;

/** @brief List of Quasar QSPI communication modes, specifying line count
 *         and dummy data bits.
 */
typedef enum quasar_qspi_mode {
    /*! Single line mode with no dummy data. */
    QUASAR_QSPI_1_LINE_MODE_0_DUMMY,
    /*! Four lines mode with no dummy data. */
    QUASAR_QSPI_4_LINES_MODE_0_DUMMY,
    /*! Four lines mode with one dummy data. */
    QUASAR_QSPI_4_LINES_MODE_1_DUMMY,
} quasar_qspi_mode_t;

/** @brief Quasar BSP QSPI configuration.
 */
typedef struct quasar_qspi_config {
    /*! Selected QSPI instance. */
    quasar_qspi_selection_t qspi_selection;
    /*! Selected QSPI prescaler. */
    uint8_t qspi_prescaler;
    /*! Selected GPIO configuration used for SCK. */
    quasar_gpio_config_t gpio_config_sck;
    /*! Selected GPIO configuration used for IO 0. */
    quasar_gpio_config_t gpio_config_io_0;
    /*! Selected GPIO configuration used for IO 1. */
    quasar_gpio_config_t gpio_config_io_1;
    /*! Selected GPIO configuration used for IO 2. */
    quasar_gpio_config_t gpio_config_io_2;
    /*! Selected GPIO configuration used for IO 3. */
    quasar_gpio_config_t gpio_config_io_3;
    /*! Available IRQ priority. */
    quasar_irq_priority_t irq_priority;
    /*! Selected DMA configuration. */
    quasar_dma_config_t dma_config;
} quasar_qspi_config_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the QSPI peripheral.
 *
 *  @note The configuration of the QSPI protocol is not complete; the number
 *        dummy data as well as the number of communication lines used still
 *        need to be determined. (See quasar_qspi_mode enumeration and the
 *        quasar_qspi_set_mode function)
 *
 *  @param[in] qspi_config  Configuration of the QSPI peripheral.
 */
void quasar_qspi_init(quasar_qspi_config_t qspi_config);

/** @brief Enable the selected QSPI interrupt.
 *
 *  @note By default, the interrupt is not enabled in the initialization.
 *        The reason is that the interrupt routine should only be called
 *        when the wireless core performs a DMA transmission.
 *
 *  @param[in] qspi_selection  Selected QSPI.
 */
void quasar_qspi_enable_interrupt(quasar_qspi_selection_t qspi_selection);

/** @brief Disable the selected QSPI interrupt.
 *
 *  @param[in] qspi_selection  Selected QSPI.
 */
void quasar_qspi_disable_interrupt(quasar_qspi_selection_t qspi_selection);

/** @brief Set the dummy data and the number of lines used in the QSPI protocol for a specified QSPI.
 *
 *  @param[in] qspi_selection  Selected QSPI.
 *  @param[in] mode            Selection dummy data bits and communication lines.
 */
void quasar_qspi_set_mode(quasar_qspi_selection_t qspi_selection, quasar_qspi_mode_t mode);

/** @brief Initiate a blocking QSPI reception for a specified QSPI instance.
 *
 *  @param[in]  qspi_selection  Selected QSPI peripheral.
 *  @param[in]  command         Command byte of the transfer.
 *  @param[out] data            Data that have been received.
 *  @param[in]  size            Size of the data received.
 *  @param[in]  timeout         Timeout duration for the transfer.
 */
void quasar_qspi_transfer_half_duplex_rx_blocking(quasar_qspi_selection_t qspi_selection, uint8_t command,
                                                  uint8_t *data, uint8_t size, uint16_t timeout);

/** @brief Initiate a blocking QSPI transmission for a specified QSPI instance.
 *
 *  @param[in]  qspi_selection  Selected QSPI peripheral.
 *  @param[in]  command         Command byte of the transfer.
 *  @param[out] data            Data that has to be transmit.
 *  @param[in]  size            Size of the data transmited.
 *  @param[in]  timeout         Timeout duration for the transfer.
 */
void quasar_qspi_transfer_half_duplex_tx_blocking(quasar_qspi_selection_t qspi_selection, uint8_t command, uint8_t *data, uint8_t size,
                                   uint16_t timeout);

/** @brief Initiate a non blocking QSPI reception for a specified QSPI instance.
 *
 *  @param[in]  qspi_selection  Selected QSPI peripheral.
 *  @param[in]  command         Command byte of the transfer.
 *  @param[out] data            Data that has to be transmit.
 *  @param[in]  size            Size of the data transmited.
 */
void quasar_qspi_transfer_half_duplex_rx_non_blocking(quasar_qspi_selection_t qspi_selection, uint8_t command, uint8_t *data,
                                      uint8_t size);

/** @brief Initiate a non blocking QSPI transmission for a specified QSPI instance.
 *
 *  @param[in]  qspi_selection  Selected QSPI peripheral.
 *  @param[in]  command         Command byte of the transfer.
 *  @param[out] data            Data that has to be transmit.
 *  @param[in]  size            Size of the data transmited.
 */
void quasar_qspi_transfer_half_duplex_tx_non_blocking(quasar_qspi_selection_t qspi_selection, uint8_t command, uint8_t *data,
                                       uint8_t size);

/** @brief This function sets the function callback for the OCTOSPI1 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_qspi_set_qspi1_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the OCTOSPI2 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_qspi_set_qspi2_callback(void (*irq_callback)(void));

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_QSPI_H_ */
