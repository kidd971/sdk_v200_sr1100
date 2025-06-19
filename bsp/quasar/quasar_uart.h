/** @file  quasar_uart.h
 *  @brief This module configure UART and provides functions to transmit and receive.
 *
 *  @note This driver only supports a UART protocol with 8 bits of data.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_UART_H_
#define QUASAR_UART_H_

/* INCLUDES *******************************************************************/
#include "quasar_gpio.h"
#include "quasar_it.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief List of all available UART instances. Also used as index into Quasar
 *  FIFO buffer array.
 */
typedef enum quasar_uart_selection {
    /*! Select the full feature USART 1. */
    QUASAR_UART_SELECTION_USART1 = 0,
    /*! Select the full feature USART 2. */
    QUASAR_UART_SELECTION_USART2 = 1,
    /*! Select the full feature USART 3. */
    QUASAR_UART_SELECTION_USART3 = 2,
    /*! Select the basic feature UART 4. */
    QUASAR_UART_SELECTION_UART4 = 3,
    /*! Select the basic feature UART 5. */
    QUASAR_UART_SELECTION_UART5 = 4,
    /*! Select the full feature USART 6. */
    QUASAR_UART_SELECTION_USART6 = 5,
    /*! Indicate the number of possible UART selection. */
    _QUASAR_UART_SELECTION_COUNT = 6
} quasar_uart_selection_t;

/** @brief List of most common baud rates.
 */
typedef enum quasar_uart_baud_rate {
    /*! Select the UART communication at 1200 baud/s */
    QUASAR_UART_BAUD_RATE_1200 = 1200,
    /*! Select the UART communication at 2400 baud/s */
    QUASAR_UART_BAUD_RATE_2400 = 2400,
    /*! Select the UART communication at 4800 baud/s */
    QUASAR_UART_BAUD_RATE_4800 = 4800,
    /*! Select the UART communication at 9600 baud/s */
    QUASAR_UART_BAUD_RATE_9600 = 9600,
    /*! Select the UART communication at 19200 baud/s */
    QUASAR_UART_BAUD_RATE_19200 = 19200,
    /*! Select the UART communication at 38400 baud/s */
    QUASAR_UART_BAUD_RATE_38400 = 38400,
    /*! Select the UART communication at 57600 baud/s */
    QUASAR_UART_BAUD_RATE_57600 = 57600,
    /*! Select the UART communication at 115200 baud/s */
    QUASAR_UART_BAUD_RATE_115200 = 115200,
    /*! Select the UART communication at 1152000 baud/s */
    QUASAR_UART_BAUD_RATE_1152000 = 1152000,
} quasar_uart_baud_rate_t;

/** @brief List of available stop bits configurations.
 */
typedef enum quasar_uart_stop {
    /*! Select the 0.5 stop bit configuration. */
    QUASAR_UART_STOP_BITS_0B5 = 0,
    /*! Select the 1 stop bit configuration. */
    QUASAR_UART_STOP_BITS_1B = 1,
    /*! Select the 1.5 stop bits configuration. */
    QUASAR_UART_STOP_BITS_1B5 = 2,
    /*! Select the 2 stop bits configuration. */
    QUASAR_UART_STOP_BITS_2B = 3,
} quasar_uart_stop_t;

/** @brief List of available parity bits configurations.
 */
typedef enum quasar_uart_parity {
    /*! Select no parity bit configuration. */
    QUASAR_UART_PARITY_NONE = 0,
    /*! Select even parity bit configuration. */
    QUASAR_UART_PARITY_EVEN = 1,
    /*! Select odd parity bit configuration. */
    QUASAR_UART_PARITY_ODD = 2,
} quasar_uart_parity_t;

/** @brief Quasar BSP UART configuration.
 */
typedef struct quasar_uart_config {
    /*! Selected UART instance. */
    quasar_uart_selection_t uart_selection;
    /*! Selected baud rate configuration. */
    quasar_uart_baud_rate_t baud_rate;
    /*! Selected parity bits configuration. */
    quasar_uart_parity_t parity;
    /*! Selected stop bits configuration. */
    quasar_uart_stop_t stop;
    /*! Selected GPIO configuration used for reception. */
    quasar_gpio_config_t gpio_config_rx;
    /*! Selected GPIO configuration used for transmission. */
    quasar_gpio_config_t gpio_config_tx;
    /*! Available IRQ priority. */
    quasar_irq_priority_t irq_priority;
} quasar_uart_config_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the UART peripheral.
 *
 *  @param[in] uart_config  Configuration of the UART peripheral.
 */
void quasar_uart_init(quasar_uart_config_t uart_config);

