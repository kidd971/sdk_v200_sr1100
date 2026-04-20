/** @file link_random_datarate_offset.c
 *  @brief Random datarate offset algorithm.
 *
 *  This algorithm is use for the concurrency to delay the sync value
 *  between device. It is use by the WPS Layer 2 internal connection.
 *  The output value of this algorithm is sent between device WPS.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "link_random_datarate_offset.h"
#include "sr_def.h"

/* CONSTANTS ******************************************************************/
#define DEFAULT_ROLLOVER    15
#define RDO_OFFSET_MASK     0x7f
#define COUNT_UP_BIT_OFFSET 7

/* PUBLIC FUNCTIONS ***********************************************************/
void link_rdo_init(link_rdo_t *link_rdo, uint16_t target_rollover_value, uint16_t step_ms)
{
    link_rdo->offset = 0;
    link_rdo->enabled = false;
    link_rdo->target_time_us = step_ms * MS_TO_US;
    link_rdo->elapsed_time_us = 0;
    link_rdo->count_up = true;

    if (target_rollover_value == 0) {
        link_rdo->rollover_n = DEFAULT_ROLLOVER;
    } else {
        link_rdo->rollover_n = target_rollover_value;
    }
}

void link_rdo_enable(link_rdo_t *link_rdo)
{
    link_rdo->enabled = true;
}

void link_rdo_disable(link_rdo_t *link_rdo)
{
    link_rdo->enabled = false;
}

void link_rdo_send_offset(link_rdo_t *link_rdo, uint8_t *buffer_to_send)
{
    if (buffer_to_send != NULL) {
        *buffer_to_send = (link_rdo->offset & RDO_OFFSET_MASK) | (link_rdo->count_up << COUNT_UP_BIT_OFFSET);
    }
}

void link_rdo_set_offset(link_rdo_t *link_rdo, uint8_t *buffer_to_received)
{
    if (buffer_to_received != NULL) {
        link_rdo->offset = *buffer_to_received & RDO_OFFSET_MASK;
        link_rdo->count_up = *buffer_to_received >> COUNT_UP_BIT_OFFSET;
    }
}

uint16_t link_rdo_get_offset(link_rdo_t *link_rdo)
{
    return link_rdo->enabled ? (link_rdo->offset) : 0;
}

void link_rdo_update_offset(link_rdo_t *link_rdo, uint32_t sleep_time_us)
{
    link_rdo->elapsed_time_us += sleep_time_us;
    if (link_rdo->elapsed_time_us >= link_rdo->target_time_us) {
        if (link_rdo->count_up == true) {
            link_rdo->offset = (link_rdo->rollover_n != 0) ? (link_rdo->offset + 1) : 0;
            if (link_rdo->offset == link_rdo->rollover_n - 1) {
                link_rdo->count_up = false;
            }
        } else {
            link_rdo->offset = (link_rdo->rollover_n != 0) ? (link_rdo->offset - 1) : 0;
            if (link_rdo->offset == 0) {
                link_rdo->count_up = true;
            }
        }

        link_rdo->elapsed_time_us = 0;
    }
}
