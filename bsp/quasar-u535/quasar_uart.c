/** @file  quasar_uart.c
 *  @brief This module configure UART and provides functions to transmit and receive.
 *
 *  @note This driver supports UART in interrupt mode.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_uart.h"
#include "quasar_clock.h"
#include "quasar_def.h"
#include "quasar_error.h"
#include "quasar_fifo.h"

/* PRIVATE GLOBALS ************************************************************/
void (*uart1_rx_callback)(void) = NULL;
void (*lpuart1_rx_callback)(void) = NULL;
void (*uart3_rx_callback)(void) = NULL;
void (*uart4_rx_callback)(void) = NULL;
void (*uart5_rx_callback)(void) = NULL;
#if !defined(STM32U535xx)
void (*uart6_rx_callback)(void) = NULL;
#endif

/*! Each cell of the array is the associated FIFO buffer RX or TX for the selected UART.  */
static quasar_fifo_t quasar_uart_fifo_rx[_QUASAR_UART_SELECTION_COUNT];
static quasar_fifo_t quasar_uart_fifo_tx[_QUASAR_UART_SELECTION_COUNT];

/* Global peripheral handles to ensure DMA fonctionnalities. */
UART_HandleTypeDef uart_handle_usart1 = {
    .Instance = USART1,
    .gState = HAL_UART_STATE_READY,
    .RxState = HAL_UART_STATE_READY,
};
UART_HandleTypeDef uart_handle_lpuart1 = {
    .Instance = LPUART1,
    .gState = HAL_UART_STATE_READY,
    .RxState = HAL_UART_STATE_READY,
};
UART_HandleTypeDef uart_handle_usart3 = {
    .Instance = USART3,
    .gState = HAL_UART_STATE_READY,
    .RxState = HAL_UART_STATE_READY,
};
UART_HandleTypeDef uart_handle_uart4 = {
    .Instance = UART4,
    .gState = HAL_UART_STATE_READY,
    .RxState = HAL_UART_STATE_READY,
};
UART_HandleTypeDef uart_handle_uart5 = {
    .Instance = UART5,
    .gState = HAL_UART_STATE_READY,
    .RxState = HAL_UART_STATE_READY,
};
#if !defined(STM32U535xx)
UART_HandleTypeDef uart_handle_usart6 = {
    .Instance = USART6,
    .gState = HAL_UART_STATE_READY,
    .RxState = HAL_UART_STATE_READY,
};
#endif

/*! Depth of the USART/LPUART hardware RX FIFO, used to bound the drain loop in the handler. */
#define QUASAR_UART_HW_FIFO_DEPTH 8

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void uart_enable_clock(quasar_uart_selection_t uart_selection);
static void uart_disable_clock(quasar_uart_selection_t uart_selection);
static USART_TypeDef *uart_get_instance(quasar_uart_selection_t uart_selection);
static void uart_configure_protocol(USART_TypeDef *uart_instance, quasar_uart_config_t uart_config);
static void uart_unconfigure_protocol(USART_TypeDef *uart_instance);
static void uart_configure_parity(USART_TypeDef *uart_instance, quasar_uart_parity_t uart_parity);
static void uart_configure_stop(USART_TypeDef *uart_instance, quasar_uart_stop_t uart_stop);
static IRQn_Type uart_get_selected_irq(quasar_uart_selection_t uart_selection);
static void uart_irq_handler_routine(quasar_uart_selection_t uart_selection, UART_HandleTypeDef *uart_handle,
                                     void (*rx_irq_callback)(void));

/* TEMP DEBUG: 1 = echo bytes received on LPUART1 (AT/expansion UART) back out its TX,
 * so we can tell if RX physically delivers a byte at all. Set to 0 for release. */
#define LPUART1_RX_ECHO_DEBUG 0