/** @brief Deinitialize the UART peripheral.
 *
 *  @param[in] uart_config  Configuration of the UART peripheral.
 */
void quasar_uart_deinit(quasar_uart_config_t uart_config);

/** @brief Initiate UART transmission with interrupt for a specified UART instance.
 *
 *  @note This function should be used only if the UART instance has been initialized with IRQ.
 *
 *  @param[in] uart_selection    Selected UART peripheral.
 *  @param[in] data_to_transmit  Data to be transmitted.
 *  @param[in] size              Size of the data array to be transmitted.
 */
void quasar_uart_transmit_byte_irq(quasar_uart_selection_t uart_selection, uint8_t data_to_transmit);

/** @brief Initiate UART transmission with interrupt for a specified UART instance.
 *
 *  @note This function should be used only if the UART instance has been initialized with IRQ.
 *
 *  @param[in] uart_selection          Selected UART peripheral.
 *  @param[in] data_array_to_transmit  Array of data to be transmitted.
 *  @param[in] size                    Size of the data array to be transmitted.
 */
void quasar_uart_transmit_bytes_irq(quasar_uart_selection_t uart_selection, uint8_t *data_array_to_transmit,
                                    uint32_t size);

/** @brief Initiate UART transmission with interrupt for a specified UART instance.
 *
 *  @note This function should be used only if the UART instance has been initialized with IRQ.
 *
 *  @param[in] uart_selection      Selected UART peripheral.
 *  @param[in] string_to_transmit  String to be transmitted.
 *  @param[in] size                Size of the data array to be transmitted.
 */
void quasar_uart_transmit_string_irq(quasar_uart_selection_t uart_selection, char *string_to_transmit, uint32_t size);

/** @brief Retrieve received data from the associated FIFO buffer used for reception.
 *
 *  Received data is automatically pushed into the reception FIFO buffer using interrupts.
 *  This function pulls data from this FIFO buffer.
 *
 *  @note This function should be used only if the UART instance has been initialized with IRQ.
 *
 *  @param[in] uart_selection          Selected UART peripheral.
 *  @param[in] data_array_to_transmit  Array of data to be transmitted.
 *  @param[in] size                    Size of the data array to be transmitted.
 *  @return The received data.
 */
uint8_t quasar_uart_receive_irq(quasar_uart_selection_t uart_selection);

/** @brief Initiate UART transmission with DMA for a specified UART instance.
 *
 *  @note This function should be used only if the UART instance has been initialized without
 *        IRQ and if the DMA has to be initialized before.
 *
 *  @param[in] uart_selection  Selected UART peripheral.
 *  @param[in] data            Data to be transmitted.
 *  @param[in] size            Size of the data array to be transmitted.
 */
void quasar_uart_transmit_dma(quasar_uart_selection_t uart_selection, uint8_t *data, uint16_t size);

/** @brief Initiate UART reception with DMA for a specified UART instance.
 *
 *  @note This function should be used only if the UART instance has been initialized without
 *        IRQ and if the DMA has to be initialized before.
 *
 *  @param[in] uart_selection  Selected UART peripheral.
 *  @return The received data.
 */
uint8_t quasar_uart_receive_dma(quasar_uart_selection_t uart_selection);

/** @brief Transmit over UART using blocking method.
 *
 *  @note This function should be used only if the UART instance has been initialized without
 *        IRQ and if no DMA has been initialized.
 *
 *  @param[in] uart_selection  Selected UART peripheral.
 *  @param[in] data            Data to be transmitted.
 *  @param[in] size            Size of the data array to be transmitted.
 *  @param[in] timeout         Timeout period
 */
void quasar_uart_transmit_blocking(quasar_uart_selection_t uart_selection, uint8_t *data, uint16_t size,
                                   uint16_t timeout);

/** @brief Receive over UART using blocking method.
 *
 *  @note This function should be used only if the UART instance has been initialized without
 *        IRQ and if no DMA has been initialized.
 *
 *  @param[in] uart_selection  Selected UART peripheral.
 *  @param[in] timeout         Timeout period
 *  @return The received data.
 */
uint8_t quasar_uart_receive_blocking(quasar_uart_selection_t uart_selection, uint16_t timeout);

/** @brief Return the handle from the selected UART.
 *
 *  @param[in] uart_selection  Available UART selection.
 *  @return Selected UART handle.
 */
UART_HandleTypeDef *quasar_uart_get_selected_handle(quasar_uart_selection_t uart_selection);

/** @brief Return the instance from the selected UART.
 *
 *  @param[in] uart_selection  Available UART selection.
 *  @return Selected UART instance.
 */
USART_TypeDef *quasar_uart_get_instance(quasar_uart_selection_t uart_selection);

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_UART_H_ */
