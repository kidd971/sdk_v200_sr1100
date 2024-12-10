/** @file link_gain_loop.c
 *  @brief Gain loop module.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "link_gain_loop.h"
#include "sr_def.h"

/* CONSTANTS ******************************************************************/
#define LOWER_BOUND_MARGIN_TENTH_DB  120
#define HIGHER_BOUND_MARGIN_TENTH_DB 40
#define GAIN_ENTRY_COUNT             10
#define UPPER_BOUND_RSSI             5
#define LOWER_BOUND_RSSI             15
#define RF_BUF_GAIN_OFFSET           5
#define RF_MIX_GAIN_OFFSET           3
#define RF_OA_GAIN_OFFSET            0
#define PLACE_HOLDER_RSSI_TENTH_DB   44


/* PRIVATE GLOBALS ************************************************************/
static const gain_entry_t gain_lookup_table[GAIN_ENTRY_COUNT] = {
    {(0 << RF_BUF_GAIN_OFFSET) | (0 << RF_MIX_GAIN_OFFSET) | (3 << RF_OA_GAIN_OFFSET), 0, 235, 0},
    {(0 << RF_BUF_GAIN_OFFSET) | (3 << RF_MIX_GAIN_OFFSET) | (3 << RF_OA_GAIN_OFFSET), 46, 281, 15},
    {(0 << RF_BUF_GAIN_OFFSET) | (0 << RF_MIX_GAIN_OFFSET) | (6 << RF_OA_GAIN_OFFSET), 87, 322, 26},
    {(0 << RF_BUF_GAIN_OFFSET) | (1 << RF_MIX_GAIN_OFFSET) | (6 << RF_OA_GAIN_OFFSET), 104, 339, 38},
    {(0 << RF_BUF_GAIN_OFFSET) | (2 << RF_MIX_GAIN_OFFSET) | (6 << RF_OA_GAIN_OFFSET), 119, 354, 49},
    {(0 << RF_BUF_GAIN_OFFSET) | (3 << RF_MIX_GAIN_OFFSET) | (6 << RF_OA_GAIN_OFFSET), 134, 369, 62},
    {(1 << RF_BUF_GAIN_OFFSET) | (3 << RF_MIX_GAIN_OFFSET) | (6 << RF_OA_GAIN_OFFSET), 166, 401, 91},
    {(2 << RF_BUF_GAIN_OFFSET) | (3 << RF_MIX_GAIN_OFFSET) | (6 << RF_OA_GAIN_OFFSET), 199, 434, 122},
    {(3 << RF_BUF_GAIN_OFFSET) | (3 << RF_MIX_GAIN_OFFSET) | (6 << RF_OA_GAIN_OFFSET), 226, 461, 148},
    {(3 << RF_BUF_GAIN_OFFSET) | (3 << RF_MIX_GAIN_OFFSET) | (7 << RF_OA_GAIN_OFFSET), 310, 545, 230},
};

/* PUBLIC FUNCTIONS ***********************************************************/
void link_gain_loop_init(gain_loop_t *gain_loop, bool fixed_gain_enable, uint8_t rx_gain)
{
    gain_loop->gain_index        = 0;
    gain_loop->fixed_gain_enable = fixed_gain_enable;
    gain_loop->rx_gain           = rx_gain;
}

void link_gain_loop_update(gain_loop_t *gain_loop, frame_outcome_t frame_outcome, uint8_t rssi)
{
    (void)gain_loop;
    (void)frame_outcome;
    (void)rssi;

    /* Hardcode gain loop index to 0 because the full range
     * of reception gain is available with SR1120
     */
    gain_loop->gain_index = 0;
}

uint8_t link_gain_loop_get_gain_value(gain_loop_t *gain_loop)
{
    if (gain_loop->fixed_gain_enable) {
        return gain_loop->rx_gain;
    } else {
        return gain_lookup_table[gain_loop->gain_index].gain_value;
    }
}

uint16_t link_gain_loop_get_min_tenth_db(uint8_t gain_index)
{
    return gain_lookup_table[gain_index].min_tenth_db;
}

uint16_t link_gain_loop_get_rnsi_tenth_db(uint8_t gain_index)
{
    return gain_lookup_table[gain_index].relative_noise_floor_db;
}
