/** @file link_connect_status.c
 *  @brief Link connection status module.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "link_connect_status.h"
#include <stdlib.h>
#include <string.h>

/* PUBLIC FUNCTIONS ***********************************************************/
void link_connect_status_init(link_connect_status_t *link_connect_status, connect_status_cfg_t *cfg)
{
    link_connect_status->connect_count    = cfg->connect_count;
    link_connect_status->disconnect_count = cfg->disconnect_count;
    link_connect_status->lost_count       = 0;
    link_connect_status->received_count   = 0;
    link_connect_status->status           = CONNECT_STATUS_DISCONNECTED;
}

bool link_update_connect_status(link_connect_status_t *link_connect_status, frame_outcome_t frame_outcome,
                                bool sync_status, bool ack_enabled)
{
    connect_status_t old_status = link_connect_status->status;

    if (sync_status == false) {
        link_connect_status->status = CONNECT_STATUS_DISCONNECTED;
    } else if (ack_enabled) {
        if (link_connect_status->status == CONNECT_STATUS_CONNECTED) {
            switch (frame_outcome) {
            case FRAME_REJECTED:
            case FRAME_LOST:
            case FRAME_SENT_ACK_LOST:
            case FRAME_SENT_ACK_REJECTED:
                link_connect_status->lost_count++;
                if (link_connect_status->lost_count >= link_connect_status->disconnect_count) {
                    link_connect_status->status         = CONNECT_STATUS_DISCONNECTED;
                    link_connect_status->received_count = 0;
                    link_connect_status->lost_count     = 0;
                }
                break;
            case FRAME_RECEIVED:
            case FRAME_SENT_ACK:
                link_connect_status->lost_count = 0;
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
                    link_connect_status->status         = CONNECT_STATUS_CONNECTED;
                    link_connect_status->received_count = 0;
                    link_connect_status->lost_count     = 0;
                }
                break;
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
    } else {
        link_connect_status->status         = CONNECT_STATUS_CONNECTED;
        link_connect_status->received_count = 0;
        link_connect_status->lost_count     = 0;
    }

    return old_status != link_connect_status->status;
}