/* PUBLIC FUNCTIONS ***********************************************************/
void quasar_uart_init(quasar_uart_config_t uart_config)
{
    USART_TypeDef *uart_instance = uart_get_instance(uart_config.uart_selection);
    IRQn_Type uart_irq;

    /* Guard: USART2/USART6 do not exist on STM32U535 — skip silently. */
    if (uart_instance == NULL) return;

    /* Initialize GPIOs used for UART. */
    quasar_gpio_init(uart_config.gpio_config_rx);
    quasar_gpio_init(uart_config.gpio_config_tx);

    /* Enable UART clock. */
    uart_enable_clock(uart_config.uart_selection);

    /* Configure UART protocol based on the configuration requested. */
    uart_configure_protocol(uart_instance, uart_config);

    /* Enable the transmitter, the receiver and the UART. */
    QUASAR_SET_BIT(uart_instance->CR1, USART_CR1_TE_Msk);
    QUASAR_SET_BIT(uart_instance->CR1, USART_CR1_RE_Msk);
    QUASAR_SET_BIT(uart_instance->CR1, USART_CR1_UE_Msk);

    /* Enable the UART receive interrupt flag and configure IRQ priority. */
    if (uart_config.irq_priority != QUASAR_IRQ_PRIORITY_NONE) {
        /* Initialize the two FIFOs used for UART transmission and reception. */
        quasar_fifo_init(&quasar_uart_fifo_tx[uart_config.uart_selection]);
        quasar_fifo_init(&quasar_uart_fifo_rx[uart_config.uart_selection]);

        /* Enable the interrupt flag when there is RX transmission. */
        QUASAR_SET_BIT(uart_instance->CR1, USART_CR1_RXNEIE_Msk);

        uart_irq = uart_get_selected_irq(uart_config.uart_selection);

        /* Set the UART priority. */
        NVIC_SetPriority(uart_irq, uart_config.irq_priority);

        /* Enable the UART interrupt. */
        NVIC_EnableIRQ(uart_irq);
    }
}

void quasar_uart_deinit(quasar_uart_config_t uart_config)
{
    USART_TypeDef *uart_instance = uart_get_instance(uart_config.uart_selection);
    IRQn_Type uart_irq;

    /* Guard: USART2/USART6 do not exist on STM32U535 — skip silently. */
    if (uart_instance == NULL) return;

    /* Disable the UART receive interrupt flag. */
    if (uart_config.irq_priority != QUASAR_IRQ_PRIORITY_NONE) {
        /* Disable the interrupt flag when there is RX transmission. */
        QUASAR_CLEAR_BIT(uart_instance->CR1, USART_CR1_RXNEIE_Msk);

        uart_irq = uart_get_selected_irq(uart_config.uart_selection);

        /* Disable the UART interrupt. */
        NVIC_DisableIRQ(uart_irq);
    }

    /* Disable the transmitter, the receiver and the UART. */
    QUASAR_CLEAR_BIT(uart_instance->CR1, USART_CR1_UE_Msk);
    QUASAR_CLEAR_BIT(uart_instance->CR1, USART_CR1_TE_Msk);
    QUASAR_CLEAR_BIT(uart_instance->CR1, USART_CR1_RE_Msk);

    uart_unconfigure_protocol(uart_instance);
    uart_disable_clock(uart_config.uart_selection);

    quasar_gpio_deinit(uart_config.gpio_config_rx.port, uart_config.gpio_config_rx.pin);
    quasar_gpio_deinit(uart_config.gpio_config_tx.port, uart_config.gpio_config_tx.pin);
}

void quasar_uart_set_rx_callback(quasar_uart_selection_t uart_selection, void (*callback)(void))
{
    switch (uart_selection) {
    case QUASAR_UART_SELECTION_USART1:
        uart1_rx_callback = callback;
        break;
    case QUASAR_UART_SELECTION_LPUART1:
        lpuart1_rx_callback = callback;
        break;
    case QUASAR_UART_SELECTION_USART3:
        uart3_rx_callback = callback;
        break;
    case QUASAR_UART_SELECTION_UART4:
        uart4_rx_callback = callback;
        break;
    case QUASAR_UART_SELECTION_UART5:
        uart5_rx_callback = callback;
        break;
#if !defined(STM32U535xx)
    case QUASAR_UART_SELECTION_USART6:
        uart6_rx_callback = callback;
        break;
#endif
    default:
        /* Unimplemented timer. */
        break;
    }
}

