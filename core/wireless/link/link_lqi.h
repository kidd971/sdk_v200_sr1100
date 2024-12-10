/** @file link_lqi.h
 *  @brief Link Quality Indicator module.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef LINK_LQI_H_
#define LINK_LQI_H_

/* INCLUDES *******************************************************************/
#include <stdint.h>
#include <string.h>
#include "link_gain_loop.h"
#include "link_utils.h"
#include "wps_config.h"
#if SR1000 /* Macro needs wps_config.h */
#include "sr1000_def.h"
#elif SR1100
#include "sr1100_def.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
typedef enum lqi_mode {
    /*! Consider lost and rejected frames as having weakest RSSI possible and typical RNSI */
    LQI_MODE_0,
    /*! Don't consider rejected and lost frames in RSSI and RNSI calculation */
    LQI_MODE_1,
} lqi_mode_t;

typedef struct lqi {
    /*! LQI object mode */
    lqi_mode_t mode;
    /*! RSSI in tenths of dB */
    uint64_t rssi_total_tenth_db;
    /*! RNSI in tenths of dB */
    uint64_t rnsi_total_tenth_db;
    /*! RSSI total */
    uint64_t rssi_total;
    /*! RNSI total */
    uint64_t rnsi_total;
    /*! Sent frame count */
    uint32_t sent_count;
    /*! ACKed frame count */
    uint32_t ack_count;
    /*! NACKed frame count */
    uint32_t nack_count;
    /*! Received frame count */
    uint32_t received_count;
    /*! Rejected frame count */
    uint32_t rejected_count;
    /*! Lost frame count */
    uint32_t lost_count;
    /*! Total frame count */
    uint32_t total_count;
    /*! Instantaneous RSSI measurement */
    uint8_t inst_rssi;
    /*! Instantaneous RNSI measurement */
    uint8_t inst_rnsi;
    /*! Instantaneous RSSI measurement tenths of dB */
    uint16_t inst_rssi_tenth_db;
    /*! Instantaneous RNSI measurement tenths of dB */
    uint16_t inst_rnsi_tenth_db;
#if SR1100
    /*! Instantaneous phase offset data */
    uint8_t inst_phase_offset[PHASE_OFFSET_BYTE_COUNT];
#endif
} lqi_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize LQI object.
 *
 *  @param[in] lqi  LQI object.
 *  @param[in] mode LQI mode.
 *  @return None.
 */
void link_lqi_init(lqi_t *lqi, lqi_mode_t mode);

/** @brief Get sent frame count.
 *
 *  @param[in] lqi  LQI object.
 *  @return Sent frame count.
 */
static inline uint32_t link_lqi_get_sent_count(lqi_t *lqi)
{
    return lqi->sent_count;
}

/** @brief Get acked frame count.
 *
 *  @param[in] lqi  LQI object.
 *  @return ACKed frame count.
 */
static inline uint32_t link_lqi_get_ack_count(lqi_t *lqi)
{
    return lqi->ack_count;
}

/** @brief Get nacked frame count.
 *
 *  @param[in] lqi  LQI object.
 *  @return NACKed frame count.
 */
static inline uint32_t link_lqi_get_nack_count(lqi_t *lqi)
{
    return lqi->nack_count;
}

/** @brief Get received frame count.
 *
 *  @param[in] lqi  LQI object.
 *  @return Received frame count.
 */
static inline uint32_t link_lqi_get_received_count(lqi_t *lqi)
{
    return lqi->received_count;
}

/** @brief Get rejected frame count.
 *
 *  @param[in] lqi  LQI object.
 *  @return Rejected frame count.
 */
static inline uint32_t link_lqi_get_rejected_count(lqi_t *lqi)
{
    return lqi->rejected_count;
}

/** @brief Get lost frame count.
 *
 *  @param[in] lqi  LQI object.
 *  @return Lost frame count.
 */
static inline uint32_t link_lqi_get_lost_count(lqi_t *lqi)
{
    return lqi->lost_count;
}

/** @brief Get total frame count.
 *
 *  @param[in] lqi  LQI object.
 *  @return Total frame count.
 */
static inline uint32_t link_lqi_get_total_count(lqi_t *lqi)
{
    return lqi->total_count;
}

/** @brief Get RSSI average.
 *
 *  @param[in] lqi  LQI object.
 *  @return RSSI average.
 */
uint16_t link_lqi_get_avg_rssi_tenth_db(lqi_t *lqi);

/** @brief Get RNSI average.
 *
 *  @param[in] lqi  LQI object.
 *  @return RNSI average.
 */
uint16_t link_lqi_get_avg_rnsi_tenth_db(lqi_t *lqi);

/** @brief Get RSSI average raw.
 *
 *  @param[in] lqi  LQI object.
 *  @return RSSI average.
 */
uint16_t link_lqi_get_avg_rssi_raw(lqi_t *lqi);

/** @brief Get RNSI average raw.
 *
 *  @param[in] lqi  LQI object.
 *  @return RNSI average.
 */
uint16_t link_lqi_get_avg_rnsi_raw(lqi_t *lqi);

/** @brief Get the last received RNSI measurement.
 *
 *  @param[in] lqi  LQI object.
 *  @return Last received RNSI measurement.
 */
static inline uint16_t link_lqi_get_inst_rnsi(lqi_t *lqi)
{
    return lqi->inst_rnsi;
}

/** @brief Get the last received RNSI measurement in tenths of dB.
 *
 *  @param[in] lqi  LQI object.
 *  @return Last received RNSI measurement in tenths of dB.
 */
static inline uint16_t link_lqi_get_inst_rnsi_tenth_db(lqi_t *lqi)
{
    return lqi->inst_rnsi_tenth_db;
}

/** @brief Get the last received RSSI measurement.
 *
 *  @param[in] lqi  LQI object.
 *  @return Last received RSSI measurement.
 */
static inline uint16_t link_lqi_get_inst_rssi(lqi_t *lqi)
{
    return lqi->inst_rssi;
}

/** @brief Get the last received RSSI measurement in tenths of dB.
 *
 *  @param[in] lqi  LQI object.
 *  @return Last received RSSI measurement in tenths of dB.
 */
static inline uint16_t link_lqi_get_inst_rssi_tenth_db(lqi_t *lqi)
{
    return lqi->inst_rssi_tenth_db;
}

/** @brief Get instantaneous phase offset data (SR11XX feature only).
 *
 *  @param[in] lqi    LQI object.
 *  @param[in] index  Index.
 *  @return Phase offset data.
 */
static inline uint8_t link_lqi_get_inst_phase_offset(lqi_t *lqi, uint8_t index)
{
#if SR1100
    return lqi->inst_phase_offset[index];
#else
    (void)lqi;
    (void)index;

    return 0;
#endif
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Reset LQI object.
 *
 *  @param[in] lqi  LQI object.
 */
static inline void link_lqi_reset(lqi_t *lqi)
{
    link_lqi_init(lqi, lqi->mode); /* Keep the mode when reseting. */
}

/** @brief Update LQI.
 *
 *  @param[in] lqi            LQI object.
 *  @param[in] gain_index     Gain index.
 *  @param[in] frame_outcome  Outcome of the frame.
 *  @param[in] rssi           Receiver signal strength indicator.
 *  @param[in] rnsi           Receiver noise strength indicator.
 */
void link_lqi_update(lqi_t *lqi, uint8_t gain_index, frame_outcome_t frame_outcome, uint8_t rssi,
                     uint8_t rnsi, uint8_t *phase_offset);

#ifdef __cplusplus
}
#endif

#endif /* LINK_LQI_H_ */
