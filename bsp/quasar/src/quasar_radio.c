/** @file  quasar_radio.h
 *  @brief This module provides functions for the SPARK Radio module.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_radio.h"
#include "quasar_def.h"
#include "quasar_it.h"
#include "quasar_clock.h"
#include "quasar_spi.h"

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static quasar_radio_config_t radio_1_get_config(void);
static quasar_radio_config_t radio_2_get_config(quasar_revision_t board_revision);
static void radio_init(quasar_radio_config_t radio_config);
static void radio_deinit(quasar_radio_config_t radio_config);
static void radio_init_debug_enable_gpio(void);

/* PUBLIC FUNCTIONS ***********************************************************/
void quasar_radio_1_init(void)
{
    quasar_radio_config_t radio1_config = radio_1_get_config();

    radio_init(radio1_config);

    /* This pin is only available for the radio 1. */
    radio_init_debug_enable_gpio();
}

void quasar_radio_2_init(quasar_revision_t board_revision)
{
    quasar_radio_config_t radio2_config = radio_2_get_config(board_revision);

    radio_init(radio2_config);
}

void quasar_radio_1_deinit(void)
{
    quasar_radio_config_t radio1_config = radio_1_get_config();

    radio_deinit(radio1_config);
}

void quasar_radio_2_deinit(quasar_revision_t board_revision)
{
    quasar_radio_config_t radio2_config = radio_2_get_config(board_revision);

    radio_deinit(radio2_config);
}

void quasar_radio_set_radio_1_irq_callback(void (*irq_callback)(void))
{
    quasar_it_set_exti8_irq_callback(irq_callback);
}

void quasar_radio_set_radio_2_irq_callback(void (*irq_callback)(void))
{
    quasar_it_set_exti7_irq_callback(irq_callback);
}

void quasar_radio_set_radio_1_dma_callback(void (*irq_callback)(void))
{
    quasar_dma_set_channel2_dma_callback(irq_callback);
}

void quasar_radio_set_radio_2_dma_callback(void (*irq_callback)(void))
{
    quasar_dma_set_channel6_dma_callback(irq_callback);
}

bool quasar_radio_1_read_irq_pin(void)
{
    return quasar_gpio_read_state(QUASAR_DEF_RADIO_1_IRQ_PORT, QUASAR_DEF_RADIO_1_IRQ_PIN);
}

bool quasar_radio_2_read_irq_pin(void)
{
    return quasar_gpio_read_state(QUASAR_DEF_RADIO_2_IRQ_PORT, QUASAR_DEF_RADIO_2_IRQ_PIN);
}

void quasar_radio_1_enable_irq_it(void)
{
    quasar_gpio_enable_irq(QUASAR_DEF_RADIO_1_IRQ_PIN);
}

void quasar_radio_2_enable_irq_it(void)
{
    quasar_gpio_enable_irq(QUASAR_DEF_RADIO_2_IRQ_PIN);
}

void quasar_radio_1_disable_irq_it(void)
{
    quasar_gpio_disable_irq(QUASAR_DEF_RADIO_1_IRQ_PIN);
}

void quasar_radio_2_disable_irq_it(void)
{
    quasar_gpio_disable_irq(QUASAR_DEF_RADIO_2_IRQ_PIN);
}

void quasar_radio_1_enable_dma_irq_it(void)
{
    quasar_dma_enable_irq(QUASAR_DEF_DMA_SELECTION_RADIO_1_RX);
}

void quasar_radio_2_enable_dma_irq_it(void)
{
    quasar_dma_enable_irq(QUASAR_DEF_DMA_SELECTION_RADIO_2_RX);
}

void quasar_radio_1_disable_dma_irq_it(void)
{
    quasar_dma_disable_irq(QUASAR_DEF_DMA_SELECTION_RADIO_1_RX);
}

void quasar_radio_2_disable_dma_irq_it(void)
{
    quasar_dma_disable_irq(QUASAR_DEF_DMA_SELECTION_RADIO_2_RX);
}

void quasar_radio_1_set_shutdown_pin(void)
{
    quasar_gpio_set(QUASAR_DEF_RADIO_1_SHUTDOWN_PORT, QUASAR_DEF_RADIO_1_SHUTDOWN_PIN);
}

