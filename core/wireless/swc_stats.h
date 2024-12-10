/** @file  swc_stats.h
 *  @brief SPARK Wireless Core statistics.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SWC_STATS_H_
#define SWC_STATS_H_

/* INCLUDES *******************************************************************/
#include "swc_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Update connection statistics.
 *
 *  After calling this function, the statistics inside the
 *  connection handle will be updated. The function also
 *  returns a reference to these internal statistics so they
 *  can be used by the caller.
 *
 *  @param[in] conn  Connection handle.
 *  @return Reference to the statistics.
 */
swc_statistics_t *swc_connection_update_stats(swc_connection_t *const conn);

/** @brief Format the connection statistics as a string of characters.
 *
 *  @param[in]  conn    Connection handle.
 *  @param[in]  node    Node handle.
 *  @param[out] buffer  Buffer where to put the formatted string.
 *  @param[in]  size    Size of the buffer.
 *  @return The formated string length, excluding the NULL terminator.
 */
int swc_connection_format_stats(const swc_connection_t *const conn, const swc_node_t *const node, char *const buffer,
                                uint16_t size);

#if WPS_ENABLE_PHY_STATS_PER_BANDS
/** @brief Update connection statistics on a per channel basis.
 *
 *  After calling this function, the statistics inside the
 *  connection handle will be updated. The function also
 *  returns a reference to these internal statistics so they
 *  can be used by the caller.
 *
 *  @param[in] conn            Connection handle.
 *  @param[in] channel_number  Target channel.
 *  @return Reference to the statistics for the target channel.
 */
swc_statistics_t *swc_connection_update_stats_per_channel(swc_connection_t *const conn, const uint8_t channel_number);
#endif

/** @brief Reset all the connection statistics.
 *
 *  @param[in] conn  Connection handle.
 */
void swc_connection_reset_stats(swc_connection_t *const conn);

#ifdef __cplusplus
}
#endif

#endif /* SWC_STATS_H_ */
