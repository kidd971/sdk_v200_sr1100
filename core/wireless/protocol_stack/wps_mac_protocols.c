/** @file  wps_mac_protocols.c
 *  @brief Wireless Protocol Stack protocols.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "wps_mac.h"

/* CONSTANTS ******************************************************************/
/** @brief Size of the credit flow control field.
 */
#define CREDIT_FLOW_CONTROL_PROTO_SIZE (1)

/** @brief Maximum value of credit value send in the frame header.
 */
#define CREDIT_FLOW_CONTROL_MAX_VALUE (255)

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void update_phases_data(wps_phase_info_t *phase_data, uint16_t rx_wait_time);
static bool is_phase_data_valid(wps_phase_info_t *phase_data);

/* PUBLIC FUNCTIONS ***********************************************************/
void wps_mac_send_channel_index(void *wps_mac, uint8_t *index)
{
    wps_mac_t *mac = wps_mac;
    *index         = link_channel_hopping_get_seq_index(&mac->channel_hopping);
}

void wps_mac_receive_channel_index(void *wps_mac, uint8_t *index)
{
    wps_mac_t *mac = wps_mac;

    if (mac->node_role == NETWORK_NODE) {
        link_channel_hopping_set_seq_index(&mac->channel_hopping, *index);
    }
}

uint8_t wps_mac_get_channel_index_proto_size(void *wps_mac)
{
    wps_mac_t *mac = wps_mac;

    return sizeof(mac->channel_hopping.hop_seq_index);
}

void wps_mac_send_timeslot_id_saw(void *wps_mac, uint8_t *timeslot_id_saw)
{
    wps_mac_t *mac = wps_mac;
    uint16_t index = link_scheduler_get_next_timeslot_index(&mac->scheduler);

    *timeslot_id_saw = MASK2VAL(index, HEADER_BYTE0_TIME_SLOT_ID_MASK) |
                       MOV2MASK(link_saw_arq_get_seq_num(&mac->main_connection->stop_and_wait_arq),
                                HEADER_BYTE0_SEQ_NUM_MASK);
}

void wps_mac_receive_timeslot_id_saw(void *wps_mac, uint8_t *timeslot_id_saw)
{
    wps_mac_t *mac = wps_mac;
    uint8_t time_slot_id;

    if (mac->node_role == NETWORK_NODE) {
        time_slot_id = MASK2VAL(*timeslot_id_saw, HEADER_BYTE0_TIME_SLOT_ID_MASK);
        if (time_slot_id < mac->scheduler.schedule.size) {
            if (link_scheduler_get_next_timeslot_index(&mac->scheduler) != time_slot_id) {
                link_scheduler_set_mismatch(&mac->scheduler);
            }
            link_scheduler_set_time_slot_i(&mac->scheduler, time_slot_id);
        }
    }

    link_saw_arq_update_rx_seq_num(&mac->main_connection->stop_and_wait_arq,
                                   MASK2VAL(*timeslot_id_saw, HEADER_BYTE0_SEQ_NUM_MASK));
    /* Check if received frame is an auto sync frame */
    if (mac->main_xlayer->frame.header_begin_it + mac->main_xlayer->frame.header_memory_size !=
        mac->main_xlayer->frame.payload_end_it) {
        if (link_saw_arq_is_rx_frame_duplicate(&mac->main_connection->stop_and_wait_arq)) {
            /* Frame is duplicate */
            mac->output_signal.main_signal = MAC_SIGNAL_WPS_EMPTY;
        }
    }
}

uint8_t wps_mac_get_timeslot_id_saw_proto_size(void *wps_mac)
{
    wps_mac_t *mac = wps_mac;

    return sizeof(mac->scheduler.current_time_slot_num);
}

void wps_mac_send_rdo(void *wps_mac, uint8_t *rdo)
{
    wps_mac_t *mac = wps_mac;

    link_rdo_send_offset(&mac->link_rdo, rdo);
}

void wps_mac_receive_rdo(void *wps_mac, uint8_t *rdo)
{
    wps_mac_t *mac = wps_mac;

    link_rdo_set_offset(&mac->link_rdo, rdo);
}

uint8_t wps_mac_get_rdo_proto_size(void *wps_mac)
{
    wps_mac_t *mac = wps_mac;

    return sizeof(mac->link_rdo.offset);
}

