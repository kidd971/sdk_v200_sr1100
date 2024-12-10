/** @file  wps_stats.h
 *  @brief Wireless Protocol Stack statistics.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef WPS_STATS_H
#define WPS_STATS_H

/* INCLUDES *******************************************************************/
#include "wps.h"
#include "wps_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
#if WPS_ENABLE_LINK_STATS
/** @brief Number of payloads successfully sent.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Payload successfully sent count.
 */
static inline uint32_t wps_stats_get_payload_success_count(wps_connection_t *connection)
{
    return connection->wps_stats.tx_success;
}

/** @brief Number of payloads unsuccessfully sent.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Payload successfully sent count.
 */
static inline uint32_t wps_stats_get_payload_fail_count(wps_connection_t *connection)
{
    return connection->wps_stats.tx_fail;
}

/** @brief Number of payloads dropped.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Payload dropped count.
 */
static inline uint32_t wps_stats_get_payload_dropped_count(wps_connection_t *connection)
{
    return connection->wps_stats.tx_drop;
}

/** @brief Number of payload successfully sent on the total number of payload sent.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Payload transmission success ratio.
 */
float wps_stats_get_payload_success_ratio(wps_connection_t *connection);

/** @brief Number of payloads received.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Payload received count.
 */
static inline uint32_t wps_stats_get_payload_received_count(wps_connection_t *connection)
{
    return connection->wps_stats.rx_received;
}

/** @brief Number of payloads dropped because of an RX buffer overload.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Payload overrun count.
 */
static inline uint32_t wps_stats_get_payload_overrun_count(wps_connection_t *connection)
{
    return connection->wps_stats.rx_overrun;
}

/** @brief Average TX datarate in kbps since the last stats reset.
 *
 *  @note The tx datarate is calculated from the acknowledge count,
 *        if ack are disabled the datarate will be 0.
 *
 *  @param[in] connection  WPS connection object.
 *  @param[in] time_ms     Elapsed time in ms since the last stats reset.
 *  @return RX datarate.
 */
float wps_stats_get_tx_datarate(wps_connection_t *connection, uint32_t time_ms);

/** @brief Get the number of bytes sent on a connection.
 *
 *  @param[in] connection  WPS connection object.
 *  @return The amount of bytes sent on the connection.
 */
uint32_t wps_stats_get_tx_byte_sent(wps_connection_t *connection);

/** @brief Reset the tx_byte_sent statistic.
 *
 *  @param[in] connection  WPS connection object.
 */
void wps_stats_reset_tx_byte_sent(wps_connection_t *connection);

/** @brief Average RX datarate in kbps since the last stats reset.
 *
 *  @param[in] connection  WPS connection object.
 *  @param[in] time_ms     Elapsed time in ms since the last stats reset.
 *  @return TX datarate.
 */
float wps_stats_get_rx_datarate(wps_connection_t *connection, uint32_t time_ms);

/** @brief Get the number of bytes received on a connection.
 *
 *  @param[in] connection  WPS connection object.
 *  @return The amount of bytes received on the connection.
 */
uint32_t wps_stats_get_rx_byte_received(wps_connection_t *connection);

/** @brief Reset the rx_byte_received statistic.
 *
 *  @param[in] connection  WPS connection object.
 */
void wps_stats_reset_rx_byte_received(wps_connection_t *connection);

/** @brief Number CCA pass events.
 *
 *  @param[in] connection  WPS connection object.
 *  @return CCA pass count.
 */
static inline uint32_t wps_stats_get_phy_cca_pass_count(wps_connection_t *connection)
{
    return connection->wps_stats.cca_pass;
}

/** @brief Number of timeslots in which all CCA tries failed.
 *
 *  @param[in] connection  WPS connection object.
 *  @return CCA TX fails.
 */
static inline uint32_t wps_stats_get_phy_cca_tx_fail(wps_connection_t *connection)
{
    return connection->wps_stats.cca_tx_fail;
}

/** @brief CCA pass ratio.
 *
 *  @param[in] connection  WPS connection object.
 *  @return CCA pass ratio.
 */
float wps_stats_get_phy_cca_pass_ratio(wps_connection_t *connection);

/** @brief CCA fail ratio.
 *
 *  @param[in] connection  WPS connection object.
 *  @return CCA fail ratio.
 */
