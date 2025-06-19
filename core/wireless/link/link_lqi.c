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

    return lqi->rssi_sum_tenth_db / count;
}

uint16_t link_lqi_get_avg_rnsi_tenth_db(lqi_t *lqi)
{
    uint32_t count = (lqi->mode == LQI_MODE_1) ? lqi->received_count : lqi->total_count;

    if (count == 0) {
        return 0;
    }

    return lqi->rnsi_sum_tenth_db / count;
}

uint16_t link_lqi_get_avg_rssi_raw(lqi_t *lqi)
{
    uint32_t count = (lqi->mode == LQI_MODE_1) ? lqi->received_count : lqi->total_count;

    if (count == 0) {
        return 0;
    }

    return lqi->rssi_sum_raw / count;
}

uint16_t link_lqi_get_avg_rnsi_raw(lqi_t *lqi)
{
    uint32_t count = (lqi->mode == LQI_MODE_1) ? lqi->received_count : lqi->total_count;

    if (count == 0) {
        return 0;
    }

    return lqi->rnsi_sum_raw / count;
}

void link_lqi_update(lqi_t *lqi, uint8_t gain_index, frame_outcome_t frame_outcome, uint8_t rssi, uint8_t rnsi,
                     uint8_t *phase_offset, uint8_t phase_offset_count)
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
        /* Update the instantaneous RSSI and RNSI codes from the radio register. */
        lqi->rnsi_inst_raw = rnsi;
        lqi->rssi_inst_raw = rssi;

        /* Update the instantaneous RNSI value represented in tenths of dB.*/
#if SR1100
        if (lqi->mode == LQI_MODE_1) {
            lqi->rnsi_inst_tenth_db = calculate_normalized_gain(link_gain_loop_get_min_tenth_db(gain_index), rnsi) -
                                      link_gain_loop_get_rnsi_tenth_db(gain_index);
        } else {
            lqi->rnsi_inst_tenth_db = calculate_normalized_gain(link_gain_loop_get_min_tenth_db(gain_index), rnsi);
        }
#else
        lqi->rnsi_inst_tenth_db = calculate_normalized_gain(link_gain_loop_get_min_tenth_db(gain_index), rnsi);
#endif
        /* Update the total sum of intant RNSI values represented in tenths of dB. */
        lqi->rnsi_sum_tenth_db += lqi->rnsi_inst_tenth_db;
        /* Update the sum for the next RSSI block average. */
        lqi->rnsi_block_sum_tenth_db += lqi->rnsi_inst_tenth_db;
        lqi->rnsi_block_count++;

        /* Calculate the block average RNSI when the block size is reached. */
        link_lqi_calculate_rnsi_block_avg_tenth_db(lqi);

        /* Update the instantaneous RSSI value represented in tenths of dB.*/
        lqi->rssi_inst_tenth_db = calculate_normalized_gain(link_gain_loop_get_min_tenth_db(gain_index), rssi);

        /* Update the total sum of RSSI value represented in tenths of dB. */
        lqi->rssi_sum_tenth_db += lqi->rssi_inst_tenth_db;
        /* Update the sum for the next RSSI block average. */
        lqi->rssi_block_sum_tenth_db += lqi->rssi_inst_tenth_db;
        lqi->rssi_block_count++;

        /* Calculate the block average RSSI when the block size is reached. */
        link_lqi_calculate_rssi_block_avg_tenth_db(lqi);

        /* Update the sum of all RSSI and RNSI codes. */
        lqi->rssi_sum_raw += rssi;
        lqi->rnsi_sum_raw += rnsi;

#if SR1100 && WPS_ENABLE_PHY_STATS_PER_BANDS
        for (uint8_t i = 0; i < PHASE_OFFSET_BYTE_COUNT; i++) {
            lqi->inst_phase_offset[i] = phase_offset[i];
        }
        lqi->phase_number = phase_offset_count;
        lqi->cir_sum += calculate_cir(phase_offset, phase_offset_count);
#else
        (void)phase_offset;
        (void)phase_offset_count;
#endif
        break;
    case FRAME_REJECTED:
        lqi->rejected_count++;
        /* Update the instantaneous RSSI and RNSI codes from the radio register. */
        lqi->rnsi_inst_raw = rnsi;
        lqi->rssi_inst_raw = rssi;

        /* Update the instantaneous RNSI value represented in tenths of dB.*/
#if SR1100
        if (lqi->mode == LQI_MODE_1) {
            lqi->rnsi_inst_tenth_db = calculate_normalized_gain(link_gain_loop_get_min_tenth_db(gain_index), rnsi) -
                                      link_gain_loop_get_rnsi_tenth_db(gain_index);
        } else {
            lqi->rnsi_inst_tenth_db = calculate_normalized_gain(link_gain_loop_get_min_tenth_db(gain_index), rnsi);
        }
