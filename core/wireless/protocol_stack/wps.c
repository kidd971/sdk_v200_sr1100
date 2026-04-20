/** @file  wps.c
 *  @brief SPARK Wireless Protocol Stack.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "wps.h"
#include "sr_calib.h"
#include "sr_phy_error.h"
#include "sr_pwr_up.h"
#include "sr_spectral.h"
#include "sr_utils.h"

/* CONSTANTS ******************************************************************/
#define EXTRACT_NETWORK_ID(addr, msbits_count) ((addr) >> (16 - (msbits_count)))
#define PERCENT_DENOMINATOR                    100
#define US_TO_PLL_FACTOR                       1000
#define MS_TO_S_FACTOR                         1000
#define DISCONNECT_TIMEOUT_MS                  1000
/* PHY rate specific configurations. */
#define PHY_RATE_IS_20_48 false
#define PHY_RATE_IS_40_96 true

/* PRIVATE GLOBALS ************************************************************/
static circular_queue_t schedule_ratio_cfg_queue;
static circular_queue_t write_request_queue;
static circular_queue_t read_request_queue;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static bool is_main_timeslot(int8_t id);
static uint32_t auto_reply_id_to_id(int8_t id);
static uint8_t generate_active_pattern(bool *pattern, uint8_t active_ratio);
static uint8_t find_channel_count_from_sequence(channel_sequence_t *channel_sequence);
static void config_channel(wps_connection_t *connection, wps_node_t *node, uint8_t channel_x, channel_cfg_t *config,
                           bool cfg_40_96, wps_error_t *err);
static void config_fallback_channel(wps_connection_t *connection, wps_node_t *node, uint8_t channel_x,
                                    uint8_t fallback_index, channel_cfg_t *config, bool cfg_40_96, wps_error_t *err);
static void check_auto_connection_priority_errors(timeslot_t timeslot, wps_error_t *const err);
static void check_main_connection_priority_errors(timeslot_t timeslot, wps_error_t *const err);

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
uint32_t wps_us_to_pll_cycle(uint32_t time_us, chip_rate_cfg_t chip_rate)
{
    return ((uint64_t)(time_us) * (uint64_t)(PLL_FREQ_KHZ(chip_rate)) / US_TO_PLL_FACTOR) - 1;
}

void wps_radio_init(wps_radio_t *wps_radio, bool no_reset, sr_phy_error_t *err)
{
    sr_pwr_up(&wps_radio->radio, !no_reset, err);
}

void wps_radio_calibration(wps_radio_t *wps_radio, calib_vars_t *calib_vars)
{
    sr_nvm_init(&wps_radio->radio, wps_radio->nvm);
    sr_calibrate(&wps_radio->radio, calib_vars, wps_radio->nvm);
}

uint64_t wps_radio_get_serial_number(wps_radio_t *wps_radio)
{
    return sr_nvm_get_serial_number(wps_radio->nvm);
}

uint8_t wps_radio_get_product_id_version(wps_radio_t *wps_radio)
{
    return sr_nvm_get_product_id_version(wps_radio->nvm);
}

uint8_t wps_radio_get_product_id_model(wps_radio_t *wps_radio)
{
    return sr_nvm_get_product_id_model(wps_radio->nvm);
}

uint8_t wps_radio_get_product_id_package(wps_radio_t *wps_radio)
{
    return sr_nvm_get_product_id_package(wps_radio->nvm);
}

uint8_t wps_radio_get_layout_version(wps_radio_t *wps_radio)
{
    return sr_nvm_get_layout_version(wps_radio->nvm);
}

uint16_t wps_radio_get_serial_number_binning_setup_code(wps_radio_t *wps_radio)
{
    return sr_nvm_get_serial_number_binning_setup_code(wps_radio->nvm);
}

uint64_t wps_radio_get_serial_number_chip_id(wps_radio_t *wps_radio)
{
    return sr_nvm_get_serial_number_chip_id(wps_radio->nvm);
}

int8_t wps_radio_get_rssi_offset(wps_radio_t *wps_radio)
{
    return sr_nvm_get_rssi_offset(wps_radio->nvm);
}

int8_t wps_radio_get_vref_adjust_vref_tune_offset(wps_radio_t *wps_radio)
{
    return sr_nvm_get_vref_adjust_vref_tune_offset(wps_radio->nvm);
}

#if !SR1100
uint8_t wps_radio_get_calibration(wps_radio_t *wps_radio)
{
    return sr_nvm_get_calibration(wps_radio->nvm);
}

uint16_t wps_radio_get_vcro_shift(wps_radio_t *wps_radio)
{
    return sr_nvm_get_vcro_shift(wps_radio->nvm);
}

int8_t wps_radio_get_vref_adjust_pulse_width_offset(wps_radio_t *wps_radio)
{
    return sr_nvm_get_vref_adjust_pulse_width_offset(wps_radio->nvm);
}

nvm_power_adjust_t wps_radio_get_power_adjust(wps_radio_t *wps_radio)
{
    return sr_nvm_get_power_adjust(wps_radio->nvm);
}
#endif /* SR1100 */

#if SR1100
uint8_t wps_radio_get_resistune(wps_radio_t *wps_radio)
{
    return sr_nvm_get_resistune(wps_radio->nvm);
}

uint8_t wps_radio_get_ireftune(wps_radio_t *wps_radio)
{
    return sr_nvm_get_ireftune(wps_radio->nvm);
}

nvm_power_adjust_t wps_radio_get_power_adjust(wps_radio_t *wps_radio)
{
    return sr_nvm_get_power_adjust(wps_radio->nvm);
}
#endif /* SR1100 */

void wps_init_callback_queue(wps_t *wps, wps_callback_inst_t *callback_buffer, size_t size)
{
    circular_queue_init(&wps->mac.callback_queue, callback_buffer, size, sizeof(wps_callback_inst_t));
}

void wps_init_request_queue(wps_t *wps, xlayer_request_info_t *request_buffer, size_t size,
                            wps_request_config_info_t *request_config)
{
    circular_queue_init(&wps->mac.request_queue, request_buffer, size, sizeof(xlayer_request_info_t));

    /* Initialize pattern queue for throttling */
    for (uint8_t i = 0; i < request_config->schedule_ratio_size; i++) {
        memset(request_config->schedule_ratio_buffer[i].pattern_cfg, 1, WPS_PATTERN_THROTTLE_GRANULARITY);
    }
    wps->mac.schedule_ratio_cfg_queue = &schedule_ratio_cfg_queue;
    circular_queue_init(wps->mac.schedule_ratio_cfg_queue, request_config->schedule_ratio_buffer,
                        request_config->schedule_ratio_size, sizeof(wps_schedule_ratio_cfg_t));

    /* Initialize write request buffer and queue */
    memset(request_config->write_request_buffer, 0, sizeof(*request_config->write_request_buffer));
    wps->mac.write_request_queue = &write_request_queue;
    circular_queue_init(wps->mac.write_request_queue, request_config->write_request_buffer,
                        request_config->write_request_size, sizeof(xlayer_write_request_info_t));

    /* Initialize read request buffer and queue */
    memset(request_config->read_request_buffer, 0, sizeof(*request_config->read_request_buffer));
    wps->mac.read_request_queue = &read_request_queue;
    circular_queue_init(wps->mac.read_request_queue, request_config->read_request_buffer,
                        request_config->read_request_size, sizeof(xlayer_read_request_info_t));
}

uint32_t wps_get_xlayer_tx_queue_nb_bytes_needed(wps_node_t *node, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    return xlayer_queue_get_tx_required_bytes(node->tx_queues_size);
}

uint32_t wps_get_xlayer_rx_queue_nb_bytes_needed(wps_node_t *node, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    return xlayer_queue_get_rx_required_bytes(node->rx_queues_size, node->max_header_size + EMPTY_BYTE);
}

void wps_init_xlayer(wps_node_t *node, uint8_t *mem_pool_tx, uint8_t *mem_pool_rx, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    /* Allocate queue for TX communication */
    xlayer_queue_init_pool(mem_pool_tx, &node->free_tx_queue, node->tx_queues_size, "Free TX queue");
    /* Allocate queue for RX communication */
    xlayer_queue_init_pool_with_header_data(mem_pool_rx, &node->free_rx_queue, node->rx_queues_size,
                                            node->max_header_size + EMPTY_BYTE, "Free RX queue");
}

void wps_init(wps_t *wps, wps_error_t *err)
{
    wps_mac_sync_cfg_t mac_sync_cfg = {0};

    *err = WPS_NO_ERROR;

    CHECK_ERROR(wps->node.radio == NULL, err, WPS_RADIO_NOT_INITIALIZED_ERROR, return);
    CHECK_ERROR(wps->channel_sequence.channel == NULL, err, WPS_CHANNEL_SEQUENCE_NOT_INITIALIZED_ERROR, return);

    wps->mac.signal = WPS_DISCONNECT;

    mac_sync_cfg.sleep_level = wps->node.cfg.sleep_lvl;
    mac_sync_cfg.isi_mitig = wps->node.cfg.isi_mitig;
    mac_sync_cfg.isi_mitig_pauses = link_tdma_sync_get_isi_mitigation_pauses(mac_sync_cfg.isi_mitig);
    mac_sync_cfg.preamble_len = wps->node.cfg.preamble_len;
    mac_sync_cfg.sfd_len = link_tdma_get_sfd_length(mac_sync_cfg.isi_mitig_pauses, wps->node.cfg.sfd_cfg.sfd_length);

    mac_sync_cfg.tx_jitter_enabled = wps->node.cfg.tx_jitter_enabled;
    mac_sync_cfg.chip_rate = wps->chip_rate;
    wps_mac_init(&wps->mac, &wps->channel_sequence, &mac_sync_cfg, wps->node.cfg.local_address, wps->node.cfg.role,
                 wps->random_channel_sequence_enabled, wps->network_id, wps->node.cfg.frame_lost_max_duration,
                 wps->node.max_payload_size, wps->node.max_header_size, wps->node.max_payload_size_auto,
                 wps->node.max_header_size_auto, &wps->node.conn_list);
    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        wps->phy[i].wps_phy_callback = wps_mac_phy_callback;
        wps->phy[i].mac = (void *)&wps->mac;
    }
}

