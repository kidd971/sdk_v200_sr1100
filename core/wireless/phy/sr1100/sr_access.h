/** @file sr_access.h
 *  @brief SR1100 Protocol external access layer file.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
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
#include "sr1100_def.h"
#include "sr_bit_macros.h"
#include "sr_phy_error.h"
#include "sr_utils.h"
#include "swc_hal_facade.h"

/* ERROR **********************************************************************/
#ifndef ACCESS_ADV_ERR_CHECK_EN
#define ACCESS_ADV_ERR_CHECK_EN 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/*! Specify the maximum number of radios that can be used. */
#define MAX_NUMBER_OF_RADIOS 2
/*! Max header size. */
#define MAX_HEADER_SIZE 10
/*! SPI first byte command size. */
#define SPI_EMPTY_CMD_BYTE_SIZE 1

/* MACROS *********************************************************************/
/* Use struct member to do an unaligned write on uint8_t. This macros tells the compiler to compile for a safe
 * unaligned write of a uint8_t.
 */
#define SR_UINT8_WRITE(addr, val) ((((struct unaligned_uint8_t *)(void *)(addr))->v) = (val))
/* Use struct member to do an unaligned read on uint8_t. This macros tells the compiler to compile for a safe
 * unaligned write of a uint8_t.
 */
#define SR_UINT8_READ(addr) (((const struct unaligned_uint8_t *)(const void *)(addr))->v)
/* Use struct member to do an unaligned or equal on uint8_t. This macros tells the compiler to compile for a safe
 * unaligned write of a uint8_t.
 */
#define SR_UINT8_OR_WRITE(addr, val) ((((struct unaligned_uint8_t *)(void *)(addr))->v) |= (val))
/* Use struct member to do an unaligned write on uint16_t. This macros tells the compiler to compile for a safe
 * unaligned write of a uint16_t.
 */
#define SR_UINT16_WRITE(addr, val) ((((struct unaligned_uint16_t *)(void *)(addr))->v) = (val))
/* Use struct member to do an unaligned read on uint16_t. This macros tells the compiler to compile for a safe unaligned
 * read of a uint16_t.
 */
#define SR_UINT16_READ(addr) (((const struct unaligned_uint16_t *)(const void *)(addr))->v)
/* Use struct member to do an unaligned or equal on uint16_t. This macros tells the compiler to compile for a safe
 * unaligned or write of a uint16_t.
 */
#define SR_UINT16_OR_WRITE(addr, val) ((((struct unaligned_uint16_t *)(void *)(addr))->v) |= (val))

/* TYPES **********************************************************************/
/** @brief Structure to allow unaligned uint8_t to be written/read.
 */
struct __attribute__((packed)) unaligned_uint8_t {
    /*! Data holder. */
    uint8_t v;
};

/** @brief Structure to allow unaligned uint16_t to be written/read.
 */
struct __attribute__((packed)) unaligned_uint16_t {
    /*! Data holder. */
    uint16_t v;
};

/** @brief Types of access mode to communicate with the radio.
 */
typedef enum access_mode {
    /*! SPI access mode. */
    SPI = 0,
    /*! QSPI access mode. */
    QSPI,
} access_mode_t;

/** @brief Hardware Abstraction Layer for Spark Radio.
 *
 *  Provides an interface for controlling SPI communication, managing chip select (CS) pins,
 *  handling data transfer in both blocking and non-blocking modes, and managing IRQ and DMA
 *  interrupt sources for a Spark Radio device. This abstraction facilitates flexible integration
 *  with different hardware platforms and enhances portability by decoupling the radio operation
 *  specifics from the main application logic.
 *
 *  Functions:
 *  - end_transfer: Set the CS pin high.
 *  - begin_transfer: Set the CS pin low.
 *  - transfer_half_duplex_rx_blocking: Transfer half duplex from the radio in blocking mode.
 *  - transfer_half_duplex_rx_non_blocking: Transfer half duplex from the radio in non-blocking mode.
 *  - transfer_half_duplex_tx_blocking: Transfer half duplex to the radio in blocking mode.
 *  - transfer_half_duplex_tx_non_blocking: Transfer half duplex to the radio in non-blocking mode.
 *  - transfer_full_duplex_blocking: Perform transfer in full duplex blocking mode.
 *  - transfer_full_duplex_non_blocking: Perform transfer in full duplex non-blocking mode.
 *  - radio_context_switch: Trigger the radio's IRQ pin interrupt context.
 *  - disable_radio_irq: Disable the radio IRQ interrupt source.
 *  - enable_radio_irq: Enable the radio IRQ interrupt source.
 *  - disable_radio_non_blocking_transfer_irq: Disable the DMA or peripheral interrupt source.
 *  - enable_radio_non_blocking_transfer_irq: Enable the DMA or peripheral interrupt source.
 *
 *  This structure should be initialized statically, pointing to the appropriate subset of facade
 *  functions that implement the specified operations, allowing for tailored behavior based on
 *  the specific radio and platform in use.
 */
typedef struct radio_hal {
    /*! Set reset pin HIGH. */
    void (*set_reset_pin)(void);
    /*! Set reset pin LOW. */
    void (*reset_reset_pin)(void);
    /*! Manually end the transfer to the radio. */
    void (*end_transfer)(void);
    /*! Manually begin the transfer to the radio. */
    void (*begin_transfer)(void);
    /*! Transfer half duplex RX in blocking mode. */
    void (*transfer_half_duplex_rx_blocking)(uint8_t command, uint8_t *rx_data, uint16_t size);
    /*! Transfer half duplex RX in non-blocking mode. */
    void (*transfer_half_duplex_rx_non_blocking)(uint8_t command, uint8_t *rx_data, uint16_t size);
    /*! Transfer half duplex TX in blocking mode. */
    void (*transfer_half_duplex_tx_blocking)(uint8_t command, uint8_t *tx_data, uint16_t size);
    /*! Transfer half duplex RX in non-blocking mode. */
    void (*transfer_half_duplex_tx_non_blocking)(uint8_t command, uint8_t *tx_data, uint16_t size);
    /*! Transfer full duplex in blocking mode. */
    void (*transfer_full_duplex_blocking)(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);
    /*! Transfer full duplex in non-blocking mode. */
    void (*transfer_full_duplex_non_blocking)(uint8_t *tx_data, uint8_t *rx_data, uint16_t size);
    /*! Set the access mode to be SPI. */
    void (*set_access_mode_spi)(void);
    /*! Set the access mode to be QSPI. */
    void (*set_access_mode_qspi)(void);
    /*! Check if the status of the busy flag in the SPI Status Register. */
    bool (*is_transfer_busy)(void);
    /*! Return IRQ pin state. 0 (LOW), 1(HIGH). */
    bool (*read_irq_pin)(void);
    /*! Trigger the radio IRQ context. */
    void (*radio_context_switch)(void);
    /*! Disable radio IRQ interrupt source. */
    void (*disable_radio_irq)(void);
    /*! Enable radio IRQ interrupt source. */
    void (*enable_radio_irq)(void);
    /*! Disable DMA or peripheral interrupt source. */
    void (*disable_radio_non_blocking_transfer_irq)(void);
    /*! Enable DMA or peripheral interrupt source. */
    void (*enable_radio_non_blocking_transfer_irq)(void);
} radio_hal_t;

