/** @file  quasar_backend.c
 *  @brief Implement swc_hal_facade facade prototype functions.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar.h"
#include "quasar_qspi.h"
#include "swc_hal_facade.h"

/* CONSTANTS ******************************************************************/
#define MULTI_RADIO_MAX_TIMER_PERIOD 0xFFFE
#define MULTI_RADIO_TIMER_PRESCALER  8

/* SINGLE-RADIO PHYSICAL SELECT ***********************************************/
/*
 * On a single-radio build (SWC_RADIO_COUNT == 1) the WPS only drives the
 * "radio 1" slot. SWC_SINGLE_RADIO_ID selects which physical transceiver that
 * slot is wired to: 1 (default) keeps it on radio 1 -> byte-identical to the
 * historical behaviour; 2 re-binds the slot to the physical radio-2 SPI/pins so
 * the same firmware can exercise either transceiver (board bring-up / isolating
 * a radio-2-line x supply interaction). Only meaningful when SWC_RADIO_COUNT == 1;
 * ignored for dual builds, where the slot always stays on radio 1.
 *
 * Radio 2 is SPI-only (no QSPI peripheral), so the remap is a 1:1 swap of the
 * plain-SPI paths. The QSPI/OSPI1 half-duplex and access-mode helpers are left
 * pointing at radio 1: they are compiled out or provably never called when
 * RADIO_QSPI_ENABLED is 0 (see sr_access_set_mode()), and the #error below
 * forbids the one combination (radio 2 + QSPI) that would reach them.
 */
#ifndef SWC_SINGLE_RADIO_ID
#define SWC_SINGLE_RADIO_ID 1
#endif

#if (SWC_RADIO_COUNT == 1) && (SWC_SINGLE_RADIO_ID == 2)
#if RADIO_QSPI_ENABLED
#error "SWC_SINGLE_RADIO_ID=2 requires RADIO_QSPI_ENABLED=0 (radio 2 is SPI-only)."
#endif
#define SLOT1_SPI_SELECTION      QUASAR_DEF_SPI_SELECTION_RADIO_2
#define slot1_context_switch     quasar_radio_2_context_switch
#define slot1_set_irq_callback   quasar_radio_set_radio_2_irq_callback
#define slot1_set_nbt_callback   quasar_radio_set_radio_2_non_blocking_transfer_callback
#define slot1_disable_irq_it     quasar_radio_2_disable_irq_it
#define slot1_enable_irq_it      quasar_radio_2_enable_irq_it
#define slot1_disable_nbt_irq_it quasar_radio_2_disable_non_blocking_transfer_irq_it
#define slot1_enable_nbt_irq_it  quasar_radio_2_enable_non_blocking_transfer_irq_it
#define slot1_read_irq_pin       quasar_radio_2_read_irq_pin
#define slot1_set_reset_pin      quasar_radio_2_set_reset_pin
#define slot1_reset_reset_pin    quasar_radio_2_reset_reset_pin
#define slot1_spi_set_cs         quasar_radio_2_spi_set_cs
#define slot1_spi_reset_cs       quasar_radio_2_spi_reset_cs
#else
#define SLOT1_SPI_SELECTION      QUASAR_DEF_SPI_SELECTION_RADIO_1
#define slot1_context_switch     quasar_radio_1_context_switch
#define slot1_set_irq_callback   quasar_radio_set_radio_1_irq_callback
#define slot1_set_nbt_callback   quasar_radio_set_radio_1_non_blocking_transfer_callback
#define slot1_disable_irq_it     quasar_radio_1_disable_irq_it
#define slot1_enable_irq_it      quasar_radio_1_enable_irq_it
#define slot1_disable_nbt_irq_it quasar_radio_1_disable_non_blocking_transfer_irq_it
#define slot1_enable_nbt_irq_it  quasar_radio_1_enable_non_blocking_transfer_irq_it
#define slot1_read_irq_pin       quasar_radio_1_read_irq_pin
#define slot1_set_reset_pin      quasar_radio_1_set_reset_pin
#define slot1_reset_reset_pin    quasar_radio_1_reset_reset_pin
#define slot1_spi_set_cs         quasar_radio_1_spi_set_cs
#define slot1_spi_reset_cs       quasar_radio_1_spi_reset_cs
#endif

/* PUBLIC FUNCTIONS ***********************************************************/
/* Context Switching and Interrupt Management */
void swc_hal_radio_1_context_switch(void)
{
    slot1_context_switch();
}

#if (SWC_RADIO_COUNT > 1)
void swc_hal_radio_2_context_switch(void)
{
    quasar_radio_2_context_switch();
}
#endif