void wps_set_syncing_address(wps_t *wps, uint16_t address, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    wps->mac.syncing_address = address;
}

void wps_set_network_id(wps_t *wps, uint8_t network_id, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    wps->network_id = network_id;
}

void wps_config_node(wps_t *wps, wps_node_cfg_t *cfg, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    wps->node.cfg = *cfg;
    wps->node.cfg.sfd_cfg.sfd_bit_cost = 2;
    wps->node.cfg.sfd_cfg.sfd_tolerance = 0xC;
    wps->node.max_payload_size = 0;
    wps->node.tx_queues_size = 0;
    wps->node.rx_queues_size = 0;
    wps->node.max_total_rx_data_size = 0;

    wps_connection_list_init(&wps->node.conn_list);

    wps->node.is_initialized = true;
}

void wps_config_network_schedule(wps_t *wps, uint32_t *timeslot_duration_pll_cycles, timeslot_t *timeslot,
                                 uint32_t schedule_size, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    wps->mac.scheduler.schedule.size = schedule_size;
    wps->mac.scheduler.schedule.timeslot = timeslot;

    for (uint32_t i = 0; i < schedule_size; ++i) {
        timeslot[i].duration_pll_cycles = timeslot_duration_pll_cycles[i];
        timeslot[i].main_conn_list.connection_count = 0;
        timeslot[i].auto_conn_list.connection_count = 0;
    }
}

void wps_reset_schedule(wps_t *wps, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    link_scheduler_reset(&wps->mac.scheduler);
}

void wps_config_network_channel_sequence(wps_t *wps, const uint32_t *channel_sequence, uint8_t *channel_sequence_buffer,
                                         uint32_t sequence_size, wps_error_t *err)
{
    *err = WPS_NO_ERROR;
    CHECK_ERROR(channel_sequence_buffer == NULL, err, WPS_CHANNEL_SEQUENCE_INIT_ERROR, return);
    wps->channel_sequence.channel = channel_sequence;
    wps->channel_sequence.sequence_size = sequence_size;
    wps->channel_sequence.channel_number = find_channel_count_from_sequence(&wps->channel_sequence);
    wps->channel_sequence.channel_sequence_buffer = channel_sequence_buffer;
}

void wps_enable_random_channel_sequence(wps_t *wps, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    wps->random_channel_sequence_enabled = true;
}

void wps_disable_random_channel_sequence(wps_t *wps, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    wps->random_channel_sequence_enabled = false;
}

uint8_t wps_get_connection_header_size(wps_t *wps, const wps_connection_t *connection)
{
    uint8_t header_size = 0;
    wps_header_cfg_t header_cfg = connection->header_cfg;

    header_size += header_cfg.main_connection ? wps_mac_get_channel_index_proto_size(&wps->mac) +
                                                    wps_mac_get_timeslot_id_saw_proto_size(&wps->mac) :
                                                0;
    header_size += header_cfg.rdo_enabled ? sizeof(wps->mac.link_rdo.offset) : 0;
    header_size += header_cfg.dynamic_phy_mode ? sizeof(wps->mac.next_chip_rate) : 0;

    switch (header_cfg.ranging_mode) {
    case WPS_RANGING_STANDALONE_INITIATOR:
    case WPS_RANGING_INITIATOR_TX:
    case WPS_RANGING_STANDALONE_RESPONDER:
    case WPS_RANGING_RESPONDER_RX:
        header_size += wps_mac_get_ranging_phase_count_proto_size(&wps->mac);
        break;
    case WPS_RANGING_INITIATOR_RX:
    case WPS_RANGING_RESPONDER_TX:
        header_size += wps_mac_get_ranging_phases_proto_size(&wps->mac);
        break;
    default:
        break;
    }

    header_size += header_cfg.connection_id ? wps_mac_get_connection_id_proto_size(&wps->mac) : 0;
    header_size += header_cfg.credit_fc_enabled ? wps_mac_get_credit_flow_control_proto_size(&wps->mac) : 0;

    return header_size;
}

uint8_t wps_get_connection_ack_header_size(wps_t *wps, const wps_connection_t *connection)
{
    uint8_t header_size = 0;
    wps_header_cfg_t header_cfg = connection->header_cfg;

    header_size += header_cfg.rdo_enabled ? sizeof(wps->mac.link_rdo.offset) : 0;
    if (header_cfg.ranging_mode == WPS_RANGING_STANDALONE_RESPONDER ||
        header_cfg.ranging_mode == WPS_RANGING_STANDALONE_INITIATOR) {
        header_size += wps_mac_get_ranging_phases_proto_size(&wps->mac);
    }
    header_size += header_cfg.connection_id ? wps_mac_get_connection_id_proto_size(&wps->mac) : 0;
    header_size += header_cfg.credit_fc_enabled ? wps_mac_get_credit_flow_control_proto_size(&wps->mac) : 0;

    return header_size;
}

wps_header_cfg_t wps_get_header_cfg(const wps_connection_t *connection)
{
    return connection->header_cfg;
}

void wps_set_header_cfg(wps_t *wps, wps_connection_t *connection, wps_header_cfg_t header_cfg, uint8_t max_payload_size,
                        uint8_t max_frame_size, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    /* Set the header configuration */
    connection->header_cfg = header_cfg;

    /* Calculate and validate header sizes */
    uint8_t header_size = wps_get_connection_header_size(wps, connection);
    uint8_t ack_header_size = wps_get_connection_ack_header_size(wps, connection);

    /* Check if header size exceeds maximum frame size (add 1 for payload size byte) */
    if ((max_payload_size + header_size + 1) > max_frame_size) {
        *err = WPS_HEADER_SIZE_ERROR;
        return;
    }

    if ((max_payload_size + ack_header_size + 1) > max_frame_size) {
        *err = WPS_HEADER_SIZE_ERROR;
        return;
    }

    /* Update connection header sizes */
    connection->cfg.header_size = header_size;
    connection->cfg.ack_header_size = ack_header_size;
}

