/** @file wps_mac.c
 *  @brief Wireless protocol stack MAC.
 *
 *  @copyright Copyright (C) 2020 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "wps_mac.h"
#include "wps.h"
#include "wps_config.h"

/* CONSTANTS ******************************************************************/
#define SYNC_PLL_STARTUP_CYCLES        ((uint32_t)0x60)
#define SYNC_RX_SETUP_PLL_CYCLES       ((uint32_t)147)
#define MULTI_RADIO_BASE_IDX           0

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void process_main_frame_outcome(wps_mac_t *wps_mac);
static void process_auto_frame_outcome(wps_mac_t *wps_mac);
static void update_sync(wps_mac_t *wps_mac);
static void process_rx_main(wps_mac_t *wps_mac);
static void process_rx_auto(wps_mac_t *wps_mac);
static void process_tx_main(wps_mac_t *wps_mac);
static void process_tx_main_empty(wps_mac_t *wps_mac);
static void process_tx_auto(wps_mac_t *wps_mac);
static void process_tx_auto_empty(wps_mac_t *wps_mac);
static void prepare_frame(wps_mac_t *wps_mac, wps_phy_t *wps_phy);
static void prepare_tx_main(wps_mac_t *wps_mac);
static void prepare_rx_main(wps_mac_t *wps_mac);
static void config_tx(wps_mac_t *wps_mac, uint32_t next_channel);
static void config_rx(wps_mac_t *wps_mac, uint32_t next_channel);
static void prepare_tx_auto(wps_mac_t *wps_mac);
static void prepare_rx_auto(wps_mac_t *wps_mac);
static void process_next_timeslot(wps_mac_t *wps_mac);
static bool is_saw_arq_enable(wps_connection_t *connection);
static bool is_saw_arq_guaranteed_delivery_mode(saw_arq_t *saw_arq);
static bool no_payload_received(xlayer_t *current_queue);
static void extract_header_main(wps_mac_t *wps_mac, xlayer_t *current_queue);
static void extract_header_auto(wps_mac_t *wps_mac, xlayer_t *current_queue);
static void fill_header(wps_connection_t *connection, xlayer_t *current_queue);
static void fill_ack_header(wps_connection_t *connection, xlayer_t *current_queue);
static void flush_timeout_frames_before_sending(wps_mac_t *wps_mac, wps_connection_t *connection,
                                                xlayer_callback_t *callback);
static void flush_tx_frame(wps_mac_t *wps_mac, wps_connection_t *connection,
                           xlayer_callback_t *callback);
static bool send_done(wps_connection_t *connection);
#if !WPS_DISABLE_LINK_THROTTLE
static void handle_link_throttle(wps_mac_t *wps_mac, uint8_t *inc_count);
#endif /* !WPS_DISABLE_LINK_THROTTLE */
static inline wps_error_t get_status_error(link_connect_status_t *link_connect_status);
static void update_connect_status(wps_mac_t *wps_mac, wps_connection_t *conn, bool synced, bool ack_enabled,
                                  xlayer_t *xlayer);
static void process_pending_request(wps_mac_t *wps_mac, wps_phy_t *wps_phy);
static void process_schedule_request(wps_mac_t *wps_mac, xlayer_request_info_t *request);
static void process_write_request(wps_mac_t *wps_mac, wps_phy_t *wps_phy, xlayer_request_info_t *request);
static void process_read_request(wps_mac_t *wps_mac, wps_phy_t *wps_phy, xlayer_request_info_t *request);
static void process_disconnect_request(wps_mac_t *wps_mac, wps_phy_t *wps_phy);
static void reset_connections_parameters(wps_mac_t *wps_mac);

/* PUBLIC FUNCTIONS ***********************************************************/
void wps_mac_init(wps_mac_t *wps_mac, channel_sequence_t *channel_sequence, wps_mac_sync_cfg_t *sync_cfg,
                  uint16_t local_address, wps_role_t node_role, bool random_channel_sequence_enabled,
                  uint8_t network_id, uint32_t frame_lost_max_duration, uint8_t max_expected_payload_size,
                  uint8_t max_expected_header_size)
{
    wps_mac->local_address             = local_address;
    wps_mac->node_role                 = node_role;
    wps_mac->delay_in_last_timeslot    = false;
    wps_mac->last_timeslot_delay       = 0;
    wps_mac->max_expected_header_size  = max_expected_header_size;
    wps_mac->max_expected_payload_size = max_expected_payload_size;
    wps_mac->network_id                = network_id;
    memset(&wps_mac->muted_transfer_channel, 0, sizeof(rf_channel_t));

    /* Scheduler init */
    link_scheduler_init(&wps_mac->scheduler, wps_mac->local_address);
    link_scheduler_set_first_time_slot(&wps_mac->scheduler);
    link_scheduler_enable_tx(&wps_mac->scheduler);
    wps_mac->timeslot           = link_scheduler_get_current_timeslot(&wps_mac->scheduler);
    wps_mac->main_connection_id = 0;
    wps_mac->auto_connection_id = 0;
    wps_mac->main_connection =
        link_scheduler_get_current_main_connection(&wps_mac->scheduler,
                                                   wps_mac->main_connection_id);
    wps_mac->auto_connection =
        link_scheduler_get_current_auto_connection(&wps_mac->scheduler,
                                                   wps_mac->auto_connection_id);

    link_channel_hopping_init(&wps_mac->channel_hopping, channel_sequence,
                              random_channel_sequence_enabled, wps_mac->network_id);

    /* Sync module init */
    link_tdma_sync_init(&wps_mac->tdma_sync, sync_cfg->sleep_level, SYNC_RX_SETUP_PLL_CYCLES, frame_lost_max_duration,
                        sync_cfg->syncword_len, sync_cfg->preamble_len, SYNC_PLL_STARTUP_CYCLES, sync_cfg->isi_mitig,
                        sync_cfg->isi_mitig_pauses, local_address, wps_mac->fast_sync_enabled,
                        sync_cfg->tx_jitter_enabled, sync_cfg->chip_rate);

    wps_mac_statistics_init(&wps_mac->stats_process_data);
}

void wps_mac_reset(wps_mac_t *wps_mac)
{
    /* Sync module reset */
    wps_mac->tdma_sync.frame_lost_duration = 0;
    wps_mac->tdma_sync.sync_slave_offset = 0;
    wps_mac->tdma_sync.slave_sync_state  = STATE_SYNCING;
    wps_mac->output_signal.main_signal     = MAC_SIGNAL_WPS_EMPTY;
}

void wps_mac_enable_fast_sync(wps_mac_t *wps_mac)
{
    wps_mac->fast_sync_enabled = true;
}

void wps_mac_disable_fast_sync(wps_mac_t *wps_mac)
{
    wps_mac->fast_sync_enabled = false;
}

void wps_mac_phy_callback(void *mac, wps_phy_t *wps_phy)
{
    wps_mac_t *wps_mac = (wps_mac_t *)mac;

    wps_mac->input_signal.main_signal = wps_phy_get_main_signal(wps_phy);
    wps_mac->input_signal.auto_signal = wps_phy_get_auto_signal(wps_phy);

    switch (wps_mac->input_signal.main_signal) {
    case PHY_SIGNAL_CONFIG_COMPLETE:
        process_pending_request(mac, wps_phy);
        wps_mac->callback_context_switch();
        break;
    case PHY_SIGNAL_BLOCKING_CONFIG_DONE:
        process_pending_request(mac, wps_phy);
        break;
    case PHY_SIGNAL_FRAME_SENT_ACK:
    case PHY_SIGNAL_FRAME_SENT_NACK:
    case PHY_SIGNAL_FRAME_RECEIVED:
    case PHY_SIGNAL_FRAME_MISSED:
        process_main_frame_outcome(wps_mac);
        process_auto_frame_outcome(wps_mac);
        process_next_timeslot(wps_mac);
        prepare_frame(wps_mac, wps_phy);
        break;
    case PHY_SIGNAL_CONNECT:
        reset_connections_parameters(wps_mac);
        process_next_timeslot(wps_mac);
        prepare_frame(wps_mac, wps_phy);
        wps_mac->callback_context_switch();
        break;
    default:
        break;
    }
}

/* PRIVATE STATE FUNCTIONS ****************************************************/
/** @brief Process main frame outcome.
 *
 *  @param[in] mac MAC layer instance.
 */