void quasar_uart_transmit_byte_irq(quasar_uart_selection_t uart_selection, uint8_t data_to_transmit)
{
    USART_TypeDef *uart_instance = uart_get_instance(uart_selection);
    if (uart_instance == NULL) return;

    quasar_it_enter_critical();
    /* Push each element of the array into the associated FIFO buffer. */
    quasar_fifo_push(&quasar_uart_fifo_tx[uart_selection], data_to_transmit);
    quasar_it_exit_critical();

    /* Set the interrupt flag for TX transmission. */
    QUASAR_SET_BIT(uart_instance->CR1, USART_CR1_TXEIE_Msk);
}

void quasar_uart_transmit_bytes_irq(quasar_uart_selection_t uart_selection, uint8_t *data_array_to_transmit,
                                    uint32_t size)
{
    USART_TypeDef *uart_instance = uart_get_instance(uart_selection);
    if (uart_instance == NULL) return;

    quasar_it_enter_critical();
    /* Push each element of the array into the associated FIFO buffer. */
    for (uint32_t i = 0; i < size; i++) {
        quasar_fifo_push(&quasar_uart_fifo_tx[uart_selection], data_array_to_transmit[i]);
    }
    quasar_it_exit_critical();

    /* Set the interrupt flag for TX transmission. */
    QUASAR_SET_BIT(uart_instance->CR1, USART_CR1_TXEIE_Msk);
}

void quasar_uart_transmit_string_irq(quasar_uart_selection_t uart_selection, char *string_to_transmit, uint32_t size)
{
    USART_TypeDef *uart_instance = uart_get_instance(uart_selection);
    if (uart_instance == NULL) return;

    quasar_it_enter_critical();
    /* Push each element of the array into the associated FIFO buffer. */
    for (uint32_t i = 0; i < size; i++) {
        quasar_fifo_push(&quasar_uart_fifo_tx[uart_selection], *string_to_transmit);
        string_to_transmit++;
    }
    quasar_it_exit_critical();

    /* Set the interrupt flag for TX transmission. */
    QUASAR_SET_BIT(uart_instance->CR1, USART_CR1_TXEIE_Msk);
}

uint8_t quasar_uart_receive_irq(quasar_uart_selection_t uart_selection)
{
    uint8_t received_data = 0;

    quasar_it_enter_critical();
    /* Pull one element from the associated FIFO buffer if it is not empty. */
    if (quasar_fifo_get_count(&quasar_uart_fifo_rx[uart_selection]) != 0) {
        quasar_fifo_pull(&quasar_uart_fifo_rx[uart_selection], &received_data);
    }
    quasar_it_exit_critical();

    return received_data;
}

void quasar_uart_transmit_dma(quasar_uart_selection_t uart_selection, uint8_t *data, uint16_t size,
                              quasar_bsp_status_t *err)
{
    *err = QUASAR_OK;

    UART_HandleTypeDef *uart_handle = quasar_uart_get_selected_handle(uart_selection);
    if (uart_handle == NULL) return;

    QUASAR_BSP_CHECK_ERROR(HAL_UART_Transmit_DMA(uart_handle, data, size) != HAL_OK, err, QUASAR_ERR_UART_TRANSMIT_DMA,
                           return);

    /* Tx process is ended, restore huart->gState to Ready */
    uart_handle->gState = HAL_UART_STATE_READY;
    /* Clear TxISR function pointer */
    uart_handle->TxISR = NULL;
}

