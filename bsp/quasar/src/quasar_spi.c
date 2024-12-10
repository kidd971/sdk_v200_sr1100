/** @file  quasar_spi.c
 *  @brief This module configure SPI and provides functions to transmit and receive.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_spi.h"
#include "quasar_def.h"

/* CONSTANTS ******************************************************************/
#ifndef QUASAR_SPI_INTERDATA_IDLENESS
/*! SPI interdata idleness.
 *  Note: Interdata idleness of 1 cycle is necessary to use with SPARK SR1000 radio.
 */
#define QUASAR_SPI_INTERDATA_IDLENESS SPI_MASTER_INTERDATA_IDLENESS_01CYCLE
#endif

/* PUBLIC GLOBALS *************************************************************/
/* Handle for each SPI peripheral. */
static SPI_HandleTypeDef spi_handle_spi1 = {
        .Instance = SPI1
};
static SPI_HandleTypeDef spi_handle_spi2 = {
        .Instance = SPI2
};
static SPI_HandleTypeDef spi_handle_spi3 = {
        .Instance = SPI3
};

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void spi_select_clock_source(quasar_spi_selection_t spi_selection, quasar_spi_clk_source_t clk_source);
static void spi_enable_clock(quasar_spi_selection_t spi_selection);
static void spi_disable_clock(quasar_spi_selection_t spi_selection);

/* PUBLIC FUNCTIONS ***********************************************************/
void quasar_spi_init(quasar_spi_config_t spi_config)
{
    /* Initialize the chip select GPIO and configure the output level at high. */
    quasar_gpio_init(spi_config.gpio_config_ncs);
    quasar_gpio_set(spi_config.gpio_config_ncs.port, spi_config.gpio_config_ncs.pin);

    /* Initialize SCK, MOSI and MISO GPIOs used for SPI. */
    quasar_gpio_init(spi_config.gpio_config_sck);
    quasar_gpio_init(spi_config.gpio_config_mosi);
    quasar_gpio_init(spi_config.gpio_config_miso);

    /* Select and enable the clock. */
    spi_select_clock_source(spi_config.spi_selection, spi_config.clk_source);
    spi_enable_clock(spi_config.spi_selection);

    /* Configure and initialize SPI. */
    SPI_HandleTypeDef *spi_handle = quasar_spi_get_selected_handle(spi_config.spi_selection);

    spi_handle->Init.Mode                     = SPI_MODE_MASTER;
    spi_handle->Init.Direction                = SPI_DIRECTION_2LINES;
    spi_handle->Init.DataSize                 = SPI_DATASIZE_8BIT;
    spi_handle->Init.CLKPolarity              = SPI_POLARITY_LOW;
    spi_handle->Init.CLKPhase                 = SPI_PHASE_1EDGE;
    spi_handle->Init.NSS                      = SPI_NSS_SOFT;
    spi_handle->Init.BaudRatePrescaler        = spi_config.spi_prescaler;
    spi_handle->Init.FirstBit                 = SPI_FIRSTBIT_MSB;
    spi_handle->Init.TIMode                   = SPI_TIMODE_DISABLE;
    spi_handle->Init.CRCCalculation           = SPI_CRCCALCULATION_DISABLE;
    spi_handle->Init.NSSPMode                 = SPI_NSS_PULSE_DISABLE;
    spi_handle->Init.MasterInterDataIdleness  = QUASAR_SPI_INTERDATA_IDLENESS;

    if (HAL_SPI_Init(spi_handle) != HAL_OK) {
        while (1);
    }
}

void quasar_spi_deinit(quasar_spi_config_t spi_config)
{
    /* Deinitialize the SPI. */
    SPI_HandleTypeDef *spi_handle = quasar_spi_get_selected_handle(spi_config.spi_selection);

    if (HAL_SPI_DeInit(spi_handle) != HAL_OK) {
        while (1);
    }
    spi_disable_clock(spi_config.spi_selection);

    /* Deinitialize GPIOs used for SPI. */
    quasar_gpio_deinit(spi_config.gpio_config_ncs.port, spi_config.gpio_config_ncs.pin);
    quasar_gpio_deinit(spi_config.gpio_config_sck.port, spi_config.gpio_config_sck.pin);
    quasar_gpio_deinit(spi_config.gpio_config_mosi.port, spi_config.gpio_config_mosi.pin);
    quasar_gpio_deinit(spi_config.gpio_config_miso.port, spi_config.gpio_config_miso.pin);
}

uint8_t quasar_spi_transmit(quasar_spi_selection_t spi_selection, uint8_t data)
{
    uint8_t status = 0xFF;
    SPI_HandleTypeDef *spi_handle = quasar_spi_get_selected_handle(spi_selection);

    status = HAL_SPI_Transmit(spi_handle, &data, 1, 1000);
    return status;
}