void quasar_radio_2_set_shutdown_pin(void)
{
    quasar_gpio_set(QUASAR_DEF_RADIO_2_SHUTDOWN_PORT, QUASAR_DEF_RADIO_2_SHUTDOWN_PIN);
}

void quasar_radio_1_reset_shutdown_pin(void)
{
    quasar_gpio_clear(QUASAR_DEF_RADIO_1_SHUTDOWN_PORT, QUASAR_DEF_RADIO_1_SHUTDOWN_PIN);
}

void quasar_radio_2_reset_shutdown_pin(void)
{
    quasar_gpio_clear(QUASAR_DEF_RADIO_2_SHUTDOWN_PORT, QUASAR_DEF_RADIO_2_SHUTDOWN_PIN);
}

void quasar_radio_1_set_reset_pin(void)
{
    quasar_gpio_set(QUASAR_DEF_RADIO_1_RESET_PORT, QUASAR_DEF_RADIO_1_RESET_PIN);
}

void quasar_radio_2_set_reset_pin(void)
{
    quasar_gpio_set(QUASAR_DEF_RADIO_2_RESET_PORT, QUASAR_DEF_RADIO_2_RESET_PIN);
}

void quasar_radio_1_reset_reset_pin(void)
{
    quasar_gpio_clear(QUASAR_DEF_RADIO_1_RESET_PORT, QUASAR_DEF_RADIO_1_RESET_PIN);
}

void quasar_radio_2_reset_reset_pin(void)
{
    quasar_gpio_clear(QUASAR_DEF_RADIO_2_RESET_PORT, QUASAR_DEF_RADIO_2_RESET_PIN);
}

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
    quasar_gpio_set_pending(QUASAR_DEF_RADIO_1_IRQ_PIN);
}

void quasar_radio_2_context_switch(void)
{
    quasar_gpio_set_pending(QUASAR_DEF_RADIO_2_IRQ_PIN);
}

void quasar_radio_callback_context_switch(void)
{
    SET_BIT(SCB->ICSR, SCB_ICSR_PENDSVSET_Msk);
}

void quasar_radio_1_set_spi_baudrate(quasar_spi_prescaler_t prescaler)
{
    quasar_spi_set_baudrate(QUASAR_DEF_SPI_SELECTION_RADIO_1, prescaler);
}

void quasar_radio_2_set_spi_baudrate(quasar_spi_prescaler_t prescaler)
{
    quasar_spi_set_baudrate(QUASAR_DEF_SPI_SELECTION_RADIO_2, prescaler);
}

void quasar_radio_1_spi_set_cs(void)
{
    quasar_gpio_set(QUASAR_DEF_RADIO_1_CS_PORT, QUASAR_DEF_RADIO_1_CS_PIN);
}

void quasar_radio_2_spi_set_cs(void)
{
    quasar_gpio_set(QUASAR_DEF_RADIO_2_CS_PORT, QUASAR_DEF_RADIO_2_CS_PIN);
}

void quasar_radio_1_spi_reset_cs(void)
{
    quasar_gpio_clear(QUASAR_DEF_RADIO_1_CS_PORT, QUASAR_DEF_RADIO_1_CS_PIN);
}

void quasar_radio_2_spi_reset_cs(void)
{
    quasar_gpio_clear(QUASAR_DEF_RADIO_2_CS_PORT, QUASAR_DEF_RADIO_2_CS_PIN);
}

void quasar_radio_1_spi_transfer_full_duplex_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    quasar_spi_transfer_full_duplex_blocking(QUASAR_DEF_SPI_SELECTION_RADIO_1, tx_data, rx_data, size);
}

void quasar_radio_2_spi_transfer_full_duplex_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    quasar_spi_transfer_full_duplex_blocking(QUASAR_DEF_SPI_SELECTION_RADIO_2, tx_data, rx_data, size);
}

void quasar_radio_1_spi_transfer_full_duplex_non_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
   quasar_spi_transfer_full_duplex_non_blocking(QUASAR_DEF_SPI_SELECTION_RADIO_1, tx_data, rx_data, size);
}