uint8_t quasar_uart_receive_dma(quasar_uart_selection_t uart_selection)
{
    uint8_t received_data = 0;
    UART_HandleTypeDef *uart_handle = quasar_uart_get_selected_handle(uart_selection);
    if (uart_handle == NULL) return 0;

    if (HAL_UART_Receive_DMA(uart_handle, &received_data, 1) != HAL_OK) {
        return 0;
    }

    return received_data;
}

void quasar_uart_transmit_blocking(quasar_uart_selection_t uart_selection, uint8_t *data, uint16_t size,
                                   uint16_t timeout, quasar_bsp_status_t *err)
{
    *err = QUASAR_OK;

    UART_HandleTypeDef *uart_handle = quasar_uart_get_selected_handle(uart_selection);
    if (uart_handle == NULL) return;

    QUASAR_BSP_CHECK_ERROR(HAL_UART_Transmit(uart_handle, data, size, timeout) != HAL_OK, err,
                           QUASAR_ERR_UART_TRANSMIT_BLOCKING, return);
}

uint8_t quasar_uart_receive_blocking(quasar_uart_selection_t uart_selection, uint16_t timeout)
{
    UART_HandleTypeDef *uart_handle = quasar_uart_get_selected_handle(uart_selection);
    if (uart_handle == NULL) return 0;
    uint8_t received_data = 0;

    if (HAL_UART_Receive(uart_handle, &received_data, 1, timeout) != HAL_OK) {
        return 0;
    }

    return received_data;
}

UART_HandleTypeDef *quasar_uart_get_selected_handle(quasar_uart_selection_t uart_selection)
{
    UART_HandleTypeDef *uart_handle = {0};

    switch (uart_selection) {
    case QUASAR_UART_SELECTION_USART1:
        uart_handle = &uart_handle_usart1;
        break;
    case QUASAR_UART_SELECTION_LPUART1:
        uart_handle = &uart_handle_lpuart1;
        break;
    case QUASAR_UART_SELECTION_USART3:
        uart_handle = &uart_handle_usart3;
        break;
    case QUASAR_UART_SELECTION_UART4:
        uart_handle = &uart_handle_uart4;
        break;
    case QUASAR_UART_SELECTION_UART5:
        uart_handle = &uart_handle_uart5;
        break;
#if !defined(STM32U535xx)
    case QUASAR_UART_SELECTION_USART6:
        uart_handle = &uart_handle_usart6;
        break;
#endif
    default:
        /* Unimplemented timer. */
        break;
    }

    return uart_handle;
}

