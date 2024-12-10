/** @file  quasar_dma.c
 *  @brief This module sets up DMA transactions and provides DMA mode read/write functions.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_dma.h"
#include "quasar_def.h"
#include "quasar_i2c.h"
#include "quasar_it.h"
#include "quasar_spi.h"
#include "quasar_uart.h"

/* TYPES **********************************************************************/
/* Typedef is used to avoid a checkpath error due to unrecognized "DMA_HandleTypeDef" type. */
typedef DMA_HandleTypeDef DMA_HandleTypeDef_t;
typedef UART_HandleTypeDef UART_HandleTypeDef_t;
typedef SPI_HandleTypeDef SPI_HandleTypeDef_t;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static DMA_HandleTypeDef_t *dma_get_selected_handle(quasar_dma_selection_t dma_selection);
static IRQn_Type dma_get_selected_irq(quasar_dma_selection_t dma_selection);
static uint8_t dma_configure_init_and_link(quasar_dma_selection_t dma_selection_tx, quasar_dma_selection_t dma_selection_rx,
                                            quasar_dma_peripheral_t dma_peripheral, uint8_t peripheral_selection);
static void dma_init_and_link_with_uart(DMA_HandleTypeDef *dma_handle_tx, DMA_HandleTypeDef *dma_handle_rx, quasar_uart_selection_t uart_selection);
static void dma_configure_request_with_uart(DMA_HandleTypeDef *dma_handle_tx, DMA_HandleTypeDef *dma_handle_rx, quasar_uart_selection_t uart_selection);
static void dma_init_and_link_with_spi(DMA_HandleTypeDef *dma_handle_tx, DMA_HandleTypeDef *dma_handle_rx, quasar_spi_selection_t spi_selection);
static void dma_configure_request_with_spi(DMA_HandleTypeDef *dma_handle_tx, DMA_HandleTypeDef *dma_handle_rx, quasar_spi_selection_t spi_selection);

static void default_irq_callback(void);

/* PRIVATE GLOBALS ************************************************************/
DMA_HandleTypeDef gpdma_handle_channel0 = {
    .Instance = GPDMA1_Channel0
};
DMA_HandleTypeDef gpdma_handle_channel1 = {
    .Instance = GPDMA1_Channel1
};
DMA_HandleTypeDef gpdma_handle_channel2 = {
    .Instance = GPDMA1_Channel2
};
DMA_HandleTypeDef gpdma_handle_channel3 = {
    .Instance = GPDMA1_Channel3
};
DMA_HandleTypeDef gpdma_handle_channel4 = {
    .Instance = GPDMA1_Channel4
};
DMA_HandleTypeDef gpdma_handle_channel5 = {
    .Instance = GPDMA1_Channel5
};
DMA_HandleTypeDef gpdma_handle_channel6 = {
    .Instance = GPDMA1_Channel6
};
DMA_HandleTypeDef gpdma_handle_channel7 = {
    .Instance = GPDMA1_Channel7
};
DMA_HandleTypeDef gpdma_handle_channel8 = {
    .Instance = GPDMA1_Channel8
};
DMA_HandleTypeDef gpdma_handle_channel9 = {
    .Instance = GPDMA1_Channel9
};
DMA_HandleTypeDef gpdma_handle_channel10 = {
    .Instance = GPDMA1_Channel10
};
DMA_HandleTypeDef gpdma_handle_channel11 = {
    .Instance = GPDMA1_Channel11
};
DMA_HandleTypeDef gpdma_handle_channel12 = {
    .Instance = GPDMA1_Channel12
};
DMA_HandleTypeDef gpdma_handle_channel13 = {
    .Instance = GPDMA1_Channel13
};
DMA_HandleTypeDef gpdma_handle_channel14 = {
    .Instance = GPDMA1_Channel14
};
DMA_HandleTypeDef gpdma_handle_channel15 = {
    .Instance = GPDMA1_Channel15
};

