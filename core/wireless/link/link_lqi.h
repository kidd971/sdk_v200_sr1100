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

/* CONSTANTS ******************************************************************/
/*! Size of the RSSI and RNSI block average. */
#ifndef RSSI_RNSI_BLOCK_AVG_MAX_COUNT
#define RSSI_RNSI_BLOCK_AVG_MAX_COUNT 100
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

    /*! Instantaneous RSSI code from the radio register. */
    uint8_t rssi_inst_raw;
    /*! Instantaneous RNSI code from the radio register. */
    uint8_t rnsi_inst_raw;
    /*! Sum of all instantaneous RSSI codes. */
    uint64_t rssi_sum_raw;
    /*! Sum of all instantaneous RNSI codes. */
    uint64_t rnsi_sum_raw;

    /*! Instantaneous RSSI value represented in tenths of dB. */
    uint16_t rssi_inst_tenth_db;
    /*! Instantaneous RNSI value represented in tenths of dB. */
    uint16_t rnsi_inst_tenth_db;
    /*! Sum of all RSSI values in tenth of dB. */
    uint64_t rssi_sum_tenth_db;
    /*! Sum of all RNSI values in tenth of dB. */
    uint64_t rnsi_sum_tenth_db;

    /*! Sum of RSSI block values in tenths of dB. */
    uint32_t rssi_block_sum_tenth_db;
    /*! Sum of RNSI block values in tenths of dB. */
    uint32_t rnsi_block_sum_tenth_db;
    /*! Number of values included in the sum of the RSSI block values. */
    uint8_t rssi_block_count;
    /*! Number of values included in the sum of the RNSI block values. */
    uint8_t rnsi_block_count;
    /*! RSSI block average in tenths of dB. */
    uint16_t rssi_block_avg_tenth_db;
    /*! RNSI block average in tenths of dB. */
    uint16_t rnsi_block_avg_tenth_db;

#if SR1100
    /*! Instantaneous phase offset data */
    uint8_t inst_phase_offset[PHASE_OFFSET_BYTE_COUNT];
    /*! Phase number */
    uint8_t phase_number;
    /*! Channel impulse response sum for average. */
    uint32_t cir_sum;
#endif
} lqi_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize LQI object.
 *
 *  @param[in] lqi  Pointer to the LQI (Link Quality Indicator) object.
 *  @param[in] mode LQI mode.
 *  @return None.
 */
void link_lqi_init(lqi_t *lqi, lqi_mode_t mode);

/** @brief Get sent frame count.
 *
 *  @param[in] lqi  Pointer to the LQI (Link Quality Indicator) object.
 *  @return Sent frame count.
 */
static inline uint32_t link_lqi_get_sent_count(lqi_t *lqi)
{
    return lqi->sent_count;
}

/** @brief Get acked frame count.
 *
 *  @param[in] lqi  Pointer to the LQI (Link Quality Indicator) object.
 *  @return ACKed frame count.
 */
static inline uint32_t link_lqi_get_ack_count(lqi_t *lqi)
{
    return lqi->ack_count;
}

/** @brief Get nacked frame count.
 *
 *  @param[in] lqi  Pointer to the LQI (Link Quality Indicator) object.
 *  @return NACKed frame count.
 */
static inline uint32_t link_lqi_get_nack_count(lqi_t *lqi)
{
    return lqi->nack_count;
}

/** @brief Get received frame count.
 *
 *  @param[in] lqi  Pointer to the LQI (Link Quality Indicator) object.
 *  @return Received frame count.
 */
static inline uint32_t link_lqi_get_received_count(lqi_t *lqi)
{
    return lqi->received_count;
}

/** @brief Get rejected frame count.
 *
 *  @param[in] lqi  Pointer to the LQI (Link Quality Indicator) object.
 *  @return Rejected frame count.
 */
static inline uint32_t link_lqi_get_rejected_count(lqi_t *lqi)
{
    return lqi->rejected_count;
}

/** @brief Get lost frame count.
 *
 *  @param[in] lqi  Pointer to the LQI (Link Quality Indicator) object.
 *  @return Lost frame count.
 */
static inline uint32_t link_lqi_get_lost_count(lqi_t *lqi)
{
    return lqi->lost_count;
}

/** @brief Get total frame count.
 *
 *  @param[in] lqi  Pointer to the LQI (Link Quality Indicator) object.
 *  @return Total frame count.
 */
static inline uint32_t link_lqi_get_total_count(lqi_t *lqi)
{
    return lqi->total_count;
}

/** @brief Get RSSI average in tenths of dB.
 *
 *  @param[in] lqi  Pointer to the LQI (Link Quality Indicator) object.
 *  @return RSSI average in tenths of dB.
 */
uint16_t link_lqi_get_avg_rssi_tenth_db(lqi_t *lqi);

/** @brief Get RNSI average in tenths of dB.
 *
 *  @param[in] lqi  Pointer to the LQI (Link Quality Indicator) object.
 *  @return RNSI average in tenths of dB.
 */
uint16_t link_lqi_get_avg_rnsi_tenth_db(lqi_t *lqi);

/** @brief Get RSSI average raw.
 *
 *  @note Raw values are hardware-specific codes that require conversion to human-readable units (e.g., tenths of dB)
 *        for interpretation. The average is calculated based on all received RSSI values.
 *
 *  @param[in] lqi  Pointer to the LQI (Link Quality Indicator) object.
 *  @return RSSI average represented in radio-specific code (raw value).
 */
uint16_t link_lqi_get_avg_rssi_raw(lqi_t *lqi);

