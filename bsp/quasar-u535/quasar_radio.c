/** @file  quasar_radio.c
 *  @brief This module provides functions for the SPARK Radio module.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_radio.h"
#include "quasar_def.h"
#include "quasar_error.h"
#include "quasar_it.h"
#include "quasar_qspi.h"
#include "quasar_spi.h"

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static quasar_radio_config_t radio_1_get_config(void);
#if 1 /* RADIO 2 ENABLED (535 dual radio) */
static quasar_radio_config_t radio_2_get_config(quasar_revision_t board_revision, quasar_bsp_status_t *err);
#endif
static void radio_init(quasar_radio_config_t radio_config, quasar_bsp_status_t *err);
static void radio_deinit(quasar_radio_config_t radio_config, quasar_bsp_status_t *err);
static void radio_init_debug_enable_gpio(void);
static void enable_pendsv_irq(uint32_t pendsv_prio);

/* PUBLIC FUNCTIONS ***********************************************************/
void quasar_radio_1_init(quasar_bsp_status_t *err)
{
    *err = QUASAR_OK;

    quasar_radio_config_t radio1_config = radio_1_get_config();

    radio_init(radio1_config, err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);

    /* This pin is only available for the radio 1. */
    radio_init_debug_enable_gpio();

    /* Enable PendSV IRQ. */
    enable_pendsv_irq(QUASAR_DEF_PRIO_PENDSV_IRQ);
}

#if 1 /* RADIO 2 ENABLED (535 dual radio) */
void quasar_radio_2_init(quasar_revision_t board_revision, quasar_bsp_status_t *err)
{
    *err = QUASAR_OK;

    quasar_radio_config_t radio2_config = radio_2_get_config(board_revision, err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);

    radio_init(radio2_config, err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);
}
#endif

void quasar_radio_1_deinit(quasar_bsp_status_t *err)
{
    *err = QUASAR_OK;

    quasar_radio_config_t radio1_config = radio_1_get_config();

    radio_deinit(radio1_config, err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);
}

#if 1 /* RADIO 2 ENABLED (535 dual radio) */
void quasar_radio_2_deinit(quasar_revision_t board_revision, quasar_bsp_status_t *err)
{
    *err = QUASAR_OK;

    quasar_radio_config_t radio2_config = radio_2_get_config(board_revision, err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);

    radio_deinit(radio2_config, err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);
}
#endif

void quasar_radio_set_radio_1_irq_callback(void (*irq_callback)(void))
{
    quasar_it_set_exti8_irq_callback(irq_callback);
}

#if 1 /* RADIO 2 ENABLED (535 dual radio) */
void quasar_radio_set_radio_2_irq_callback(void (*irq_callback)(void))
{
    quasar_it_set_exti7_irq_callback(irq_callback);
}
#endif

void quasar_radio_set_radio_1_non_blocking_transfer_callback(void (*irq_callback)(void))
{
    /* DMA RX transfer complete triggers when the data is actually in memory, so we can use the DMA. */
    quasar_dma_set_channel2_dma_callback(irq_callback);

#if 0 /* QSPI DISABLED */
#if RADIO_QSPI_ENABLED
    /* DMA TX transfer complete triggers when the data is in the peripheral, so we must use the peripheral's IRQ. */
    quasar_qspi_set_qspi1_callback(irq_callback);
#endif
#endif
}

#if 1 /* RADIO 2 ENABLED (535 dual radio) */
void quasar_radio_set_radio_2_non_blocking_transfer_callback(void (*irq_callback)(void))
{
    /* DMA RX transfer complete triggers when the data is actually in memory, so we can use the DMA. */
    quasar_dma_set_channel6_dma_callback(irq_callback);

#if 0 /* QSPI DISABLED */
#if RADIO_QSPI_ENABLED
    /* DMA TX transfer complete triggers when the data is in the peripheral, so we must use the peripheral's
     * IRQ.
     */
    quasar_qspi_set_qspi2_callback(irq_callback);
#endif
#endif
}
#endif

bool quasar_radio_1_read_irq_pin(void)
{
    if (QUASAR_READ_BIT(QUASAR_DEF_RADIO_1_IRQ_PORT->IDR, QUASAR_DEF_RADIO_1_IRQ_PIN)) {
        return true;
    }
    return false;
}

#if 1 /* RADIO 2 ENABLED (535 dual radio) */
bool quasar_radio_2_read_irq_pin(void)
{
    if (QUASAR_READ_BIT(QUASAR_DEF_RADIO_2_IRQ_PORT->IDR, QUASAR_DEF_RADIO_2_IRQ_PIN)) {
        return true;
    }
    return false;
}
#endif

void quasar_radio_1_enable_irq_it(void)
{
    /* Mask the interrupt event line. */
    QUASAR_SET_BIT(EXTI->IMR1, (1 << QUASAR_DEF_RADIO_1_IRQ_PIN));
    /* Synchronize after context switch. */
    __DSB();
    __ISB();
}

#if 1 /* RADIO 2 ENABLED (535 dual radio) */
void quasar_radio_2_enable_irq_it(void)
{
    /* Mask the interrupt event line. */
    QUASAR_SET_BIT(EXTI->IMR1, (1 << QUASAR_DEF_RADIO_2_IRQ_PIN));
    /* Synchronize after context switch. */
    __DSB();
    __ISB();
}
#endif

void quasar_radio_1_disable_irq_it(void)
{
    /* Unmask the interrupt event line. */
    QUASAR_CLEAR_BIT(EXTI->IMR1, (1 << QUASAR_DEF_RADIO_1_IRQ_PIN));
    /* Clear the interruption flag. */
    QUASAR_SET_BIT(EXTI->RPR1, (1 << QUASAR_DEF_RADIO_1_IRQ_PIN));
}

#if 1 /* RADIO 2 ENABLED (535 dual radio) */
void quasar_radio_2_disable_irq_it(void)
{
    /* Unmask the interrupt event line. */
    QUASAR_CLEAR_BIT(EXTI->IMR1, (1 << QUASAR_DEF_RADIO_2_IRQ_PIN));
    /* Clear the interruption flag. */
    QUASAR_SET_BIT(EXTI->RPR1, (1 << QUASAR_DEF_RADIO_2_IRQ_PIN));
}
#endif

