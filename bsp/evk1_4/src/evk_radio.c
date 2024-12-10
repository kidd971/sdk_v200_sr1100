/** @file  evk_radio.c
 *  @brief This module controls the peripherals for the SR10x0 radio.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "evk_def.h"
#include "evk_clock.h"
#include "evk_it.h"
#include "evk_radio.h"

/* PRIVATE GLOBALS ************************************************************/
SPI_HandleTypeDef hradio_spi;
DMA_HandleTypeDef hradio_dma_spi_rx;
DMA_HandleTypeDef hradio_dma_spi_tx;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void init_radio_irq_gpio(void);
static void init_radio_shutdown_gpio(void);
static void init_radio_reset_gpio(void);
static void init_radio_debug_en_gpio(void);
static void init_radio_spi_peripheral(void);
static void init_radio_dma_controller(void);
static void init_radio_pendsv(uint32_t pendsv_prio);

/* PUBLIC FUNCTIONS ***********************************************************/
void evk_radio_peripherals_init(uint32_t pendsv_prio)
{
    __HAL_RCC_DMAMUX1_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
    init_radio_dma_controller();
    init_radio_irq_gpio();
    init_radio_shutdown_gpio();
    init_radio_reset_gpio();
    init_radio_debug_en_gpio();
    init_radio_spi_peripheral();
    init_radio_pendsv(pendsv_prio);
    evk_clock_ext_osc_init();
    evk_radio_set_reset_pin();
    evk_radio_reset_reset_pin();
}

bool evk_radio_read_irq_pin(void)
{
    if (HAL_GPIO_ReadPin(RADIO_IRQ_PORT, RADIO_IRQ_PIN)) {
        return true;
    } else {
        return false;
    }
}

void evk_radio_enable_irq_it(void)
{
    SET_BIT(EXTI->IMR1, RADIO_IRQ_PIN);
}

void evk_radio_disable_irq_it(void)
{
    CLEAR_BIT(EXTI->IMR1, RADIO_IRQ_PIN);
    SET_BIT(EXTI->PR1, RADIO_IRQ_PIN);
}

void evk_radio_enable_dma_irq_it(void)
{
    NVIC_EnableIRQ(NVIC_RADIO_DMA_RX_CPLT);
}

void evk_radio_disable_dma_irq_it(void)
{
    NVIC_DisableIRQ(NVIC_RADIO_DMA_RX_CPLT);
}

void evk_radio_set_shutdown_pin(void)
{
    HAL_GPIO_WritePin(RADIO_SHUTDOWN_PORT, RADIO_SHUTDOWN_PIN, GPIO_PIN_SET);
}

void evk_radio_reset_shutdown_pin(void)
{
    HAL_GPIO_WritePin(RADIO_SHUTDOWN_PORT, RADIO_SHUTDOWN_PIN, GPIO_PIN_RESET);
}

void evk_radio_set_reset_pin(void)
{
    HAL_GPIO_WritePin(RADIO_RESET_PORT, RADIO_RESET_PIN, GPIO_PIN_SET);
}

void evk_radio_reset_reset_pin(void)
{
    HAL_GPIO_WritePin(RADIO_RESET_PORT, RADIO_RESET_PIN, GPIO_PIN_RESET);
}

void evk_radio_spi_set_cs(void)
{
    (RADIO_CS_PORT)->BSRR = (uint32_t)RADIO_CS_PIN;
}

void evk_radio_spi_reset_cs(void)
{
    (RADIO_CS_PORT)->BRR = (uint32_t)RADIO_CS_PIN;
}

void evk_radio_set_debug_en(void)
{
    HAL_GPIO_WritePin(RADIO_DEBUG_EN_PORT, RADIO_DEBUG_EN_PIN, GPIO_PIN_SET);
}

void evk_radio_reset_debug_en(void)
{
    HAL_GPIO_WritePin(RADIO_DEBUG_EN_PORT, RADIO_DEBUG_EN_PIN, GPIO_PIN_RESET);
}

