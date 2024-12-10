/** @file wps_mac_def.h
 *  @brief Wireless Protocol Stack MAC definitions.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef WSP_MAC_DEF_H_
#define WSP_MAC_DEF_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "link_ddcm.h"
#include "link_scheduler.h"
#include "link_tdma_sync.h"
#include "wps_config.h"
#include "wps_def.h"
#include "wps_mac_statistics.h"
#include "wps_phy.h"
#include "xlayer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/*! MAC header byte 0, bit 7 */
#define HEADER_BYTE0_SEQ_NUM_MASK BIT(7)
/*! MAC header byte 0, bit 6 downto 0 */
#define HEADER_BYTE0_TIME_SLOT_ID_MASK BITS8(6, 0)

/* TYPES **********************************************************************/
/** @brief Wireless protocol stack MAC Layer output signal.
 */
typedef enum wps_mac_output_signal {
    /*! MAC layer empty output signal */
    MAC_SIGNAL_WPS_EMPTY = 0,
    /*! MAC layer frame receive output signal */
    MAC_SIGNAL_WPS_FRAME_RX_SUCCESS,
    /*! MAC layer frame miss output signal */
    MAC_SIGNAL_WPS_FRAME_RX_FAIL,
    /*! MAC layer No more space available in RX queue */
    MAC_SIGNAL_WPS_FRAME_RX_OVERRUN,
    /*! MAC layer successful transmission output signal */
    MAC_SIGNAL_WPS_TX_SUCCESS,
    /*! MAC layer unsuccessful transmission output signal */
    MAC_SIGNAL_WPS_TX_FAIL,
    /*! MAC layer dropped frame output signal */
    MAC_SIGNAL_WPS_TX_DROP,
    /*! MAC layer frame prepare done signal */
    MAC_SIGNAL_WPS_PREPARE_DONE,
    /*! MAC layer Enter syncing state output signal */
    MAC_SIGNAL_SYNCING,
} wps_mac_output_signal_t;

/** @brief Wireless protocol stack MAC Layer protocols identifiers.
 */
typedef enum wps_mac_proto_id {
    /*! MAC layer timeslot ID and SAW protocol identifier */
    MAC_PROTO_ID_TIMESLOT_SAW = 0,
    /*! MAC layer channel index protocol identifier */
    MAC_PROTO_ID_CHANNEL_INDEX,
    /*! MAC layer RDO protocol identifier */
    MAC_PROTO_ID_RDO,
    /*! MAC layer ranging phase provider ID protocol identifier */
    MAC_PROTO_ID_RANGING_RESPONDER,
    /*! MAC layer ranging phase protocol identifier */
    MAC_PROTO_ID_RANGING_INITIATOR,
    /*! MAC layer connection ID protocol identifier */
    MAC_PROTO_ID_CONNECTION_ID,
    /*! MAC layer Credit Flow Control protocol identifier */
    MAC_PROTO_ID_CREDIT_FC,
} wps_mac_proto_id_t;

/** @brief Wireless protocol stack MAC Layer output signal parameter.
 */
typedef struct wps_mac_output_signal_info {
    /*! Main output signal */
    wps_mac_output_signal_t main_signal;
    /*! Pending output signal */
    wps_mac_output_signal_t auto_signal;
} wps_mac_output_signal_info_t;

/** @brief Wireless protocol stack Layer 2 input signal parameters.
 */
typedef struct wps_mac_input_signal_info {
    /*! MAC Layer input signal */
    phy_output_signal_t main_signal;
    /*! MAC Layer input signal */
    phy_output_signal_t auto_signal;
} wps_mac_input_signal_info_t;

/** @brief Wireless protocol stack MAC Layer sync module init field.
 */
typedef struct wps_mac_sync_cfg {
    /*! Desired sleep level for Sync */
    sleep_lvl_t sleep_level;
    /*! Frame preamble length */
    uint32_t preamble_len;
    /*! Frame syncword length */
    uint32_t syncword_len;
    /*! ISI mitigation level */
    isi_mitig_t isi_mitig;
    /*! ISI mitigation level corresponding pauses */
    uint8_t isi_mitig_pauses;
    /*! TX jitter enable flag */
    bool tx_jitter_enabled;
    /*! Chip rate */
    chip_rate_cfg_t chip_rate;
} wps_mac_sync_cfg_t;

/** @brief Wireless Protocol Stack input signals.
 */
