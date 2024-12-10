/** @file  wps_phy_def.h
 *  @brief Include every definition needed by the WPS PHY layer.
 *
 *  @note Supports little endian only.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef WPS_PHY_DEF_H_
#define WPS_PHY_DEF_H_

#ifdef __cplusplus
extern "C" {
#endif

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "circular_queue.h"
#include "wps_config.h"
#include "wps_def.h"
#include "xlayer.h"
#ifdef SPARK_WPS_CFG_FILE_EXISTS
#include "spark_wps_cfg.h"
#endif /* SPARK_WPS_CFG_FILE_EXISTS */

/* CONSTANTS ******************************************************************/
/*! Queue size for PHY layer state machine. */
#define PHY_STATE_Q_SIZE 10
/*! MAX number of RF channel bundle pointer */
#define MAX_NUMBER_OF_RF_CHANNEL_BUNDLE_PTR 10
/*! Max header size */
#define MAX_HEADER_SIZE 10
/*! Queue size for overwrite registers queue. */
#define PHY_OVERWRITE_REG_Q_SIZE 10
/*! Size of the empty byte for the RX frame data */
#define EMPTY_BYTE 1

/* TYPES **********************************************************************/
/** @brief Wireless protocol stack PHY Layer input signal.
 */
typedef enum phy_input_signal {
    /*! PHY Radio IRQ signal */
    PHY_SIGNAL_RADIO_IRQ = 0,
    /*! PHY DMA transfer complete signal */
    PHY_SIGNAL_DMA_CMPLT,
    /*! PHY prepare radio signal */
    PHY_SIGNAL_PREPARE_RADIO,
    /*! PHY syncing signal */
    PHY_SIGNAL_SYNCING
} phy_input_signal_t;

/** @brief Wireless protocol stack PHY Layer output signal.
 */
typedef enum phy_output_signal {
    /*! PHY no signal */
    PHY_SIGNAL_NONE = 0,
    /*! PHY processing signal */
    PHY_SIGNAL_PROCESSING,
    /*! PHY yield signal */
    PHY_SIGNAL_YIELD,
    /*! PHY config has been sent */
    PHY_SIGNAL_CONFIG_COMPLETE,
    /*! PHY config has been sent (blocking) */
    PHY_SIGNAL_BLOCKING_CONFIG_DONE,
    /*! PHY connect signal */
    PHY_SIGNAL_CONNECT,
    /*! PHY prepare done signal */
    PHY_SIGNAL_PREPARE_DONE,
    /*! PHY frame sent and ack signal */
    PHY_SIGNAL_FRAME_SENT_ACK,
    /*! PHY frame sent and nack signal */
    PHY_SIGNAL_FRAME_SENT_NACK,
    /*! PHY frame not sent signal */
    PHY_SIGNAL_FRAME_NOT_SENT,
    /*! PHY frame received signal */
    PHY_SIGNAL_FRAME_RECEIVED,
    /*! PHY frame missed signal */
    PHY_SIGNAL_FRAME_MISSED,
    /*! PHY error signal */
    PHY_SIGNAL_ERROR
} phy_output_signal_t;

/** @brief WPS PHY instance.
 */
typedef struct wps_phy wps_phy_t;

/** @brief Layer one state machine function pointer type.
 */
typedef void (*wps_phy_state_t)(wps_phy_t *phy);

/** @brief WPS PHY handle function pointer.
 */
typedef void (*wps_phy_handle_t)(wps_phy_t *phy);

/** @brief PHY layer configuration.
 */
typedef struct phy_cfg {
    /*! Radio instance */
    radio_t *radio;
    /*! Local address */
    uint16_t local_address;
    /*! Sync word configuration */
    syncword_cfg_t syncword_cfg;
    /*! Preamble length */
    uint32_t preamble_len;
    /*! Sleep level */
    sleep_lvl_t sleep_lvl;
    /*! CRC polynomial */
    uint32_t crc_polynomial;
    /*! Radio chip rate, only 20.48MHz is supported on SR1XXX */
    chip_rate_cfg_t chip_rate;
    /*! Radio RX gain */
    uint8_t rx_gain;
} wps_phy_cfg_t;

/** @brief Configure radio SPI transfer structure.
 */
