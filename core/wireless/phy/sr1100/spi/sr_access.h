/** @file sr_access.h
 *  @brief SR1100 Protocol external access layer file.
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
#include "swc_hal_facade.h"

/* ERROR **********************************************************************/
#ifndef ACCESS_ADV_ERR_CHECK_EN
#define ACCESS_ADV_ERR_CHECK_EN 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Hardware Abstraction Layer for Spark Radio.
 *
 *  Provides an interface for controlling SPI communication, managing chip select (CS) pins,
 *  handling data transfer in both blocking and non-blocking modes, and managing IRQ and DMA
 *  interrupt sources for a Spark Radio device. This abstraction facilitates flexible integration
 *  with different hardware platforms and enhances portability by decoupling the radio operation
 *  specifics from the main application logic.
 *
 *  Functions:
 *  - set_cs: Set the CS pin high.
 *  - reset_cs: Set the CS pin low.
 *  - transfer_full_duplex_blocking: Perform SPI transfer in full duplex blocking mode.
 *  - transfer_full_duplex_non_blocking: Perform SPI transfer in full duplex non-blocking mode using DMA.
 *  - radio_context_switch: Trigger the radio's IRQ pin interrupt context.
 *  - disable_radio_irq: Disable the radio IRQ interrupt source.
 *  - enable_radio_irq: Enable the radio IRQ interrupt source.
 *  - disable_radio_dma_irq: Disable the SPI DMA interrupt source.
 *  - enable_radio_dma_irq: Enable the SPI DMA interrupt source.
 *
 *  This structure should be initialized statically, pointing to the appropriate subset of facade
 *  functions that implement the specified operations, allowing for tailored behavior based on
 *  the specific radio and platform in use.
 */
typedef struct {
    /*! Set reset pin HIGH */
    void (*set_reset_pin)(void);
    /*! Set reset pin LOW */
    void (*reset_reset_pin)(void);
    /*! Set CS pin HIGH */
    void (*set_cs)(void);
    /*! Set CS pin LOW */
    void (*reset_cs)(void);
    /*! SPI Transfer full duplex in blocking mode */
    void (*transfer_full_duplex_blocking)(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);
    /*! SPI Transfer full duplex in non-blocking mode using DMA */
    void (*transfer_full_duplex_non_blocking)(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);
    /*! Check if the status of the busy flag in the SPI Status Register */
    bool (*is_spi_busy)(void);
    /*! Return IRQ pin state. 0 (LOW), 1(HIGH) */
    bool (*read_irq_pin)(void);
    /*! Trigger the radio IRQ context */
    void (*radio_context_switch)(void);
    /*! Disable radio IRQ interrupt source */
    void (*disable_radio_irq)(void);
    /*! Enable radio IRQ interrupt source */
    void (*enable_radio_irq)(void);
    /*! Disable SPI DMA interrupt source */
    void (*disable_radio_dma_irq)(void);
    /*! Enable SPI DMA interrupt source */
    void (*enable_radio_dma_irq)(void);
} radio_hal_t;

/* PRIVATE GLOBALS *************************************************************/
static radio_hal_t radio_hal[2] = {
    {
        .set_reset_pin = swc_hal_radio_1_set_reset_pin,
        .reset_reset_pin = swc_hal_radio_1_reset_reset_pin,
        .set_cs = swc_hal_radio_1_spi_set_cs,
        .reset_cs = swc_hal_radio_1_spi_reset_cs,
        .transfer_full_duplex_blocking = swc_hal_radio_1_spi_transfer_full_duplex_blocking,
        .transfer_full_duplex_non_blocking = swc_hal_radio_1_spi_transfer_full_duplex_non_blocking,
        .is_spi_busy = swc_hal_radio_1_is_spi_busy,
        .read_irq_pin = swc_hal_radio_1_read_irq_pin,
        .radio_context_switch = swc_hal_radio_1_context_switch,
        .disable_radio_irq = swc_hal_radio_1_disable_irq_it,
        .enable_radio_irq = swc_hal_radio_1_enable_irq_it,
        .disable_radio_dma_irq = swc_hal_radio_1_disable_dma_irq_it,
        .enable_radio_dma_irq = swc_hal_radio_1_enable_dma_irq_it,
    },
    {
        .set_reset_pin = swc_hal_radio_2_set_reset_pin,
        .reset_reset_pin = swc_hal_radio_2_reset_reset_pin,
        .set_cs = swc_hal_radio_2_spi_set_cs,
        .reset_cs = swc_hal_radio_2_spi_reset_cs,
        .transfer_full_duplex_blocking = swc_hal_radio_2_spi_transfer_full_duplex_blocking,
        .transfer_full_duplex_non_blocking = swc_hal_radio_2_spi_transfer_full_duplex_non_blocking,
        .is_spi_busy = swc_hal_radio_2_is_spi_busy,
        .read_irq_pin = swc_hal_radio_2_read_irq_pin,
        .radio_context_switch = swc_hal_radio_2_context_switch,
        .disable_radio_irq = swc_hal_radio_2_disable_irq_it,
        .enable_radio_irq = swc_hal_radio_2_enable_irq_it,
        .disable_radio_dma_irq = swc_hal_radio_2_disable_dma_irq_it,
        .enable_radio_dma_irq = swc_hal_radio_2_enable_dma_irq_it,
    }};

