/** @file link_utils.h
 *  @brief Link utility macros and functions.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is confidential and proprietary.
 *  @author    SPARK FW Team.
 */
#ifndef LINK_UTILS_H_
#define LINK_UTILS_H_

/* INCLUDES *******************************************************************/
#include <stdint.h>
#include "sr1100_def.h"

/* CONSTANTS ******************************************************************/
#define WEAKEST_SIGNAL_CODE       115
#define GAIN_ENTRY_RANGE_TENTH_DB 575
#define CIR_LIMIT                 33
#define MAX_CIR_SPREAD            300
#define MAX(a, b)                 ((a) > (b) ? (a) : (b))
#define MIN(a, b)                 ((a) < (b) ? (a) : (b))
/*! Multiply an unsigned int by two using bit shift (which is faster than typical mul and div operations). */
#define MUL_2(x) ((x) << 1)
/*! Divide an unsigned int by two using bit shift (which is faster than typical mul and div operations). */
#define DIV_2(x) ((x) >> 1)
/*! Multiply an unsigned int by four using bit shift (which is faster than typical mul and div operations). */
#define MUL_4(x) ((x) << 2)
/*! Divide an unsigned int by four using bit shift (which is faster than typical mul and div operations). */
#define DIV_4(x) ((x) >> 2)

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Calculate normalized gain.
 *
 *  @param[in] gain_index  Current gain index.
 *  @param[in] rssi        Receiver signal strength indicator.
 *  @return Normalized gain in tenths of dB.
 */
static inline uint16_t calculate_normalized_gain(uint16_t min_db, uint8_t rssi)
{
    return (min_db + (GAIN_ENTRY_RANGE_TENTH_DB * (WEAKEST_SIGNAL_CODE - rssi) / WEAKEST_SIGNAL_CODE));
}

/** @brief Compute CIR based off Phase offset data directly extracted from FIFO.
 *
 *  @param[in] phase_offset_data  Phase offset data extracted from the FIFO.
 *  @param[in] phase_number       Number of phase.
 *  @return  CIR spread, from 0 to 300%
 */
static inline uint16_t calculate_cir(uint8_t *phase_offset_data, uint8_t phase_number)
{
    uint8_t max_val, max_index = 0;
    uint8_t min_val;
    uint8_t cir_spread = 0;

    max_val = 0;
    min_val = UINT8_MAX;

    for (uint8_t i = 0; i < phase_number; i++) {
        if (phase_offset_data[i] > max_val) {
            max_val = phase_offset_data[i];
            max_index = i;
        }
        if (phase_offset_data[i] < min_val) {
            min_val = phase_offset_data[i];
        }
    }

    /* Normalize phase offset data */
    for (uint8_t i = 0; i < phase_number; i++) {
        phase_offset_data[i] -= min_val;
    }

    if (phase_offset_data[max_index] == 0) {
        return 0;
    }

    for (uint8_t i = 0; i <= (phase_number / 2); i++) {
        uint32_t computed_cir_ratio1 = ((uint32_t)phase_offset_data[(max_index + i) % phase_number]) * 100 /
                                       ((uint32_t)phase_offset_data[max_index]);
        uint32_t computed_cir_ratio2 = ((uint32_t)phase_offset_data[(max_index + (phase_number - i)) % phase_number]) *
                                       100 / ((uint32_t)phase_offset_data[max_index]);
        uint16_t cir_ratio = (uint16_t)MAX(computed_cir_ratio1, computed_cir_ratio2);

        if (cir_ratio > CIR_LIMIT) {
            cir_spread = i;
        }
    }

    return MIN(cir_spread * 100 / 4, MAX_CIR_SPREAD);
}

#endif /* LINK_UTILS_H_ */