void quasar_radio_1_enable_non_blocking_transfer_irq_it(void)
{
    /* DMA RX transfer complete triggers when the data is actually in memory, so we can use the DMA.
     *
     * In QSPI mode, only one DMA channel can bind to the QSPI peripheral. Hence, enabling this will
     * trigger an interrupt when doing a TX or an RX. A choice was made to bind it to the radio 1 RX
     * channel (DMA channel 2). We must be carefull to invoke the callback only when we've actually
     * done an RX in GPDMA1_Channel2_IRQHandler.
     */
    NVIC_EnableIRQ(QUASAR_DMA_GET_SELECTED_IRQ(QUASAR_DEF_DMA_SELECTION_RADIO_1_RX));

#if 0 /* QSPI DISABLED */
#if RADIO_QSPI_ENABLED
    /* DMA TX transfer complete triggers when the data is in the peripheral, so we must use the peripheral's
     * IRQ.
     *
     * Since the flag triggering the QSPI interrupt is on a transfer complete, some logic in the interrupt
     * invokes the configured callback only if it was on a TX operation.
     */
    quasar_qspi_enable_interrupt(QUASAR_QSPI_SELECTION_OSPI1);
#else
    NVIC_EnableIRQ(QUASAR_DMA_GET_SELECTED_IRQ(QUASAR_DEF_DMA_SELECTION_RADIO_1_TX));
#endif
#else
    NVIC_EnableIRQ(QUASAR_DMA_GET_SELECTED_IRQ(QUASAR_DEF_DMA_SELECTION_RADIO_1_TX));
#endif
    /* Synchronize after context switch. */
    __DSB();
    __ISB();
}

#if 1 /* RADIO 2 ENABLED (535 dual radio) */
void quasar_radio_2_enable_non_blocking_transfer_irq_it(void)
{
    /* DMA RX transfer complete triggers when the data is actually in memory, so we can use the DMA.
     *
     * In QSPI mode, only one DMA channel can bind to the QSPI peripheral. Hence, enabling this will
     * trigger an interrupt when doing a TX or an RX. A choice was made to bind it to the radio 2 RX
     * channel (DMA channel 6). We must be carefull to invoke the callback only when we've actually
     * done an RX in GPDMA1_Channel6_IRQHandler.
     */
    NVIC_EnableIRQ(QUASAR_DMA_GET_SELECTED_IRQ(QUASAR_DEF_DMA_SELECTION_RADIO_2_RX));

#if 0 /* QSPI DISABLED */
#if RADIO_QSPI_ENABLED
    /* DMA TX transfer complete triggers when the data is in the peripheral, so we must use the peripheral's
     * IRQ.
     *
     * Since the flag triggering the QSPI interrupt is on a transfer complete, some logic in the interrupt
     * invokes the configured callback only if it was on a TX operation.
     */
    quasar_qspi_enable_interrupt(QUASAR_QSPI_SELECTION_OSPI2);
#else
    NVIC_EnableIRQ(QUASAR_DMA_GET_SELECTED_IRQ(QUASAR_DEF_DMA_SELECTION_RADIO_2_TX));
#endif
#else
    NVIC_EnableIRQ(QUASAR_DMA_GET_SELECTED_IRQ(QUASAR_DEF_DMA_SELECTION_RADIO_2_TX));
#endif
    /* Synchronize after context switch. */
    __DSB();
    __ISB();
}
#endif

void quasar_radio_1_disable_non_blocking_transfer_irq_it(void)
{
    /* DMA RX transfer complete triggers when the data is actually in memory, so we can use the DMA. */
    NVIC_DisableIRQ(QUASAR_DMA_GET_SELECTED_IRQ(QUASAR_DEF_DMA_SELECTION_RADIO_1_RX));

#if 0 /* QSPI DISABLED */
#if RADIO_QSPI_ENABLED
    /* DMA TX transfer complete triggers when the data is in the peripheral, so we must use the peripheral's
     * IRQ.
     */
    quasar_qspi_disable_interrupt(QUASAR_QSPI_SELECTION_OSPI1);
#endif
#endif
}

#if 1 /* RADIO 2 ENABLED (535 dual radio) */
void quasar_radio_2_disable_non_blocking_transfer_irq_it(void)
{
    /* DMA RX transfer complete triggers when the data is actually in memory, so we can use the DMA. */
    NVIC_DisableIRQ(QUASAR_DMA_GET_SELECTED_IRQ(QUASAR_DEF_DMA_SELECTION_RADIO_2_RX));

#if 0 /* QSPI DISABLED */
#if RADIO_QSPI_ENABLED
    /* DMA TX transfer complete triggers when the data is in the peripheral, so we must use the peripheral's
     * IRQ.
     */
    quasar_qspi_disable_interrupt(QUASAR_QSPI_SELECTION_OSPI2);
#endif
#endif
}
#endif

// void quasar_radio_1_set_shutdown_pin(void)
// {
//     quasar_gpio_set(QUASAR_DEF_RADIO_1_SHUTDOWN_PORT, QUASAR_DEF_RADIO_1_SHUTDOWN_PIN);
// }

#if 1 /* RADIO 2 ENABLED (535 dual radio) */
void quasar_radio_2_set_shutdown_pin(void)
{
    quasar_gpio_set(QUASAR_DEF_RADIO_2_SHUTDOWN_PORT, QUASAR_DEF_RADIO_2_SHUTDOWN_PIN);
}
#endif

// void quasar_radio_1_reset_shutdown_pin(void)
// {
//     quasar_gpio_clear(QUASAR_DEF_RADIO_1_SHUTDOWN_PORT, QUASAR_DEF_RADIO_1_SHUTDOWN_PIN);
// }