void evk_radio_spi_write_blocking(uint8_t *data, uint8_t size)
{
    if (hradio_spi.State != HAL_SPI_STATE_READY) {
        return;
    }

    hradio_spi.State = HAL_SPI_STATE_BUSY_TX_RX;

    if ((hradio_spi.Instance->CR1 & SPI_CR1_SPE) != SPI_CR1_SPE) {
        /* Enable SPI peripheral */
        SET_BIT(hradio_spi.Instance->CR1, SPI_CR1_SPE);
    }
    while (size--) {
        while (!(hradio_spi.Instance->SR & SPI_SR_TXE))
            ;
        *(volatile uint8_t *)&hradio_spi.Instance->DR = *data++;
        while (!(hradio_spi.Instance->SR & SPI_SR_RXNE))
            ;
        *(volatile uint8_t *)&hradio_spi.Instance->DR;
    }

    hradio_spi.State = HAL_SPI_STATE_READY;
}

void evk_radio_spi_read_blocking(uint8_t *data, uint8_t size)
{
    if (hradio_spi.State != HAL_SPI_STATE_READY) {
        return;
    }

    hradio_spi.State = HAL_SPI_STATE_BUSY_TX_RX;
    if ((hradio_spi.Instance->CR1 & SPI_CR1_SPE) != SPI_CR1_SPE) {
        /* Enable SPI peripheral */
        SET_BIT(hradio_spi.Instance->CR1, SPI_CR1_SPE);
    }
    while (size--) {
        while (!(hradio_spi.Instance->SR & SPI_SR_TXE));
        *(volatile uint8_t *)&hradio_spi.Instance->DR = *data;
        while (!(hradio_spi.Instance->SR & SPI_SR_RXNE));
        *data++ = *(volatile uint8_t *)&hradio_spi.Instance->DR;
    }

    hradio_spi.State = HAL_SPI_STATE_READY;
}

void evk_radio_spi_transfer_full_duplex_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    if (hradio_spi.State != HAL_SPI_STATE_READY) {
        return;
    }

    hradio_spi.State = HAL_SPI_STATE_BUSY_TX_RX;

    if ((hradio_spi.Instance->CR1 & SPI_CR1_SPE) != SPI_CR1_SPE) {
        /* Enable SPI peripheral */
        SET_BIT(hradio_spi.Instance->CR1, SPI_CR1_SPE);
    }
    while (size--) {
        while (!(hradio_spi.Instance->SR & SPI_SR_TXE));
        *(volatile uint8_t *)&hradio_spi.Instance->DR = *tx_data++;
        while (!(hradio_spi.Instance->SR & SPI_SR_RXNE));
        *rx_data++ = *(volatile uint8_t *)&hradio_spi.Instance->DR;
    }

    hradio_spi.State = HAL_SPI_STATE_READY;
}

void evk_radio_spi_transfer_full_duplex_non_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    SPI_HandleTypeDef *spi_local = &hradio_spi;

    evk_radio_spi_reset_cs();

    /* Disable the peripheral */
    __HAL_DMA_DISABLE((spi_local)->hdmarx);

    /* Configure DMA Channel data length */
    (spi_local)->hdmarx->Instance->CNDTR = size;

    /* Configure DMA Channel source address */
    (spi_local)->hdmarx->Instance->CPAR = (uint32_t)&(spi_local)->Instance->DR;

    /* Configure DMA Channel destination address */
    (spi_local)->hdmarx->Instance->CMAR = (uint32_t)rx_data;

    /* Enable the peripheral */
    __HAL_DMA_ENABLE((spi_local)->hdmarx);

    /* Enable Rx DMA Request */
    SET_BIT((spi_local)->Instance->CR2, SPI_CR2_RXDMAEN);

    /* Disable the peripheral */
    __HAL_DMA_DISABLE((spi_local)->hdmatx);

    /* Configure DMA Channel data length */
    (spi_local)->hdmatx->Instance->CNDTR = size;

    /* Configure DMA Channel destination address */
    (spi_local)->hdmatx->Instance->CPAR = (uint32_t)&(spi_local)->Instance->DR;

    /* Configure DMA Channel source address */
    (spi_local)->hdmatx->Instance->CMAR = (uint32_t)tx_data;

    /* Enable the Peripheral */
    __HAL_DMA_ENABLE((spi_local)->hdmatx);

    /* Enable Tx DMA Request */
    SET_BIT((spi_local)->Instance->CR2, SPI_CR2_TXDMAEN);
}