void quasar_radio_2_spi_transfer_full_duplex_non_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    quasar_spi_transfer_full_duplex_non_blocking(QUASAR_DEF_SPI_SELECTION_RADIO_2, tx_data, rx_data, size);
}

bool quasar_radio_1_is_spi_busy(void)
{
    bool is_busy = quasar_spi_is_busy(QUASAR_DEF_SPI_SELECTION_RADIO_1);
    return is_busy;
}

bool quasar_radio_2_is_spi_busy(void)
{
    bool is_busy = quasar_spi_is_busy(QUASAR_DEF_SPI_SELECTION_RADIO_2);
    return is_busy;
}

void quasar_radio_init_unused_qspi_gpios(void)
{
    quasar_gpio_config_t gpio_config1 = {
        .port         = QUASAR_DEF_RADIO_1_QSPI_IO_0_PORT,
        .pin          = QUASAR_DEF_RADIO_1_QSPI_IO_0_PIN,
        .mode         = QUASAR_GPIO_MODE_INPUT,
        .type         = QUASAR_GPIO_TYPE_NONE,
        .pull         = QUASAR_GPIO_PULL_NONE,
        .speed        = QUASAR_GPIO_SPEED_NONE,
        .alternate    = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config1);

    quasar_gpio_config_t gpio_config2 = {
        .port         = QUASAR_DEF_RADIO_1_QSPI_IO_1_PORT,
        .pin          = QUASAR_DEF_RADIO_1_QSPI_IO_1_PIN,
        .mode         = QUASAR_GPIO_MODE_INPUT,
        .type         = QUASAR_GPIO_TYPE_NONE,
        .pull         = QUASAR_GPIO_PULL_NONE,
        .speed        = QUASAR_GPIO_SPEED_NONE,
        .alternate    = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config2);

    quasar_gpio_config_t gpio_config3 = {
        .port         = QUASAR_DEF_RADIO_1_QSPI_IO_2_PORT,
        .pin          = QUASAR_DEF_RADIO_1_QSPI_IO_2_PIN,
        .mode         = QUASAR_GPIO_MODE_INPUT,
        .type         = QUASAR_GPIO_TYPE_NONE,
        .pull         = QUASAR_GPIO_PULL_NONE,
        .speed        = QUASAR_GPIO_SPEED_NONE,
        .alternate    = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config3);

    quasar_gpio_config_t gpio_config4 = {
        .port         = QUASAR_DEF_RADIO_1_QSPI_IO_3_PORT,
        .pin          = QUASAR_DEF_RADIO_1_QSPI_IO_3_PIN,
        .mode         = QUASAR_GPIO_MODE_INPUT,
        .type         = QUASAR_GPIO_TYPE_NONE,
        .pull         = QUASAR_GPIO_PULL_NONE,
        .speed        = QUASAR_GPIO_SPEED_NONE,
        .alternate    = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config4);

    quasar_gpio_config_t gpio_config5 = {
        .port         = QUASAR_DEF_RADIO_2_QSPI_IO_0_PORT,
        .pin          = QUASAR_DEF_RADIO_2_QSPI_IO_0_PIN,
        .mode         = QUASAR_GPIO_MODE_INPUT,
        .type         = QUASAR_GPIO_TYPE_NONE,
        .pull         = QUASAR_GPIO_PULL_NONE,
        .speed        = QUASAR_GPIO_SPEED_NONE,
        .alternate    = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config5);

    quasar_gpio_config_t gpio_config6 = {
        .port         = QUASAR_DEF_RADIO_2_QSPI_IO_1_PORT,
        .pin          = QUASAR_DEF_RADIO_2_QSPI_IO_1_PIN,
        .mode         = QUASAR_GPIO_MODE_INPUT,
        .type         = QUASAR_GPIO_TYPE_NONE,
        .pull         = QUASAR_GPIO_PULL_NONE,
        .speed        = QUASAR_GPIO_SPEED_NONE,
        .alternate    = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config6);

    quasar_gpio_config_t gpio_config7 = {
        .port         = QUASAR_DEF_RADIO_2_QSPI_IO_2_PORT,
        .pin          = QUASAR_DEF_RADIO_2_QSPI_IO_2_PIN,
        .mode         = QUASAR_GPIO_MODE_INPUT,
        .type         = QUASAR_GPIO_TYPE_NONE,
        .pull         = QUASAR_GPIO_PULL_NONE,
        .speed        = QUASAR_GPIO_SPEED_NONE,
        .alternate    = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config7);

    quasar_gpio_config_t gpio_config8 = {
        .port         = QUASAR_DEF_RADIO_2_QSPI_IO_3_PORT,
        .pin          = QUASAR_DEF_RADIO_2_QSPI_IO_3_PIN,
        .mode         = QUASAR_GPIO_MODE_INPUT,
        .type         = QUASAR_GPIO_TYPE_NONE,
        .pull         = QUASAR_GPIO_PULL_NONE,
        .speed        = QUASAR_GPIO_SPEED_NONE,
        .alternate    = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config8);

    quasar_gpio_config_t gpio_config9 = {
        .port         = QUASAR_DEF_RADIO_1_QSPI_SCK_PORT,
        .pin          = QUASAR_DEF_RADIO_1_QSPI_SCK_PIN,
        .mode         = QUASAR_GPIO_MODE_INPUT,
        .type         = QUASAR_GPIO_TYPE_NONE,
        .pull         = QUASAR_GPIO_PULL_NONE,
        .speed        = QUASAR_GPIO_SPEED_NONE,
        .alternate    = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config9);

    quasar_gpio_config_t gpio_config10 = {
        .port         = QUASAR_DEF_RADIO_2_QSPI_SCK_PORT,
        .pin          = QUASAR_DEF_RADIO_2_QSPI_SCK_PIN,
        .mode         = QUASAR_GPIO_MODE_INPUT,
        .type         = QUASAR_GPIO_TYPE_NONE,
        .pull         = QUASAR_GPIO_PULL_NONE,
        .speed        = QUASAR_GPIO_SPEED_NONE,
        .alternate    = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config10);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Get the radio 1 configuration.
 *
 *  @return Configuration of the radio 1.
 */
static quasar_radio_config_t radio_1_get_config(void)
{
    /* Radio 1 SPI config and its four associated GPIOs */
    quasar_gpio_config_t gpio_config_radio1_sck = {
        .port         = QUASAR_DEF_RADIO_1_SCK_PORT,
        .pin          = QUASAR_DEF_RADIO_1_SCK_PIN,
        .mode         = QUASAR_GPIO_MODE_ALTERNATE,
        .type         = QUASAR_GPIO_TYPE_PP,
        .pull         = QUASAR_GPIO_PULL_NONE,
        .speed        = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate    = QUASAR_GPIO_ALTERNATE_AF5
    };
    quasar_gpio_config_t gpio_config_radio1_miso = {
        .port         = QUASAR_DEF_RADIO_1_MISO_PORT,
        .pin          = QUASAR_DEF_RADIO_1_MISO_PIN,
        .mode         = QUASAR_GPIO_MODE_ALTERNATE,
        .type         = QUASAR_GPIO_TYPE_PP,
        .pull         = QUASAR_GPIO_PULL_NONE,
        .speed        = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate    = QUASAR_GPIO_ALTERNATE_AF5
    };
    quasar_gpio_config_t gpio_config_radio1_mosi = {
        .port         = QUASAR_DEF_RADIO_1_MOSI_PORT,
        .pin          = QUASAR_DEF_RADIO_1_MOSI_PIN,
        .mode         = QUASAR_GPIO_MODE_ALTERNATE,
        .type         = QUASAR_GPIO_TYPE_PP,
        .pull         = QUASAR_GPIO_PULL_NONE,
        .speed        = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate    = QUASAR_GPIO_ALTERNATE_AF5
    };
    quasar_gpio_config_t gpio_config_radio1_cs = {
        .port         = QUASAR_DEF_RADIO_1_CS_PORT,
        .pin          = QUASAR_DEF_RADIO_1_CS_PIN,
        .mode         = QUASAR_GPIO_MODE_OUTPUT,
        .type         = QUASAR_GPIO_TYPE_PP,
        .pull         = QUASAR_GPIO_PULL_NONE,
        .speed        = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate    = QUASAR_GPIO_ALTERNATE_NONE
    };
    quasar_spi_config_t radio1_spi_config = {
        .spi_selection = QUASAR_DEF_SPI_SELECTION_RADIO_1,
        .gpio_config_sck = gpio_config_radio1_sck,
        .gpio_config_miso = gpio_config_radio1_miso,
        .gpio_config_mosi = gpio_config_radio1_mosi,
        .gpio_config_ncs = gpio_config_radio1_cs,
        .clk_source = QUASAR_SPI_CLK_SOURCE_SYSCLK,
        .spi_prescaler = SPI_BAUDRATEPRESCALER_4
    };

    /* Radio 1 GPIOs config (reset, shutdown and irq pin). */
    quasar_gpio_config_t radio1_gpio_config_reset = {
        .port         = QUASAR_DEF_RADIO_1_RESET_PORT,
        .pin          = QUASAR_DEF_RADIO_1_RESET_PIN,
        .mode         = QUASAR_GPIO_MODE_OUTPUT,
        .type         = QUASAR_GPIO_TYPE_PP,
        .pull         = QUASAR_GPIO_PULL_NONE,
        .speed        = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate    = QUASAR_GPIO_ALTERNATE_NONE
    };
    quasar_gpio_config_t radio1_gpio_config_shutdown = {
        .port         = QUASAR_DEF_RADIO_1_SHUTDOWN_PORT,
        .pin          = QUASAR_DEF_RADIO_1_SHUTDOWN_PIN,
        .mode         = QUASAR_GPIO_MODE_OUTPUT,
        .type         = QUASAR_GPIO_TYPE_PP,
        .pull         = QUASAR_GPIO_PULL_UP,
        .speed        = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate    = QUASAR_GPIO_ALTERNATE_NONE
    };
    quasar_gpio_config_t radio1_gpio_config_irq = {
        .port         = QUASAR_DEF_RADIO_1_IRQ_PORT,
        .pin          = QUASAR_DEF_RADIO_1_IRQ_PIN,
        .mode         = QUASAR_GPIO_MODE_INPUT,
        .type         = QUASAR_GPIO_TYPE_NONE,
        .pull         = QUASAR_GPIO_PULL_NONE,
        .speed        = QUASAR_GPIO_SPEED_NONE,
        .alternate    = QUASAR_GPIO_ALTERNATE_NONE
    };

    /* Radio 1 DMA config */
    quasar_dma_config_t radio1_dma_config = {
        .dma_peripheral = QUASAR_DMA_PERIPHERAL_SPI,
        .peripheral_selection = QUASAR_DEF_SPI_SELECTION_RADIO_1,
        .dma_selection_rx = QUASAR_DEF_DMA_SELECTION_RADIO_1_RX,
        .dma_selection_tx = QUASAR_DEF_DMA_SELECTION_RADIO_1_TX,
        .irq_priority = QUASAR_DEF_PRIO_RADIO_1_DMA_IRQ
    };

    quasar_radio_config_t radio_config = {
        .dma_config = radio1_dma_config,
        .spi_config = radio1_spi_config,
        .reset_io = radio1_gpio_config_reset,
        .shutdown_io = radio1_gpio_config_shutdown,
        .irq_io = radio1_gpio_config_irq,
        .irq_priority_of_irq_io = QUASAR_DEF_PRIO_RADIO_1_IRQ
    };
    return radio_config;
}

/** @brief Get the radio 2 configuration.
 *
 *  @note Depending on the board revision the MOSI pin GPIO differs.
 *
 *  @param[in] board_revision  The board revision.
 *  @return Configuration of the radio 2.
 */
static quasar_radio_config_t radio_2_get_config(quasar_revision_t board_revision)
{
    /* Radio 2 SPI config and its four associated GPIOs */
    quasar_gpio_config_t gpio_config_radio2_sck = {
        .port         = QUASAR_DEF_RADIO_2_SCK_PORT,
        .pin          = QUASAR_DEF_RADIO_2_SCK_PIN,
        .mode         = QUASAR_GPIO_MODE_ALTERNATE,
        .type         = QUASAR_GPIO_TYPE_PP,
        .pull         = QUASAR_GPIO_PULL_NONE,
        .speed        = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate    = QUASAR_GPIO_ALTERNATE_AF5
    };
    quasar_gpio_config_t gpio_config_radio2_miso = {
        .port         = QUASAR_DEF_RADIO_2_MISO_PORT,
        .pin          = QUASAR_DEF_RADIO_2_MISO_PIN,
        .mode         = QUASAR_GPIO_MODE_ALTERNATE,
        .type         = QUASAR_GPIO_TYPE_PP,
        .pull         = QUASAR_GPIO_PULL_NONE,
        .speed        = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate    = QUASAR_GPIO_ALTERNATE_AF5
    };

    /* The radio 2 MOSI pin differs depending on the board revision. */
    quasar_gpio_config_t gpio_config_radio2_mosi;

    if (board_revision == QUASAR_REVA) {
        gpio_config_radio2_mosi.port = QUASAR_DEF_RADIO_2_MOSI_PORT_REVA;
        gpio_config_radio2_mosi.pin = QUASAR_DEF_RADIO_2_MOSI_PIN_REVA;
        gpio_config_radio2_mosi.mode = QUASAR_GPIO_MODE_ALTERNATE;
        gpio_config_radio2_mosi.type = QUASAR_GPIO_TYPE_PP;
        gpio_config_radio2_mosi.pull = QUASAR_GPIO_PULL_NONE;
        gpio_config_radio2_mosi.speed = QUASAR_GPIO_SPEED_VERY_HIGH;
        gpio_config_radio2_mosi.alternate = QUASAR_GPIO_ALTERNATE_AF5;
    } else if (board_revision == QUASAR_REVB) {
        gpio_config_radio2_mosi.port = QUASAR_DEF_RADIO_2_MOSI_PORT_REVB;
        gpio_config_radio2_mosi.pin = QUASAR_DEF_RADIO_2_MOSI_PIN_REVB;
        gpio_config_radio2_mosi.mode = QUASAR_GPIO_MODE_ALTERNATE;
        gpio_config_radio2_mosi.type = QUASAR_GPIO_TYPE_PP;
        gpio_config_radio2_mosi.pull = QUASAR_GPIO_PULL_NONE;
        gpio_config_radio2_mosi.speed = QUASAR_GPIO_SPEED_VERY_HIGH;
        gpio_config_radio2_mosi.alternate = QUASAR_GPIO_ALTERNATE_AF5;
    } else {
        /* Unsupported board revision. */
        while (1);
    }

    quasar_gpio_config_t gpio_config_radio2_cs = {
        .port         = QUASAR_DEF_RADIO_2_CS_PORT,
        .pin          = QUASAR_DEF_RADIO_2_CS_PIN,
        .mode         = QUASAR_GPIO_MODE_OUTPUT,
        .type         = QUASAR_GPIO_TYPE_PP,
        .pull         = QUASAR_GPIO_PULL_NONE,
        .speed        = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate    = QUASAR_GPIO_ALTERNATE_NONE
    };
    quasar_spi_config_t radio2_spi_config = {
        .spi_selection = QUASAR_DEF_SPI_SELECTION_RADIO_2,
        .gpio_config_sck = gpio_config_radio2_sck,
        .gpio_config_miso = gpio_config_radio2_miso,
        .gpio_config_mosi = gpio_config_radio2_mosi,
        .gpio_config_ncs = gpio_config_radio2_cs,
        .clk_source = QUASAR_SPI_CLK_SOURCE_SYSCLK,
        .spi_prescaler = SPI_BAUDRATEPRESCALER_4
    };

    /* Radio 2 GPIOs config (reset, shutdown and irq pin). */
    quasar_gpio_config_t radio2_gpio_config_reset = {
        .port         = QUASAR_DEF_RADIO_2_RESET_PORT,
        .pin          = QUASAR_DEF_RADIO_2_RESET_PIN,
        .mode         = QUASAR_GPIO_MODE_OUTPUT,
        .type         = QUASAR_GPIO_TYPE_PP,
        .pull         = QUASAR_GPIO_PULL_NONE,
        .speed        = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate    = QUASAR_GPIO_ALTERNATE_NONE
    };
    quasar_gpio_config_t radio2_gpio_config_shutdown = {
        .port         = QUASAR_DEF_RADIO_2_SHUTDOWN_PORT,
        .pin          = QUASAR_DEF_RADIO_2_SHUTDOWN_PIN,
        .mode         = QUASAR_GPIO_MODE_OUTPUT,
        .type         = QUASAR_GPIO_TYPE_PP,
        .pull         = QUASAR_GPIO_PULL_UP,
        .speed        = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate    = QUASAR_GPIO_ALTERNATE_NONE
    };
    quasar_gpio_config_t radio2_gpio_config_irq = {
        .port         = QUASAR_DEF_RADIO_2_IRQ_PORT,
        .pin          = QUASAR_DEF_RADIO_2_IRQ_PIN,
        .mode         = QUASAR_GPIO_MODE_INPUT,
        .type         = QUASAR_GPIO_TYPE_NONE,
        .pull         = QUASAR_GPIO_PULL_NONE,
        .speed        = QUASAR_GPIO_SPEED_NONE,
        .alternate    = QUASAR_GPIO_ALTERNATE_NONE
    };

    /* Radio 2 DMA config */
    quasar_dma_config_t radio2_dma_config = {
        .dma_peripheral = QUASAR_DMA_PERIPHERAL_SPI,
        .peripheral_selection = QUASAR_DEF_SPI_SELECTION_RADIO_2,
        .dma_selection_rx = QUASAR_DEF_DMA_SELECTION_RADIO_2_RX,
        .dma_selection_tx = QUASAR_DEF_DMA_SELECTION_RADIO_2_TX,
        .irq_priority = QUASAR_DEF_PRIO_RADIO_2_DMA_IRQ
    };

    quasar_radio_config_t radio_config = {
        .dma_config = radio2_dma_config,
        .spi_config = radio2_spi_config,
        .reset_io = radio2_gpio_config_reset,
        .shutdown_io = radio2_gpio_config_shutdown,
        .irq_io = radio2_gpio_config_irq,
        .irq_priority_of_irq_io = QUASAR_DEF_PRIO_RADIO_2_IRQ
    };

    return radio_config;
}

/** @brief Initializes the radio module.
 *
 *  This function handles the initialization of the radio's GPIOs, SPI, and DMA. It also resets the radio module.
 *
 *  @param[in] radio_config  Configuration parameters for the radio initialization.
 */
static void radio_init(quasar_radio_config_t radio_config)
{
    /* Initialize radio GPIOs. */
    quasar_gpio_init(radio_config.reset_io);
    quasar_gpio_init(radio_config.shutdown_io);
    quasar_gpio_init(radio_config.irq_io);
    quasar_gpio_configure_irq(radio_config.irq_io.port, radio_config.irq_io.pin, radio_config.irq_priority_of_irq_io);

    /* Initialize radio SPI */
    quasar_spi_init(radio_config.spi_config);

    /* Initialise radio DMA */
    quasar_dma_init(radio_config.dma_config);

    /* Reset the Radio */
    quasar_gpio_clear(radio_config.reset_io.port, radio_config.reset_io.pin);
    HAL_Delay(50);
    quasar_gpio_set(radio_config.reset_io.port, radio_config.reset_io.pin);
    HAL_Delay(50);
}

/** @brief Deinitialize the radio module.
 *
 *  This function handles the deinitialization of the radio's GPIOs, SPI, and DMA.
 *
 *  @param[in] radio_config  Configuration parameters for the radio deinitialization.
 */
static void radio_deinit(quasar_radio_config_t radio_config)
{
    /* Deinitialize the DMA of the SPI for the radio. */
    quasar_dma_deinit(radio_config.dma_config);
    /* Deinitialize the SPI associated with the radio. */
    quasar_spi_deinit(radio_config.spi_config);
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
        .port         = QUASAR_DEF_RADIO_1_DBG_EN_PORT,
        .pin          = QUASAR_DEF_RADIO_1_DBG_EN_PIN,
        .mode         = QUASAR_GPIO_MODE_OUTPUT,
        .type         = QUASAR_GPIO_TYPE_PP,
        .pull         = QUASAR_GPIO_PULL_NONE,
        .speed        = QUASAR_GPIO_SPEED_LOW,
        .alternate    = QUASAR_GPIO_ALTERNATE_NONE
    };
    quasar_gpio_init(radio1_gpio_config_debug);
}