#else
        lqi->rnsi_inst_tenth_db = calculate_normalized_gain(link_gain_loop_get_min_tenth_db(gain_index), rnsi);
#endif
        /* Update the total sum of intant RNSI values represented in tenths of dB. */
        lqi->rnsi_sum_tenth_db += lqi->rnsi_inst_tenth_db;
        /* Update the sum for the next RSSI block average. */
        lqi->rnsi_block_sum_tenth_db += lqi->rnsi_inst_tenth_db;
        lqi->rnsi_block_count++;

        /* Calculate the block average RNSI when the block size is reached. */
        link_lqi_calculate_rnsi_block_avg_tenth_db(lqi);

        /* Update the instantaneous RSSI value represented in tenths of dB.*/
        lqi->rssi_inst_tenth_db = calculate_normalized_gain(link_gain_loop_get_min_tenth_db(gain_index), rssi);

        /* Update the total sum of RSSI value represented in tenths of dB. */
        lqi->rssi_sum_tenth_db += lqi->rssi_inst_tenth_db;
        /* Update the sum for the next RSSI block average. */
        lqi->rssi_block_sum_tenth_db += lqi->rssi_inst_tenth_db;
        lqi->rssi_block_count++;

        /* Calculate the block average RSSI when the block size is reached. */
        link_lqi_calculate_rssi_block_avg_tenth_db(lqi);

        /* Update the sum of all RSSI and RNSI codes. */
        lqi->rssi_sum_raw += rssi;
        lqi->rnsi_sum_raw += rnsi;
        break;
    case FRAME_LOST:
        lqi->lost_count++;
        if (lqi->mode == LQI_MODE_0) {
            /* Without a received frame, RNSI cannot be measured, the typical value is used. */
            lqi->rnsi_inst_tenth_db = link_gain_loop_get_rnsi_tenth_db(gain_index);
            /* Without a received frame, RSSI cannot be measured, the minimum strength is used. */
            lqi->rssi_inst_tenth_db = link_gain_loop_get_min_tenth_db(gain_index);

            /* Update the total sum of intant RSSI and RNSI values represented in tenths of dB. */
            lqi->rnsi_sum_tenth_db += lqi->rnsi_inst_tenth_db;
            lqi->rssi_sum_tenth_db += lqi->rssi_inst_tenth_db;

            /* Update the sum for the next RSSI and RNSI block average and increment their counts. */
            lqi->rnsi_block_sum_tenth_db += lqi->rnsi_inst_tenth_db;
            lqi->rssi_block_sum_tenth_db += lqi->rssi_inst_tenth_db;
            lqi->rnsi_block_count++;
            lqi->rssi_block_count++;

            /* Calculate the block average of the RSSI and RNSI when the block size is reached. */
            link_lqi_calculate_rnsi_block_avg_tenth_db(lqi);
            link_lqi_calculate_rssi_block_avg_tenth_db(lqi);
        }
        break;
    case FRAME_SENT_ACK:
        lqi->sent_count++;
        lqi->ack_count++;
        lqi->received_count++;
        /* Update the instantaneous RSSI and RNSI codes from the radio register. */
        lqi->rnsi_inst_raw = rnsi;
        lqi->rssi_inst_raw = rssi;

        /* Update the instantaneous RNSI value represented in tenths of dB.*/
#if SR1100
#if WPS_ENABLE_PHY_STATS_PER_BANDS
        for (uint8_t i = 0; i < PHASE_OFFSET_BYTE_COUNT; i++) {
            lqi->inst_phase_offset[i] = phase_offset[i];
        }
        lqi->phase_number = phase_offset_count;
        lqi->cir_sum += calculate_cir(phase_offset, phase_offset_count);
#endif
        if (lqi->mode == LQI_MODE_1) {
            lqi->rnsi_inst_tenth_db = calculate_normalized_gain(link_gain_loop_get_min_tenth_db(gain_index), rnsi) -
                                      link_gain_loop_get_rnsi_tenth_db(gain_index);
        } else {
            lqi->rnsi_inst_tenth_db = calculate_normalized_gain(link_gain_loop_get_min_tenth_db(gain_index), rnsi);
        }
#else
        lqi->rnsi_inst_tenth_db = calculate_normalized_gain(link_gain_loop_get_min_tenth_db(gain_index), rnsi);
