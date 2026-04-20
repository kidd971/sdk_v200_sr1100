/** @file  wps_phy_def.h
 *  @brief Include every definition needed by the WPS PHY layer.
 *
 *  @note Supports little endian only.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
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
#include "sr_access.h"
#include "wps_config.h"
#include "xlayer.h"
#ifdef SPARK_WPS_CFG_FILE_EXISTS
#include "spark_wps_cfg.h"
#endif /* SPARK_WPS_CFG_FILE_EXISTS */
#if WPS_RADIO_COUNT > 1
#include "link_multi_radio.h"
#endif

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
/* The byte required to hold the size of the header. */
#define HDR_SIZE_SIZE 1

/* TYPES **********************************************************************/
/** @brief Wireless protocol stack PHY Layer input signal.
 */
typedef enum phy_input_signal {
    /*! PHY Radio IRQ signal. */
    PHY_SIGNAL_RADIO_IRQ = 0,
    /*! PHY DMA transfer complete signal. */
    PHY_SIGNAL_DMA_CMPLT,
    /*! PHY prepare radio signal. */
    PHY_SIGNAL_PREPARE_RADIO,
    /*! PHY syncing signal. */
    PHY_SIGNAL_SYNCING
} phy_input_signal_t;

/** @brief Wireless protocol stack PHY Layer output signal.
 */
typedef enum phy_output_signal {
    /*! PHY no signal. */
    PHY_SIGNAL_NONE = 0,
    /*! PHY processing signal. */
    PHY_SIGNAL_PROCESSING,
    /*! PHY yield signal. */
    PHY_SIGNAL_YIELD,
    /*! PHY config has been sent. */
    PHY_SIGNAL_CONFIG_COMPLETE,
    /*! PHY config has been sent (blocking). */
    PHY_SIGNAL_BLOCKING_CONFIG_DONE,
    /*! PHY connect signal. */
    PHY_SIGNAL_CONNECT,
    /*! PHY prepare done signal. */
    PHY_SIGNAL_PREPARE_DONE,
    /*! PHY frame sent and ack signal. */
    PHY_SIGNAL_FRAME_SENT_ACK,
    /*! PHY frame sent and nack signal. */
    PHY_SIGNAL_FRAME_SENT_NACK,
    /*! PHY frame not sent signal. */
    PHY_SIGNAL_FRAME_NOT_SENT,
    /*! PHY frame received signal. */
    PHY_SIGNAL_FRAME_RECEIVED,
    /*! PHY frame missed signal. */
    PHY_SIGNAL_FRAME_MISSED,
    /*! PHY error signal. */
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
    /*! Radio instance. */
    radio_t *radio;
    /*! Local address. */
    uint16_t local_address;
    /*! SFD configuration. */
    sfd_cfg_t sfd_cfg;
    /*! Preamble length register value. */
    uint16_t preamble_len_reg_val;
    /*! Sleep level. */
    sleep_lvl_t sleep_lvl;
    /*! CRC polynomial. */
    uint32_t crc_polynomial;
    /*! Radio chip rate. */
    chip_rate_cfg_t chip_rate;
    /*! Radio RX gain. */
    uint8_t rx_gain;
    /*! Enable extraction of phase offset data for ISI Mitigation indicator. */
    bool isi_indicator_enabled;
} wps_phy_cfg_t;

/** @brief PHY layer configuration.
 */
typedef struct reg {
    /*! Register address. */
    uint8_t addr;
    /*! Register value. */
    uint16_t val;
} reg_t;

/** @brief WPS PHY instance.
 */
struct wps_phy {
    /*! Wireless protocol stack phy Layer input signal. */
    phy_input_signal_t input_signal;

    /*! Wireless protocol stack phy Layer output signal main. */
    phy_output_signal_t signal_main;
    /*! Wireless protocol stack phy Layer output signal auto. */
    phy_output_signal_t signal_auto;

    /*! Already initialized radio instance. */
    radio_t *radio;

    /*! Node local address. */
    uint16_t local_address;
    /*! Main cross layer. */
    xlayer_t *xlayer_main;
    /*! Auto cross layer. */
    xlayer_t *xlayer_auto;
    /*! Configuration. */
    xlayer_cfg_internal_t *config;

    /*! Internal Variables. */
    /*! Current state machine. state. */
    wps_phy_state_t *current_state;
    /*! Current state machine. state. */
    wps_phy_state_t end_state;
    /*! Next state machine state queue. */
    circular_queue_t next_states;
    /*! Next state machine state pool. */
    wps_phy_state_t *next_state_pool[PHY_STATE_Q_SIZE];
    /*! Ovewrite registers queue. */
    circular_queue_t overwrite_regs_queue;
    /*! Ovewrite registers pool. */
    reg_t overwrite_regs_pool[PHY_OVERWRITE_REG_Q_SIZE];
    /*! State. index*/
    uint8_t state_step;
    /*! Pwr status and command register value. */
    uint8_t pwr_status_cmd;

    /*! Radio actions register fields settings. */
    uint8_t radio_actions;
    /*! Header size. */
    uint8_t header_size;
    /*! Number of byte that will be available in the RXbuffer for ISI mitigation indicator. */
    uint8_t phase_offset_bytes_to_read;
    /*! Wheter or not the Phase offset reading feature is enabled. */
    uint8_t phase_offset_feature_enabled;

    /*! Syncing period in PLL cycles. */
    uint16_t syncing_period_pll_cycles;

    /*! Empty TX flag. */
    bool empty_tx;

    /*! Sleep level per timeslot enable flag. */
    bool sleep_lvl_per_ts_enabled;

    /*! Wait for end of transmission of ack frame. */
    bool wait_for_ack_tx;

    /*! Contains info about a write register access. */
    xlayer_write_request_info_t write_request_info;
    /*! Contains info about a read register access. */
    xlayer_read_request_info_t read_request_info;
    /*! Current state machine state. */
    wps_phy_handle_t phy_handle;
    /*! Spark radio shadow memory. */
    sr_registers_t sr_reg;
    /*! MAC pointer. */
    void *mac;
    /*! PHY callback. */
    void (*wps_phy_callback)(void *mac, wps_phy_t *wps_phy);
};

#if WPS_RADIO_COUNT > 1

/** @brief WPS Dual radio function pointer configuration structure.
 */
typedef struct wps_multi_cfg {
    /*! Radio timer frequency in Hz. */
    uint32_t timer_frequency_hz;
    /*! Leading radio selection average sample count. */
    uint16_t avg_sample_count;
    /*! Leading radio selection mode. */
    multi_radio_mode_t mode;
    /*! Radio TX wakeup mode. */
    multi_radio_tx_wakeup_mode_t tx_wakeup_mode;
    /*! Leading radio selection RSSI threshold. */
    uint8_t rssi_threshold;
} wps_multi_cfg_t;

#endif

#ifdef __cplusplus
}
#endif

#endif /* WPS_PHY_DEF_H_ */
