/** @file  wps_mac_xlayer.c
 *  @brief Wireless Protocol Stack MAC xlayer component.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "wps_mac.h"
#include "xlayer_circular_data.h"

/* CONSTANTS ******************************************************************/
#define RADIO_MAX_PACKET_SIZE 255

/* PRIVATE GLOBALS ************************************************************/
/** @brief Buffer for xlayer instance when application RX/TX queue is empty.
 */
uint8_t overrun_buffer[RADIO_MAX_PACKET_SIZE];
/** @brief Auto-reply frame buffer when no auto-reply connection exist.
 * Contains only data for the header.
 */
uint8_t auto_reply_buffer[HEADER_MAX_SIZE + 1];

/* PUBLIC FUNCTIONS ***********************************************************/
xlayer_t *wps_mac_xlayer_get_xlayer_for_tx_main(wps_mac_t *wps_mac, wps_connection_t *connection)
{
    xlayer_t *free_xlayer;
    xlayer_queue_node_t *node;
    bool unsync = ((wps_mac->tdma_sync.slave_sync_state == STATE_SYNCING) &&
                   (wps_mac->node_role == NETWORK_NODE));
    bool valid_credits = link_credit_flow_ctrl_is_available(&connection->credit_flow_ctrl);

    if (connection->currently_enabled && valid_credits) {
        node = xlayer_queue_get_node(&connection->xlayer_queue);
        /* Check if there is something available to send on the COORDINATOR after connect event*/
        if (connection->first_tx_after_connect && node != NULL && wps_mac->node_role == NETWORK_COORDINATOR) {
            /* Prepare sync frame */
            node = NULL;
            wps_mac->empty_frame_tx.frame.header_memory = overrun_buffer;
            wps_mac->empty_frame_tx.frame.header_end_it = overrun_buffer + connection->header_size;
            wps_mac->empty_frame_tx.frame.header_begin_it = wps_mac->empty_frame_tx.frame.header_end_it;
            wps_mac->empty_frame_tx.frame.payload_end_it = wps_mac->empty_frame_tx.frame.header_end_it;
            wps_mac->empty_frame_tx.frame.payload_begin_it = wps_mac->empty_frame_tx.frame.header_end_it;
            free_xlayer = &wps_mac->empty_frame_tx;
            wps_mac->empty_frame_tx.frame.time_stamp = connection->get_tick();
            return free_xlayer;
        }
    } else {
        node = NULL;
    }
    if (node == NULL) {
        free_xlayer = NULL;
    } else {
        free_xlayer = &node->xlayer;
    }

    if (free_xlayer == NULL || unsync) {
        bool credit_left_out_frames_exceed = link_credit_flow_ctrl_is_skipped_frames_exceed(
            &connection->credit_flow_ctrl);

        if ((connection->auto_sync_enable && !unsync) || (credit_left_out_frames_exceed == true)) {
            wps_mac->empty_frame_tx.frame.header_memory = overrun_buffer;
            wps_mac->empty_frame_tx.frame.header_end_it = overrun_buffer + connection->header_size;
        } else {
            wps_mac->empty_frame_tx.frame.header_memory = NULL;
            wps_mac->empty_frame_tx.frame.header_end_it = NULL;
        }
        wps_mac->empty_frame_tx.frame.header_begin_it = wps_mac->empty_frame_tx.frame.header_end_it;
        wps_mac->empty_frame_tx.frame.payload_end_it  = wps_mac->empty_frame_tx.frame.header_end_it;
        wps_mac->empty_frame_tx.frame.payload_begin_it =
            wps_mac->empty_frame_tx.frame.header_end_it;
        free_xlayer                              = &wps_mac->empty_frame_tx;
        wps_mac->empty_frame_tx.frame.time_stamp = connection->get_tick();
    } else {
        free_xlayer->frame.header_begin_it = free_xlayer->frame.header_end_it;
    }

    return free_xlayer;
}