static void (*gpdma1_channel0_callback)(void)    = default_irq_callback;
static void (*gpdma1_channel1_callback)(void)    = default_irq_callback;
static void (*gpdma1_channel2_callback)(void)    = default_irq_callback;
static void (*gpdma1_channel3_callback)(void)    = default_irq_callback;
static void (*gpdma1_channel4_callback)(void)    = default_irq_callback;
static void (*gpdma1_channel5_callback)(void)    = default_irq_callback;
static void (*gpdma1_channel6_callback)(void)    = default_irq_callback;
static void (*gpdma1_channel7_callback)(void)    = default_irq_callback;
static void (*gpdma1_channel8_callback)(void)    = default_irq_callback;
static void (*gpdma1_channel9_callback)(void)    = default_irq_callback;
static void (*gpdma1_channel10_callback)(void)   = default_irq_callback;
static void (*gpdma1_channel11_callback)(void)   = default_irq_callback;
static void (*gpdma1_channel12_callback)(void)   = default_irq_callback;
static void (*gpdma1_channel13_callback)(void)   = default_irq_callback;
static void (*gpdma1_channel14_callback)(void)   = default_irq_callback;
static void (*gpdma1_channel15_callback)(void)   = default_irq_callback;

/* PUBLIC FUNCTIONS ***********************************************************/
void quasar_dma_init(quasar_dma_config_t dma_config)
{
    IRQn_Type dma_tx_irq;
    IRQn_Type dma_rx_irq;

    /* Enable IRQ for selected DMA instances, if the selected DMA is in use. */
    if (dma_config.dma_selection_tx != QUASAR_DMA_SELECTION_NOT_USED) {
        dma_tx_irq = dma_get_selected_irq(dma_config.dma_selection_tx);
        NVIC_SetPriority(dma_tx_irq, dma_config.irq_priority);
        NVIC_EnableIRQ(dma_tx_irq);
    }

    if (dma_config.dma_selection_rx != QUASAR_DMA_SELECTION_NOT_USED) {
        dma_rx_irq = dma_get_selected_irq(dma_config.dma_selection_rx);
        NVIC_SetPriority(dma_rx_irq, dma_config.irq_priority);
        NVIC_EnableIRQ(dma_rx_irq);
    }

    /* Enable the clock for the DMA peripheral, if it is not already enabled. */
    if (QUASAR_READ_BIT(RCC->AHB1ENR, RCC_AHB1ENR_GPDMA1EN_Pos)) {
        __HAL_RCC_GPDMA1_CLK_ENABLE();
    }

    /* Configure the DMA instances, initialize them and associate them to the corresponding peripherals. */
    if (dma_configure_init_and_link(dma_config.dma_selection_tx, dma_config.dma_selection_rx, dma_config.dma_peripheral, dma_config.peripheral_selection)) {
        while (1);
    }
}

void quasar_dma_deinit(quasar_dma_config_t dma_config)
{
    DMA_HandleTypeDef *dma_handle_tx = NULL;
    DMA_HandleTypeDef *dma_handle_rx = NULL;
    IRQn_Type dma_tx_irq;
    IRQn_Type dma_rx_irq;

    /* Disable IRQs and deinitialize peripherals for selected DMA instances, if the selected DMA was in use. */
    if (dma_config.dma_selection_tx != QUASAR_DMA_SELECTION_NOT_USED) {
        dma_handle_tx = dma_get_selected_handle(dma_config.dma_selection_tx);
        dma_tx_irq = dma_get_selected_irq(dma_config.dma_selection_tx);

        NVIC_DisableIRQ(dma_tx_irq);
        if (HAL_DMA_DeInit(dma_handle_tx) != HAL_OK) {
            while (1);
        }
    }

    if (dma_config.dma_selection_rx != QUASAR_DMA_SELECTION_NOT_USED) {
        dma_handle_rx = dma_get_selected_handle(dma_config.dma_selection_rx);
        dma_rx_irq = dma_get_selected_irq(dma_config.dma_selection_rx);

        NVIC_DisableIRQ(dma_rx_irq);
        if (HAL_DMA_DeInit(dma_handle_rx) != HAL_OK) {
            while (1);
        }
    }
}

void quasar_dma_enable_irq(quasar_dma_selection_t dma_selection)
{
    IRQn_Type dma_irq = dma_get_selected_irq(dma_selection);

    NVIC_EnableIRQ(dma_irq);
}

void quasar_dma_disable_irq(quasar_dma_selection_t dma_selection)
{
    IRQn_Type dma_irq = dma_get_selected_irq(dma_selection);

    NVIC_DisableIRQ(dma_irq);
}