typedef struct __packed radio_cfg {
    /*! Register address for actions */
    uint8_t addr_actions;
    /*! Data value for actions */
    uint8_t actions;
    /*! Register address for receive address */
    uint8_t addr_rx_address;
    /*! Data value for receive address */
    uint16_t rx_address;
    /*! Register address for transmit address */
    uint8_t addr_tx_address;
    /*! Data value for transmit address */
    uint16_t tx_address;
    /*! Register address for receive/transmit size */
    uint8_t addr_rx_tx_size;
    /*! Data value for receive/transmit size */
    uint16_t rx_tx_size;
    /*! Register address for PHY settings 0-1 */
    uint8_t addr_phy_0_1;
    /*! Data value for PHY settings 0-1 */
    uint16_t phy_0_1;
    /*! Start address for burst write */
    uint8_t burst_write_start_addr;
    /*! CCA (Clear Channel Assessment) settings */
    uint16_t cca_settings;
    /*! CCA threshold gain */
    uint16_t cca_thres_gain;
    /*! Manual RF gain settings */
    uint16_t rf_gain_manu;
    /*! IF (Intermediate Frequency)/BB (Baseband) gain LNA (Low Noise Amplifier) settings */
    uint16_t if_bb_gain_lna;
    /*! Frequency settings for RX band configuration 1 */
    uint16_t rxbandfre_cfg1freq;
    /*! Frequency settings for configuration 2 and 3 */
    uint16_t cfg2freq_cfg3freq;
    /*! Configuration settings for widths, transmit power, and random pulses */
    uint16_t cfg_widths_txpwr_randpulse;
    /*! Transmit pulse position */
    uint16_t tx_pulse_pos;
    /*! Sleep period settings bits 15-0 */
    uint16_t slpperiod_15_0;
    /*! Sleep period power-up delay */
    uint16_t slpperiod_pwrupdlay;
    /*! Time limit for bias delay */
    uint16_t timelimit_biasdelay;
    /*! Timer configuration and sleep configuration */
    uint16_t timercfg_sleepcfg;
    /*! Interrupt request settings */
    uint16_t irq;
    /*! Frame processing phase data */
    uint16_t frameproc_phasedata;
} radio_cfg_t;

/** @brief Fill header SPI transfer structure.
 */
typedef struct __packed fill_header {
    /*! Register address for FIFO (burst write) */
    uint8_t addr_fifo;
    /*! Header data buffer */
    uint8_t data_fifo[MAX_HEADER_SIZE];
} fill_header_t;

/** @brief Read events SPI transfer structure.
 */
typedef struct __packed read_events {
    /*! Register address for the power status. */
    uint8_t addr_pwr_status;
    /*! Data value for the power status. */
    uint8_t pwr_status;
#if WPS_RADIO_COUNT == 2
    /*! Register address for Timer configuration and sleep configuration */
    uint8_t addr_timercfg_sleepcfg;
    /*! Data value for Timer configuration and sleep configuration */
    uint16_t set_timercfg_sleepcfg;
#endif
    /*! Register address for setting actions */
    uint8_t addr_set_actions;
    /*! Data value for setting actions */
    uint8_t set_actions;
    /*! Register address for actions */
    uint8_t addr_actions;
    /*! Data value for actions */
    uint8_t actions;
    /*! Register address for setting IRQ (Interrupt Request) */
    uint8_t addr_set_irq;
    /*! Data value for setting IRQ (Interrupt Request) */
    uint16_t set_irq;
    /*! Register address for interrupt request (IRQ) */
    uint8_t addr_irq;
    /*! Data value for interrupt request (IRQ) */
    uint16_t irq;
} read_events_t;

/** @brief Read info SPI transfer structure.
 */
typedef struct __packed read_info {
    /*! Register address for frame size */
    uint8_t addr_frame_size;
    /*! Data value for frame size */
    uint8_t data_frame_size;
    /*! Register address for header size */
    uint8_t addr_header_size;
    /*! Data value for header size, if 'data_frame_size' == 0 then header size contains random value */
    uint8_t data_header_size;
    /*! Start address for burst read */
    uint8_t burst_read_start_addr;
    /*! Frame processing phase data */
    uint16_t frameproc_phasedata;
    /*! RSSI (Received Signal Strength Indication) and RNSI (Received Noise Signal Indication) values */
    uint16_t rssi_rnsi;
    /*! Receive time */
    uint16_t rxtime;
} read_info_t;

/** @brief Wireless protocol stack PHY Layer SPI transfer structures.
 */
typedef struct spi_transfers {
    /*! Configuration for radio, out */
    radio_cfg_t radio_cfg_out;
    /*! Header filling, out */
    fill_header_t fill_header_out;
    /*! Event reading, out */
    read_events_t read_events_out;
    /*! Event reading, in */
    read_events_t read_events_in;
    /*! Information reading, out */
    read_info_t read_info_out;
    /*! Information reading, in */
    read_info_t read_info_in;
    /*! SPI dummy buffer */
    uint8_t spi_dummy_buffer[MAX_FRAMESIZE];
} spi_xfer_t;

