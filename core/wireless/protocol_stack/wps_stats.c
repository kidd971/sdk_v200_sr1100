/** @file  wps_stats.c
 *  @brief Wireless Protocol Stack statistics.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "wps_stats.h"
#include <string.h>

/* CONSTANTS ******************************************************************/
#define CHAR_BIT 8

/* PUBLIC FUNCTIONS ***********************************************************/
#if WPS_ENABLE_LINK_STATS
float wps_stats_get_payload_success_ratio(wps_connection_t *connection)
{
    uint32_t tx_sum = connection->wps_stats.tx_success + connection->wps_stats.tx_fail;

    if (tx_sum == 0) {
        return 0.f;
    }

    return (float)connection->wps_stats.tx_success / tx_sum;
}

float wps_stats_get_tx_datarate(wps_connection_t *connection, uint32_t time_ms)
{
    if (time_ms == 0) {
        return 0.f;
    }

    return CHAR_BIT * ((float)connection->wps_stats.tx_byte_sent / time_ms);
}

uint32_t wps_stats_get_tx_byte_sent(wps_connection_t *connection)
{
    return connection->wps_stats.tx_byte_sent;
}

void wps_stats_reset_tx_byte_sent(wps_connection_t *connection)
{
    connection->wps_stats.tx_byte_sent = 0;
}

float wps_stats_get_rx_datarate(wps_connection_t *connection, uint32_t time_ms)
{
    if (time_ms == 0) {
        return 0.f;
    }

    return CHAR_BIT * ((float)connection->wps_stats.rx_byte_received / time_ms);
}

uint32_t wps_stats_get_rx_byte_received(wps_connection_t *connection)
{
    return connection->wps_stats.rx_byte_received;
}

void wps_stats_reset_rx_byte_received(wps_connection_t *connection)
{
    connection->wps_stats.rx_byte_received = 0;
}

float wps_stats_get_phy_cca_pass_ratio(wps_connection_t *connection)
{
    return (float)(connection->wps_stats.cca_pass) /
           (connection->wps_stats.cca_pass + connection->wps_stats.cca_tx_fail);
}

float wps_stats_get_phy_cca_fail_ratio(wps_connection_t *connection)
{
    return (float)(connection->wps_stats.cca_tx_fail) /
           (connection->wps_stats.cca_pass + connection->wps_stats.cca_tx_fail);
}
#endif /* WPS_ENABLE_LINK_STATS */

#if WPS_ENABLE_PHY_STATS
#if WPS_ENABLE_STATS_USED_TIMESLOTS
float wps_stats_get_tx_link_usage_ratio(wps_connection_t *connection)
{
    if (connection->lqi.total_count == 0) {
        return 0.f;
    }

    return (float)connection->used_frame_lqi.sent_count / connection->lqi.total_count;
}

float wps_stats_get_rx_link_usage_ratio(wps_connection_t *connection)
{
    if (connection->lqi.total_count == 0) {
        return 0.f;
    }

    return (float)connection->used_frame_lqi.received_count / connection->lqi.total_count;
}
#endif /* WPS_ENABLE_STATS_USED_TIMESLOTS */

int32_t wps_stats_get_phy_margin_avg(wps_connection_t *connection)
{
    int32_t link_margin;

    uint16_t rssi = link_lqi_get_avg_rssi_tenth_db(&connection->lqi);
    uint16_t rnsi = link_lqi_get_avg_rnsi_tenth_db(&connection->lqi);

    /* RSSI can't be lower than noise floor */
    link_margin = (rssi < rnsi) ? 0 : rssi - rnsi;

    return link_margin;
}

int32_t wps_stats_get_inst_phy_margin(wps_connection_t *connection)
{
    return link_lqi_get_inst_rssi_tenth_db(&connection->lqi) - link_lqi_get_inst_rnsi_tenth_db(&connection->lqi);
}

float wps_stats_get_phy_ack_frame_ratio(wps_connection_t *connection)
{
    uint32_t sent_count = link_lqi_get_sent_count(&connection->lqi);

    if (sent_count == 0) {
        return 0.f;
    }

    return (float)link_lqi_get_ack_count(&connection->lqi) / sent_count;
}

float wps_stats_get_phy_nack_frame_ratio(wps_connection_t *connection)
{
    uint32_t sent_count = link_lqi_get_sent_count(&connection->lqi);

    if (sent_count == 0) {
        return 0.f;
    }

    return (float)link_lqi_get_nack_count(&connection->lqi) / sent_count;
}

float wps_stats_get_phy_received_frame_ratio(wps_connection_t *connection)
{
    uint32_t total_count = link_lqi_get_total_count(&connection->lqi);

    if (total_count == 0) {
        return 0.f;
    }

    return (float)link_lqi_get_received_count(&connection->lqi) / total_count;
}