void quasar_dma_set_channel0_dma_callback(void (*irq_callback)(void))
{
    gpdma1_channel0_callback = irq_callback;
}

void quasar_dma_set_channel1_dma_callback(void (*irq_callback)(void))
{
    gpdma1_channel1_callback = irq_callback;
}

void quasar_dma_set_channel2_dma_callback(void (*irq_callback)(void))
{
    gpdma1_channel2_callback = irq_callback;
}

void quasar_dma_set_channel3_dma_callback(void (*irq_callback)(void))
{
    gpdma1_channel3_callback = irq_callback;
}

void quasar_dma_set_channel4_dma_callback(void (*irq_callback)(void))
{
    gpdma1_channel4_callback = irq_callback;
}

void quasar_dma_set_channel5_dma_callback(void (*irq_callback)(void))
{
    gpdma1_channel5_callback = irq_callback;
}

void quasar_dma_set_channel6_dma_callback(void (*irq_callback)(void))
{
    gpdma1_channel6_callback = irq_callback;
}

void quasar_dma_set_channel7_dma_callback(void (*irq_callback)(void))
{
    gpdma1_channel7_callback = irq_callback;
}

void quasar_dma_set_channel8_dma_callback(void (*irq_callback)(void))
{
    gpdma1_channel8_callback = irq_callback;
}

void quasar_dma_set_channel9_dma_callback(void (*irq_callback)(void))
{
    gpdma1_channel9_callback = irq_callback;
}

void quasar_dma_set_channel10_dma_callback(void (*irq_callback)(void))
{
    gpdma1_channel10_callback = irq_callback;
}

void quasar_dma_set_channel11_dma_callback(void (*irq_callback)(void))
{
    gpdma1_channel11_callback = irq_callback;
}

void quasar_dma_set_channel12_dma_callback(void (*irq_callback)(void))
{
    gpdma1_channel12_callback = irq_callback;
}

void quasar_dma_set_channel13_dma_callback(void (*irq_callback)(void))
{
    gpdma1_channel13_callback = irq_callback;
}

void quasar_dma_set_channel14_dma_callback(void (*irq_callback)(void))
{
    gpdma1_channel14_callback = irq_callback;
}

