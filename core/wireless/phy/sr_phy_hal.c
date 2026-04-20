/** @file  sr_phy_hal.c
 *  @brief Hardware abstraction layer that links the MCU hardware drivers required for the radio to operate.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sr_phy_hal.h"
#include "sr_access.h"
#include "sr_utils.h"

/* PUBLIC FUNCTIONS ***********************************************************/
bool sr_phy_hal_radio_gpio_link(uint8_t number_of_radios, sr_phy_hal_gpio_functions_t *sr_phy_hal_gpio_functions)
{
    if (number_of_radios > MAX_NUMBER_OF_RADIOS) {
        return false;
    }

    for (uint8_t i = 0; i < number_of_radios; i++) {
        if (&sr_phy_hal_gpio_functions[i] == NULL) {
            return false;
        }
        if (!sr_access_radio_gpio_link(i, sr_phy_hal_gpio_functions[i].set_reset_pin,
                                       sr_phy_hal_gpio_functions[i].reset_reset_pin)) {
            return false;
        }
    }

    return true;
}

bool sr_phy_hal_radio_comm_link(uint8_t number_of_radios, sr_phy_hal_comm_functions_t *sr_phy_hal_comm_functions)
{
    if (number_of_radios > MAX_NUMBER_OF_RADIOS) {
        return false;
    }
    for (uint8_t i = 0; i < number_of_radios; i++) {
        if (&sr_phy_hal_comm_functions[i] == NULL) {
            return false;
        }
        if (!sr_access_radio_comm_link(i, sr_phy_hal_comm_functions[i].end_transfer,
                                       sr_phy_hal_comm_functions[i].begin_transfer,
                                       sr_phy_hal_comm_functions[i].transfer_half_duplex_rx_blocking,
                                       sr_phy_hal_comm_functions[i].transfer_half_duplex_rx_non_blocking,
                                       sr_phy_hal_comm_functions[i].transfer_half_duplex_tx_blocking,
                                       sr_phy_hal_comm_functions[i].transfer_half_duplex_tx_non_blocking,
                                       sr_phy_hal_comm_functions[i].transfer_full_duplex_blocking,
                                       sr_phy_hal_comm_functions[i].transfer_full_duplex_non_blocking,
                                       sr_phy_hal_comm_functions[i].set_access_mode_spi,
                                       sr_phy_hal_comm_functions[i].set_access_mode_qspi,
                                       sr_phy_hal_comm_functions[i].is_transfer_busy)) {
            return false;
        }
    }

    return true;
}

bool sr_phy_hal_radio_irq_link(uint8_t number_of_radios, sr_phy_hal_irq_functions_t *sr_phy_hal_irq_functions)
{
    if (number_of_radios > MAX_NUMBER_OF_RADIOS) {
        return false;
    }
    for (uint8_t i = 0; i < number_of_radios; i++) {
        if (&sr_phy_hal_irq_functions[i] == NULL) {
            return false;
        }
        if (!sr_access_radio_irq_link(
                i, sr_phy_hal_irq_functions[i].read_irq_pin, sr_phy_hal_irq_functions[i].radio_context_switch,
                sr_phy_hal_irq_functions[i].disable_radio_irq, sr_phy_hal_irq_functions[i].enable_radio_irq,
                sr_phy_hal_irq_functions[i].disable_radio_dma_irq, sr_phy_hal_irq_functions[i].enable_radio_dma_irq)) {
            return false;
        }
    }

    return true;
}

bool sr_phy_hal_free_runing_timer_link(
    sr_phy_hal_free_running_timer_functions_t sr_phy_hal_free_running_timer_functions)
{
    return sr_utils_free_runing_timer_link(sr_phy_hal_free_running_timer_functions.get_free_running_timer_frequency_hz,
                                           sr_phy_hal_free_running_timer_functions.get_tick_free_running_timer);
}