void swc_hal_set_radio_1_irq_callback(void (*callback)(void))
{
    slot1_set_irq_callback(callback);
}

void swc_hal_set_radio_2_irq_callback(void (*callback)(void))
{
    quasar_radio_set_radio_2_irq_callback(callback);
}

void swc_hal_set_radio_1_non_blocking_transfer_callback(void (*callback)(void))
{
    slot1_set_nbt_callback(callback);
}

void swc_hal_set_radio_2_non_blocking_transfer_callback(void (*callback)(void))
{
    quasar_radio_set_radio_2_non_blocking_transfer_callback(callback);
}

void swc_hal_radio_1_disable_irq_it(void)
{
    slot1_disable_irq_it();
}

#if (SWC_RADIO_COUNT > 1)
void swc_hal_radio_2_disable_irq_it(void)
{
    quasar_radio_2_disable_irq_it();
}
#endif

void swc_hal_radio_1_enable_irq_it(void)
{
    slot1_enable_irq_it();
}

#if (SWC_RADIO_COUNT > 1)
void swc_hal_radio_2_enable_irq_it(void)
{
    quasar_radio_2_enable_irq_it();
}
#endif

void swc_hal_radio_1_disable_non_blocking_transfer_irq_it(void)
{
    slot1_disable_nbt_irq_it();
}

#if (SWC_RADIO_COUNT > 1)
void swc_hal_radio_2_disable_non_blocking_transfer_irq_it(void)
{
    quasar_radio_2_disable_non_blocking_transfer_irq_it();
}
#endif

void swc_hal_radio_1_enable_non_blocking_transfer_irq_it(void)
{
    slot1_enable_nbt_irq_it();
}

#if (SWC_RADIO_COUNT > 1)
void swc_hal_radio_2_enable_non_blocking_transfer_irq_it(void)
{
    quasar_radio_2_enable_non_blocking_transfer_irq_it();
}
#endif

/* GPIO Controls for Radios */
bool swc_hal_radio_1_read_irq_pin(void)
{
    return slot1_read_irq_pin();
}

#if (SWC_RADIO_COUNT > 1)
bool swc_hal_radio_2_read_irq_pin(void)
{
    return quasar_radio_2_read_irq_pin();
}
#endif

void swc_hal_radio_1_set_reset_pin(void)
{
    slot1_set_reset_pin();
}

#if (SWC_RADIO_COUNT > 1)
void swc_hal_radio_2_set_reset_pin(void)
{
    quasar_radio_2_set_reset_pin();
}
#endif

void swc_hal_radio_1_reset_reset_pin(void)
{
    slot1_reset_reset_pin();
}

#if (SWC_RADIO_COUNT > 1)
void swc_hal_radio_2_reset_reset_pin(void)
{
    quasar_radio_2_reset_reset_pin();
}
#endif

/* SPI Communication */
void swc_hal_radio_1_end_transfer(void)
{
    /* The CS pin is the same for SPI and QSPI. */
    slot1_spi_set_cs();
}

#if (SWC_RADIO_COUNT > 1)
void swc_hal_radio_2_end_transfer(void)
{
    /* The CS pin is the same for SPI and QSPI. */
    quasar_radio_2_spi_set_cs();
}
#endif

void swc_hal_radio_1_begin_transfer(void)
{
    /* The CS pin is the same for SPI and QSPI. */
    slot1_spi_reset_cs();
}

#if (SWC_RADIO_COUNT > 1)
void swc_hal_radio_2_begin_transfer(void)
{
    /* The CS pin is the same for SPI and QSPI. */
    quasar_radio_2_spi_reset_cs();
}
#endif

void swc_hal_radio_1_transfer_half_duplex_rx_blocking(uint8_t command, uint8_t *rx_data, uint16_t size)
{
#if !RADIO_QSPI_ENABLED
    (void)command;
    (void)rx_data;
    (void)size;

    /* SPI sends full duplex by default. */
    while (1);
#else
    /* Radio 1 uses OSPI1. */
    quasar_qspi_transfer_half_duplex_rx_blocking(QUASAR_DEF_QSPI_SELECTION_RADIO_1, command, rx_data, size, 1);
#endif
}

void swc_hal_radio_1_transfer_half_duplex_tx_blocking(uint8_t command, uint8_t *tx_data, uint16_t size)
{
#if !RADIO_QSPI_ENABLED
    (void)command;
    (void)tx_data;
    (void)size;

    /* SPI sends full duplex by default. */
    while (1);
#else
    /* Radio 1 uses OSPI1. */
    quasar_qspi_transfer_half_duplex_tx_blocking(QUASAR_DEF_QSPI_SELECTION_RADIO_1, command, tx_data, size, 1);
#endif
}

