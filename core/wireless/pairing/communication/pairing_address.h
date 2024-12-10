/** @file  pairing_address.h
 *  @brief This file manages the pairing addresses and the discovery list.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef PAIRING_ADDRESS_H_
#define PAIRING_ADDRESS_H_

/* INCLUDES *******************************************************************/
#include "pairing_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Pass the pairing address reference from the application and initialize it.
 *
 *  @param[in] pairing_assigned_address  Pairing addresses that are discoverable during the pairing procedure.
 */
void pairing_address_init(pairing_assigned_address_t *pairing_assigned_address);

/** @brief Set the PAN ID of the pairing address structure.
 *
 *  @param[in] pan_id  The PAN ID to be set in the pairing address structure.
 */
void pairing_address_set_pan_id(uint16_t pan_id);

/** @brief Set the coordinator address of the pairing address structure.
 *
 *  @param[in] coordinator_address  The coordinator address to be set in the pairing address structure.
 */
void pairing_address_set_coordinator_address(uint8_t coordinator_address);

/** @brief Set the node address of the pairing address structure.
 *
 *  @param[in] node_address  The coordinator address to be set in the pairing address structure.
 */
void pairing_address_set_node_address(uint8_t node_address);

/** @brief Get the PAN ID value of the pairing address structure.
 *
 *  @return PAN ID value.
 */
uint16_t pairing_address_get_pan_id(void);

/** @brief Get the coordinator value of the pairing address structure.
 *
 *  @return Coordinator value.
 */
uint8_t pairing_address_get_coordinator_address(void);

/** @brief Get the node value of the pairing address structure.
 *
 *  @return Node value.
 */
uint8_t pairing_address_get_node_address(void);

/** @brief Reset the values to 0 inside the pairing address structure.
 */
void pairing_address_reset(void);

/** @brief Initialize the discovery list from the application.
 *
 *  @param[in] discovery_list       Discovery list from the application.
 *  @param[in] discovery_list_size  Discovery list size from the application.
 */
void pairing_address_discovery_list_init(pairing_discovery_list_t *discovery_list, uint8_t discovery_list_size);

/** @brief Get the configured discovery list passed from the application.
 *
 *  @return The discovery list from the application
 */
pairing_discovery_list_t *pairing_address_get_discovery_list(void);

/** @brief Get the configured discovery list size passed from the application.
 *
 *  @return The discovery list size passed from the application
 */
uint8_t pairing_address_get_discovery_list_size(void);

/** @brief Generate a serialized address from the SPARK radio's chip ID.
 *
 *  @param[in] seed  Use a seed to generate a serialized address.
 *  @return The device's serialized address.
 */
uint32_t pairing_address_generate_serialized_address(uint64_t seed);

/** @brief Verify if the provided address is a reserved address that cannot be used.
 *
 *  @param[in] address  Address that need to be verified.
 *  @return A boolean indicating whether the address is reserved or not.
 */
bool pairing_address_is_address_reserved(uint32_t address);

/** @brief Look in the paired device list for an available Device ID.
 *
 *  If the node ID is not available the ID will increment until one is available.
 *
 *  @param[in] generated_node_address  Use a serialized address
 *  @return An available node ID.
 */
uint8_t pairing_address_get_available_node_id(uint8_t generated_node_address);

/** @brief Add a node into the pairing device discovery list.
 *
 *  @param[in] device_role  The pairing device role used as the list's index.
 *  @param[in] address      The address stored in the list.
 *  @param[in] unique_id    The Unique ID stored in the list.
 */
void pairing_address_add_node_to_device_discovery_list(uint8_t device_role, uint32_t address, uint64_t unique_id);

/** @brief Set the application level device role that will be used by the discovery list.
 *
 *  @param[in] pairing_device_role  The device role from the application.
 */
void pairing_address_set_device_role(uint8_t pairing_device_role);

/** @brief Get the pairing device role that was chosen from the application.
 *
 *  @return the device role from the application.
 */
uint8_t pairing_address_get_device_role(void);

#ifdef __cplusplus
}
#endif

#endif /* PAIRING_ADDRESS_H_ */