static void process_main_frame_outcome(wps_mac_t *wps_mac)
{
    switch (wps_mac->input_signal.main_signal) {
    case PHY_SIGNAL_FRAME_SENT_ACK:
    case PHY_SIGNAL_FRAME_SENT_NACK:
        if (wps_mac->main_xlayer == &wps_mac->empty_frame_tx) {
            process_tx_main_empty(wps_mac);
        } else {
            process_tx_main(wps_mac);
        }
        break;
    case PHY_SIGNAL_FRAME_RECEIVED:
    case PHY_SIGNAL_FRAME_MISSED:
        update_sync(wps_mac);
        process_rx_main(wps_mac);
        break;
    default:
        break;
    }

    wps_mac_statistics_update_main_stats(wps_mac);
}

/** @brief Process auto frame outcome.
 *
 *  @param[in] mac MAC layer instance.
 */
static void process_auto_frame_outcome(wps_mac_t *wps_mac)
{
    if (wps_mac->auto_xlayer != NULL) {
        switch (wps_mac->input_signal.auto_signal) {
        case PHY_SIGNAL_FRAME_SENT_ACK:
        case PHY_SIGNAL_FRAME_SENT_NACK:
        case PHY_SIGNAL_FRAME_NOT_SENT:
            if (wps_mac->auto_xlayer == &wps_mac->empty_frame_tx) {
                process_tx_auto_empty(wps_mac);
            } else {
                process_tx_auto(wps_mac);
            }
            break;
        case PHY_SIGNAL_FRAME_RECEIVED:
        case PHY_SIGNAL_FRAME_MISSED:
            process_rx_auto(wps_mac);
            break;
        default:
            break;
        }

        wps_mac_statistics_update_auto_stats(wps_mac);
    }
}

/** @brief Update sync.
 *
 * This function handle sync module update.
 *
 *  @param[in] wps_mac  MAC structure.
 */
static void update_sync(wps_mac_t *wps_mac)
{
    if (wps_mac->output_signal.main_signal == MAC_SIGNAL_SYNCING) {
        wps_mac->config.rx_wait_time = 0;
    }

    if (wps_mac_is_network_node(wps_mac)) {
        if (!link_tdma_sync_is_slave_synced(&wps_mac->tdma_sync)) {
            link_tdma_sync_slave_find(&wps_mac->tdma_sync,
                                      wps_mac->main_xlayer->frame.frame_outcome,
                                      wps_mac->config.rx_wait_time, &wps_mac->main_connection->cca,
                                      wps_mac->config.rx_cca_retry_count);
        } else if (wps_mac->main_connection->source_address == wps_mac->syncing_address) {
            link_tdma_sync_slave_adjust(&wps_mac->tdma_sync,
                                        wps_mac->main_xlayer->frame.frame_outcome,
                                        wps_mac->config.rx_wait_time,
                                        &wps_mac->main_connection->cca,
                                        wps_mac->config.rx_cca_retry_count);
        }
    }
}

/** @brief Update the connection status for the current main connection.
 *
 *  @param[in] wps_mac      WPS MAC instance.
 *  @param[in] synced       Device is synced.
 *  @param[in] ack_enabled  Acknowledge enabled.
 */
static void update_connect_status(wps_mac_t *wps_mac, wps_connection_t *conn, bool synced, bool ack_enabled,
                                  xlayer_t *xlayer)
{
    if (conn == NULL) {
        return;
    }

    if (link_update_connect_status(&conn->connect_status, xlayer->frame.frame_outcome, synced, ack_enabled)) {
        wps_mac->config.callback_main.callback = conn->evt_callback;
        wps_mac->config.callback_main.parg_callback = conn->evt_parg_callback;
        wps_callback_enqueue(&wps_mac->callback_queue, &wps_mac->config.callback_main);

        connect_status_t status = conn->connect_status.status;

        conn->wps_event = (status == CONNECT_STATUS_CONNECTED) ? WPS_EVENT_CONNECT : WPS_EVENT_DISCONNECT;
    }
}

/** @brief Process reception of main frame.
 *
 * This function handles header extraction and operation after
 * the reception of valid main frame.
 *
 *  @param[in] wps_mac  MAC structure.
 */
static void process_rx_main(wps_mac_t *wps_mac)
{
    bool duplicate     = false;
    bool ack_enabled   = wps_mac->main_connection->ack_enable;
    bool synced        = wps_mac_is_network_node(wps_mac) ?
                             link_tdma_sync_is_slave_synced(&wps_mac->tdma_sync) :
                             true;
    wps_connection_t *connection;

    link_ddcm_pll_cycles_update(&wps_mac->link_ddcm,
                                link_tdma_sync_get_sleep_cycles(&wps_mac->tdma_sync));

    if (wps_mac->input_signal.main_signal != PHY_SIGNAL_FRAME_RECEIVED) {
        /* Update status of all connections in the timeslot (None of them received a packet). */
        for (uint8_t i = 0; i < wps_mac->timeslot->main_connection_count; i++) {
            connection = link_scheduler_get_current_main_connection(&wps_mac->scheduler, i);
            update_connect_status(wps_mac, connection, synced, ack_enabled, wps_mac->main_xlayer);
        }
        wps_mac_xlayer_free_node_with_data(wps_mac->main_connection, wps_mac->rx_node);
        wps_mac->rx_node        = NULL;
        wps_mac->output_signal.main_signal = MAC_SIGNAL_WPS_FRAME_RX_FAIL;
        /* Update LQI statistics for empty frame */
        wps_mac_statistics_update_main_conn_empty_frame(wps_mac);
        return;
    }

    /* Extract Header, Current connection might be adjusted if timeslot ID don't match*/
    extract_header_main(wps_mac, wps_mac->main_xlayer);

    /* Update connection status for the current connection.
     *
     * Note: For a connection's status to go from disconnected to connected, it needs to successfully receive packets.
     */
    update_connect_status(wps_mac, wps_mac->main_connection, synced, ack_enabled, wps_mac->main_xlayer);

    /* Copy application specific info */
    wps_mac->main_xlayer->config.rssi_raw = wps_mac->config.rssi_raw;
    wps_mac->main_xlayer->config.rnsi_raw = wps_mac->config.rnsi_raw;

    duplicate = link_saw_arq_is_rx_frame_duplicate(&wps_mac->main_connection->stop_and_wait_arq);
    /* Increment duplicate only if frame have payload and is not internal to the MAC */
    if (duplicate && !no_payload_received(wps_mac->main_xlayer)) {
        link_saw_arq_incr_duplicate_count(&wps_mac->main_connection->stop_and_wait_arq);
    }
    /* No payload received  or duclicate */
    if (no_payload_received(wps_mac->main_xlayer) || duplicate) {
        /* Frame received is internal to MAC */
        wps_mac_xlayer_free_node_with_data(wps_mac->main_connection, wps_mac->rx_node);
        wps_mac->rx_node        = NULL;
        wps_mac->output_signal.main_signal = MAC_SIGNAL_WPS_EMPTY;
        wps_mac_statistics_update_main_conn_empty_frame(wps_mac);
        return;
    }

    /* Update LQI statistics */
    wps_mac_statistics_update_main_conn(wps_mac);

    /* Frame is receive but there's no place for it in connection queue */
    if (!xlayer_queue_get_free_space(&wps_mac->main_connection->xlayer_queue)) {
        wps_mac_xlayer_free_node_with_data(wps_mac->main_connection, wps_mac->rx_node);
        wps_mac->rx_node        = NULL;
        wps_mac->config.callback_main.callback      = wps_mac->main_connection->evt_callback;
        wps_mac->config.callback_main.parg_callback = wps_mac->main_connection->evt_parg_callback;
        wps_mac->output_signal.main_signal          = MAC_SIGNAL_WPS_FRAME_RX_OVERRUN;
        wps_mac->main_connection->wps_error         = WPS_RX_OVERRUN_ERROR;
        wps_callback_enqueue(&wps_mac->callback_queue, &wps_mac->config.callback_main);
        return;
    }

    /* Frame successfully received */
    wps_mac->output_signal.main_signal     = MAC_SIGNAL_WPS_FRAME_RX_SUCCESS;
    wps_mac->config.callback_main.callback = wps_mac->main_connection->rx_success_callback;
    wps_mac->config.callback_main.parg_callback =
        wps_mac->main_connection->rx_success_parg_callback;
    xlayer_queue_enqueue_node(wps_mac->main_connection->rx_queue, wps_mac->rx_node);
    wps_callback_enqueue(&wps_mac->callback_queue, &wps_mac->config.callback_main);
    if (wps_mac->config.phases_info != NULL) {
        memcpy(&wps_mac->main_xlayer->config.phases_info, wps_mac->config.phases_info, sizeof(phase_info_t));
    }
}