uint8_t quasar_spi_receive(quasar_spi_selection_t spi_selection, uint8_t *data)
{
    uint8_t status = 0xFF;
    SPI_HandleTypeDef *spi_handle = quasar_spi_get_selected_handle(spi_selection);

    status = HAL_SPI_Receive(spi_handle, data, 1, 1000);
    return status;
}

void quasar_spi_transfer_full_duplex_blocking(quasar_spi_selection_t spi_selection, uint8_t *tx_data, uint8_t *rx_data, uint16_t data_size)
{
    SPI_HandleTypeDef *spi_handle = quasar_spi_get_selected_handle(spi_selection);

    uint16_t tx_size = data_size;
    uint16_t rx_size = data_size;

    if (spi_handle->State != HAL_SPI_STATE_READY) {
        return;
    }

    spi_handle->State = HAL_SPI_STATE_BUSY_TX_RX;

    /* Set the number of data at current transfer */
    MODIFY_REG(spi_handle->Instance->CR2, SPI_CR2_TSIZE, data_size);

    __HAL_SPI_ENABLE(spi_handle);

    /* Master transfer start */
    SET_BIT(spi_handle->Instance->CR1, SPI_CR1_CSTART);

    while ((tx_size > 0) || (rx_size > 0)) {
        if ((spi_handle->Instance->SR & SPI_FLAG_TXP) && (tx_size > 0)) {
            *(volatile uint8_t *)&(spi_handle->Instance->TXDR) = *tx_data++;
            tx_size--;
        }

        if ((spi_handle->Instance->SR & (SPI_FLAG_RXWNE | SPI_FLAG_FRLVL)) && (rx_size > 0)) {
            *rx_data++ = *(volatile uint8_t *)&(spi_handle->Instance->RXDR);
            rx_size--;
        };
    }

    while (!(spi_handle->Instance->SR & SPI_FLAG_EOT));

    /* Close Transfer */
    __HAL_SPI_CLEAR_EOTFLAG(spi_handle);
    __HAL_SPI_CLEAR_TXTFFLAG(spi_handle);

    /* Disable SPI peripheral */
    __HAL_SPI_DISABLE(spi_handle);

    /* Disable Tx DMA Request */
    CLEAR_BIT(spi_handle->Instance->CFG1, SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN);

    spi_handle->State = HAL_SPI_STATE_READY;
}

void quasar_spi_transfer_full_duplex_non_blocking(quasar_spi_selection_t spi_selection, uint8_t *tx_data, uint8_t *rx_data, uint16_t data_size)
{
    SPI_HandleTypeDef *spi_handle = quasar_spi_get_selected_handle(spi_selection);

    if (spi_handle->State != HAL_SPI_STATE_READY) {
        return;
    }

    spi_handle->State = HAL_SPI_STATE_BUSY_TX_RX;

    /* Reset the Tx/Rx DMA bits */
    CLEAR_BIT(spi_handle->Instance->CFG1, SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN);

    /* Disable the peripheral */
    __HAL_DMA_DISABLE((spi_handle)->hdmarx);

    /* Configure the DMA channel data size */
    MODIFY_REG((spi_handle)->hdmarx->Instance->CBR1, DMA_CBR1_BNDT, (data_size & DMA_CBR1_BNDT));

    /* Configure DMA Channel source address */
    (spi_handle)->hdmarx->Instance->CSAR = (uint32_t) &(spi_handle)->Instance->RXDR;

    /* Configure DMA Channel destination address */
    (spi_handle)->hdmarx->Instance->CDAR = (uint32_t)rx_data;

    /* Enable the peripheral */
    __HAL_DMA_ENABLE((spi_handle)->hdmarx);

    /* Enable Rx DMA Request */
    SET_BIT((spi_handle)->Instance->CFG1, SPI_CFG1_RXDMAEN);

    /* Disable the peripheral */
    __HAL_DMA_DISABLE((spi_handle)->hdmatx);

    /* Configure the DMA channel data size */
    MODIFY_REG((spi_handle)->hdmatx->Instance->CBR1, DMA_CBR1_BNDT, (data_size & DMA_CBR1_BNDT));

    /* Configure DMA Channel source address */
    (spi_handle)->hdmatx->Instance->CSAR = (uint32_t)tx_data;

    /* Configure DMA Channel destination address */
    (spi_handle)->hdmatx->Instance->CDAR = (uint32_t) &(spi_handle)->Instance->TXDR;

    /* Enable the Peripheral */
    __HAL_DMA_ENABLE((spi_handle)->hdmatx);

    MODIFY_REG((spi_handle)->Instance->CR2, SPI_CR2_TSIZE, data_size);

    /* Enable Tx DMA Request */
    SET_BIT((spi_handle)->Instance->CFG1, SPI_CFG1_TXDMAEN);

    /* Enable SPI peripheral */
    __HAL_SPI_ENABLE(spi_handle);

    SET_BIT(spi_handle->Instance->CR1, SPI_CR1_CSTART);
}

