/** @file  swc_stats.c
 *  @brief SPARK Wireless Core statistics.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "swc_stats.h"
#include <stdio.h>
#include "swc_api.h"
#include "wps_config.h"
#include "wps_stats.h"

/* PUBLIC FUNCTIONS ***********************************************************/
swc_statistics_t *swc_connection_update_stats(swc_connection_t *const conn)
{
/* TX stats */
#if WPS_ENABLE_PHY_STATS
    conn->stats.tx_timeslot_occurrence = wps_stats_get_phy_sent_count(conn->wps_conn_handle);
#endif

#if WPS_ENABLE_STATS_USED_TIMESLOTS
    conn->stats.packet_sent_and_acked_count = wps_stats_get_ack_frame_count(conn->wps_conn_handle);
    conn->stats.packet_sent_and_not_acked_count = wps_stats_get_nack_frame_count(conn->wps_conn_handle);
#endif

    uint32_t tx_count = conn->stats.packet_sent_and_acked_count + conn->stats.packet_sent_and_not_acked_count;

    conn->stats.tick_since_reset = conn->wps_conn_handle->get_tick() - conn->stats.tick_on_reset;
    conn->stats.bytes_sent = wps_stats_get_tx_byte_sent(conn->wps_conn_handle);
    conn->stats.tx_data_rate_bps = ((uint64_t)conn->wps_conn_handle->tick_frequency_hz *
                                    (uint64_t)conn->stats.bytes_sent * 8) /
                                   conn->stats.tick_since_reset;
    conn->stats.bytes_received = wps_stats_get_rx_byte_received(conn->wps_conn_handle);
    conn->stats.rx_data_rate_bps = ((uint64_t)conn->wps_conn_handle->tick_frequency_hz *
                                    (uint64_t)conn->stats.bytes_received * 8) /
                                   conn->stats.tick_since_reset;

    if ((conn->stats.bytes_sent > INT32_MAX) || (conn->stats.bytes_received > INT32_MAX)) {
        /* Reset stats to avoid overflow. */
        conn->stats.tick_on_reset = conn->wps_conn_handle->get_tick();
        wps_stats_reset_tx_byte_sent(conn->wps_conn_handle);
        wps_stats_reset_rx_byte_received(conn->wps_conn_handle);
    }

    /* tx_timeslot_occurrence value can be lower than tx_count value if after updating tx_timeslot_occurrence,
     * and before updating packet_sent_and_acked_count and/or packet_sent_and_not_acked_count, a new packet is
     * transmitted. In such case, adjust tx_timeslot_occurrence to reflect the new TX event(s).
     */
    if (tx_count > conn->stats.tx_timeslot_occurrence) {
        conn->stats.tx_timeslot_occurrence = tx_count;
    }

#if WPS_ENABLE_PHY_STATS && WPS_ENABLE_STATS_USED_TIMESLOTS
    conn->stats.no_packet_tranmission_count = conn->stats.tx_timeslot_occurrence -
                                              conn->stats.packet_sent_and_acked_count -
                                              conn->stats.packet_sent_and_not_acked_count;
#endif

    if (conn->stats.tx_timeslot_occurrence != 0) {
        conn->stats.tx_used_capacity_pc = ((float)tx_count / (conn->stats.tx_timeslot_occurrence)) * 100;
    }

#if WPS_ENABLE_LINK_STATS
    conn->stats.packet_dropped_count = wps_stats_get_payload_dropped_count(conn->wps_conn_handle);
    conn->stats.cca_pass_count = wps_stats_get_phy_cca_pass_count(conn->wps_conn_handle);
    conn->stats.cca_fail_count = wps_stats_get_phy_cca_tx_fail(conn->wps_conn_handle);
    conn->stats.cca_try_fail_count = wps_stats_get_phy_cca_fail(conn->wps_conn_handle);
#endif

/* RX stats */
#if WPS_ENABLE_PHY_STATS
    conn->stats.packet_rejected_count = wps_stats_get_phy_rejected_frame_count(conn->wps_conn_handle);
    conn->stats.rx_timeslot_occurrence = conn->stats.packet_rejected_count +
                                         wps_stats_get_phy_received_frame_count(conn->wps_conn_handle) +
                                         wps_stats_get_phy_missing_frame_count(conn->wps_conn_handle);
    conn->stats.rssi_avg = wps_stats_get_phy_rssi_avg(conn->wps_conn_handle);
    conn->stats.rnsi_avg = wps_stats_get_phy_rnsi_avg(conn->wps_conn_handle);
    conn->stats.link_margin_avg = wps_stats_get_phy_margin_avg(conn->wps_conn_handle);
    conn->stats.rssi_avg_raw = wps_stats_get_phy_rssi_avg_raw(conn->wps_conn_handle);
    conn->stats.rnsi_avg_raw = wps_stats_get_phy_rnsi_avg_raw(conn->wps_conn_handle);
#endif

#if WPS_ENABLE_STATS_USED_TIMESLOTS
    conn->stats.packet_successfully_received_count = wps_stats_get_received_frame_count(conn->wps_conn_handle);
#endif

    /* rx_timeslot_occurrence value can be lower than packet_successfully_received_count value if after updating
     * rx_timeslot_occurrence, and before updating packet_successfully_received_count, a new packet is received.
     * In such case, adjust rx_timeslot_occurrence to reflect the new RX event(s).
     */
    if (conn->stats.packet_successfully_received_count > conn->stats.rx_timeslot_occurrence) {
        conn->stats.rx_timeslot_occurrence = conn->stats.packet_successfully_received_count;
    }

#if WPS_ENABLE_PHY_STATS && WPS_ENABLE_STATS_USED_TIMESLOTS
    conn->stats.no_packet_reception_count = conn->stats.rx_timeslot_occurrence -
                                            conn->stats.packet_successfully_received_count;
#endif

    conn->stats.packet_duplicated_count = wps_stats_get_duplicated_frame_count(conn->wps_conn_handle);
#if WPS_ENABLE_LINK_STATS
    conn->stats.packet_overrun_count = wps_stats_get_payload_overrun_count(conn->wps_conn_handle);
#endif

    conn->stats.packet_ack_data_received_count = wps_stats_get_payload_received_count(conn->wps_conn_handle);
    conn->stats.packet_ack_data_send_count = wps_stats_get_payload_success_count(conn->wps_conn_handle);

    return &conn->stats;
}