/** @brief Shadow SR11xx registers to modify the data bytes in the transfer structures.
 */
typedef struct sr_registers {
    /*! 0x01. */
    void *harddisables_ioconfig;
    /*! 0x02. */
    void *v_i_time_refs;
    /*! 0x03. */
    void *pllwait_pllres;
    /*! 0x04. */
    void *cca_settings;
    /*! 0x05. */
    void *cca_thres_gain;
    /*! 0x06. */
    void *rf_gain_manugain;
    /*! 0x07. */
    void *if_baseband_gain_lna;
    /*! 0x08. */
    void *rxbandfre_cfg1freq;
    /*! 0x09. */
    void *cfg2freq_cfg3freq;
    /*! 0x0A. */
    void *cfg_widths_txpwr_randpulse;
    /*! 0x0B. */
    void *tx_pulse_pos;
    /*! 0x0C. */
    void *slpperiod_15_0;
    /*! 0x0D. */
    void *slpperiod_pwrupdlay;
    /*! 0x0E. */
    void *timelimit_biasdelay;
    /*! 0x0F. */
    void *timercfg_sleepcfg_write;
#if WPS_RADIO_COUNT == 2
    /*! 0x0F. */
    void *timercfg_sleepcfg_write_read_events;
#endif /* WPS_RADIO_COUNT == 2 */
    /*! 0x10. */
    void *irq_write;
    /*! 0x10. */
    void *irq_read;
    /*! 0x11. */
    void *frameproc_phasedata_write;
    /*! 0x11. */
    void *frameproc_phasedata_read;
    /*! 0x12. */
    void *rssi_rnsi;
    /*! 0x13. */
    void *rxtime;
    /*! 0x14. */
    void *txtime;
    /*! 0x15. */
    void *irqtime;
    /*! 0x16. */
    void *xtalcount_15_0;
    /*! 0x17. */
    void *xtalcount_23_16;
    /*! 0x18. */
    void *phy_0_1;
    /*! 0x19. */
    void *phy_1_2;
    /*! 0x1A. */
    void *phy_2_3;
    /*! 0x1B. */
    void *phy_4_5;
    /*! 0x1C. */
    void *phy_5_6;
    /*! 0x1D. */
    void *phy_6_7;
    /*! 0x1E -> 0x2C. */
    void *_empty_memory_space;
    /*! 0x2D. */
    void *preamb_debug;
    /*! 0x2E. */
    void *prelude;
    /*! 0x2F. */
    void *preamb_sfdlen;
    /*! 0x30. */
    void *sfd_15_0;
    /*! 0x31. */
    void *sfd_31_16;
    /*! 0x32. */
    void *crc_15_1;
    /*! 0x33. */
    void *crc_30_16;
    /*! 0x34. */
    void *framecfg_savetobuf;
    /*! 0x35. */
    void *rx_address;
    /*! 0x36. */
    void *tx_address;
    /*! 0x37. */
    void *rx_tx_size;
    /*! 0x38. */
    struct {
        /*! ===== Reading FIFO (after radio IRQ). ===== */
        /*! 0x38: If CIR is enabled: read CIR info from the FIFO. */
        void *recv_cir_info;
        /*! 0x38: If we have a main RX: read the CCA tries the TX had to make from FIFO. */
        void *recv_cca_retry_count;
        /*! 0x38: Always: read the frame size. */
        void *recv_frame_size;
        /*! 0x38: Always: read the header size. */
        void *recv_header_size;
        /*! 0x38: Always: read the header. */
        void *recv_header;
        /*! 0x38: Always: read the payload. */
        void *recv_payload;
        /*! ===== Writing FIFO (config the radio). ===== */
        /*! 0x38: Always: write the header size. */
        void *send_header_size;
        /*! 0x38: If user payload: write the header. */
        void *send_header;
        /*! 0x38: If user payload: write the payload. */
        void *send_payload;
        /*! 0x38: If not user payload: write the header and payload. */
        void *send_header_and_payload;
    } fifo;
    /*! 0x39. */
    void *rx_bufload;
    /*! 0x3A. */
    void *tx_bufload;
    /*! 0x3B. */
    void *actions_write;
    /*! 0x3B. */
    void *actions_read;
    /*! 0x3C. */
    void *power_state;
    /*! 0x3D. */
    void *supply_monitor;
    /*! 0x3E. */
    void *dcro_config;
    /*! 0x3F. */
    void *nvm;
} sr_registers_t;

#if !RADIO_QSPI_ENABLED

/** @brief Configure radio SPI transfer structure.
 */
