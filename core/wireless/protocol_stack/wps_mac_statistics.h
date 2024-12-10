/** @file  wps_mac_statistics.h
 *  @brief Wireless Protocol Stack MAC statistics.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef WPS_MAC_STATISTICS_H
#define WPS_MAC_STATISTICS_H

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "wps_mac.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/** @brief Size of the statistics buffer to store processing data for
 * main and auto connections. Do not change!
 */
#define WPS_MAC_STATISTICS_BUFFER_STAT_SIZE (2)

/* TYPES **********************************************************************/
/** @brief Wireless Protocol Stack MAC statistics connection processing data structure.
 */
typedef struct wps_mac_stats_entry {
    /*! Processing data connection */
    wps_connection_t *connection;
    /*! Phase offset */
    uint8_t phase_offset[PHASE_OFFSET_BYTE_COUNT];
    /*! Current channel hopping index */
    uint8_t channel_index;
    /*! Receiver signal strength indicator */
    uint8_t rssi;
    /*! Receiver noise strength indicator */
    uint8_t rnsi;
    /*! Frame outcome */
    frame_outcome_t frame_outcome;
    /*! Denotes if processing data are for empty frame */
    bool empty_frame;
} wps_mac_stats_entry_t;

/** @brief Wireless Protocol Stack MAC statistics structure.
 */
typedef struct wps_mac_stats {
    /*! Buffer with the processing data */
    wps_mac_stats_entry_t input_data[WPS_MAC_STATISTICS_BUFFER_STAT_SIZE];
    /*! No space counter to store statistic processing data */
    uint16_t no_space_counter;
} wps_mac_stats_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
#if WPS_ENABLE_STATS_USED_TIMESLOTS || WPS_ENABLE_PHY_STATS || WPS_ENABLE_PHY_STATS_PER_BANDS
/** @brief Initialize statistic object.
 *
 *  @param[in] stats_process_data  Statistic data instance.
 */
void wps_mac_statistics_init(wps_mac_stats_t *stats_process_data);

/** @brief Update link quality statistics for current main connection.
 *
 * Processing data is performed in background.
 *
 *  @param[in] wps_mac  MAC Layer instance.
 */
void wps_mac_statistics_update_main_conn(void *wps_mac);

/** @brief Update link quality statistics for current auto-reply connection.
 *
 * Processing data is performed in background.
 *
 *  @param[in] wps_mac  MAC Layer instance.
 */
void wps_mac_statistics_update_auto_conn(void *wps_mac);

/** @brief Update link quality statistics for current main connection for empty frame.
 *
 * Processing data is performed in background.
 *
 *  @param[in] wps_mac  MAC Layer instance.
 */
void wps_mac_statistics_update_main_conn_empty_frame(void *wps_mac);

/** @brief Update link quality statistics for current auto-reply connection for empty frame.
 *
 * Processing data is performed in background.
 *
 *  @param[in] wps_mac  MAC Layer instance.
 */
void wps_mac_statistics_update_auto_conn_empty_frame(void *wps_mac);

/** @brief Calculate statistics data for main and auto connection stored in buffer.
 *
 *  @param[in] stats_process_data  Statistic data instance.
 */
void wps_mac_statistics_process_data(wps_mac_stats_t *stats_process_data);
#else
/// \cond DO_NOT_DOCUMENT
#define wps_mac_statistics_init(stats_process_data)
#define wps_mac_statistics_update_main_conn(wps_mac)
#define wps_mac_statistics_update_auto_conn(wps_mac)
#define wps_mac_statistics_update_main_conn_empty_frame(wps_mac)
#define wps_mac_statistics_update_auto_conn_empty_frame(wps_mac)
#define wps_mac_statistics_process_data(stats_process_data)
/// \endcond
#endif /* WPS_ENABLE_STATS_USED_TIMESLOTS, WPS_ENABLE_PHY_STATS, WPS_ENABLE_PHY_STATS_PER_BANDS */

#if WPS_ENABLE_LINK_STATS
/** @brief Update WPS statistics for main connection
 *
 *  @param[in] wps_mac  MAC Layer instance.
 */
void wps_mac_statistics_update_main_stats(void *wps_mac);

/** @brief Update WPS statistics for auto-reply connection
 *
 *  @param[in] wps_mac  MAC Layer instance.
 */
void wps_mac_statistics_update_auto_stats(void *wps_mac);

/** @brief Update statistics for TX packets drop for particular connection
 *
 *  @param[in] connection  WPS connection object.
 */
static inline void wps_mac_statistics_update_tx_dropped_conn_stats(wps_connection_t *connection)
{
    connection->wps_stats.tx_drop++;
    connection->total_pkt_dropped++;
}
#else
/// \cond DO_NOT_DOCUMENT
#define wps_mac_statistics_update_main_stats(wps_mac)
#define wps_mac_statistics_update_auto_stats(wps_mac)
#define wps_mac_statistics_update_tx_dropped_conn_stats(connection)
/// \endcond
#endif /* WPS_ENABLE_LINK_STATS */

#ifdef __cplusplus
}
#endif

#endif /* WPS_MAC_STATISTICS_H */