void quasar_dma_set_channel15_dma_callback(void (*irq_callback)(void))
{
    gpdma1_channel15_callback = irq_callback;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Return the handle from the selected DMA.
 *
 *  @param[in] dma_selection  Available DMA selection.
 *  @return Selected DMA handle.
 */
static DMA_HandleTypeDef_t *dma_get_selected_handle(quasar_dma_selection_t dma_selection)
{
    DMA_HandleTypeDef *dma_handle = {0};

    switch (dma_selection) {
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL0:
        dma_handle = &gpdma_handle_channel0;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL1:
        dma_handle = &gpdma_handle_channel1;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL2:
        dma_handle = &gpdma_handle_channel2;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL3:
        dma_handle = &gpdma_handle_channel3;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL4:
        dma_handle = &gpdma_handle_channel4;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL5:
        dma_handle = &gpdma_handle_channel5;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL6:
        dma_handle = &gpdma_handle_channel6;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL7:
        dma_handle = &gpdma_handle_channel7;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL8:
        dma_handle = &gpdma_handle_channel8;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL9:
        dma_handle = &gpdma_handle_channel9;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL10:
        dma_handle = &gpdma_handle_channel10;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL11:
        dma_handle = &gpdma_handle_channel11;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL12:
        dma_handle = &gpdma_handle_channel12;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL13:
        dma_handle = &gpdma_handle_channel13;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL14:
        dma_handle = &gpdma_handle_channel14;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL15:
        dma_handle = &gpdma_handle_channel15;
        break;
    default:
        /* Unimplemented dma. */
        break;
    }

    return dma_handle;
}

/** @brief Return the selected DMA's global interrupt.
 *
 *  @param[in] dma_selection  Selected DMA to get associated global interrupt.
 *  @return Selected DMA global interrupt.
 */
static IRQn_Type dma_get_selected_irq(quasar_dma_selection_t dma_selection)
{
    IRQn_Type dma_irq = {0};

    switch (dma_selection) {
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL0:
        dma_irq = GPDMA1_Channel0_IRQn;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL1:
        dma_irq = GPDMA1_Channel1_IRQn;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL2:
        dma_irq = GPDMA1_Channel2_IRQn;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL3:
        dma_irq = GPDMA1_Channel3_IRQn;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL4:
        dma_irq = GPDMA1_Channel4_IRQn;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL5:
        dma_irq = GPDMA1_Channel5_IRQn;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL6:
        dma_irq = GPDMA1_Channel6_IRQn;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL7:
        dma_irq = GPDMA1_Channel7_IRQn;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL8:
        dma_irq = GPDMA1_Channel8_IRQn;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL9:
        dma_irq = GPDMA1_Channel9_IRQn;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL10:
        dma_irq = GPDMA1_Channel10_IRQn;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL11:
        dma_irq = GPDMA1_Channel11_IRQn;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL12:
        dma_irq = GPDMA1_Channel12_IRQn;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL13:
        dma_irq = GPDMA1_Channel13_IRQn;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL14:
        dma_irq = GPDMA1_Channel14_IRQn;
        break;
    case QUASAR_DMA_SELECTION_GPDMA1_CHANNEL15:
        dma_irq = GPDMA1_Channel15_IRQn;
        break;
    default:
        /* Unimplemented dma. */
        break;
    }

    return dma_irq;
}

/** @brief Configure the DMA instances, initialize them and associate them with the corresponding
 *         peripheral for DMA transfer.
 *
 *  @param[in] dma_selection_tx      Selected DMA instance for transmission.
 *  @param[in] dma_selection_rx      Selected DMA instance for reception.
 *  @param[in] dma_peripheral        Selected peripheral to be associated with DMA transfer.
 *  @param[in] peripheral_selection  Selected peripheral selection to be associated with DMA transfer.
 *  @return A zero value if the configuration, the initialization and the linking are done correctly; a non-zero value otherwise.
 */
static uint8_t dma_configure_init_and_link(quasar_dma_selection_t dma_selection_tx, quasar_dma_selection_t dma_selection_rx,
                                           quasar_dma_peripheral_t dma_peripheral, uint8_t peripheral_selection)
{
    DMA_HandleTypeDef *dma_handle_tx = NULL;
    DMA_HandleTypeDef *dma_handle_rx = NULL;

    if (dma_selection_tx != QUASAR_DMA_SELECTION_NOT_USED) {
        dma_handle_tx = dma_get_selected_handle(dma_selection_tx);
    }

    if (dma_selection_rx != QUASAR_DMA_SELECTION_NOT_USED) {
        dma_handle_rx = dma_get_selected_handle(dma_selection_rx);
    }

    switch (dma_peripheral) {
    case QUASAR_DMA_PERIPHERAL_UART:
        dma_configure_request_with_uart(dma_handle_tx, dma_handle_rx, peripheral_selection);
        dma_init_and_link_with_uart(dma_handle_tx, dma_handle_rx, peripheral_selection);
        break;
    case QUASAR_DMA_PERIPHERAL_SPI:
        dma_configure_request_with_spi(dma_handle_tx, dma_handle_rx, peripheral_selection);
        dma_init_and_link_with_spi(dma_handle_tx, dma_handle_rx, peripheral_selection);
        break;
    default:
        /* Unimplemented dma. */
        return 1;
        break;
    }

    return 0;
}

/** @brief Configure the DMA instances, initialize them and associate them with the corresponding
 *         peripheral for DMA transfer.
 *
 *  @param[in] dma_selection_tx  Selected DMA instance for transmission.
 *  @param[in] dma_selection_rx  Selected DMA instance for reception.
 *  @param[in] uart_selection    Selected peripheral to be associated with DMA transfer.
 */
static void dma_init_and_link_with_uart(DMA_HandleTypeDef *dma_handle_tx, DMA_HandleTypeDef *dma_handle_rx, quasar_uart_selection_t uart_selection)
{
    if (uart_selection >= _QUASAR_UART_SELECTION_COUNT) {
        while (1);
    }

    UART_HandleTypeDef_t *uart_handle = quasar_uart_get_selected_handle(uart_selection);
    USART_TypeDef *uart_instance = quasar_uart_get_instance(uart_selection);

    uart_handle->gState = HAL_UART_STATE_READY;
    if (dma_handle_tx != NULL) {
        dma_handle_tx->Init.BlkHWRequest          = DMA_BREQ_SINGLE_BURST;
        dma_handle_tx->Init.Direction             = DMA_MEMORY_TO_PERIPH;
        dma_handle_tx->Init.SrcInc                = DMA_SINC_INCREMENTED;
        dma_handle_tx->Init.DestInc               = DMA_DINC_FIXED;
        dma_handle_tx->Init.SrcDataWidth          = DMA_SRC_DATAWIDTH_BYTE;
        dma_handle_tx->Init.DestDataWidth         = DMA_DEST_DATAWIDTH_BYTE;
        dma_handle_tx->Init.Priority              = DMA_LOW_PRIORITY_LOW_WEIGHT;
        dma_handle_tx->Init.SrcBurstLength        = 1;
        dma_handle_tx->Init.DestBurstLength       = 1;
        dma_handle_tx->Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT0;
        dma_handle_tx->Init.TransferEventMode     = DMA_TCEM_BLOCK_TRANSFER;
        dma_handle_tx->Init.Mode                  = DMA_NORMAL;

        if (HAL_DMA_Init(dma_handle_tx) != HAL_OK) {
            while (1);
        }
        __HAL_LINKDMA(uart_handle, hdmatx, *dma_handle_tx);
    }

    if (dma_handle_rx != NULL) {
        QUASAR_SET_BIT(uart_instance->CR3, USART_CR3_DMAR_Msk);
        dma_handle_rx->Init.BlkHWRequest          = DMA_BREQ_SINGLE_BURST;
        dma_handle_rx->Init.Direction             = DMA_PERIPH_TO_MEMORY;
        dma_handle_rx->Init.SrcInc                = DMA_SINC_FIXED;
        dma_handle_rx->Init.DestInc               = DMA_DINC_INCREMENTED;
        dma_handle_rx->Init.SrcDataWidth          = DMA_SRC_DATAWIDTH_BYTE;
        dma_handle_rx->Init.DestDataWidth         = DMA_DEST_DATAWIDTH_BYTE;
        dma_handle_rx->Init.Priority              = DMA_LOW_PRIORITY_LOW_WEIGHT;
        dma_handle_rx->Init.SrcBurstLength        = 1;
        dma_handle_rx->Init.DestBurstLength       = 1;
        dma_handle_rx->Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT0;
        dma_handle_rx->Init.TransferEventMode     = DMA_TCEM_BLOCK_TRANSFER;
        dma_handle_rx->Init.Mode                  = DMA_NORMAL;
        if (HAL_DMA_Init(dma_handle_rx) != HAL_OK) {
            while (1);
        }
        __HAL_LINKDMA(uart_handle, hdmarx, *dma_handle_rx);
        __HAL_DMA_ENABLE_IT(dma_handle_rx, (DMA_IT_TC | DMA_IT_DTE));
    }
}

/** @brief Based on UART selection, this function configures the DMA request for transmission and reception.
 *
 *  @param[in] dma_selection_tx  Selected DMA instance for transmission.
 *  @param[in] dma_selection_rx  Selected DMA instance for reception.
 *  @param[in] uart_selection    Selected UART to be associated with DMA transfer.
 */
static void dma_configure_request_with_uart(DMA_HandleTypeDef *dma_handle_tx, DMA_HandleTypeDef *dma_handle_rx, quasar_uart_selection_t uart_selection)
{
    switch (uart_selection) {
    case QUASAR_UART_SELECTION_USART1:
        if (dma_handle_tx != NULL) {
            dma_handle_tx->Init.Request = GPDMA1_REQUEST_USART1_TX;
        }
        if (dma_handle_rx != NULL) {
            dma_handle_rx->Init.Request = GPDMA1_REQUEST_USART1_RX;
        }
        break;
    case QUASAR_UART_SELECTION_USART2:
        if (dma_handle_tx != NULL) {
            dma_handle_tx->Init.Request = GPDMA1_REQUEST_USART2_TX;
        }
        if (dma_handle_rx != NULL) {
            dma_handle_rx->Init.Request = GPDMA1_REQUEST_USART2_RX;
        }
        break;
    case QUASAR_UART_SELECTION_USART3:
        if (dma_handle_tx != NULL) {
            dma_handle_tx->Init.Request = GPDMA1_REQUEST_USART3_TX;
        }
        if (dma_handle_rx != NULL) {
            dma_handle_rx->Init.Request = GPDMA1_REQUEST_USART3_RX;
        }
        break;
    case QUASAR_UART_SELECTION_UART4:
        if (dma_handle_tx != NULL) {
            dma_handle_tx->Init.Request = GPDMA1_REQUEST_UART4_TX;
        }
        if (dma_handle_rx != NULL) {
            dma_handle_rx->Init.Request = GPDMA1_REQUEST_UART4_RX;
        }
        break;
    case QUASAR_UART_SELECTION_UART5:
        if (dma_handle_tx != NULL) {
            dma_handle_tx->Init.Request = GPDMA1_REQUEST_UART5_TX;
        }
        if (dma_handle_rx != NULL) {
            dma_handle_rx->Init.Request = GPDMA1_REQUEST_UART5_RX;
        }
        break;
    case QUASAR_UART_SELECTION_USART6:
        if (dma_handle_tx != NULL) {
            dma_handle_tx->Init.Request = GPDMA1_REQUEST_USART6_TX;
        }
        if (dma_handle_rx != NULL) {
            dma_handle_rx->Init.Request = GPDMA1_REQUEST_USART6_RX;
        }
        break;
    default:
        /* Peripheral is not available. */
        break;
    }
}

/** @brief Configure the DMA instances, initialize them and associate them with the corresponding
 *         peripheral for DMA transfer.
 *
 *  @param[in] dma_selection_tx  Selected DMA instance for transmission.
 *  @param[in] dma_selection_rx  Selected DMA instance for reception.
 *  @param[in] spi_selection    Selected peripheral to be associated with DMA transfer.
 */
static void dma_init_and_link_with_spi(DMA_HandleTypeDef *dma_handle_tx, DMA_HandleTypeDef *dma_handle_rx, quasar_spi_selection_t spi_selection)
{
    if (spi_selection >= _QUASAR_SPI_SELECTION_COUNT) {
        while (1);
    }

    SPI_HandleTypeDef_t *spi_handle = quasar_spi_get_selected_handle(spi_selection);

    spi_handle->State = HAL_SPI_STATE_READY;
    if (dma_handle_tx != NULL) {
        dma_handle_tx->Init.BlkHWRequest          = DMA_BREQ_SINGLE_BURST;
        dma_handle_tx->Init.Direction             = DMA_MEMORY_TO_PERIPH;
        dma_handle_tx->Init.SrcInc                = DMA_SINC_INCREMENTED;
        dma_handle_tx->Init.DestInc               = DMA_DINC_FIXED;
        dma_handle_tx->Init.SrcDataWidth          = DMA_SRC_DATAWIDTH_BYTE;
        dma_handle_tx->Init.DestDataWidth         = DMA_DEST_DATAWIDTH_BYTE;
        dma_handle_tx->Init.Priority              = DMA_LOW_PRIORITY_LOW_WEIGHT;
        dma_handle_tx->Init.SrcBurstLength        = 1;
        dma_handle_tx->Init.DestBurstLength       = 1;
        dma_handle_tx->Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT0;
        dma_handle_tx->Init.TransferEventMode     = DMA_TCEM_BLOCK_TRANSFER;
        dma_handle_tx->Init.Mode                  = DMA_NORMAL;

        if (HAL_DMA_Init(dma_handle_tx) != HAL_OK) {
            while (1);
        }
        __HAL_LINKDMA(spi_handle, hdmatx, *dma_handle_tx);
    }

    if (dma_handle_rx != NULL) {
        dma_handle_rx->Init.BlkHWRequest          = DMA_BREQ_SINGLE_BURST;
        dma_handle_rx->Init.Direction             = DMA_PERIPH_TO_MEMORY;
        dma_handle_rx->Init.SrcInc                = DMA_SINC_FIXED;
        dma_handle_rx->Init.DestInc               = DMA_DINC_INCREMENTED;
        dma_handle_rx->Init.SrcDataWidth          = DMA_SRC_DATAWIDTH_BYTE;
        dma_handle_rx->Init.DestDataWidth         = DMA_DEST_DATAWIDTH_BYTE;
        dma_handle_rx->Init.Priority              = DMA_LOW_PRIORITY_LOW_WEIGHT;
        dma_handle_rx->Init.SrcBurstLength        = 1;
        dma_handle_rx->Init.DestBurstLength       = 1;
        dma_handle_rx->Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0|DMA_DEST_ALLOCATED_PORT0;
        dma_handle_rx->Init.TransferEventMode     = DMA_TCEM_BLOCK_TRANSFER;
        dma_handle_rx->Init.Mode                  = DMA_NORMAL;

        if (HAL_DMA_Init(dma_handle_rx) != HAL_OK) {
            while (1);
        }
        __HAL_LINKDMA(spi_handle, hdmarx, *dma_handle_rx);

        __HAL_DMA_ENABLE_IT(dma_handle_rx, (DMA_IT_TC | DMA_IT_DTE));
    }
}

/** @brief Based on SPI selection, this function configure the DMA request for transmisson and reception.
 *
 *  @param[in] dma_selection_tx  Selected DMA instance for transmission.
 *  @param[in] dma_selection_rx  Selected DMA instance for reception.
 *  @param[in] spi_selection     Selected SPI to be associated with DMA transfer.
 */
static void dma_configure_request_with_spi(DMA_HandleTypeDef *dma_handle_tx, DMA_HandleTypeDef *dma_handle_rx, quasar_spi_selection_t spi_selection)
{
    switch (spi_selection) {
    case QUASAR_SPI_SELECTION_SPI1:
        if (dma_handle_tx != NULL) {
            dma_handle_tx->Init.Request = GPDMA1_REQUEST_SPI1_TX;
        }
        if (dma_handle_rx != NULL) {
            dma_handle_rx->Init.Request = GPDMA1_REQUEST_SPI1_RX;
        }
        break;
    case QUASAR_SPI_SELECTION_SPI2:
        if (dma_handle_tx != NULL) {
            dma_handle_tx->Init.Request = GPDMA1_REQUEST_SPI2_TX;
        }
        if (dma_handle_rx != NULL) {
            dma_handle_rx->Init.Request = GPDMA1_REQUEST_SPI2_RX;
        }
        break;
    case QUASAR_SPI_SELECTION_SPI3:
        if (dma_handle_tx != NULL) {
            dma_handle_tx->Init.Request = GPDMA1_REQUEST_SPI3_TX;
        }
        if (dma_handle_rx != NULL) {
            dma_handle_rx->Init.Request = GPDMA1_REQUEST_SPI3_RX;
        }
        break;
    default:
        /* Peripheral is not available. */
        break;
    }
}

/** @brief Default IRQ callback to prevent page fault.
 */
static void default_irq_callback(void)
{
    return;
}

/* ST HAL FUNCTIONS IMPLEMENTATION ********************************************/
/** @brief This function handles interrupt for channel 0 of GPDMA1.
 */
void GPDMA1_Channel0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&gpdma_handle_channel0);
    gpdma1_channel0_callback();
}