typedef struct __attribute__((packed)) radio_cfg {
    /*! Register address for IRQs. */
    uint8_t addr_irq;
    /*! IRQ events. */
    uint16_t irq_reg;
    /*! Register address for actions. */
    uint8_t addr_actions;
    /*! Data value for actions. */
    uint8_t actions;
    /*! Register address for receive address. */
    uint8_t addr_rx_address;
    /*! Data value for receive address. */
    uint16_t rx_address;
    /*! Register address for transmit address. */
    uint8_t addr_tx_address;
    /*! Data value for transmit address. */
    uint16_t tx_address;
    /*! Register address for receive/transmit size. */
    uint8_t addr_rx_tx_size;
    /*! Data value for receive/transmit size. */
    uint16_t rx_tx_size;
    /*! Register address for PHY settings 0-1. */
    uint8_t addr_phy_0_1;
    /*! Data value for PHY settings 0-1. */
    uint16_t phy_0_1;
    /*! Register address for hard disable IO configs. */
    uint8_t addr_harddisables_ioconfig;
    /*! Data value for hard disable IO configs. */
    uint16_t harddisables_ioconfig;
    /*! Register address for preamble debug. */
    uint8_t addr_preamb_debug;
    /*! Data value for preamble debug. */
    uint16_t preamb_debug;
    /*! Start address for burst write. */
    uint8_t burst_write_start_addr;
    /*! CCA (Clear Channel Assessment) settings. */
    uint16_t cca_settings;
    /*! CCA threshold gain. */
    uint16_t cca_thres_gain;
    /*! Manual RF gain settings. */
    uint16_t rf_gain_manu;
    /*! IF (Intermediate Frequency)/BB (Baseband) gain LNA (Low Noise Amplifier) settings. */
    uint16_t if_bb_gain_lna;
    /*! Frequency settings for RX band configuration 1. */
    uint16_t rxbandfre_cfg1freq;
    /*! Frequency settings for configuration 2 and 3. */
    uint16_t cfg2freq_cfg3freq;
    /*! Configuration settings for widths, transmit power, and random pulses. */
    uint16_t cfg_widths_txpwr_randpulse;
    /*! Transmit pulse position. */
    uint16_t tx_pulse_pos;
    /*! Sleep period settings bits 15-0. */
    uint16_t slpperiod_15_0;
    /*! Sleep period power-up delay. */
    uint16_t slpperiod_pwrupdlay;
    /*! Time limit for bias delay. */
    uint16_t timelimit_biasdelay;
    /*! Timer configuration and sleep configuration. */
    uint16_t timercfg_sleepcfg;
    /*! Interrupt request settings. */
    uint16_t irq;
    /*! Frame processing phase data. */
    uint16_t frameproc_phasedata;
} radio_cfg_t;

/** @brief Fill header SPI transfer structure.
 */
typedef struct __attribute__((packed)) fill_header {
    /*! Register address for FIFO (burst write). */
    uint8_t addr_fifo;
    /*! Header data buffer. */
    uint8_t data_fifo[MAX_HEADER_SIZE];
} fill_header_t;

/** @brief Read events SPI transfer structure.
 */
typedef struct __attribute__((packed)) read_events {
    /*! Register address for the power status. */
    uint8_t addr_pwr_status;
    /*! Data value for the power status. */
    uint8_t pwr_status;
#if WPS_RADIO_COUNT == 2
    /*! Register address for Timer configuration and sleep configuration. */
    uint8_t addr_timercfg_sleepcfg;
    /*! Data value for Timer configuration and sleep configuration. */
    uint16_t set_timercfg_sleepcfg;
#endif /*  WPS_RADIO_COUNT == 2 */
    /*! Register address for setting actions. */
    uint8_t addr_set_actions;
    /*! Data value for setting actions. */
    uint8_t set_actions;
    /*! Register address for actions. */
    uint8_t addr_actions;
    /*! Data value for actions. */
    uint8_t actions;
    /*! Register address for setting IRQ (Interrupt Request). */
    uint8_t addr_set_irq;
    /*! Data value for setting IRQ (Interrupt Request). */
    uint16_t set_irq;
    /*! Register address for interrupt request (IRQ). */
    uint8_t addr_irq;
    /*! Data value for interrupt request (IRQ). */
    uint16_t irq;
} read_events_t;

/** @brief Read info SPI transfer structure.
 */
typedef struct __attribute__((packed)) read_info {
    /*! Register address for frame size. */
    uint8_t addr_frame_size;
    /*! Data value for frame size. */
    uint8_t data_frame_size;
    /*! Register address for header size. */
    uint8_t addr_header_size;
    /*! Data value for header size, if 'data_frame_size' == 0 then header size contains random value. */
    uint8_t data_header_size;
    /*! Start address for burst read. */
    uint8_t burst_read_start_addr;
    /*! Frame processing phase data. */
    uint16_t frameproc_phasedata;
    /*! RSSI (Received Signal Strength Indication) and RNSI (Received Noise Signal Indication) values. */
    uint16_t rssi_rnsi;
    /*! Receive time. */
    uint16_t rxtime;
} read_info_t;

/** @brief Read CIR Info SPI transfer structure.
 */
typedef struct __attribute__((packed)) read_cir_info {
    /*! Register address for CIR Info. */
    uint8_t addr_cir_info;
    /*! Data value for CIR Info. */
    uint8_t data_cir_info[PHASE_OFFSET_BYTE_COUNT];
} read_cir_info_t;

/** @brief Wireless protocol stack PHY Layer SPI transfer structures.
 */
typedef struct spi_transfers {
    /*! Configuration for radio, out. */
    radio_cfg_t radio_cfg_out;
    /*! Header filling, out. */
    fill_header_t fill_header_out;
    /*! Event reading, out. */
    read_events_t read_events_out;
    /*! Event reading, in. */
    read_events_t read_events_in;
    /*! Information reading, out. */
    read_info_t read_info_out;
    /*! Information reading, in. */
    read_info_t read_info_in;
    /*! CIR Information reading, in. */
    read_cir_info_t read_cir_info_in;
    /*! CIR Information reading, out. */
    read_cir_info_t read_cir_info_out;
    /*! SPI dummy buffer. */
    uint8_t spi_dummy_buffer[MAX_FRAMESIZE];
} spi_xfer_t;

#else

/** @brief Configure radio QSPI transfer structure.
 */
typedef struct __attribute__((packed)) radio_cfg {
    /*! Actions. */
    uint8_t actions;
    /*! Receive address. */
    uint16_t rx_address;
    /*! Transmit address. */
    uint16_t tx_address;
    /*! Receive/transmit size. */
    uint16_t rx_tx_size;
    /*! PHY settings 0-1. */
    uint16_t phy_0_1;
    /*! Hard disable IO configs. */
    uint16_t harddisables_ioconfig;
    /*! Preamble debug. */
    uint16_t preamb_debug;
    /*! CCA (Clear Channel Assessment) settings. */
    uint16_t cca_settings;
    /*! CCA threshold gain. */
    uint16_t cca_thres_gain;
    /*! Manual RF gain settings. */
    uint16_t rf_gain_manu;
    /*! IF (Intermediate Frequency)/BB (Baseband) gain LNA (Low Noise Amplifier) settings. */
    uint16_t if_bb_gain_lna;
    /*! Frequency settings for RX band configuration 1. */
    uint16_t rxbandfre_cfg1freq;
    /*! Frequency settings for configuration 2 and 3. */
    uint16_t cfg2freq_cfg3freq;
    /*! Configuration settings for widths, transmit power, and random pulses. */
    uint16_t cfg_widths_txpwr_randpulse;
    /*! Transmit pulse position. */
    uint16_t tx_pulse_pos;
    /*! Sleep period settings bits 15-0. */
    uint16_t slpperiod_15_0;
    /*! Sleep period power-up delay. */
    uint16_t slpperiod_pwrupdlay;
    /*! Time limit for bias delay. */
    uint16_t timelimit_biasdelay;
    /*! Timer configuration and sleep configuration. */
    uint16_t timercfg_sleepcfg;
    /*! Interrupt request settings. */
    uint16_t irq;
    /*! Frame processing phase data. */
    uint16_t frameproc_phasedata;
} radio_cfg_t;

