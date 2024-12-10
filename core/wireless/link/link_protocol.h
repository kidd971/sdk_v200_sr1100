/** @file link_protocol.h
 *  @brief WPS layer 2 internal connection protocol.
 *
 *  This file is a wrapper use to send/received payload
 *  through the WPS MAC internal connection. Its used to
 *  properly generate a complete packet regrouping one
 *  or multiple information.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef LINK_PROTOCOL_H_
#define LINK_PROTOCOL_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "link_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
#define MAX_NUMBER_OF_PROTOCOL 10

/* TYPES **********************************************************************/
/** @brief Protocol internal info for link_protocol module.
 *
 *  @note This should be provided when adding a protocol
 *        to the link_protocol instance.
 */
typedef struct link_protocol_info {
    /*! Protocol object. */
    void *instance;
    /*! Protocol transmit function, this function should populate the tx_buffer provided based on the init size. */
    void (*send)(void *self, uint8_t *tx_buffer);
    /*! Protocol receive function, this function should extract the data from the received payload. */
    void (*receive)(void *self, uint8_t *rx_buffer);
    /*! Protocol RX/TX size. */
    uint8_t size;
    /*! Protocol identifier. */
    uint8_t id;
    /*! Protocol buffer offset, given by the link_protocol for each protocol. to write/read their buffer. */
    uint8_t index;
} link_protocol_info_t;

/** @brief Link protocol instance.
 */
typedef struct link_protocol {
    /*! Transmission buffer that encapsulate every protocol. */
    uint8_t index;
    /*! Buffer offset use to know where every protocol put their data. */
    uint8_t current_buffer_offset;
    /*! Total number of protocol. */
    uint8_t current_number_of_protocol;
    /*! Total protocol buffer size. */
    uint16_t max_buffer_size;
    /*! Internal protocol info array. */
    link_protocol_info_t protocol_info[MAX_NUMBER_OF_PROTOCOL];
} link_protocol_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the link protocol instance.
 *
 *  @note Only the protocol buffer is needed here.
 *        Every protocol should be added using the
 *        link_protocol_add function.
 *
 *  @param[in]  link_protocol   Link protocol instance.
 *  @param[in]  max_buffer_size Transmission buffer size.
 */
void link_protocol_init(link_protocol_t *link_protocol, uint16_t max_buffer_size);

/** @brief Add a protocol to the link protocol.
 *
 *  @note  RX/TX size of the given protocol should be the same.
 *
 *  @param[in]  link_protocol  Link protocol instance.
 *  @param[in]  protocol_cfg   Protocol configuration instance.
 *  @param[out] err            Link error instance.
 */
void link_protocol_add(link_protocol_t *link_protocol, link_protocol_info_t *protocol_info, link_error_t *err);

/** @brief Populate the given TX buffer with all the protocol.
 *
 *  *  @note This will call every protocols send function with
 *           their respective buffer in order for them to properly
 *           populate their data.
 *
 *  @param[in]  link_protocol   Link protocol instance.
 *  @param[out] buffer_to_send  Pointer to the buffer that will be sent.
 *  @param[out] size            Size of the buffer to send.
 */
void link_protocol_send_buffer(link_protocol_t *link_protocol, uint8_t *buffer_to_send, uint32_t *size);

/** @brief Receive a giving buffer with all the protocol.
 *
 *  @note This will call every protocols receive function with
 *        their respective buffer in order for them to properly
 *        extract their data.
 *
 *  @param[in] link_protocol    Link protocol instance.
 *  @param[in] received_buffer  Buffer to received.
 *  @param[in] size             Size of the received buffer.
 */
void link_protocol_receive_buffer(link_protocol_t *link_protocol, uint8_t *received_buffer, uint32_t size);

/** @brief Get the protocol buffer offset associated with the provided ID.
 *
 *  @param[in] link_protocol    Link protocol instance.
 *  @param[in] protocol_id      The protocol ID.
 */
uint8_t link_protocol_get_buffer_offset(link_protocol_t *link_protocol, uint8_t protocol_id);

#ifdef __cplusplus
}
#endif
#endif /* LINK_PROTOCOL_H_ */