#if 1 /* RADIO 2 ENABLED (535 dual radio) */
void quasar_radio_2_reset_shutdown_pin(void)
{
    quasar_gpio_clear(QUASAR_DEF_RADIO_2_SHUTDOWN_PORT, QUASAR_DEF_RADIO_2_SHUTDOWN_PIN);
}
#endif

void quasar_radio_1_set_reset_pin(void)
{
    QUASAR_SET_BIT(QUASAR_DEF_RADIO_1_RESET_PORT->BSRR, (1 << QUASAR_DEF_RADIO_1_RESET_PIN));
}

#if 1 /* RADIO 2 ENABLED (535 dual radio) */
void quasar_radio_2_set_reset_pin(void)
{
    QUASAR_SET_BIT(QUASAR_DEF_RADIO_2_RESET_PORT->BSRR, (1 << QUASAR_DEF_RADIO_2_RESET_PIN));
}
#endif

void quasar_radio_1_reset_reset_pin(void)
{
    QUASAR_SET_BIT(QUASAR_DEF_RADIO_1_RESET_PORT->BRR, (1 << QUASAR_DEF_RADIO_1_RESET_PIN));
}

#if 1 /* RADIO 2 ENABLED (535 dual radio) */
void quasar_radio_2_reset_reset_pin(void)
{
    QUASAR_SET_BIT(QUASAR_DEF_RADIO_2_RESET_PORT->BRR, (1 << QUASAR_DEF_RADIO_2_RESET_PIN));
}
#endif

void quasar_radio_1_set_debug_en(void)
{
    quasar_gpio_set(QUASAR_DEF_RADIO_1_DBG_EN_PORT, QUASAR_DEF_RADIO_1_DBG_EN_PIN);
}

void quasar_radio_1_reset_debug_en(void)
{
    quasar_gpio_clear(QUASAR_DEF_RADIO_1_DBG_EN_PORT, QUASAR_DEF_RADIO_1_DBG_EN_PIN);
}

void quasar_radio_1_context_switch(void)
{
    __NVIC_SetPendingIRQ(QUASAR_GPIO_GET_SELECTED_IRQ(QUASAR_DEF_RADIO_1_IRQ_PIN));
    /* Synchronize after context switch. */
    __DSB();
    __ISB();
}

#if 1 /* RADIO 2 ENABLED (535 dual radio) */
void quasar_radio_2_context_switch(void)
{
    __NVIC_SetPendingIRQ(QUASAR_GPIO_GET_SELECTED_IRQ(QUASAR_DEF_RADIO_2_IRQ_PIN));
    /* Synchronize after context switch. */
    __DSB();
    __ISB();
}
#endif

void quasar_radio_callback_context_switch(void)
{
    SET_BIT(SCB->ICSR, SCB_ICSR_PENDSVSET_Msk);
    /* Synchronize after context switch. */
    __DSB();
    __ISB();
}

void quasar_radio_1_set_spi_baudrate(quasar_spi_prescaler_t prescaler, quasar_bsp_status_t *err)
{
    *err = QUASAR_OK;

    quasar_spi_set_baudrate(QUASAR_DEF_SPI_SELECTION_RADIO_1, prescaler, err);
}

#if 1 /* RADIO 2 ENABLED (535 dual radio) */
void quasar_radio_2_set_spi_baudrate(quasar_spi_prescaler_t prescaler, quasar_bsp_status_t *err)
{
    *err = QUASAR_OK;

    quasar_spi_set_baudrate(QUASAR_DEF_SPI_SELECTION_RADIO_2, prescaler, err);
}
#endif

void quasar_radio_1_spi_set_cs(void)
{
    QUASAR_SET_BIT(QUASAR_DEF_RADIO_1_CS_PORT->BSRR, (1 << QUASAR_DEF_RADIO_1_CS_PIN));
}

#if 1 /* RADIO 2 ENABLED (535 dual radio) */
void quasar_radio_2_spi_set_cs(void)
{
    QUASAR_SET_BIT(QUASAR_DEF_RADIO_2_CS_PORT->BSRR, (1 << QUASAR_DEF_RADIO_2_CS_PIN));
}
#endif

void quasar_radio_1_spi_reset_cs(void)
{
    QUASAR_SET_BIT(QUASAR_DEF_RADIO_1_CS_PORT->BRR, (1 << QUASAR_DEF_RADIO_1_CS_PIN));
}

#if 1 /* RADIO 2 ENABLED (535 dual radio) */
void quasar_radio_2_spi_reset_cs(void)
{
    QUASAR_SET_BIT(QUASAR_DEF_RADIO_2_CS_PORT->BRR, (1 << QUASAR_DEF_RADIO_2_CS_PIN));
}
#endif

void quasar_radio_1_spi_transfer_full_duplex_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    quasar_spi_transfer_full_duplex_blocking(QUASAR_DEF_SPI_SELECTION_RADIO_1, tx_data, rx_data, size);
}

#if 1 /* RADIO 2 ENABLED (535 dual radio) */
void quasar_radio_2_spi_transfer_full_duplex_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    quasar_spi_transfer_full_duplex_blocking(QUASAR_DEF_SPI_SELECTION_RADIO_2, tx_data, rx_data, size);
}
#endif

void quasar_radio_1_spi_transfer_full_duplex_non_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    quasar_spi_transfer_full_duplex_non_blocking(QUASAR_DEF_SPI_SELECTION_RADIO_1, tx_data, rx_data, size);
}

#if 1 /* RADIO 2 ENABLED (535 dual radio) */
void quasar_radio_2_spi_transfer_full_duplex_non_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    quasar_spi_transfer_full_duplex_non_blocking(QUASAR_DEF_SPI_SELECTION_RADIO_2, tx_data, rx_data, size);
}
#endif

bool quasar_radio_1_is_spi_busy(void)
{
    return QUASAR_SPI_IS_BUSY(QUASAR_DEF_SPI_SELECTION_RADIO_1);
}

#if 1 /* RADIO 2 ENABLED (535 dual radio) */
bool quasar_radio_2_is_spi_busy(void)
{
    return QUASAR_SPI_IS_BUSY(QUASAR_DEF_SPI_SELECTION_RADIO_2);
}
#endif