float wps_stats_get_phy_missing_frame_ratio(wps_connection_t *connection)
{
    uint32_t total_count = link_lqi_get_total_count(&connection->lqi);

    if (total_count == 0) {
        return 0.f;
    }

    return (float)link_lqi_get_lost_count(&connection->lqi) / total_count;
}

float wps_stats_get_phy_rejected_frame_ratio(wps_connection_t *connection)
{
    uint32_t total_count = link_lqi_get_total_count(&connection->lqi);

    if (total_count == 0) {
        return 0.f;
    }

    return (float)link_lqi_get_rejected_count(&connection->lqi) / total_count;
}

float wps_stats_get_phy_mrr(wps_connection_t *connection)
{
    uint32_t bad_frame_count = link_lqi_get_rejected_count(&connection->lqi) +
                               link_lqi_get_lost_count(&connection->lqi);

    if (bad_frame_count == 0) {
        return 0.f;
    }

    return (float)link_lqi_get_lost_count(&connection->lqi) / bad_frame_count;
}

float wps_stats_get_phy_per(wps_connection_t *connection)
{
    uint32_t total_frame_count = link_lqi_get_total_count(&connection->lqi);

    if (total_frame_count == 0) {
        return 0.f;
    }

    return (float)(total_frame_count - link_lqi_get_received_count(&connection->lqi)) /
           total_frame_count;
}

int32_t wps_stats_get_phy_rssi_avg_raw(wps_connection_t *connection)
{
    return link_lqi_get_avg_rssi_raw(&connection->lqi);
}

int32_t wps_stats_get_phy_rnsi_avg_raw(wps_connection_t *connection)
{
    return link_lqi_get_avg_rnsi_raw(&connection->lqi);
}

int32_t wps_stats_get_phy_margin_avg_raw(wps_connection_t *connection)
{
    int32_t margin_avg = link_lqi_get_avg_rnsi_raw(&connection->lqi) -
                         link_lqi_get_avg_rssi_raw(&connection->lqi);

    if (margin_avg < 0) {
        return 0;
    } else {
        return margin_avg / 2;
    }
}

#endif /* WPS_ENABLE_PHY_STATS */

#if WPS_ENABLE_STATS_USED_TIMESLOTS
int32_t wps_stats_get_margin_avg(wps_connection_t *connection)
{
    int32_t link_margin;

    uint16_t rssi = link_lqi_get_avg_rssi_tenth_db(&connection->used_frame_lqi);
    uint16_t rnsi = link_lqi_get_avg_rnsi_tenth_db(&connection->used_frame_lqi);

    /* RSSI can't be lower than noise floor */
    link_margin = (rssi < rnsi) ? 0 : rssi - rnsi;

    return link_margin;
}

float wps_stats_get_ack_frame_ratio(wps_connection_t *connection)
{
    uint32_t sent_count = link_lqi_get_sent_count(&connection->used_frame_lqi);

    if (sent_count == 0) {
        return 0.f;
    }

    return (float)link_lqi_get_ack_count(&connection->used_frame_lqi) / sent_count;
}

float wps_stats_get_nack_frame_ratio(wps_connection_t *connection)
{
    uint32_t sent_count = link_lqi_get_sent_count(&connection->used_frame_lqi);

    if (sent_count == 0) {
        return 0.f;
    }

    return (float)link_lqi_get_nack_count(&connection->used_frame_lqi) / sent_count;
}

float wps_stats_get_received_frame_ratio(wps_connection_t *connection)
{
    uint32_t total_count = link_lqi_get_total_count(&connection->used_frame_lqi);

    if (total_count == 0) {
        return 0.f;
    }

    return (float)link_lqi_get_received_count(&connection->used_frame_lqi) / total_count;
}

float wps_stats_get_missing_frame_ratio(wps_connection_t *connection)
{
    uint32_t total_count = link_lqi_get_total_count(&connection->used_frame_lqi);

    if (total_count == 0) {
        return 0.f;
    }

    return (float)link_lqi_get_lost_count(&connection->used_frame_lqi) / total_count;
}

float wps_stats_get_rejected_frame_ratio(wps_connection_t *connection)
{
    uint32_t total_count = link_lqi_get_total_count(&connection->used_frame_lqi);

    if (total_count == 0) {
        return 0.f;
    }

    return (float)link_lqi_get_rejected_count(&connection->used_frame_lqi) / total_count;
}

float wps_stats_get_mrr(wps_connection_t *connection)
{
    uint32_t bad_frame_count = link_lqi_get_rejected_count(&connection->used_frame_lqi) +
                               link_lqi_get_lost_count(&connection->used_frame_lqi);

    if (bad_frame_count == 0) {
        return 0.f;
    }

    return (float)(link_lqi_get_lost_count(&connection->used_frame_lqi)) / bad_frame_count;
}