/** @brief Get RNSI average raw.
 *
 *  @note Raw values are hardware-specific codes that require conversion to human-readable units (e.g., tenths of dB)
 *        for interpretation. The average is calculated based on all received RNSI values.
 *
 *  @param[in] lqi  Pointer to the LQI (Link Quality Indicator) object.
 *  @return RNSI average represented in radio-specific code (raw value).
 */
uint16_t link_lqi_get_avg_rnsi_raw(lqi_t *lqi);

/** @brief Get the last received RNSI measurement.
 *
 *  @param[in] lqi  Pointer to the LQI (Link Quality Indicator) object.
 *  @return Last received RNSI measurement represented in radio-specific code (raw value).
 */
static inline uint16_t link_lqi_get_inst_rnsi(lqi_t *lqi)
{
    return lqi->rnsi_inst_raw;
}

/** @brief Get the last received RNSI measurement in tenths of dB.
 *
 *  @param[in] lqi  Pointer to the LQI (Link Quality Indicator) object.
 *  @return Last received RNSI measurement in tenths of dB.
 */
static inline uint16_t link_lqi_get_inst_rnsi_tenth_db(lqi_t *lqi)
{
    return lqi->rnsi_inst_tenth_db;
}

/** @brief Get the last received RSSI measurement.
 *
 *  @param[in] lqi  Pointer to the LQI (Link Quality Indicator) object.
 *  @return Last received RSSI measurement represented in radio-specific code (raw value).
 */
static inline uint16_t link_lqi_get_inst_rssi(lqi_t *lqi)
{
    return lqi->rssi_inst_raw;
}

/** @brief Get the last received RSSI measurement in tenths of dB.
 *
 *  @param[in] lqi  Pointer to the LQI (Link Quality Indicator) object.
 *  @return Last received RSSI measurement in tenths of dB.
 */
static inline uint16_t link_lqi_get_inst_rssi_tenth_db(lqi_t *lqi)
{
    return lqi->rssi_inst_tenth_db;
}

/** @brief Get the last computed RSSI block average in tenths of dB.
 *
 *  @param[in] lqi  Pointer to the LQI (Link Quality Indicator) object.
 *  @return RSSI block average in tenths of dB.
 */
static inline uint16_t link_lqi_get_rssi_block_avg_tenth_db(lqi_t *lqi)
{
    return lqi->rssi_block_avg_tenth_db;
}

/** @brief Get the last computed RNSI block average in tenths of dB.
 *
 *  @param[in] lqi  Pointer to the LQI (Link Quality Indicator) object.
 *  @return RNSI block average in tenths of dB.
 */
static inline uint16_t link_lqi_get_rnsi_block_avg_tenth_db(lqi_t *lqi)
{
    return lqi->rnsi_block_avg_tenth_db;
}

/** @brief Get instantaneous phase offset data (SR11XX feature only).
 *
 *  @param[in] lqi    Pointer to the LQI (Link Quality Indicator) object.
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

/** @brief Calculate and update the RSSI block average in tenths of dB if the block size limit is reached.
 *
 *  @param[in] lqi  Pointer to the LQI (Link Quality Indicator) object that holds RSSI data.
 */
static inline void link_lqi_calculate_rssi_block_avg_tenth_db(lqi_t *lqi)
{
    if (lqi->rssi_block_count >= RSSI_RNSI_BLOCK_AVG_MAX_COUNT) {
        lqi->rssi_block_avg_tenth_db = lqi->rssi_block_sum_tenth_db / lqi->rssi_block_count;
        lqi->rssi_block_count = 0;
        lqi->rssi_block_sum_tenth_db = 0;
    }
}

/** @brief Calculate and update the RNSI block average in tenths of dB if the block size limit is reached.
 *
 *  @param[in] lqi  Pointer to the LQI (Link Quality Indicator) object that holds RNSI data.
 */
static inline void link_lqi_calculate_rnsi_block_avg_tenth_db(lqi_t *lqi)
{
    if (lqi->rnsi_block_count >= RSSI_RNSI_BLOCK_AVG_MAX_COUNT) {
        lqi->rnsi_block_avg_tenth_db = lqi->rnsi_block_sum_tenth_db / lqi->rnsi_block_count;
        lqi->rnsi_block_count = 0;
        lqi->rnsi_block_sum_tenth_db = 0;
    }
}

/** @brief Reset the LQI object while preserving the mode.
 *
 * This function resets all fields of the given LQI structure to their default values, except for the mode, which
 * remains unchanged.
 *
 *  @param[in] lqi  Pointer to the LQI (Link Quality Indicator) object to reset.
 */
static inline void link_lqi_reset(lqi_t *lqi)
{
    /* Reset all the LQI structure except for the mode. */
    link_lqi_init(lqi, lqi->mode);
}

/** @brief Update Link Quality Indicator (LQI).
 *
 *  @param[in] lqi                 Pointer to the LQI (Link Quality Indicator) object.
 *  @param[in] gain_index          Gain index.
 *  @param[in] frame_outcome       Outcome of the frame.
 *  @param[in] rssi                Receiver signal strength indicator.
 *  @param[in] rnsi                Receiver noise strength indicator.
 *  @param[in] phase_offset        Phase offset data array for CIR computation.
 *  @param[in] phase_offset_count  Number of phase depending on ISI Mitigation level.
 */
void link_lqi_update(lqi_t *lqi, uint8_t gain_index, frame_outcome_t frame_outcome, uint8_t rssi, uint8_t rnsi,
                     uint8_t *phase_offset, uint8_t phase_offset_count);

#ifdef __cplusplus
}
#endif

#endif /* LINK_LQI_H_ */
