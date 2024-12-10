/** @file  wps_conn_priority.h
 *  @brief Wireless Protocol Stack connection priority module.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef WPS_CONN_PRIORITY_H
#define WPS_CONN_PRIORITY_H

/* INCLUDES *******************************************************************/
#include "wps_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Get the index of the highest priority for main connection.
 *
 *  @param[in] connections            Connection table.
 *  @param[in] connection_priorities  Connection priorities table.
 *  @param[in] connection_count       Connection count.
 *  @return Main connection index with the highest priority.
 */
uint8_t wps_conn_priority_get_highest_main_conn_index(wps_connection_t **connections, const uint8_t *connection_priorities,
                                                      uint8_t connection_count);

/** @brief Get the index of the highest priority for auto-reply connection.
 *
 *  @param[in] connections            Connection table.
 *  @param[in] connection_priorities  Connection priorities table.
 *  @param[in] connection_count       Connection count.
 *  @return Auto reply connection index with the highest priority.
 */
uint8_t wps_conn_priority_get_highest_auto_conn_index(wps_connection_t **connections, const uint8_t *connection_priorities,
                                                      uint8_t connection_count);

#ifdef __cplusplus
}
#endif

#endif /* WPS_CONN_PRIORITY_H */
