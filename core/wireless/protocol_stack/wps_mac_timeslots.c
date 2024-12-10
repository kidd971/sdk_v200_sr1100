/** @file  wps_mac_timeslots.c
 *  @brief Wireless Protocol Stack MAC time slots module.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "wps_mac.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void wps_mac_timeslots_find_received_timeslot_and_connection_main(wps_mac_t *wps_mac, const xlayer_frame_t *const frame)
{
    uint8_t ts_id_saw;
    uint8_t time_slot_id;
    uint8_t connection_id;
    wps_connection_t *connection;
    uint8_t connection_count;
    uint8_t *conn_id;
    wps_connection_t *prev_auto_connection;

    connection = wps_mac->main_connection;
    connection_count = wps_mac->timeslot->main_connection_count;
    conn_id = &wps_mac->main_connection_id;
    prev_auto_connection = wps_mac->auto_connection;
    if (wps_mac_is_network_node(wps_mac)) {
        ts_id_saw = *(frame->header_begin_it +
                      link_protocol_get_buffer_offset(&connection->link_protocol, MAC_PROTO_ID_TIMESLOT_SAW));
        time_slot_id = MASK2VAL(ts_id_saw, HEADER_BYTE0_TIME_SLOT_ID_MASK);
        if (time_slot_id < wps_mac->scheduler.schedule.size) {
            if (link_scheduler_get_next_timeslot_index(&wps_mac->scheduler) != time_slot_id) {
                link_scheduler_set_mismatch(&wps_mac->scheduler);
            }
            link_scheduler_set_time_slot_i(&wps_mac->scheduler, time_slot_id);
        }
    }
    if ((!link_tdma_sync_is_slave_synced(&wps_mac->tdma_sync) && (wps_mac->node_role == NETWORK_NODE)) ||
        link_scheduler_get_mismatch(&wps_mac->scheduler)) {
        wps_mac->timeslot = link_scheduler_get_current_timeslot(&wps_mac->scheduler);
        wps_mac->main_connection = link_scheduler_get_current_main_connection(&wps_mac->scheduler,
                                                                              wps_mac->main_connection_id);
        wps_mac->auto_connection = link_scheduler_get_current_auto_connection(&wps_mac->scheduler, 0);
        connection = wps_mac->main_connection;
        connection_count = wps_mac->timeslot->main_connection_count;
    }

    if (connection_count > 1) {
        connection_id = *(frame->header_begin_it +
                          link_protocol_get_buffer_offset(&connection->link_protocol, MAC_PROTO_ID_CONNECTION_ID));
        if (connection_id < connection_count) {
            *conn_id = connection_id;
        } else {
            *conn_id = 0;
        }
    } else {
        *conn_id = 0;
    }
    wps_mac->main_connection = link_scheduler_get_current_main_connection(&wps_mac->scheduler,
                                                                          wps_mac->main_connection_id);
    wps_mac->auto_connection = link_scheduler_get_current_auto_connection(&wps_mac->scheduler,
                                                                          wps_mac->auto_connection_id);

    /* Prevent a crash for `process_auto_frame_outcome` when an auto-reply TX was expected to be processed,
     * but a time slot mismatch was detected and the new time slot has no connection to the auto-reply.
     * The previously scheduled auto-reply TX connection will continue to be processed to free memory properly.
     */
    if (wps_mac->auto_connection == NULL && link_scheduler_get_mismatch(&wps_mac->scheduler) == true) {
        wps_mac->auto_connection = prev_auto_connection;
    }
}

void wps_mac_timeslots_find_received_timeslot_and_connection_auto(wps_mac_t *wps_mac, const xlayer_frame_t *const frame)
{
    uint8_t connection_id;
    wps_connection_t *connection;
    uint8_t connection_count;
    uint8_t *conn_id;
    link_protocol_t *link_protocol;

    connection = wps_mac->auto_connection;
    /* If an auto-reply connection does not exist, use the main connection to parse the header */
    if (connection == NULL) {
        connection = wps_mac->main_connection;
    }
    connection_count = wps_mac->timeslot->auto_connection_count;
    conn_id = &wps_mac->auto_connection_id;
    link_protocol = &connection->link_protocol;

    /* If an auto-reply connection does not exist, use the main connection to parse the header
     * and assign connection id to the main connection
     */
    if (connection == NULL) {
        connection = wps_mac->main_connection;
        connection_count = wps_mac->timeslot->main_connection_count;
        conn_id = &wps_mac->main_ack_connection_id;
        link_protocol = connection->auto_link_protocol;
    }

    if (connection_count > 1) {
        connection_id = *(frame->header_begin_it +
                          link_protocol_get_buffer_offset(link_protocol, MAC_PROTO_ID_CONNECTION_ID));
        if (connection_id < connection_count) {
            *conn_id = connection_id;
        } else {
            *conn_id = 0;
        }
    } else {
        *conn_id = 0;
    }
    wps_mac->main_connection = link_scheduler_get_current_main_connection(&wps_mac->scheduler,
                                                                          wps_mac->main_connection_id);
    wps_mac->auto_connection = link_scheduler_get_current_auto_connection(&wps_mac->scheduler,
                                                                          wps_mac->auto_connection_id);
}
