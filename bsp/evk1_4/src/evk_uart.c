/** @file  evk_uart.c
 *  @brief This module controls UART features of SPARK EVK board.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "evk_it.h"
#include "evk_uart.h"
#include "stm32g4xx_hal_dma.h"

/* CONSTANTS ******************************************************************/
#define BYTE_SIZE 1

/* PRIVATE GLOBALS ************************************************************/
UART_HandleTypeDef hlp_uart;
UART_HandleTypeDef huart4;
DMA_HandleTypeDef  hdma_lpuart1_tx;
DMA_HandleTypeDef  hdma_lpuart1_rx;
void (*swd_uart_tx_callback)(void) = NULL;
void (*swd_uart_rx_callback)(void) = NULL;
void (*exp_uart_rx_callback)(void) = NULL;
void (*exp_uart_tx_callback)(void) = NULL;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void uart_receive_dma(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
static void uart_dma_receive_complete(DMA_HandleTypeDef *hdma);

/* PUBLIC FUNCTIONS ***********************************************************/
void evk_exp_uart_init(void)
{
    huart4.Instance                    = UART4;
    huart4.Init.BaudRate               = 31250;
    huart4.Init.WordLength             = UART_WORDLENGTH_8B;
    huart4.Init.StopBits               = UART_STOPBITS_1;
    huart4.Init.Parity                 = UART_PARITY_NONE;
    huart4.Init.Mode                   = UART_MODE_TX_RX;
    huart4.Init.HwFlowCtl              = UART_HWCONTROL_NONE;
    huart4.Init.OverSampling           = UART_OVERSAMPLING_16;
    huart4.Init.OneBitSampling         = UART_ONE_BIT_SAMPLE_DISABLE;
    huart4.Init.ClockPrescaler         = UART_PRESCALER_DIV1;
    huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart4) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_SetTxFifoThreshold(&huart4, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_SetRxFifoThreshold(&huart4, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_DisableFifoMode(&huart4) != HAL_OK) {
        Error_Handler();
    }
}

void evk_exp_lpuart_init(void)
{
    hlp_uart.Instance                    = LPUART1;
    hlp_uart.Init.BaudRate               = 115200;
    hlp_uart.Init.WordLength             = UART_WORDLENGTH_8B;
    hlp_uart.Init.StopBits               = UART_STOPBITS_1;
    hlp_uart.Init.Parity                 = UART_PARITY_NONE;
    hlp_uart.Init.Mode                   = UART_MODE_TX_RX;
    hlp_uart.Init.HwFlowCtl              = UART_HWCONTROL_NONE;
    hlp_uart.Init.OneBitSampling         = UART_ONE_BIT_SAMPLE_DISABLE;
    hlp_uart.Init.ClockPrescaler         = UART_PRESCALER_DIV1;
    hlp_uart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_SWAP_INIT;
    hlp_uart.AdvancedInit.Swap           = UART_ADVFEATURE_SWAP_ENABLE;
    if (HAL_UART_Init(&hlp_uart) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_SetTxFifoThreshold(&hlp_uart, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_SetRxFifoThreshold(&hlp_uart, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_UARTEx_DisableFifoMode(&hlp_uart) != HAL_OK) {
        Error_Handler();
    }
}

void evk_exp_lpuart_update_config(void)
{
     if (UART_SetConfig(&hlp_uart) != HAL_OK) {
        Error_Handler();
     }
}

void evk_uart_swd_write_blocking(uint8_t *data, uint16_t size)
{
    HAL_UART_Transmit(&hlp_uart, data, size, LOG_UART_TIMEOUT_MS);
}

void evk_uart_swd_write_non_blocking(uint8_t *data, uint16_t size)
{
    HAL_UART_Transmit_DMA(&hlp_uart, data, size);
}

void evk_uart_swd_read_blocking(uint8_t *data, uint16_t size)
{
    HAL_UART_Receive(&hlp_uart, data, size, LOG_UART_TIMEOUT_MS);
}

void evk_uart_swd_read_byte_non_blocking(uint8_t *data)
{
    uart_receive_dma(&hlp_uart, data, BYTE_SIZE);
}

void evk_uart_exp_set_tx_callback(void (*callback)(void))
{
    exp_uart_tx_callback = callback;
}

void evk_uart_exp_set_rx_callback(void (*callback)(void))
{
    exp_uart_rx_callback = callback;
}

void evk_uart_exp_write_non_blocking_interrupt(uint8_t *data, uint16_t size)
{
    HAL_UART_Transmit_IT(&huart4, data, size);
}

void evk_uart_exp_read_non_blocking_interrupt(uint8_t *data, uint16_t size)
{
    HAL_UART_Receive_IT(&huart4, data, size);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &hlp_uart) {
        if (swd_uart_rx_callback != NULL) {
            swd_uart_rx_callback();
        }
    } else {
        if (exp_uart_rx_callback != NULL) {
            exp_uart_rx_callback();
        }
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &hlp_uart) {
        if (swd_uart_tx_callback != NULL) {
            swd_uart_tx_callback();
        }
    } else {
        if (exp_uart_tx_callback != NULL) {
            exp_uart_tx_callback();
        }
    }
}

void evk_uart_swd_set_tx_callback(void (*callback)(void))
{
    swd_uart_tx_callback = callback;
}

void evk_uart_swd_set_rx_callback(void (*callback)(void))
{
    swd_uart_rx_callback = callback;
}

void evk_uart_swd_dma_rx_enter_critical(void)
{
    NVIC_DisableIRQ(NVIC_LPUART_RX_CPLT);
}

void evk_uart_swd_dma_rx_exit_critical(void)
{
    NVIC_EnableIRQ(NVIC_LPUART_RX_CPLT);
}

void evk_uart_swd_stop_read_non_blocking(void)
{
    __HAL_DMA_DISABLE(&hdma_lpuart1_rx);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize the UART pins.
 *
 *  @param[in] huart  UART handler pointer.
 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    if (huart->Instance == LPUART1) {
        /* Peripheral clock enable */
        __HAL_RCC_LPUART1_CLK_ENABLE();

        /* LPUART1 GPIO Configuration
         * PC0     ------> LPUART1_RX
         * PC1     ------> LPUART1_TX
         */
        GPIO_InitStruct.Pin       = GPIO_PIN_0 | GPIO_PIN_1;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF8_LPUART1;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        /* LPUART1 DMA Init */
        __HAL_RCC_DMAMUX1_CLK_ENABLE();
        __HAL_RCC_DMA1_CLK_ENABLE();

        /* LPUART1_TX Init */
        hdma_lpuart1_tx.Instance                 = DMA1_CHANNEL_LPUART_TX;
        hdma_lpuart1_tx.Init.Request             = DMA_REQUEST_LPUART1_TX;
        hdma_lpuart1_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
        hdma_lpuart1_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
        hdma_lpuart1_tx.Init.MemInc              = DMA_MINC_ENABLE;
        hdma_lpuart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_lpuart1_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
        hdma_lpuart1_tx.Init.Mode                = DMA_NORMAL;
        hdma_lpuart1_tx.Init.Priority            = DMA_PRIORITY_LOW;

        if (HAL_DMA_Init(&hdma_lpuart1_tx) != HAL_OK) {
            Error_Handler();
        }

        hdma_lpuart1_rx.Instance                 = DMA1_CHANNEL_LPUART_RX;
        hdma_lpuart1_rx.Init.Request             = DMA_REQUEST_LPUART1_RX;
        hdma_lpuart1_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
        hdma_lpuart1_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
        hdma_lpuart1_rx.Init.MemInc              = DMA_MINC_ENABLE;
        hdma_lpuart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_lpuart1_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
        hdma_lpuart1_rx.Init.Mode                = DMA_NORMAL;
        hdma_lpuart1_rx.Init.Priority            = DMA_PRIORITY_LOW;

        if (HAL_DMA_Init(&hdma_lpuart1_rx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(huart, hdmatx, hdma_lpuart1_tx);
        __HAL_LINKDMA(huart, hdmarx, hdma_lpuart1_rx);

        HAL_NVIC_SetPriority(NVIC_LPUART_IRQ, PRIO_LPUART_IRQ, 0);
        HAL_NVIC_EnableIRQ(NVIC_LPUART_IRQ);

        /* DMA interrupt init */
        /* DMA1_Channel5_IRQn interrupt configuration */
        HAL_NVIC_SetPriority(NVIC_LPUART_TX_CPLT, PRIO_LPUART_DMA_TX_CPLT, 0);
        HAL_NVIC_EnableIRQ(NVIC_LPUART_TX_CPLT);

        HAL_NVIC_SetPriority(NVIC_LPUART_RX_CPLT, PRIO_LPUART_DMA_RX_CPLT, 0);
        HAL_NVIC_EnableIRQ(NVIC_LPUART_RX_CPLT);
    }

    if (huart->Instance == UART4) {

        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_UART4;
        PeriphClkInit.Uart4ClockSelection = RCC_UART4CLKSOURCE_PCLK1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        /* Peripheral clock enable */
        __HAL_RCC_UART4_CLK_ENABLE();

        __HAL_RCC_GPIOC_CLK_ENABLE();
        /* UART4 GPIO Configuration
         * PC10    ------> UART4_TX
         * PC11    ------> UART4_RX
         */
        GPIO_InitStruct.Pin       = EXP_PC10_PIN | EXP_PC11_PIN;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF5_UART4;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        /* UART4 interrupt Init */
        HAL_NVIC_SetPriority(UART4_IRQn, PRIO_UART4_IRQ, 0);
        HAL_NVIC_EnableIRQ(UART4_IRQn);
    }
}

/** @brief De-initializes the UART pins.
 *
 *  @param[in] huart  UART handler pointer.
 */
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    if (huart->Instance == LPUART1) {
        /* Peripheral clock disable */
        __HAL_RCC_LPUART1_CLK_DISABLE();

        /* LPUART1 GPIO Configuration
         * PC0     ------> LPUART1_RX
         * PC1     ------> LPUART1_TX
         */
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_0 | GPIO_PIN_1);
    }

    if (huart->Instance == UART4) {
        /* Peripheral clock disable */
        __HAL_RCC_UART4_CLK_DISABLE();

        /* UART4 GPIO Configuration
         * PC10    ------> UART4_TX
         * PC11    ------> UART4_RX
         */
        HAL_GPIO_DeInit(GPIOC, EXP_PC10_PIN | EXP_PC11_PIN);

        /* UART4 interrupt DeInit */
        HAL_NVIC_DisableIRQ(UART4_IRQn);
    }
}

/** @brief Receive an amount of data in DMA mode.
 *
 *  @param[in]  huart  UART handle.
 *  @param[out] pData  Pointer to data buffer.
 *  @param[in]  Size   Amount of data elements to be received.
 */
static void uart_receive_dma(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
    /* Set Reception type to Standard reception */
    huart->ReceptionType = HAL_UART_RECEPTION_STANDARD;

    /* Set the UART DMA transfer complete callback */
    huart->hdmarx->XferCpltCallback = uart_dma_receive_complete;

    /* Disable the peripheral */
    __HAL_DMA_DISABLE(huart->hdmarx);

    /* Clear all flags */
    huart->hdmarx->DmaBaseAddress->IFCR = (DMA_ISR_GIF1 << (huart->hdmarx->ChannelIndex & 0x1FU));

    /* Configure DMA Channel data length */
    huart->hdmarx->Instance->CNDTR = Size;

    /* Configure DMA Channel source address */
    huart->hdmarx->Instance->CPAR = (uint32_t)&huart->Instance->RDR;

    /* Configure DMA Channel destination address */
    huart->hdmarx->Instance->CMAR = (uint32_t)pData;

    __HAL_DMA_DISABLE_IT(huart->hdmarx, DMA_IT_HT);
    __HAL_DMA_DISABLE_IT(huart->hdmarx, DMA_IT_TE);
    __HAL_DMA_ENABLE_IT(huart->hdmarx, (DMA_IT_TC));

    /* Enable the Peripheral */
    __HAL_DMA_ENABLE(huart->hdmarx);

    /* Enable the UART Parity Error Interrupt */
    SET_BIT(huart->Instance->CR1, USART_CR1_PEIE);

    /* Enable the UART Error Interrupt: (Frame error, noise error, overrun error) */
    SET_BIT(huart->Instance->CR3, USART_CR3_EIE);

    /* Enable the DMA transfer for the receiver request by setting the DMAR bit
     * in the UART CR3 register
     */
    SET_BIT(huart->Instance->CR3, USART_CR3_DMAR);
}

/** @brief DMA XferCpltCallback function
 *
 *  @param[in] hdma  UART DMA RX handle.
 */
static void uart_dma_receive_complete(DMA_HandleTypeDef *hdma)
{
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)(hdma->Parent);

    huart->RxXferCount = 0U;

    /* Disable PE and ERR (Frame error, noise error, overrun error) interrupts */
    CLEAR_BIT(huart->Instance->CR1, USART_CR1_PEIE);
    CLEAR_BIT(huart->Instance->CR3, USART_CR3_EIE);

    /* Disable the DMA transfer for the receiver request by resetting the DMAR bit
     * in the UART CR3 register
     */
    CLEAR_BIT(huart->Instance->CR3, USART_CR3_DMAR);
    huart->RxCpltCallback(huart);
}
