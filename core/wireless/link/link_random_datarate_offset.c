/** @file link_random_datarate_offset.c
 *  @brief Random datarate offset algorithm.
 *
 *  This algorithm is use for the concurrency to delay the sync value
 *  between device. It is use by the WPS Layer 2 internal connection.
 *  The output value of this algorithm is sent between device WPS.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "link_random_datarate_offset.h"

/* CONSTANTS ******************************************************************/
#define DEFAULT_ROLLOVER 15

/* PUBLIC FUNCTIONS ***********************************************************/
void link_rdo_init(link_rdo_t *link_rdo, uint16_t target_rollover_value, uint16_t target_increment_step)
{
    link_rdo->offset  = 0;
    link_rdo->enabled = false;
    link_rdo->increment_step = target_increment_step != 0 ? target_increment_step : 1;

    if (target_rollover_value == 0) {
        link_rdo->rollover_n = DEFAULT_ROLLOVER * link_rdo->increment_step;
    } else {
        link_rdo->rollover_n = target_rollover_value * link_rdo->increment_step;
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
        uint16_t offset        = link_rdo->offset;

        link_rdo->offset_u8[0] = (offset >> 8) & 0x00FF;
        link_rdo->offset_u8[1] = (offset)&0x00FF;
        memcpy(buffer_to_send, link_rdo->offset_u8, sizeof(uint16_t));
    }
}

void link_rdo_set_offset(link_rdo_t *link_rdo, uint8_t *buffer_to_received)
{
    if (buffer_to_received != NULL) {
        memcpy(&link_rdo->offset_u8, buffer_to_received, sizeof(uint16_t));
        link_rdo->offset = (uint16_t)(((uint16_t)link_rdo->offset_u8[0] << 8) | ((uint16_t)link_rdo->offset_u8[1]));
    }
}

uint16_t link_rdo_get_offset(link_rdo_t *link_rdo)
{
    return link_rdo->enabled ? (link_rdo->offset / link_rdo->increment_step) : 0;
}

void link_rdo_update_offset(link_rdo_t *link_rdo)
{
    link_rdo->offset = (link_rdo->rollover_n != 0) ? (link_rdo->offset + 1) % link_rdo->rollover_n : 0;
}
