/** @file  pseudo_data.c
 *  @brief Pseudo random data generator and validator with the help of a CRC.
 *         Often use to validate application.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <string.h>
#include "pseudo_data.h"

/* MACRO **********************************************************************/
#define EXTRACT_BYTE(x, n) (((x) >> (8 * (n))) & 0x00ff) /*!< Extract the nth (0 = 1st, 1 = 2nd,..) byte from an int */

/* CONSTANTS ******************************************************************/
#define CRC_TYPE  0xBAAD
#define CRC_SIZE sizeof(uint32_t)

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void serialize_uint32_to_uint8_array(uint32_t in_data, uint8_t *out_data);
static uint32_t get_crc(uint32_t crc, const void *buffer, size_t size);

/* PUBLIC FUNCTIONS ***********************************************************/
void pseudo_data_generate(uint8_t *data, size_t size)
{
    static size_t k;

    for (size_t j = 0; j < size; j++) {
        data[j] = j * k;
    }

    if (size > CRC_SIZE) {
        serialize_uint32_to_uint8_array(
            get_crc(CRC_TYPE, data, size - CRC_SIZE), &data[size - CRC_SIZE]);
    }
    k++;
}

bool pseudo_data_validate(uint8_t *data, size_t size)
{
    uint32_t crc    = get_crc(0xBAAD, data, size - CRC_SIZE);
    uint32_t crc_in = data[size - 1] | (data[size - 2] << 8) |
                              (data[size - 3] << 16) | (data[size - 4] << 24);
    return crc_in == crc;
}

bool pseudo_data_is_crc_populated(size_t size)
{
    return size > CRC_SIZE;
}

/* PRIVATE FUNCTIONS ***********************************************************/
/** @brief Serialize an uint32_t byte array into an uint8_t byte array
 *
 *  @param[in]  in_data   Input array in uint32_t
 *  @param[out] out_data  Output array in uint8_t
 */
static void serialize_uint32_to_uint8_array(uint32_t in_data, uint8_t *out_data)
{
    out_data[0] = EXTRACT_BYTE(in_data, 3);
    out_data[1] = EXTRACT_BYTE(in_data, 2);
    out_data[2] = EXTRACT_BYTE(in_data, 1);
    out_data[3] = EXTRACT_BYTE(in_data, 0);
}

/** @brief  Compute CRC on a buffer.
 *
 *  @param[in] crc     CRC seed value.
 *  @param[in] buffer  Pointer to the buffer for the CRC
 *  @param[in] size    size of the buffer
 *  @return CRC value calculated.
 */
static uint32_t get_crc(uint32_t crc, const void *buffer, size_t size)
{
    static const uint32_t rtable[16] = {
        0x00000000,
        0x1db71064,
        0x3b6e20c8,
        0x26d930ac,
        0x76dc4190,
        0x6b6b51f4,
        0x4db26158,
        0x5005713c,
        0xedb88320,
        0xf00f9344,
        0xd6d6a3e8,
        0xcb61b38c,
        0x9b64c2b0,
        0x86d3d2d4,
        0xa00ae278,
        0xbdbdf21c,
    };

    const uint8_t *data = buffer;

    for (size_t i = 0; i < size; i++) {
        crc = (crc >> 4) ^ rtable[(crc ^ (data[i] >> 0)) & 0xf];
        crc = (crc >> 4) ^ rtable[(crc ^ (data[i] >> 4)) & 0xf];
    }

    return crc;
}
