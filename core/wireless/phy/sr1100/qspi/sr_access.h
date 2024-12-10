/** @file sr_access.h
 *  @brief SR hardware abstraction layer for SR1120 QSPI.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SR_ACCESS_H_
#define SR_ACCESS_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "sr_phy_error.h"
#include "sr_reg.h"
#include "sr_utils.h"

/* ERROR **********************************************************************/
#ifndef ACCESS_ADV_ERR_CHECK_EN
#define ACCESS_ADV_ERR_CHECK_EN 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief SR API Hardware Abstraction Layer for SR1120 QSPI.
 *
 *  This structure contains all function pointers used to interact with the
 *  microcontroller's peripherals.
 */
typedef struct {
    /*! Set reset pin HIGH */
    void (*set_reset_pin)(void);
    /*! Set reset pin LOW */
    void (*reset_reset_pin)(void);
    /*! Return IRQ pin state. 0 (LOW), 1(HIGH) */
    bool (*read_irq_pin)(void);
    /*! Set CS pin HIGH */
    void (*set_cs)(void);
    /*! Set CS pin LOW */
    void (*reset_cs)(void);
    /*! Blocking delay function in milliseconds */
    void (*delay_ms)(uint32_t ms);
    /*! Get the current tick timestamp */
    uint64_t (*get_tick)(void);
    /*! Tick frequency in Hz for the get_tick function */
    uint32_t tick_frequency_hz;
    /*! SPI Transfer full duplex in blocking mode. Spark Radio only support full duplex on
     *  instructions actual read and write are always in half-duplex mode.
     */
    void (*transfer_full_duplex_blocking)(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);
    /*! SPI Transfer full duplex in non-blocking mode using DMA. Spark Radio only support
     *  full duplex on instructions actual read and write are always in half-duplex mode.
     *  FYI : CS Pin need to be externally controlled when using this mode.
     */
    void (*transfer_full_duplex_non_blocking)(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);
    /*! Check if the status of the busy flag in the SPI Status Register */
    bool (*is_spi_busy)(void);
    /*! Trigger the Spark radio IRQ context */
    void (*context_switch)(void);
    /*! Disable radio IRQ interrupt source */
    void (*disable_radio_irq)(void);
    /*! Enable radio IRQ interrupt source */
    void (*enable_radio_irq)(void);
    /*! Disable SPI DMA interrupt source */
    void (*disable_radio_dma_irq)(void);
    /*! Enable SPI DMA interrupt source */
    void (*enable_radio_dma_irq)(void);
    /*! QSPI Write half duplex in blocking mode. */
    void (*write_half_duplex_blocking)(uint8_t address, uint8_t *data, uint8_t size);
    /*! QSPI Read half duplex in blocking mode. */
    void (*read_half_duplex_blocking)(uint8_t address, uint8_t *data, uint8_t size);
    /*! Set the QSPI operating mode */
    void (*set_qspi_mode)(uint8_t mode);
    /*! Check if the status of the busy flag in the QSPI Status Register */
    bool (*is_qspi_busy)(void);
} radio_hal_t;

/* PUBLIC FUNCTIONS ***********************************************************/
/* SPI I/O and Context Switch API */

/** @brief Open the communication with the radio.
 *
 *  @param[in] radio  Radio's instance.
 */
static inline void sr_access_open(radio_hal_t *radio)
{
    radio->reset_cs();
}

/** @brief Close the communication with the radio.
 *
 *  @param[in] radio  Radio's instance.
 */
static inline void sr_access_close(radio_hal_t *radio)
{
    radio->set_cs();
}

/** @brief Check if SPI is busy
 *
 *  @param[in] radio_hal_adv  Full duplex transfer implementation.
 */
static inline bool sr_access_is_spi_busy(radio_hal_t *radio_hal)
{
    return radio_hal->is_qspi_busy();
}

/** @brief Trigger a context switch to the Radio IRQ context.
 *
 *  @param[in] radio_adv  Radio HAL ADV instance.
 */
static inline void sr_access_context_switch(radio_hal_t *radio_hal)
{
    radio_hal->context_switch();
}

/** @brief Enable the Radio DMA interrupt.
 *
 *  @param[in] radio_adv  Radio HAL ADV instance.
 */
static inline void sr_access_enable_dma_irq(radio_hal_t *radio_hal)
{
    radio_hal->enable_radio_dma_irq();
}

/** @brief Disable the Radio DMA interrupt.
 *
 *  @param[in] radio_adv  Radio HAL ADV instance.
 */
static inline void sr_access_disable_dma_irq(radio_hal_t *radio_hal)
{
    radio_hal->disable_radio_dma_irq();
}

