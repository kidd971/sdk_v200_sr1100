/** @file  wps_mac_certification.h
 *  @brief Wireless Protocol Stack MAC certification module.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef WPS_MAC_CERTIFICATION_H
#define WPS_MAC_CERTIFICATION_H

/* INCLUDES *******************************************************************/
#include "wps_mac.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Enqueue initial certification frames.
 *
 *  @param[in] wps_mac  MAC Layer instance.
 */
void wps_mac_certification_init(void *wps_mac);

/** @brief Send certification frame on the connection.
 *
 *  @param[in] connection  Connection instance.
 */
void wps_mac_certification_send(wps_connection_t *connection);

/** @brief Fill the frame header with certification data
 *
 *  @param[in] header            Frame connection header
 *  @param[in] header_size       Header size of the frame
 */
void wps_mac_certification_fill_header(uint8_t *header, uint8_t header_size);

#ifdef __cplusplus
}
#endif

#endif /* WPS_MAC_CERTIFICATION_H */