float wps_stats_get_phy_cca_fail_ratio(wps_connection_t *connection);

/** @brief Get number of CCA fail events.
 *
 *  @param[in] connection  WPS connection object.
 *  @return CCA fail count.
 */
static inline uint32_t wps_stats_get_phy_cca_fail(wps_connection_t *connection)
{
    return connection->wps_stats.cca_fail;
}
#endif /* WPS_ENABLE_LINK_STATS */

#if WPS_ENABLE_PHY_STATS
#if WPS_ENABLE_STATS_USED_TIMESLOTS
/** @brief Number of sync frame sent or the number of empty tx timeslots.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Empty count.
 */
static inline uint32_t wps_stats_get_empty_count(wps_connection_t *connection)
{
    return connection->lqi.sent_count - connection->used_frame_lqi.sent_count;
}

/** @brief Number of sync frame received.
 *
 *  @param[in] connection  WPS connection object.
 *  @return RX sync count.
 */
static inline uint32_t wps_stats_get_rx_sync_count(wps_connection_t *connection)
{
    return connection->lqi.received_count - connection->used_frame_lqi.received_count;
}

/** @brief Ratio of TX timeslots with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @return TX link usage.
 */
float wps_stats_get_tx_link_usage_ratio(wps_connection_t *connection);

/** @brief Ratio of RX timeslots with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @return RX link usage.
 */
float wps_stats_get_rx_link_usage_ratio(wps_connection_t *connection);
#endif /* WPS_ENABLE_STATS_USED_TIMESLOTS */

/** @brief Get average RSSI on the physical layer.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Average RSSI.
 */
static inline uint32_t wps_stats_get_phy_rssi_avg(wps_connection_t *connection)
{
    uint16_t rssi = link_lqi_get_avg_rssi_tenth_db(&connection->lqi);
    uint16_t rnsi = link_lqi_get_avg_rnsi_tenth_db(&connection->lqi);

    /* RSSI can't be lower than noise floor */
    rssi = (rssi < rnsi) ? rnsi : rssi;

    return rssi;
}

/** @brief Get last received RSSI measurement on the given connection.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Instantaneous RSSI.
 */
static inline uint32_t wps_stats_get_phy_rssi(wps_connection_t *connection)
{
    return link_lqi_get_inst_rssi(&connection->lqi);
}

/** @brief Get last received RSSI measurement on the given connection in tenth of dB.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Instantaneous RSSI in tenth of dB.
 */
static inline uint32_t wps_stats_get_inst_phy_rssi_tenth_db(wps_connection_t *connection)
{
    return link_lqi_get_inst_rssi_tenth_db(&connection->lqi);
}

/** @brief Get average RNSI on the physical layer.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Average RNSI.
 */
static inline uint32_t wps_stats_get_phy_rnsi_avg(wps_connection_t *connection)
{
    return link_lqi_get_avg_rnsi_tenth_db(&connection->lqi);
}

/** @brief Get last received RNSI measurement on the given connection.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Instantaneous RNSI.
 */
static inline uint32_t wps_stats_get_phy_rnsi(wps_connection_t *connection)
{
    return link_lqi_get_inst_rnsi(&connection->lqi);
}

/** @brief Get last received RNSI measurement on the given connection in tenth of dB.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Instantaneous RNSI in tenth of dB.
 */
static inline uint32_t wps_stats_get_inst_phy_rnsi_tenth_db(wps_connection_t *connection)
{
    return link_lqi_get_inst_rnsi_tenth_db(&connection->lqi);
}

/** @brief Get link margin on the physical layer.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Link margin.
 */
int32_t wps_stats_get_phy_margin_avg(wps_connection_t *connection);

/** @brief Get the instantaneous link margin on the physical layer.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Link margin.
 */
int32_t wps_stats_get_inst_phy_margin(wps_connection_t *connection);

/** @brief Get phase offset instantaneous values.
 *
 *  @param[in] connection  WPS connection object.
 *  @param[in] index       Index.
 *  @return Phase offset values.
 */
static inline uint32_t wps_stats_get_phy_inst_phase_offset(wps_connection_t *connection,
                                                           uint8_t index)
{
    return link_lqi_get_inst_phase_offset(&connection->lqi, index);
}