void wps_mac_send_ranging_phases(void *wps_mac, uint8_t *phases)
{
    wps_mac_t *mac = wps_mac;

    *phases = mac->phase_data.local_phases_count;
    phases++;
    *phases = mac->phase_data.local_phases_info.phase1;
    phases++;
    *phases = mac->phase_data.local_phases_info.phase2;
    phases++;
    *phases = mac->phase_data.local_phases_info.phase3;
    phases++;
    *phases = mac->phase_data.local_phases_info.phase4;
}

void wps_mac_receive_ranging_phases(void *wps_mac, uint8_t *phases)
{
    wps_mac_t *mac = wps_mac;
    wps_connection_t *connection = (mac->auto_connection == NULL) ? mac->main_connection : mac->auto_connection;
    link_phase_t *link_phase = &connection->link_phase;

    mac->phase_data.remote_phases_count = *phases;
    phases++;
    mac->phase_data.remote_phases_info.phase1 = *phases;
    phases++;
    mac->phase_data.remote_phases_info.phase2 = *phases;
    phases++;
    mac->phase_data.remote_phases_info.phase3 = *phases;
    phases++;
    mac->phase_data.remote_phases_info.phase4 = *phases;

    if (is_phase_data_valid(&mac->phase_data)) {
        if (link_phase_add_data(link_phase, mac->phase_data.last_local_phases_info,
                                mac->phase_data.remote_phases_info)) {
            mac->config.callback_auto.callback = connection->ranging_data_ready_callback;
            mac->config.callback_auto.parg_callback = connection->ranging_data_ready_parg_callback;
            wps_callback_enqueue(&mac->callback_queue, &mac->config.callback_auto);
        };
    }
    update_phases_data(&mac->phase_data, mac->config.rx_wait_time);
}

uint8_t wps_mac_get_ranging_phases_proto_size(void *wps_mac)
{
    wps_mac_t *mac = wps_mac;

    return sizeof(mac->phase_data.local_phases_count) +
           sizeof(mac->phase_data.local_phases_info.phase1) +
           sizeof(mac->phase_data.local_phases_info.phase2) +
           sizeof(mac->phase_data.local_phases_info.phase3) +
           sizeof(mac->phase_data.local_phases_info.phase4);
}

void wps_mac_send_ranging_phase_count(void *wps_mac, uint8_t *phase_count)
{
    wps_mac_t *mac = wps_mac;

    /* Transmit count */
    *phase_count = mac->phase_data.local_phases_count;
}

void wps_mac_receive_ranging_phase_count(void *wps_mac, uint8_t *phase_count)
{
    wps_mac_t *mac = wps_mac;

    mac->phase_data.local_phases_count = *phase_count;
}

uint8_t wps_mac_get_ranging_phase_count_proto_size(void *wps_mac)
{
    wps_mac_t *mac = wps_mac;

    return sizeof(mac->phase_data.local_phases_count);
}

void wps_mac_send_connection_id(void *wps_mac, uint8_t *connection_id)
{
    wps_mac_t *mac = wps_mac;

    if ((mac->auto_connection != NULL) &&
        (mac->auto_connection->source_address == mac->local_address)) {
        *connection_id = mac->auto_connection_id;
    } else {
        *connection_id = mac->main_connection_id;
    }
}

void wps_mac_receive_connection_id(void *wps_mac, uint8_t *connection_id)
{
    wps_mac_t *mac = wps_mac;
    uint8_t connection_count;
    uint8_t *conn_id;

    if ((mac->auto_connection != NULL) &&
        !(mac->auto_connection->source_address == mac->local_address)) {
        connection_count = mac->timeslot->auto_connection_count;
        conn_id          = &mac->auto_connection_id;
    } else {
        connection_count = mac->timeslot->main_connection_count;
        conn_id          = &mac->main_connection_id;
    }
    if (connection_count > 1) {
        if (*connection_id < connection_count) {
            *conn_id = *connection_id;
        } else {
            *conn_id = 0;
        }
    } else {
        *conn_id = 0;
    }
}

uint8_t wps_mac_get_connection_id_proto_size(void *wps_mac)
{
    wps_mac_t *mac = wps_mac;

    return sizeof(mac->main_connection_id);
}

void wps_mac_send_connection_id_header_acknowledge(void *wps_mac, uint8_t *connection_id)
{
    wps_mac_t *mac = wps_mac;

    *connection_id = mac->main_ack_connection_id;
}

void wps_mac_receive_connection_id_header_acknowledge(void *wps_mac, uint8_t *connection_id)
{
    wps_mac_t *mac = wps_mac;

    mac->main_ack_connection_id = *connection_id;
}

