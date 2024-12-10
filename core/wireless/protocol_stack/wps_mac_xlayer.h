/** @file  wps_mac_xlayer.h
 *  @brief Wireless Protocol Stack MAC xlayer component.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef WPS_MAC_XLAYER_H
#define WPS_MAC_XLAYER_H

/* INCLUDES *******************************************************************/
#include "wps_mac_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Update the xlayer sync module value for PHY.
 *
 *  @param[in]  wps_mac     MAC Layer instance.
 *  @param[out] xlayer_cfg  Current xlayer node to update.
 */
static inline void wps_max_xlayer_update_sync(wps_mac_t *wps_mac, xlayer_cfg_internal_t *xlayer_cfg)
{
    xlayer_cfg->power_up_delay = link_tdma_sync_get_pwr_up(&wps_mac->tdma_sync);
    xlayer_cfg->rx_timeout     = link_tdma_sync_get_timeout(&wps_mac->tdma_sync);
    xlayer_cfg->sleep_time     = link_tdma_sync_get_sleep_cycles(&wps_mac->tdma_sync);
}

/** @brief Update the main connection xlayer gain loop value for PHY.
 *
 *  @param[in] wps_mac MAC Layer instance.
 *  @param[in] xlayer  xlayer node to update.
 */
static inline void wps_mac_xlayer_update_main_link_parameter(wps_mac_t *wps_mac, xlayer_t *xlayer)
{
    xlayer->frame.destination_address = wps_mac->main_connection->destination_address;
    xlayer->frame.source_address      = wps_mac->main_connection->source_address;
}

/** @brief Update the main connection xlayer gain loop value for PHY.
 *
 *  @param[in] wps_mac MAC Layer instance.
 *  @param[in] xlayer  xlayer node to update.
 */
static inline void wps_mac_xlayer_update_auto_reply_link_parameter(wps_mac_t *wps_mac,
                                                                   xlayer_t *xlayer)
{
    if (xlayer != NULL) {
        xlayer->frame.destination_address = wps_mac->auto_connection->destination_address;
        xlayer->frame.source_address      = wps_mac->auto_connection->source_address;
    }
}

/** @brief Update the main connection xlayer modem feat value for PHY.
 *
 *  @param[in] wps_mac     MAC Layer instance.
 *  @param[in] xlayer_cfg  xlayer node to update.
 */
static inline void update_xlayer_modem_feat(wps_mac_t *wps_mac, xlayer_cfg_internal_t *xlayer_cfg)
{
    xlayer_cfg->fec        = wps_mac->main_connection->frame_cfg.fec;
    xlayer_cfg->modulation = wps_mac->main_connection->frame_cfg.modulation;
    xlayer_cfg->chip_repet = wps_mac->main_connection->frame_cfg.chip_repet;
}

/** @brief Return the corresponding queue for TX main connection depending on the input connection.
 *
 * @note For TX timeslot, Application should have enqueue a node
 *       inside the queue, so the MAC only need to get the front of
 *       the queue in order to get the good node for the process.
 *
 *  @param[in] wps_mac     MAC Layer instance.
 *  @param[in] connection  Queue node connection.
 *  @return Current pending node in queue.
 */
xlayer_t *wps_mac_xlayer_get_xlayer_for_tx_main(wps_mac_t *wps_mac, wps_connection_t *connection);

/** @brief Return the corresponding queue for TX auto connection depending on the input connection.
 *
 * @note For TX timeslot, Application should have enqueue a node
 *       inside the queue, so the MAC only need to get the front of
 *       the queue in order to get the good node for the process.
 *
 *  @param[in] wps_mac     MAC Layer instance.
 *  @param[in] connection  Queue node connection.
 *  @return Current pending node in queue.
 */
xlayer_t *wps_mac_xlayer_get_xlayer_for_tx_auto(wps_mac_t *wps_mac, wps_connection_t *connection);

/** @brief Return the corresponding queue for RX depending on the input connection.
 *
 * For RX timeslot, MAC should get the first free slot, WPS
 * should enqueue for application .
 *
 *  @param[in] wps_mac     MAC Layer instance.
 *  @param[in] connection  Queue node connection.
 *  @return Current pending node in queue.
 */
xlayer_t *wps_mac_xlayer_get_xlayer_for_rx(wps_mac_t *wps_mac, wps_connection_t *connection);

/** @brief Free node data and return node to its free xlayer_queue.
 *
 *  @param[in] connection  Queue node connection.
 *  @param[in] node        Node to be freed.
 */
void wps_mac_xlayer_free_node_with_data(wps_connection_t *connection, xlayer_queue_node_t *node);

/** @brief Update the xlayer frame pointer based on the received header for the main RX connection
 *
 *  @param[in] wps_mac         MAC Layer instance.
 *  @param[in] frame           Cross layer main frame.
 *  @param[in] required_space  Required space for payload data
 */
void wps_mac_xlayer_update_main_rx_payload_buffer(void *wps_mac, xlayer_frame_t *frame, uint8_t required_space);

/** @brief Update the xlayer frame pointer based on the received header for the auto-reply RX
 * connection
 *
 *  @param[in] wps_mac         MAC Layer instance.
 *  @param[in] frame           Cross layer auto reply frame.
 *  @param[in] required_space  Required space for payload data
 */
void wps_mac_xlayer_update_auto_reply_rx_payload_buffer(void *wps_mac, xlayer_frame_t *frame, uint8_t required_space);

/** @brief Return an empty xlayer for RX auto-reply connection.
 *
 *  @param[in] wps_mac     MAC Layer instance.
 *  @param[in] connection  Main connection.
 *  @return Empty auto-reply xlayer node.
 */
xlayer_t *wps_mac_xlayer_get_xlayer_for_empty_rx_auto(wps_mac_t *wps_mac, wps_connection_t *connection);

/** @brief Return an empty xlayer for TX auto-reply connection.
 *
 *  @param[in] wps_mac     MAC Layer instance.
 *  @param[in] connection  Main connection.
 *  @return Empty auto-reply xlayer node.
 */
xlayer_t *wps_mac_xlayer_get_xlayer_for_empty_tx_auto(wps_mac_t *wps_mac, wps_connection_t *connection);

/** @brief Update the empty auto-reply connection xlayer addresses for PHY based on main connection.
 *
 *  @param[in] wps_mac MAC Layer instance.
 *  @param[in] xlayer  xlayer node to update.
 */
static inline void wps_mac_xlayer_update_empty_auto_conn_reply_link_parameter(wps_mac_t *wps_mac, xlayer_t *xlayer)
{
    if (xlayer != NULL) {
        xlayer->frame.destination_address = wps_mac->main_connection->source_address;
        xlayer->frame.source_address = wps_mac->main_connection->destination_address;
    }
}

#ifdef __cplusplus
}
#endif

#endif /* WPS_MAC_XLAYER_H */
