/** @file wps_mac.h
 *  @brief Wireless protocol stack layer 2.
 *
 *  @copyright Copyright (C) 2020 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef MAC_LAYER_H_
#define MAC_LAYER_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "link_channel_hopping.h"
#include "link_credit_flow_ctrl.h"
#include "link_ddcm.h"
#include "link_fallback.h"
#include "link_gain_loop.h"
#include "link_lqi.h"
#include "link_phase.h"
#include "link_protocol.h"
#include "link_random_datarate_offset.h"
#include "link_scheduler.h"
#include "link_tdma_sync.h"
#include "wps_callback.h"
#include "wps_config.h"
#include "wps_conn_priority.h"
#include "wps_def.h"
#include "wps_mac_certification.h"
#include "wps_mac_def.h"
#include "wps_mac_protocols.h"
#include "wps_mac_statistics.h"
#include "wps_mac_timeslots.h"
#include "wps_mac_xlayer.h"
#include "wps_phy.h"
#include "xlayer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief MAC Layer initialization function.
 *
 *  @param[out] wps_mac                          MAC layer instance.
 *  @param[in]  channel_sequence                 Channel sequence.
 *  @param[in]  sync_cfg                         Synchronization module configuration.
 *  @param[in]  local_address                    Node local address.
 *  @param[in]  node_role                        Node role.
 *  @param[in]  random_channel_sequence_enabled  Random channel sequence enabled.
 *  @param[in]  network_id                       Network ID.
 *  @param[in]  frame_lost_max_duration          Maximum frame lost duration before link is
 *                                               considered unsynced.
 *  @param[in]  max_expected_payload_size        Maximum expected payload size.
 *  @param[in]  max_expected_header_size         Maximum expected header size.
 */
void wps_mac_init(wps_mac_t *wps_mac, channel_sequence_t *channel_sequence, wps_mac_sync_cfg_t *sync_cfg,
                  uint16_t local_address, wps_role_t node_role, bool random_channel_sequence_enabled,
                  uint8_t network_id, uint32_t frame_lost_max_duration, uint8_t max_expected_payload_size,
                  uint8_t max_expected_header_size);

/** @brief Reset the MAC Layer object.
 *
 *  @param wps_mac  MAC Layer instance.
 */
void wps_mac_reset(wps_mac_t *wps_mac);

/** @brief Enable fast sync.
 *
 *  @param wps_mac  MAC Layer instance.
 */
void wps_mac_enable_fast_sync(wps_mac_t *wps_mac);

/** @brief Disable fast sync.
 *
 *  @param wps_mac  MAC Layer instance.
 */
void wps_mac_disable_fast_sync(wps_mac_t *wps_mac);

/** @brief WPS MAC callback from PHY.
 *
 *  @param[in] mac            MAC Layer instance.
 *  @param[in] wps_phy        PHY Layer instance.
 */
void wps_mac_phy_callback(void *mac, wps_phy_t *wps_phy);

/** @brief Output if node role is NETWORK_NODE.
 *
 *  @param[in] wps_mac  MAC structure.
 *  @retval True   Node role is NETWORK_NODE.
 *  @retval False  Node role is not NETWORK_NODE.
 */
static inline bool wps_mac_is_network_node(wps_mac_t *wps_mac)
{
    return (wps_mac->node_role == NETWORK_NODE);
}

#ifdef __cplusplus
}
#endif

#endif /* MAC_LAYER_H_ */