/** @brief Process reception of auto reply frame.
 *
 * This function handles header extraction and operation after
 * the reception of valid auto reply frame.
 *
 *  @param[in] wps_mac  MAC structure.
 */
static void process_rx_auto(wps_mac_t *wps_mac)
{
    wps_connection_t *connection;

    link_ddcm_pll_cycles_update(&wps_mac->link_ddcm,
                                link_tdma_sync_get_sleep_cycles(&wps_mac->tdma_sync));

    if (wps_mac->input_signal.auto_signal != PHY_SIGNAL_FRAME_RECEIVED) {
        /* Update status of all auto connections in the timeslot (None of them received a packet). */
        for (uint8_t i = 0; i < wps_mac->timeslot->auto_connection_count; i++) {
            connection = link_scheduler_get_current_auto_connection(&wps_mac->scheduler, i);
            update_connect_status(wps_mac, connection, true, true, wps_mac->auto_xlayer);
        }
        wps_mac_xlayer_free_node_with_data(wps_mac->auto_connection, wps_mac->rx_node);
        wps_mac->rx_node                   = NULL;
        wps_mac->output_signal.auto_signal = MAC_SIGNAL_WPS_FRAME_RX_FAIL;
        /* Update LQI statistics for empty frame */
        wps_mac_statistics_update_auto_conn_empty_frame(wps_mac);
        return;
    }

    /* Extract Header, Current connection might be adjusted if timeslot ID don't match*/
    extract_header_auto(wps_mac, wps_mac->auto_xlayer);

    /* Update connection status for the current connection.
     *
     * Note: For a connection's status to go from disconnected to connected, it needs to successfully receive packets.
     */
    update_connect_status(wps_mac, wps_mac->auto_connection, true, false, wps_mac->auto_xlayer);

    /* Copy application specific info */
    wps_mac->auto_xlayer->config.rssi_raw = wps_mac->config.rssi_raw;
    wps_mac->auto_xlayer->config.rnsi_raw = wps_mac->config.rnsi_raw;

    /* No payload received */
    if (no_payload_received(wps_mac->auto_xlayer)) {
        /* Frame received is internal to MAC */
        wps_mac_xlayer_free_node_with_data(wps_mac->auto_connection, wps_mac->rx_node);
        wps_mac->rx_node                   = NULL;
        wps_mac->output_signal.auto_signal = MAC_SIGNAL_WPS_EMPTY;
        wps_mac_statistics_update_auto_conn_empty_frame(wps_mac);
        return;
    }

    /* Update LQI statistics */
    wps_mac_statistics_update_auto_conn(wps_mac);

    /* Frame is receive but there's no place for it in connection queue */
    if (!xlayer_queue_get_free_space(&wps_mac->auto_connection->xlayer_queue)) {
        wps_mac_xlayer_free_node_with_data(wps_mac->auto_connection, wps_mac->rx_node);
        wps_mac->rx_node                            = NULL;
        wps_mac->config.callback_auto.callback      = wps_mac->auto_connection->evt_callback;
        wps_mac->config.callback_auto.parg_callback = wps_mac->auto_connection->evt_parg_callback;
        wps_mac->output_signal.auto_signal          = MAC_SIGNAL_WPS_FRAME_RX_OVERRUN;
        wps_mac->auto_connection->wps_error         = WPS_RX_OVERRUN_ERROR;
    } else {
        /* Frame successfully received */
        wps_mac->output_signal.auto_signal     = MAC_SIGNAL_WPS_FRAME_RX_SUCCESS;
        wps_mac->config.callback_auto.callback = wps_mac->auto_connection->rx_success_callback;
        wps_mac->config.callback_auto.parg_callback =
            wps_mac->auto_connection->rx_success_parg_callback;
        xlayer_queue_enqueue_node(wps_mac->auto_connection->rx_queue, wps_mac->rx_node);
    }

    wps_callback_enqueue(&wps_mac->callback_queue, &wps_mac->config.callback_auto);
}

/** @brief Process transmission of main frame.
 *
 * This function handles operation after
 * the transmission of valid main frame.
 *
 *  @param[in] wps_mac  MAC structure.
 */
static void process_tx_main(wps_mac_t *wps_mac)
{
    wps_connection_t *connection;
    bool tx_success = (wps_mac->input_signal.main_signal == PHY_SIGNAL_FRAME_SENT_ACK) ||
                      (!wps_mac->main_connection->ack_enable);

    if (tx_success) {
        /* Update connection status for the current connection.
         *
         * Note: For a connection's status to go from disconnected to connected, it needs to successfully transmit
         * packets.
         */
        wps_mac->main_xlayer->config.rssi_raw = wps_mac->config.rssi_raw;
        wps_mac->main_xlayer->config.rnsi_raw = wps_mac->config.rnsi_raw;
        update_connect_status(wps_mac, wps_mac->main_connection, true, wps_mac->main_connection->ack_enable,
                              wps_mac->main_xlayer);
        wps_mac->output_signal.main_signal = MAC_SIGNAL_WPS_TX_SUCCESS;
        wps_mac->config.callback_main.callback = wps_mac->main_connection->tx_success_callback;
        wps_mac->config.callback_main.parg_callback = wps_mac->main_connection->tx_success_parg_callback;
        wps_callback_enqueue(&wps_mac->callback_queue, &wps_mac->config.callback_main);
        if (is_saw_arq_enable(wps_mac->main_connection)) {
            link_saw_arq_inc_seq_num(&wps_mac->main_connection->stop_and_wait_arq);
            link_credit_flow_ctrl_frame_ack_received(&wps_mac->main_connection->credit_flow_ctrl);
        }
        send_done(wps_mac->main_connection);
    } else {
        /* Update status of all connections in the timeslot (None of them transmitted a packet). */
        for (uint8_t i = 0; i < wps_mac->timeslot->main_connection_count; i++) {
            connection = link_scheduler_get_current_main_connection(&wps_mac->scheduler, i);
            update_connect_status(wps_mac, connection, true, connection->ack_enable, wps_mac->main_xlayer);
        }
        wps_mac->output_signal.main_signal = MAC_SIGNAL_WPS_TX_FAIL;
        wps_mac->config.callback_main.callback = wps_mac->main_connection->tx_fail_callback;
        wps_mac->config.callback_main.parg_callback = wps_mac->main_connection->tx_fail_parg_callback;
        wps_callback_enqueue(&wps_mac->callback_queue, &wps_mac->config.callback_main);
        if (!is_saw_arq_enable(wps_mac->main_connection)) {
            send_done(wps_mac->main_connection);
        }
    }

    /* Update LQI statistics */
    wps_mac_statistics_update_main_conn(wps_mac);

    link_ddcm_pll_cycles_update(&wps_mac->link_ddcm,
                                link_tdma_sync_get_sleep_cycles(&wps_mac->tdma_sync));
    link_ddcm_post_tx_update(&wps_mac->link_ddcm, wps_mac->config.cca_try_count,
                             wps_mac->config.cca_retry_time,
                             wps_mac->output_signal.main_signal == MAC_SIGNAL_WPS_TX_SUCCESS);
}

/** @brief Process transmission of empty main frame.
 *
 * This function handles operation after the transmission of empty main frame (sync frame or no packet to transmit).
 *
 *  @param[in] wps_mac  MAC structure.
 */
static void process_tx_main_empty(wps_mac_t *wps_mac)
{
    wps_connection_t *connection;

    /* Update status of all connections in the timeslot. */
    for (uint8_t i = 0; i < wps_mac->timeslot->main_connection_count; i++) {
        connection = link_scheduler_get_current_main_connection(&wps_mac->scheduler, i);
        update_connect_status(wps_mac, connection, true, connection->ack_enable, wps_mac->main_xlayer);
    }

    /* Sync frame was acknowledge */
    if (wps_mac->main_connection->first_tx_after_connect && wps_mac->node_role == NETWORK_COORDINATOR &&
        wps_get_connect_status(wps_mac->main_connection) == true) {
        wps_mac->main_connection->first_tx_after_connect = false;
    }

    if (wps_mac->input_signal.main_signal == PHY_SIGNAL_FRAME_SENT_ACK) {
        link_saw_arq_inc_seq_num(&wps_mac->main_connection->stop_and_wait_arq);
    }

    wps_mac->output_signal.main_signal = MAC_SIGNAL_WPS_EMPTY;

    /* Update LQI statistics for empty frame */
    wps_mac_statistics_update_main_conn_empty_frame(wps_mac);

    link_ddcm_pll_cycles_update(&wps_mac->link_ddcm, link_tdma_sync_get_sleep_cycles(&wps_mac->tdma_sync));
}