void wps_configure_header_connection(wps_t *wps, wps_connection_t *connection, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    wps_header_cfg_t header_cfg = connection->header_cfg;

    uint16_t proto_buffer_size = 0;
    link_error_t link_err = {0};

    proto_buffer_size = wps_get_connection_header_size(wps, connection);

    link_protocol_init(&connection->link_protocol, proto_buffer_size);

    link_protocol_info_t link_proto_info = {0};

    if (header_cfg.main_connection == true) {
        link_proto_info.id = MAC_PROTO_ID_TIMESLOT_SAW;
        link_proto_info.instance = &wps->mac;
        link_proto_info.send = wps_mac_send_timeslot_id_saw;
        link_proto_info.receive = wps_mac_receive_timeslot_id_saw;
        link_proto_info.size = wps_mac_get_timeslot_id_saw_proto_size(&wps->mac);

        link_protocol_add(&connection->link_protocol, &link_proto_info, &link_err);

        link_proto_info.id = MAC_PROTO_ID_CHANNEL_INDEX;
        link_proto_info.instance = &wps->mac;
        link_proto_info.send = wps_mac_send_channel_index;
        link_proto_info.receive = wps_mac_receive_channel_index;
        link_proto_info.size = wps_mac_get_channel_index_proto_size(&wps->mac);

        link_protocol_add(&connection->link_protocol, &link_proto_info, &link_err);
    }

    if (header_cfg.rdo_enabled == true) {
        link_proto_info.id = MAC_PROTO_ID_RDO;
        link_proto_info.instance = &wps->mac;
        link_proto_info.send = wps_mac_send_rdo;
        link_proto_info.receive = wps_mac_receive_rdo;
        link_proto_info.size = wps_mac_get_rdo_proto_size(&wps->mac);

        link_protocol_add(&connection->link_protocol, &link_proto_info, &link_err);
    }

    if (header_cfg.dynamic_phy_mode == true) {
        link_proto_info.id = MAC_PROTO_ID_PHY_MODE;
        link_proto_info.instance = &wps->mac;
        link_proto_info.send = wps_mac_send_phy_mode;
        link_proto_info.receive = wps_mac_receive_phy_mode;
        link_proto_info.size = wps_mac_get_phy_mode_proto_size(&wps->mac);

        link_protocol_add(&connection->link_protocol, &link_proto_info, &link_err);
        wps->mac.dynamic_phy_mode_en = true;
    }

    switch (header_cfg.ranging_mode) {
    case WPS_RANGING_STANDALONE_INITIATOR:
    case WPS_RANGING_INITIATOR_TX:
        link_proto_info.id = MAC_PROTO_ID_RANGING_INITIATOR;
        link_proto_info.instance = &wps->mac;
        link_proto_info.send = wps_mac_send_ranging_phase_count;
        link_proto_info.receive = NULL;
        link_proto_info.size = wps_mac_get_ranging_phase_count_proto_size(&wps->mac);
        link_protocol_add(&connection->link_protocol, &link_proto_info, &link_err);
        break;
    case WPS_RANGING_STANDALONE_RESPONDER:
    case WPS_RANGING_RESPONDER_RX:
        link_proto_info.id = MAC_PROTO_ID_RANGING_RESPONDER;
        link_proto_info.instance = &wps->mac;
        link_proto_info.send = NULL;
        link_proto_info.receive = wps_mac_receive_ranging_phase_count;
        link_proto_info.size = wps_mac_get_ranging_phase_count_proto_size(&wps->mac);
        link_protocol_add(&connection->link_protocol, &link_proto_info, &link_err);
        break;
    case WPS_RANGING_INITIATOR_RX:
        link_proto_info.id = MAC_PROTO_ID_RANGING_INITIATOR;
        link_proto_info.instance = &wps->mac;
        link_proto_info.send = NULL;
        link_proto_info.receive = wps_mac_receive_ranging_phases;
        link_proto_info.size = wps_mac_get_ranging_phases_proto_size(&wps->mac);
        link_protocol_add(&connection->link_protocol, &link_proto_info, &link_err);
        break;
    case WPS_RANGING_RESPONDER_TX:
        link_proto_info.id = MAC_PROTO_ID_RANGING_RESPONDER;
        link_proto_info.instance = &wps->mac;
        link_proto_info.send = wps_mac_send_ranging_phases;
        link_proto_info.receive = NULL;
        link_proto_info.size = wps_mac_get_ranging_phases_proto_size(&wps->mac);
        link_protocol_add(&connection->link_protocol, &link_proto_info, &link_err);
        break;
    default:
        break;
    }

    if (header_cfg.connection_id == true) {
        link_proto_info.id = MAC_PROTO_ID_CONNECTION_ID;
        link_proto_info.instance = &wps->mac;
        link_proto_info.send = wps_mac_send_connection_id;
        link_proto_info.receive = wps_mac_receive_connection_id;
        link_proto_info.size = wps_mac_get_connection_id_proto_size(&wps->mac);

        link_protocol_add(&connection->link_protocol, &link_proto_info, &link_err);
    }

    if (header_cfg.credit_fc_enabled == true) {
        link_proto_info.id = MAC_PROTO_ID_CREDIT_FC;
        link_proto_info.instance = &wps->mac;
        link_proto_info.send = wps_mac_send_credit_flow_control;
        link_proto_info.receive = wps_mac_receive_credit_flow_control;
        link_proto_info.size = wps_mac_get_credit_flow_control_proto_size(&wps->mac);

        link_protocol_add(&connection->link_protocol, &link_proto_info, &link_err);
    }
}

void wps_configure_header_acknowledge(wps_t *wps, wps_connection_t *connection, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    if (connection->auto_link_protocol == NULL) {
        return;
    }

    wps_header_cfg_t header_cfg = connection->header_cfg;

    uint16_t proto_auto_buffer_size = 0;
    link_error_t link_err = {0};
    link_protocol_info_t link_proto_info = {0};

    proto_auto_buffer_size = wps_get_connection_ack_header_size(wps, connection);
    link_protocol_init(connection->auto_link_protocol, proto_auto_buffer_size);

    if (header_cfg.rdo_enabled == true) {
        link_proto_info.id = MAC_PROTO_ID_RDO;
        link_proto_info.instance = &wps->mac;
        link_proto_info.send = wps_mac_send_rdo;
        link_proto_info.receive = wps_mac_receive_rdo;
        link_proto_info.size = wps_mac_get_rdo_proto_size(&wps->mac);

        link_protocol_add(connection->auto_link_protocol, &link_proto_info, &link_err);
    }

    switch (header_cfg.ranging_mode) {
    case WPS_RANGING_STANDALONE_INITIATOR:
        link_proto_info.id = MAC_PROTO_ID_RANGING_INITIATOR;
        link_proto_info.instance = &wps->mac;
        link_proto_info.send = NULL;
        link_proto_info.receive = wps_mac_receive_ranging_phases;
        link_proto_info.size = wps_mac_get_ranging_phases_proto_size(&wps->mac);
        link_protocol_add(connection->auto_link_protocol, &link_proto_info, &link_err);
        break;
    case WPS_RANGING_STANDALONE_RESPONDER:
        link_proto_info.id = MAC_PROTO_ID_RANGING_RESPONDER;
        link_proto_info.instance = &wps->mac;
        link_proto_info.send = wps_mac_send_ranging_phases;
        link_proto_info.receive = NULL;
        link_proto_info.size = wps_mac_get_ranging_phases_proto_size(&wps->mac);
        link_protocol_add(connection->auto_link_protocol, &link_proto_info, &link_err);
        break;
    default:
        break;
    }

    if (header_cfg.connection_id == true) {
        link_proto_info.id = MAC_PROTO_ID_CONNECTION_ID;
        link_proto_info.instance = &wps->mac;
        link_proto_info.send = wps_mac_send_connection_id_header_acknowledge;
        link_proto_info.receive = wps_mac_receive_connection_id_header_acknowledge;
        link_proto_info.size = wps_mac_get_connection_id_proto_size(&wps->mac);

        link_protocol_add(connection->auto_link_protocol, &link_proto_info, &link_err);
    }

    if (header_cfg.credit_fc_enabled == true) {
        link_proto_info.id = MAC_PROTO_ID_CREDIT_FC;
        link_proto_info.instance = &wps->mac;
        link_proto_info.send = wps_mac_send_credit_flow_control_header_acknowledge;
        link_proto_info.receive = wps_mac_receive_credit_flow_control_header_acknowledge;
        link_proto_info.size = wps_mac_get_credit_flow_control_proto_size(&wps->mac);

        link_protocol_add(connection->auto_link_protocol, &link_proto_info, &link_err);
    }
}

void wps_create_connection(wps_connection_t *connection, wps_node_t *node, wps_connection_cfg_t *config,
                           wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    CHECK_ERROR(connection->assigned_to_scheduler == false, err, WPS_CONNECTION_NOT_ASSIGNED_TO_SCHEDULER, return);

    connection->cfg = *config;
    connection->free_tx_queue = &node->free_tx_queue;
    connection->free_rx_queue = &node->free_rx_queue;
    connection->auto_sync_enable = true;
    connection->certification_mode_enabled = false;
    connection->currently_enabled = true;
    connection->is_tx_connection = node->cfg.local_address == config->source_address;

    connection->payload_size = config->frame_length - config->header_size - 1;
    /* Count queue size separate for TX and RX connection */
    if (connection->is_tx_connection) {
        node->tx_queues_size += config->fifo_buffer_size;
    } else {
        node->rx_queues_size += config->fifo_buffer_size;
        node->max_total_rx_data_size += config->fifo_buffer_size *
                                        (config->header_size + connection->payload_size + EMPTY_BYTE);
    }

    if (connection->is_main) {
        /* Set max payload and header size for main connection */
        if (connection->payload_size > node->max_payload_size) {
            node->max_payload_size = connection->payload_size;
        }

        if (config->header_size > node->max_header_size) {
            node->max_header_size = config->header_size;
        }
    } else {
        /* Set max payload and header size for auto connection */
        if (connection->payload_size > node->max_payload_size_auto) {
            node->max_payload_size_auto = connection->payload_size;
        }

        if (config->header_size > node->max_header_size_auto) {
            node->max_header_size_auto = config->header_size;
        }
    }

    xlayer_queue_init_queue(&connection->xlayer_queue, config->fifo_buffer_size, "connection queue");

    connection->rx_queue = &connection->xlayer_queue;
    connection->tx_success_callback = NULL;
    connection->tx_fail_callback = NULL;
    connection->tx_drop_callback = NULL;
    connection->rx_success_callback = NULL;
    connection->evt_callback = NULL;
    connection->total_cca_events = 0;
    connection->total_cca_fail_count = 0;
    connection->total_cca_tx_fail_count = 0;
    connection->total_pkt_dropped = 0;
    connection->ack_frame_enable = false;
    if (node->cfg.role == NETWORK_COORDINATOR && config->tx_sync_frame_on_syncing) {
        connection->send_sync_frame = true;
    } else {
        connection->send_sync_frame = false;
    }
    if (config->ranging_mode == WPS_RANGING_STANDALONE_INITIATOR ||
        config->ranging_mode == WPS_RANGING_STANDALONE_RESPONDER) {
        connection->ack_frame_enable = true;
    }
    /* Allow sending/receiving auto-reply frame when auto-reply connection does not exist in case:
     * - credit control flow is enabled
     */
    if (config->credit_fc_enabled == true) {
        connection->ack_frame_enable = true;
    }

    connection->pattern = NULL;

    link_fallback_init(&connection->link_fallback, NULL, 0);
#if WPS_ENABLE_PHY_STATS
#if SR1100
    link_lqi_init(&connection->lqi, LQI_MODE_1);
#else
    link_lqi_init(&connection->lqi, LQI_MODE_0);
#endif /* SR1100 */
#endif /* WPS_ENABLE_PHY_STATS */

    wps_connection_list_append_conn(&node->conn_list, &connection->conn_list_node, (void *)connection);
}

