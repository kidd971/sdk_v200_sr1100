/** @file  link_credit_flow_ctrl.c
 *  @brief Link Credit Control Flow module.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "link_credit_flow_ctrl.h"
#include <string.h>

/* PUBLIC FUNCTIONS ***********************************************************/
void link_credit_flow_ctrl_init(credit_flow_ctrl_t *credit_flow_ctrl, bool enabled, uint8_t init_credits_count)
{
    memset(credit_flow_ctrl, 0, sizeof(credit_flow_ctrl_t));

    credit_flow_ctrl->enabled = enabled;
    credit_flow_ctrl->credits_count = init_credits_count;
}
