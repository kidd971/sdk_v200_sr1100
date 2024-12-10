/** @file  xlayer.h
 *  @brief SPARK cross layer queue.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef XLAYER_H
#define XLAYER_H

/* INCLUDES *******************************************************************/
#include "link_cca.h"
#include "link_gain_loop.h"
#include "link_phase.h"
#include "sr_def.h"
#include "sr_spectral.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Cross layer frame.
 */
typedef struct xlayer_frame {
    /* Layer 2 */
    /*! Source address */
    uint16_t source_address;
    /*! Destination address */
    uint16_t destination_address;

    /* Frame */
    /*! Header's buffer memory, point to index 0 */
    uint8_t *header_memory;
    /*! Header's begin iterator */
    uint8_t *header_begin_it;
    /*! Header's end iterator */
    uint8_t *header_end_it;
    /*! Payload's buffer memory, point to index 0 */
    uint8_t *payload_memory;
    /*! Payload's begin iterator */
    uint8_t *payload_begin_it;
    /*! Payload's end iterator */
    uint8_t *payload_end_it;
    /*! Frame's timestamps */
    uint64_t time_stamp;
    /*! Frame's retry count */
    uint16_t retry_count;
    /*! Header's buffer size */
    uint8_t header_memory_size;
    /*! Payload's buffer size */
    uint8_t payload_memory_size;
    /*! Denotes if frame payload is created from user memory space.
     *   Flag set to 'false', when payload memory is used from queue buffer
     */
    uint8_t user_payload;
    /*! Maximum possible frame buffer size */
    uint8_t max_frame_size;

    /*! Frame outcome */
    frame_outcome_t frame_outcome;
} xlayer_frame_t;

/** @brief Wireless protocol stack PHY Layer input signal.
 */
typedef enum reg_write_cfg {
    /*! PHY Radio IRQ signal */
    WRITE_ONCE = 0,
    /*! PHY DMA transfer complete signal */
    WRITE_PERIODIC,
} reg_write_cfg_t;

/** @brief xlayer request type enumeration.
 */
typedef enum xlayer_request {
    REQUEST_NONE = 0,                  /*! No request */
    REQUEST_MAC_CHANGE_SCHEDULE_RATIO, /*! Request allowing application to change active timeslot ratio */
    REQUEST_PHY_WRITE_REG,             /*! Request allowing application to write to a register */
    REQUEST_PHY_READ_REG,              /*! Request allowing application to read a register */
    REQUEST_PHY_DISCONNECT,            /*! Request to disconnect the wireless protocol stack */
} xlayer_request_t;

/** @brief xlayer request structure configuration.
 *
 *  @note Available choice for configuration structure are
 *      - xlayer_schedule_ratio_cfg_t (if link throttle is not disable)
 *      - xlayer_write_request_info_t
 *      - xlayer_read_request_info_t
 */
typedef struct xlayer_request_info {
    void *config;          /*! xlayer request structure configuration. */
    xlayer_request_t type; /*! xlayer request type enumeration */
} xlayer_request_info_t;

/** @brief xlayer write register request info structure.
 */
typedef struct xlayer_write_request_info {
    uint8_t target_register; /*! Target register to write data */
    uint16_t data;           /*! Data to send to the radio register */
    bool pending_request;    /*! Flag to notify that a request is pending */
    reg_write_cfg_t cfg;     /*! Write config */
} xlayer_write_request_info_t;

/** @brief xlayer read register request info structure.
 */
typedef struct xlayer_read_request_info {
    uint8_t target_register; /*! Target register to read. */
    uint16_t *rx_buffer;     /*! RX buffer containing register value */
    bool pending_request;    /*! Flag to notify that a request is pending */
    bool *xfer_cmplt;        /*! Bool to notify that read register is complete */
} xlayer_read_request_info_t;

/** @brief Cross layer callback structure.
 */
typedef struct xlayer_callback {
    /*! Function called when the frame is fully processed */
    void (*callback)(void *parg);
    /*! callback void pointer argument*/
    void *parg_callback;
} xlayer_callback_t;

/** @brief Cross layer configuration - internal xlayer use.
 */
typedef struct xlayer_cfg_internal {
    /* Layer 2 */
    /*! Expect ACK? */
    bool expect_ack;

    /* Layer 1 */
    /*! modulation */
    modulation_t modulation;
    /*! Chip repetition */
    chip_repetition_t chip_repet;
    /*! FEC level */
    fec_level_t fec;
    /*! Current channel information */
    rf_channel_t *channel;
    /*! Gain loop */
    gain_loop_t *gain_loop;
    /*! Power up delay */
    uint16_t power_up_delay;
    /*! RX timeout */
    uint16_t rx_timeout;
    /*! Sleep time in PLL cycles */
    uint32_t sleep_time;
    /*! RX wait time */
    uint16_t rx_wait_time;
    /*! Receiver constant gain */
    uint8_t rx_constgain;
    /*! Clear Channel Assessment threshold */
    uint8_t cca_threshold;
    /*! CCA on time (SR1120 only)*/
    uint8_t cca_on_time;
    /*! CCA retry time */
    uint16_t cca_retry_time;
    /*! CCA max try count */
    uint8_t cca_max_try_count;
    /*! Phase offset */
    uint8_t phase_offset[PHASE_OFFSET_BYTE_COUNT];
    /*! CCA fail action */
    cca_fail_action_t cca_fail_action;
    /*! CCA try count */
    uint8_t cca_try_count;
    /*! RNSI in 1/10 dB */
    uint32_t rnsi_raw;
    /*! RSSI in 1/10 dB */
    uint32_t rssi_raw;
    /*! Sleep Level */
    sleep_lvl_t sleep_level;
    /*! Next cycle sleep Level */
    sleep_lvl_t next_sleep_level;
    /*! phase info */
    phase_info_t *phases_info;
    /*! RX CCA retry count */
    uint8_t rx_cca_retry_count;
    /*! ISI mitigation level */
    isi_mitig_t isi_mitig;
    /*! Certification header usage flag */
    bool certification_header_en;
    /*! Max expected header size */
    uint8_t expected_header_size;
    /*! Max expected payload size */
    uint8_t expected_payload_size;
    /*! WPS write request structure array */
    xlayer_write_request_info_t *write_request_buffer;
    /*! WPS read request structure array */
    xlayer_read_request_info_t *read_request_buffer;
    /* Callback */
    /*! Main connection callback */
    xlayer_callback_t callback_main;
    /*! Main connection callback */
    xlayer_callback_t callback_auto;

    /*! Handle for the data buffer update function from the MAC layer */
    void (*update_payload_buffer)(void *mac, xlayer_frame_t *frame, uint8_t requested_size);
} xlayer_cfg_internal_t;

/** @brief Cross layer configuration.
 */
typedef struct xlayer_cfg {

    /*! RNSI in 1/10 dB */
    uint32_t rnsi_raw;
    /*! RSSI in 1/10 dB */
    uint32_t rssi_raw;
    /*! phase info */
    phase_info_t phases_info;

} xlayer_cfg_t;

/** @brief Cross layer.
 */
typedef struct xlayer {
    /*! Configuration */
    xlayer_cfg_t config;
    /*! Frame */
    xlayer_frame_t frame;
} xlayer_t;

#ifdef __cplusplus
}
#endif

#endif /* XLAYER_H */