void wps_connection_set_timeslot(wps_connection_t *connection, wps_t *network, const int32_t *const timeslot_id,
                                 uint32_t nb_timeslots, wps_error_t *err)
{
    uint32_t id = 0;

    *err = WPS_NO_ERROR;

    for (uint32_t i = 0; i < nb_timeslots; ++i) {
        id = timeslot_id[i];

        if (is_main_timeslot(id)) {
            uint8_t count = network->mac.scheduler.schedule.timeslot[id].main_conn_list.connection_count;

            CHECK_ERROR(!(count < WPS_MAX_CONN_PER_TIMESLOT), err, WPS_TIMESLOT_CONN_LIMIT_REACHED_ERROR, return);
            network->mac.scheduler.schedule.timeslot[id].main_conn_list.connection[count] = connection;
            network->mac.scheduler.schedule.timeslot[id].main_conn_list.connection_count += 1;
            connection->is_main = true;
            connection->assigned_to_scheduler = true;
        } else {
            id = auto_reply_id_to_id(id);

            uint8_t count = network->mac.scheduler.schedule.timeslot[id].auto_conn_list.connection_count;

            CHECK_ERROR(!(count < WPS_MAX_CONN_PER_TIMESLOT), err, WPS_TIMESLOT_CONN_LIMIT_REACHED_ERROR, return);
            network->mac.scheduler.schedule.timeslot[id].auto_conn_list.connection[count] = connection;
            network->mac.scheduler.schedule.timeslot[id].auto_conn_list.connection_count += 1;
            connection->is_main = false;
            connection->assigned_to_scheduler = true;
        }
    }
}

void wps_connection_set_timeslot_priority(wps_connection_t *connection, wps_t *network,
                                          const int32_t *const timeslot_id, uint32_t nb_timeslots,
                                          const uint8_t *const slots_priority)
{
    uint32_t id = 0;

    for (uint32_t i = 0; i < nb_timeslots; ++i) {
        id = timeslot_id[i];

        if (is_main_timeslot(id)) {
            /* This need to be reduced by one since the main connection count was incremented after the timeslot init */
            uint8_t count = network->mac.scheduler.schedule.timeslot[id].main_conn_list.connection_count - 1;
            /* If priority is not provided for connection time slots, use global connection
             * priority.
             */
            if (slots_priority != NULL) {
                network->mac.scheduler.schedule.timeslot[id].main_conn_list.priority[count] = slots_priority[i];
            } else {
                network->mac.scheduler.schedule.timeslot[id].main_conn_list.priority[count] = connection->cfg.priority;
            }
        } else {
            id = auto_reply_id_to_id(id);
            /* This need to be reduced by one since the main connection count was incremented after the timeslot init */
            uint8_t count = network->mac.scheduler.schedule.timeslot[id].auto_conn_list.connection_count - 1;
            /* If priority is not provided for connection time slots, use global connection
             * priority.
             */
            if (slots_priority != NULL) {
                network->mac.scheduler.schedule.timeslot[id].auto_conn_list.priority[count] = slots_priority[i];
            } else {
                network->mac.scheduler.schedule.timeslot[id].auto_conn_list.priority[count] = connection->cfg.priority;
            }
        }
    }
}

void wps_connection_config_channel_20_48(wps_connection_t *connection, wps_node_t *node, uint8_t channel_x,
                                         channel_cfg_t *config, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    config_channel(connection, node, channel_x, config, PHY_RATE_IS_20_48, err);
}

void wps_connection_config_channel_40_96(wps_connection_t *connection, wps_node_t *node, uint8_t channel_x,
                                         channel_cfg_t *config, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    config_channel(connection, node, channel_x, config, PHY_RATE_IS_40_96, err);
}

void wps_connection_config_fallback_channel_20_48(wps_connection_t *connection, wps_node_t *node, uint8_t channel_x,
                                                  uint8_t fallback_index, channel_cfg_t *config, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    config_fallback_channel(connection, node, channel_x, fallback_index, config, PHY_RATE_IS_20_48, err);
}

void wps_connection_config_fallback_channel_40_96(wps_connection_t *connection, wps_node_t *node, uint8_t channel_x,
                                                  uint8_t fallback_index, channel_cfg_t *config, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    config_fallback_channel(connection, node, channel_x, fallback_index, config, PHY_RATE_IS_40_96, err);
}

void wps_connection_config_frame(wps_connection_t *connection, modulation_t modulation, chip_repetition_t chip_repet,
                                 fec_level_t fec, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

#if SR1100
    if (modulation == MODULATION_OOK) {
        /* For OOK, CHIPCODE is the same as IOOK, but CHIPREPET bit #0 should be 1. */
        modulation = MODULATION_IOOK;
        chip_repet = SET_CHIPREPE0(GET_CHIPREPE0(chip_repet) | 0x1);
    }
#endif

    connection->frame_cfg.modulation = modulation;
    connection->frame_cfg.chip_repet = chip_repet;
    connection->frame_cfg.fec = fec;
}

void wps_connection_enable_ack(wps_connection_t *connection, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    connection->ack_enable = true;
}

void wps_connection_disable_ack(wps_connection_t *connection, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    connection->ack_enable = false;
}

void wps_connection_enable_phases_aquisition(wps_connection_t *connection, phase_infos_t *phase_info_buffer,
                                             uint8_t max_sample_size, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    link_phase_init(&connection->link_phase, phase_info_buffer, max_sample_size);
}

void wps_connection_enable_stop_and_wait_arq(wps_connection_t *connection, uint16_t local_address, uint32_t retry,
                                             uint32_t deadline, wps_error_t *err)
{
    uint8_t board_seq;

    *err = WPS_NO_ERROR;
    CHECK_ERROR(connection->ack_enable == false, err, WPS_ACK_DISABLED_ERROR, return);

    if (local_address == connection->cfg.destination_address) {
        board_seq = RX_DEFAULT_SAW_INDEX;
    } else {
        board_seq = TX_DEFAULT_SAW_INDEX;
    }

    link_saw_arq_init(&connection->stop_and_wait_arq, deadline, retry, board_seq, true);
}

void wps_connection_disable_stop_and_wait_arq(wps_connection_t *connection, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    link_saw_arq_init(&connection->stop_and_wait_arq, 0, 0, false, false);
}

void wps_connection_enable_auto_sync(wps_connection_t *connection, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    connection->auto_sync_enable = true;
}

void wps_connection_disable_auto_sync(wps_connection_t *connection, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    connection->auto_sync_enable = false;
}

void wps_connection_disable_gain_loop(wps_connection_t *connection, uint8_t rx_gain, wps_error_t *err)
{
    *err = WPS_NO_ERROR;
    for (uint8_t i = 0; i < connection->max_channel_count; i++) {
        for (uint8_t j = 0; j < WPS_RADIO_COUNT; j++) {
            link_gain_loop_init(&connection->gain_loop[i][j], true, rx_gain);
        }
    }
}

void wps_connection_enable_gain_loop(wps_connection_t *connection, wps_error_t *err)
{
    *err = WPS_NO_ERROR;
    for (uint8_t i = 0; i < connection->max_channel_count; i++) {
        for (uint8_t j = 0; j < WPS_RADIO_COUNT; j++) {
            link_gain_loop_init(&connection->gain_loop[i][j], false, 0);
        }
    }
}

void wps_connection_optimize_latency(wps_connection_t *connection, uint8_t ack_payload_size, wps_node_t *node,
                                     bool extended_addr_en, bool extended_crc_en, wps_error_t *err)
{
    uint32_t isi_mitig_pause = link_tdma_sync_get_isi_mitigation_pauses(node->cfg.isi_mitig);
    uint32_t sfd_bits = link_tdma_get_sfd_length(isi_mitig_pause, node->cfg.sfd_cfg.sfd_length);
    uint32_t preamble_bits = node->cfg.preamble_len;
    bool iook = (connection->frame_cfg.modulation == MODULATION_IOOK) ? true : false;
    bool two_bit_ppm = (connection->frame_cfg.modulation == MODULATION_2BITPPM) ? true : false;
    uint8_t fec = FEC_TYPE_TO_RAW(connection->frame_cfg.fec);
    uint8_t address_bits = extended_addr_en ? 16 : 8;
    uint8_t crc_bits = extended_crc_en ? 31 : 16;
    uint8_t chip_repet = CHIP_REPET_TO_RAW(connection->frame_cfg.chip_repet);
    uint8_t isi_mitig = ISI_TYPE_TO_RAW(node->cfg.isi_mitig);

    *err = WPS_NO_ERROR;

    connection->empty_queue_max_delay =
        wps_utils_get_delayed_wakeup_event(preamble_bits, sfd_bits, iook, fec, two_bit_ppm, chip_repet, isi_mitig,
                                           address_bits, connection->payload_size + connection->cfg.header_size,
                                           crc_bits, connection->cca.retry_time_pll_cycles,
                                           connection->cca.max_try_count, connection->ack_enable, ack_payload_size);
}

void wps_init_rdo(wps_t *wps, uint16_t rollover_value, uint16_t step_ms, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    link_rdo_init(&wps->mac.link_rdo, rollover_value, step_ms);
}

void wps_enable_rdo(wps_t *wps, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    link_rdo_enable(&wps->mac.link_rdo);
}

void wps_disable_rdo(wps_t *wps, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    link_rdo_disable(&wps->mac.link_rdo);
}