typedef enum wps_input_signal {
    /*! WPS is not initialized */
    WPS_NOT_INIT,
    /*! WPS radio IRQ signal */
    WPS_RADIO_IRQ,
    /*! WPS transfer complete signal */
    WPS_TRANSFER_COMPLETE,
    /*! WPS complete signal */
    WPS_CONNECT,
    /*! WPS disconnect signal */
    WPS_DISCONNECT,
    /*! WPS halt signal */
    WPS_HALT,
    /*! WPS resume signal */
    WPS_RESUME,
} wps_input_signal_t;

/** @brief Wireless protocol stack MAC Layer main structure.
 */
typedef struct wps_mac_struct {
    /*! Input signal instance */
    wps_mac_input_signal_info_t input_signal;
    /*! Output signal instance */
    wps_mac_output_signal_info_t output_signal;

    /*! Current scheduler timeslot */
    timeslot_t *timeslot;
    /*! Schedule instance */
    scheduler_t scheduler;
    /*! Channel hopping instance */
    channel_hopping_t channel_hopping;
    /*! Current channel hopping index */
    uint8_t channel_index;
    /*! Concurrent network ID */
    uint8_t network_id;
    /*! Fast sync enable flag */
    bool fast_sync_enabled;
    /*! Delay was applied in last timeslot */
    bool delay_in_last_timeslot;
    /*! Delay, in radio clock cycle, of the last timeslot. */
    uint16_t last_timeslot_delay;

    /*! Node address to handle RX/TX timeslot */
    uint16_t local_address;
    /*! Syncing address */
    uint16_t syncing_address;

    /*! Synchronization module instance */
    tdma_sync_t tdma_sync;

    /*! Current node role (Coordinator/Node) */
    wps_role_t node_role;

    /*! Xlayer instance when application TX queue is empty */
    xlayer_t empty_frame_tx;
    /*! Xlayer instance when application RX queue is empty */
    xlayer_t empty_frame_rx;
    /*! Xlayer instance when auto-reply connection doesn't exist */
    xlayer_t empty_auto_reply_frame;

    /*! MAC Layer main xlayer node */
    xlayer_t *main_xlayer;
    /*! MAC Layer auto xlayer node */
    xlayer_t *auto_xlayer;
    /*! Configuration */
    xlayer_cfg_internal_t config;

    /*! Random Datarate Offset (RDO) instance. */
    link_rdo_t link_rdo;
    /*! Distributed desync instance. */
    link_ddcm_t link_ddcm;
    /*! RX node */
    xlayer_queue_node_t *rx_node;
    /*! Main connection ID */
    uint8_t main_connection_id;
    /*! Auto_reply connection ID */
    uint8_t auto_connection_id;
    /*! Main connection ID for receiving ACK - no auto-reply connection */
    uint8_t main_ack_connection_id;
    /*! Current main connection */
    wps_connection_t *main_connection;
    /*! Current auto reply connection */
    wps_connection_t *auto_connection;

    /* phases */
    /*! Phase data */
    wps_phase_info_t phase_data;

    /*! Max possible header size to be received */
    uint8_t max_expected_header_size;
    /*! Max possible payload size to be received */
    uint8_t max_expected_payload_size;
    /*! Channel structure for muted transfer */
    rf_channel_t muted_transfer_channel;

    /*! function pointer to trigger the callback process */
    void (*callback_context_switch)(void);
    /*! Circular queue instance to save the callbacks */
    circular_queue_t callback_queue;
    /*! Circular queue to forward application request to WPS */
    circular_queue_t request_queue;
    /*! WPS throttle feature configuration structure */
    circular_queue_t *schedule_ratio_cfg_queue;
    /*! WPS write register request queue */
    circular_queue_t *write_request_queue;
    /*! WPS write register request queue */
    circular_queue_t *read_request_queue;
    /*! WPS current signal */
    volatile wps_input_signal_t signal;

#if WPS_ENABLE_STATS_USED_TIMESLOTS || WPS_ENABLE_PHY_STATS || WPS_ENABLE_PHY_STATS_PER_BANDS
    wps_mac_stats_t stats_process_data;
#endif /* WPS_ENABLE_STATS_USED_TIMESLOTS, WPS_ENABLE_PHY_STATS, WPS_ENABLE_PHY_STATS_PER_BANDS */
} wps_mac_t;

#ifdef __cplusplus
}
#endif

#endif /* WSP_MAC_DEF_H_ */