void swc_hal_radio_1_transfer_half_duplex_rx_non_blocking(uint8_t command, uint8_t *rx_data, uint16_t size)
{
#if !RADIO_QSPI_ENABLED
    (void)command;
    (void)rx_data;
    (void)size;

    /* SPI sends full duplex by default. */
    while (1);
#else
    /* Radio 1 uses OSPI1. */
    quasar_qspi_transfer_half_duplex_rx_non_blocking(QUASAR_DEF_QSPI_SELECTION_RADIO_1, command, rx_data, size);
#endif
}

void swc_hal_radio_1_transfer_half_duplex_tx_non_blocking(uint8_t command, uint8_t *tx_data, uint16_t size)
{
#if !RADIO_QSPI_ENABLED
    (void)command;
    (void)tx_data;
    (void)size;

    /* SPI sends full duplex by default. */
    while (1);
#else
    /* Radio 1 uses OSPI1. */
    quasar_qspi_transfer_half_duplex_tx_non_blocking(QUASAR_DEF_QSPI_SELECTION_RADIO_1, command, tx_data, size);
#endif
}

void swc_hal_radio_1_transfer_full_duplex_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
#if !RADIO_QSPI_ENABLED
    quasar_spi_transfer_full_duplex_blocking(SLOT1_SPI_SELECTION, tx_data, rx_data, size);
#else
    (void)tx_data;
    (void)rx_data;
    (void)size;

    /* QSPI cannot operate in bidirectional mode. */
    while (1);
#endif
}

void swc_hal_radio_2_transfer_full_duplex_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
#if !RADIO_QSPI_ENABLED
    quasar_spi_transfer_full_duplex_blocking(QUASAR_DEF_SPI_SELECTION_RADIO_2, tx_data, rx_data, size);
#else
    (void)tx_data;
    (void)rx_data;
    (void)size;

    /* QSPI cannot operate in bidirectional mode. */
    while (1);
#endif
}

void swc_hal_radio_1_transfer_full_duplex_non_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
#if !RADIO_QSPI_ENABLED
    quasar_spi_transfer_full_duplex_non_blocking(SLOT1_SPI_SELECTION, tx_data, rx_data, size);
#else
    (void)tx_data;
    (void)rx_data;
    (void)size;

    /* QSPI cannot operate in bidirectional mode. */
    while (1);
#endif
}

void swc_hal_radio_2_transfer_full_duplex_non_blocking(uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
#if !RADIO_QSPI_ENABLED
    quasar_spi_transfer_full_duplex_non_blocking(QUASAR_DEF_SPI_SELECTION_RADIO_2, tx_data, rx_data, size);
#else
    (void)tx_data;
    (void)rx_data;
    (void)size;

    /* QSPI cannot operate in bidirectional mode. */
    while (1);
#endif
}

void swc_hal_radio_1_set_access_mode_spi(void)
{
    quasar_qspi_set_mode(QUASAR_DEF_QSPI_SELECTION_RADIO_1, QUASAR_QSPI_1_LINE_MODE_0_DUMMY);
}

void swc_hal_radio_1_set_access_mode_qspi(void)
{
    quasar_qspi_set_mode(QUASAR_DEF_QSPI_SELECTION_RADIO_1, QUASAR_QSPI_4_LINES_MODE_1_DUMMY);
}

bool swc_hal_radio_1_is_transfer_busy(void)
{
#if !RADIO_QSPI_ENABLED
    return QUASAR_SPI_IS_BUSY(SLOT1_SPI_SELECTION);
#else
    return QUASAR_QSPI_IS_BUSY(QUASAR_DEF_QSPI_SELECTION_RADIO_1);
#endif
}

bool swc_hal_radio_2_is_transfer_busy(void)
{
#if !RADIO_QSPI_ENABLED
    return QUASAR_SPI_IS_BUSY(QUASAR_DEF_SPI_SELECTION_RADIO_2);
#else
    return QUASAR_QSPI_IS_BUSY(QUASAR_DEF_QSPI_SELECTION_RADIO_2);
#endif
}

/* Timer and Delay Management */
uint64_t swc_hal_get_tick_free_running_timer(void)
{
    return quasar_timer_free_running_ms_get_tick_count();
}

uint32_t swc_hal_get_free_running_timer_frequency_hz(void)
{
    return quasar_timer_free_running_ms_get_tick_frequency();
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