void wps_enable_ddcm(wps_t *wps, uint16_t max_timeslot_offset, uint32_t sync_loss_max_duration_pll, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    link_ddcm_init(&wps->mac.link_ddcm, max_timeslot_offset, sync_loss_max_duration_pll);
}

void wps_disable_ddcm(wps_t *wps, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    link_ddcm_init(&wps->mac.link_ddcm, DDCM_DISABLE, 0);
}

void wps_connection_config_status(wps_connection_t *connection, connect_status_cfg_t *status_cfg, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    link_connect_status_init(&connection->connect_status, status_cfg);
}

void wps_connection_enable_credit_flow_ctrl(wps_connection_t *connection, bool has_main_ts, wps_error_t *err)
{
    *err = WPS_NO_ERROR;
    CHECK_ERROR((connection->ack_enable == false && has_main_ts == true), err, WPS_ACK_DISABLED_ERROR, return);
    link_credit_flow_ctrl_init(&connection->credit_flow_ctrl, true, WPS_MIN_QUEUE_SIZE);
}

void wps_connection_disable_credit_flow_ctrl(wps_connection_t *connection, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    link_credit_flow_ctrl_init(&connection->credit_flow_ctrl, false, 0);
}

void wps_set_tx_success_callback(wps_connection_t *connection, void (*callback)(void *conn, void *parg), void *parg,
                                 wps_error_t *err)
{
    CHECK_ERROR(connection == NULL, err, WPS_CONNECTION_NOT_ALLOCATED, return);
    connection->tx_success_callback = callback;
    connection->tx_success_parg_callback = parg;
}

void wps_set_tx_fail_callback(wps_connection_t *connection, void (*callback)(void *conn, void *parg), void *parg,
                              wps_error_t *err)
{
    CHECK_ERROR(connection == NULL, err, WPS_CONNECTION_NOT_ALLOCATED, return);
    connection->tx_fail_callback = callback;
    connection->tx_fail_parg_callback = parg;
}

void wps_set_tx_drop_callback(wps_connection_t *connection, void (*callback)(void *conn, void *parg), void *parg,
                              wps_error_t *err)
{
    CHECK_ERROR(connection == NULL, err, WPS_CONNECTION_NOT_ALLOCATED, return);
    connection->tx_drop_callback = callback;
    connection->tx_drop_parg_callback = parg;
}

void wps_set_rx_success_callback(wps_connection_t *connection, void (*callback)(void *conn, void *parg), void *parg,
                                 wps_error_t *err)
{
    CHECK_ERROR(connection == NULL, err, WPS_CONNECTION_NOT_ALLOCATED, return);
    connection->rx_success_callback = callback;
    connection->rx_success_parg_callback = parg;
}

void wps_set_ranging_data_ready_callback(wps_connection_t *connection, void (*callback)(void *conn, void *parg),
                                         void *parg, wps_error_t *err)
{
    CHECK_ERROR(connection == NULL, err, WPS_CONNECTION_NOT_ALLOCATED, return);
    connection->ranging_data_ready_callback = callback;
    connection->ranging_data_ready_parg_callback = parg;
}

void wps_set_event_callback(wps_connection_t *connection, void (*callback)(void *conn, void *parg), void *parg,
                            wps_error_t *err)
{
    CHECK_ERROR(connection == NULL, err, WPS_CONNECTION_NOT_ALLOCATED, return);
    connection->evt_callback = callback;
    connection->evt_parg_callback = parg;
}

void wps_connect(wps_t *wps, wps_error_t *err)
{
    wps_phy_cfg_t phy_cfg = {0};

    *err = WPS_NO_ERROR;
    CHECK_ERROR(wps->mac.signal == WPS_NOT_INIT, err, WPS_NOT_INIT_ERROR, return);
    CHECK_ERROR(!(wps->mac.signal == WPS_DISCONNECT), err, WPS_ALREADY_CONNECTED_ERROR, return);

    wps->mac.signal = WPS_CONNECT;

    uint8_t isi_mitigation_pauses = link_tdma_sync_get_isi_mitigation_pauses(wps->node.cfg.isi_mitig);

    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        phy_cfg.radio = &wps->node.radio[i].radio;
        phy_cfg.local_address = wps->node.cfg.local_address;
        phy_cfg.sfd_cfg = wps->node.cfg.sfd_cfg;
        phy_cfg.preamble_len_reg_val = link_tdma_get_preamble_reg_value(isi_mitigation_pauses,
                                                                        wps->node.cfg.preamble_len,
                                                                        wps->node.cfg.sfd_cfg.sfd_length);
        phy_cfg.sleep_lvl = wps->node.cfg.sleep_lvl;
        phy_cfg.crc_polynomial = wps->node.cfg.crc_polynomial;
        phy_cfg.rx_gain = wps->node.cfg.rx_gain;
#if SR1100
        phy_cfg.isi_indicator_enabled = wps->node.cfg.isi_indicator_enabled;
#endif
        wps_phy_init(&wps->phy[i], &phy_cfg);
    }

    wps_mac_reset(&wps->mac);
    wps_phy_connect(wps->phy);
}

void wps_disconnect(wps_t *wps, wps_error_t *err)
{
    *err = WPS_NO_ERROR;
    xlayer_request_info_t *request = NULL;

    CHECK_ERROR(wps->mac.signal == WPS_NOT_INIT, err, WPS_NOT_INIT_ERROR, return);
    CHECK_ERROR(wps->mac.signal == WPS_DISCONNECT, err, WPS_ALREADY_DISCONNECTED_ERROR, return);

    if (wps->mac.fast_sync_enabled && !link_tdma_sync_is_slave_synced(&wps->mac.tdma_sync)) {
        wps_phy_disconnect(wps->phy);
        wps->mac.signal = WPS_DISCONNECT;
    } else {
        request = circular_queue_get_free_slot(&wps->mac.request_queue);
        CHECK_ERROR(request == NULL, err, WPS_REQUEST_QUEUE_FULL, return);
        request->config = NULL;
        request->type = REQUEST_PHY_DISCONNECT;
        circular_queue_enqueue(&wps->mac.request_queue);

        uint64_t disconnect_timout_time = sr_util_get_tick_free_running_timer() +
                                          (DISCONNECT_TIMEOUT_MS * sr_util_get_free_running_timer_frequency_hz() /
                                           MS_TO_S_FACTOR);

        while (wps->mac.signal != WPS_DISCONNECT) {
            CHECK_ERROR(sr_util_get_tick_free_running_timer() > disconnect_timout_time, err,
                        WPS_DISCONNECT_TIMEOUT_ERROR, return);
        };
    }
}

void wps_reset(wps_t *wps, wps_error_t *err)
{
    *err = WPS_NO_ERROR;
    CHECK_ERROR(wps->mac.signal == WPS_DISCONNECT, err, WPS_ALREADY_DISCONNECTED_ERROR, return);

    wps_disconnect(wps, err);
    wps_connect(wps, err);
}

void wps_halt(wps_t *wps, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    (void)wps;
}

void wps_resume(wps_t *wps, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    (void)wps;
}

void wps_init_connection_throttle(wps_connection_t *connection, wps_error_t *err)
{
    *err = WPS_NO_ERROR;
    CHECK_ERROR(connection->pattern == NULL, err, WPS_THROTTLING_PATTERN_NOT_ALLOCATED, return);
    memset(connection->pattern, 1, WPS_PATTERN_THROTTLE_GRANULARITY * sizeof(bool));
    connection->pattern_count = 0;
    connection->pattern_total_count = WPS_PATTERN_THROTTLE_GRANULARITY;
    connection->active_ratio = 100;
}

void wps_set_active_ratio(wps_t *wps, wps_connection_t *connection, uint8_t ratio_percent, wps_error_t *err)
{
    xlayer_request_info_t *request = NULL;
    wps_schedule_ratio_cfg_t *schedule_ratio_cfg = circular_queue_get_free_slot(wps->mac.schedule_ratio_cfg_queue);

    *err = WPS_NO_ERROR;

    CHECK_ERROR(schedule_ratio_cfg == NULL, err, WPS_SCHEDULE_RATIO_REQUEST_QUEUE_FULL, return);
    request = circular_queue_get_free_slot(&wps->mac.request_queue);
    CHECK_ERROR(request == NULL, err, WPS_REQUEST_QUEUE_FULL, return);

    schedule_ratio_cfg->active_ratio = ratio_percent;
    schedule_ratio_cfg->pattern_total_count = generate_active_pattern(schedule_ratio_cfg->pattern_cfg, ratio_percent);
    schedule_ratio_cfg->pattern_current_count = 0;
    schedule_ratio_cfg->target_conn = connection;

    request->config = schedule_ratio_cfg;
    request->type = REQUEST_MAC_CHANGE_SCHEDULE_RATIO;
    circular_queue_enqueue(wps->mac.schedule_ratio_cfg_queue);
    circular_queue_enqueue(&wps->mac.request_queue);
}

