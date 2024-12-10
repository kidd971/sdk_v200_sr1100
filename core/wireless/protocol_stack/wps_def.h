/** @file  wps_def.h
 *  @brief Wireless Protocol Stack definitions used by multiple modules.
 *
 *  @copyright Copyright (C) 2020 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef WPS_DEF_H
#define WPS_DEF_H

#ifdef __cplusplus
extern "C" {
#endif

/* INCLUDES *******************************************************************/
#include "circular_queue.h"
#include "link_cca.h"
#include "link_channel_hopping.h"
#include "link_connect_status.h"
#include "link_credit_flow_ctrl.h"
#include "link_fallback.h"
#include "link_gain_loop.h"
#include "link_lqi.h"
#include "link_phase.h"
#include "link_protocol.h"
#include "link_random_datarate_offset.h"
#include "link_saw_arq.h"
#include "sr_def.h"
#include "sr_spectral.h"
#include "wps_config.h"
#include "wps_connection_list.h"
#include "wps_error.h"
#include "xlayer_circular_data.h"
#include "xlayer_queue.h"

/* CONSTANTS ******************************************************************/
/*! WPS radio FIFO size */
#define WPS_RADIO_FIFO_SIZE 128
/*! Size of the payload size automatically loaded in the FIFO. */
#define WPS_PAYLOAD_SIZE_BYTE_SIZE 1

/*! WPS throttle ratio granularity (100 / value) */
#define WPS_PATTERN_THROTTLE_GRANULARITY 20
/*! WPS threshold to disable CCA */
#define WPS_DISABLE_CCA_THRESHOLD 0xFF
#ifndef WPS_MAX_CONN_PER_TIMESLOT
/*! Maximum number of connections per time slot */
#define WPS_MAX_CONN_PER_TIMESLOT 3
#endif
/*! Maximum priority allowed */
#define WPS_MAX_CONN_PRIORITY (WPS_MAX_CONN_PER_TIMESLOT - 1)

/** @brief Minimum size of queue elements.
 */
#define WPS_MIN_QUEUE_SIZE 2

/* TYPES **********************************************************************/
#if !WPS_DISABLE_FRAGMENTATION
/** @brief WPS fragment Connection instance
 */
typedef struct frag {
    /*! Fragmentation enable flag */
    bool enabled;
    /*! Fragmentation xlayer queue */
    xlayer_queue_t xlayer_queue;
    /*! Use to track the number of fragment that has been sent*/
    circular_queue_t meta_data_queue_tx;
    /*! Remaining fragment for the frame*/
    uint16_t remaining_fragment;
    /*! Current fragement index */
    uint16_t fragment_index;
    /*! Current transaction ID */
    uint8_t transaction_id;
    /*! Tell whether the current frame have been dropped */
    bool dropped_frame;
    /*! Number of payloads ready to read */
    uint16_t enqueued_count;
    /*! Function called by the wps to indicate the transmission failed */
    void (*tx_success_callback)(void *parg);
    /*! TX fail callback void pointer argument */
    void *tx_success_parg_callback;
    /*! Function called by the wps to indicate the frame has been received */
    void (*tx_drop_callback)(void *parg);
    /*! RX fail callback void pointer argument */
    void *tx_drop_parg_callback;
    /*! Function called by the wps to indicate the transmission failed */
    void (*tx_fail_callback)(void *parg);
    /*! TX fail callback void pointer argument */
    void *tx_fail_parg_callback;
    /*! Function called by the wps to indicate the frame has been received */
    void (*rx_success_callback)(void *parg);
    /*! RX success callback void pointer argument */
    void *rx_success_parg_callback;
    /*! Function called by the wps to indicate the frame has been received */
    void (*rx_fail_callback)(void *parg);
    /*! RX fail callback void pointer argument */
    void *rx_fail_parg_callback;
    /*! Function called by the wps to indicate an event */
    void (*event_callback)(void *parg);
    /*! Event callback void pointer argument */
    void *event_parg_callback;
} frag_t;
#endif /* !WPS_DISABLE_FRAGMENTATION */

/** @brief WPS Connection.
 */
typedef struct wps_connection wps_connection_t;

#if WPS_ENABLE_LINK_STATS
/** @brief WPS statistics function.
 */