/* PUBLIC FUNCTION PROTOTYPES *************************************************/

/** @brief Trigger a context switch to the Radio IRQ context.
 *
 *  @param[in] radio_id  Radio HAL structure array index
 */
static inline void sr_access_radio_context_switch(uint8_t radio_id)
{
    radio_hal[radio_id].radio_context_switch();
}

/** @brief Enable the Radio DMA interrupt.
 *
 *  @param[in] radio_id  Radio HAL structure array index
 */
static inline void sr_access_enable_dma_irq(uint8_t radio_id)
{
    radio_hal[radio_id].enable_radio_dma_irq();
}

/** @brief Disable the Radio DMA interrupt.
 *
 *  @param[in] radio_id  Radio HAL structure array index
 */
static inline void sr_access_disable_dma_irq(uint8_t radio_id)
{
    radio_hal[radio_id].disable_radio_dma_irq();
}

/** @brief Enable the Radio external interrupt.
 *
 *  @param[in] radio_id  Radio HAL structure array index
 */
static inline void sr_access_enable_radio_irq(uint8_t radio_id)
{
    radio_hal[radio_id].enable_radio_irq();
}

/** @brief Disable the Radio external interrupt.
 *
 *  @param[in] radio_id  Radio HAL structure array index
 */
static inline void sr_access_disable_radio_irq(uint8_t radio_id)
{
    radio_hal[radio_id].disable_radio_irq();
}

/** @brief Initiate an SPI transfer in non blocking mode
 *
 *  @param[in]  radio_id   Radio HAL structure array index
 *  @param[in]  tx_buffer  Pointer to the buffer to send to the radio.
 *  @param[out] rx_buffer  Buffer containing the radio response.
 *  @param[in]  size       Size of the transfer.
 */
static inline void sr_access_spi_transfer_non_blocking(uint8_t radio_id, uint8_t *tx_buffer, uint8_t *rx_buffer,
                                                       uint16_t size)
{
    radio_hal[radio_id].reset_cs();
    radio_hal[radio_id].transfer_full_duplex_non_blocking(tx_buffer, rx_buffer, size);
}

/** @brief Initiate an SPI transfer in blocking mode
 *
 *  @param[in]  radio_id   Radio HAL structure array index
 *  @param[in]  tx_buffer  Pointer to the buffer to send to the radio.
 *  @param[out] rx_buffer  Buffer containing the radio response.
 *  @param[in]  size       Size of the transfer.
 */
static inline void sr_access_spi_transfer_blocking(uint8_t radio_id, uint8_t *tx_buffer, uint8_t *rx_buffer,
                                                   uint16_t size)
{
    radio_hal[radio_id].reset_cs();
    radio_hal[radio_id].transfer_full_duplex_blocking(tx_buffer, rx_buffer, size);
    radio_hal[radio_id].set_cs();
}

/** @brief Open the communication with the radio.
 *
 *  @param[in] radio_id  Radio HAL structure array index
 */
static inline void sr_access_open(uint8_t radio_id)
{
    radio_hal[radio_id].reset_cs();
}