/** @brief Enable the Radio external interrupt.
 *
 *  @param[in] radio_adv  Radio HAL ADV instance.
 */
static inline void sr_access_enable_radio_irq(radio_hal_t *radio_hal)
{
    radio_hal->enable_radio_irq();
}

/** @brief Disable the Radio external interrupt.
 *
 *  @param[in] radio_adv  Radio HAL ADV instance.
 */
static inline void sr_access_disable_radio_irq(radio_hal_t *radio_hal)
{
    radio_hal->disable_radio_irq();
}

/* SPI Peripherals half-duplex RX/TX */

/** @brief Initiate an SPI transfer in non blocking mode
 *
 *  @param[in]  radio_adv  Radio HAL ADV instance.
 *  @param[in]  tx_buffer  Pointer to the buffer to send to the radio.
 *  @param[out] rx_buffer  Buffer containing the radio response.
 *  @param[in]  size       Size of the transfer.
 */
static inline void sr_access_spi_transfer_non_blocking(radio_hal_t *radio_hal, uint8_t *tx_buffer, uint8_t *rx_buffer, uint16_t size)
{
    radio_hal->reset_cs();
    radio_hal->transfer_full_duplex_non_blocking(tx_buffer, rx_buffer, size);
}

/** @brief Initiate an SPI transfer in blocking mode
 *
 *  @param[in]  radio_adv  Radio HAL ADV instance.
 *  @param[in]  tx_buffer  Pointer to the buffer to send to the radio.
 *  @param[out] rx_buffer  Buffer containing the radio response.
 *  @param[in]  size       Size of the transfer.
 */
static inline void sr_access_spi_transfer_blocking(radio_hal_t *radio_hal, uint8_t *tx_buffer, uint8_t *rx_buffer, uint16_t size)
{
    radio_hal->reset_cs();
    radio_hal->transfer_full_duplex_blocking(tx_buffer, rx_buffer, size);
    radio_hal->set_cs();
}

/** @brief Write to a register using QSPI.
 *
 *  @param[in] radio  Radio's instance.
 *  @param[in] reg    Register to write.
 *  @param[in] value  Value to write.
 */
static inline void sr_access_write_reg8(radio_hal_t *radio_hal, uint8_t reg, uint8_t value)
{
    uint8_t tx_buffer[2];
    uint8_t rx_buffer[2];

    tx_buffer[0] = REG_WRITE | reg;
    tx_buffer[1] = value;

    radio_hal->reset_cs();
    radio_hal->transfer_full_duplex_blocking(tx_buffer, rx_buffer, 2);
    radio_hal->set_cs();
}

/** @brief Write 16 bits into 2 concecutive registers.
 *
 *  @param[in] radio  Radio's instance.
 *  @param[in] reg    Register to write.
 *  @param[in] value  Value to write.
 */
static inline void sr_access_write_reg16(radio_hal_t *radio_hal, uint8_t reg, uint16_t value)
{
    uint8_t tx_buffer[3];
    uint8_t rx_buffer[3];

    tx_buffer[0] = REG_WRITE | reg;
    memcpy(&tx_buffer[1], (uint8_t *)&value, 2);

    radio_hal->reset_cs();
    radio_hal->transfer_full_duplex_blocking(tx_buffer, rx_buffer, 3);
    radio_hal->set_cs();
}

/** @brief Read from a register.
 *
 *  @param[in] radio  Radio's instance.
 *  @param[in] reg    Register to read.
 */
static inline uint8_t sr_access_read_reg8(radio_hal_t *radio_hal, uint8_t reg)
{
    uint8_t tx_buffer[2];
    uint8_t rx_buffer[2];

    tx_buffer[0] = reg;

    radio_hal->reset_cs();
    radio_hal->transfer_full_duplex_blocking(tx_buffer, rx_buffer, 2);
    radio_hal->set_cs();

    return rx_buffer[1];
}

/** @brief Read from a register.
 *
 *  @param[in] radio  Radio's instance.
 *  @param[in] reg    Register to read.
 */
static inline uint16_t sr_access_read_reg16(radio_hal_t *radio_hal, uint8_t reg)
{
    uint8_t tx_buffer[3];
    uint8_t rx_buffer[3];

    tx_buffer[0] = reg;

    radio_hal->reset_cs();
    radio_hal->transfer_full_duplex_blocking(tx_buffer, rx_buffer, 2);
    radio_hal->set_cs();

    return rx_buffer[1];
}

#ifdef __cplusplus
}
#endif

#endif /* SR_ACCESS_H_ */