typedef struct wps_stats {
    /*! Number of payload sent */
    uint32_t tx_success;
    /*! Number of byte sent */
    uint32_t tx_byte_sent;
    /*! Number of payload dropped */
    uint32_t tx_drop;
    /*! Number of TX payload fail */
    uint32_t tx_fail;
    /*! Number of payload received */
    uint32_t rx_received;
    /*! Number of byte received */
    uint32_t rx_byte_received;
    /*! Number of payload dropped because of an RX buffer overrun */
    uint32_t rx_overrun;
    /*! Number of times one of the CCA attempts passed */
    uint32_t cca_pass;
    /*! Number of times all CCA attempts failed */
    uint32_t cca_tx_fail;
    /*! Total number of CCA fails */
    uint32_t cca_fail;
} wps_stats_t;
#endif /* WPS_ENABLE_LINK_STATS */

/** @brief WPS event enum definition.
 */
typedef enum wps_event {
    /*! No event */
    WPS_EVENT_NONE = 0,
    /*! There is an error on the WPS */
    WPS_EVENT_ERROR,
    /*! Connection event */
    WPS_EVENT_CONNECT,
    /*! Disconnection event */
    WPS_EVENT_DISCONNECT
} wps_event_t;

/** @brief WPS ranging mode enum definition.
 */
typedef enum wps_ranging_mode {
    /*! Ranging mode is disabled */
    WPS_RANGING_DISABLED = 0,
    /*! Ranging initiator mode without dedicated auto-reply connection */
    WPS_RANGING_STANDALONE_INITIATOR,
    /*! Ranging responder mode without dedicated auto-reply connection */
    WPS_RANGING_STANDALONE_RESPONDER,
    /*! Ranging initiator TX mode for main connection */
    WPS_RANGING_INITIATOR_TX,
    /*! Ranging initiator RX mode for auto-reply connection */
    WPS_RANGING_INITIATOR_RX,
    /*! Ranging responder TX mode for auto-reply connection*/
    WPS_RANGING_RESPONDER_TX,
    /*! Ranging responder RX mode for main connection */
    WPS_RANGING_RESPONDER_RX
} wps_ranging_mode_t;

/** @brief Phase information.
 */
typedef struct wps_phase_info {
    /*! Last Local phase info */
    phase_info_t last_local_phases_info;
    /*! Local phase info */
    phase_info_t local_phases_info;
    /*! Remote phase info */
    phase_info_t remote_phases_info;
    /*! Count to synchronize phase information */
    uint8_t local_phases_count;
    /*! Count to synchronize phase information */
    uint8_t remote_phases_count;
} wps_phase_info_t;

/** @brief WPS Connection
 */
struct wps_connection {
    /*! Source address */
    uint16_t source_address;
    /*! Destination address */
    uint16_t destination_address;

    /*! Frame size (only used if fixed frame size mode is enabled) */
    uint8_t payload_size;
    /*! WPS event */
    wps_event_t wps_event;
    /*! WPS error */
    wps_error_t wps_error;

    /* Layer 2 */
    /*! Ack received frame or expect ack when sending frame */
    bool ack_enable;
    /*! Expect an ACK frame containing only header data when no auto-reply connection exists */
    bool ack_frame_enable;
    /*! Phase information management module */
    link_phase_t link_phase;
    /*! Auto sync mode enable flag*/
    bool auto_sync_enable;
    /*! Header size in bytes */
    uint8_t header_size;
    /*! ACK frame header size in bytes */
    uint8_t ack_header_size;
    /*! Max time to delay the connection timeslot when connection queue is empty */
    int32_t empty_queue_max_delay;
    /*! Internal connection protocol */
    link_protocol_t link_protocol;
    /*! Internal auto-reply connection protocol */
    link_protocol_t *auto_link_protocol;
    /*! Stop and Wait (SaW) and Automatic Repeat Query (ARQ) */
    saw_arq_t stop_and_wait_arq;
    /*! Clear Channel Assessment */
    link_cca_t cca;
    /*! Fallback Module instance */
    link_fallback_t link_fallback;
    /*! Connection status */
    link_connect_status_t connect_status;
    /*! Connection priority */
    uint8_t priority;
    /*! Certification mode enable flag */
    bool certification_mode_enabled;
    /*! Ranging mode */
    wps_ranging_mode_t ranging_mode;
    /*! Credit flow control data */
    credit_flow_ctrl_t credit_flow_ctrl;
    /*! Flag to send sync frame when frame is available after connect event */
    bool first_tx_after_connect;
#if !WPS_DISABLE_FRAGMENTATION
    /*! Fragmentation instance */
    frag_t frag;
#endif