void wps_get_free_slot(wps_connection_t *connection, uint8_t **payload, uint16_t size, wps_error_t *err)
{
    xlayer_frame_t *frame = NULL;

    *err = WPS_NO_ERROR;
    CHECK_ERROR(xlayer_queue_get_size(&connection->xlayer_queue) >=
                    xlayer_queue_get_max_size(&connection->xlayer_queue),
                err, WPS_QUEUE_FULL_ERROR, return);
    connection->tx_node = xlayer_queue_get_free_node(connection->free_tx_queue);
    CHECK_ERROR(connection->tx_node == NULL, err, WPS_QUEUE_FULL_ERROR, return);

    frame = &connection->tx_node->xlayer.frame;

    /* Allocate space for node data */
    frame->max_frame_size = connection->cfg.header_size + size + XLAYER_QUEUE_SPI_COMM_ADDITIONAL_BYTES;
    uint8_t *slot_data = xlayer_circular_data_allocate_space(connection->tx_data, frame->max_frame_size);

    if (slot_data == NULL) {
        xlayer_queue_free_node(connection->tx_node);
        connection->tx_node = NULL;
        *err = WPS_NOT_ENOUGH_MEMORY_ERROR;
        return;
    }

    xlayer_queue_set_tx_frame_buffer(frame, connection->cfg.header_size, slot_data);

    *payload = frame->payload_begin_it;
}

void wps_send(wps_connection_t *connection, const uint8_t *payload, uint8_t size, wps_error_t *err)
{
    xlayer_frame_t *frame = NULL;
    bool user_payload = false;

    *err = WPS_NO_ERROR;

    CHECK_ERROR(size > connection->payload_size && (connection->payload_size != 0), err, WPS_WRONG_TX_SIZE_ERROR,
                return);
    CHECK_ERROR((xlayer_queue_get_size(&connection->xlayer_queue) >=
                 xlayer_queue_get_max_size(&connection->xlayer_queue)),
                err, WPS_QUEUE_FULL_ERROR, return);

    if (connection->tx_node == NULL) {
        /* case where get free slot was not used first */
        connection->tx_node = xlayer_queue_get_free_node(connection->free_tx_queue);
        user_payload = true;

        /* if free node is not available, will return an error */
        CHECK_ERROR(connection->tx_node == NULL, err, WPS_QUEUE_FULL_ERROR, return);

        /* Allocate space for node data */
        frame = &connection->tx_node->xlayer.frame;
        frame->max_frame_size = connection->cfg.header_size + XLAYER_QUEUE_SPI_COMM_ADDITIONAL_BYTES;
        uint8_t *slot_data = xlayer_circular_data_allocate_space(connection->tx_data, frame->max_frame_size);

        if (slot_data == NULL) {
            xlayer_queue_free_node(connection->tx_node);
            connection->tx_node = NULL;
            *err = WPS_NOT_ENOUGH_MEMORY_ERROR;
            return;
        }
        xlayer_queue_set_tx_frame_buffer(frame, connection->cfg.header_size, slot_data);
    }

    frame = &connection->tx_node->xlayer.frame;
    frame->retry_count = 0;
    frame->time_stamp = connection->cfg.get_tick();
    frame->payload_memory_size = size;
    frame->header_memory_size = connection->cfg.header_size;
    frame->payload_memory = (uint8_t *)payload;
    frame->payload_begin_it = (uint8_t *)payload;
    frame->payload_end_it = frame->payload_begin_it + size;
    frame->user_payload = user_payload;
    if (xlayer_queue_enqueue_node(&connection->xlayer_queue, connection->tx_node) == false) {
        xlayer_circular_data_free_space(connection->tx_data, frame->header_memory, frame->max_frame_size);
        xlayer_queue_free_node(connection->tx_node);
    }
    connection->tx_node = NULL;
}

wps_rx_frame wps_read(wps_connection_t *connection, wps_error_t *err)
{
    wps_rx_frame frame_out = {0};

    *err = WPS_NO_ERROR;

    if (xlayer_queue_get_size(&connection->xlayer_queue) == 0) {
        *err = WPS_QUEUE_EMPTY_ERROR;
        frame_out.payload = NULL;
        frame_out.size = 0;
        return frame_out;
    }

    xlayer_t *frame = &xlayer_queue_get_node(&connection->xlayer_queue)->xlayer;

    frame_out.payload = (frame->frame.payload_begin_it);
    frame_out.size = frame->frame.payload_end_it - frame->frame.payload_begin_it;

    return frame_out;
}

void wps_read_done(wps_connection_t *connection, wps_error_t *err)
{
    xlayer_queue_node_t *node = NULL;

    node = xlayer_queue_dequeue_node(&connection->xlayer_queue);
    CHECK_ERROR(node == NULL, err, WPS_QUEUE_EMPTY_ERROR, return);

    wps_mac_xlayer_free_node_with_data(connection, node);

    *err = WPS_NO_ERROR;
}

wps_rx_frame wps_read_to_buffer(wps_connection_t *connection, uint8_t *payload, size_t max_size, wps_error_t *err)
{
    wps_rx_frame frame_out = {0};

    frame_out = wps_read(connection, err);
    if (*err != WPS_NO_ERROR) {
        frame_out.payload = NULL;
        frame_out.size = 0;
        return frame_out;
    }

    if (frame_out.size > max_size) {
        *err = WPS_WRONG_RX_SIZE_ERROR;
        frame_out.payload = NULL;
        frame_out.size = 0;
        return frame_out;
    }

    memcpy(payload, frame_out.payload, frame_out.size);

    wps_read_done(connection, err);
    if (*err != WPS_NO_ERROR) {
        frame_out.payload = NULL;
        frame_out.size = 0;
        return frame_out;
    }

    return frame_out;
}

uint16_t wps_get_read_payload_size(wps_connection_t *connection, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    CHECK_ERROR(xlayer_queue_get_size(&connection->xlayer_queue) == 0, err, WPS_QUEUE_EMPTY_ERROR, return 0);

    xlayer_t *frame = &xlayer_queue_get_node(&connection->xlayer_queue)->xlayer;

    return (frame->frame.payload_end_it - frame->frame.payload_begin_it);
}

uint32_t wps_get_fifo_size(wps_connection_t *connection)
{
    return xlayer_queue_get_size(&connection->xlayer_queue);
}

uint32_t wps_get_fifo_free_space(wps_connection_t *connection)
{
    return xlayer_queue_get_free_space(&connection->xlayer_queue);
}

bool wps_get_connect_status(wps_connection_t *connection)
{
    connect_status_t current_status = connection->connect_status.status;

    return (current_status == CONNECT_STATUS_CONNECTED) ? true : false;
}

wps_error_t wps_get_error(wps_connection_t *connection)
{
    wps_error_t error = connection->wps_error;

    connection->wps_error = WPS_NO_ERROR;
    return error;
}

wps_event_t wps_get_event(wps_connection_t *connection)
{
    wps_event_t event = connection->wps_event;

    connection->wps_event = WPS_EVENT_NONE;
    return event;
}

void wps_request_write_register(wps_t *wps, uint8_t starting_reg, uint16_t data, reg_write_cfg_t cfg, wps_error_t *err)
{
    xlayer_request_info_t *request = NULL;
    xlayer_write_request_info_t *write_request = circular_queue_get_free_slot(wps->mac.write_request_queue);

    *err = WPS_NO_ERROR;
    CHECK_ERROR(write_request == NULL, err, WPS_WRITE_REQUEST_QUEUE_FULL, return);

    request = circular_queue_get_free_slot(&wps->mac.request_queue);
    CHECK_ERROR(request == NULL, err, WPS_REQUEST_QUEUE_FULL, return);

    write_request->target_register = starting_reg;
    write_request->data = data;
    write_request->cfg = cfg;
    circular_queue_enqueue(wps->mac.write_request_queue);

    request->config = write_request;
    request->type = REQUEST_PHY_WRITE_REG;
    circular_queue_enqueue(&wps->mac.request_queue);
}

void wps_clear_write_register(wps_t *wps)
{
    wps_phy_clear_write_register(wps->phy);
}

void wps_request_read_register(wps_t *wps, uint8_t target_register, uint16_t *rx_buffer, volatile bool *xfer_cmplt,
                               wps_error_t *err)
{
    xlayer_request_info_t *request = NULL;
    xlayer_read_request_info_t *read_request = circular_queue_get_free_slot(wps->mac.read_request_queue);

    *err = WPS_NO_ERROR;
    CHECK_ERROR(read_request == NULL, err, WPS_READ_REQUEST_QUEUE_FULL, return);

    request = circular_queue_get_free_slot(&wps->mac.request_queue);
    CHECK_ERROR(request == NULL, err, WPS_REQUEST_QUEUE_FULL, return);

    *xfer_cmplt = false;
    read_request->rx_buffer = rx_buffer;
    read_request->target_register = target_register;
    read_request->xfer_cmplt = xfer_cmplt;
    circular_queue_enqueue(wps->mac.read_request_queue);

    request->config = read_request;
    request->type = REQUEST_PHY_READ_REG;
    circular_queue_enqueue(&wps->mac.request_queue);
}

void wps_process_callback(wps_t *wps)
{
    wps_callback_inst_t *callback = NULL;

    /* Process MAC connection statistics */
    wps_mac_statistics_process_data(&wps->mac.stats_process_data);

    while (circular_queue_is_empty(&wps->mac.callback_queue) == false) {
        callback = circular_queue_front(&wps->mac.callback_queue);
        if (callback != NULL && callback->func != NULL) {
            callback->func(callback->conn, callback->parg);
        }
        circular_queue_dequeue(&wps->mac.callback_queue);
    }

    wps->node.low_power_allowed = true;

    if (wps->node.low_power_callback) {
        wps->node.low_power_callback(&wps->node);
    }
}

