/** @file  wps_frag.h
 *  @brief WPS Fragmentation module.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef WPS_FRAG_H_
#define WPS_FRAG_H_

/* INCLUDES *******************************************************************/
#include "wps.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the fragmentation module.
 *
 *  @param[in] connection      Connection instance.
 *  @param[in] meta_tx_buffer  Meta data transmission buffer.
 *  @param[in] meta_tx_size    Meta data transmission buffer size.
 */
void wps_frag_init(wps_connection_t *connection, void *meta_tx_buffer, uint32_t meta_tx_size);

/** @brief Send payload over the air.
 *
 *  Enqueue a node in the connection Xlayer and WPS will
 *  send at next available timeslot.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[in]  payload     Application payload to send over the air.
 *  @param[in]  size        Payload size in bytes.
 *  @param[out] err         Pointer to the error code.
 */
void wps_frag_send(wps_connection_t *connection, const uint8_t *payload, size_t size, wps_error_t *err);

/** @brief Read last received frame.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[out] payload     Pointer to the output buffer.
 *  @param[in]  max_size    Size of the output buffer.
 *  @param[in]  err         Pointer to the error code.
 *  @return WPS Received frame structure, including payload and size.
 */
wps_rx_frame wps_frag_read(wps_connection_t *connection, uint8_t *payload, size_t max_size, wps_error_t *err);

/** @brief Read size of the last received frame.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[in]  err         Pointer to the error code.
 *  @return WPS Received frame payload size.
 */
uint16_t wps_frag_get_read_payload_size(wps_connection_t *connection, wps_error_t *err);

/** @brief Set the callback function to execute when a payload is successfully transmitted.
 *
 *  @note This callback is raised every time a fragment is successfully transmitted
 *
 *  @param[in] connection  Pointer to the connection.
 *  @param[in] callback    Function pointer to the callback.
 *  @param[in] parg        Void pointer argument for the callback.
 */
void wps_frag_set_tx_success_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg);

/** @brief Set the callback function to execute when the WPS fail to transmit a frame.
 *
 *  @note An ACK frame was expected and was not received. If ACK is not enabled, this never triggers.
 *
 *  @param[in] connection  Pointer to the connection.
 *  @param[in] callback    Function pointer to the callback.
 *  @param[in] parg        Void pointer argument for the callback.
 */
void wps_frag_set_tx_fail_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg);

/** @brief Set the callback function to execute when the WPS drops a frame.
 *
 *  @note The Core has discarded the frame because the deadline configured for the ARQ
 *        (either in time or in number of retries) has been met. If ARQ is not enabled,
 *        this never triggers.
 *
 *  @param[in] connection  Pointer to the connection.
 *  @param[in] callback    Function pointer to the callback.
 *  @param[in] parg        Void pointer argument for the callback.
 */
void wps_frag_set_tx_drop_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg);

/** @brief Set the callback function to execute when the WPS successfully receives a frame.
 *
 *  @note The Core has successfully received a frame. The address matches its local address or the
 *        broadcast address and the CRC checked. The frame is ready to be picked up from the RX FIFO.
 *
 *  @param[in] connection  Pointer to the connection.
 *  @param[in] callback    Function pointer to the callback.
 *  @param[in] parg        Void pointer argument for the callback.
 */
void wps_frag_set_rx_success_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg);

/** @brief Set the callback function to execute when the WPS successfully receives a frame.
 *
 *  @note The Core has successfully received a frame. The address matches its local address or the
 *        broadcast address and the CRC checked. The frame is ready to be picked up from the RX FIFO.
 *
 *  @param[in] connection  Pointer to the connection.
 *  @param[in] callback    Function pointer to the callback.
 *  @param[in] parg        Void pointer argument for the callback.
 */
void wps_frag_set_rx_fail_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg);

/** @brief Set the callback function to execute when a WPS event occur.
 *
 *  @param[in] connection       Pointer to the connection.
 *  @param[in] callback         Function pointer to the callback.
 *  @param[in] parg             Void pointer argument for the callback.
 */
void wps_frag_set_event_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg);

/** @brief Return the used space of the connection Xlayer queue.
 *
 *  @param[in] connection  Connection instance.
 */
uint16_t wps_frag_get_fifo_size(wps_connection_t *connection);

#ifdef __cplusplus
}
#endif

#endif /* WPS_FRAG_H_ */
