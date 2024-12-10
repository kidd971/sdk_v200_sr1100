/** @file  wps_mac_statistics.c
 *  @brief Wireless Protocol Stack MAC statistics.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "wps_mac.h"

/* CONSTANTS ******************************************************************/
/** @brief Buffer index to store statistics processing data for main connection.
 */
#define MAIN_CONN_STAT_INPUT_ID (0)
/** @brief Buffer index to store statistics processing data for auto connection.
 */
#define AUTO_CONN_STAT_INPUT_ID (1)

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
#if WPS_ENABLE_STATS_USED_TIMESLOTS || WPS_ENABLE_PHY_STATS || WPS_ENABLE_PHY_STATS_PER_BANDS
static inline void wps_mac_statistics_store_process_data(wps_mac_stats_entry_t *stats_data,
                                                         wps_mac_t *mac,
                                                         wps_connection_t *connection,
                                                         frame_outcome_t frame_outcome);
#endif

/* PUBLIC FUNCTIONS ***********************************************************/
#if WPS_ENABLE_STATS_USED_TIMESLOTS || WPS_ENABLE_PHY_STATS || WPS_ENABLE_PHY_STATS_PER_BANDS
void wps_mac_statistics_init(wps_mac_stats_t *stats_process_data)
{
    stats_process_data->no_space_counter = 0;
    for (uint8_t stat_idx = 0; stat_idx < WPS_MAC_STATISTICS_BUFFER_STAT_SIZE; stat_idx++) {
        stats_process_data->input_data[stat_idx].connection = NULL;
    }
}

void wps_mac_statistics_update_main_conn(void *wps_mac)
{
    wps_mac_t *mac = (wps_mac_t *)wps_mac;
    wps_mac_stats_entry_t *stats_data;

    stats_data = &mac->stats_process_data.input_data[MAIN_CONN_STAT_INPUT_ID];

    if (stats_data->connection != NULL) {
        mac->stats_process_data.no_space_counter++;
        return;
    }

    wps_mac_statistics_store_process_data(stats_data, mac, mac->main_connection,
                                          mac->main_xlayer->frame.frame_outcome);
    stats_data->empty_frame = false;
}

void wps_mac_statistics_update_auto_conn(void *wps_mac)
{
    wps_mac_t *mac = (wps_mac_t *)wps_mac;
    wps_mac_stats_entry_t *stats_data;

    stats_data = &mac->stats_process_data.input_data[AUTO_CONN_STAT_INPUT_ID];

    if (stats_data->connection != NULL) {
        mac->stats_process_data.no_space_counter++;
        return;
    }

    wps_mac_statistics_store_process_data(stats_data, mac, mac->auto_connection,
                                          mac->auto_xlayer->frame.frame_outcome);
    stats_data->empty_frame = false;
}

void wps_mac_statistics_update_main_conn_empty_frame(void *wps_mac)
{
    wps_mac_t *mac = (wps_mac_t *)wps_mac;
    wps_mac_stats_entry_t *stats_data;

    stats_data = &mac->stats_process_data.input_data[MAIN_CONN_STAT_INPUT_ID];

    if (stats_data->connection != NULL) {
        mac->stats_process_data.no_space_counter++;
        return;
    }

    wps_mac_statistics_store_process_data(stats_data, mac, mac->main_connection,
                                          mac->main_xlayer->frame.frame_outcome);
    stats_data->empty_frame = true;
}

void wps_mac_statistics_update_auto_conn_empty_frame(void *wps_mac)
{
    wps_mac_t *mac = (wps_mac_t *)wps_mac;
    wps_mac_stats_entry_t *stats_data;

    if (mac->auto_connection == NULL) {
        return;
    }

    stats_data = &mac->stats_process_data.input_data[AUTO_CONN_STAT_INPUT_ID];

    if (stats_data->connection != NULL) {
        mac->stats_process_data.no_space_counter++;
        return;
    }

    wps_mac_statistics_store_process_data(stats_data, mac, mac->auto_connection,
                                          mac->auto_xlayer->frame.frame_outcome);
    stats_data->empty_frame = true;
}