#if WPS_RADIO_COUNT == 1

void wps_enable_fast_sync(wps_t *wps, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    wps_mac_enable_fast_sync(&wps->mac);
}

void wps_disable_fast_sync(wps_t *wps, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    wps_mac_disable_fast_sync(&wps->mac);
}

#elif WPS_RADIO_COUNT > 1

void wps_multi_init(wps_multi_cfg_t multi_cfg, chip_rate_cfg_t chip_rate, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    wps_multi_radio_init(multi_cfg, chip_rate);
}

#endif

uint32_t wps_get_phy_total_cca_events(wps_connection_t *connection)
{
    return connection->total_cca_events;
}

uint32_t wps_get_phy_total_cca_fail_count(wps_connection_t *connection)
{
    return connection->total_cca_fail_count;
}

uint32_t wps_get_phy_total_cca_tx_fail_count(wps_connection_t *connection)
{
    return connection->total_cca_tx_fail_count;
}

uint32_t wps_get_phy_total_pkt_dropped(wps_connection_t *connection)
{
    return connection->total_pkt_dropped;
}

wps_phase_frame wps_read_phase(wps_connection_t *connection, wps_error_t *err)
{
    wps_phase_frame phase_frame = {0};

    *err = WPS_NO_ERROR;

    phase_frame.size = link_phase_get_metrics_array(&connection->link_phase, &phase_frame.payload);
    CHECK_ERROR(phase_frame.payload == NULL, err, WPS_QUEUE_EMPTY_ERROR, return phase_frame);
    return phase_frame;
}

void wps_read_phase_done(wps_connection_t *connection, wps_error_t *err)
{
    *err = WPS_NO_ERROR;
    CHECK_ERROR(!link_phase_done(&connection->link_phase), err, WPS_QUEUE_EMPTY_ERROR, return);
}

uint8_t wps_get_channel_count(wps_t *wps, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    CHECK_ERROR(wps->channel_sequence.channel == NULL, err, WPS_CHANNEL_SEQUENCE_NOT_INITIALIZED_ERROR, return 0);
    return wps->channel_sequence.channel_number;
}

void wps_set_chip_repet(wps_connection_t *connection, chip_repetition_t chip_repet, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    CHECK_ERROR(connection == NULL, err, WPS_CONNECTION_NOT_ALLOCATED, return);

    connection->frame_cfg.chip_repet = chip_repet;
}

void wps_validate_connection_priority_in_schedule(wps_t *wps, wps_error_t *const err)
{
    wps_connection_t *current_main_conn = NULL;
    timeslot_conn_list_t *main_conn_list = NULL;
    wps_connection_t *current_auto_conn = NULL;
    timeslot_conn_list_t *auto_conn_list = NULL;

    /* Loop over the scheduler to find connection priority issue. */
    for (uint8_t i = 0; i < wps->mac.scheduler.schedule.size; i++) {
        /* Validate parameters for main connection */
        main_conn_list = &wps->mac.scheduler.schedule.timeslot[i].main_conn_list;

        if (main_conn_list->connection_count > 0) {
            current_main_conn = main_conn_list->connection[0];
            main_conn_list->max_payload_size = current_main_conn->payload_size;
            main_conn_list->max_cca_rx_timeout_offset = current_main_conn->cca.rx_timeout_offset_pll_cycles;
        }

        if (main_conn_list->connection_count > 1) {
            /* Check every connection if they have connection priority enable. */
            for (uint8_t j = 0; j < main_conn_list->connection_count; j++) {
                current_main_conn = main_conn_list->connection[j];
                uint8_t current_prio = current_main_conn->cfg.priority;
                bool priority_enable = false;

                /* Update the timeslot max payload size. */
                if (current_main_conn->payload_size > main_conn_list->max_payload_size) {
                    main_conn_list->max_payload_size = current_main_conn->payload_size;
                }

                /* Update the timeslot max CCA RX timeout offset. */
                if (current_main_conn->cca.rx_timeout_offset_pll_cycles > main_conn_list->max_cca_rx_timeout_offset) {
                    main_conn_list->max_cca_rx_timeout_offset = current_main_conn->cca.rx_timeout_offset_pll_cycles;
                }

                /* Check if priority is enabled by checking header link protocol. */
                for (uint8_t k = 0; k < current_main_conn->link_protocol.current_number_of_protocol; k++) {
                    if (current_main_conn->link_protocol.protocol_info[k].id == MAC_PROTO_ID_CONNECTION_ID) {
                        priority_enable = true;
                    }
                }
                /* Check if priority is enable on all connection that share same timeslot. */
                CHECK_ERROR(priority_enable == false, err, WPS_PRIO_NOT_ENABLE_ON_ALL_CONN_ERROR, return);
                /* Check if invalid priority */
                CHECK_ERROR((current_prio > WPS_MAX_CONN_PRIORITY) && current_main_conn->is_tx_connection, err,
                            WPS_NOT_ALLOWED_CONN_PRIORITY_CONFIGURATION_ERROR, return);
            }

            check_main_connection_priority_errors(wps->mac.scheduler.schedule.timeslot[i], err);
            CHECK_ERROR(*err != WPS_NO_ERROR, err, *err, return);
        }

        /* Validate parameters for auto connection. */
        auto_conn_list = &wps->mac.scheduler.schedule.timeslot[i].auto_conn_list;

        if (auto_conn_list->connection_count > 0) {
            current_auto_conn = auto_conn_list->connection[0];
            auto_conn_list->max_payload_size = current_auto_conn->payload_size;
            auto_conn_list->max_cca_rx_timeout_offset = current_auto_conn->cca.rx_timeout_offset_pll_cycles;
        }

        if (auto_conn_list->connection_count > 1) {
            /* Check every connection if they have connection priority enable. */
            for (uint8_t j = 0; j < auto_conn_list->connection_count; j++) {
                current_auto_conn = auto_conn_list->connection[j];
                uint8_t current_prio = current_auto_conn->cfg.priority;
                bool priority_enable = false;

                /* Update the timeslot max payload size. */
                if (current_auto_conn->payload_size > auto_conn_list->max_payload_size) {
                    auto_conn_list->max_payload_size = current_auto_conn->payload_size;
                }

                /* Update the timeslot max CCA RX timeout offset. */
                if (current_auto_conn->cca.rx_timeout_offset_pll_cycles > auto_conn_list->max_cca_rx_timeout_offset) {
                    auto_conn_list->max_cca_rx_timeout_offset = current_auto_conn->cca.rx_timeout_offset_pll_cycles;
                }

                /* Check if priority is enabled by checking header link protocol. */
                for (uint8_t k = 0; k < current_auto_conn->link_protocol.current_number_of_protocol; k++) {
                    if (current_auto_conn->link_protocol.protocol_info[k].id == MAC_PROTO_ID_CONNECTION_ID) {
                        priority_enable = true;
                    }
                }
                /* Check if priority is enable on all connection that share same timeslot. */
                CHECK_ERROR(priority_enable == false, err, WPS_PRIO_NOT_ENABLE_ON_ALL_CONN_ERROR, return);
                /* Check if invalid priority. */
                CHECK_ERROR((current_prio > WPS_MAX_CONN_PRIORITY) && current_main_conn->is_tx_connection, err,
                            WPS_NOT_ALLOWED_CONN_PRIORITY_CONFIGURATION_ERROR, return);
            }

            check_auto_connection_priority_errors(wps->mac.scheduler.schedule.timeslot[i], err);
            CHECK_ERROR(*err != WPS_NO_ERROR, err, *err, return);
        }
    }
}

/* PRIVATE FUNCTION ***********************************************************/
/** @brief Check if ID is main or auto timeslot
 *
 * @retval false  Connection is on auto-reply.
 * @retval true   Connection is on main.
 */
static bool is_main_timeslot(int8_t id)
{
    if (id & (BIT_AUTO_REPLY_TIMESLOT)) {
        return false;
    } else {
        return true;
    }
}

/** @brief Convert auto reply ID to timeslot ID.
 *
 * @return Timeslot ID.
 */
static uint32_t auto_reply_id_to_id(int8_t id)
{
    return (id & TIMESLOT_VALUE_MASK);
}

/** @brief Generate active pattern based on given ratio.
 *
 *  @note This will generate a bool array that properly
 *        distribute 1 and 0 through all the array size.
 *
 *  @note Number of active timeslot is the nominator of
 *        the reduced fraction (active_ratio / 100).
 *
 *  @note Total pattern size is the denominator of the
 *        reduced fraction (active_ratio / 100).
 *
 *
 *  @param[in] pattern       Allocated bool pattern array.
 *                           Size should be WPS_PATTERN_THROTTLE_GRANULARITY.
 *  @param[in] active_ratio  Active timeslot ratio, in percent.
 *  @return Total pattern size.
 */
static uint8_t generate_active_pattern(bool *pattern, uint8_t active_ratio)
{
    uint8_t current_gcd = wps_utils_gcd(active_ratio, PERCENT_DENOMINATOR);
    uint8_t active_elements = active_ratio / current_gcd;
    uint8_t total_number_of_val = PERCENT_DENOMINATOR / current_gcd;
    uint16_t pos = 0;

    memset(pattern, 0, total_number_of_val);

    for (uint8_t i = 0; i < active_elements; i++) {
        pos = ((i * total_number_of_val) / active_elements);
        pattern[pos % total_number_of_val] = 1;
    }

    return total_number_of_val;
}