void quasar_spi_clear_cs(quasar_gpio_port_t gpio_port_cs, quasar_gpio_pin_t gpio_pin_cs)
{
    quasar_gpio_clear(gpio_port_cs, gpio_pin_cs);
}

void quasar_spi_set_cs(quasar_gpio_port_t gpio_port_cs, quasar_gpio_pin_t gpio_pin_cs)
{
    quasar_gpio_set(gpio_port_cs, gpio_pin_cs);
}

void quasar_spi_set_baudrate(quasar_spi_selection_t spi_selection, quasar_spi_prescaler_t prescaler)
{
    SPI_HandleTypeDef *spi_handle = quasar_spi_get_selected_handle(spi_selection);

    spi_handle->Init.BaudRatePrescaler = prescaler;
    if (HAL_SPI_Init(spi_handle) != HAL_OK) {
        Error_Handler();
    }
}

bool quasar_spi_is_busy(quasar_spi_selection_t spi_selection)
{
    SPI_HandleTypeDef *spi_handle = quasar_spi_get_selected_handle(spi_selection);

    return (spi_handle->State != HAL_SPI_STATE_READY);
}

SPI_HandleTypeDef *quasar_spi_get_selected_handle(quasar_spi_selection_t spi_selection)
{
    SPI_HandleTypeDef *spi_handle = {0};

    switch (spi_selection) {
    case QUASAR_SPI_SELECTION_SPI1:
        spi_handle = &spi_handle_spi1;
        break;
    case QUASAR_SPI_SELECTION_SPI2:
        spi_handle = &spi_handle_spi2;
        break;
    case QUASAR_SPI_SELECTION_SPI3:
        spi_handle = &spi_handle_spi3;
        break;
    default:
        /* Unimplemented timer. */
        break;
    }

    return spi_handle;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Select the SPI clock source.
 *
 *  @param[in] spi_selection  Selected SPI to select the clock source.
 *  @param[in] clk_source     Clock source to select.
 */
static void spi_select_clock_source(quasar_spi_selection_t spi_selection, quasar_spi_clk_source_t clk_source)
{
/** From the reference manual :
 *   SPI1    : RCC_CCIPR1
 *   SPI2    : RCC_CCIPR1
 *   SPI3    : RCC_CCIPR3
 */
    switch (spi_selection) {
    case QUASAR_SPI_SELECTION_SPI1:
        RCC->CCIPR1 |= (RCC_CCIPR1_SPI1SEL_Msk & (clk_source << RCC_CCIPR1_SPI1SEL_Pos));
        break;
    case QUASAR_SPI_SELECTION_SPI2:
        RCC->CCIPR1 |= (RCC_CCIPR1_SPI2SEL_Msk & (clk_source << RCC_CCIPR1_SPI2SEL_Pos));
        break;
    case QUASAR_SPI_SELECTION_SPI3:
        RCC->CCIPR3 |= (RCC_CCIPR3_SPI3SEL_Msk & (clk_source << RCC_CCIPR3_SPI3SEL_Pos));
        break;
    default:
        /* Unimplemented */
        break;
    }
}

/** @brief Enables the clock for the selected SPI.
 *
 *  @param[in] spi_selection  Selected SPI to enable the clock.
 */
static void spi_enable_clock(quasar_spi_selection_t spi_selection)
{
    switch (spi_selection) {
    case QUASAR_SPI_SELECTION_SPI1:
        __HAL_RCC_SPI1_CLK_ENABLE();
        break;
    case QUASAR_SPI_SELECTION_SPI2:
        __HAL_RCC_SPI2_CLK_ENABLE();
        break;
    case QUASAR_SPI_SELECTION_SPI3:
        __HAL_RCC_SPI3_CLK_ENABLE();
        break;
    default:
        /* Unimplemented timer. */
        break;
    }
}

/** @brief Disables the clock for the selected SPI.
 *
 *  @param[in] spi_selection  Selected SPI to disable the clock.
 */
static void spi_disable_clock(quasar_spi_selection_t spi_selection)
{
    switch (spi_selection) {
    case QUASAR_SPI_SELECTION_SPI1:
        __HAL_RCC_SPI1_CLK_DISABLE();
        break;
    case QUASAR_SPI_SELECTION_SPI2:
        __HAL_RCC_SPI2_CLK_DISABLE();
        break;
    case QUASAR_SPI_SELECTION_SPI3:
        __HAL_RCC_SPI3_CLK_DISABLE();
        break;
    default:
        /* Unimplemented timer. */
        break;
    }
}