/** @brief Process transmission of auto reply frame.
 *
 * This function handles operation after
 * the transmission of valid auto reply frame.
 *
 *  @param[in] wps_mac  MAC structure.
 */
static void process_tx_auto(wps_mac_t *wps_mac)
{
    wps_connection_t *connection;

    if (wps_mac->auto_connection == NULL) {
        if (wps_mac->input_signal.auto_signal == PHY_SIGNAL_FRAME_SENT_NACK) {
            wps_mac->output_signal.auto_signal = MAC_SIGNAL_WPS_TX_SUCCESS;
        } else {
            wps_mac->output_signal.auto_signal = MAC_SIGNAL_WPS_TX_FAIL;
        }
        return;
    }

    if (wps_mac->input_signal.auto_signal == PHY_SIGNAL_FRAME_NOT_SENT) {
        /* Update status of all auto connections in the timeslot (None of them transmitted a packet). */
        for (uint8_t i = 0; i < wps_mac->timeslot->auto_connection_count; i++) {
            connection = link_scheduler_get_current_auto_connection(&wps_mac->scheduler, i);
            update_connect_status(wps_mac, connection, true, false, wps_mac->auto_xlayer);
        }
        wps_mac->auto_xlayer->frame.frame_outcome = FRAME_WAIT;
        wps_mac->output_signal.auto_signal = MAC_SIGNAL_WPS_TX_FAIL;
        wps_mac->config.callback_auto.callback = wps_mac->auto_connection->tx_fail_callback;
        wps_mac->config.callback_auto.parg_callback = wps_mac->auto_connection->tx_fail_parg_callback;
    } else {
        /* Update connection status for the current auto connection.
         *
         * Note: For a connection's status to go from disconnected to connected, it needs to successfully transmit
         * packets.
         */
        update_connect_status(wps_mac, wps_mac->auto_connection, true, false, wps_mac->auto_xlayer);
        wps_mac->auto_xlayer->frame.frame_outcome = FRAME_SENT_ACK_LOST;
        wps_mac->output_signal.auto_signal = MAC_SIGNAL_WPS_TX_SUCCESS;
        wps_mac->config.callback_auto.callback = wps_mac->auto_connection->tx_success_callback;
        wps_mac->config.callback_auto.parg_callback = wps_mac->auto_connection->tx_success_parg_callback;
        wps_callback_enqueue(&wps_mac->callback_queue, &wps_mac->config.callback_auto);
        link_credit_flow_ctrl_auto_frame_sent(&wps_mac->auto_connection->credit_flow_ctrl);
        send_done(wps_mac->auto_connection);
    }

    /* Update LQI statistics */
    wps_mac_statistics_update_auto_conn(wps_mac);

    link_ddcm_pll_cycles_update(&wps_mac->link_ddcm,
                                link_tdma_sync_get_sleep_cycles(&wps_mac->tdma_sync));
    link_ddcm_post_tx_update(&wps_mac->link_ddcm, wps_mac->config.cca_try_count,
                             wps_mac->config.cca_retry_time,
                             wps_mac->output_signal.auto_signal == MAC_SIGNAL_WPS_TX_SUCCESS);
}

/** @brief Process transmission of empty auto reply frame.
 *
 * This function handles operation after the transmission of empty auto reply frame (sync frame or no packet to
 * transmit).
 *
 *  @param[in] wps_mac  MAC structure.
 */
static void process_tx_auto_empty(wps_mac_t *wps_mac)
{
    wps_connection_t *connection;

    /* Update status of all auto connections in the timeslot (None of them transmitted a packet). */
    for (uint8_t i = 0; i < wps_mac->timeslot->auto_connection_count; i++) {
        connection = link_scheduler_get_current_auto_connection(&wps_mac->scheduler, i);
        update_connect_status(wps_mac, connection, true, false, wps_mac->auto_xlayer);
    }

    wps_mac->output_signal.auto_signal = MAC_SIGNAL_WPS_EMPTY;
    wps_mac->auto_xlayer->frame.frame_outcome = FRAME_SENT_ACK_LOST;

    /* Update LQI statistics for empty frame */
    wps_mac_statistics_update_auto_conn_empty_frame(wps_mac);

    link_ddcm_pll_cycles_update(&wps_mac->link_ddcm, link_tdma_sync_get_sleep_cycles(&wps_mac->tdma_sync));
}

/** @brief Prepare frame.
 *
 * This function fills the mac header and send commands to the PHY to execute transfers.
 *
 *  @param[in] wps_mac  MAC structure.
 */
static void prepare_frame(wps_mac_t *wps_mac, wps_phy_t *wps_phy)
{
    if (wps_mac_timeslots_is_current_timeslot_tx(wps_mac)) {
        /* TX timeslot */
        if (wps_mac->main_xlayer->frame.header_begin_it != NULL) {
            fill_header(wps_mac->main_connection, wps_mac->main_xlayer);
        }
    } else if (wps_mac->auto_connection != NULL) {
        /* TX timeslot auto reply */
        if (wps_mac->auto_xlayer->frame.header_begin_it != NULL) {
            fill_header(wps_mac->auto_connection, wps_mac->auto_xlayer);
        }
    } else if (wps_mac->main_connection->ack_frame_enable == true) {
        /* TX timeslot for non exist auto reply connection */
        if (wps_mac->auto_xlayer != NULL) {
            fill_ack_header(wps_mac->main_connection, wps_mac->auto_xlayer);
        }
    }

    if (wps_mac->output_signal.main_signal == MAC_SIGNAL_SYNCING) {
        wps_phy_set_input_signal(wps_phy, PHY_SIGNAL_SYNCING);
    } else {
        wps_phy_set_input_signal(wps_phy, PHY_SIGNAL_PREPARE_RADIO);
    }
    wps_phy_set_main_xlayer(wps_phy, wps_mac->main_xlayer, &wps_mac->config);
    wps_phy_set_auto_xlayer(wps_phy, wps_mac->auto_xlayer);
    wps_phy_prepare_frame(wps_phy);
}

/** @brief Prepare main frame transmission.
 *
 * This function prepares the MAC for main frame transmission.
 *
 *  @param[in] wps_mac  MAC structure.
 */