/** @brief Get phy sent frame count.
 *
 *  @note This will increment every TX timeslot.
 *
 *  @param[in] connection  WPS connection object.
 *  @return PHY sent count.
 */
static inline uint32_t wps_stats_get_phy_sent_count(wps_connection_t *connection)
{
    return link_lqi_get_sent_count(&connection->lqi);
}

/** @brief Get ack frame count on the physical layer.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Ack frame count.
 */
static inline uint32_t wps_stats_get_phy_ack_frame_count(wps_connection_t *connection)
{
    return link_lqi_get_ack_count(&connection->lqi);
}

/** @brief Get nack frame count on the physical layer.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Nack frame count.
 */
static inline uint32_t wps_stats_get_phy_nack_frame_count(wps_connection_t *connection)
{
    return link_lqi_get_nack_count(&connection->lqi);
}

/** @brief Get received frame count on the physical layer.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Received frame count.
 */
static inline uint32_t wps_stats_get_phy_received_frame_count(wps_connection_t *connection)
{
    return link_lqi_get_received_count(&connection->lqi);
}

/** @brief Get missing frame count on the physical layer.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Missing frame count.
 */
static inline uint32_t wps_stats_get_phy_missing_frame_count(wps_connection_t *connection)
{
    return link_lqi_get_lost_count(&connection->lqi);
}

/** @brief Get rejected frame count on the physical layer.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Rejected frame count.
 */
static inline uint32_t wps_stats_get_phy_rejected_frame_count(wps_connection_t *connection)
{
    return link_lqi_get_rejected_count(&connection->lqi);
}

/** @brief Get ack frame ratio on the physical layer.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Ack frame ratio.
 */
float wps_stats_get_phy_ack_frame_ratio(wps_connection_t *connection);

/** @brief Get nack frame ratio on the physical layer.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Nack frame ratio.
 */
float wps_stats_get_phy_nack_frame_ratio(wps_connection_t *connection);

/** @brief Get received frame ratio on the physical layer.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Received frame ratio.
 */
float wps_stats_get_phy_received_frame_ratio(wps_connection_t *connection);

/** @brief Get missing frame ratio on the physical layer.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Missing frame ratio.
 */
float wps_stats_get_phy_missing_frame_ratio(wps_connection_t *connection);

/** @brief Get rejected frame ratio on the physical layer.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Rejected frame ratio.
 */
float wps_stats_get_phy_rejected_frame_ratio(wps_connection_t *connection);

/** @brief Get Missing/Reject Ration (MRR) frame ratio on the physical layer.
 *
 *  @param[in] connection  WPS connection object.
 *  @return MRR frame ratio.
 */
float wps_stats_get_phy_mrr(wps_connection_t *connection);

/** @brief Get Payload Error Rate (PER) on the physical layer.
 *
 *  @param[in] connection  WPS connection object.
 *  @return PER.
 */
float wps_stats_get_phy_per(wps_connection_t *connection);

/** @brief Get RSSI code average.
 *
 *  @param[in] connection  WPS connection object.
 *  @return RSSI average.
 */
int32_t wps_stats_get_phy_rssi_avg_raw(wps_connection_t *connection);

/** @brief Get RNSI code average.
 *
 *  @param[in] connection  WPS connection object.
 *  @return RNSI average.
 */
int32_t wps_stats_get_phy_rnsi_avg_raw(wps_connection_t *connection);

/** @brief Get link margin code average.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Link margin average.
 */
int32_t wps_stats_get_phy_margin_avg_raw(wps_connection_t *connection);
#endif /* WPS_ENABLE_PHY_STATS */

/** @brief Get duplicated frame count on the physical layer.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Duplicated frame count.
 */
static inline uint32_t wps_stats_get_phy_duplicated_frame_count(wps_connection_t *connection)
{
    return link_saw_arq_get_duplicate_count(&connection->stop_and_wait_arq);
}

/** @brief Get retry frame count on the physical layer.
 *
 *  @param connection  WPS connection object.
 *  @return Number of retries.
 */
static inline uint32_t wps_stats_get_phy_retry_frame_count(wps_connection_t *connection)
{
    return link_saw_arq_get_retry_count(&connection->stop_and_wait_arq);
}

/** @brief Get duplicated frame count of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Duplicated frame count.
 */