void quasar_radio_init_unused_qspi_gpios(void)
{
#if 0 /* QSPI DISABLED */
    quasar_gpio_config_t gpio_config1 = {
        .port = QUASAR_DEF_RADIO_1_QSPI_IO_0_PORT,
        .pin = QUASAR_DEF_RADIO_1_QSPI_IO_0_PIN,
        .mode = QUASAR_GPIO_MODE_INPUT,
        .type = QUASAR_GPIO_TYPE_NONE,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_NONE,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config1);

    quasar_gpio_config_t gpio_config2 = {
        .port = QUASAR_DEF_RADIO_1_QSPI_IO_1_PORT,
        .pin = QUASAR_DEF_RADIO_1_QSPI_IO_1_PIN,
        .mode = QUASAR_GPIO_MODE_INPUT,
        .type = QUASAR_GPIO_TYPE_NONE,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_NONE,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config2);

    quasar_gpio_config_t gpio_config3 = {
        .port = QUASAR_DEF_RADIO_1_QSPI_IO_2_PORT,
        .pin = QUASAR_DEF_RADIO_1_QSPI_IO_2_PIN,
        .mode = QUASAR_GPIO_MODE_INPUT,
        .type = QUASAR_GPIO_TYPE_NONE,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_NONE,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config3);

    quasar_gpio_config_t gpio_config4 = {
        .port = QUASAR_DEF_RADIO_1_QSPI_IO_3_PORT,
        .pin = QUASAR_DEF_RADIO_1_QSPI_IO_3_PIN,
        .mode = QUASAR_GPIO_MODE_INPUT,
        .type = QUASAR_GPIO_TYPE_NONE,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_NONE,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config4);

    quasar_gpio_config_t gpio_config5 = {
        .port = QUASAR_DEF_RADIO_2_QSPI_IO_0_PORT,
        .pin = QUASAR_DEF_RADIO_2_QSPI_IO_0_PIN,
        .mode = QUASAR_GPIO_MODE_INPUT,
        .type = QUASAR_GPIO_TYPE_NONE,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_NONE,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config5);

    quasar_gpio_config_t gpio_config6 = {
        .port = QUASAR_DEF_RADIO_2_QSPI_IO_1_PORT,
        .pin = QUASAR_DEF_RADIO_2_QSPI_IO_1_PIN,
        .mode = QUASAR_GPIO_MODE_INPUT,
        .type = QUASAR_GPIO_TYPE_NONE,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_NONE,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config6);

    quasar_gpio_config_t gpio_config7 = {
        .port = QUASAR_DEF_RADIO_2_QSPI_IO_2_PORT,
        .pin = QUASAR_DEF_RADIO_2_QSPI_IO_2_PIN,
        .mode = QUASAR_GPIO_MODE_INPUT,
        .type = QUASAR_GPIO_TYPE_NONE,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_NONE,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config7);

    quasar_gpio_config_t gpio_config8 = {
        .port = QUASAR_DEF_RADIO_2_QSPI_IO_3_PORT,
        .pin = QUASAR_DEF_RADIO_2_QSPI_IO_3_PIN,
        .mode = QUASAR_GPIO_MODE_INPUT,
        .type = QUASAR_GPIO_TYPE_NONE,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_NONE,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config8);

    quasar_gpio_config_t gpio_config9 = {
        .port = QUASAR_DEF_RADIO_1_QSPI_SCK_PORT,
        .pin = QUASAR_DEF_RADIO_1_QSPI_SCK_PIN,
        .mode = QUASAR_GPIO_MODE_INPUT,
        .type = QUASAR_GPIO_TYPE_NONE,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_NONE,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config9);

    quasar_gpio_config_t gpio_config10 = {
        .port = QUASAR_DEF_RADIO_2_QSPI_SCK_PORT,
        .pin = QUASAR_DEF_RADIO_2_QSPI_SCK_PIN,
        .mode = QUASAR_GPIO_MODE_INPUT,
        .type = QUASAR_GPIO_TYPE_NONE,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_NONE,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config10);
#endif
}

void quasar_radio_1_init_unused_spi_gpios(void)
{
    quasar_gpio_config_t gpio_config_radio_sck = {
        .port = QUASAR_DEF_RADIO_1_SCK_PORT,
        .pin = QUASAR_DEF_RADIO_1_SCK_PIN,
        .mode = QUASAR_GPIO_MODE_INPUT,
        .type = QUASAR_GPIO_TYPE_NONE,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_NONE,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config_radio_sck);

    quasar_gpio_config_t gpio_config_radio_miso = {
        .port = QUASAR_DEF_RADIO_1_MISO_PORT,
        .pin = QUASAR_DEF_RADIO_1_MISO_PIN,
        .mode = QUASAR_GPIO_MODE_INPUT,
        .type = QUASAR_GPIO_TYPE_NONE,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_NONE,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config_radio_miso);

    quasar_gpio_config_t gpio_config_radio_mosi = {
        .port = QUASAR_DEF_RADIO_1_MOSI_PORT,
        .pin = QUASAR_DEF_RADIO_1_MOSI_PIN,
        .mode = QUASAR_GPIO_MODE_INPUT,
        .type = QUASAR_GPIO_TYPE_NONE,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_NONE,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config_radio_mosi);
}