    /* Statistics */
#if WPS_ENABLE_PHY_STATS
    /*! Link quality indicator */
    lqi_t lqi;
#endif
#if WPS_ENABLE_STATS_USED_TIMESLOTS
    /*! WPS frames Link quality indicator (Excludes unused or sync* timeslots) */
    lqi_t used_frame_lqi;
#endif
#if WPS_ENABLE_PHY_STATS_PER_BANDS
    /*! Channel frames Link quality indicator */
    lqi_t *channel_lqi;
#endif
#if WPS_ENABLE_LINK_STATS
    /*! Wireless protocol stack statistics */
    wps_stats_t wps_stats;
#if WPS_ENABLE_PHY_STATS_PER_BANDS
    /*! Wireless protocol stack statistics per channel */
    wps_stats_t *wps_chan_stats;
#endif
#endif
    /*! Running total of CCA events */
    uint32_t total_cca_events;
    /*! Running total of individual CCA fails */
    uint32_t total_cca_fail_count;
    /*! Running total of tx fails due to CCA */
    uint32_t total_cca_tx_fail_count;
    /*! Running total of packets dropped */
    uint32_t total_pkt_dropped;

    /* Link throttle */
    /*! Current pattern array index count */
    uint8_t pattern_count;
    /*! Active timeslot ratio, in percent */
    uint8_t active_ratio;
    /*! Total pattern array count based on reduced ratio fraction */
    uint8_t pattern_total_count;
    /*! Connection currently enabled flag */
    bool currently_enabled;
    /*! Pattern array pointer, need to be allocated by application and initialized to 1 */
    bool *pattern;

    /*! Gain loop ,
     *   1D = Channel number
     *   2D = Radio number
     */
    gain_loop_t (*gain_loop)[WPS_RADIO_COUNT];

    /* Queue */
    /*! Xlayer free TX queue */
    xlayer_queue_t *free_tx_queue;
    /*! Xlayer free RX queue */
    xlayer_queue_t *free_rx_queue;
    /*! Cross layer queue */
    xlayer_queue_t xlayer_queue;
    /*! RX queue */
    xlayer_queue_t *rx_queue;
    /*! TX node */
    xlayer_queue_node_t *tx_node;
    /*! Data container for the TX communication */
    xlayer_circular_data_t *tx_data;
    /*! Data container for the RX communication */
    xlayer_circular_data_t *rx_data;

    /* Layer 1 */
    /*! Connection frame config */
    frame_cfg_t frame_cfg;
    /*! RF channel information,
     *   1D = Channel number
     *   2D = Radio number
     */
    rf_channel_t (*channel)[WPS_RADIO_COUNT];
    /*! RF fallback channel information
     *   1D = Fallback index number
     *   2D = Channel number
     *   3D = Radio number
     */
    rf_channel_t (**fallback_channel)[WPS_RADIO_COUNT];
    /*! Max number of different channel that the connection uses */
    uint8_t max_channel_count;
    /* Callback */
    /*! Function called by the wps to indicate the frame has been successfully transmitted */
    void (*tx_success_callback)(void *parg);
    /*! Function called by the wps to indicate the transmission failed */
    void (*tx_fail_callback)(void *parg);
    /*! Function called by the wps to indicate a frame is dropped */
    void (*tx_drop_callback)(void *parg);
    /*! Function called by the wps to indicate the frame has been received */
    void (*rx_success_callback)(void *parg);
    /*! Function called by the wps to indicate ranging data readiness */
    void (*ranging_data_ready_callback)(void *parg);
    /*! Function called by the wps to indicate that a WPS event appened */
    void (*evt_callback)(void *parg);

    /*! TX success callback void pointer argument. */
    void *tx_success_parg_callback;
    /*! TX fail callback void pointer argument. */
    void *tx_fail_parg_callback;
    /*! TX drop callback void pointer argument. */
    void *tx_drop_parg_callback;
    /*! RX success callback void pointer argument. */
    void *rx_success_parg_callback;
    /*! RX success callback void pointer argument. */
    void *ranging_data_ready_parg_callback;
    /*! Event callback void pointer argument. */
    void *evt_parg_callback;
    /*! Get free running timer tick */
    uint64_t (*get_tick)(void);
    /*! Tick frequency in Hertz. */
    uint32_t tick_frequency_hz;
    /*! Flush next packet in the wps tx queue */
    bool tx_flush;
    /*! Connection list node  */
    wps_connection_list_node_t conn_list_node;
    /*! Connection is main or auto reply */
    bool is_main;
};

/** @brief WPS role enumeration.
 */
