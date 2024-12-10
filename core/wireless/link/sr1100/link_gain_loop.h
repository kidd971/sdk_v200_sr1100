/** @file link_gain_loop.h
 *  @brief Gain loop module.
 *
 *  @copyright Copyright (C) 2020 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef LINK_GAIN_LOOP_H_
#define LINK_GAIN_LOOP_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "link_utils.h"
#include "sr1100_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
typedef struct gain_entry {
    /*! Gain value */
    uint8_t gain_value;
    /*! Minimum boundary (tenths of dB) */
    uint16_t min_tenth_db;
    /*! Maximum boundary (tenths of dB) */
    uint16_t max_tenth_db;
    /*! Typical RNSI (tenths of dB) */
    uint16_t relative_noise_floor_db;
} gain_entry_t;

typedef struct gain_loop {
    /*! Gain index */
    uint8_t gain_index;
    /*! Fixed gain loop enable */
    bool fixed_gain_enable;
    /*! RX gain */
    uint8_t rx_gain;
} gain_loop_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the gain loop.
 *
 *  @param[in] gain_loop          Gain loop object.
 *  @param[in] fixed_gain_enable  Not yet implemented. Fixed gain loop enable flag.
 *  @param[in] rx_gain            Not yet implemented. Fixed gain loop value.
 */
void link_gain_loop_init(gain_loop_t *gain_loop, bool fixed_gain_enable, uint8_t rx_gain);

/** @brief Update gain index value.
 *
 *  @param[in] gain_loop      Gain loop object.
 *  @param[in] frame_outcome  Outcome of the frame.
 *  @param[in] rssi           Receiver signal strength indicator.
 */
void link_gain_loop_update(gain_loop_t *gain_loop, frame_outcome_t frame_outcome, uint8_t rssi);

/** @brief Get gain value.
 *
 *  @param[in] gain_loop  Gain loop object.
 *  @return gain value.
 */
uint8_t link_gain_loop_get_gain_value(gain_loop_t *gain_loop);

/** @brief Get minimum gain value.
 *
 *  @param[in] gain_index  Gain loop index.
 *  @return min db.
 */
uint16_t link_gain_loop_get_min_tenth_db(uint8_t gain_index);

/** @brief Get RNSI value.
 *
 *  @param[in] gain_index  Gain loop index.
 *  @return rnsi.
 */
uint16_t link_gain_loop_get_rnsi_tenth_db(uint8_t gain_index);

/** @brief Get the gain index of the gainloop.
 *
 *  @param[in] gain_loop  Gain loop object.
 *  @return Gain loop index
 */
static inline uint8_t link_gain_loop_get_gain_index(gain_loop_t *gain_loop)
{
    return gain_loop->gain_index;
}

/** @brief Reset gain index.
 *
 *  @param[in] gain_loop  Gain loop object.
 */
static inline void link_gain_loop_reset_gain_index(gain_loop_t *gain_loop)
{
    gain_loop->gain_index = 0;
}

#ifdef __cplusplus
}
#endif
#endif /* LINK_GAIN_LOOP_H_ */