void wps_mac_statistics_process_data(wps_mac_stats_t *stats_process_data)
{
    wps_mac_stats_entry_t *stats_entry;

    for (uint8_t stat_idx = 0; stat_idx < WPS_MAC_STATISTICS_BUFFER_STAT_SIZE; stat_idx++) {
        stats_entry = &stats_process_data->input_data[stat_idx];
        if (stats_entry->connection == NULL) {
            continue;
        }

#if WPS_ENABLE_STATS_USED_TIMESLOTS
        if (stats_entry->empty_frame == false) {
            link_lqi_update(&stats_entry->connection->used_frame_lqi,
                            link_gain_loop_get_gain_index(
                                stats_entry->connection->gain_loop[stats_entry->channel_index]),
                            stats_entry->frame_outcome, stats_entry->rssi, stats_entry->rnsi,
                            stats_entry->phase_offset);
        }
#endif

#if WPS_ENABLE_PHY_STATS
        link_lqi_update(&stats_entry->connection->lqi,
                        link_gain_loop_get_gain_index(
                            stats_entry->connection->gain_loop[stats_entry->channel_index]),
                        stats_entry->frame_outcome, stats_entry->rssi, stats_entry->rnsi,
                        stats_entry->phase_offset);

#if WPS_ENABLE_PHY_STATS_PER_BANDS
        link_lqi_update(&stats_entry->connection->channel_lqi[stats_entry->channel_index],
                        link_gain_loop_get_gain_index(
                            stats_entry->connection->gain_loop[stats_entry->channel_index]),
                        stats_entry->frame_outcome, stats_entry->rssi, stats_entry->rnsi,
                        stats_entry->phase_offset);
#endif /* WPS_ENABLE_PHY_STATS_PER_BANDS */
#endif /* WPS_ENABLE_PHY_STATS */

        /* Set connection pointer to null to inform that processing statistics is done */
        stats_entry->connection = NULL;
    }
}
#endif

