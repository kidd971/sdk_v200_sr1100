/** @file  swc_hal_facade.c
 *
 *  @brief Default implementations of weak facade function prototypes.
 *
 *  @note This file provides default weak implementations for certain facade
 *  functions defined in the swc_hal_facade.h header. If the user does not provide
 *  their own implementations for these functions in their backend code, the
 *  weakly linked functions in this file will be used as defaults.
 *
 *  This ensures that the SPARK Wireless Core library remains functional even if
 *  the target platform does not need or support these additional features, particularly
 *  those related to dual radio operations.
 *
 *  The weak attribute allows users to override these functions with their own
 *  implementations specific to their hardware and application needs.
 *
 *  @note Users implementing support for a dual-radio system will need to provide
 *  concrete implementations for these functions to ensure proper functionality.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "swc_hal_facade.h"

/* PUBLIC FUNCTIONS ***********************************************************/
__attribute__((weak)) void swc_hal_radio_2_context_switch(void)
{
    return;
}

__attribute__((weak)) void swc_hal_set_radio_2_irq_callback(void (*callback)(void))
{
    (void)callback;
    return;
}

__attribute__((weak)) void swc_hal_set_radio_2_dma_rx_callback(void (*callback)(void))
{
    (void)callback;
    return;
}

__attribute__((weak)) void swc_hal_radio_2_disable_irq_it(void)
{
    return;
}

__attribute__((weak)) void swc_hal_radio_2_enable_irq_it(void)
{
    return;
}

__attribute__((weak)) void swc_hal_radio_2_disable_dma_irq_it(void)
{
    return;
}

__attribute__((weak)) void swc_hal_radio_2_enable_dma_irq_it(void)
{
    return;
}

__attribute__((weak)) bool swc_hal_radio_2_read_irq_pin(void)
{
    return false;
}

__attribute__((weak)) void swc_hal_radio_2_set_reset_pin(void)
{
    return;
}

__attribute__((weak)) void swc_hal_radio_2_reset_reset_pin(void)
{
    return;
}

__attribute__((weak)) void swc_hal_radio_2_spi_set_cs(void)
{
    return;
}

__attribute__((weak)) void swc_hal_radio_2_spi_reset_cs(void)
{
    return;
}

__attribute__((weak)) void swc_hal_radio_2_spi_transfer_full_duplex_blocking(uint8_t *tx_data, uint8_t *rx_data,
                                                                             uint16_t size)
{
    (void)tx_data;
    (void)rx_data;
    (void)size;
    return;
}

__attribute__((weak)) void swc_hal_radio_2_spi_transfer_full_duplex_non_blocking(uint8_t *tx_data, uint8_t *rx_data,
                                                                                 uint16_t size)
{
    (void)tx_data;
    (void)rx_data;
    (void)size;
    return;
}

__attribute__((weak)) bool swc_hal_radio_2_is_spi_busy(void)
{
    return false;
}

/* Dual Radio Timer Management */
__attribute__((weak)) void swc_hal_multi_radio_timer_init(void)
{
    return;
}

__attribute__((weak)) void swc_hal_set_multi_radio_timer_callback(void (*callback)(void))
{
    (void)callback;
    return;
}

__attribute__((weak)) void swc_hal_timer_multi_radio_timer_start(void)
{
    return;
}

__attribute__((weak)) void swc_hal_timer_multi_radio_timer_stop(void)
{
    return;
}

__attribute__((weak)) void swc_hal_timer_multi_radio_timer_set_period(uint16_t period)
{
    (void)period;
    return;
}

__attribute__((weak)) void swc_hal_timer_multi_radio_timer_set_max_period(void)
{
    return;
}

__attribute__((weak)) uint32_t swc_hal_get_timer_multi_frequency_hz(void)
{
    return 0;
}
