/** @file link_lqi.c
 *  @brief Link Quality Indicator module.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "link_lqi.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void link_lqi_init(lqi_t *lqi, lqi_mode_t mode)
{
    memset(lqi, 0, sizeof(lqi_t));
    lqi->mode = mode;
}

uint16_t link_lqi_get_avg_rssi_tenth_db(lqi_t *lqi)
{
    uint32_t count = (lqi->mode == LQI_MODE_1) ? lqi->received_count : lqi->total_count;

    if (count == 0) {
        return 0;
    }

    return lqi->rssi_total_tenth_db / count;
}

uint16_t link_lqi_get_avg_rnsi_tenth_db(lqi_t *lqi)
{
    uint32_t count = (lqi->mode == LQI_MODE_1) ? lqi->received_count : lqi->total_count;

    if (count == 0) {
        return 0;
    }

    return lqi->rnsi_total_tenth_db / count;
}

uint16_t link_lqi_get_avg_rssi_raw(lqi_t *lqi)
{
    uint32_t count = (lqi->mode == LQI_MODE_1) ? lqi->received_count : lqi->total_count;

    if (count == 0) {
        return 0;
    }

    return lqi->rssi_total / count;
}

uint16_t link_lqi_get_avg_rnsi_raw(lqi_t *lqi)
{
    uint32_t count = (lqi->mode == LQI_MODE_1) ? lqi->received_count : lqi->total_count;

    if (count == 0) {
        return 0;
    }

    return lqi->rnsi_total / count;
}

/* PRIVATE FUNCTIONS **********************************************************/
void link_lqi_update(lqi_t *lqi, uint8_t gain_index, frame_outcome_t frame_outcome, uint8_t rssi,
                     uint8_t rnsi, uint8_t *phase_offset)
{
    /* Total count overflow */
    if (++lqi->total_count == 0) {
        link_lqi_reset(lqi);
        return;
    }
#if SR1100
    /* FIXME:
     * This is a hardcoded value while ASIC teams figures out a way to fix the
     * RNSI reading from the radio.
     */
    rnsi = 85;
#endif

    switch (frame_outcome) {
    case FRAME_RECEIVED:
        lqi->received_count++;
        lqi->inst_rnsi = rnsi;
        lqi->inst_rssi = rssi;
#if SR1100
        if (lqi->mode == LQI_MODE_1) {
            lqi->inst_rnsi_tenth_db =
                calculate_normalized_gain(link_gain_loop_get_min_tenth_db(gain_index), rnsi) -
                link_gain_loop_get_rnsi_tenth_db(gain_index);
        } else {
            lqi->inst_rnsi_tenth_db =
                calculate_normalized_gain(link_gain_loop_get_min_tenth_db(gain_index), rnsi);
        }
#else
        lqi->inst_rnsi_tenth_db =
            calculate_normalized_gain(link_gain_loop_get_min_tenth_db(gain_index), rnsi);
#endif
        lqi->inst_rssi_tenth_db =
            calculate_normalized_gain(link_gain_loop_get_min_tenth_db(gain_index), rssi);
        lqi->rssi_total_tenth_db += lqi->inst_rssi_tenth_db;
        lqi->rnsi_total_tenth_db += lqi->inst_rnsi_tenth_db;
        lqi->rssi_total += rssi;
        lqi->rnsi_total += rnsi;
#if SR1100
        for (uint8_t i = 0; i < PHASE_OFFSET_BYTE_COUNT; i++) {
            lqi->inst_phase_offset[i] = phase_offset[i];
        }
#else
        (void)phase_offset;
#endif
        break;
    case FRAME_REJECTED:
        lqi->rejected_count++;
        if (lqi->mode == LQI_MODE_0) {
            lqi->rssi_total_tenth_db += link_gain_loop_get_min_tenth_db(gain_index);
            lqi->rnsi_total_tenth_db += link_gain_loop_get_rnsi_tenth_db(gain_index);
        }
        break;
    case FRAME_LOST:
        lqi->lost_count++;
        if (lqi->mode == LQI_MODE_0) {
            lqi->rssi_total_tenth_db += link_gain_loop_get_min_tenth_db(gain_index);
            lqi->rnsi_total_tenth_db += link_gain_loop_get_rnsi_tenth_db(gain_index);
        }
        break;
    case FRAME_SENT_ACK:
        lqi->sent_count++;
        lqi->ack_count++;
        lqi->received_count++;
        lqi->inst_rnsi = rnsi;
        lqi->inst_rssi = rssi;
#if SR1100
        if (lqi->mode == LQI_MODE_1) {
            lqi->inst_rnsi_tenth_db =
                calculate_normalized_gain(link_gain_loop_get_min_tenth_db(gain_index), rnsi) -
                link_gain_loop_get_rnsi_tenth_db(gain_index);
        } else {
            lqi->inst_rnsi_tenth_db =
                calculate_normalized_gain(link_gain_loop_get_min_tenth_db(gain_index), rnsi);
        }
#else
        lqi->inst_rnsi_tenth_db =
            calculate_normalized_gain(link_gain_loop_get_min_tenth_db(gain_index), rnsi);
#endif
        lqi->inst_rssi_tenth_db =
            calculate_normalized_gain(link_gain_loop_get_min_tenth_db(gain_index), rssi);
        lqi->rssi_total_tenth_db += lqi->inst_rssi_tenth_db;
        lqi->rnsi_total_tenth_db += lqi->inst_rnsi_tenth_db;
        lqi->rssi_total += rssi;
        lqi->rnsi_total += rnsi;
        break;
    case FRAME_SENT_ACK_LOST:
    case FRAME_SENT_ACK_REJECTED:
        lqi->sent_count++;
        lqi->nack_count++;
        if (lqi->mode == LQI_MODE_0) {
            lqi->rssi_total_tenth_db += link_gain_loop_get_min_tenth_db(gain_index);
            lqi->rnsi_total_tenth_db += link_gain_loop_get_rnsi_tenth_db(gain_index);
        }
        break;
    case FRAME_WAIT:
        lqi->sent_count++;
        break;
    }
}