/** @brief This function handles interrupt for channel 1 of GPDMA1.
 */
void GPDMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&gpdma_handle_channel1);
    gpdma1_channel1_callback();
}

/** @brief This function handles interrupt for channel 2 of GPDMA1.
 *
 *  TODO : Find a way to keep this IRQ Handler generic.
 *
 */
void GPDMA1_Channel2_IRQHandler(void)
{
    SPI_HandleTypeDef *spi_handle = quasar_spi_get_selected_handle(QUASAR_DEF_SPI_SELECTION_RADIO_1);
    /* Change the DMA state */
    gpdma_handle_channel2.State = HAL_DMA_STATE_READY;
    spi_handle->State = HAL_SPI_STATE_READY;

    __HAL_SPI_CLEAR_EOTFLAG(spi_handle);
    __HAL_SPI_CLEAR_TXTFFLAG(spi_handle);

    __HAL_SPI_DISABLE(spi_handle);

    /* Disable Tx DMA Request */
    CLEAR_BIT(spi_handle->Instance->CFG1, SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN);

    /* Clear the transfer complete flag */
    __HAL_DMA_CLEAR_FLAG(&gpdma_handle_channel2, DMA_FLAG_TC | DMA_FLAG_HT | DMA_FLAG_DTE);

    /* Process Unlocked */
    __HAL_UNLOCK(&gpdma_handle_channel2);

    /* Uncomment when the TODO in the description is completed and remove the code specific to the SPI handler. */
    //HAL_DMA_IRQHandler(&gpdma_handle_channel2);
    gpdma1_channel2_callback();
}