/** @brief Fill header QSPI transfer structure.
 */
typedef struct __attribute__((packed)) fill_header {
    /*! Header data buffer. */
    uint8_t header[MAX_HEADER_SIZE];
} fill_header_t;

/** @brief Read events QSPI transfer structure.
 */
typedef struct __attribute__((packed)) read_events {
    /*! The power status. */
    uint8_t pwr_status;
#if WPS_RADIO_COUNT == 2
    /*! Timer configuration and sleep configuration. */
    uint16_t set_timercfg_sleepcfg;
#endif /*  WPS_RADIO_COUNT == 2 */
    /*! Actions. */
    uint8_t actions;
    /*! Interrupt request (IRQ). */
    uint16_t irq;
} read_events_t;

/** @brief Read info QSPI transfer structure.
 */
typedef struct __attribute__((packed)) read_info {
    /*! Frame size. */
    uint8_t frame_size;
    /*! Header size, if 'data_frame_size' == 0 then header size contains random value. */
    uint8_t header_size;
    /*! Frame processing phase data. */
    uint16_t frameproc_phasedata;
    /*! RSSI (Received Signal Strength Indication) and RNSI (Received Noise Signal Indication) values. */
    uint16_t rssi_rnsi;
    /*! Receive time. */
    uint16_t rxtime;
} read_info_t;

/** @brief Read CIR Info QSPI transfer structure.
 */
typedef struct __attribute__((packed)) read_cir_info {
    /*! Register address for CIR Info. */
    uint8_t addr_cir_info;
    /*! Data value for CIR Info. */
    uint8_t data_cir_info[PHASE_OFFSET_BYTE_COUNT];
} read_cir_info_t;

/** @brief Wireless protocol stack PHY Layer QSPI transfer structures.
 */
typedef struct qspi_transfers {
    /*! Radio configuration transfer struct (out). */
    radio_cfg_t radio_cfg;
    /*! Fill header transfer struct (out). */
    fill_header_t fill_header;
    /*! Read events transfer struct (in). */
    read_events_t read_events;
    /*! Read info transfer struct (in). */
    read_info_t read_info;
    /*! Read CIR info transfer struct (in). */
    read_cir_info_t read_cir_info;
} qspi_transfers_t;
#endif /* !RADIO_QSPI_ENABLED */

/* EXTERNS ********************************************************************/
extern radio_hal_t radio_hal[MAX_NUMBER_OF_RADIOS];

#if !RADIO_QSPI_ENABLED
extern spi_xfer_t spi_xfer[WPS_RADIO_COUNT];
#else
extern qspi_transfers_t qspi_xfer[WPS_RADIO_COUNT];
#endif /* !RADIO_QSPI_ENABLED */

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Links GPIO control functions to the static radio_hal instance.
 *
 *  @param[in] radio_id         Radio HAL structure array index.
 *  @param[in] set_reset_pin    Pointer to a function that sets the reset pin of radio.
 *  @param[in] reset_reset_pin  Pointer to a function that resets the reset pin of radio.
 *  @retval true   Link success.
 *  @retval false  Link fail.
 */
bool sr_access_radio_gpio_link(uint8_t radio_id, void (*set_reset_pin)(void), void (*reset_reset_pin)(void));

/** @brief Links SPI control functions to the static radio_hal instance.
 *
 *  @param[in] radio_id                              Radio HAL structure array index.
 *  @param[in] end_transfer                          Pointer to a function that sets the on-board controller SPI
 *                                                   chip-select pin of the radio.
 *  @param[in] begin_transfer                        Pointer to a function that reset the on-board controller SPI
 *                                                   chip-select pin of the radio.
 *  @param[in] transfer_half_duplex_rx_blocking      Pointer to a function that receives data from the radio in
 *                                                   blocking mode.
 *  @param[in] transfer_half_duplex_rx_non_blocking  Pointer to a function that receives data from the radio in
 *                                                   non-blocking mode.
 *  @param[in] transfer_half_duplex_tx_blocking      Pointer to a function that transmits data to the radio in
 *                                                   blocking mode.
 *  @param[in] transfer_half_duplex_tx_non_blocking  Pointer to a function that transmits data to the radio in
 *                                                   non-blocking mode.
 *  @param[in] transfer_full_duplex_blocking         Pointer to a function that reads and writes data full duplex on
 *                                                   the radio in blocking mode.
 *  @param[in] transfer_full_duplex_non_blocking     Pointer to a function that reads and writes data full duplex on
 *                                                   the radio in non-blocking mode.
 *  @param[in] set_access_mode_spi                   Pointer to a function that configures the access mode of the radio
 *                                                   for SPI.
 *  @param[in] set_access_mode_qspi                  Pointer to a function that configures the access mode of the radio
 *                                                   for QSPI.
 *  @param[in] is_transfer_busy                      Pointer to a function that reads the status of the radio's SPI.
 *                                                   interface.
 *  @retval true   Link success.
 *  @retval false  Link fail.
 */
bool sr_access_radio_comm_link(
    uint8_t radio_id, void (*end_transfer)(void), void (*begin_transfer)(void),
    void (*transfer_half_duplex_rx_blocking)(uint8_t command, uint8_t *tx_data, uint16_t size),
    void (*transfer_half_duplex_rx_non_blocking)(uint8_t command, uint8_t *tx_data, uint16_t size),
    void (*transfer_half_duplex_tx_blocking)(uint8_t command, uint8_t *rx_data, uint16_t size),
    void (*transfer_half_duplex_tx_non_blocking)(uint8_t command, uint8_t *rx_data, uint16_t size),
    void (*transfer_full_duplex_blocking)(uint8_t *tx_data, uint8_t *rx_data, uint16_t size),
    void (*transfer_full_duplex_non_blocking)(uint8_t *tx_data, uint8_t *rx_data, uint16_t size),
    void (*set_access_mode_spi)(void), void (*set_access_mode_qspi)(void), bool (*is_transfer_busy)(void));