static inline uint32_t wps_stats_get_duplicated_frame_count(wps_connection_t *connection)
{
    return link_saw_arq_get_duplicate_count(&connection->stop_and_wait_arq);
}

/** @brief Get retry frame count of frames with payload.
 *
 *  @param connection  WPS connection object.
 *  @return Number of retries.
 */
static inline uint32_t wps_stats_get_retry_frame_count(wps_connection_t *connection)
{
    return link_saw_arq_get_retry_count(&connection->stop_and_wait_arq);
}

#if WPS_ENABLE_STATS_USED_TIMESLOTS
/** @brief Get average Received Signal Strength Indicator (RSSI) of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Average RSSI.
 */
static inline uint32_t wps_stats_get_rssi_avg(wps_connection_t *connection)
{
    uint16_t rssi = link_lqi_get_avg_rssi_tenth_db(&connection->used_frame_lqi);
    uint16_t rnsi = link_lqi_get_avg_rnsi_tenth_db(&connection->used_frame_lqi);

    /* RSSI can't be lower than noise floor */
    rssi = (rssi < rnsi) ? rnsi : rssi;

    return rssi;
}

/** @brief Get average Received Noise Strength Indicator (RNSI) of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Average RNSI.
 */
static inline uint32_t wps_stats_get_rnsi_avg(wps_connection_t *connection)
{
    return link_lqi_get_avg_rnsi_tenth_db(&connection->used_frame_lqi);
}

/** @brief Get link margin of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Link margin.
 */
int32_t wps_stats_get_margin_avg(wps_connection_t *connection);

/** @brief Get ACK frame count of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @return ACK frame count.
 */
static inline uint32_t wps_stats_get_ack_frame_count(wps_connection_t *connection)
{
    return link_lqi_get_ack_count(&connection->used_frame_lqi);
}

/** @brief Get NACK frame count of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @return NACK frame count.
 */
static inline uint32_t wps_stats_get_nack_frame_count(wps_connection_t *connection)
{
    return link_lqi_get_nack_count(&connection->used_frame_lqi);
}

/** @brief Get received frame count of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Received frame count.
 */
static inline uint32_t wps_stats_get_received_frame_count(wps_connection_t *connection)
{
    return link_lqi_get_received_count(&connection->used_frame_lqi);
}

/** @brief Get missing frame count of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Missing frame count.
 */
static inline uint32_t wps_stats_get_missing_frame_count(wps_connection_t *connection)
{
    return link_lqi_get_lost_count(&connection->used_frame_lqi);
}

/** @brief Get rejected frame count of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Rejected frame count.
 */
static inline uint32_t wps_stats_get_rejected_frame_count(wps_connection_t *connection)
{
    return link_lqi_get_rejected_count(&connection->used_frame_lqi);
}

/** @brief Get ACK frame ratio of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @return ACK frame ratio.
 */
float wps_stats_get_ack_frame_ratio(wps_connection_t *connection);

/** @brief Get NACK frame ratio of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @return NACK frame ratio.
 */
float wps_stats_get_nack_frame_ratio(wps_connection_t *connection);

/** @brief Get received frame ratio of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Received frame ratio.
 */
float wps_stats_get_received_frame_ratio(wps_connection_t *connection);

/** @brief Get missing frame ratio of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Missing frame ratio.
 */
float wps_stats_get_missing_frame_ratio(wps_connection_t *connection);

/** @brief Get rejected frame ratio of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Rejected frame ratio.
 */
float wps_stats_get_rejected_frame_ratio(wps_connection_t *connection);

/** @brief Get Missing/Rejected Ratio (MRR) of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @return MRR frame ratio.
 */
float wps_stats_get_mrr(wps_connection_t *connection);

/** @brief Get PER of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @return PER.
 */
float wps_stats_get_per(wps_connection_t *connection);
#endif /* WPS_ENABLE_STATS_USED_TIMESLOTS */

/** @brief Reset stats.
 *
 *  @param[in] connection  WPS connection object.
 */
void wps_stats_reset(wps_connection_t *connection);

#if WPS_ENABLE_PHY_STATS_PER_BANDS
/** @brief Get average RSSI of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @param[in] channel_idx Channel index.
 *  @return Average RSSI.
 */
