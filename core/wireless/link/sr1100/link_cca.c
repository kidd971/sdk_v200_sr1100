/** @file link_cca.c
 *  @brief Clear Channel Assessment module.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
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
    cca->fbk_try_count = NULL;

    return true;
}

void link_cca_disable(link_cca_t *cca)
{
    /* Reset all values. */
    memset(cca, 0, sizeof(link_cca_t));
    /* Set threshold to maximum value. */
    cca->threshold = WPS_DISABLE_CCA_THRESHOLD;
}

bool link_cca_set_fbk_try_count(link_cca_t *cca, uint8_t *fbk_try_count, size_t fallback_size)
{
    if (cca == NULL || cca->enable == false || fbk_try_count == NULL) {
        /* Invalid CCA configuration. */
        return false;
    }

    /* Initialize CCA fbk_try_count array. */
    cca->fbk_try_count = fbk_try_count;

    /* Update CCA max_try_count value. */
    if (fbk_try_count != NULL) {
        for (size_t i = 0; i < fallback_size; i++) {
            if (fbk_try_count[i] > CCA_MAX_RETRY_COUNT(cca->fail_action)) {
                /* Invalid max_try_count value. */
                return false;
            }
            if (fbk_try_count[i] > cca->max_try_count) {
                cca->max_try_count = fbk_try_count[i];
            }
        }
    }

    return true;
}

uint16_t link_cca_get_on_time(link_cca_t *cca)
{
    return (cca->on_time_pll_cycles / 8) - 1;
}
