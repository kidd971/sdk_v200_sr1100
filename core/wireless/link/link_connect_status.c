/** @file link_connect_status.c
 *  @brief Link connection status module.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "link_connect_status.h"
#include <stdlib.h>
#include <string.h>

/* CONSTANTS ******************************************************************/
/*! Maximum duration in millisecond so that the microsecond value fits in a 16-bit unsigned integer. */
#define MAX_DISCONNECT_DURATION_MS 65

/* PUBLIC FUNCTIONS ***********************************************************/
void link_connect_status_init(link_connect_status_t *link_connect_status, connect_status_cfg_t *cfg)
{
    link_connect_status->connect_count = cfg->connect_count;
    if (cfg->disconnect_duration_ms > MAX_DISCONNECT_DURATION_MS) {
        cfg->disconnect_duration_ms = MAX_DISCONNECT_DURATION_MS;
    }
    link_connect_status->disconnect_duration_us = cfg->disconnect_duration_ms * MS_TO_US;
    link_connect_status->lost_duration_us = 0;
    link_connect_status->received_count = 0;
    link_connect_status->status = CONNECT_STATUS_DISCONNECTED;
}

bool link_update_connect_status(link_connect_status_t *link_connect_status, frame_outcome_t frame_outcome,
                                bool sync_status, bool always_connected, uint32_t time_elapsed_us)
{
    connect_status_t old_status = link_connect_status->status;

    if (sync_status == false) {
        link_connect_status->status = CONNECT_STATUS_DISCONNECTED;
    } else if (always_connected) {
        link_connect_status->status = CONNECT_STATUS_CONNECTED;
        link_connect_status->received_count = 0;
        link_connect_status->lost_duration_us = 0;
    } else {
        if (link_connect_status->status == CONNECT_STATUS_CONNECTED) {
            switch (frame_outcome) {
            case FRAME_RECEIVED_DATA_SIZE_INVALID:
            case FRAME_RECEIVED_HEADER_SIZE_INVALID:
            case FRAME_REJECTED:
            case FRAME_LOST:
            case FRAME_SENT_ACK_LOST:
            case FRAME_SENT_ACK_REJECTED:
                link_connect_status->lost_duration_us += time_elapsed_us;
                if (link_connect_status->lost_duration_us >= link_connect_status->disconnect_duration_us) {
                    link_connect_status->status = CONNECT_STATUS_DISCONNECTED;
                    link_connect_status->received_count = 0;
                    link_connect_status->lost_duration_us = 0;
                }
                break;
            case FRAME_RECEIVED:
            case FRAME_SENT_ACK:
                link_connect_status->lost_duration_us = 0;
                break;
            default:
                break;
            }
        } else if (link_connect_status->status == CONNECT_STATUS_DISCONNECTED) {
            switch (frame_outcome) {
            case FRAME_RECEIVED:
            case FRAME_SENT_ACK:
                link_connect_status->received_count++;
                if (link_connect_status->received_count >= link_connect_status->connect_count) {
                    link_connect_status->status = CONNECT_STATUS_CONNECTED;
                    link_connect_status->received_count = 0;
                    link_connect_status->lost_duration_us = 0;
                }
                break;
            case FRAME_RECEIVED_DATA_SIZE_INVALID:
            case FRAME_RECEIVED_HEADER_SIZE_INVALID:
            case FRAME_REJECTED:
            case FRAME_LOST:
            case FRAME_SENT_ACK_LOST:
            case FRAME_SENT_ACK_REJECTED:
                link_connect_status->received_count = 0;
                break;
            default:
                break;
            }
        }
    }

    return old_status != link_connect_status->status;
}

void link_connect_status_reset(link_connect_status_t *link_connect_status)
{
    link_connect_status->lost_duration_us = 0;
    link_connect_status->received_count = 0;
    link_connect_status->status = CONNECT_STATUS_DISCONNECTED;
}
