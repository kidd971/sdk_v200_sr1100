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

/* PUBLIC FUNCTIONS ***********************************************************/
void link_cca_init(link_cca_t *cca, uint8_t threshold, uint16_t retry_time_pll_cycles, uint16_t on_time_pll_cycles,
                   uint8_t max_try_count, cca_fail_action_t fail_action, bool enable)
{
    cca->enable = enable;
    cca->threshold = threshold;
    cca->retry_time_pll_cycles = ((retry_time_pll_cycles / 32 - 1) + 1) * 32;
    cca->on_time_pll_cycles = ((on_time_pll_cycles / 8 - 1) + 1) * 8;
    cca->max_try_count = max_try_count;
    cca->fail_action = fail_action;
    cca->fbk_try_count = NULL;
}

void link_cca_set_fbk_try_count(link_cca_t *cca, uint8_t *fbk_try_count, size_t fallback_size)
{
    if (cca->enable == false) {
        return;
    }

    cca->fbk_try_count = fbk_try_count;

    if (fbk_try_count != NULL) {
        for (size_t i = 0; i < fallback_size; i++) {
            if (fbk_try_count[i] > cca->max_try_count) {
                cca->max_try_count = fbk_try_count[i];
            }
        }
    }
}

uint16_t link_cca_get_on_time(link_cca_t *cca)
{
    return (cca->on_time_pll_cycles / 8) - 1;
}
