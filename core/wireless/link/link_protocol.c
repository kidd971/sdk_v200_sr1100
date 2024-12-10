/** @file link_protocol.c
 *  @brief WPS layer 2 internal connection protocol.
 *
 *  This file is a wrapper use to send/received payload
 *  through the WPS MAC internal connection. Its used to
 *  properly generate a complete packet regrouping one
 *  or multiple information.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
/* INCLUDES *******************************************************************/
#include "link_protocol.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void link_protocol_init(link_protocol_t *link_protocol, uint16_t max_buffer_size)
{
    memset(link_protocol, 0, sizeof(link_protocol_t));
    link_protocol->max_buffer_size = max_buffer_size;
}

void link_protocol_add(link_protocol_t *link_protocol, link_protocol_info_t *protocol_info, link_error_t *err)
{
    *err = LINK_PROTO_NO_ERROR;
    if (link_protocol->current_number_of_protocol < MAX_NUMBER_OF_PROTOCOL) {
        if (link_protocol->current_buffer_offset < link_protocol->max_buffer_size) {
            link_protocol->protocol_info[link_protocol->current_number_of_protocol]       = *protocol_info;
            link_protocol->protocol_info[link_protocol->current_number_of_protocol].index = link_protocol->current_buffer_offset;
            link_protocol->current_number_of_protocol++;
            link_protocol->current_buffer_offset += protocol_info->size;
        } else {
            *err = LINK_PROTO_NO_MORE_SPACE;
        }
    } else {
        *err = LINK_PROTO_TOO_MANY_PROTO;
    }
}

void link_protocol_send_buffer(link_protocol_t *link_protocol, uint8_t *buffer_to_send, uint32_t *size)
{
    uint32_t size_to_send       = 0;

    for (uint8_t i = 0; i < link_protocol->current_number_of_protocol; i++) {
        link_protocol->protocol_info[i].send(link_protocol->protocol_info[i].instance, buffer_to_send + size_to_send);
        size_to_send += link_protocol->protocol_info[i].size;
    }

    *size = size_to_send;
}

void link_protocol_receive_buffer(link_protocol_t *link_protocol, uint8_t *receive_buffer, uint32_t size)
{
    uint8_t *receive_buffer_end = receive_buffer + size;

    for (uint8_t i = 0; i < link_protocol->current_number_of_protocol && receive_buffer < receive_buffer_end; i++) {
        link_protocol->protocol_info[i].receive(link_protocol->protocol_info[i].instance, receive_buffer);
        receive_buffer += link_protocol->protocol_info[i].size;
    }
}

uint8_t link_protocol_get_buffer_offset(link_protocol_t *link_protocol, uint8_t protocol_id)
{
    for (uint8_t i = 0; i < link_protocol->current_number_of_protocol; i++) {
        if (link_protocol->protocol_info[i].id == protocol_id) {
            return link_protocol->protocol_info[i].index + 1;
        }
    }

    return 0;
}