static void prepare_tx_main(wps_mac_t *wps_mac)
{
    uint32_t next_channel  = link_channel_hopping_get_channel(&wps_mac->channel_hopping);
    uint16_t rdo_value     = link_rdo_get_offset(&wps_mac->link_rdo);
    int32_t timeslot_delay = 0;
    sleep_lvl_t sleep_lvl = (wps_mac->input_signal.main_signal == PHY_SIGNAL_CONNECT) ?
                                SLEEP_IDLE :
                                wps_mac->scheduler.current_sleep_lvl;

    link_rdo_update_offset(&wps_mac->link_rdo);

    if (!wps_mac_is_network_node(wps_mac)) {
        timeslot_delay += link_ddcm_get_offset(&wps_mac->link_ddcm);
    }
    for (uint8_t i = 0; i < wps_mac->timeslot->main_connection_count; i++) {
        if (is_saw_arq_enable(wps_mac->timeslot->connection_main[i]) &&
            is_saw_arq_guaranteed_delivery_mode(&wps_mac->timeslot->connection_main[i]->stop_and_wait_arq) == false) {
            flush_timeout_frames_before_sending(wps_mac, wps_mac->timeslot->connection_main[i],
                                                &wps_mac->config.callback_main);
        }
        if (wps_mac->timeslot->connection_main[i]->tx_flush) {
            flush_tx_frame(wps_mac, wps_mac->timeslot->connection_main[i],
                           &wps_mac->config.callback_main);
        }
    }
    if (wps_mac->timeslot->main_connection_count > 1) {
        wps_mac->main_connection_id =
            wps_conn_priority_get_highest_main_conn_index(wps_mac->timeslot->connection_main,
                                                          wps_mac->timeslot->connection_main_priority,
                                                          wps_mac->timeslot->main_connection_count);
        wps_mac->main_connection =
            link_scheduler_get_current_main_connection(&wps_mac->scheduler,
                                                       wps_mac->main_connection_id);
    }
    wps_mac->main_xlayer = wps_mac_xlayer_get_xlayer_for_tx_main(wps_mac, wps_mac->main_connection);
    wps_mac->auto_xlayer = NULL;
    if (wps_mac->main_xlayer == &wps_mac->empty_frame_tx &&
        wps_mac->empty_frame_tx.frame.header_memory == NULL) {
        timeslot_delay += wps_mac->main_connection->empty_queue_max_delay;
    }
    if (wps_mac->delay_in_last_timeslot) {
        timeslot_delay -= wps_mac->last_timeslot_delay;
        wps_mac->delay_in_last_timeslot = false;
    }
    link_tdma_sync_update_tx(&wps_mac->tdma_sync,
                             timeslot_delay + link_scheduler_get_sleep_time(&wps_mac->scheduler) + rdo_value,
                             &wps_mac->main_connection->cca, sleep_lvl);
    if (wps_mac->main_xlayer == &wps_mac->empty_frame_tx &&
        wps_mac->empty_frame_tx.frame.header_memory == NULL) {
        wps_mac->last_timeslot_delay    = wps_mac->main_connection->empty_queue_max_delay;
        wps_mac->delay_in_last_timeslot = true;
    }
    wps_mac->output_signal.main_signal = MAC_SIGNAL_WPS_PREPARE_DONE;
    wps_mac->output_signal.auto_signal = MAC_SIGNAL_WPS_EMPTY;

    if (wps_mac->main_connection->connect_status.status == CONNECT_STATUS_DISCONNECTED) {
        /* Consider link broken, so maximize gain to increase chances to
         * resync at high attenuation/high range
         */
        for (uint8_t i = 0; i < wps_mac->main_connection->max_channel_count; i++) {
            for (uint8_t j = 0; j < WPS_RADIO_COUNT; j++) {
                link_gain_loop_reset_gain_index(&wps_mac->main_connection->gain_loop[i][j]);
            }
        }
    }

    config_tx(wps_mac, next_channel);
    wps_mac_xlayer_update_main_link_parameter(wps_mac, wps_mac->main_xlayer);
    wps_max_xlayer_update_sync(wps_mac, &wps_mac->config);
    update_xlayer_modem_feat(wps_mac, &wps_mac->config);
}

/** @brief Prepare main frame reception.
 *
 * This function prepares the MAC for main frame reception.
 *
 *  @param[in] wps_mac  MAC structure.
 */
static void prepare_rx_main(wps_mac_t *wps_mac)
{
    uint32_t next_channel  = link_channel_hopping_get_channel(&wps_mac->channel_hopping);
    uint16_t rdo_value     = link_rdo_get_offset(&wps_mac->link_rdo);
    int32_t timeslot_delay = 0;
    sleep_lvl_t sleep_lvl = (wps_mac->input_signal.main_signal == PHY_SIGNAL_CONNECT) ?
                                SLEEP_IDLE :
                                wps_mac->scheduler.current_sleep_lvl;

    link_rdo_update_offset(&wps_mac->link_rdo);

    if (wps_mac->delay_in_last_timeslot) {
        timeslot_delay -= wps_mac->last_timeslot_delay;
        wps_mac->delay_in_last_timeslot = false;
    }
    link_tdma_sync_update_rx(&wps_mac->tdma_sync,
                             timeslot_delay + link_scheduler_get_sleep_time(&wps_mac->scheduler) + rdo_value,
                             &wps_mac->main_connection->cca, sleep_lvl);
    wps_mac->output_signal.main_signal = MAC_SIGNAL_WPS_PREPARE_DONE;
    wps_mac->output_signal.auto_signal = MAC_SIGNAL_WPS_EMPTY;
    wps_mac->main_xlayer = wps_mac_xlayer_get_xlayer_for_rx(wps_mac, wps_mac->main_connection);
    wps_mac->auto_xlayer               = NULL;
    if ((!link_tdma_sync_is_slave_synced(&wps_mac->tdma_sync)) &&
        (wps_mac->node_role == NETWORK_NODE) &&
        (wps_mac->main_connection->source_address == wps_mac->syncing_address)) {
        if (wps_mac->fast_sync_enabled) {
            wps_mac->output_signal.main_signal = MAC_SIGNAL_SYNCING;
            next_channel                       = (wps_mac->channel_hopping.middle_channel_idx %
                            wps_mac->channel_hopping.channel_sequence->sequence_size);
        }
    }

    if (wps_mac->main_connection->connect_status.status == CONNECT_STATUS_DISCONNECTED) {
        /* Consider link broken, so maximize gain to increase chances to
         * resync at high attenuation/high range
         */
        for (uint8_t i = 0; i < wps_mac->main_connection->max_channel_count; i++) {
            for (uint8_t j = 0; j < WPS_RADIO_COUNT; j++) {
                link_gain_loop_reset_gain_index(&wps_mac->main_connection->gain_loop[i][j]);
            }
        }
    }

    config_rx(wps_mac, next_channel);
    wps_mac_xlayer_update_main_link_parameter(wps_mac, wps_mac->main_xlayer);
    wps_max_xlayer_update_sync(wps_mac, &wps_mac->config);
    update_xlayer_modem_feat(wps_mac, &wps_mac->config);
}

/** @brief Fill configuration fo TX.
 *
 * This function prepares the MAC for main frame transmission.
 *
 *  @param[in] wps_mac       MAC structure.
 *  @param[in] next_channel  Next channel.
 */
static void config_tx(wps_mac_t *wps_mac, uint32_t next_channel)
{
    uint8_t payload_size = wps_mac->main_xlayer->frame.payload_memory_size;
    uint8_t fallback_index;
    bool fallback_active = link_fallback_get_index(&wps_mac->main_connection->link_fallback, payload_size,
                                                   &fallback_index);
    uint8_t cca_max_try_count;

    if ((wps_mac->main_connection->cca.fbk_try_count != NULL) &&
        (wps_mac->main_connection->link_fallback.threshold != NULL) && (fallback_active == true) &&
        (payload_size != 0)) {
        cca_max_try_count = wps_mac->main_connection->cca.fbk_try_count[fallback_index];
    } else {
        cca_max_try_count = wps_mac->main_connection->cca.max_try_count;
    }
    if (cca_max_try_count == 0) {
        wps_mac->config.cca_threshold = WPS_DISABLE_CCA_THRESHOLD;
    } else {
        wps_mac->config.cca_threshold = wps_mac->main_connection->cca.threshold;
    }

    if (fallback_active == true && payload_size != 0) {
        wps_mac->config.channel =
            &wps_mac->main_connection->fallback_channel[fallback_index][next_channel][MULTI_RADIO_BASE_IDX];
    } else {
        wps_mac->config.channel = &wps_mac->main_connection->channel[next_channel][MULTI_RADIO_BASE_IDX];
    }

    /* When unsynced, mute all transfers that are not in a time slot of the lightest sleep level*/
    if (wps_mac->main_connection->connect_status.status == CONNECT_STATUS_DISCONNECTED &&
        wps_mac->scheduler.next_sleep_lvl != wps_mac->scheduler.schedule.lightest_sleep_lvl) {
        wps_mac->config.channel = &wps_mac->muted_transfer_channel;
    }

    wps_mac->config.cca_retry_time    = wps_mac->main_connection->cca.retry_time_pll_cycles;
    wps_mac->config.cca_max_try_count = cca_max_try_count;
    wps_mac->config.cca_try_count     = 0;
    wps_mac->config.cca_fail_action   = wps_mac->main_connection->cca.fail_action;
    wps_mac->config.cca_on_time = link_cca_get_on_time(&wps_mac->main_connection->cca);
    if (wps_mac->input_signal.main_signal == PHY_SIGNAL_CONNECT) {
        wps_mac->config.sleep_level = SLEEP_IDLE;
    } else {
        wps_mac->config.sleep_level = wps_mac->scheduler.current_sleep_lvl;
    }
    wps_mac->config.next_sleep_level = wps_mac->scheduler.next_sleep_lvl;
    wps_mac->config.gain_loop         = wps_mac->main_connection->gain_loop[wps_mac->channel_index];
    if (wps_mac->main_connection->ranging_mode != WPS_RANGING_DISABLED) {
        wps_mac->config.phases_info = &wps_mac->phase_data.local_phases_info;
    } else {
        wps_mac->config.phases_info = NULL;
    }
    wps_mac->config.isi_mitig         = wps_mac->tdma_sync.isi_mitig;
    wps_mac->config.expect_ack        = wps_mac->main_connection->ack_enable;
    wps_mac->config.certification_header_en = wps_mac->main_connection->certification_mode_enabled;
    wps_mac->config.expected_header_size    = wps_mac->max_expected_header_size;
    wps_mac->config.expected_payload_size = wps_mac->max_expected_payload_size;
    wps_mac->config.update_payload_buffer = wps_mac_xlayer_update_auto_reply_rx_payload_buffer;
}