/** @brief Close the communication with the radio.
 *
 *  @param[in] radio_id  Radio HAL structure array index
 */
static inline void sr_access_close(uint8_t radio_id)
{
    radio_hal[radio_id].set_cs();
}

/** @brief Return the status of the radio's SPI
 *
 *  @param[in] radio_id  Radio HAL structure array index
 *
 *  @retval true   SPI is busy.
 *  @retval false  SPI is not busy.
 */
static inline bool sr_access_is_spi_busy(uint8_t radio_id)
{
    return radio_hal[radio_id].is_spi_busy();
}

/** @brief Return the status of the radio's IRQ pin
 *
 *  @param[in] radio_id  Radio HAL structure array index
 *
 *  @retval true   High
 *  @retval false  Low
 */
static inline bool sr_access_read_irq_pin(uint8_t radio_id)
{
    return radio_hal[radio_id].read_irq_pin();
}

/** @brief Sets the reset pin of radio.
 *
 *  @param[in] radio_id  Radio HAL structure array index
 */
static inline void sr_access_set_reset_pin(uint8_t radio_id)
{
    return radio_hal[radio_id].set_reset_pin();
}

/** @brief Resets the reset pin of radio.
 *
 *  @param[in] radio_id  Radio HAL structure array index
 */
static inline void sr_access_reset_reset_pin(uint8_t radio_id)
{
    return radio_hal[radio_id].reset_reset_pin();
}

/** @brief Write to a register.
 *
 *  @param[in] radio_id  Radio HAL structure array index
 *  @param[in] reg       Register to write.
 *  @param[in] value     Value to write.
 */
static inline void sr_access_write_reg8(uint8_t radio_id, uint8_t reg, uint8_t value)
{
    uint8_t tx_buffer[2];
    uint8_t rx_buffer[2];

    tx_buffer[0] = REG_WRITE | reg;
    tx_buffer[1] = value;

    radio_hal[radio_id].reset_cs();
    radio_hal[radio_id].transfer_full_duplex_blocking(tx_buffer, rx_buffer, 2);
    radio_hal[radio_id].set_cs();
}

/** @brief Write 16 bits into 2 concecutive registers.
 *
 *  @param[in] radio_id  Radio HAL structure array index
 *  @param[in] reg       Register to write.
 *  @param[in] value     Value to write.
 */
static inline void sr_access_write_reg16(uint8_t radio_id, uint8_t reg, uint16_t value)
{
    uint8_t tx_buffer[3];
    uint8_t rx_buffer[3];

    tx_buffer[0] = REG_WRITE | reg;
    memcpy(&tx_buffer[1], (uint8_t *)&value, 2);

    radio_hal[radio_id].reset_cs();
    radio_hal[radio_id].transfer_full_duplex_blocking(tx_buffer, rx_buffer, 3);
    radio_hal[radio_id].set_cs();
}

/** @brief Read from a register.
 *
 *  @param[in] radio_id  Radio HAL structure array index
 *  @param[in] reg       Register to read.
 */
static inline uint8_t sr_access_read_reg8(uint8_t radio_id, uint8_t reg)
{
    uint8_t tx_buffer[2];
    uint8_t rx_buffer[2];

    tx_buffer[0] = reg;

    radio_hal[radio_id].reset_cs();
    radio_hal[radio_id].transfer_full_duplex_blocking(tx_buffer, rx_buffer, 2);
    radio_hal[radio_id].set_cs();

    return rx_buffer[1];
}

/** @brief Read from a register.
 *
 *  @param[in] radio_id  Radio HAL structure array index
 *  @param[in] reg       Register to read.
 */
static inline uint16_t sr_access_read_reg16(uint8_t radio_id, uint8_t reg)
{
    uint8_t tx_buffer[3];
    uint8_t rx_buffer[3];

    tx_buffer[0] = reg;

    radio_hal[radio_id].reset_cs();
    radio_hal[radio_id].transfer_full_duplex_blocking(tx_buffer, rx_buffer, 3);
    radio_hal[radio_id].set_cs();

    return rx_buffer[1] | (rx_buffer[2] << 8);
}

#ifdef __cplusplus
}
#endif

#endif /* SR_ACCESS_H_ */