static inline uint32_t wps_stats_get_chan_rssi_avg(wps_connection_t *connection,
                                                   uint8_t channel_idx)
{
    uint16_t rssi = link_lqi_get_avg_rssi_tenth_db(&connection->channel_lqi[channel_idx]);
    uint16_t rnsi = link_lqi_get_avg_rnsi_tenth_db(&connection->channel_lqi[channel_idx]);

    /* RSSI can't be lower than noise floor */
    rssi = (rssi < rnsi) ? rnsi : rssi;

    return rssi;
}

/** @brief Get last received RSSI of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @param[in] channel_idx Channel index.
 *  @return Last RSSI measurement.
 */
static inline uint32_t wps_stats_get_chan_rssi(wps_connection_t *connection, uint8_t channel_idx)
{
    return link_lqi_get_inst_rssi(&connection->channel_lqi[channel_idx]);
}

/** @brief Get average RNSI of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @param[in] channel_idx Channel index.
 *  @return Average RNSI.
 */
static inline uint32_t wps_stats_get_chan_rnsi_avg(wps_connection_t *connection,
                                                   uint8_t channel_idx)
{
    return link_lqi_get_avg_rnsi_tenth_db(&connection->channel_lqi[channel_idx]);
}

/** @brief Get last received RNSI of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @param[in] channel_idx Channel index.
 *  @return Last RNSI measurment.
 */
static inline uint32_t wps_stats_get_chan_rnsi(wps_connection_t *connection, uint8_t channel_idx)
{
    return link_lqi_get_inst_rnsi(&connection->channel_lqi[channel_idx]);
}

/** @brief Get RSSI code average for target channel.
 *
 *  @param[in] connection   WPS connection object.
 *  @param[in] channel_idx  Target channel.
 *  @return RSSI average raw, from 47 to 0.
 */
static inline int32_t wps_stats_get_chan_rssi_avg_raw(wps_connection_t *connection, uint8_t channel_idx)
{
    return link_lqi_get_avg_rssi_raw(&connection->channel_lqi[channel_idx]);
}

/** @brief Get RNSI code average for target channel.
 *
 *  @param[in] connection  WPS connection object.
 *  @param[in] channel_idx  Target channel.
 *  @return RNSI average raw, from 47 to 0.
 */
static inline int32_t wps_stats_get_chan_rnsi_avg_raw(wps_connection_t *connection, uint8_t channel_idx)
{
    return link_lqi_get_avg_rnsi_raw(&connection->channel_lqi[channel_idx]);
}

/** @brief Get link margin of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @param[in] channel_idx Channel index.
 *  @return Link margin.
 */
int32_t wps_stats_get_chan_margin_avg(wps_connection_t *connection, uint8_t channel_idx);

/** @brief Get ack frame count of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @param[in] channel_idx Channel index.
 *  @return Ack frame count.
 */
static inline uint32_t wps_stats_get_chan_ack_frame_count(wps_connection_t *connection,
                                                          uint8_t channel_idx)
{
    return link_lqi_get_ack_count(&connection->channel_lqi[channel_idx]);
}

/** @brief Get nack frame count of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @param[in] channel_idx Channel index.
 *  @return Nack frame count.
 */
static inline uint32_t wps_stats_get_chan_nack_frame_count(wps_connection_t *connection,
                                                           uint8_t channel_idx)
{
    return link_lqi_get_nack_count(&connection->channel_lqi[channel_idx]);
}

/** @brief Get received frame count of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @param[in] channel_idx Channel index.
 *  @return Received frame count.
 */
static inline uint32_t wps_stats_get_chan_received_frame_count(wps_connection_t *connection,
                                                               uint8_t channel_idx)
{
    return link_lqi_get_received_count(&connection->channel_lqi[channel_idx]);
}

/** @brief Get missing frame count of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @param[in] channel_idx Channel index.
 *  @return Missing frame count.
 */
static inline uint32_t wps_stats_get_chan_missing_frame_count(wps_connection_t *connection,
                                                              uint8_t channel_idx)
{
    return link_lqi_get_lost_count(&connection->channel_lqi[channel_idx]);
}

/** @brief Get rejected frame count of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @param[in] channel_idx Channel index.
 *  @return Rejected frame count.
 */
static inline uint32_t wps_stats_get_chan_rejected_frame_count(wps_connection_t *connection,
                                                               uint8_t channel_idx)
{
    return link_lqi_get_rejected_count(&connection->channel_lqi[channel_idx]);
}