/** @brief Fill configuration fo RX.
 *
 * This function prepares the MAC for main frame transmission.
 *
 *  @param[in] wps_mac       MAC structure.
 *  @param[in] next_channel  Next channel.
 */
static void config_rx(wps_mac_t *wps_mac, uint32_t next_channel)
{
    uint8_t payload_size = wps_mac->main_xlayer->frame.payload_memory_size;
    uint8_t fallback_index;
    bool fallback_active = link_fallback_get_index(&wps_mac->main_connection->link_fallback, payload_size,
                                                   &fallback_index);
    uint8_t cca_max_try_count;

    cca_max_try_count = wps_mac->main_connection->cca.max_try_count;
    if (cca_max_try_count == 0) {
        wps_mac->config.cca_threshold = WPS_DISABLE_CCA_THRESHOLD;
    } else {
        wps_mac->config.cca_threshold = wps_mac->main_connection->cca.threshold;
    }

    if (fallback_active == true) {
        wps_mac->config.channel =
            &wps_mac->main_connection->fallback_channel[fallback_index][next_channel][MULTI_RADIO_BASE_IDX];
    } else {
        wps_mac->config.channel = &wps_mac->main_connection->channel[next_channel][MULTI_RADIO_BASE_IDX];
    }

    /* When unsynced, mute all transfers that are not in a time slot of the lightest sleep level*/
    if (wps_mac->main_connection->connect_status.status == CONNECT_STATUS_DISCONNECTED &&
        wps_mac->scheduler.next_sleep_lvl != wps_mac->scheduler.schedule.lightest_sleep_lvl) {
        wps_mac->config.channel = &wps_mac->muted_transfer_channel;
    }

    wps_mac->config.cca_retry_time    = wps_mac->main_connection->cca.retry_time_pll_cycles;
    wps_mac->config.cca_max_try_count = cca_max_try_count;
    wps_mac->config.cca_try_count     = 0;
    wps_mac->config.cca_fail_action   = wps_mac->main_connection->cca.fail_action;
    if (wps_mac->input_signal.main_signal == PHY_SIGNAL_CONNECT) {
        wps_mac->config.sleep_level = SLEEP_IDLE;
    } else {
        wps_mac->config.sleep_level = wps_mac->scheduler.current_sleep_lvl;
    }
    wps_mac->config.next_sleep_level = wps_mac->scheduler.next_sleep_lvl;
    wps_mac->config.gain_loop         = wps_mac->main_connection->gain_loop[wps_mac->channel_index];
    if (wps_mac->main_connection->ranging_mode != WPS_RANGING_DISABLED) {
        wps_mac->config.phases_info = &wps_mac->phase_data.local_phases_info;
    } else {
        wps_mac->config.phases_info = NULL;
    }
    wps_mac->config.isi_mitig         = wps_mac->tdma_sync.isi_mitig;
    wps_mac->config.expect_ack        = wps_mac->main_connection->ack_enable;
    wps_mac->config.certification_header_en = wps_mac->main_connection->certification_mode_enabled;

    wps_mac->config.expected_payload_size   = wps_mac->max_expected_payload_size;
    wps_mac->config.expected_header_size = wps_mac->max_expected_header_size;
    wps_mac->config.update_payload_buffer = wps_mac_xlayer_update_main_rx_payload_buffer;
}

/** @brief Prepare auto reply frame transmission.
 *
 * This function prepares the MAC for auto reply frame transmission.
 *
 *  @param[in] wps_mac  MAC structure.
 */
static void prepare_tx_auto(wps_mac_t *wps_mac)
{
    for (uint8_t i = 0; i < wps_mac->timeslot->auto_connection_count; i++) {
        if (wps_mac->timeslot->connection_auto_reply[i]->tx_flush) {
            flush_tx_frame(wps_mac, wps_mac->timeslot->connection_auto_reply[i],
                           &wps_mac->config.callback_auto);
        }
    }
    if (wps_mac->timeslot->auto_connection_count > 1) {
        wps_mac->auto_connection_id =
            wps_conn_priority_get_highest_auto_conn_index(wps_mac->timeslot->connection_auto_reply,
                                                          wps_mac->timeslot->connection_auto_priority,
                                                          wps_mac->timeslot->auto_connection_count);
        wps_mac->auto_connection =
            link_scheduler_get_current_auto_connection(&wps_mac->scheduler,
                                                       wps_mac->auto_connection_id);
    }

    wps_mac->output_signal.auto_signal = MAC_SIGNAL_WPS_PREPARE_DONE;
    wps_mac->auto_xlayer = wps_mac_xlayer_get_xlayer_for_tx_auto(wps_mac, wps_mac->auto_connection);
    wps_mac_xlayer_update_auto_reply_link_parameter(wps_mac, wps_mac->auto_xlayer);
}

/** @brief Prepare auto reply frame reception.
 *
 * This function prepares the MAC for auto reply frame reception.
 *
 *  @param[in] wps_mac  MAC structure.
 */
static void prepare_rx_auto(wps_mac_t *wps_mac)
{
    wps_mac->output_signal.auto_signal = MAC_SIGNAL_WPS_PREPARE_DONE;
    wps_mac->auto_xlayer = wps_mac_xlayer_get_xlayer_for_rx(wps_mac, wps_mac->auto_connection);
    wps_mac_xlayer_update_auto_reply_link_parameter(wps_mac, wps_mac->auto_xlayer);
}

/** @brief Prepare an auto-reply frame transmission for a non-existent auto-reply connection.
 *
 * This function prepares the MAC for auto reply frame transmission.
 *
 *  @param[in] wps_mac  MAC structure.
 */
static void prepare_tx_empty_conn_auto(wps_mac_t *wps_mac)
{
    /* When auto-reply connection doesn't exist, use previous main connection from this slot */
    wps_mac->main_ack_connection_id = wps_mac->timeslot->last_used_main_connection;
    wps_connection_t *connection = link_scheduler_get_current_main_connection(&wps_mac->scheduler,
                                                                              wps_mac->main_ack_connection_id);

    wps_mac->output_signal.auto_signal = MAC_SIGNAL_WPS_PREPARE_DONE;
    wps_mac->auto_xlayer = wps_mac_xlayer_get_xlayer_for_empty_tx_auto(wps_mac, connection);
    wps_mac_xlayer_update_empty_auto_conn_reply_link_parameter(wps_mac, wps_mac->auto_xlayer);
}

/** @brief Prepare an auto-reply frame reception for a non-existent auto-reply connection.
 *
 * This function prepares the MAC for auto reply frame reception.
 *
 *  @param[in] wps_mac  MAC structure.
 */
static void prepare_rx_empty_conn_auto(wps_mac_t *wps_mac)
{
    wps_mac->output_signal.auto_signal = MAC_SIGNAL_WPS_PREPARE_DONE;
    wps_mac->auto_xlayer = wps_mac_xlayer_get_xlayer_for_empty_rx_auto(wps_mac, wps_mac->main_connection);
    wps_mac_xlayer_update_empty_auto_conn_reply_link_parameter(wps_mac, wps_mac->auto_xlayer);
}

/** @brief Process next time slot.
 *
 * This state get the next timeslot to handle and executes accordingy.
 *
 *  @param[in] wps_mac  MAC structure.
 */