#if WPS_ENABLE_PHY_STATS_PER_BANDS
swc_statistics_t *swc_connection_update_stats_per_channel(swc_connection_t *const conn, const uint8_t channel_number)
{
    if (channel_number >= conn->channel_count || conn->stats_per_bands == NULL) {
        return NULL;
    }

/* TX stats */
#if WPS_ENABLE_PHY_STATS
    conn->stats_per_bands[channel_number].tx_timeslot_occurrence =
        conn->wps_conn_handle->channel_lqi[channel_number].sent_count;
#endif

#if WPS_ENABLE_STATS_USED_TIMESLOTS
    conn->stats_per_bands[channel_number].packet_sent_and_acked_count =
        conn->wps_conn_handle->channel_lqi[channel_number].ack_count;
    conn->stats_per_bands[channel_number].packet_sent_and_not_acked_count =
        conn->wps_conn_handle->channel_lqi[channel_number].nack_count;
#endif

    uint32_t tx_count = conn->stats_per_bands[channel_number].packet_sent_and_acked_count +
                        conn->stats_per_bands[channel_number].packet_sent_and_not_acked_count;

    /* tx_timeslot_occurrence value can be lower than tx_count value if after updating tx_timeslot_occurrence,
     * and before updating packet_sent_and_acked_count and/or packet_sent_and_not_acked_count, a new packet is
     * transmitted. In such case, adjust tx_timeslot_occurrence to reflect the new TX event(s).
     */
    if (tx_count > conn->stats_per_bands[channel_number].tx_timeslot_occurrence) {
        conn->stats_per_bands[channel_number].tx_timeslot_occurrence = tx_count;
    }

#if WPS_ENABLE_PHY_STATS && WPS_ENABLE_STATS_USED_TIMESLOTS
    conn->stats_per_bands[channel_number].no_packet_tranmission_count =
        conn->stats_per_bands[channel_number].tx_timeslot_occurrence -
        conn->stats_per_bands[channel_number].packet_sent_and_acked_count -
        conn->stats_per_bands[channel_number].packet_sent_and_not_acked_count;
#endif

    if (conn->stats_per_bands[channel_number].tx_timeslot_occurrence != 0) {
        conn->stats_per_bands[channel_number].tx_used_capacity_pc =
            ((float)tx_count / (conn->stats_per_bands[channel_number].tx_timeslot_occurrence)) * 100;
    }

#if WPS_ENABLE_LINK_STATS
    conn->stats_per_bands[channel_number].packet_dropped_count =
        wps_stats_get_chan_payload_dropped_count(conn->wps_conn_handle, channel_number);
    conn->stats_per_bands[channel_number].cca_pass_count = wps_stats_get_chan_phy_cca_pass_count(conn->wps_conn_handle,
                                                                                                 channel_number);
    conn->stats_per_bands[channel_number].cca_fail_count = wps_stats_get_chan_phy_cca_tx_fail(conn->wps_conn_handle,
                                                                                              channel_number);
    conn->stats_per_bands[channel_number].cca_try_fail_count = wps_stats_get_chan_phy_cca_fail(conn->wps_conn_handle,
                                                                                               channel_number);
#endif

/* RX stats */
#if WPS_ENABLE_PHY_STATS
    conn->stats_per_bands[channel_number].packet_rejected_count =
        wps_stats_get_chan_rejected_frame_count(conn->wps_conn_handle, channel_number);
    conn->stats_per_bands[channel_number].rx_timeslot_occurrence =
        conn->stats_per_bands[channel_number].packet_rejected_count +
        wps_stats_get_chan_received_frame_count(conn->wps_conn_handle, channel_number) +
        wps_stats_get_chan_missing_frame_count(conn->wps_conn_handle, channel_number);
    conn->stats_per_bands[channel_number].rssi_avg = wps_stats_get_chan_rssi_avg(conn->wps_conn_handle, channel_number);
    conn->stats_per_bands[channel_number].rnsi_avg = wps_stats_get_chan_rnsi_avg(conn->wps_conn_handle, channel_number);
    conn->stats_per_bands[channel_number].link_margin_avg = wps_stats_get_chan_margin_avg(conn->wps_conn_handle,
                                                                                          channel_number);
    conn->stats_per_bands[channel_number].rssi_avg_raw = wps_stats_get_chan_rssi_avg_raw(conn->wps_conn_handle,
                                                                                         channel_number);
    conn->stats_per_bands[channel_number].rnsi_avg_raw = wps_stats_get_chan_rnsi_avg_raw(conn->wps_conn_handle,
                                                                                         channel_number);
#endif

#if WPS_ENABLE_STATS_USED_TIMESLOTS
    conn->stats_per_bands[channel_number].packet_successfully_received_count =
        wps_stats_get_chan_received_frame_count(conn->wps_conn_handle, channel_number);
#endif

    /* rx_timeslot_occurrence value can be lower than packet_successfully_received_count value if after updating
     * rx_timeslot_occurrence, and before updating packet_successfully_received_count, a new packet is received.
     * In such case, adjust rx_timeslot_occurrence to reflect the new RX event(s).
     */
    if (conn->stats_per_bands[channel_number].packet_successfully_received_count >
        conn->stats_per_bands[channel_number].rx_timeslot_occurrence) {
        conn->stats_per_bands[channel_number].rx_timeslot_occurrence =
            conn->stats_per_bands[channel_number].packet_successfully_received_count;
    }

#if WPS_ENABLE_PHY_STATS && WPS_ENABLE_STATS_USED_TIMESLOTS
    conn->stats_per_bands[channel_number].no_packet_reception_count =
        conn->stats_per_bands[channel_number].rx_timeslot_occurrence -
        conn->stats_per_bands[channel_number].packet_successfully_received_count;
#endif
    /* Unsupported per channel stats since Stop and Wait module is not implemented per channel. */
    conn->stats_per_bands[channel_number].packet_duplicated_count = 0;
#if WPS_ENABLE_LINK_STATS
    conn->stats_per_bands[channel_number].packet_overrun_count =
        wps_stats_get_chan_payload_overrun_count(conn->wps_conn_handle, channel_number);
#endif

    return &conn->stats_per_bands[channel_number];
}
#endif

