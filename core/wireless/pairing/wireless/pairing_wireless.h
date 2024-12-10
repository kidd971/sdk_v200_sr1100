/** @file  pairing_wireless.h
 *  @brief This file handles the wireless management for pairing module.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef PAIRING_WIRELESS_H_
#define PAIRING_WIRELESS_H_

/* INCLUDES *******************************************************************/
#include "pairing_api.h"
#include "swc_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/*! The maximum payload size in bytes during a wireless transfer. */
#define PAIRING_MAX_PAYLOAD_SIZE  16

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Initialize the pairing wireless configuration.
 *
 *  @param[in] pairing_cfg   Pairing configuration from the application.
 *  @param[in] network_role  Network role of the current device.
 */
void pairing_wireless_init(pairing_cfg_t *pairing_cfg, swc_role_t network_role);

/** @brief Wrapper function to connect the device to the wireless network.
 */
void pairing_wireless_connect(void);

/** @brief Wrapper function to disconnect the device to the wireless network.
 */
void pairing_wireless_disconnect(void);

/** @brief Wrapper function to reset the memory from the wireless core API.
 */
void pairing_wireless_free_memory(void);

/** @brief Wrapper function to easily send a message through the wireless network.
 *
 *  @param[in] payload_buffer Payload buffer to be sent.
 *  @param[in] size           Payload buffer size in byte.
 */
void pairing_wireless_send_message(uint8_t *payload_buffer, uint16_t size);

/** @brief Get the radio serial number.
 *
 *  @return Device's radio serial number.
 */
uint64_t pairing_wireless_get_radio_serial_number(void);

/** @brief Get the configured network role.
 *
 *  @return Device's network role.
 */
swc_role_t pairing_wireless_get_network_role(void);

/** @brief Set the callback which is called when a message was successfully sent.
 *
 *  @param[in] callback  The assigned callback.
 */
void pairing_wireless_set_sent_message_callback(void (*callback)(void));

/** @brief Set the callback which is called when a message is received.
 *
 *  @param[in] callback  The assigned callback.
 */
void pairing_wireless_set_received_message_callback(void (*callback)(uint8_t *received_message, uint8_t message_size));

/** @brief Get the wireless core status.
 *
 *  @return The wireless core status.
 */
swc_status_t pairing_wireless_get_status(void);

#ifdef __cplusplus
}
#endif

#endif /* PAIRING_WIRELESS_H_ */
