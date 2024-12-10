/** @file  quasar_backend.c
 *  @brief Implement swc_hal_facade facade prototype functions.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar.h"
#include "swc_hal_facade.h"

/* CONSTANTS ******************************************************************/
#define FREE_RUNNING_TIMER_PRIORITY       3
#define MULTI_RADIO_MAX_TIMER_PERIOD      0xFFFE
#define QUASAR_FREE_RUNNING_TIMER_FREQ_HZ 1000
#define MULTI_RADIO_TIMER_PRESCALER       8

/* PUBLIC FUNCTIONS ***********************************************************/
/* Context Switching and Interrupt Management */
void swc_hal_radio_1_context_switch(void)
{
    quasar_radio_1_context_switch();
}

void swc_hal_radio_2_context_switch(void)
{
    quasar_radio_2_context_switch();
}

void swc_hal_set_radio_1_irq_callback(void (*callback)(void))
{
    quasar_radio_set_radio_1_irq_callback(callback);
}

void swc_hal_set_radio_2_irq_callback(void (*callback)(void))
{
    quasar_radio_set_radio_2_irq_callback(callback);
}

void swc_hal_set_radio_1_dma_rx_callback(void (*callback)(void))
{
    quasar_radio_set_radio_1_dma_callback(callback);
}

void swc_hal_set_radio_2_dma_rx_callback(void (*callback)(void))
{
    quasar_radio_set_radio_2_dma_callback(callback);
}

void swc_hal_radio_1_disable_irq_it(void)
{
    quasar_radio_1_disable_irq_it();
}

void swc_hal_radio_2_disable_irq_it(void)
{
    quasar_radio_2_disable_irq_it();
}

void swc_hal_radio_1_enable_irq_it(void)
{
    quasar_radio_1_enable_irq_it();
}

void swc_hal_radio_2_enable_irq_it(void)
{
    quasar_radio_2_enable_irq_it();
}

void swc_hal_radio_1_disable_dma_irq_it(void)
{
    quasar_radio_1_disable_dma_irq_it();
}

void swc_hal_radio_2_disable_dma_irq_it(void)
{
    quasar_radio_2_disable_dma_irq_it();
}

void swc_hal_radio_1_enable_dma_irq_it(void)
{
    quasar_radio_1_enable_dma_irq_it();
}

void swc_hal_radio_2_enable_dma_irq_it(void)
{
    quasar_radio_2_enable_dma_irq_it();
}

/* GPIO Controls for Radios */
bool swc_hal_radio_1_read_irq_pin(void)
{
    return quasar_radio_1_read_irq_pin();
}

bool swc_hal_radio_2_read_irq_pin(void)
{
    return quasar_radio_2_read_irq_pin();
}

void swc_hal_radio_1_set_reset_pin(void)
{
    quasar_radio_1_set_reset_pin();
}

void swc_hal_radio_2_set_reset_pin(void)
{
    quasar_radio_2_set_reset_pin();
}

void swc_hal_radio_1_reset_reset_pin(void)
{
    quasar_radio_1_reset_reset_pin();
}

void swc_hal_radio_2_reset_reset_pin(void)
{
    quasar_radio_2_reset_reset_pin();
}

/* SPI Communication */
void swc_hal_radio_1_spi_set_cs(void)
{
    quasar_radio_1_spi_set_cs();
}

void swc_hal_radio_2_spi_set_cs(void)
{
    quasar_radio_2_spi_set_cs();
}

void swc_hal_radio_1_spi_reset_cs(void)
{
    quasar_radio_1_spi_reset_cs();
}

void swc_hal_radio_2_spi_reset_cs(void)
{
    quasar_radio_2_spi_reset_cs();
}

void swc_hal_radio_1_spi_transfer_full_duplex_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    quasar_radio_1_spi_transfer_full_duplex_blocking(tx_data, rx_data, size);
}

void swc_hal_radio_2_spi_transfer_full_duplex_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    quasar_radio_2_spi_transfer_full_duplex_blocking(tx_data, rx_data, size);
}

void swc_hal_radio_1_spi_transfer_full_duplex_non_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    quasar_radio_1_spi_transfer_full_duplex_non_blocking(tx_data, rx_data, size);
}

void swc_hal_radio_2_spi_transfer_full_duplex_non_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    quasar_radio_2_spi_transfer_full_duplex_non_blocking(tx_data, rx_data, size);
}

bool swc_hal_radio_1_is_spi_busy(void)
{
    return quasar_radio_1_is_spi_busy();
}

bool swc_hal_radio_2_is_spi_busy(void)
{
    return quasar_radio_2_is_spi_busy();
}

/* Timer and Delay Management */
void swc_hal_free_running_timer_init(void)
{
    quasar_irq_priority_t irq_priority = FREE_RUNNING_TIMER_PRIORITY;

    quasar_timer_free_running_ms_init(irq_priority);
}

uint64_t swc_hal_get_tick_free_running_timer(void)
{
    return quasar_timer_free_running_ms_get_tick_count();
}

uint32_t swc_hal_get_free_running_timer_frequency_hz(void)
{
    return QUASAR_FREE_RUNNING_TIMER_FREQ_HZ;
}

/* Dual Radio Timer Management */
void swc_hal_multi_radio_timer_init(void)
{
    quasar_timer_multi_radio_init(QUASAR_DEF_PRIO_MULTI_RADIO_TIMER_IRQ);
    quasar_timer_multi_radio_set_prescaler(MULTI_RADIO_TIMER_PRESCALER);
}

void swc_hal_set_multi_radio_timer_callback(void (*callback)(void))
{
    quasar_timer_multi_radio_set_callback(callback);
}

void swc_hal_timer_multi_radio_timer_start(void)
{
    quasar_timer_multi_radio_start();
}

void swc_hal_timer_multi_radio_timer_stop(void)
{
    quasar_timer_multi_radio_stop();
}

void swc_hal_timer_multi_radio_timer_set_period(uint16_t period)
{
    quasar_timer_multi_radio_set_period(period);
}

void swc_hal_timer_multi_radio_timer_set_max_period(void)
{
    quasar_timer_multi_radio_set_period(MULTI_RADIO_MAX_TIMER_PERIOD);
}

uint32_t swc_hal_get_timer_multi_frequency_hz(void)
{
    return quasar_timer_multi_radio_get_freq_hz();
}