bool evk_radio_is_spi_busy(void)
{
    return (&hradio_spi)->Instance->SR & SPI_SR_BSY;
}

void evk_radio_context_switch(void)
{
    NVIC->ISPR[(((uint32_t)NVIC_RADIO_IRQ) >> 5UL)] = (uint32_t)(1UL << (((uint32_t)NVIC_RADIO_IRQ) & 0x1FUL));
}

void evk_radio_callback_context_switch(void)
{
    SET_BIT(SCB->ICSR, SCB_ICSR_PENDSVSET_Msk);
}

void evk_radio_set_spi_baudrate(spi_prescaler_t prescaler)
{
    hradio_spi.Init.BaudRatePrescaler = prescaler;
    HAL_SPI_Init(&hradio_spi);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize the SPI pins.
 *
 *  @param[in] spi_handle  SPI instance handle.
 */
void HAL_SPI_MspInit(SPI_HandleTypeDef *spi_handle)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (spi_handle->Instance == SPI2) {
        __HAL_RCC_SPI2_CLK_ENABLE();

        GPIO_InitStruct.Pin       = RADIO_SCK_PIN | RADIO_MISO_PIN | RADIO_MOSI_PIN;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Alternate = RADIO_SCK_AF;
        HAL_GPIO_Init(RADIO_SCK_PORT, &GPIO_InitStruct);

        /* SPI2_TX Init */
        hradio_dma_spi_tx.Instance                 = DMA1_CHANNEL_RADIO_TX;
        hradio_dma_spi_tx.Init.Request             = DMA_REQUEST_SPI2_TX;
        hradio_dma_spi_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
        hradio_dma_spi_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
        hradio_dma_spi_tx.Init.MemInc              = DMA_MINC_ENABLE;
        hradio_dma_spi_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hradio_dma_spi_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
        hradio_dma_spi_tx.Init.Mode                = DMA_NORMAL;
        hradio_dma_spi_tx.Init.Priority            = DMA_PRIORITY_LOW;
        if (HAL_DMA_Init(&hradio_dma_spi_tx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(spi_handle, hdmatx, hradio_dma_spi_tx);

        /* SPI2_RX Init */
        hradio_dma_spi_rx.Instance                 = DMA1_CHANNEL_RADIO_RX;
        hradio_dma_spi_rx.Init.Request             = DMA_REQUEST_SPI2_RX;
        hradio_dma_spi_rx.Init.Direction           = DMA_PERIPH_TO_MEMORY;
        hradio_dma_spi_rx.Init.PeriphInc           = DMA_PINC_DISABLE;
        hradio_dma_spi_rx.Init.MemInc              = DMA_MINC_ENABLE;
        hradio_dma_spi_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hradio_dma_spi_rx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
        hradio_dma_spi_rx.Init.Mode                = DMA_NORMAL;
        hradio_dma_spi_rx.Init.Priority            = DMA_PRIORITY_LOW;

        if (HAL_DMA_Init(&hradio_dma_spi_rx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_DMA_ENABLE_IT(&hradio_dma_spi_rx, (DMA_IT_TC | DMA_IT_TE));
        __HAL_LINKDMA(spi_handle, hdmarx, hradio_dma_spi_rx);
    }
}

/** @brief De-initialize the SPI pins.
 *
 *  @param[in] spi_handle  SPI instance handle.
 */
void HAL_SPI_MspDeInit(SPI_HandleTypeDef *spi_handle)
{
    if (spi_handle->Instance == SPI2) {
        /* Peripheral clock disable */
        __HAL_RCC_SPI2_CLK_DISABLE();

        HAL_GPIO_DeInit(RADIO_SCK_PORT, RADIO_SCK_PIN | RADIO_MOSI_PIN | RADIO_MISO_PIN);

        HAL_DMA_DeInit(spi_handle->hdmarx);
    }
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize IRQ pins.
 */
static void init_radio_irq_gpio(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = RADIO_IRQ_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(RADIO_IRQ_PORT, &GPIO_InitStruct);

    /* EXTI interrupt init*/
    HAL_NVIC_SetPriority(NVIC_RADIO_IRQ, PRIO_RADIO_IRQ, 0);
    HAL_NVIC_EnableIRQ(NVIC_RADIO_IRQ);
}

/** @brief Initialize the shutdown pin.
 */
static void init_radio_shutdown_gpio(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(RADIO_SHUTDOWN_PORT, RADIO_SHUTDOWN_PIN, GPIO_PIN_RESET);

    /* Configure GPIO pins */
    GPIO_InitStruct.Pin   = RADIO_SHUTDOWN_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(RADIO_SHUTDOWN_PORT, &GPIO_InitStruct);
}

/** @brief Initialize the reset pin.
 */
static void init_radio_reset_gpio(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(RADIO_RESET_PORT, RADIO_RESET_PIN, GPIO_PIN_SET);

    /* Configure GPIO pins */
    GPIO_InitStruct.Pin   = RADIO_RESET_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(RADIO_RESET_PORT, &GPIO_InitStruct);
}

/** @brief Initialize the debug enable pin.
 */
static void init_radio_debug_en_gpio(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(RADIO_DEBUG_EN_PORT, RADIO_DEBUG_EN_PIN, GPIO_PIN_RESET);

    /* Configure GPIO pins */
    GPIO_InitStruct.Pin   = RADIO_DEBUG_EN_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(RADIO_DEBUG_EN_PORT, &GPIO_InitStruct);
}

/** @brief Initialize the SPI bus connected to the radio.
 */
static void init_radio_spi_peripheral(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    hradio_spi.Instance = SPI2;
    hradio_spi.Init.Mode = SPI_MODE_MASTER;
    hradio_spi.Init.Direction = SPI_DIRECTION_2LINES;
    hradio_spi.Init.DataSize = SPI_DATASIZE_8BIT;
    hradio_spi.Init.CLKPolarity = SPI_POLARITY_LOW;
    hradio_spi.Init.CLKPhase = SPI_PHASE_1EDGE;
    hradio_spi.Init.NSS = SPI_NSS_SOFT;
    hradio_spi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    hradio_spi.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hradio_spi.Init.TIMode = SPI_TIMODE_DISABLE;
    hradio_spi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hradio_spi.Init.CRCPolynomial = 7;
    hradio_spi.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    hradio_spi.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
    if (HAL_SPI_Init(&hradio_spi) != HAL_OK) {
        Error_Handler();
    }

    /* Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(RADIO_CS_PORT, RADIO_CS_PIN, GPIO_PIN_SET);

    /* Configure GPIO pins */
    GPIO_InitStruct.Pin   = RADIO_CS_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(RADIO_CS_PORT, &GPIO_InitStruct);
}

/** @brief Initialize DMA clock and ISR channel
 */
static void init_radio_dma_controller(void)
{
    /* DMA1_Channel2_IRQn interrupt configuration (SPI2_RX)*/
    HAL_NVIC_SetPriority(NVIC_RADIO_DMA_RX_CPLT, PRIO_RADIO_DMA_RX_CPLT, 0);
    HAL_NVIC_EnableIRQ(NVIC_RADIO_DMA_RX_CPLT);

    /* DMA1_Channel2_IRQn interrupt configuration (SPI2_TX)*/
    HAL_NVIC_SetPriority(NVIC_RADIO_DMA_TX_CPLT, PRIO_RADIO_DMA_TX_CPLT, 0);
    HAL_NVIC_EnableIRQ(NVIC_RADIO_DMA_TX_CPLT);
}

/** @brief Initialize PendSV Priority and enable its interrupt.
 *
 *  @param[in] pendsv_prio  Pendsv Priority.
 */
static void init_radio_pendsv(uint32_t pendsv_prio)
{
    HAL_NVIC_SetPriority(NVIC_PENDSV_IRQ, pendsv_prio, 0);
    HAL_NVIC_ClearPendingIRQ(NVIC_PENDSV_IRQ);
    HAL_NVIC_EnableIRQ(NVIC_PENDSV_IRQ);
}