xlayer_t *wps_mac_xlayer_get_xlayer_for_tx_auto(wps_mac_t *wps_mac, wps_connection_t *connection)
{
    xlayer_t *free_xlayer;
    xlayer_queue_node_t *node;
    bool unsync = ((wps_mac->tdma_sync.slave_sync_state == STATE_SYNCING) && (wps_mac->node_role == NETWORK_NODE));
    bool valid_credits = link_credit_flow_ctrl_is_available(&connection->credit_flow_ctrl);

    if (connection->currently_enabled && valid_credits) {
        node = xlayer_queue_get_node(&connection->xlayer_queue);
    } else {
        node = NULL;
    }
    if (node == NULL) {
        free_xlayer = NULL;
    } else {
        free_xlayer = &node->xlayer;
    }

    if (free_xlayer == NULL || unsync) {
        xlayer_frame_t *empty_frame = &wps_mac->empty_frame_tx.frame;
        bool force_empty_frame = (connection->credit_flow_ctrl.enabled == true) ||
                                 (connection->ranging_mode != WPS_RANGING_DISABLED);

        if ((connection->auto_sync_enable && !unsync) || force_empty_frame == true) {
            empty_frame->header_memory = overrun_buffer;
            empty_frame->header_end_it = overrun_buffer + connection->header_size;
        } else {
            empty_frame->header_memory = NULL;
            empty_frame->header_end_it = NULL;
        }
        empty_frame->header_begin_it = empty_frame->header_end_it;
        empty_frame->payload_end_it = empty_frame->header_end_it;
        empty_frame->payload_begin_it = empty_frame->header_end_it;
        free_xlayer = &wps_mac->empty_frame_tx;
        empty_frame->time_stamp = connection->get_tick();
    } else {
        free_xlayer->frame.header_begin_it = free_xlayer->frame.header_end_it;
    }

    return free_xlayer;
}

xlayer_t *wps_mac_xlayer_get_xlayer_for_rx(wps_mac_t *wps_mac, wps_connection_t *connection)
{
    wps_mac->rx_node = xlayer_queue_get_free_node(connection->free_rx_queue);

    /* if free node is not available, will return an empty frame*/
    if (wps_mac->rx_node == NULL) {
        wps_mac->empty_frame_rx.frame.header_memory   = overrun_buffer;
        wps_mac->empty_frame_rx.frame.header_end_it   = overrun_buffer;
        wps_mac->empty_frame_rx.frame.header_begin_it = wps_mac->empty_frame_rx.frame.header_end_it;
        wps_mac->empty_frame_rx.frame.payload_begin_it = overrun_buffer + connection->header_size + EMPTY_BYTE;
        wps_mac->empty_frame_rx.frame.payload_memory_size = connection->payload_size;
        wps_mac->empty_frame_rx.frame.header_memory_size  = connection->header_size;
        return &wps_mac->empty_frame_rx;
    }

    wps_mac->rx_node->xlayer.frame.payload_memory_size = connection->payload_size;
    wps_mac->rx_node->xlayer.frame.header_memory_size  = connection->header_size;
    return &wps_mac->rx_node->xlayer;
}

void wps_mac_xlayer_free_node_with_data(wps_connection_t *connection, xlayer_queue_node_t *node)
{
    if (node != NULL) {
        xlayer_circular_data_free_space(connection->rx_data, node->xlayer.frame.payload_memory,
                                        node->xlayer.frame.max_frame_size);
        node->xlayer.frame.payload_memory = NULL;
        node->xlayer.frame.max_frame_size = 0;
        xlayer_queue_free_node(node);
    }
}

void wps_mac_xlayer_update_main_rx_payload_buffer(void *wps_mac, xlayer_frame_t *frame, uint8_t required_space)
{
    wps_connection_t *connection;
    wps_mac_t *mac = (wps_mac_t *)wps_mac;

#if (WPS_RADIO_COUNT == 2)
    /* If the payload memory is allocated by another radio, do not re-allocate it. */
    if (frame->payload_memory != NULL) {
        return;
    }
#endif

    wps_mac_timeslots_find_received_timeslot_and_connection_main(mac, frame);

    /* If RX node is null this mean that empty_frame_rx is used and buffer data is provided from
     * overrun_buffer.
     */
    if (mac->rx_node == NULL || required_space == 0) {
        return;
    }

    connection = link_scheduler_get_current_main_connection(&mac->scheduler, mac->main_connection_id);

    /* Do not allow space allocation larger than the configured maximum payload size */
    if (required_space > connection->payload_size) {
        frame->payload_memory = NULL;
        frame->payload_begin_it = NULL;
        return;
    }
    /* Allocate maximum possible connection payload size to prevent creating smaller blocks which can be used later to
     * read larger amount of data.
     */
    required_space = connection->payload_size;
    uint8_t *payload_memory = xlayer_circular_data_allocate_space(connection->rx_data, required_space);

    mac->main_xlayer->frame.payload_memory = payload_memory;
    mac->main_xlayer->frame.payload_begin_it = payload_memory;

    if (payload_memory != NULL) {
        mac->main_xlayer->frame.max_frame_size = required_space;
        /* Dual radio use the same payload memory */
#if (WPS_RADIO_COUNT == 2)
        wps_phy_multi.following_main_xlayer.frame.payload_memory = payload_memory;
        wps_phy_multi.following_main_xlayer.frame.payload_begin_it = payload_memory;
        wps_phy_multi.following_main_xlayer.frame.max_frame_size = required_space;
#endif
    }
}