typedef enum wps_role {
    /*! Coordinator dictate the time to the whole network */
    NETWORK_COORDINATOR = 0,
    /*! Node re-adjust its timer to stay in sync */
    NETWORK_NODE
} wps_role_t;

/** @brief Wireless Protocol Stack radio.
 *
 *  @note This is the parameter to setup one
 *        radio instance.
 */
typedef struct wps_radio {
    /*! Radio instance */
    radio_t radio;
    /*! Calibration variables */
    calib_vars_t *spectral_calib_vars;
    /*! NVM variables */
    nvm_t *nvm;
} wps_radio_t;

/** @brief Wireless Protocol Stack node configuration.
 */
typedef struct node_config {
    /*! Current node role : Coordinator or node */
    wps_role_t role;
    /*! Length of the preamble, in bits */
    uint32_t preamble_len;
    /*! Radio sleep level */
    sleep_lvl_t sleep_lvl;
    /*! Radio CRC polynomial */
    uint32_t crc_polynomial;
    /*! Node current address */
    uint16_t local_address;
    /*! Radio(s) configuration syncword */
    syncword_cfg_t syncword_cfg;
    /*! ISI mitigation level */
    isi_mitig_t isi_mitig;
    /*! Default radio RX gain */
    uint8_t rx_gain;
    /*! TX jitter enabled */
    bool tx_jitter_enabled;
    /*! Maximum frame lost duration before link is considered unsynced. */
    uint32_t frame_lost_max_duration;
} wps_node_cfg_t;

/** @brief WPS Node definition.
 *
 *  @note This is the parameters used to
 *        setup on node instance. One node
 *        can contains multiple radio.
 */
typedef struct node {
    /*! Wireless Protocol Stack radio */
    wps_radio_t *radio;
    /*! Node configuration */
    wps_node_cfg_t cfg;
    /*! Free TX xlayer_queue */
    xlayer_queue_t free_tx_queue;
    /*! Free RX xlayer_queue */
    xlayer_queue_t free_rx_queue;
    /*! Maximum frame size */
    uint8_t max_payload_size;
    /*! Maximum header size */
    uint8_t max_header_size;
    /*! Total node count in all TX connections queues */
    uint16_t tx_queues_size;
    /*! Total node count in all RX connections queues */
    uint16_t rx_queues_size;
    /*! Total data size required for all RX connections */
    uint32_t max_total_rx_data_size;
    /*! Linked list of connections */
    wps_connection_list_t conn_list;
    /*! Low power callback */
    void (*low_power_callback)(void *node);
    /*! Denotes whether low power mode can be activated */
    volatile bool low_power_allowed;
} wps_node_t;

/** @brief Received frame.
 */
typedef struct rx_frame {
    /*! Pointer to payload */
    uint8_t *payload;
    /*! Size of payload */
    uint16_t size;
} wps_rx_frame;

/** @brief Phase frame.
 */
typedef struct phase_frame {
    /*! Pointer to phase info data */
    phase_infos_t *payload;
    /*! Size of array of phase data */
    uint16_t size;
} wps_phase_frame;

/** @brief WPS schedule request configuration.
 */
typedef struct wps_schedule_ratio_cfg {
    /*! Target connection current active ratio */
    uint8_t active_ratio;
    /*! Target connection total pattern array size */
    uint8_t pattern_total_count;
    /*! Target connection pattern index count */
    uint8_t pattern_current_count;
    /*! Connection to change active timeslot ratio */
    wps_connection_t *target_conn;
    /*! Throttle configuration pattern */
    bool pattern_cfg[WPS_PATTERN_THROTTLE_GRANULARITY];
} wps_schedule_ratio_cfg_t;

/** @brief WPS request arrays structure configuration.
 */
typedef struct wps_request_config_info {
    /*! WPS schedule request structure array */
    wps_schedule_ratio_cfg_t *schedule_ratio_buffer;
    /*! WPS write request structure array */
    xlayer_write_request_info_t *write_request_buffer;
    /*! WPS read request structure array */
    xlayer_read_request_info_t *read_request_buffer;
    /*! WPS schedule request structure array size */
    uint8_t schedule_ratio_size;
    /*! WPS write request structure array size */
    uint8_t write_request_size;
    /*! WPS read request structure array size */
    uint8_t read_request_size;
} wps_request_config_info_t;

/** @brief WPS events callback.
 */
typedef void (*wps_callback_t)(void *parg);

#ifdef __cplusplus
}
#endif

#endif /* WPS_DEF_H */
