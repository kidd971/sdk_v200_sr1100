/** @file link_utils.h
 *  @brief Link utility macros and functions.
 *
 *  @copyright Copyright (C) 2020 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is confidential and proprietary.
 *  @author    SPARK FW Team.
 */
#ifndef LINK_UTILS_H_
#define LINK_UTILS_H_

/* INCLUDES *******************************************************************/
#include <stdint.h>

/* CONSTANTS ******************************************************************/
#define WEAKEST_SIGNAL_CODE       115
#define GAIN_ENTRY_RANGE_TENTH_DB 575

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

#endif /* LINK_UTILS_H_ */