/** @brief Links irq functions to the static radio_hal instance.
 *
 *  @param[in] radio_id                                 Radio HAL structure array index.
 *  @param[in] read_irq_pin                             Pointer to a function that reads the status of radio IRQ pin.
 *  @param[in] radio_context_switch                     Pointer to a function that manually triggers the radio IRQ pin
 *                                                      interrupt.
 *  @param[in] disable_radio_irq                        Pointer to a function that disables the IRQ external interrupt
 *                                                      for radio.
 *  @param[in] enable_radio_irq                         Pointer to a function that enables the IRQ external interrupt
 *                                                      for radio.
 *  @param[in] disable_radio_non_blocking_transfer_irq  Pointer to a function that disables the DMA SPI interrupt for
 *                                                      radio.
 *  @param[in] enable_radio_non_blocking_transfer_irq   Pointer to a function that enables the DMA SPI interrupt for
 *                                                      radio.
 *  @retval true   Link success.
 *  @retval false  Link fail.
 */
bool sr_access_radio_irq_link(uint8_t radio_id, bool (*read_irq_pin)(void), void (*radio_context_switch)(void),
                              void (*disable_radio_irq)(void), void (*enable_radio_irq)(void),
                              void (*disable_radio_non_blocking_transfer_irq)(void),
                              void (*enable_radio_non_blocking_transfer_irq)(void));

/** @brief Setup the transfer structures with the `sr_reg` in the PHY.
 *
 *  Associate the references to the pre-defined transfer structures for the.
 *  chosen radio transfer protocol. It will associate the `sr_reg` structure
 *  in the PHY to the buffers being transfered
 *
 *  @param[in] radio_id  Radio HAL structure array index.
 *  @param[in] sr_reg    SR11xx shadow registers.
 */
void sr_access_setup_transfer_structures(uint8_t radio_id, sr_registers_t *sr_reg);

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Trigger a context switch to the Radio IRQ context.
 *
 *  @param[in] radio_id  Radio HAL structure array index.
 */
static inline void sr_access_radio_context_switch(uint8_t radio_id)
{
    radio_hal[radio_id].radio_context_switch();
}

/** @brief Enable the Radio DMA interrupt.
 *
 *  @param[in] radio_id  Radio HAL structure array index.
 */
static inline void sr_access_enable_non_blocking_transfer_irq(uint8_t radio_id)
{
    radio_hal[radio_id].enable_radio_non_blocking_transfer_irq();
}

/** @brief Disable the Radio DMA interrupt.
 *
 *  @param[in] radio_id  Radio HAL structure array index.
 */
static inline void sr_access_disable_non_blocking_transfer_irq(uint8_t radio_id)
{
    radio_hal[radio_id].disable_radio_non_blocking_transfer_irq();
}

/** @brief Enable the Radio external interrupt.
 *
 *  @param[in] radio_id  Radio HAL structure array index.
 */
static inline void sr_access_enable_radio_irq(uint8_t radio_id)
{
    radio_hal[radio_id].enable_radio_irq();
}

/** @brief Disable the Radio external interrupt.
 *
 *  @param[in] radio_id  Radio HAL structure array index.
 */
static inline void sr_access_disable_radio_irq(uint8_t radio_id)
{
    radio_hal[radio_id].disable_radio_irq();
}

/** @brief Open the communication with the radio.
 *
 *  @param[in] radio_id  Radio HAL structure array index.
 */
static inline void sr_access_open(uint8_t radio_id)
{
    radio_hal[radio_id].begin_transfer();
}

/** @brief Close the communication with the radio.
 *
 *  @param[in] radio_id  Radio HAL structure array index.
 */
static inline void sr_access_close(uint8_t radio_id)
{
    radio_hal[radio_id].end_transfer();
}

/** @brief Return the status of the radio's SPI.
 *
 *  @param[in] radio_id  Radio HAL structure array index.
 *
 *  @retval true   Transfer is busy.
 *  @retval false  Transfer is not busy.
 */
static inline bool sr_access_is_transfer_busy(uint8_t radio_id)
{
    return radio_hal[radio_id].is_transfer_busy();
}

/** @brief Return the status of the radio's IRQ pin.
 *
 *  @param[in] radio_id  Radio HAL structure array index.
 *
 *  @retval true   High.
 *  @retval false  Low.
 */
static inline bool sr_access_read_irq_pin(uint8_t radio_id)
{
    return radio_hal[radio_id].read_irq_pin();
}

/** @brief Sets the reset pin of radio.
 *
 *  @param[in] radio_id  Radio HAL structure array index.
 */
static inline void sr_access_set_reset_pin(uint8_t radio_id)
{
    return radio_hal[radio_id].set_reset_pin();
}

/** @brief Resets the reset pin of radio.
 *
 *  @param[in] radio_id  Radio HAL structure array index.
 */
static inline void sr_access_reset_reset_pin(uint8_t radio_id)
{
    return radio_hal[radio_id].reset_reset_pin();
}

#if !RADIO_QSPI_ENABLED

/** @brief Write to an 8 bit register.
 *
 *  @param[in] radio_id  Radio HAL structure array index.
 *  @param[in] reg       Register to write.
 *  @param[in] value     Value to write.
 */
static inline void sr_access_write_reg8(uint8_t radio_id, uint8_t reg, uint8_t value)
{
    uint8_t tx_buffer[2];
    uint8_t rx_buffer[2];

    tx_buffer[0] = REG_WRITE | reg;
    tx_buffer[1] = value;

    radio_hal[radio_id].begin_transfer();
    radio_hal[radio_id].transfer_full_duplex_blocking(tx_buffer, rx_buffer, 2);
    radio_hal[radio_id].end_transfer();
}

/** @brief Write to a 16 bit register.
 *
 *  @param[in] radio_id  Radio HAL structure array index.
 *  @param[in] reg       Register to write.
 *  @param[in] value     Value to write.
 */
static inline void sr_access_write_reg16(uint8_t radio_id, uint8_t reg, uint16_t value)
{
    uint8_t tx_buffer[3];
    uint8_t rx_buffer[3];

    tx_buffer[0] = REG_WRITE | reg;
    memcpy(&tx_buffer[1], (uint8_t *)&value, 2);

    radio_hal[radio_id].begin_transfer();
    radio_hal[radio_id].transfer_full_duplex_blocking(tx_buffer, rx_buffer, 3);
    radio_hal[radio_id].end_transfer();
}