static void process_next_timeslot(wps_mac_t *wps_mac)
{
    uint8_t inc_count;

    link_scheduler_reset_sleep_time(&wps_mac->scheduler);
    inc_count = link_scheduler_increment_time_slot(&wps_mac->scheduler);
    #if !WPS_DISABLE_LINK_THROTTLE
        handle_link_throttle(wps_mac, &inc_count);
    #endif
    link_channel_hopping_increment_sequence(&wps_mac->channel_hopping, inc_count);

    wps_mac->channel_index   = link_channel_hopping_get_channel(&wps_mac->channel_hopping);
    wps_mac->timeslot        = link_scheduler_get_current_timeslot(&wps_mac->scheduler);
    wps_mac->main_connection_id = 0;
    wps_mac->auto_connection_id = 0;
    wps_mac->main_connection = link_scheduler_get_current_main_connection(&wps_mac->scheduler, wps_mac->main_connection_id);
    wps_mac->auto_connection = link_scheduler_get_current_auto_connection(&wps_mac->scheduler, wps_mac->auto_connection_id);

    if (wps_mac_timeslots_is_current_timeslot_tx(wps_mac)) {
        prepare_tx_main(wps_mac);
    } else {
        prepare_rx_main(wps_mac);
    }
    if (wps_mac->auto_connection != NULL) {
        if (wps_mac_timeslots_is_current_auto_reply_timeslot_tx(wps_mac)) {
            prepare_tx_auto(wps_mac);
        } else {
            prepare_rx_auto(wps_mac);
        }
    } else if (wps_mac->main_connection->ack_frame_enable == true) {
        if (wps_mac_timeslots_is_current_timeslot_tx(wps_mac) == true) {
            prepare_rx_empty_conn_auto(wps_mac);
        } else {
            prepare_tx_empty_conn_auto(wps_mac);
        }
    }
}

/** @brief Return if stop and wait is enable or not.
 *
 *  @param wps_mac  MAC structure.
 *  @retval True   Stop and wait ARQ is enable.
 *  @retval False  Stop and wait ARQ is disable.
 */
static bool is_saw_arq_enable(wps_connection_t *connection)
{
    return connection->stop_and_wait_arq.enable;
}

/** @brief Return if stop and wait is in guaranteed delivery mode.
 *
 *  @note Guaranteed delivery mode is achieved when ttl_retry and ttl_ms are
 *        both set to 0.
 *
 *  @param[in] saw_arq  Stop and wait module.
 *  @retval True   Stop and wait ARQ is in guaranteed delivery mode.
 *  @retval False  Stop and wait ARQ is not in guaranteed delivery mode.
 */
static bool is_saw_arq_guaranteed_delivery_mode(saw_arq_t *saw_arq)
{
    return (saw_arq->ttl_tick == 0 && saw_arq->ttl_retries == 0);
}

/** @brief Extract the header fields from a received main frame.
 *
 *  @param[out] wps_mac        Frame header MAC instance.
 *  @param[in]  current_queue  xlayer header.
 */
static void extract_header_main(wps_mac_t *wps_mac, xlayer_t *current_queue)
{
    /* MAC should always be the first to extract */
    current_queue->frame.header_begin_it = current_queue->frame.header_memory;
    if (current_queue->frame.header_begin_it != NULL) {
        /* First byte should always be the radio automatic response */
        current_queue->frame.header_begin_it++;
        link_protocol_receive_buffer(&wps_mac->main_connection->link_protocol,
                                     wps_mac->main_xlayer->frame.header_begin_it,
                                     wps_mac->main_connection->header_size);
        wps_mac->main_connection =
            link_scheduler_get_current_main_connection(&wps_mac->scheduler,
                                                       wps_mac->main_connection_id);
        wps_mac->main_xlayer->frame.header_begin_it += wps_mac->main_connection->header_size;

        /* Store last used main connection id */
        wps_mac->timeslot->last_used_main_connection = wps_mac->main_connection_id;
    }
}

/** @brief Extract the header fields from a received auto reply frame.
 *
 *  @param[out] wps_mac        Frame header MAC instance.
 *  @param[in]  current_queue  xlayer header.
 */
static void extract_header_auto(wps_mac_t *wps_mac, xlayer_t *current_queue)
{
    wps_connection_t *connection;
    link_protocol_t *link_protocol;
    uint8_t header_size;

    /* If an auto-reply connection does not exist and a frame with a header is received,
     * use the main connection to parse the frame.
     */
    if (wps_mac->auto_connection != NULL) {
        connection = wps_mac->auto_connection;
        link_protocol = &connection->link_protocol;
        header_size = connection->header_size;
    } else {
        connection = link_scheduler_get_current_main_connection(&wps_mac->scheduler, wps_mac->main_ack_connection_id);
        link_protocol = connection->auto_link_protocol;
        header_size = connection->ack_header_size;
    }

    /* MAC should always be the first to extract */
    current_queue->frame.header_begin_it = current_queue->frame.header_memory;
    if (current_queue->frame.header_begin_it != NULL) {
        /* First byte should always be the radio automatic response */
        current_queue->frame.header_begin_it++;

        link_protocol_receive_buffer(link_protocol, wps_mac->auto_xlayer->frame.header_begin_it, header_size);
        wps_mac->auto_connection =
            link_scheduler_get_current_auto_connection(&wps_mac->scheduler,
                                                       wps_mac->auto_connection_id);
        wps_mac->auto_xlayer->frame.header_begin_it += header_size;
    }
}

/** @brief Fill the header fields for a TX node queue.
 *
 *  @param[in]  connection     Connection.
 *  @param[in]  current_queue  header xlayer.
 */
static void fill_header(wps_connection_t *connection, xlayer_t *current_queue)
{
    uint32_t size = 0;

    if (current_queue->frame.user_payload) {
        current_queue->frame.header_begin_it -= connection->header_size;
    } else {
        /* The header and payload data must be provided in contiguous memory blocks */
        current_queue->frame.header_begin_it = current_queue->frame.payload_begin_it -
                                               connection->header_size;
        current_queue->frame.header_end_it = current_queue->frame.header_begin_it +
                                             connection->header_size;
    }

    if (connection->certification_mode_enabled) {
        wps_mac_certification_fill_header(current_queue->frame.header_begin_it,
                                          connection->header_size);
    } else {
        link_protocol_send_buffer(&connection->link_protocol, current_queue->frame.header_begin_it,
                                  &size);
    }
}

/** @brief Fill the ACK frame header fields for a RX node queue.
 *
 *  @param[in]  connection     Connection.
 *  @param[in]  current_queue  header xlayer.
 */
static void fill_ack_header(wps_connection_t *connection, xlayer_t *current_queue)
{
    uint32_t size = 0;

    if (current_queue->frame.user_payload) {
        current_queue->frame.header_begin_it -= connection->ack_header_size;
    } else {
        /* The header and payload data must be provided in contiguous memory blocks */
        current_queue->frame.header_begin_it = current_queue->frame.payload_begin_it - connection->ack_header_size;
        current_queue->frame.header_end_it = current_queue->frame.header_begin_it + connection->ack_header_size;
    }

    if (connection->certification_mode_enabled) {
        wps_mac_certification_fill_header(current_queue->frame.header_begin_it, connection->ack_header_size);
    } else {
        link_protocol_send_buffer(connection->auto_link_protocol, current_queue->frame.header_begin_it, &size);
    }
}

/** @brief Fill the header fields for a TX node queue.
 *
 *  @param[in] current_queue  Current xlayer.
 *  @retval True   No payload have been received.
 *  @retval False  Payload have been received.
 */
static bool no_payload_received(xlayer_t *current_queue)
{
    return (current_queue->frame.header_begin_it == current_queue->frame.payload_end_it);
}

/** @brief Finish a transmission.
 *
 *  @param[in] connection wps_connection_t instance.
 *  @retval true   On success.
 *  @retval false  On error.
 */
static bool send_done(wps_connection_t *connection)
{
    xlayer_queue_node_t *node;

    if (connection == NULL) {
        return false;
    }

    connection->tx_flush = false;
    node = xlayer_queue_dequeue_node(&connection->xlayer_queue);
    xlayer_circular_data_free_space(connection->tx_data, node->xlayer.frame.header_memory,
                                    node->xlayer.frame.max_frame_size);
    xlayer_queue_free_node(node);
    if (connection->certification_mode_enabled) {
        wps_mac_certification_send(connection);
    }
    return true;
}

/** @brief  Check and flush timeout frame before sending to PHY.
 *
 *  @param wps_mac  WPS MAC instance.
 */