/** @brief This function handles interrupt for channel 3 of GPDMA1.
 */
void GPDMA1_Channel3_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&gpdma_handle_channel3);
    gpdma1_channel3_callback();
}

/** @brief This function handles interrupt for channel 4 of GPDMA1.
 */
void GPDMA1_Channel4_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&gpdma_handle_channel4);
    gpdma1_channel4_callback();
}

/** @brief This function handles interrupt for channel 5 of GPDMA1.
 */
void GPDMA1_Channel5_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&gpdma_handle_channel5);
    gpdma1_channel5_callback();
}

/** @brief This function handles interrupt for channel 6 of GPDMA1.
 *
 *  TO DO : Find a way to keep this IRQ Handler generic. When dual
 *  radio will be done, this IRQ handler will be exactly like GPDMA1
 *  channel 2.
 *
 */
void GPDMA1_Channel6_IRQHandler(void)
{
    SPI_HandleTypeDef *spi_handle = quasar_spi_get_selected_handle(QUASAR_DEF_SPI_SELECTION_RADIO_2);
    /* Change the DMA state */
    gpdma_handle_channel6.State = HAL_DMA_STATE_READY;
    spi_handle->State = HAL_SPI_STATE_READY;

    __HAL_SPI_CLEAR_EOTFLAG(spi_handle);
    __HAL_SPI_CLEAR_TXTFFLAG(spi_handle);

    __HAL_SPI_DISABLE(spi_handle);

    /* Disable Tx DMA Request */
    CLEAR_BIT(spi_handle->Instance->CFG1, SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN);

    /* Clear the transfer complete flag */
    __HAL_DMA_CLEAR_FLAG(&gpdma_handle_channel6, DMA_FLAG_TC | DMA_FLAG_HT | DMA_FLAG_DTE);

    /* Process Unlocked */
    __HAL_UNLOCK(&gpdma_handle_channel6);

    /* Uncomment when the TODO in the description is completed and remove the code specific to the SPI handler. */
    // HAL_DMA_IRQHandler(&gpdma_handle_channel6);
    gpdma1_channel6_callback();
}