USART_TypeDef *quasar_uart_get_instance(quasar_uart_selection_t uart_selection)
{
    USART_TypeDef *uart_instance = NULL;

    switch (uart_selection) {
    case QUASAR_UART_SELECTION_USART1:
        uart_instance = USART1;
        break;
    case QUASAR_UART_SELECTION_LPUART1:
        uart_instance = LPUART1;
        break;
    case QUASAR_UART_SELECTION_USART3:
        uart_instance = USART3;
        break;
    case QUASAR_UART_SELECTION_UART4:
        uart_instance = UART4;
        break;
    case QUASAR_UART_SELECTION_UART5:
        uart_instance = UART5;
        break;
#if !defined(STM32U535xx)
    case QUASAR_UART_SELECTION_USART6:
        uart_instance = USART6;
        break;
#endif
    default:
        /* Unimplemented timer. */
        break;
    }

    return uart_instance;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Enables the clock for the selected UART.
 *
 *  @param[in] uart_selection  Selected UART to enable the clock.
 */
static void uart_enable_clock(quasar_uart_selection_t uart_selection)
{
    switch (uart_selection) {
    case QUASAR_UART_SELECTION_USART1:
        __HAL_RCC_USART1_CLK_ENABLE();
        break;
    case QUASAR_UART_SELECTION_LPUART1:
        __HAL_RCC_LPUART1_CLK_ENABLE();
        break;
    case QUASAR_UART_SELECTION_USART3:
        __HAL_RCC_USART3_CLK_ENABLE();
        break;
    case QUASAR_UART_SELECTION_UART4:
        __HAL_RCC_UART4_CLK_ENABLE();
        break;
    case QUASAR_UART_SELECTION_UART5:
        __HAL_RCC_UART5_CLK_ENABLE();
        break;
#if !defined(STM32U535xx)
    case QUASAR_UART_SELECTION_USART6:
        __HAL_RCC_USART6_CLK_ENABLE();
        break;
#endif
    default:
        /* Unimplemented timer. */
        break;
    }
}

/** @brief Disables the clock for the selected UART.
 *
 *  @param[in] uart_selection  Selected UART to disable the clock.
 */
static void uart_disable_clock(quasar_uart_selection_t uart_selection)
{
    switch (uart_selection) {
    case QUASAR_UART_SELECTION_USART1:
        __HAL_RCC_USART1_CLK_DISABLE();
        break;
    case QUASAR_UART_SELECTION_LPUART1:
        __HAL_RCC_LPUART1_CLK_DISABLE();
        break;
    case QUASAR_UART_SELECTION_USART3:
        __HAL_RCC_USART3_CLK_DISABLE();
        break;
    case QUASAR_UART_SELECTION_UART4:
        __HAL_RCC_UART4_CLK_DISABLE();
        break;
    case QUASAR_UART_SELECTION_UART5:
        __HAL_RCC_UART5_CLK_DISABLE();
        break;
#if !defined(STM32U535xx)
    case QUASAR_UART_SELECTION_USART6:
        __HAL_RCC_USART6_CLK_DISABLE();
        break;
#endif
    default:
        /* Unimplemented timer. */
        break;
    }
}

/** @brief Return the instance to the selected UART.
 *
 *  @param[in] uart_selection  Selected UART to get associated instance.
 *  @return Selected UART instance.
 */
static USART_TypeDef *uart_get_instance(quasar_uart_selection_t uart_selection)
{
    USART_TypeDef *uart_instance = NULL;

    switch (uart_selection) {
    case QUASAR_UART_SELECTION_USART1:
        uart_instance = USART1;
        break;
    case QUASAR_UART_SELECTION_LPUART1:
        uart_instance = LPUART1;
        break;
    case QUASAR_UART_SELECTION_USART3:
        uart_instance = USART3;
        break;
    case QUASAR_UART_SELECTION_UART4:
        uart_instance = UART4;
        break;
    case QUASAR_UART_SELECTION_UART5:
        uart_instance = UART5;
        break;
#if !defined(STM32U535xx)
    case QUASAR_UART_SELECTION_USART6:
        uart_instance = USART6;
        break;
#endif
    default:
        /* Unimplemented timer. */
        break;
    }

    return uart_instance;
}

/** @brief Configure the UART protocol.
 *
 *  @param[in] uart_instance  Selected UART instance for which to configure the protocol.
 *  @param[in] uart_config    The UART configuration used to set up the protocol.
 */
static void uart_configure_protocol(USART_TypeDef *uart_instance, quasar_uart_config_t uart_config)
{
    uint32_t clock_frequency = 0;

    /* Disable the UART while configuring settings. */
    QUASAR_CLEAR_BIT(uart_instance->CR1, USART_CR1_UE_Msk);

    /* Configure the oversampling mode at 16 bits */
    QUASAR_CLEAR_BIT(uart_instance->CR1, USART_CR1_OVER8_Msk);

    /* Enable the 8-byte hardware RX/TX FIFO. Without it the handler has to move every byte
     * out of RDR within one byte-time (~87 us at 115200) or the byte is lost to an overrun.
     * The AT/expansion UART sits at NVIC priority 15 so radio and audio work preempts it at
     * will, and back-to-back input (a pasted command) was losing about a third of its
     * characters. The FIFO turns that deadline into eight byte-times. FIFOEN may only be
     * written while UE is cleared, which is the case here. */
    QUASAR_SET_BIT(uart_instance->CR1, USART_CR1_FIFOEN_Msk);

    /* Configure the wordlength at 8 */
    QUASAR_CLEAR_BIT(uart_instance->CR1, USART_CR1_M0_Msk);
    QUASAR_CLEAR_BIT(uart_instance->CR1, USART_CR1_M1_Msk);

    /* Configure the parity bits based on the UART configuration. */
    uart_configure_parity(uart_instance, uart_config.parity);

    /* Configure the stop bits based on the UART configuration. */
    uart_configure_stop(uart_instance, uart_config.stop);

    /* Get the system clock frequency. */
    clock_frequency = quasar_clock_get_system_clock_freq();

    /* LPUART1 BRR uses a 256x multiplier to achieve fine resolution at low baud rates. */
    if (uart_instance == LPUART1) {
        uart_instance->BRR = (uint32_t)((256ULL * clock_frequency) / uart_config.baud_rate);
    } else {
        uart_instance->BRR = (uint32_t)(clock_frequency / uart_config.baud_rate);
    }
}

/** @brief Unconfigure the UART protocol.
 *
 *  @param[in] uart_instance  Selected UART instance for which to unconfigure the protocol.
 */
static void uart_unconfigure_protocol(USART_TypeDef *uart_instance)
{
    /* Deconfigure the parity bits. */
    QUASAR_CLEAR_BIT(uart_instance->CR1, USART_CR1_PCE_Msk);
    QUASAR_CLEAR_BIT(uart_instance->CR1, USART_CR1_PS_Msk);
    /* Unconfigure the stop bits. */
    QUASAR_CLEAR_BIT(uart_instance->CR2, USART_CR2_STOP_0);
    QUASAR_CLEAR_BIT(uart_instance->CR2, USART_CR2_STOP_1);

    /* Unconfigure the baud rate register. */
    uart_instance->BRR = (uint16_t)0;
}

/** @brief Configure the parity bits of the UART protocol.
 *
 *  @param[in] uart_instance  Selected UART instance for which to configure the protocol.
 *  @param[in] uart_parity    The selected parity bits configuration.
 */
static void uart_configure_parity(USART_TypeDef *uart_instance, quasar_uart_parity_t uart_parity)
{
    if (uart_parity == QUASAR_UART_PARITY_NONE) {
        QUASAR_CLEAR_BIT(uart_instance->CR1, USART_CR1_PCE_Msk);
    } else if (uart_parity == QUASAR_UART_PARITY_EVEN) {
        QUASAR_SET_BIT(uart_instance->CR1, USART_CR1_PCE_Msk);
        QUASAR_CLEAR_BIT(uart_instance->CR1, USART_CR1_PS_Msk);
    } else {
        QUASAR_SET_BIT(uart_instance->CR1, USART_CR1_PCE_Msk);
        QUASAR_SET_BIT(uart_instance->CR1, USART_CR1_PS_Msk);
    }
}

/** @brief Configure the stop bits of the UART protocol.
 *
 *  @param[in] uart_instance  Selected UART instance for which to configure the protocol.
 *  @param[in] uart_stop      The selected stop bits configuration.
 */
static void uart_configure_stop(USART_TypeDef *uart_instance, quasar_uart_stop_t uart_stop)
{
    if (uart_stop == QUASAR_UART_STOP_BITS_1B) {
        QUASAR_CLEAR_BIT(uart_instance->CR2, USART_CR2_STOP_0);
        QUASAR_CLEAR_BIT(uart_instance->CR2, USART_CR2_STOP_1);
    } else if (uart_stop == QUASAR_UART_STOP_BITS_0B5) {
        QUASAR_SET_BIT(uart_instance->CR2, USART_CR2_STOP_0);
        QUASAR_CLEAR_BIT(uart_instance->CR2, USART_CR2_STOP_1);
    } else if (uart_stop == QUASAR_UART_STOP_BITS_2B) {
        QUASAR_CLEAR_BIT(uart_instance->CR2, USART_CR2_STOP_0);
        QUASAR_SET_BIT(uart_instance->CR2, USART_CR2_STOP_1);
    } else {
        QUASAR_SET_BIT(uart_instance->CR2, USART_CR2_STOP_0);
        QUASAR_SET_BIT(uart_instance->CR2, USART_CR2_STOP_1);
    }
}

/** @brief Return the selected UART's global interrupt.
 *
 *  @param[in] uart_selection  Selected UART to get associated global interrupt.
 *  @return Selected UART global interrupt.
 */
static IRQn_Type uart_get_selected_irq(quasar_uart_selection_t uart_selection)
{
    IRQn_Type uart_irq = {0};

    switch (uart_selection) {
    case QUASAR_UART_SELECTION_USART1:
        uart_irq = USART1_IRQn;
        break;
    case QUASAR_UART_SELECTION_LPUART1:
        uart_irq = LPUART1_IRQn;
        break;
    case QUASAR_UART_SELECTION_USART3:
        uart_irq = USART3_IRQn;
        break;
    case QUASAR_UART_SELECTION_UART4:
        uart_irq = UART4_IRQn;
        break;
    case QUASAR_UART_SELECTION_UART5:
        uart_irq = UART5_IRQn;
        break;
#if !defined(STM32U535xx)
    case QUASAR_UART_SELECTION_USART6:
        uart_irq = USART6_IRQn;
        break;
#endif
    default:
        /* Unimplemented timer. */
        break;
    }

    return uart_irq;
}

/** @brief UART interrupt routine for reception and transmission.
 *
 *  @note  The USART interrupt routines retrieve the received data directly from the
 *  reception register (RDR) and temporarily store it in a reception
 *  FIFO (which can be fetched using the UART data reception function afterward).
 *
 *  When the TXEIE flag is raised in the UART transmission functions, they also
 *  move the data from the transmission FIFO to the transmission register (TDR)
 *  for sending.
 *
 *  These routines have access to the static private variables "quasar_uart_fifo_rx"
 *  and "quasar_uart_fifo_tx", which contain the associated reception or
 *  transmission FIFO for the UART instance in each element of the array.
 *
 *  @param[in] uart_selection   Selected UART selection.
 *  @param[in] uart_handle      Selected UART handle.
 *  @param[in] rx_irq_callback  Function pointer to be called when a byte is received via UART IRQ.
 */
static void uart_irq_handler_routine(quasar_uart_selection_t uart_selection, UART_HandleTypeDef *uart_handle,
                                     void (*rx_irq_callback)(void))
{
    uint32_t count = 0;
    uint8_t new_data = 0;

    /* In case of RDR is not empty a reception occurs. Drain the whole hardware FIFO in one
     * entry: the flag stays asserted while bytes remain, and emptying it here is what buys
     * the tolerance to a late interrupt. The iteration cap means a flag that somehow never
     * clears cannot storm this handler. */
    for (uint32_t fifo_slot = 0;
         (fifo_slot < QUASAR_UART_HW_FIFO_DEPTH) &&
         ((uart_handle->Instance->ISR & USART_ISR_RXNE) == USART_ISR_RXNE) &&
         ((uart_handle->Instance->CR3 & USART_CR3_DMAR) != USART_CR3_DMAR);
         fifo_slot++) {
        /* Transfer the contents of RDR into the associated FIFO buffer. */
        uint8_t rx_byte = (uint8_t)uart_handle->Instance->RDR;
        quasar_fifo_push(&quasar_uart_fifo_rx[uart_selection], rx_byte);

#if LPUART1_RX_ECHO_DEBUG
        /* TEMP DEBUG: echo the byte straight back on the AT/expansion UART TX. If you see
         * your typed characters mirrored, RX physically works and the fault is upstream
         * (parser/wiring); if nothing echoes, RX never delivered a byte (pin/init). */
        if (uart_selection == QUASAR_UART_SELECTION_LPUART1) {
            while ((uart_handle->Instance->ISR & USART_ISR_TXE) != USART_ISR_TXE) {
                /* wait for TX data register empty */
            }
            uart_handle->Instance->TDR = rx_byte;
        }
#endif

        if (rx_irq_callback != NULL) {
            rx_irq_callback();
        }
    }
    /* In case of a transmission, we check if there are data to be transmitted. */
    if (((uart_handle->Instance->ISR & USART_ISR_TXE) == USART_ISR_TXE) &&
        ((uart_handle->Instance->CR1 & USART_CR1_TXEIE) == USART_CR1_TXEIE)) {
        /* Retrieve the count of the number of elements to be transmitted.  */
        count = quasar_fifo_get_count(&quasar_uart_fifo_tx[uart_selection]);
        if (count > 0) {
            /* Transfer what is in the associated FIFO buffer into the TDR. */
            quasar_fifo_pull(&quasar_uart_fifo_tx[uart_selection], &new_data);
            uart_handle->Instance->TDR = new_data;
        } else if (count <= 0) {
            /* Disable the transmission interrupt because everything has been transmitted. */
            QUASAR_CLEAR_BIT(uart_handle->Instance->CR1, USART_CR1_TXEIE_Msk);
        }
    }
    /* In case of a DMA or Blocking mode transmission, the TCIE flag is used instead of TXEIE, and HAL is utilized. */
    if (((uart_handle->Instance->ISR & USART_ISR_TC) == USART_ISR_TC) &&
        ((uart_handle->Instance->CR1 & USART_CR1_TCIE) == USART_CR1_TCIE)) {
        /* Disable the UART Transmit Complete Interrupt */
        QUASAR_CLEAR_BIT(uart_handle->Instance->CR1, USART_CR1_TCIE);
        /* Tx process is ended, restore huart->gState to Ready */
        uart_handle->gState = HAL_UART_STATE_READY;
        /* Clear TxISR function pointer */
        uart_handle->TxISR = NULL;
    }

    if ((uart_handle->Instance->ISR & USART_ISR_ORE) == USART_ISR_ORE) {
        /* Clear the overrun flag by writing ORECF into ICR (write-1-to-clear). The old code
         * cleared this mask in CR1 instead, which never cleared ORE (and wiped TE): once an
         * overrun latched, RXNE stopped firing and RX died permanently. */
        uart_handle->Instance->ICR = USART_ICR_ORECF;
    }
}

/* ST HAL FUNCTIONS IMPLEMENTATION ********************************************/
/** @brief This function handles USART 1 interrupt.
 */
void USART1_IRQHandler(void)
{
    uart_irq_handler_routine(QUASAR_UART_SELECTION_USART1, &uart_handle_usart1, uart1_rx_callback);
}

/** @brief This function handles LPUART 1 interrupt.
 */
void LPUART1_IRQHandler(void)
{
    uart_irq_handler_routine(QUASAR_UART_SELECTION_LPUART1, &uart_handle_lpuart1, lpuart1_rx_callback);
}

/** @brief This function handles USART 3 interrupt.
 */
void USART3_IRQHandler(void)
{
    uart_irq_handler_routine(QUASAR_UART_SELECTION_USART3, &uart_handle_usart3, uart3_rx_callback);
}

/** @brief This function handles UART 4 interrupt.
 */
void UART4_IRQHandler(void)
{
    uart_irq_handler_routine(QUASAR_UART_SELECTION_UART4, &uart_handle_uart4, uart4_rx_callback);
}

/** @brief This function handles UART 5 interrupt.
 */
void UART5_IRQHandler(void)
{
    uart_irq_handler_routine(QUASAR_UART_SELECTION_UART5, &uart_handle_uart5, uart5_rx_callback);
}

#if !defined(STM32U535xx)
/** @brief This function handles USART 6 interrupt.
 */
void USART6_IRQHandler(void)
{
    uart_irq_handler_routine(QUASAR_UART_SELECTION_USART6, &uart_handle_usart6, uart6_rx_callback);
}
#endif