/** @brief PHY layer configuration.
 */
typedef struct reg {
    /*! Register address */
    uint8_t addr;
    /*! Register value */
    uint16_t val;
} reg_t;

/** @brief SR1100 PHY debugging registers.
 */
typedef struct phy_debug_cfg {
    /*! Enable/Disable debugging feature of the radio */
    bool            enable;
    /*! Flag to notify application when its safe to modify channel cfg */
    bool            busy_channel_config;
    /*! Phase offset enable flag */
    bool            phase_offset_stats_enable;
    /*! Preamble detection register 0x2C raw value */
    uint16_t preamble_detection;
    /*! Interleav flag in register 0x28 */
    interleav_cfg_t interleav;
    /*! Array of the pointers of each RF channel bundle. */
    rf_channel_t    *rf_channel_bundle[MAX_NUMBER_OF_RF_CHANNEL_BUNDLE_PTR];
    /*! Actual number of RF channel bundle pointer */
    uint8_t         nb_rf_channel_bundle;
    /*! Intermediate-frequency variable gain amplifier #4 gain setting, register 0x26 */
    uint8_t         if_gain_4;
                    /* Value for register 0x11*/
    uint8_t         ant_lna_bias;
    /*! Syncword detection register 0x2D raw value */
    uint16_t syncword_detection;
} phy_debug_cfg_t;

/** @brief WPS PHY instance.
 */
struct wps_phy {
    /*! Wireless protocol stack phy Layer input signal*/
    phy_input_signal_t input_signal;

    /*! Wireless protocol stack phy Layer output signal main*/
    phy_output_signal_t signal_main;
    /*! Wireless protocol stack phy Layer output signal auto*/
    phy_output_signal_t signal_auto;

    /*! Already initialized radio instance. */
    radio_t *radio;

    /*! Node local address */
    uint16_t local_address;
    /*! Main cross layer*/
    xlayer_t *xlayer_main;
    /*! Auto cross layer*/
    xlayer_t *xlayer_auto;
    /*! Configuration */
    xlayer_cfg_internal_t *config;

    /* Internal Variables */
    /*! Current state machine state*/
    wps_phy_state_t *current_state;
    /*! Current state machine state*/
    wps_phy_state_t end_state;
    /*! Next state machine state queue*/
    circular_queue_t next_states;
    /*! Next state machine state pool*/
    wps_phy_state_t *next_state_pool[PHY_STATE_Q_SIZE];
    /*! Ovewrite registers queue */
    circular_queue_t overwrite_regs_queue;
    /*! Ovewrite registers pool */
    reg_t overwrite_regs_pool[PHY_OVERWRITE_REG_Q_SIZE];
    /*! State index*/
    uint8_t state_step;
    /*! Pwr status and command register value */
    uint8_t pwr_status_cmd;

    /*! Radio actions register fields settings */
    uint8_t radio_actions;
    /*! Header size */
    uint8_t header_size;

    /*! Syncing period in PLL cycles */
    uint16_t syncing_period_pll_cycles;

    /*! Wait for end of transmission of ack frame */
    bool wait_for_ack_tx;

    /*! Contains info about a write register access */
    xlayer_write_request_info_t write_request_info;
    /*! Contains info about a read register access */
    xlayer_read_request_info_t read_request_info;
    /*! Current state machine state*/
    wps_phy_handle_t phy_handle;
    /*! Debug configuration raw registers */
    phy_debug_cfg_t debug_cfg;
    /*! SPI transfer structures */
    spi_xfer_t spi_xfer;

    /*! MAC pointer */
    void *mac;
    /*! PHY callback */
    void (*wps_phy_callback)(void *mac, wps_phy_t *wps_phy);
};

#if WPS_RADIO_COUNT > 1

/** @brief WPS Dual radio function pointer configuration structure.
 */
typedef struct wps_multi_cfg {
    /*! Radio timer frequency in Hz. */
    uint32_t timer_frequency_hz;
    /*! Replying radio selection average sample count */
    uint16_t avg_sample_count;
    /*! Replying radio selection mode */
    multi_radio_mode_t mode;
    /*! Radio TX wakeup mode. */
    multi_radio_tx_wakeup_mode_t tx_wakeup_mode;
    /*! Replying radio selection RSSI threshold */
    uint8_t rssi_threshold;
} wps_multi_cfg_t;

#endif

#ifdef __cplusplus
}
#endif

#endif /* WPS_PHY_DEF_H_ */
