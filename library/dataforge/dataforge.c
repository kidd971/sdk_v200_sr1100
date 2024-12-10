/** @file  dataforge.c
 *  @brief *  @brief Pseudo random data generator/validator with the help of a CRC and data pattern generator/validator.
 *         Often use to validate application.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <string.h>
#include "dataforge.h"

/* MACRO **********************************************************************/
#define EXTRACT_BYTE(x, n) (((x) >> (8 * (n))) & 0x00ff) /*!< Extract the nth (0 = 1st, 1 = 2nd,..) byte from an int */

/* CONSTANTS ******************************************************************/
#define CRC_TYPE          0xBAAD
#define CRC_SIZE          sizeof(uint32_t)
#define SEQ_NUM_VAL_RANGE 0xFF
#define SEQ_NUM_INDEX     0

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void memcpy_32bits(char *dest, uint32_t *src, size_t n);
static void serialize_uint32_to_uint8_array(uint32_t in_data, uint8_t *out_data);
static uint32_t get_crc(uint32_t crc, const void *buffer, size_t size);

/* PUBLIC FUNCTIONS ***********************************************************/
void dataforge_generate_pseudo(uint8_t *data, size_t size, uint8_t seq_num)
{
    static size_t k;

    for (size_t j = 0; j < size; j++) {
        data[j] = j * k;
    }

    data[SEQ_NUM_INDEX] = seq_num;

    if (size > CRC_SIZE) {
        serialize_uint32_to_uint8_array(
            get_crc(CRC_TYPE, data, size - CRC_SIZE), &data[size - CRC_SIZE]);
    }
    k++;
}

bool dataforge_validate_pseudo_crc(uint8_t *data, size_t size)
{
    uint32_t crc    = get_crc(0xBAAD, data, size - CRC_SIZE);
    uint32_t crc_in = data[size - 1] | (data[size - 2] << 8) |
                              (data[size - 3] << 16) | (data[size - 4] << 24);
    return crc_in == crc;
}

bool dataforge_is_pseudo_crc_populated(size_t size)
{
    return size > CRC_SIZE;
}

dataforge_seq_status_t dataforge_validate_seq_num(uint8_t *data, uint8_t seq_num)
{
    if (data[SEQ_NUM_INDEX] == dataforge_increment_seq_num(seq_num)) {
        return DATAFORGE_MATCHING_SEQ;
    } else if (data[SEQ_NUM_INDEX] == seq_num) {
        return DATAFORGE_DUPLICATE_SEQ;
    } else {
        return DATAFORGE_NON_MATCHING_SEQ;
    }
}

uint8_t dataforge_increment_seq_num(uint8_t seq_num)
{
    return (seq_num + 1) % SEQ_NUM_VAL_RANGE;
}

uint8_t dataforge_extract_seq_num(uint8_t *payload)
{
    return payload[SEQ_NUM_INDEX];
}

void dataforge_generate_pattern(char *payload, uint32_t *payload_data, uint8_t payload_size, uint8_t payload_data_count)
{
    uint8_t i = 0;

    if (payload_data_count != 0) {
        while (i < payload_size) {
            if ((payload_size - i) > payload_data_count) {
                memcpy_32bits(&payload[i], payload_data, payload_data_count);
                i += payload_data_count;
            } else {
                memcpy_32bits(&payload[i], payload_data, payload_size - i);
                i = payload_size;
            }
        }
    }
}

bool dataforge_validate_pattern(uint8_t *payload, uint32_t *pattern, uint8_t payload_size, uint8_t pattern_size)
{
    if (pattern_size == 0) {
        return false;
    }

    for (uint8_t i = 0; i < payload_size; i++) {
        if (payload[i] != pattern[i % pattern_size]) {
            return false;
        }
    }

    return true;
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

/** @brief Copy content from uint32 array to char array.
 *
 *  @param[in] dest  Destination buffer.
 *  @param[in] src   Source buffer
 *  @param[in] n     Size to copy
 */
static void memcpy_32bits(char *dest, uint32_t *src, size_t n)
{
    for (uint8_t i = 0; i < n; i++) {
        dest[i] = src[i];
    }
}