#if 1 /* RADIO 2 ENABLED (535 dual radio) */
void quasar_radio_2_init_unused_spi_gpios(quasar_revision_t board_revision)
{
    // quasar_gpio_config_t gpio_config_radio_sck = {
    //     .port = QUASAR_DEF_RADIO_2_SCK_PORT,
    //     .pin = QUASAR_DEF_RADIO_2_SCK_PIN,
    //     .mode = QUASAR_GPIO_MODE_INPUT,
    //     .type = QUASAR_GPIO_TYPE_NONE,
    //     .pull = QUASAR_GPIO_PULL_NONE,
    //     .speed = QUASAR_GPIO_SPEED_NONE,
    //     .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    // };
    // quasar_gpio_init(gpio_config_radio_sck);

    quasar_gpio_config_t gpio_config_radio_miso = {
        .port = QUASAR_DEF_RADIO_2_MISO_PORT,
        .pin = QUASAR_DEF_RADIO_2_MISO_PIN,
        .mode = QUASAR_GPIO_MODE_INPUT,
        .type = QUASAR_GPIO_TYPE_NONE,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_NONE,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config_radio_miso);

    quasar_gpio_config_t gpio_config_radio_mosi = {
        .mode = QUASAR_GPIO_MODE_INPUT,
        .type = QUASAR_GPIO_TYPE_NONE,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_NONE,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };

    if (board_revision == QUASAR_REVA) {
        gpio_config_radio_mosi.port = QUASAR_DEF_RADIO_2_MOSI_PORT_REVA;
        gpio_config_radio_mosi.pin = QUASAR_DEF_RADIO_2_MOSI_PIN_REVA;
    } else {
        gpio_config_radio_mosi.port = QUASAR_DEF_RADIO_2_MOSI_PORT_REVB;
        gpio_config_radio_mosi.pin = QUASAR_DEF_RADIO_2_MOSI_PIN_REVB;
    }

    quasar_gpio_init(gpio_config_radio_mosi);
}
#endif

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Get the radio 1 configuration.
 *
 *  @return Configuration of the radio 1.
 */