#endif
        /* Update the total sum of intant RNSI values represented in tenths of dB.  */
        lqi->rnsi_sum_tenth_db += lqi->rnsi_inst_tenth_db;
        /* Update the sum for the next RNSI block average. */
        lqi->rnsi_block_sum_tenth_db += lqi->rnsi_inst_tenth_db;
        lqi->rnsi_block_count++;

        /* Calculate the block average RSSI when the block size is reached. */
        link_lqi_calculate_rnsi_block_avg_tenth_db(lqi);

        /* Update the instantaneous RSSI value represented in tenths of dB.*/
        lqi->rssi_inst_tenth_db = calculate_normalized_gain(link_gain_loop_get_min_tenth_db(gain_index), rssi);

        /* Update the total sum of intant RSSI values represented in tenths of dB.  */
        lqi->rssi_sum_tenth_db += lqi->rssi_inst_tenth_db;
        /* Update the sum for the next RSSI block average. */
        lqi->rssi_block_sum_tenth_db += lqi->rssi_inst_tenth_db;
        lqi->rssi_block_count++;

        /* Calculate the block average of the RSSI when the block size is reached. */
        link_lqi_calculate_rssi_block_avg_tenth_db(lqi);

        /* Update the sum of all RSSI and RNSI codes. */
        lqi->rssi_sum_raw += rssi;
        lqi->rnsi_sum_raw += rnsi;

        break;
    case FRAME_SENT_ACK_LOST:
        lqi->sent_count++;
        lqi->nack_count++;
        if (lqi->mode == LQI_MODE_0) {
            /* Without a received frame, RNSI cannot be measured, the typical value is used. */
            lqi->rnsi_inst_tenth_db = link_gain_loop_get_rnsi_tenth_db(gain_index);
            /* Without a received frame, RSSI cannot be measured, the minimum strength is used. */
            lqi->rssi_inst_tenth_db = link_gain_loop_get_min_tenth_db(gain_index);

            /* Update the total sum of intant RSSI and RNSI values represented in tenths of dB. */
            lqi->rnsi_sum_tenth_db += lqi->rnsi_inst_tenth_db;
            lqi->rssi_sum_tenth_db += lqi->rssi_inst_tenth_db;

            /* Update the sum for the next RSSI and RNSI block average and increment their counts. */
            lqi->rnsi_block_sum_tenth_db += lqi->rnsi_inst_tenth_db;
            lqi->rssi_block_sum_tenth_db += lqi->rssi_inst_tenth_db;
            lqi->rnsi_block_count++;
            lqi->rssi_block_count++;

            /* Calculate the block average of the RSSI and RNSI when the block size is reached. */
            link_lqi_calculate_rnsi_block_avg_tenth_db(lqi);
            link_lqi_calculate_rssi_block_avg_tenth_db(lqi);
        }
        break;
    case FRAME_SENT_ACK_REJECTED:
        lqi->sent_count++;
        lqi->nack_count++;
        /* Update the instantaneous RSSI and RNSI codes from the radio register. */
        lqi->rnsi_inst_raw = rnsi;
        lqi->rssi_inst_raw = rssi;

        /* Update the instantaneous RNSI value represented in tenths of dB.*/
#if SR1100
        if (lqi->mode == LQI_MODE_1) {
            lqi->rnsi_inst_tenth_db = calculate_normalized_gain(link_gain_loop_get_min_tenth_db(gain_index), rnsi) -
                                      link_gain_loop_get_rnsi_tenth_db(gain_index);
        } else {
            lqi->rnsi_inst_tenth_db = calculate_normalized_gain(link_gain_loop_get_min_tenth_db(gain_index), rnsi);
        }
#else
        lqi->rnsi_inst_tenth_db = calculate_normalized_gain(link_gain_loop_get_min_tenth_db(gain_index), rnsi);
#endif
        /* Update the total sum of intant RNSI values represented in tenths of dB. */
        lqi->rnsi_sum_tenth_db += lqi->rnsi_inst_tenth_db;
        /* Update the sum for the next RSSI block average. */
        lqi->rnsi_block_sum_tenth_db += lqi->rnsi_inst_tenth_db;
        lqi->rnsi_block_count++;

        /* Calculate the block average RNSI when the block size is reached. */
        link_lqi_calculate_rnsi_block_avg_tenth_db(lqi);

        /* Update the instantaneous RSSI value represented in tenths of dB.*/
        lqi->rssi_inst_tenth_db = calculate_normalized_gain(link_gain_loop_get_min_tenth_db(gain_index), rssi);

        /* Update the total sum of RSSI value represented in tenths of dB. */
        lqi->rssi_sum_tenth_db += lqi->rssi_inst_tenth_db;
        /* Update the sum for the next RSSI block average. */
        lqi->rssi_block_sum_tenth_db += lqi->rssi_inst_tenth_db;
        lqi->rssi_block_count++;

        /* Calculate the block average RSSI when the block size is reached. */
        link_lqi_calculate_rssi_block_avg_tenth_db(lqi);

        /* Update the sum of all RSSI and RNSI codes. */
        lqi->rssi_sum_raw += rssi;
        lqi->rnsi_sum_raw += rnsi;
        break;
    case FRAME_WAIT:
        lqi->sent_count++;
        break;
    }
}