void wps_mac_send_credit_flow_control(void *wps_mac, uint8_t *const credit_fc)
{
    wps_mac_t *mac = wps_mac;
    uint16_t free_slot = 0;
    wps_connection_t *connection = NULL;

    if (wps_mac_timeslots_is_current_timeslot_tx(mac) == false) {
        /* Use ID of auto_connection_id for main connection credit data */
        connection = link_scheduler_get_current_main_connection(&mac->scheduler, mac->auto_connection_id);
    } else if (mac->auto_connection != NULL) {
        /* Use ID of main_connection_id for auto-reply connection credit data */
        connection = link_scheduler_get_current_auto_connection(&mac->scheduler, mac->main_connection_id);
    }

    if (connection != NULL) {
        free_slot = xlayer_queue_get_free_space(&connection->xlayer_queue);
    }

    if (free_slot > CREDIT_FLOW_CONTROL_MAX_VALUE) {
        free_slot = CREDIT_FLOW_CONTROL_MAX_VALUE;
    } else if (free_slot > 0) {
        free_slot = free_slot - 1;
    }

    *credit_fc = (uint8_t)free_slot;
}

void wps_mac_receive_credit_flow_control(void *wps_mac, uint8_t *credit_fc)
{
    wps_mac_t *mac = wps_mac;
    wps_connection_t *connection = NULL;

    if (wps_mac_timeslots_is_current_timeslot_tx(mac) == true) {
        connection = link_scheduler_get_current_main_connection(&mac->scheduler, mac->auto_connection_id);
    } else if (mac->auto_connection != NULL) {
        connection = link_scheduler_get_current_auto_connection(&mac->scheduler, mac->main_connection_id);
    }

    if (connection != NULL) {
        connection->credit_flow_ctrl.credits_count = *credit_fc;
    }
}

uint8_t wps_mac_get_credit_flow_control_proto_size(void *wps_mac)
{
    (void)wps_mac;

    return CREDIT_FLOW_CONTROL_PROTO_SIZE;
}

void wps_mac_send_credit_flow_control_header_acknowledge(void *wps_mac, uint8_t *const credit_fc)
{
    wps_mac_t *mac = wps_mac;
    wps_connection_t *connection = link_scheduler_get_current_main_connection(&mac->scheduler,
                                                                              mac->main_ack_connection_id);
    uint16_t free_slot = xlayer_queue_get_free_space(&connection->xlayer_queue);

    if (free_slot > CREDIT_FLOW_CONTROL_MAX_VALUE) {
        free_slot = CREDIT_FLOW_CONTROL_MAX_VALUE;
    } else if (free_slot > 0) {
        free_slot = free_slot - 1;
    }

    *credit_fc = (uint8_t)free_slot;
}

void wps_mac_receive_credit_flow_control_header_acknowledge(void *wps_mac, uint8_t *credit_fc)
{
    wps_mac_t *mac = wps_mac;
    wps_connection_t *connection = link_scheduler_get_current_main_connection(&mac->scheduler,
                                                                              mac->main_ack_connection_id);

    connection->credit_flow_ctrl.credits_count = *credit_fc;
}

/* PRIVATE FUNCTIONS ***********************************************************/
/** @brief Update phases data.
 *
 *  @param[in] phase_data   Phase data.
 *  @param[in] rx_wait_time Reception wait time.
 */
static void update_phases_data(wps_phase_info_t *phase_data, uint16_t rx_wait_time)
{
    phase_data->last_local_phases_info.phase1     = phase_data->local_phases_info.phase1;
    phase_data->last_local_phases_info.phase2     = phase_data->local_phases_info.phase2;
    phase_data->last_local_phases_info.phase3     = phase_data->local_phases_info.phase3;
    phase_data->last_local_phases_info.phase4     = phase_data->local_phases_info.phase4;
    phase_data->last_local_phases_info.rx_waited0 = rx_wait_time & 0x00ff;
    phase_data->last_local_phases_info.rx_waited1 = (rx_wait_time & 0x7f00) >> 8;
    phase_data->local_phases_count++;
}

/** @brief Return if current phase data are valid.
 *
 *  @param[in] phase_data   Phase data.
 *  @retval True   Current situationis valid.
 *  @retval False  Current is not valid.
 */
static bool is_phase_data_valid(wps_phase_info_t *phase_data)
{
    return (((uint8_t)(phase_data->remote_phases_count + 1)) == phase_data->local_phases_count);
}