static quasar_radio_config_t radio_1_get_config(void)
{
    /* Radio 1 SPI config and its four associated GPIOs. */
    quasar_gpio_config_t gpio_config_radio1_sck = {
        .port = QUASAR_DEF_RADIO_1_SCK_PORT,
        .pin = QUASAR_DEF_RADIO_1_SCK_PIN,
        .mode = QUASAR_GPIO_MODE_ALTERNATE,
        .type = QUASAR_GPIO_TYPE_PP,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate = QUASAR_GPIO_ALTERNATE_AF5,
    };
    quasar_gpio_config_t gpio_config_radio1_miso = {
        .port = QUASAR_DEF_RADIO_1_MISO_PORT,
        .pin = QUASAR_DEF_RADIO_1_MISO_PIN,
        .mode = QUASAR_GPIO_MODE_ALTERNATE,
        .type = QUASAR_GPIO_TYPE_PP,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate = QUASAR_GPIO_ALTERNATE_AF5,
    };
    quasar_gpio_config_t gpio_config_radio1_mosi = {
        .port = QUASAR_DEF_RADIO_1_MOSI_PORT,
        .pin = QUASAR_DEF_RADIO_1_MOSI_PIN,
        .mode = QUASAR_GPIO_MODE_ALTERNATE,
        .type = QUASAR_GPIO_TYPE_PP,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate = QUASAR_GPIO_ALTERNATE_AF5,
    };
    quasar_gpio_config_t gpio_config_radio1_cs = {
        .port = QUASAR_DEF_RADIO_1_CS_PORT,
        .pin = QUASAR_DEF_RADIO_1_CS_PIN,
        .mode = QUASAR_GPIO_MODE_OUTPUT,
        .type = QUASAR_GPIO_TYPE_PP,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_spi_config_t radio1_spi_config = {
        .spi_selection = QUASAR_DEF_SPI_SELECTION_RADIO_1,
        .gpio_config_sck = gpio_config_radio1_sck,
        .gpio_config_miso = gpio_config_radio1_miso,
        .gpio_config_mosi = gpio_config_radio1_mosi,
        .gpio_config_ncs = gpio_config_radio1_cs,
        .clk_source = QUASAR_SPI_CLK_SOURCE_SYSCLK,
        .spi_prescaler = SPI_BAUDRATEPRESCALER_4,
    };

    /* Radio 1 GPIOs config (reset, shutdown and irq pin). */
    quasar_gpio_config_t radio1_gpio_config_reset = {
        .port = QUASAR_DEF_RADIO_1_RESET_PORT,
        .pin = QUASAR_DEF_RADIO_1_RESET_PIN,
        .mode = QUASAR_GPIO_MODE_OUTPUT,
        .type = QUASAR_GPIO_TYPE_PP,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_config_t radio1_gpio_config_shutdown = {
        .port = QUASAR_DEF_RADIO_1_SHUTDOWN_PORT,
        .pin = QUASAR_DEF_RADIO_1_SHUTDOWN_PIN,
        .mode = QUASAR_GPIO_MODE_OUTPUT,
        .type = QUASAR_GPIO_TYPE_PP,
        .pull = QUASAR_GPIO_PULL_UP,
        .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_config_t radio1_gpio_config_irq = {
        .port = QUASAR_DEF_RADIO_1_IRQ_PORT,
        .pin = QUASAR_DEF_RADIO_1_IRQ_PIN,
        .mode = QUASAR_GPIO_MODE_INPUT,
        .type = QUASAR_GPIO_TYPE_NONE,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_NONE,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };

    /* Radio 1 DMA config. */
    quasar_dma_config_t radio1_dma_config = {
        .dma_peripheral = QUASAR_DMA_PERIPHERAL_SPI,
        .peripheral_selection = QUASAR_DEF_SPI_SELECTION_RADIO_1,
        .dma_selection_rx = QUASAR_DEF_DMA_SELECTION_RADIO_1_RX,
        .dma_selection_tx = QUASAR_DEF_DMA_SELECTION_RADIO_1_TX,
        .irq_priority = QUASAR_DEF_PRIO_RADIO_1_DMA_IRQ,
    };

#if 0 /* QSPI DISABLED */
    /* Radio 1 QSPI config. */
    quasar_qspi_config_t radio1_qspi_config = {
        .gpio_config_io_0 =
            {
                .port = QUASAR_DEF_RADIO_1_QSPI_IO_0_PORT,
                .pin = QUASAR_DEF_RADIO_1_QSPI_IO_0_PIN,
                .mode = QUASAR_GPIO_MODE_ALTERNATE,
                .type = QUASAR_GPIO_TYPE_PP,
                .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
                .pull = QUASAR_GPIO_PULL_NONE,
                .alternate = QUASAR_GPIO_ALTERNATE_AF10,
            },
        .gpio_config_io_1 =
            {
                .port = QUASAR_DEF_RADIO_1_QSPI_IO_1_PORT,
                .pin = QUASAR_DEF_RADIO_1_QSPI_IO_1_PIN,
                .type = QUASAR_GPIO_TYPE_PP,
                .mode = QUASAR_GPIO_MODE_ALTERNATE,
                .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
                .pull = QUASAR_GPIO_PULL_NONE,
                .alternate = QUASAR_GPIO_ALTERNATE_AF10,
            },
        .gpio_config_io_2 =
            {
                .port = QUASAR_DEF_RADIO_1_QSPI_IO_2_PORT,
                .pin = QUASAR_DEF_RADIO_1_QSPI_IO_2_PIN,
                .mode = QUASAR_GPIO_MODE_ALTERNATE,
                .type = QUASAR_GPIO_TYPE_PP,
                .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
                .pull = QUASAR_GPIO_PULL_NONE,
                .alternate = QUASAR_GPIO_ALTERNATE_AF10,
            },
        .gpio_config_io_3 =
            {
                .port = QUASAR_DEF_RADIO_1_QSPI_IO_3_PORT,
                .pin = QUASAR_DEF_RADIO_1_QSPI_IO_3_PIN,
                .mode = QUASAR_GPIO_MODE_ALTERNATE,
                .type = QUASAR_GPIO_TYPE_PP,
                .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
                .pull = QUASAR_GPIO_PULL_NONE,
                .alternate = QUASAR_GPIO_ALTERNATE_AF10,
            },
        .gpio_config_sck =
            {
                .port = QUASAR_DEF_RADIO_1_QSPI_SCK_PORT,
                .pin = QUASAR_DEF_RADIO_1_QSPI_SCK_PIN,
                .mode = QUASAR_GPIO_MODE_ALTERNATE,
                .type = QUASAR_GPIO_TYPE_PP,
                .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
                .pull = QUASAR_GPIO_PULL_NONE,
                .alternate = QUASAR_GPIO_ALTERNATE_AF3,
            },
        .irq_priority = QUASAR_IRQ_PRIORITY_2,
        .qspi_prescaler = 4,
        .qspi_selection = QUASAR_QSPI_SELECTION_OSPI1,
        .dma_config =
            {
                .dma_selection_tx = QUASAR_DEF_DMA_SELECTION_RADIO_1_TX,
                .dma_selection_rx = QUASAR_DEF_DMA_SELECTION_RADIO_1_RX,
                .dma_peripheral = QUASAR_DMA_PERIPHERAL_QSPI,
                .peripheral_selection = QUASAR_QSPI_SELECTION_OSPI1,
                .irq_priority = QUASAR_DEF_PRIO_RADIO_1_DMA_IRQ,
            },
    };
#endif

    quasar_radio_config_t radio_config = {
        .dma_config = radio1_dma_config,
        .spi_config = radio1_spi_config,
        .reset_io = radio1_gpio_config_reset,
        .shutdown_io = radio1_gpio_config_shutdown,
        .irq_io = radio1_gpio_config_irq,
        .irq_priority_of_irq_io = QUASAR_DEF_PRIO_RADIO_1_IRQ,
#if 0 /* QSPI DISABLED */
        .qspi_config = radio1_qspi_config,
#endif
    };

    return radio_config;
}

#if 1 /* RADIO 2 ENABLED (535 dual radio) */
/** @brief Get the radio 2 configuration.
 *
 *  @note Depending on the board revision the MOSI pin GPIO differs.
 *
 *  @param[in]  board_revision  The board revision.
 *  @param[out] err             Pointer to store error status.
 *  @return Configuration of the radio 2.
 */
static quasar_radio_config_t radio_2_get_config(quasar_revision_t board_revision, quasar_bsp_status_t *err)
{
    /* No failure path remains here (any revision maps to PD4 MOSI). */
    *err = QUASAR_OK;

    /* Radio 2 SPI config and its four associated GPIOs. */
    quasar_gpio_config_t gpio_config_radio2_sck = {
        .port = QUASAR_DEF_RADIO_2_SCK_PORT,
        .pin = QUASAR_DEF_RADIO_2_SCK_PIN,
        .mode = QUASAR_GPIO_MODE_ALTERNATE,
        .type = QUASAR_GPIO_TYPE_PP,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate = QUASAR_GPIO_ALTERNATE_AF5,
    };
    quasar_gpio_config_t gpio_config_radio2_miso = {
        .port = QUASAR_DEF_RADIO_2_MISO_PORT,
        .pin = QUASAR_DEF_RADIO_2_MISO_PIN,
        .mode = QUASAR_GPIO_MODE_ALTERNATE,
        .type = QUASAR_GPIO_TYPE_PP,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate = QUASAR_GPIO_ALTERNATE_AF5,
    };

    /* The radio 2 MOSI pin differs depending on the board revision. */
    quasar_gpio_config_t gpio_config_radio2_mosi = {0};

    if (board_revision == QUASAR_REVA) {
        gpio_config_radio2_mosi.port = QUASAR_DEF_RADIO_2_MOSI_PORT_REVA;
        gpio_config_radio2_mosi.pin = QUASAR_DEF_RADIO_2_MOSI_PIN_REVA;
        gpio_config_radio2_mosi.mode = QUASAR_GPIO_MODE_ALTERNATE;
        gpio_config_radio2_mosi.type = QUASAR_GPIO_TYPE_PP;
        gpio_config_radio2_mosi.pull = QUASAR_GPIO_PULL_NONE;
        gpio_config_radio2_mosi.speed = QUASAR_GPIO_SPEED_VERY_HIGH;
        gpio_config_radio2_mosi.alternate = QUASAR_GPIO_ALTERNATE_AF5;
    } else {
        /*
         * REVB or unknown -> use the REVB MOSI map. On this U535 board the board
         * revision is read from the PC0 divider via ADC, but that ADC GPIO is
         * not configured (commented out in quasar_adc_init), so the reading is
         * unreliable and frequently lands outside {REVA, REVB}. The old code
         * asserted QUASAR_ERR_RADIO_UNSUPPORTED_REVISION here, trapping radio 2
         * init in the error handler. Defaulting to REVB is safe because RADIO_2
         * MOSI is PD4 for BOTH revisions, so the revision gate has no effect on
         * the pin map - it only blocked bring-up.
         */
        gpio_config_radio2_mosi.port = QUASAR_DEF_RADIO_2_MOSI_PORT_REVB;
        gpio_config_radio2_mosi.pin = QUASAR_DEF_RADIO_2_MOSI_PIN_REVB;
        gpio_config_radio2_mosi.mode = QUASAR_GPIO_MODE_ALTERNATE;
        gpio_config_radio2_mosi.type = QUASAR_GPIO_TYPE_PP;
        gpio_config_radio2_mosi.pull = QUASAR_GPIO_PULL_NONE;
        gpio_config_radio2_mosi.speed = QUASAR_GPIO_SPEED_VERY_HIGH;
        gpio_config_radio2_mosi.alternate = QUASAR_GPIO_ALTERNATE_AF5;
    }

    quasar_gpio_config_t gpio_config_radio2_cs = {
        .port = QUASAR_DEF_RADIO_2_CS_PORT,
        .pin = QUASAR_DEF_RADIO_2_CS_PIN,
        .mode = QUASAR_GPIO_MODE_OUTPUT,
        .type = QUASAR_GPIO_TYPE_PP,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_spi_config_t radio2_spi_config = {
        .spi_selection = QUASAR_DEF_SPI_SELECTION_RADIO_2,
        .gpio_config_sck = gpio_config_radio2_sck,
        .gpio_config_miso = gpio_config_radio2_miso,
        .gpio_config_mosi = gpio_config_radio2_mosi,
        .gpio_config_ncs = gpio_config_radio2_cs,
        .clk_source = QUASAR_SPI_CLK_SOURCE_SYSCLK,
        .spi_prescaler = SPI_BAUDRATEPRESCALER_4,
    };

    /* Radio 2 GPIOs config (reset, shutdown and irq pin). */
    quasar_gpio_config_t radio2_gpio_config_reset = {
        .port = QUASAR_DEF_RADIO_2_RESET_PORT,
        .pin = QUASAR_DEF_RADIO_2_RESET_PIN,
        .mode = QUASAR_GPIO_MODE_OUTPUT,
        .type = QUASAR_GPIO_TYPE_PP,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_config_t radio2_gpio_config_shutdown = {
        .port = QUASAR_DEF_RADIO_2_SHUTDOWN_PORT,
        .pin = QUASAR_DEF_RADIO_2_SHUTDOWN_PIN,
        .mode = QUASAR_GPIO_MODE_OUTPUT,
        .type = QUASAR_GPIO_TYPE_PP,
        .pull = QUASAR_GPIO_PULL_UP,
        .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_config_t radio2_gpio_config_irq = {
        .port = QUASAR_DEF_RADIO_2_IRQ_PORT,
        .pin = QUASAR_DEF_RADIO_2_IRQ_PIN,
        .mode = QUASAR_GPIO_MODE_INPUT,
        .type = QUASAR_GPIO_TYPE_NONE,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_NONE,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };

    /* Radio 2 DMA config. */
    quasar_dma_config_t radio2_dma_config = {
        .dma_selection_tx = QUASAR_DEF_DMA_SELECTION_RADIO_2_TX,
        .dma_selection_rx = QUASAR_DEF_DMA_SELECTION_RADIO_2_RX,
        .dma_peripheral = QUASAR_DMA_PERIPHERAL_SPI,
        .peripheral_selection = QUASAR_DEF_SPI_SELECTION_RADIO_2,
        .irq_priority = QUASAR_DEF_PRIO_RADIO_2_DMA_IRQ,
    };

#if 0 /* QSPI DISABLED */
    /* Radio 2 QSPI config. */
    quasar_qspi_config_t radio2_qspi_config = {
        .gpio_config_io_0 =
            {
                .port = QUASAR_DEF_RADIO_2_QSPI_IO_0_PORT,
                .pin = QUASAR_DEF_RADIO_2_QSPI_IO_0_PIN,
                .mode = QUASAR_GPIO_MODE_ALTERNATE,
                .type = QUASAR_GPIO_TYPE_PP,
                .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
                .pull = QUASAR_GPIO_PULL_NONE,
                .alternate = QUASAR_GPIO_ALTERNATE_AF6,
            },
        .gpio_config_io_1 =
            {
                .port = QUASAR_DEF_RADIO_2_QSPI_IO_1_PORT,
                .pin = QUASAR_DEF_RADIO_2_QSPI_IO_1_PIN,
                .mode = QUASAR_GPIO_MODE_ALTERNATE,
                .type = QUASAR_GPIO_TYPE_PP,
                .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
                .pull = QUASAR_GPIO_PULL_NONE,
                .alternate = QUASAR_GPIO_ALTERNATE_AF6,
            },
        .gpio_config_io_2 =
            {
                .port = QUASAR_DEF_RADIO_2_QSPI_IO_2_PORT,
                .pin = QUASAR_DEF_RADIO_2_QSPI_IO_2_PIN,
                .mode = QUASAR_GPIO_MODE_ALTERNATE,
                .type = QUASAR_GPIO_TYPE_PP,
                .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
                .pull = QUASAR_GPIO_PULL_NONE,
                .alternate = QUASAR_GPIO_ALTERNATE_AF6,
            },
        .gpio_config_io_3 =
            {
                .port = QUASAR_DEF_RADIO_2_QSPI_IO_3_PORT,
                .pin = QUASAR_DEF_RADIO_2_QSPI_IO_3_PIN,
                .mode = QUASAR_GPIO_MODE_ALTERNATE,
                .type = QUASAR_GPIO_TYPE_PP,
                .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
                .pull = QUASAR_GPIO_PULL_NONE,
                .alternate = QUASAR_GPIO_ALTERNATE_AF5,
            },
        .gpio_config_sck =
            {
                .port = QUASAR_DEF_RADIO_2_QSPI_SCK_PORT,
                .pin = QUASAR_DEF_RADIO_2_QSPI_SCK_PIN,
                .mode = QUASAR_GPIO_MODE_ALTERNATE,
                .type = QUASAR_GPIO_TYPE_PP,
                .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
                .pull = QUASAR_GPIO_PULL_NONE,
                .alternate = QUASAR_GPIO_ALTERNATE_AF5,
            },
        .irq_priority = QUASAR_IRQ_PRIORITY_2,
        .qspi_prescaler = 4,
        .qspi_selection = QUASAR_QSPI_SELECTION_OSPI2,
        .dma_config =
            {
                .dma_selection_tx = QUASAR_DEF_DMA_SELECTION_RADIO_2_TX,
                .dma_selection_rx = QUASAR_DEF_DMA_SELECTION_RADIO_2_RX,
                .dma_peripheral = QUASAR_DMA_PERIPHERAL_QSPI,
                .peripheral_selection = QUASAR_QSPI_SELECTION_OSPI2,
                .irq_priority = QUASAR_DEF_PRIO_RADIO_2_DMA_IRQ,
            },
    };
#endif

    quasar_radio_config_t radio_config = {
        .dma_config = radio2_dma_config,
        .spi_config = radio2_spi_config,
        .reset_io = radio2_gpio_config_reset,
        .shutdown_io = radio2_gpio_config_shutdown,
        .irq_io = radio2_gpio_config_irq,
        .irq_priority_of_irq_io = QUASAR_DEF_PRIO_RADIO_2_IRQ,
#if 0 /* QSPI DISABLED */
        .qspi_config = radio2_qspi_config,
#endif
    };

    return radio_config;
}
#endif

/** @brief Initializes the radio module.
 *
 *  This function handles the initialization of the radio's GPIOs, SPI, and DMA. It also resets the radio module.
 *
 *  @param[in]  radio_config  Configuration parameters for the radio initialization.
 *  @param[out] err           Pointer to store error status.
 */
static void radio_init(quasar_radio_config_t radio_config, quasar_bsp_status_t *err)
{
    /* Initialize radio GPIOs. */
    quasar_gpio_init(radio_config.reset_io);
    quasar_gpio_init(radio_config.shutdown_io);
    quasar_gpio_init(radio_config.irq_io);
    quasar_gpio_configure_irq(radio_config.irq_io.port, radio_config.irq_io.pin, radio_config.irq_priority_of_irq_io);

#if 0 /* QSPI DISABLED */
#if RADIO_QSPI_ENABLED
    quasar_gpio_init(radio_config.spi_config.gpio_config_ncs);
    quasar_qspi_init(radio_config.qspi_config);
    quasar_qspi_set_mode(radio_config.qspi_config.qspi_selection, QUASAR_QSPI_1_LINE_MODE_0_DUMMY);
    quasar_dma_qspi_populate_handle(radio_config.qspi_config.dma_config);

    /* Initialise radio DMA. */
    quasar_dma_init(radio_config.qspi_config.dma_config, err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);
#else
    /* Initialize radio SPI */
    quasar_spi_init(radio_config.spi_config, err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);

    /* Initialise radio DMA */
    quasar_dma_init(radio_config.dma_config, err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);
#endif
#else
    /* Initialize radio SPI */
    quasar_spi_init(radio_config.spi_config, err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);

    /* Initialise radio DMA */
    quasar_dma_init(radio_config.dma_config, err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);
#endif

    /* Reset the Radio. */
    quasar_gpio_clear(radio_config.reset_io.port, radio_config.reset_io.pin);
    HAL_Delay(50);
    quasar_gpio_set(radio_config.reset_io.port, radio_config.reset_io.pin);
    HAL_Delay(50);
}

/** @brief Deinitialize the radio module.
 *
 *  This function handles the deinitialization of the radio's GPIOs, SPI, and DMA.
 *
 *  @param[in]  radio_config  Configuration parameters for the radio deinitialization.
 *  @param[out] err           Pointer to store error status.
 */
static void radio_deinit(quasar_radio_config_t radio_config, quasar_bsp_status_t *err)
{
    /* Deinitialize the DMA of the SPI for the radio. */
    quasar_dma_deinit(radio_config.dma_config, err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);

    /* Deinitialize the SPI associated with the radio. */
    quasar_spi_deinit(radio_config.spi_config, err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);

    /* Deinitialize the GPIOs associated with the radio */
    quasar_gpio_deinit(radio_config.reset_io.port, radio_config.reset_io.pin);
    quasar_gpio_deinit(radio_config.shutdown_io.port, radio_config.shutdown_io.pin);
    quasar_gpio_deinit(radio_config.irq_io.port, radio_config.irq_io.pin);
}

/** @brief Initialize the GPIO associated with the debug enable pin of the radio 1.
 */
static void radio_init_debug_enable_gpio(void)
{
    quasar_gpio_config_t radio1_gpio_config_debug = {
        .port = QUASAR_DEF_RADIO_1_DBG_EN_PORT,
        .pin = QUASAR_DEF_RADIO_1_DBG_EN_PIN,
        .mode = QUASAR_GPIO_MODE_OUTPUT,
        .type = QUASAR_GPIO_TYPE_PP,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_LOW,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(radio1_gpio_config_debug);
}

/** @brief Enable the PendSV IRQ priority.
 *
 *  @note This software IRQ is used to handle the Wireless Core callback functions.
 *
 *  @param[in] pendsv_prio  PendSV priority.
 */
static void enable_pendsv_irq(uint32_t pendsv_prio)
{
    HAL_NVIC_SetPriority(PendSV_IRQn, pendsv_prio, 0);
    HAL_NVIC_ClearPendingIRQ(PendSV_IRQn);
    HAL_NVIC_EnableIRQ(PendSV_IRQn);
}
