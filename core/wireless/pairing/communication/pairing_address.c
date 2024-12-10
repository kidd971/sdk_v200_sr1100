/** @file  pairing_address.c
 *  @brief This file handles the pairing address management.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "pairing_address.h"
#include <string.h>
#include "sr_utils.h"

/* CONSTANTS ******************************************************************/
#define GENERATE_SERIALIZED_LEN              4
#define GENERATE_SERIALIZED_CRC_POLY         0x1021
#define GENERATE_SERIALIZED_CRC_CCITT_RELOAD 0xFFFFFFFF

/* PRIVATE GLOBALS ************************************************************/
static pairing_discovery_list_t *local_discovery_list;
static uint8_t local_discovery_list_size;
static uint8_t local_pairing_device_role;
static pairing_assigned_address_t *local_assigned_pairing_address;

/* PUBLIC FUNCTIONS ***********************************************************/
void pairing_address_init(pairing_assigned_address_t *pairing_assigned_address)
{
    local_assigned_pairing_address = pairing_assigned_address;
    pairing_address_reset();
}

void pairing_address_set_pan_id(uint16_t pan_id)
{
    local_assigned_pairing_address->pan_id = pan_id;
}

void pairing_address_set_coordinator_address(uint8_t coordinator_address)
{
    local_assigned_pairing_address->coordinator_address = coordinator_address;
}

void pairing_address_set_node_address(uint8_t node_address)
{
    local_assigned_pairing_address->node_address = node_address;
}

uint16_t pairing_address_get_pan_id(void)
{
    return local_assigned_pairing_address->pan_id;
}

uint8_t pairing_address_get_coordinator_address(void)
{
    return local_assigned_pairing_address->coordinator_address;
}

uint8_t pairing_address_get_node_address(void)
{
    return local_assigned_pairing_address->node_address;
}

void pairing_address_reset(void)
{
    memset(local_assigned_pairing_address, 0, sizeof(*local_assigned_pairing_address));
}

void pairing_address_discovery_list_init(pairing_discovery_list_t *discovery_list, uint8_t discovery_list_size)
{
    local_discovery_list = discovery_list;
    local_discovery_list_size = discovery_list_size;
}

pairing_discovery_list_t *pairing_address_get_discovery_list(void)
{
    return local_discovery_list;
}

uint8_t pairing_address_get_discovery_list_size(void)
{
    return local_discovery_list_size;
}

uint32_t pairing_address_generate_serialized_address(uint64_t seed)
{
    uint8_t byte_array[sizeof(uint64_t)];
    uint32_t result = GENERATE_SERIALIZED_CRC_CCITT_RELOAD;
    uint32_t crc = 0;

    do {
        /* Split it into bytes in a table. */
        memcpy(byte_array, &seed, (sizeof(uint64_t)));

        for (uint8_t i = 0; i < GENERATE_SERIALIZED_LEN; i++) {
            crc = result;
            for (uint8_t j = 0; j < 8; j++) {
                if (crc & 0x80000) {  /* Most significant bit. */
                    crc = (crc << 1) ^ GENERATE_SERIALIZED_CRC_POLY;
                } else {
                    crc <<= 1;
                }
            }
            result = crc ^ byte_array[i];
        }

        /* Only keep 20 bits for the PAN ID (12 bits) and the coordinator address (8 bits). */
        result = result & 0xFFFFF;
        seed += 1;

    } while (pairing_address_is_address_reserved(result));

    return result;
}

bool pairing_address_is_address_reserved(uint32_t address)
{
    uint8_t result_syncword;
    uint8_t result_network;
    uint8_t result_address;

    result_syncword = EXTRACT_BYTE(address, 2);
    result_network  = EXTRACT_BYTE(address, 1);
    result_address  = EXTRACT_BYTE(address, 0);

    /* Verify if the result lands on a reserved address */
    if (result_syncword == 0x00 ||
        result_network == 0x00 || result_network == 0xFF ||
        result_address == 0x00 || result_address == 0xFF) {
        return true;
    } else {
        return false;
    }
}

uint8_t pairing_address_get_available_node_id(uint8_t generated_node_address)
{
    bool address_is_available;
    uint8_t result_id;

    /* Use the serialized address */
    result_id = generated_node_address;

    /* Verify if this address is available. */
    do {
        address_is_available = true;

        for (uint8_t i = 0; i < local_discovery_list_size; i++) {
            if (result_id == local_discovery_list[i].node_address) {
                address_is_available = false;
            }
        }

        /* If the address already exists, increment again. */
        if (!address_is_available) {
            /* Avoid using address 0x00 and 0xFF since they are reserved. */
            if (result_id == 0xFE) {
                result_id = 0x01;
            } else {
                result_id++;
            }
        }
    } while (!address_is_available);

    return result_id;
}

void pairing_address_add_node_to_device_discovery_list(uint8_t device_role, uint32_t address, uint64_t unique_id)
{
    if (device_role < local_discovery_list_size) {
        /* Add the address and unique ID to the paired device list. */
        local_discovery_list[device_role].node_address = address;
        local_discovery_list[device_role].unique_id = unique_id;
    }
}

void pairing_address_set_device_role(uint8_t pairing_device_role)
{
    local_pairing_device_role = pairing_device_role;
}

uint8_t pairing_address_get_device_role(void)
{
    return local_pairing_device_role;
}