float wps_stats_get_per(wps_connection_t *connection)
{
    uint32_t total_frame_count = link_lqi_get_total_count(&connection->used_frame_lqi);

    if (total_frame_count == 0) {
        return 0.f;
    }

    return (float)(total_frame_count - link_lqi_get_received_count(&connection->used_frame_lqi)) /
           total_frame_count;
}
#endif /* WPS_ENABLE_STATS_USED_TIMESLOTS */

#if WPS_ENABLE_PHY_STATS_PER_BANDS
int32_t wps_stats_get_chan_margin_avg(wps_connection_t *connection, uint8_t channel_idx)
{
    int32_t link_margin;

    uint16_t rssi = link_lqi_get_avg_rssi_tenth_db(&connection->channel_lqi[channel_idx]);
    uint16_t rnsi = link_lqi_get_avg_rnsi_tenth_db(&connection->channel_lqi[channel_idx]);

    /* RSSI can't be lower than noise floor */
    link_margin = (rssi < rnsi) ? 0 : rssi - rnsi;

    return link_margin;
}

float wps_stats_get_chan_ack_frame_ratio(wps_connection_t *connection, uint8_t channel_idx)
{
    uint32_t sent_count = link_lqi_get_sent_count(&connection->channel_lqi[channel_idx]);

    if (sent_count == 0) {
        return 0.f;
    }

    return (float)(link_lqi_get_ack_count(&connection->channel_lqi[channel_idx])) / sent_count;
}

float wps_stats_get_chan_nack_frame_ratio(wps_connection_t *connection, uint8_t channel_idx)
{
    uint32_t sent_count = link_lqi_get_sent_count(&connection->channel_lqi[channel_idx]);

    if (sent_count == 0) {
        return 0.f;
    }

    return (float)(link_lqi_get_nack_count(&connection->channel_lqi[channel_idx])) / sent_count;
}

float wps_stats_get_chan_received_frame_ratio(wps_connection_t *connection, uint8_t channel_idx)
{
    uint32_t total_count = link_lqi_get_total_count(&connection->channel_lqi[channel_idx]);

    if (total_count == 0) {
        return 0.f;
    }

    return (float)(link_lqi_get_received_count(&connection->channel_lqi[channel_idx])) /
           total_count;
}

float wps_stats_get_chan_missing_frame_ratio(wps_connection_t *connection, uint8_t channel_idx)
{
    uint32_t total_count = link_lqi_get_total_count(&connection->channel_lqi[channel_idx]);

    if (total_count == 0) {
        return 0.f;
    }

    return (float)(link_lqi_get_lost_count(&connection->channel_lqi[channel_idx])) / total_count;
}

float wps_stats_get_chan_rejected_frame_ratio(wps_connection_t *connection, uint8_t channel_idx)
{
    uint32_t total_count = link_lqi_get_total_count(&connection->channel_lqi[channel_idx]);

    if (total_count == 0) {
        return 0.f;
    }

    return (float)(link_lqi_get_rejected_count(&connection->channel_lqi[channel_idx])) /
           total_count;
}

float wps_stats_get_chan_mrr(wps_connection_t *connection, uint8_t channel_idx)
{
    uint32_t bad_frame_count = link_lqi_get_rejected_count(&connection->channel_lqi[channel_idx]) +
                               link_lqi_get_lost_count(&connection->channel_lqi[channel_idx]);

    if (bad_frame_count == 0) {
        return 0.f;
    }

    return (float)(link_lqi_get_lost_count(&connection->channel_lqi[channel_idx])) /
           bad_frame_count;
}

float wps_stats_get_chan_per(wps_connection_t *connection, uint8_t channel_idx)
{
    uint32_t total_frame_count = link_lqi_get_total_count(&connection->channel_lqi[channel_idx]);

    if (total_frame_count == 0) {
        return 0.f;
    }

    return (float)(total_frame_count -
                   link_lqi_get_received_count(&connection->channel_lqi[channel_idx])) /
           total_frame_count;
}
#endif /* WPS_ENABLE_PHY_STATS_PER_BANDS */

void wps_stats_reset(wps_connection_t *connection)
{
    #if WPS_ENABLE_PHY_STATS
        link_lqi_reset(&connection->lqi);
    #endif
    #if WPS_ENABLE_STATS_USED_TIMESLOTS
        link_lqi_reset(&connection->used_frame_lqi);
    #endif
    #if WPS_ENABLE_LINK_STATS
        memset(&connection->wps_stats, 0, sizeof(wps_stats_t));
    #endif

    link_saw_arq_reset_stats(&connection->stop_and_wait_arq);

    #if WPS_ENABLE_PHY_STATS_PER_BANDS
    for (size_t i = 0; i < connection->max_channel_count; i++) {
        link_lqi_reset(&connection->channel_lqi[i]);
    }
#endif
}
