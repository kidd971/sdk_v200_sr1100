/** @file link_multi_radio.c
 *  @brief Multi radio module.
 *
 *  @copyright Copyright (C) 2020 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "link_multi_radio.h"
#include <stdbool.h>

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void multi_radio_update_mode_0(multi_radio_t *multi_radio);
static void multi_radio_update_mode_1(multi_radio_t *multi_radio);

/* PUBLIC FUNCTIONS ***********************************************************/
void link_multi_radio_update(multi_radio_t *multi_radio)
{
    switch (multi_radio->mode) {
    case MULTI_RADIO_MODE_0:
        multi_radio_update_mode_0(multi_radio);
        break;
    case MULTI_RADIO_MODE_1:
        multi_radio_update_mode_1(multi_radio);
        break;
    default:
        multi_radio_update_mode_0(multi_radio);
        break;
    }
}

uint8_t link_multi_radio_get_replying_radio(multi_radio_t *multi_radio)
{
    if (multi_radio->radio_select == MULTI_RADIO_SELECT_ALGO) {
        return multi_radio->replying_radio;
    } else {
        return (multi_radio->radio_select - 1);
    }
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Update multi radio for mode 0.
 *
 *  @param[in] multi_radio  Multi radio object.
 *  @return None.
 */
static void multi_radio_update_mode_0(multi_radio_t *multi_radio)
{
    uint8_t best_radio    = multi_radio->replying_radio;
    uint16_t max_rssi_avg = 0;
    uint16_t temp_rssi_avg;
    uint16_t replying_radio_rssi_avg = 0;

    for (uint8_t i = 0; i < multi_radio->radio_count; i++) {
        if (multi_radio->radios_lqi[i].total_count < multi_radio->avg_sample_count) {
            return;
        }
    }

    for (uint8_t i = 0; i < multi_radio->radio_count; i++) {
        temp_rssi_avg = link_lqi_get_avg_rssi_tenth_db(&multi_radio->radios_lqi[i]);
        link_lqi_reset(&multi_radio->radios_lqi[i]);
        if (i == multi_radio->replying_radio) {
            replying_radio_rssi_avg = temp_rssi_avg;
        }
        if (temp_rssi_avg > max_rssi_avg) {
            max_rssi_avg = temp_rssi_avg;
            best_radio   = i;
        }
    }

    if (max_rssi_avg > (replying_radio_rssi_avg + multi_radio->hysteresis_tenth_db)) {
        multi_radio->replying_radio = best_radio;
    }
}

/** @brief Update multi radio for mode 1.
 *
 *  @param[in] multi_radio  Multi radio object.
 *  @return None.
 */
static void multi_radio_update_mode_1(multi_radio_t *multi_radio)
{
    uint16_t rssi_avg;

    if (multi_radio->radios_lqi[multi_radio->replying_radio].total_count < multi_radio->avg_sample_count) {
        return;
    }
    rssi_avg = link_lqi_get_avg_rssi_tenth_db(&multi_radio->radios_lqi[multi_radio->replying_radio]);
    for (uint8_t i = 0; i < multi_radio->radio_count; i++) {
        link_lqi_reset(&multi_radio->radios_lqi[i]);
    }
    if (rssi_avg < multi_radio->rssi_threshold) {
        multi_radio->replying_radio = (multi_radio->replying_radio + 1) % multi_radio->radio_count;
    }
}