/** @brief Get ack frame ratio of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @param[in] channel_idx Channel index.
 *  @return Ack frame ratio.
 */
float wps_stats_get_chan_ack_frame_ratio(wps_connection_t *connection, uint8_t channel_idx);

/** @brief Get nack frame ratio of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @param[in] channel_idx Channel index.
 *  @return Nack frame ratio.
 */
float wps_stats_get_chan_nack_frame_ratio(wps_connection_t *connection, uint8_t channel_idx);

/** @brief Get received frame ratio of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @param[in] channel_idx Channel index.
 *  @return Received frame ratio.
 */
float wps_stats_get_chan_received_frame_ratio(wps_connection_t *connection, uint8_t channel_idx);

/** @brief Get missing frame ratio of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @param[in] channel_idx Channel index.
 *  @return Missing frame ratio.
 */
float wps_stats_get_chan_missing_frame_ratio(wps_connection_t *connection, uint8_t channel_idx);

/** @brief Get rejected frame ratio of frames with payload.
 *
 *  @param[in] connection  WPS connection object.
 *  @param[in] channel_idx Channel index.
 *  @return Rejected frame ratio.
 */
float wps_stats_get_chan_rejected_frame_ratio(wps_connection_t *connection, uint8_t channel_idx);

/** @brief Get Missing/Reject Ration (MRR) of a channel.
 *
 *  @param[in] connection  WPS connection object.
 *  @param[in] channel_idx Channel index.
 *  @return MRR frame ratio.
 */
float wps_stats_get_chan_mrr(wps_connection_t *connection, uint8_t channel_idx);

/** @brief Get Payload Error Rate (PER) of a channel.
 *
 *  @param[in] connection   WPS connection object.
 *  @param[in] channel_idx  Channel index.
 *  @return PER.
 */
float wps_stats_get_chan_per(wps_connection_t *connection, uint8_t channel_idx);

#if WPS_ENABLE_LINK_STATS
/** @brief Number of payloads dropped for the target channel.
 *
 *  @param[in] connection   WPS connection object.
 *  @param[in] channel_idx  Target channel.
 *  @return Payload dropped count.
 */
static inline uint32_t wps_stats_get_chan_payload_dropped_count(wps_connection_t *connection, uint8_t channel_idx)
{
    return connection->wps_chan_stats[channel_idx].tx_drop;
}

/** @brief Number CCA pass events for the target channel.
 *
 *  @param[in] connection   WPS connection object.
 *  @param[in] channel_idx  Target channel.
 *  @return CCA pass count.
 */
static inline uint32_t wps_stats_get_chan_phy_cca_pass_count(wps_connection_t *connection, uint8_t channel_idx)
{
    return connection->wps_chan_stats[channel_idx].cca_pass;
}

/** @brief Number of timeslots in which all CCA tries failed for target channel.
 *
 *  @param[in] connection  WPS connection object.
 *  @param[in] channel_idx  Target channel.
 *  @return CCA TX fails.
 */
static inline uint32_t wps_stats_get_chan_phy_cca_tx_fail(wps_connection_t *connection, uint8_t channel_idx)
{
    return connection->wps_chan_stats[channel_idx].cca_tx_fail;
}

/** @brief Get number of CCA fail events for the target channel.
 *
 *  @param[in] connection  WPS connection object.
 *  @param[in] channel_idx  Target channel.
 *  @return CCA fail count.
 */
static inline uint32_t wps_stats_get_chan_phy_cca_fail(wps_connection_t *connection, uint8_t channel_idx)
{
    return connection->wps_chan_stats[channel_idx].cca_fail;
}

/** @brief Number of payloads dropped because of an RX buffer overload for the target channel.
 *
 *  @param[in] connection  WPS connection object.
 *  @param[in] channel_idx  Target channel.
 *  @return Payload overrun count.
 */
static inline uint32_t wps_stats_get_chan_payload_overrun_count(wps_connection_t *connection, uint8_t channel_idx)
{
    return connection->wps_chan_stats[channel_idx].rx_overrun;
}
#endif /* WPS_ENABLE_LINK_STATS */
#endif /* WPS_ENABLE_PHY_STATS_PER_BANDS */

#ifdef __cplusplus
}
#endif

#endif /* WPS_STATS_H */