/** @brief Read from an 8 bit register.
 *
 *  @param[in] radio_id  Radio HAL structure array index.
 *  @param[in] reg       Register to read.
 */
static inline uint8_t sr_access_read_reg8(uint8_t radio_id, uint8_t reg)
{
    uint8_t tx_buffer[2];
    uint8_t rx_buffer[2];

    tx_buffer[0] = reg;

    radio_hal[radio_id].begin_transfer();
    radio_hal[radio_id].transfer_full_duplex_blocking(tx_buffer, rx_buffer, 2);
    radio_hal[radio_id].end_transfer();

    return rx_buffer[1];
}

/** @brief Read from a 16 bit register.
 *
 *  @param[in] radio_id  Radio HAL structure array index.
 *  @param[in] reg       Register to read.
 */
static inline uint16_t sr_access_read_reg16(uint8_t radio_id, uint8_t reg)
{
    uint8_t tx_buffer[3];
    uint8_t rx_buffer[3];

    tx_buffer[0] = reg;

    radio_hal[radio_id].begin_transfer();
    radio_hal[radio_id].transfer_full_duplex_blocking(tx_buffer, rx_buffer, 3);
    radio_hal[radio_id].end_transfer();

    return rx_buffer[1] | (rx_buffer[2] << 8);
}

/** @brief Write the config for the next timeslot to the radio in a non-blocking manner.
 *
 *  @param[in] radio_id  The radio index.
 */
static inline void sr_access_write_radio_cfg_non_blocking(uint8_t radio_id)
{
    radio_hal[radio_id].begin_transfer();
    radio_hal[radio_id].transfer_full_duplex_non_blocking((uint8_t *)&spi_xfer[radio_id].radio_cfg_out,
                                                          spi_xfer[radio_id].spi_dummy_buffer, sizeof(radio_cfg_t));
}

/** @brief Read the events report that occured after the OTA of the radio.
 *
 *  For example, this contains the frame outcome and the CCA fail/success status.
 *
 *  @param[in] radio_id  The radio index.
 */
static inline void sr_access_read_events_non_blocking(uint8_t radio_id)
{
    spi_xfer[radio_id].read_events_out.set_actions = SLEEP_0b1;
    spi_xfer[radio_id].read_events_out.set_irq = 0;
    radio_hal[radio_id].begin_transfer();
    radio_hal[radio_id].transfer_full_duplex_non_blocking((uint8_t *)&spi_xfer[radio_id].read_events_out,
                                                          (uint8_t *)&spi_xfer[radio_id].read_events_in,
                                                          sizeof(read_events_t));
}

/** @brief Read the info that occured after the OTA of the radio from the relevant registers.
 *
 *  For example, this contains the frame size, the RSSI and the RNSI.
 *
 *  @param[in] radio_id  The radio index.
 */
static inline void sr_access_read_info_non_blocking(uint8_t radio_id)
{
    radio_hal[radio_id].begin_transfer();
    radio_hal[radio_id].transfer_full_duplex_non_blocking((uint8_t *)&spi_xfer[radio_id].read_info_out,
                                                          (uint8_t *)&spi_xfer[radio_id].read_info_in,
                                                          sizeof(read_info_t));
}

/** @brief Read the Channel Impulse Response info that occured after the OTA of the radio from the RXFIFO.
 *
 *  @param[in] radio_id      The radio index.
 *  @param[in] cir_info_len  The number of bytes for the CIR info + 1.
 */
static inline void sr_access_read_cir_info_non_blocking(uint8_t radio_id, uint8_t cir_info_len)
{
    radio_hal[radio_id].begin_transfer();
    radio_hal[radio_id].transfer_full_duplex_non_blocking((uint8_t *)&spi_xfer[radio_id].read_cir_info_out,
                                                          (uint8_t *)&spi_xfer[radio_id].read_cir_info_in,
                                                          cir_info_len);
}

/** @brief Write the header to be sent OTA to the TXFIFO.
 *
 *  @param[in] radio_id    The radio index.
 *  @param[in] header_len  The number of bytes for the header + 1.
 */
static inline void sr_access_write_header_non_blocking(uint8_t radio_id, uint8_t header_len)
{
    radio_hal[radio_id].begin_transfer();
    radio_hal[radio_id].transfer_full_duplex_non_blocking((uint8_t *)&spi_xfer[radio_id].fill_header_out,
                                                          spi_xfer[radio_id].spi_dummy_buffer, header_len);
}

/** @brief Read the header received OTA from the RXFIFO.
 *
 *  @param[in] radio_id    The radio index.
 *  @param[in] header      The buffer where the read header should be stored to.
 *  @param[in] header_len  The number of bytes for the header + 1.
 */
static inline void sr_access_read_header_non_blocking(uint8_t radio_id, uint8_t *header, uint8_t header_len)
{
    spi_xfer[radio_id].spi_dummy_buffer[0] = REG_READ_BURST | REG8_FIFOS;
    radio_hal[radio_id].begin_transfer();
    radio_hal[radio_id].transfer_full_duplex_non_blocking(spi_xfer[radio_id].spi_dummy_buffer, header, header_len);
}

/** @brief Write the payload to be sent OTA to the TXFIFO.
 *
 *  @param[in] radio_id          The radio index.
 *  @param[in] payload_begin_it  The buffer to copy from into the TXFIFO.
 *  @param[in] payload_len       The number of bytes for the payload.
 */
static inline void sr_access_write_payload_non_blocking(uint8_t radio_id, uint8_t *payload_begin_it,
                                                        uint8_t payload_len)
{
    radio_hal[radio_id].begin_transfer();
    radio_hal[radio_id].transfer_full_duplex_non_blocking(payload_begin_it, spi_xfer[radio_id].spi_dummy_buffer,
                                                          payload_len);
}

/** @brief Read the payload received OTA from the RXFIFO.
 *
 *  @param[in] radio_id     The radio index.
 *  @param[in] payload      The buffer where the read payload should be stored to.
 *  @param[in] payload_len  The number of bytes for the payload.
 */