static void flush_timeout_frames_before_sending(wps_mac_t *wps_mac, wps_connection_t *connection,
                                                xlayer_callback_t *callback)
{
    bool timeout = false;
    xlayer_queue_node_t *xlayer_queue_node;

    do {
        xlayer_queue_node = xlayer_queue_get_node(&connection->xlayer_queue);
        if ((xlayer_queue_node != NULL) && (&xlayer_queue_node->xlayer != NULL)) {
            timeout = link_saw_arq_is_frame_timeout(&connection->stop_and_wait_arq,
                                                    xlayer_queue_node->xlayer.frame.time_stamp,
                                                    xlayer_queue_node->xlayer.frame.retry_count++,
                                                    connection->get_tick());
            if (timeout) {
                callback->callback      = connection->tx_drop_callback;
                callback->parg_callback = connection->tx_drop_parg_callback;
                wps_callback_enqueue(&wps_mac->callback_queue, &wps_mac->config.callback_main);
                wps_mac->output_signal.main_signal = MAC_SIGNAL_WPS_TX_DROP;
                wps_mac_statistics_update_tx_dropped_conn_stats(connection);
                send_done(connection);
            }
        } else {
            timeout = false;
        }
    } while (timeout);
}

/** @brief  Flush the next packet from the wps tx queue.
 *
 *  @param wps_mac  WPS MAC instance.
 */
static void flush_tx_frame(wps_mac_t *wps_mac, wps_connection_t *connection,
                           xlayer_callback_t *callback)
{
    xlayer_t *xlayer;

    xlayer = &xlayer_queue_get_node(&connection->xlayer_queue)->xlayer;

    if (xlayer != NULL) {
        callback->callback      = connection->tx_drop_callback;
        callback->parg_callback = connection->tx_drop_parg_callback;
        wps_callback_enqueue(&wps_mac->callback_queue, &wps_mac->config.callback_main);
        wps_mac->output_signal.main_signal = MAC_SIGNAL_WPS_TX_DROP;
        wps_mac_statistics_update_tx_dropped_conn_stats(connection);
        send_done(connection);
    }
}

#if !WPS_DISABLE_LINK_THROTTLE
/** @brief Handle link throttle.
 *
 *  @param[in] wps_mac    WPS MAC instance.
 *  @param[in] inc_count  Increment count.
 */
static void handle_link_throttle(wps_mac_t *wps_mac, uint8_t *inc_count)
{
    wps_connection_t *candidate_connection;
    timeslot_t *time_slot;
    bool ts_enabled;

    do {
        time_slot = link_scheduler_get_current_timeslot(&wps_mac->scheduler);
        for (uint8_t i = 0; i < time_slot->main_connection_count; i++) {
            candidate_connection                    = time_slot->connection_main[i];
            candidate_connection->currently_enabled = true;

            if (candidate_connection->pattern != NULL) {
                candidate_connection->pattern_count = (candidate_connection->pattern_count + 1) %
                                                      candidate_connection->pattern_total_count;

                candidate_connection->currently_enabled =
                    candidate_connection->pattern[candidate_connection->pattern_count];
            }
        }

        for (uint8_t i = 0; i < time_slot->auto_connection_count; i++) {
            candidate_connection                    = time_slot->connection_auto_reply[i];
            candidate_connection->currently_enabled = true;
        }

        ts_enabled = false;
        for (uint8_t i = 0; i < time_slot->main_connection_count; i++) {
            ts_enabled = time_slot->connection_main[i]->currently_enabled;
            if (ts_enabled == true) {
                break;
            }
        }

        if (ts_enabled == false) {
            *inc_count += link_scheduler_increment_time_slot(&wps_mac->scheduler);
        }

    } while (ts_enabled == false);
}
#endif /* !WPS_DISABLE_LINK_THROTTLE */

/** @brief Get the event associated with the current connection status.
 *
 *  @param[in] link_connect_status Link connection status module instance.
 *  @retval [WPS_EVENT_CONNECT]    if the status is connected.
 *  @retval [WPS_EVENT_DISCONNECT] if the status is disconnected.
 */
static inline wps_error_t get_status_error(link_connect_status_t *link_connect_status)
{
    return (link_connect_status->status == CONNECT_STATUS_CONNECTED) ? WPS_CONNECT_EVENT : WPS_DISCONNECT_EVENT;
}

/** @brief Process application pending request.
 *
 *  @param[in] request  WPS request info structure.
 */
static void process_pending_request(wps_mac_t *wps_mac, wps_phy_t *wps_phy)
{
    xlayer_request_info_t *request;

    request = circular_queue_front(&wps_mac->request_queue);
    if (request != NULL) {
        switch (request->type) {
        case REQUEST_MAC_CHANGE_SCHEDULE_RATIO: {
            process_schedule_request(wps_mac, request);
            break;
        }
        case REQUEST_PHY_WRITE_REG: {
            if (WPS_RADIO_COUNT == 1) {
                process_write_request(wps_mac, wps_phy, request);
            }
            break;
        }
        case REQUEST_PHY_READ_REG: {
            if (WPS_RADIO_COUNT == 1) {
                process_read_request(wps_mac, wps_phy, request);
            }
            break;
        }
        case REQUEST_PHY_DISCONNECT:
            process_disconnect_request(wps_mac, wps_phy);
            break;
        default:
            break;
        }
        circular_queue_dequeue(&wps_mac->request_queue);
    }
}

/** @brief Process MAC schedule change.
 *
 *  @note This allow the user to modify the active timeslot
 *        in the schedule of a given connection.
 *
 *  @note This process the request of type REQUEST_MAC_CHANGE_SCHEDULE_RATIO.
 *        Config structure should be of type wps_schedule_ratio_cfg_t.
 *
 *  @param[in] wps_mac  MAC structure.
 *  @param[in] request  WPS request info structure.
 */
static void process_schedule_request(wps_mac_t *wps_mac, xlayer_request_info_t *request)
{
    wps_schedule_ratio_cfg_t *schedule_ratio_cfg = (wps_schedule_ratio_cfg_t *)request->config;
    bool *pattern                                = schedule_ratio_cfg->pattern_cfg;

    if (pattern != NULL) {
        schedule_ratio_cfg->target_conn->active_ratio = schedule_ratio_cfg->active_ratio;
        schedule_ratio_cfg->target_conn->pattern_total_count =
            schedule_ratio_cfg->pattern_total_count;
        schedule_ratio_cfg->target_conn->pattern_count = schedule_ratio_cfg->pattern_current_count;
        memcpy(schedule_ratio_cfg->target_conn->pattern, pattern,
               schedule_ratio_cfg->pattern_total_count);
        circular_queue_dequeue(wps_mac->schedule_ratio_cfg_queue);
    }
}

/** @brief Process a write register request from application
 *
 *  @param[in] wps     WPS instance.
 *  @param[in] request WPS request info structure.
 */
static void process_write_request(wps_mac_t *wps_mac, wps_phy_t *wps_phy, xlayer_request_info_t *request)
{
    xlayer_write_request_info_t *write_request = (xlayer_write_request_info_t *)request->config;

    wps_phy_write_register(wps_phy, write_request->target_register, write_request->data,
                           write_request->cfg);

    circular_queue_dequeue(wps_mac->write_request_queue);
}

/** @brief Process a read register request from application
 *
 *  @param[in] wps     WPS instance.
 *  @param[in] request WPS request info structure.
 */
static void process_read_request(wps_mac_t *wps_mac, wps_phy_t *wps_phy, xlayer_request_info_t *request)
{
    xlayer_read_request_info_t *read_request = (xlayer_read_request_info_t *)request->config;

    wps_phy_read_register(wps_phy, read_request->target_register, read_request->rx_buffer,
                          read_request->xfer_cmplt);

    circular_queue_dequeue(wps_mac->read_request_queue);
}

/** @brief Process disconnection request.
 *
 *  @param[in] wps  WPS instance.
 */
static void process_disconnect_request(wps_mac_t *wps_mac, wps_phy_t *wps_phy)
{
    wps_phy_disconnect(wps_phy);

    /* Free MAC RX node in case a frame was received after the disconnect request */
    xlayer_queue_free_node(wps_mac->rx_node);

    wps_mac->signal = WPS_DISCONNECT;
}

/** @brief Reset specifics parameters of each connection.
 *
 *  @param[in] wps_mac  MAC structure.
 */
static void reset_connections_parameters(wps_mac_t *wps_mac)
{
    /* Loop over the scheduler to find each connection */
    for (uint8_t i = 0; i < wps_mac->scheduler.schedule.size; i++) {
        /* Reset target field for each main connection */
        for (uint8_t j = 0; j < wps_mac->scheduler.schedule.timeslot[i].main_connection_count; j++) {
            wps_connection_t *current_conn = wps_mac->scheduler.schedule.timeslot[i].connection_main[j];

            /* Reset first send after connect flag */
            current_conn->first_tx_after_connect = true;
        }
    }
}