#if WPS_ENABLE_LINK_STATS
void wps_mac_statistics_update_main_stats(void *wps_mac)
{
    wps_mac_t *mac = (wps_mac_t *)wps_mac;
#if WPS_ENABLE_PHY_STATS_PER_BANDS
    uint8_t current_channel = mac->channel_index;
#endif

    switch (mac->output_signal.main_signal) {
    case MAC_SIGNAL_WPS_FRAME_RX_SUCCESS:
        mac->main_connection->wps_stats.rx_received++;
        mac->main_connection->wps_stats.rx_byte_received +=
            (mac->main_xlayer->frame.payload_end_it - mac->main_xlayer->frame.payload_begin_it);
#if WPS_ENABLE_PHY_STATS_PER_BANDS
        mac->main_connection->wps_chan_stats[current_channel].rx_received++;
        mac->main_connection->wps_chan_stats[current_channel].rx_byte_received +=
            (mac->main_xlayer->frame.payload_end_it - mac->main_xlayer->frame.payload_begin_it);
#endif
        break;
    case MAC_SIGNAL_WPS_FRAME_RX_OVERRUN:
        mac->main_connection->wps_stats.rx_overrun++;
#if WPS_ENABLE_PHY_STATS_PER_BANDS
        mac->main_connection->wps_chan_stats[current_channel].rx_overrun++;
#endif
        break;
    case MAC_SIGNAL_WPS_TX_SUCCESS:
        mac->main_connection->wps_stats.tx_success++;
        mac->main_connection->wps_stats.tx_byte_sent += (mac->main_xlayer->frame.payload_end_it -
                                                         mac->main_xlayer->frame.payload_begin_it);
        if (mac->main_connection->cca.enable) {
            if (mac->config.cca_try_count >= mac->config.cca_max_try_count) {
                mac->main_connection->total_cca_events++;
                mac->main_connection->wps_stats.cca_fail += mac->config.cca_try_count;
                mac->main_connection->total_cca_fail_count += mac->config.cca_try_count;
                mac->main_connection->wps_stats.cca_tx_fail++;
                mac->main_connection->total_cca_tx_fail_count++;
#if WPS_ENABLE_PHY_STATS_PER_BANDS
                mac->main_connection->wps_chan_stats[current_channel].cca_fail += mac->config.cca_try_count;
                mac->main_connection->wps_chan_stats[current_channel].cca_tx_fail++;
#endif
            } else if (mac->main_xlayer->frame.frame_outcome != FRAME_WAIT) {
                mac->main_connection->total_cca_events++;
                mac->main_connection->wps_stats.cca_fail += mac->config.cca_try_count;
                mac->main_connection->total_cca_fail_count += mac->config.cca_try_count;
                mac->main_connection->wps_stats.cca_pass++;
#if WPS_ENABLE_PHY_STATS_PER_BANDS
                mac->main_connection->wps_chan_stats[current_channel].cca_fail += mac->config.cca_try_count;
                mac->main_connection->wps_chan_stats[current_channel].cca_pass++;
#endif
            }
        }
#if WPS_ENABLE_PHY_STATS_PER_BANDS
        mac->main_connection->wps_chan_stats[current_channel].tx_success++;
        mac->main_connection->wps_chan_stats[current_channel].tx_byte_sent +=
            (mac->main_xlayer->frame.payload_end_it - mac->main_xlayer->frame.payload_begin_it);
#endif
        break;
    case MAC_SIGNAL_WPS_TX_FAIL:

        mac->main_connection->wps_stats.tx_fail++;
#if WPS_ENABLE_PHY_STATS_PER_BANDS
        mac->main_connection->wps_chan_stats[current_channel].tx_fail++;
#endif
        if (mac->main_connection->cca.enable) {
            if (mac->config.cca_try_count >= mac->config.cca_max_try_count) {
                mac->main_connection->total_cca_events++;
                mac->main_connection->wps_stats.cca_fail += mac->config.cca_try_count;
                mac->main_connection->total_cca_fail_count += mac->config.cca_try_count;
                mac->main_connection->wps_stats.cca_tx_fail++;
                mac->main_connection->total_cca_tx_fail_count++;
#if WPS_ENABLE_PHY_STATS_PER_BANDS
                mac->main_connection->wps_chan_stats[current_channel].cca_fail += mac->config.cca_try_count;
                mac->main_connection->wps_chan_stats[current_channel].cca_tx_fail++;
#endif
            } else if (mac->main_xlayer->frame.frame_outcome != FRAME_WAIT) {
                mac->main_connection->total_cca_events++;
                mac->main_connection->wps_stats.cca_fail += mac->config.cca_try_count;
                mac->main_connection->total_cca_fail_count += mac->config.cca_try_count;
                mac->main_connection->wps_stats.cca_pass++;
#if WPS_ENABLE_PHY_STATS_PER_BANDS
                mac->main_connection->wps_chan_stats[current_channel].cca_fail += mac->config.cca_try_count;
                mac->main_connection->wps_chan_stats[current_channel].cca_pass++;
#endif
            }
        }
        break;
    case MAC_SIGNAL_WPS_TX_DROP:
        mac->main_connection->wps_stats.tx_drop++;
        mac->main_connection->total_pkt_dropped++;
#if WPS_ENABLE_PHY_STATS_PER_BANDS
        mac->main_connection->wps_chan_stats[current_channel].tx_drop++;
#endif
        break;
    case MAC_SIGNAL_WPS_EMPTY:
        /* PHY NACK signal occurred but SAW has not yet trigger, handle CCA stats only. */
        if (mac->main_connection->cca.enable) {
            if (mac->config.cca_try_count >= mac->config.cca_max_try_count) {
                mac->main_connection->total_cca_events++;
                mac->main_connection->wps_stats.cca_fail += mac->config.cca_try_count;
                mac->main_connection->total_cca_fail_count += mac->config.cca_try_count;
                mac->main_connection->wps_stats.cca_tx_fail++;
                mac->main_connection->total_cca_tx_fail_count++;
#if WPS_ENABLE_PHY_STATS_PER_BANDS
                mac->main_connection->wps_chan_stats[current_channel].cca_fail += mac->config.cca_try_count;
                mac->main_connection->wps_chan_stats[current_channel].cca_tx_fail++;
#endif
            } else if (mac->main_xlayer->frame.frame_outcome != FRAME_WAIT) {
                mac->main_connection->total_cca_events++;
                mac->main_connection->wps_stats.cca_fail += mac->config.cca_try_count;
                mac->main_connection->total_cca_fail_count += mac->config.cca_try_count;
                mac->main_connection->wps_stats.cca_pass++;
#if WPS_ENABLE_PHY_STATS_PER_BANDS
                mac->main_connection->wps_chan_stats[current_channel].cca_fail += mac->config.cca_try_count;
                mac->main_connection->wps_chan_stats[current_channel].cca_pass++;
#endif
            }
        }
        break;
    default:
        break;
    }
}