/** @brief Find number of channel from the input channel_sequence.
 *
 *  @param[in] channel_sequence  Channel sequence instance.
 *  @return  Number of unique channel in the channel sequence.
 */
static uint8_t find_channel_count_from_sequence(channel_sequence_t *channel_sequence)
{
    uint8_t max_channel_index = 0;
    /* Find max number in channel sequence. */
    for (uint8_t i = 0; i < channel_sequence->sequence_size; i++) {
        if (max_channel_index < channel_sequence->channel[i]) {
            max_channel_index = channel_sequence->channel[i];
        }
    }

    if (max_channel_index == 0) {
        return 1;
    }

    uint8_t channel_seen[max_channel_index + 1];
    uint8_t unique_count = 0;

    memset(channel_seen, 0, max_channel_index + 1);
    for (uint8_t i = 0; i < channel_sequence->sequence_size; i++) {
        if (channel_seen[channel_sequence->channel[i]] == 0) { /* If the number is not yet seen */
            channel_seen[channel_sequence->channel[i]] = 1;    /* Mark it as seen */
            unique_count++;                                    /* Increment the unique count */
        }
    }

    return unique_count;
}

/** @brief Configure connection's RF channel.
 *
 *  Configure the receiver's filter, transmission power and power amplifier.
 *
 *  @param[in]  connection      Connection instance.
 *  @param[in]  node            Node instance.
 *  @param[in]  channel_x       Channel number.
 *  @param[in]  config          Channel configuration.
 *  @param[in]  cfg_40_96       Flag to indicate if 40.96 structure is to be used.
 *  @param[out] err             Pointer to the error code.
 */
static void config_channel(wps_connection_t *connection, wps_node_t *node, uint8_t channel_x, channel_cfg_t *config,
                           bool cfg_40_96, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    sr_phy_error_t phy_err = SR_SPECTRAL_ERROR_NONE;
    calib_vars_t *calib_vars;
    rf_channel_t *channel;

    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        calib_vars = cfg_40_96 ? node->radio[i].spectral_calib_vars_40_96 : node->radio[i].spectral_calib_vars_20_48;
        channel = cfg_40_96 ? &connection->channel_40_96[channel_x][i] : &connection->channel_20_48[channel_x][i];
        phy_err = config_spectrum_advance(calib_vars, config, channel);
        if (phy_err != SR_SPECTRAL_ERROR_NONE) {
            *err = WPS_SPECTRAL_CONFIG_FAILED;
            return;
        }
    }
}

/** @brief Configure connection's fallback RF channel.
 *
 *  Configure the receiver's filter, transmission power and power amplifier.
 *
 *  @param[in]  connection      Connection instance.
 *  @param[in]  node            Node instance.
 *  @param[in]  channel_x       Channel number.
 *  @param[in]  fallback_index  Fallback index.
 *  @param[in]  config          Channel configuration.
 *  @param[in]  cfg_40_96       Flag to indicate if 40.96 structure is to be used.
 *  @param[out] err             Pointer to the error code.
 */
static void config_fallback_channel(wps_connection_t *connection, wps_node_t *node, uint8_t channel_x,
                                    uint8_t fallback_index, channel_cfg_t *config, bool cfg_40_96, wps_error_t *err)
{
    *err = WPS_NO_ERROR;

    sr_phy_error_t phy_err = SR_SPECTRAL_ERROR_NONE;
    calib_vars_t *calib_vars;
    rf_channel_t *channel;

    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        calib_vars = cfg_40_96 ? node->radio[i].spectral_calib_vars_40_96 : node->radio[i].spectral_calib_vars_20_48;
        channel = cfg_40_96 ? &connection->fallback_channel_40_96[fallback_index][channel_x][i] :
                              &connection->fallback_channel_20_48[fallback_index][channel_x][i];
        phy_err = config_spectrum_advance(calib_vars, config, channel);
        if (phy_err != SR_SPECTRAL_ERROR_NONE) {
            *err = WPS_SPECTRAL_CONFIG_FAILED;
            return;
        }
    }
}

/** @brief Do main connection priority error checks.
 *
 * Some main connection fields must be identical in all connections assigned to the same timeslot,
 * we need to return an error to the SWC user if this is not the case for any of those fields.
 * This check needs to be done every time a connection is assigned to a time slot.
 *
 * @param[in]  timeslots  Timeslot to verify.
 * @param[out] err        Wireless Core error code.
 */
static void check_main_connection_priority_errors(timeslot_t timeslot, wps_error_t *const err)
{
    wps_connection_t *connection = NULL;
    wps_connection_t *first_connection = timeslot.main_conn_list.connection[0];

    for (uint8_t i = 1; i < timeslot.main_conn_list.connection_count; i++) {
        connection = timeslot.main_conn_list.connection[i];

        if (first_connection->is_tx_connection) {
            /* Both local address should match. */
            CHECK_ERROR((first_connection->cfg.source_address != connection->cfg.source_address), err,
                        WPS_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD_ERROR, return);
        } else {
            /* Both local address should match. */
            CHECK_ERROR((first_connection->cfg.destination_address != connection->cfg.destination_address), err,
                        WPS_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD_ERROR, return);
            /* If first connection does not have ack, radio won't be configured with acks. */
            CHECK_ERROR((!first_connection->ack_enable && connection->ack_enable), err,
                        WPS_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD_ERROR, return);
        }
        CHECK_ERROR(memcmp(&first_connection->link_phase, &connection->link_phase, sizeof(link_phase_t)), err,
                    WPS_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD_ERROR, return);
        CHECK_ERROR((first_connection->cfg.header_size != connection->cfg.header_size), err,
                    WPS_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD_ERROR, return);
        CHECK_ERROR(memcmp(&first_connection->link_protocol, &connection->link_protocol, sizeof(link_protocol_t)), err,
                    WPS_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD_ERROR, return);
#if !WPS_DISABLE_FRAGMENTATION
        CHECK_ERROR(memcmp(&first_connection->frag, &connection->frag, sizeof(frag_t)), err,
                    WPS_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD_ERROR, return);
#endif
        CHECK_ERROR(memcmp(&first_connection->frame_cfg, &connection->frame_cfg, sizeof(frame_cfg_t)), err,
                    WPS_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD_ERROR, return);
    }
}

/** @brief Do auto connection priority error checks.
 *
 * Some auto connection fields must be identical in all connections assigned to the same timeslot,
 * we need to return an error to the SWC user if this is not the case for any of those fields.
 * This check needs to be done every time a connection is assigned to a time slot.
 *
 * @param[in]  timeslots  Timeslot to verify.
 * @param[out] err        Wireless Core error code.
 */
static void check_auto_connection_priority_errors(timeslot_t timeslot, wps_error_t *const err)
{
    wps_connection_t *connection = NULL;
    wps_connection_t *first_connection = timeslot.auto_conn_list.connection[0];

    for (uint8_t i = 1; i < timeslot.main_conn_list.connection_count; i++) {
        connection = timeslot.main_conn_list.connection[i];
        /* Main connection source address should match auto reply destination address. */
        CHECK_ERROR((first_connection->cfg.destination_address != connection->cfg.source_address), err,
                    WPS_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD_ERROR, return);
        /* Main connection destination address should match auto reply source address. */
        CHECK_ERROR((first_connection->cfg.source_address != connection->cfg.destination_address), err,
                    WPS_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD_ERROR, return);
    }

    for (uint8_t i = 1; i < timeslot.auto_conn_list.connection_count; i++) {
        connection = timeslot.auto_conn_list.connection[i];

        /* Both destination and source address should match. */
        CHECK_ERROR((first_connection->cfg.destination_address != connection->cfg.destination_address), err,
                    WPS_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD_ERROR, return);
        CHECK_ERROR((first_connection->cfg.source_address != connection->cfg.source_address), err,
                    WPS_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD_ERROR, return);
        if (!first_connection->is_tx_connection) {
            /* If first connection does not have ack, radio won't be configured with acks. */
            CHECK_ERROR((!first_connection->ack_enable && connection->ack_enable), err,
                        WPS_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD_ERROR, return);
        }
        CHECK_ERROR(memcmp(&first_connection->link_phase, &connection->link_phase, sizeof(link_phase_t)), err,
                    WPS_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD_ERROR, return);
        CHECK_ERROR((first_connection->cfg.header_size != connection->cfg.header_size), err,
                    WPS_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD_ERROR, return);
        CHECK_ERROR(memcmp(&first_connection->link_protocol, &connection->link_protocol, sizeof(link_protocol_t)), err,
                    WPS_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD_ERROR, return);
#if !WPS_DISABLE_FRAGMENTATION
        CHECK_ERROR(memcmp(&first_connection->frag, &connection->frag, sizeof(frag_t)), err,
                    WPS_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD_ERROR, return);
#endif
        CHECK_ERROR(memcmp(&first_connection->frame_cfg, &connection->frame_cfg, sizeof(frame_cfg_t)), err,
                    WPS_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD_ERROR, return);
    }

    wps_connection_t *first_main_connection = timeslot.main_conn_list.connection[0];

    if (first_main_connection != NULL) {
        /* If credit control flow is enabled for the main connection, it must also be enabled for the auto-reply
         * connection.
         */
        CHECK_ERROR((first_main_connection->credit_flow_ctrl.enabled != first_connection->credit_flow_ctrl.enabled),
                    err, WPS_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD_ERROR, return);
    }
}
