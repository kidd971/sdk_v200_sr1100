/** @file link_cca.c
 *  @brief Clear Channel Assessment module.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is confidential and proprietary.
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "link_cca.h"
#include "wps_def.h"

/* PUBLIC FUNCTIONS ***********************************************************/
bool link_cca_init(link_cca_t *cca, uint8_t threshold, uint16_t retry_time_pll_cycles, uint16_t on_time_pll_cycles,
                   uint8_t max_try_count, cca_fail_action_t fail_action)
{
    if (cca == NULL || max_try_count > CCA_MAX_RETRY_COUNT(cca->fail_action) ||
        retry_time_pll_cycles > CCAINTERV_MAX_VALUE) {
        /* Invalid CCA configuration. */
        return false;
    }

    cca->enable = true;
    cca->threshold = threshold;
    cca->retry_time_pll_cycles = ((retry_time_pll_cycles / 32 - 1) + 1) * 32;
    cca->on_time_pll_cycles = ((on_time_pll_cycles / 8 - 1) + 1) * 8;
    cca->max_try_count = max_try_count;
    cca->fail_action = fail_action;
    cca->fallback_cca_try_count = NULL;
    cca->fallback_mode_count = 0;

    if (cca->fail_action == CCA_FAIL_ACTION_ABORT_TX) {
        cca->rx_timeout_offset_pll_cycles = (link_cca_get_highest_try_count(cca) - 1) *
                                            (cca->retry_time_pll_cycles + cca->on_time_pll_cycles);
    } else {
        cca->rx_timeout_offset_pll_cycles = link_cca_get_highest_try_count(cca) *
                                            (cca->retry_time_pll_cycles + cca->on_time_pll_cycles);
    }

    return true;
}

void link_cca_disable(link_cca_t *cca)
{
    /* Reset all values. */
    memset(cca, 0, sizeof(link_cca_t));
    /* Set threshold to maximum value. */
    cca->threshold = WPS_DISABLE_CCA_THRESHOLD;
}

bool link_cca_set_fallback_try_count(link_cca_t *cca, uint8_t *fallback_cca_try_count, uint8_t fallback_mode_count)
{
    if (cca == NULL || cca->enable == false || fallback_cca_try_count == NULL) {
        /* Invalid CCA configuration. */
        return false;
    }

    /* Initialize CCA fallback_cca_try_count array. */
    cca->fallback_cca_try_count = fallback_cca_try_count;
    cca->fallback_mode_count = fallback_mode_count;

    return true;
}

uint16_t link_cca_get_on_time(link_cca_t *cca)
{
    return (cca->on_time_pll_cycles / 8) - 1;
}