static inline void sr_access_read_payload_non_blocking(uint8_t radio_id, uint8_t *payload, uint8_t payload_len)
{
    spi_xfer[radio_id].spi_dummy_buffer[0] = REG_READ_BURST | REG8_FIFOS;
    uint8_t dummy_data;

    radio_hal[radio_id].begin_transfer();

    /* The transfer reading the payload is the second part of a burst transfer that was began when reading the header.
     * If the header size = 0, no transfer was done to retreive the header. Therefore, the burst transfer has not begun.
     * To simulate that the burst transfer is on going, we must do a 1 byte dummy transfer.
     */
    if (spi_xfer[radio_id].read_info_in.data_header_size == 0) {
        radio_hal[radio_id].transfer_full_duplex_blocking(spi_xfer[radio_id].spi_dummy_buffer, &dummy_data, 1);
    }

    radio_hal[radio_id].transfer_full_duplex_non_blocking(spi_xfer[radio_id].spi_dummy_buffer, payload, payload_len);
}

/** @brief Write the header and payload to be sent OTA to the TXFIFO.
 *
 *  @param[in] radio_id     The radio index.
 *  @param[in] payload      The buffer where the read payload should be stored to.
 *  @param[in] payload_len  The number of bytes for the payload.
 */
static inline void sr_access_write_header_and_payload_non_blocking(uint8_t radio_id, uint8_t *tx_fifo,
                                                                   uint8_t tx_fifo_len)
{
    radio_hal[radio_id].begin_transfer();
    radio_hal[radio_id].transfer_full_duplex_non_blocking(tx_fifo, spi_xfer[radio_id].spi_dummy_buffer, tx_fifo_len);
}

#else

/** @brief Write to an 8 bit register.
 *
 *  @param[in] radio_id  Radio HAL structure array index.
 *  @param[in] reg       Register to write.
 *  @param[in] value     Value to write.
 */
static inline void sr_access_write_reg8(uint8_t radio_id, uint8_t reg, uint8_t value)
{
    radio_hal[radio_id].begin_transfer();
    radio_hal[radio_id].transfer_half_duplex_tx_blocking(REG_WRITE | reg, &value, 1);
    radio_hal[radio_id].end_transfer();
}

/** @brief Write to a 16 bit register.
 *
 *  @param[in] radio_id  Radio HAL structure array index.
 *  @param[in] reg       Register to write.
 *  @param[in] value     Value to write.
 */
static inline void sr_access_write_reg16(uint8_t radio_id, uint8_t reg, uint16_t value)
{
    radio_hal[radio_id].begin_transfer();
    radio_hal[radio_id].transfer_half_duplex_tx_blocking(REG_WRITE | reg, (uint8_t *)&value, 2);
    radio_hal[radio_id].end_transfer();
}

/** @brief Read from an 8 bit register.
 *
 *  @param[in] radio_id  Radio HAL structure array index.
 *  @param[in] reg       Register to read.
 */
static inline uint8_t sr_access_read_reg8(uint8_t radio_id, uint8_t reg)
{
    uint8_t value;

    radio_hal[radio_id].begin_transfer();
    radio_hal[radio_id].transfer_half_duplex_rx_blocking(reg, &value, 1);
    radio_hal[radio_id].end_transfer();

    return value;
}

/** @brief Read from a 16 bit register.
 *
 *  @param[in] radio_id  Radio HAL structure array index.
 *  @param[in] reg       Register to read.
 */
static inline uint16_t sr_access_read_reg16(uint8_t radio_id, uint8_t reg)
{
    uint16_t value;

    radio_hal[radio_id].begin_transfer();
    radio_hal[radio_id].transfer_half_duplex_rx_blocking(reg, (uint8_t *)&value, 2);
    radio_hal[radio_id].end_transfer();

    return value;
}

/** @brief Write the config for the next timeslot to the radio in a non-blocking manner.
 *
 *  @param[in] radio_id  The radio index.
 */
static inline void sr_access_write_radio_cfg_non_blocking(uint8_t radio_id)
{
    /* Read the IRQ register. */
    sr_access_read_reg16(radio_id, REG16_IRQ);

    /* Fill the actions register. */
    sr_access_write_reg8(radio_id, REG8_ACTIONS, qspi_xfer[radio_id].radio_cfg.actions);

    /* Burst 0x35 -> 0x37. */
    uint16_t transfer_size = (REG16_RX_TX_SIZEREG - REG16_RXADDRESS + 1) * 2;

    radio_hal[radio_id].begin_transfer();
    radio_hal[radio_id].transfer_half_duplex_tx_blocking(REG_WRITE_BURST | REG16_RXADDRESS,
                                                         (uint8_t *)&qspi_xfer[radio_id].radio_cfg.rx_address,
                                                         transfer_size);
    radio_hal[radio_id].end_transfer();

    /* Other sparse configurations. */
    sr_access_write_reg16(radio_id, REG16_PHY_0_1, qspi_xfer[radio_id].radio_cfg.phy_0_1);
    sr_access_write_reg16(radio_id, REG16_HARDDISABLES_IOCONFIG, qspi_xfer[radio_id].radio_cfg.harddisables_ioconfig);
    sr_access_write_reg16(radio_id, REG16_PREAMB_DEBUG, qspi_xfer[radio_id].radio_cfg.preamb_debug);

    /* Burst 0x04 -> 0x11. */
    transfer_size = (REG16_FRAMEPROC_PHASEDATA - REG16_CCA_SETTINGS + 1) * 2;

    radio_hal[radio_id].begin_transfer();
    radio_hal[radio_id].transfer_half_duplex_tx_non_blocking(REG_WRITE_BURST | REG16_CCA_SETTINGS,
                                                             (uint8_t *)&qspi_xfer[radio_id].radio_cfg.cca_settings,
                                                             transfer_size);
}

/** @brief Read the events report that occured after the OTA of the radio.
 *
 *  For example, this contains the frame outcome and the CCA fail/success status.
 *
 *  @param[in] radio_id  The radio index.
 */
static inline void sr_access_read_events_non_blocking(uint8_t radio_id)
{
    qspi_xfer[radio_id].read_events.pwr_status = sr_access_read_reg8(radio_id, REG8_POWER_STATE);

#if WPS_RADIO_COUNT == 2
    qspi_xfer[radio_id].read_events.timercfg_sleepcfg = sr_access_read_reg16(radio_id, REG16_TIMERCFG_SLEEPCFG);
#endif /*  WPS_RADIO_COUNT == 2 */

    /* Put radio to sleep. */
    sr_access_write_reg8(radio_id, REG8_ACTIONS, SLEEP_0b1);
    qspi_xfer[radio_id].read_events.actions = sr_access_read_reg8(radio_id, REG8_ACTIONS);

    /* Disable all IRQ sources. */
    sr_access_write_reg16(radio_id, REG16_IRQ, 0);
    radio_hal[radio_id].begin_transfer();
    radio_hal[radio_id].transfer_half_duplex_rx_non_blocking(REG16_IRQ, (uint8_t *)&qspi_xfer[radio_id].read_events.irq,
                                                             sizeof(qspi_xfer[radio_id].read_events.irq));
}