void wps_mac_xlayer_update_auto_reply_rx_payload_buffer(void *wps_mac, xlayer_frame_t *frame, uint8_t required_space)
{
    wps_connection_t *connection;
    wps_mac_t *mac = (wps_mac_t *)wps_mac;

#if (WPS_RADIO_COUNT == 2)
    /* If the payload memory is allocated by another radio, do not re-allocate it. */
    if (frame->payload_memory != NULL) {
        return;
    }
#endif

    wps_mac_timeslots_find_received_timeslot_and_connection_auto(mac, frame);

    /* If RX node is null this mean that empty_frame_rx is used and buffer data is provided from
     * overrun_buffer.
     */
    if (mac->rx_node == NULL || required_space == 0) {
        return;
    }

    connection = link_scheduler_get_current_auto_connection(&mac->scheduler, mac->auto_connection_id);

    /* Do not allow space allocation larger than the configured maximum payload size */
    if (required_space > connection->payload_size) {
        frame->payload_memory = NULL;
        frame->payload_begin_it = NULL;
        return;
    }
    /* Allocate maximum possible connection payload size to prevent creating smaller blocks which can be used later to
     * read larger amount of data.
     */
    required_space = connection->payload_size;
    uint8_t *payload_memory = xlayer_circular_data_allocate_space(connection->rx_data, required_space);

    mac->auto_xlayer->frame.payload_memory = payload_memory;
    mac->auto_xlayer->frame.payload_begin_it = payload_memory;

    if (payload_memory != NULL) {
        mac->auto_xlayer->frame.max_frame_size = required_space;
        /* Dual radio use the same payload memory */
#if (WPS_RADIO_COUNT == 2)
        wps_phy_multi.following_auto_xlayer.frame.payload_memory = payload_memory;
        wps_phy_multi.following_auto_xlayer.frame.payload_begin_it = payload_memory;
        wps_phy_multi.following_auto_xlayer.frame.max_frame_size = required_space;
#endif
    }
}

xlayer_t *wps_mac_xlayer_get_xlayer_for_empty_rx_auto(wps_mac_t *wps_mac, wps_connection_t *connection)
{
    wps_mac->rx_node = NULL;
    wps_mac->empty_auto_reply_frame.frame.header_memory = auto_reply_buffer;
    wps_mac->empty_auto_reply_frame.frame.header_end_it = auto_reply_buffer;
    wps_mac->empty_auto_reply_frame.frame.header_begin_it = wps_mac->empty_auto_reply_frame.frame.header_end_it;
    wps_mac->empty_auto_reply_frame.frame.payload_end_it = wps_mac->empty_auto_reply_frame.frame.header_end_it;
    wps_mac->empty_auto_reply_frame.frame.payload_begin_it = wps_mac->empty_auto_reply_frame.frame.header_end_it;
    wps_mac->empty_auto_reply_frame.frame.payload_memory_size = 0;
    wps_mac->empty_auto_reply_frame.frame.header_memory_size = connection->ack_header_size;

    return &wps_mac->empty_auto_reply_frame;
}

xlayer_t *wps_mac_xlayer_get_xlayer_for_empty_tx_auto(wps_mac_t *wps_mac, wps_connection_t *connection)
{
    wps_mac->empty_auto_reply_frame.frame.header_memory = auto_reply_buffer;
    wps_mac->empty_auto_reply_frame.frame.header_end_it = auto_reply_buffer;
    wps_mac->empty_auto_reply_frame.frame.header_begin_it = wps_mac->empty_auto_reply_frame.frame.header_end_it;
    wps_mac->empty_auto_reply_frame.frame.payload_end_it = wps_mac->empty_auto_reply_frame.frame.header_end_it;
    wps_mac->empty_auto_reply_frame.frame.payload_begin_it = wps_mac->empty_auto_reply_frame.frame.header_end_it;
    wps_mac->empty_auto_reply_frame.frame.payload_memory_size = 0;
    wps_mac->empty_auto_reply_frame.frame.header_memory_size = connection->ack_header_size;

    return &wps_mac->empty_auto_reply_frame;
}