/** @brief This function handles interrupt for channel 7 of GPDMA1.
 *
 *  TO DO : Extract the SAI driver from audio files integrate it
 *  with this DMA driver back into the audio files. Once this is
 *  done, interrupts of all channels will be included here.
 */
// void GPDMA1_Channel7_IRQHandler(void)
// {
//     HAL_DMA_IRQHandler(&gpdma_handle_channel7);
//     gpdma1_channel7_callback();
// }

/** @brief This function handles interrupt for channel 8 of GPDMA1.
 *
 *  TO DO : Extract the SAI driver from audio files integrate it
 *  with this DMA driver back into the audio files. Once this is
 *  done, interrupts of all channels will be included here.
 */
// void GPDMA1_Channel8_IRQHandler(void)
// {
//     HAL_DMA_IRQHandler(&gpdma_handle_channel8);
//     gpdma1_channel8_callback();
// }

/** @brief This function handles interrupt for channel 9 of GPDMA1.
 */
void GPDMA1_Channel9_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&gpdma_handle_channel9);
    gpdma1_channel9_callback();
}

/** @brief This function handles interrupt for channel 10 of GPDMA1.
 */
void GPDMA1_Channel10_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&gpdma_handle_channel10);
    gpdma1_channel10_callback();
}

/** @brief This function handles interrupt for channel 11 of GPDMA1.
 */
void GPDMA1_Channel11_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&gpdma_handle_channel11);
    gpdma1_channel11_callback();
}

/** @brief This function handles interrupt for channel 12 of GPDMA1.
 */
void GPDMA1_Channel12_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&gpdma_handle_channel12);
    gpdma1_channel12_callback();
}

/** @brief This function handles interrupt for channel 13 of GPDMA1.
 */
void GPDMA1_Channel13_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&gpdma_handle_channel13);
    gpdma1_channel13_callback();
}

/** @brief This function handles interrupt for channel 14 of GPDMA1.
 */
void GPDMA1_Channel14_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&gpdma_handle_channel14);
    gpdma1_channel14_callback();
}

/** @brief This function handles interrupt for channel 15 of GPDMA1.
 */
void GPDMA1_Channel15_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&gpdma_handle_channel15);
    gpdma1_channel15_callback();
}