/** @brief Read the info that occured after the OTA of the radio from the relevant registers.
 *
 *  For example, this contains the frame size, the RSSI and the RNSI.
 *
 *  @param[in] radio_id  The radio index.
 */
static inline void sr_access_read_info_non_blocking(uint8_t radio_id)
{
    qspi_xfer[radio_id].read_info.frame_size = sr_access_read_reg8(radio_id, REG8_FIFOS);
    qspi_xfer[radio_id].read_info.header_size = sr_access_read_reg8(radio_id, REG8_FIFOS);

    uint16_t transfer_size = (REG16_RXTIME - REG16_FRAMEPROC_PHASEDATA + 1) * 2;

    radio_hal[radio_id].begin_transfer();
    radio_hal[radio_id].transfer_half_duplex_rx_non_blocking(
        REG_READ_BURST | REG16_FRAMEPROC_PHASEDATA, (uint8_t *)&qspi_xfer[radio_id].read_info.frameproc_phasedata,
        transfer_size);
}

/** @brief Read the Channel Impulse Response info that occured after the OTA of the radio from the RXFIFO.
 *
 *  @param[in] radio_id      The radio index.
 *  @param[in] cir_info_len  The number of bytes for the CIR info + 1.
 */
static inline void sr_access_read_cir_info_non_blocking(uint8_t radio_id, uint8_t cir_info_len)
{
    radio_hal[radio_id].begin_transfer();
    radio_hal[radio_id].transfer_half_duplex_rx_non_blocking(REG_READ_BURST | REG8_FIFOS,
                                                             (uint8_t *)&qspi_xfer[radio_id].read_cir_info,
                                                             cir_info_len - 1);
}

/** @brief Write the header to be sent OTA to the TXFIFO.
 *
 *  @param[in] radio_id    The radio index.
 *  @param[in] header_len  The number of bytes for the header + 1.
 */
static inline void sr_access_write_header_non_blocking(uint8_t radio_id, uint8_t header_len)
{
    radio_hal[radio_id].begin_transfer();
    radio_hal[radio_id].transfer_half_duplex_tx_non_blocking(REG_WRITE_BURST | REG8_FIFOS,
                                                             qspi_xfer[radio_id].fill_header.header,
                                                             header_len - SPI_EMPTY_CMD_BYTE_SIZE);
}

/** @brief Read the header received OTA from the RXFIFO.
 *
 *  @param[in] radio_id    The radio index.
 *  @param[in] header      The buffer where the read header should be stored to.
 *  @param[in] header_len  The number of bytes for the header + 1.
 */
static inline void sr_access_read_header_non_blocking(uint8_t radio_id, uint8_t *header, uint8_t header_len)
{
    radio_hal[radio_id].begin_transfer();

    /* The -1 is because the header_len counts a normal SPI first "dummy byte" to receive the command. */
    radio_hal[radio_id].transfer_half_duplex_rx_non_blocking(REG_READ_BURST | REG8_FIFOS,
                                                             header + SPI_EMPTY_CMD_BYTE_SIZE,
                                                             header_len - SPI_EMPTY_CMD_BYTE_SIZE);
}

/** @brief Write the payload to be sent OTA to the TXFIFO.
 *
 *  @param[in] radio_id          The radio index.
 *  @param[in] payload_begin_it  The buffer to copy from into the TXFIFO.
 *  @param[in] payload_len       The number of bytes for the payload.
 */
static inline void sr_access_write_payload_non_blocking(uint8_t radio_id, uint8_t *payload_begin_it,
                                                        uint8_t payload_len)
{
    /* The CS should still be high. */
    radio_hal[radio_id].end_transfer();
    radio_hal[radio_id].begin_transfer();
    radio_hal[radio_id].transfer_half_duplex_tx_non_blocking(REG_WRITE_BURST | REG8_FIFOS, payload_begin_it,
                                                             payload_len);
}

/** @brief Read the payload received OTA from the RXFIFO.
 *
 *  @param[in] radio_id     The radio index.
 *  @param[in] payload      The buffer where the read payload should be stored to.
 *  @param[in] payload_len  The number of bytes for the payload.
 */
static inline void sr_access_read_payload_non_blocking(uint8_t radio_id, uint8_t *payload, uint8_t payload_len)
{
    /* The CS should still be high. */
    radio_hal[radio_id].end_transfer();
    radio_hal[radio_id].begin_transfer();
    radio_hal[radio_id].transfer_half_duplex_rx_non_blocking(REG_READ_BURST | REG8_FIFOS, payload, payload_len);
}

/** @brief Write the header and payload to be sent OTA to the TXFIFO.
 *
 *  @param[in] radio_id     The radio index.
 *  @param[in] payload      The buffer where the read payload should be stored to.
 *  @param[in] payload_len  The number of bytes for the payload.
 */
static inline void sr_access_write_header_and_payload_non_blocking(uint8_t radio_id, uint8_t *tx_fifo,
                                                                   uint8_t tx_fifo_len)
{
    radio_hal[radio_id].begin_transfer();
    radio_hal[radio_id].transfer_half_duplex_tx_non_blocking(REG_WRITE_BURST | REG8_FIFOS, &tx_fifo[1],
                                                             tx_fifo_len - 1);
}

#endif /* !RADIO_QSPI_ENABLED */

/** @brief Set the communication mode to the radio (SPI or QSPI).
 *
 *  @param[in] radio_id     The radio index.
 *  @param[in] access_mode  SPI or QSPI.
 */
static inline void sr_access_set_mode(uint8_t radio_id, access_mode_t access_mode)
{
#if !RADIO_QSPI_ENABLED
    /* We do not want to set the access mode of the QSPI peripheral if using only SPI. */
    return;
#endif

    switch (access_mode) {
    case SPI:
        radio_hal[radio_id].set_access_mode_spi();
        break;
    case QSPI:
        radio_hal[radio_id].set_access_mode_qspi();
        break;
    default:
        while (1);
    }
}

#ifdef __cplusplus
}
#endif

#endif /* SR_ACCESS_H_ */