void wps_mac_statistics_update_auto_stats(void *wps_mac)
{
    wps_mac_t *mac = (wps_mac_t *)wps_mac;
#if WPS_ENABLE_PHY_STATS_PER_BANDS
    uint8_t current_channel = mac->channel_index;
#endif

    if (mac->auto_connection == NULL) {
        switch (mac->output_signal.auto_signal) {
        case MAC_SIGNAL_WPS_FRAME_RX_OVERRUN:
        case MAC_SIGNAL_WPS_FRAME_RX_SUCCESS:
            mac->main_connection->wps_stats.rx_received++;
#if WPS_ENABLE_PHY_STATS_PER_BANDS
            mac->main_connection->wps_chan_stats[current_channel].rx_received++;
#endif
            break;
        case MAC_SIGNAL_WPS_TX_SUCCESS:
            if (mac->auto_xlayer->frame.header_memory_size != 0) {
                mac->main_connection->wps_stats.tx_success++;
#if WPS_ENABLE_PHY_STATS_PER_BANDS
                mac->main_connection->wps_chan_stats[current_channel].tx_success++;
#endif
            }
            break;
        case MAC_SIGNAL_WPS_EMPTY:
            if (mac->input_signal.auto_signal == PHY_SIGNAL_FRAME_RECEIVED) {
                mac->main_connection->wps_stats.rx_received++;
#if WPS_ENABLE_PHY_STATS_PER_BANDS
                mac->main_connection->wps_chan_stats[current_channel].rx_received++;
#endif
            } else {
                if (mac->auto_xlayer->frame.header_memory_size != 0) {
                    mac->main_connection->wps_stats.tx_success++;
#if WPS_ENABLE_PHY_STATS_PER_BANDS
                    mac->main_connection->wps_chan_stats[current_channel].tx_success++;
#endif
                }
            }
            break;
        default:
            break;
        }

        return;
    }

    switch (mac->output_signal.auto_signal) {
    case MAC_SIGNAL_WPS_FRAME_RX_SUCCESS:
        mac->auto_connection->wps_stats.rx_received++;
        mac->auto_connection->wps_stats.rx_byte_received +=
            (mac->auto_xlayer->frame.payload_end_it - mac->auto_xlayer->frame.payload_begin_it);
#if WPS_ENABLE_PHY_STATS_PER_BANDS
        mac->auto_connection->wps_chan_stats[current_channel].rx_received++;
        mac->auto_connection->wps_chan_stats[current_channel].rx_byte_received +=
            (mac->auto_xlayer->frame.payload_end_it - mac->auto_xlayer->frame.payload_begin_it);
#endif
        break;
    case MAC_SIGNAL_WPS_FRAME_RX_OVERRUN:
        mac->auto_connection->wps_stats.rx_overrun++;
#if WPS_ENABLE_PHY_STATS_PER_BANDS
        mac->auto_connection->wps_chan_stats[current_channel].rx_overrun++;
#endif
        break;
    case MAC_SIGNAL_WPS_TX_SUCCESS:
        mac->auto_connection->wps_stats.tx_success++;
        mac->auto_connection->wps_stats.tx_byte_sent += (mac->auto_xlayer->frame.payload_end_it -
                                                         mac->auto_xlayer->frame.payload_begin_it);
#if WPS_ENABLE_PHY_STATS_PER_BANDS
        mac->auto_connection->wps_chan_stats[current_channel].tx_success++;
        mac->auto_connection->wps_chan_stats[current_channel].tx_byte_sent +=
            (mac->auto_xlayer->frame.payload_end_it - mac->auto_xlayer->frame.payload_begin_it);
#endif
        break;
    case MAC_SIGNAL_WPS_TX_FAIL:
        mac->auto_connection->wps_stats.tx_fail++;
#if WPS_ENABLE_PHY_STATS_PER_BANDS
        mac->auto_connection->wps_chan_stats[current_channel].tx_fail++;
#endif
        break;
    case MAC_SIGNAL_WPS_TX_DROP:
        mac->auto_connection->wps_stats.tx_drop++;
        mac->auto_connection->total_pkt_dropped++;
#if WPS_ENABLE_PHY_STATS_PER_BANDS
        mac->auto_connection->wps_chan_stats[current_channel].tx_drop++;
#endif
        break;
    default:
        break;
    }
}
#endif /* WPS_ENABLE_LINK_STATS */

/* PRIVATE STATE FUNCTIONS ****************************************************/
#if WPS_ENABLE_STATS_USED_TIMESLOTS || WPS_ENABLE_PHY_STATS || WPS_ENABLE_PHY_STATS_PER_BANDS
/** @brief Store statistics processing data to buffer.
 *
 *  @param[in] stats_data        Statistics processing data instance.
 *  @param[in] mac               MAC Layer instance.
 *  @param[in] connection        Connection instance.
 *  @param[in] frame_outcome     Frame outcome value.
 */
static inline void wps_mac_statistics_store_process_data(wps_mac_stats_entry_t *stats_data,
                                                         wps_mac_t *mac,
                                                         wps_connection_t *connection,
                                                         frame_outcome_t frame_outcome)
{
    stats_data->connection    = connection;
    stats_data->channel_index = mac->channel_index;
    stats_data->frame_outcome = frame_outcome;
    stats_data->rssi          = mac->config.rssi_raw;
    stats_data->rnsi          = mac->config.rnsi_raw;
#if PHASE_OFFSET_BYTE_COUNT == 1
    stats_data->phase_offset[0] = mac->config.phase_offset[0];
#else
    memcpy(stats_data->phase_offset, mac->config.phase_offset, PHASE_OFFSET_BYTE_COUNT);
#endif
}
#endif
