/** @file  wps_mac_protocols.h
 *  @brief Wireless Protocol Stack protocols.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef WPS_MAC_PROTOCOLS_H
#define WPS_MAC_PROTOCOLS_H

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Interface to write the channel index to the header buffer.
 *
 *  @param[in] wps_mac MAC Layer instance.
 *  @param[in] index   Channel index buffer pointer.
 */
void wps_mac_send_channel_index(void *wps_mac, uint8_t *index);

/** @brief Interface to read the channel index to the header buffer.
 *
 *  @param[in]  wps_mac MAC Layer instance.
 *  @param[out] index   Channel index buffer pointer.
 */
void wps_mac_receive_channel_index(void *wps_mac, uint8_t *index);

/** @brief Get the size of the channel index header field.
 *
 *  @param[in] wps_mac MAC Layer instance.
 *  @return Header field size.
 */
uint8_t wps_mac_get_channel_index_proto_size(void *wps_mac);

/** @brief Interface to write the timeslot id and stop and wait to the header buffer.
 *
 *  @param[in] wps_mac         MAC Layer instance.
 *  @param[in] timeslot_id_saw Timeslot id and stop and wait buffer pointer.
 */
void wps_mac_send_timeslot_id_saw(void *wps_mac, uint8_t *timeslot_id_saw);

/** @brief Interface to read the timeslot id and stop and wait to the header buffer.
 *
 *  @param[in]  wps_mac         MAC Layer instance.
 *  @param[out] timeslot_id_saw Timeslot id and stop and wait buffer pointer.
 */
void wps_mac_receive_timeslot_id_saw(void *wps_mac, uint8_t *timeslot_id_saw);

/** @brief Get the size of the timeslot id and stop and wait header field.
 *
 *  @param[in] wps_mac MAC Layer instance.
 *  @return Header field size.
 */
uint8_t wps_mac_get_timeslot_id_saw_proto_size(void *wps_mac);

/** @brief Interface to write the random datarate offset to the header buffer.
 *
 *  @param[in] wps_mac MAC Layer instance.
 *  @param[in] rdo     Random datarate offset buffer pointer.
 */
void wps_mac_send_rdo(void *wps_mac, uint8_t *rdo);

/** @brief Interface to read the random datarate offset to the header buffer.
 *
 *  @param[in]  wps_mac MAC Layer instance.
 *  @param[out] rdo     Random datarate offset buffer pointer.
 */
void wps_mac_receive_rdo(void *wps_mac, uint8_t *rdo);

/** @brief Get the size of the random datarate offset header field.
 *
 *  @param[in] wps_mac MAC Layer instance.
 *  @return Header field size.
 */
uint8_t wps_mac_get_rdo_proto_size(void *wps_mac);

/** @brief Interface to write the ranging phases to the header buffer.
 *
 *  @param[in] wps_mac MAC Layer instance.
 *  @param[in] phases  Ranging phases buffer pointer.
 */
void wps_mac_send_ranging_phases(void *wps_mac, uint8_t *phases);

/** @brief Interface to read the ranging phases to the header buffer.
 *
 *  @param[in]  wps_mac MAC Layer instance.
 *  @param[out] phases  Ranging phases buffer pointer.
 */
void wps_mac_receive_ranging_phases(void *wps_mac, uint8_t *phases);

/** @brief Get the size of the ranging phases header field.
 *
 *  @param[in] wps_mac MAC Layer instance.
 *  @return Header field size.
 */
uint8_t wps_mac_get_ranging_phases_proto_size(void *wps_mac);

/** @brief Interface to write the ranging phase count to the header buffer.
 *
 *  @param[in] wps_mac      MAC Layer instance.
 *  @param[in] phase_count  Ranging phases count buffer pointer.
 */
void wps_mac_send_ranging_phase_count(void *wps_mac, uint8_t *phase_count);

/** @brief Interface to read the ranging phase count to the header buffer.
 *
 *  @param[in]  wps_mac      MAC Layer instance.
 *  @param[out] phase_count  Ranging phases count buffer pointer.
 */
void wps_mac_receive_ranging_phase_count(void *wps_mac, uint8_t *phase_count);

/** @brief Get the size of the ranging phase count header field.
 *
 *  @param[in] wps_mac MAC Layer instance.
 *  @return Header field size.
 */
uint8_t wps_mac_get_ranging_phase_count_proto_size(void *wps_mac);

/** @brief Interface to write the connection ID to the header buffer.
 *
 *  @param[in] wps_mac        MAC Layer instance.
 *  @param[in] connection_id  Connection ID buffer pointer.
 */
void wps_mac_send_connection_id(void *wps_mac, uint8_t *connection_id);

/** @brief Interface to read the connection ID to the header buffer.
 *
 *  @param[in]  wps_mac        MAC Layer instance.
 *  @param[out] connection_id  Connection ID buffer pointer.
 */
void wps_mac_receive_connection_id(void *wps_mac, uint8_t *connection_id);

/** @brief Get the size of the connection ID header field.
 *
 *  @param[in] wps_mac MAC Layer instance.
 *  @return Header field size.
 */
uint8_t wps_mac_get_connection_id_proto_size(void *wps_mac);

/** @brief Interface to write the connection ID to the header buffer for ACK frame without dedicated auto-reply
 * connection.
 *
 *  @param[in] wps_mac        MAC Layer instance.
 *  @param[in] connection_id  Connection ID buffer pointer.
 */
void wps_mac_send_connection_id_header_acknowledge(void *wps_mac, uint8_t *connection_id);

/** @brief Interface to read the connection ID to the header buffer for ACK frame without dedicated auto-reply
 * connection.
 *
 *  @param[in]  wps_mac        MAC Layer instance.
 *  @param[out] connection_id  Connection ID buffer pointer.
 */
void wps_mac_receive_connection_id_header_acknowledge(void *wps_mac, uint8_t *connection_id);

/** @brief Interface to write the credit flow control value to the header buffer.
 *
 *  @param[in] wps_mac    MAC Layer instance.
 *  @param[in] credit_fc  Credit flow control instance.
 */
void wps_mac_send_credit_flow_control(void *wps_mac, uint8_t *credit_fc);

/** @brief Interface to read the credit flow control value to the header buffer.
 *
 *  @param[in] wps_mac    MAC Layer instance.
 *  @param[in] credit_fc  Credit flow control instance.
 */
void wps_mac_receive_credit_flow_control(void *wps_mac, uint8_t *credit_fc);

/** @brief Get the size of the credit control flow header field.
 *
 *  @param[in] wps_mac MAC Layer instance.
 *  @return Header field size.
 */
uint8_t wps_mac_get_credit_flow_control_proto_size(void *wps_mac);

/** @brief Interface to write the credit flow control value to the header buffer for ACK frame without dedicated
 *         auto-reply connection.
 *
 *  @param[in] wps_mac    MAC Layer instance.
 *  @param[in] credit_fc  Credit flow control instance.
 */
void wps_mac_send_credit_flow_control_header_acknowledge(void *wps_mac, uint8_t *const credit_fc);

/** @brief Interface to read the credit flow control value to the header buffer for ACK frame without dedicated
 *         auto-reply connection.
 *
 *  @param[in] wps_mac    MAC Layer instance.
 *  @param[in] credit_fc  Credit flow control instance.
 */
void wps_mac_receive_credit_flow_control_header_acknowledge(void *wps_mac, uint8_t *credit_fc);

#ifdef __cplusplus
}
#endif

#endif /* WPS_MAC_PROTOCOLS_H */