int swc_connection_format_stats(const swc_connection_t *const conn, const swc_node_t *const node, char *const buffer,
                                uint16_t size)
{
    int string_length;
    const char *datarate_str = "Datarate";

    if (conn->wps_conn_handle->source_address == node->wps_node_handle->cfg.local_address) {
        /* TX stats */
        const char *tx_timeslot_occurrence_str = "TX Timeslot Occurrence";
        const char *packet_sent_and_acked_str = "Packet Sent And ACK'd";
        const char *packet_sent_and_not_acked_str = "Packet Sent And Not ACK'd";
        const char *no_packet_transmission_str = "No Packet Transmission";
        const char *tx_packet_dropped_str = "Packet Dropped";
        const char *tx_used_capacity_str = "TX Used Capacity";
        const char *cca_pass_str = "CCA Pass";
        const char *cca_fail_str = "CCA Fail";
        const char *cca_try_fail_str = "CCA Try Fail";

        string_length =
            snprintf(buffer, size,
                     "<<< %s >>>\r\n"
                     "%s:\t\t\t%10.2f (kbps)\r\n"
                     "%s:\t\t%10lu\r\n"
                     "  %s:\t%10lu (%05.2f%%)\r\n"
                     "  %s:\t%10lu (%05.2f%%)\r\n"
                     "  %s:\t%10lu (%05.2f%%)\r\n"
                     "%s:\t\t\t%10lu\r\n"
                     "%s:\t\t%10.2f%%\r\n"
                     "%s:\t\t\t%10lu\r\n"
                     "%s:\t\t\t%10lu\r\n"
                     "%s:\t\t\t%10lu\r\n",
                     conn->cfg.name, datarate_str, (double)conn->stats.tx_data_rate_bps / 1000,
                     tx_timeslot_occurrence_str, conn->stats.tx_timeslot_occurrence, packet_sent_and_acked_str,
                     conn->stats.packet_sent_and_acked_count,
                     (double)conn->stats.packet_sent_and_acked_count / conn->stats.tx_timeslot_occurrence * 100,
                     packet_sent_and_not_acked_str, conn->stats.packet_sent_and_not_acked_count,
                     (double)conn->stats.packet_sent_and_not_acked_count / conn->stats.tx_timeslot_occurrence * 100,
                     no_packet_transmission_str, conn->stats.no_packet_tranmission_count,
                     (double)conn->stats.no_packet_tranmission_count / conn->stats.tx_timeslot_occurrence * 100,
                     tx_packet_dropped_str, conn->stats.packet_dropped_count, tx_used_capacity_str,
                     (double)conn->stats.tx_used_capacity_pc, cca_pass_str, conn->stats.cca_pass_count, cca_fail_str,
                     conn->stats.cca_fail_count, cca_try_fail_str, conn->stats.cca_try_fail_count);
    } else {
        /* RX stats */
        const char *rx_timeslot_occurrence_str = "RX Timeslot Occurrence";
        const char *packet_successfully_received_count_str = "Packet Successfully Received";
        const char *no_packet_reception_count_str = "No Packet Reception";
        const char *packet_duplicated_count_str = "Packet Duplicated";
        const char *packet_rejected_count_str = "Packet Rejected";
        const char *packet_overrun_count_str = "Packet Overrun";
        const char *rssi_str = "RSSI Average";
        const char *rnsi_str = "RNSI Average";
        const char *link_margin_str = "Link Margin Average";
        const char *rssi_raw_str = "RSSI Average Raw";
        const char *rnsi_raw_str = "RNSI Average Raw";

        string_length =
            snprintf(buffer, size,
                     "<<< %s >>>\r\n"
                     "%s:\t\t\t%10.2f (kbps)\r\n"
                     "%s:\t\t%10lu\r\n"
                     "  %s:\t%10lu (%05.2f%%)\r\n"
                     "  %s:\t\t%10lu (%05.2f%%)\r\n"
                     "%s:\t\t%10lu\r\n"
                     "%s:\t\t%10lu\r\n"
                     "%s:\t\t\t%10lu\r\n"
                     "%s:\t\t\t%10lu\r\n"
                     "%s:\t\t\t%10lu\r\n"
                     "%s:\t\t%10lu\r\n"
                     "%s:\t\t%10lu\r\n"
                     "%s:\t\t%10lu\r\n",
                     conn->cfg.name, datarate_str, (double)conn->stats.rx_data_rate_bps / 1000,
                     rx_timeslot_occurrence_str, conn->stats.rx_timeslot_occurrence,
                     packet_successfully_received_count_str, conn->stats.packet_successfully_received_count,
                     (double)conn->stats.packet_successfully_received_count / conn->stats.rx_timeslot_occurrence * 100,
                     no_packet_reception_count_str, conn->stats.no_packet_reception_count,
                     (double)conn->stats.no_packet_reception_count / conn->stats.rx_timeslot_occurrence * 100,
                     packet_duplicated_count_str, conn->stats.packet_duplicated_count, packet_rejected_count_str,
                     conn->stats.packet_rejected_count, packet_overrun_count_str, conn->stats.packet_overrun_count,
                     rssi_str, conn->stats.rssi_avg, rnsi_str, conn->stats.rnsi_avg, link_margin_str,
                     conn->stats.link_margin_avg, rssi_raw_str, conn->stats.rssi_avg_raw, rnsi_raw_str,
                     conn->stats.rnsi_avg_raw);
    }

    return string_length;
}

void swc_connection_reset_stats(swc_connection_t *const conn)
{
    memset(&conn->stats, 0, sizeof(swc_statistics_t));
    conn->stats.tick_on_reset = conn->wps_conn_handle->get_tick();
    wps_stats_reset(conn->wps_conn_handle);
}
