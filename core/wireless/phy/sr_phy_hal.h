/** @file  sr_phy_hal.h
 *  @brief Hardware abstraction layer that links the MCU hardware drivers required for the radio to operate.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SR_PHY_HAL_H_
#define SR_PHY_HAL_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Structure of GPIO function pointers used by the sr_phy_hal to link MCU hardware drivers with sr_access.h.
 */
typedef struct sr_phy_hal_gpio_functions {
    /*! Pointer to a function that sets the reset pin of radio. */
    void (*set_reset_pin)(void);
    /*! Pointer to a function that resets the reset pin of radio. */
    void (*reset_reset_pin)(void);
} sr_phy_hal_gpio_functions_t;

/** @brief Structure of radio communication function pointers used by the sr_phy_hal to link MCU hardware drivers with.
 *  sr_access.h.
 */
typedef struct sr_phy_hal_comm_functions {
    /*! Pointer to a function that sets the on-board controller SPI chip-select pin of the radio. */
    void (*end_transfer)(void);
    /*! Pointer to a function that reset the on-board controller SPI chip-select pin of the radio. */
    void (*begin_transfer)(void);
    /*! Transfer half duplex RX in blocking mode. */
    void (*transfer_half_duplex_rx_blocking)(uint8_t command, uint8_t *rx_data, uint16_t size);
    /*! Transfer half duplex RX in non-blocking mode. */
    void (*transfer_half_duplex_rx_non_blocking)(uint8_t command, uint8_t *rx_data, uint16_t size);
    /*! Transfer half duplex TX in blocking mode. */
    void (*transfer_half_duplex_tx_blocking)(uint8_t command, uint8_t *tx_data, uint16_t size);
    /*! Transfer half duplex TX in non-blocking mode. */
    void (*transfer_half_duplex_tx_non_blocking)(uint8_t command, uint8_t *tx_data, uint16_t size);
    /*! Transfer full duplex in blocking mode. */
    void (*transfer_full_duplex_blocking)(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);
    /*! Transfer full duplex in non-blocking mode using DMA. */
    void (*transfer_full_duplex_non_blocking)(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);
    /*! Set the access mode to be SPI. */
    void (*set_access_mode_spi)(void);
    /*! Set the access mode to be QSPI. */
    void (*set_access_mode_qspi)(void);
    /*! Pointer to a function that reads the status of the radio's SPI. interface.*/
    bool (*is_transfer_busy)(void);
} sr_phy_hal_comm_functions_t;

/** @brief Structure of IRQ function pointers used by the sr_phy_hal to link MCU hardware drivers with sr_access.h.
 */
typedef struct sr_phy_hal_irq_functions {
    /*! Pointer to a function that reads the status of radio IRQ pin. */
    bool (*read_irq_pin)(void);
    /*! Pointer to a function that manually triggers the radio IRQ pin interrupt. */
    void (*radio_context_switch)(void);
    /*! Pointer to a function that disables the IRQ external interrupt for radio. */
    void (*disable_radio_irq)(void);
    /*! Pointer to a function that enables the IRQ external interrupt for radio. */
    void (*enable_radio_irq)(void);
    /*! Pointer to a function that disables the DMA SPI interrupt for radio. */
    void (*disable_radio_dma_irq)(void);
    /*! Pointer to a function that enables the DMA SPI interrupt for radio. */
    void (*enable_radio_dma_irq)(void);
} sr_phy_hal_irq_functions_t;

/** @brief Structure of the free running timer function pointers used by the sr_phy_hal to configure sr_utils.h.
 *         functions.
 */
typedef struct sr_phy_hal_free_running_timer_functions {
    /*! Pointer to a function that returns the configured tick frequency of the free running timer. */
    uint32_t (*get_free_running_timer_frequency_hz)(void);
    /*! Pointer to a function that get the free running timer tick count. */
    uint64_t (*get_tick_free_running_timer)(void);
} sr_phy_hal_free_running_timer_functions_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Links GPIO control functions to the radio_hal instance in the sr_access.h file.
 *
 *  @param[in] number_of_radios           Number of radios.
 *  @param[in] sr_phy_hal_gpio_functions  Structure of GPIO function pointers used by the sr_phy_hal to configure.
 *                                        sr_access.h functions.
 *  @retval true  Link success.
 *  @retval false  Link fail.
 */
bool sr_phy_hal_radio_gpio_link(uint8_t number_of_radios, sr_phy_hal_gpio_functions_t *sr_phy_hal_gpio_functions);

/** @brief Links radio communication control functions to the radio_hal instance in the sr_access.h file.
 *
 *  @param[in] number_of_radios          Number of radios.
 *  @param[in] sr_phy_hal_comm_function  Structure of SPI function pointers used by the sr_phy_hal to configure.
 *                                       sr_access.h functions.
 *  @retval true  Link success.
 *  @retval false  Link fail.
 */
bool sr_phy_hal_radio_comm_link(uint8_t number_of_radios, sr_phy_hal_comm_functions_t *sr_phy_hal_comm_functions);

/** @brief Links irq functions to the radio_hal instance in the sr_access.h file.
 *
 *  @param[in] number_of_radios         Number of radios.
 *  @param[in] sr_phy_hal_irq_function  Structure of IRQ function pointers used by the sr_phy_hal to configure.
 *                                      sr_access.h functions.
 *  @retval true  Link success.
 *  @retval false  Link fail.
 */
bool sr_phy_hal_radio_irq_link(uint8_t number_of_radios, sr_phy_hal_irq_functions_t *sr_phy_hal_irq_functions);

/** @brief Links free running timer functions to the utils hal instance in the sr_utils.h file.
 *
 *  @param[in] sr_phy_hal_free_running_timer_functions  Structure of the free running timer function pointers used by.
 *                                                      the sr_phy_hal to configure sr_utils.h functions.
 *  @retval true  Link success.
 *  @retval false  Link fail.
 */
bool sr_phy_hal_free_runing_timer_link(
    sr_phy_hal_free_running_timer_functions_t sr_phy_hal_free_running_timer_functions);

#ifdef __cplusplus
}
#endif

#endif /* MODULE_NAME_H_ */
