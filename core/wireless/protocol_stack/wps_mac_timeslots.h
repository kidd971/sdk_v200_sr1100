/** @file  wps_mac_timeslots.h
 *  @brief Wireless Protocol Stack MAC timeslots module.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef WPS_MAC_TIMESLOTS_H
#define WPS_MAC_TIMESLOTS_H

/* INCLUDES *******************************************************************/
#include "wps_mac_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Find the received time slot ID and connection ID for main frame.
 *
 *  @param[in] wps_mac  MAC Layer instance.
 *  @param[in] frame    Cross layer main frame.
 */
void wps_mac_timeslots_find_received_timeslot_and_connection_main(wps_mac_t *wps_mac, const xlayer_frame_t *frame);

/** @brief Find the received time slot ID and connection ID for auto reply frame.
 *
 *  @param[in] wps_mac  MAC Layer instance.
 *  @param[in] frame    Cross layer auto reply frame.
 */
void wps_mac_timeslots_find_received_timeslot_and_connection_auto(wps_mac_t *wps_mac, const xlayer_frame_t *frame);

/** @brief Output if current main connection timeslot is TX.
 *
 *  @param[in] wps_mac  MAC Layer instance.
 *  @retval True   Main connection timeslot is TX.
 *  @retval False  Main connection timeslot is RX.
 */
static inline bool wps_mac_timeslots_is_current_timeslot_tx(wps_mac_t *wps_mac)
{
    return (wps_mac->main_connection->source_address == wps_mac->local_address);
}

/** @brief Output if auto-reply connection timeslot is TX.
 *
 *  @param[in] wps_mac  MAC Layer instance.
 *  @retval True   Auto-reply connection timeslot is TX.
 *  @retval False  Auto-reply connection timeslot is RX.
 */
static inline bool wps_mac_timeslots_is_current_auto_reply_timeslot_tx(wps_mac_t *wps_mac)
{
    return (wps_mac->auto_connection->source_address == wps_mac->local_address);
}

#ifdef __cplusplus
}
#endif

#endif /* WPS_MAC_TIMESLOTS_H */
