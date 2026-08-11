/** @file crc8.h
 *  @brief 8-bit CRC implementation.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef CRC8_H_
#define CRC8_H_

/* INCLUDES *******************************************************************/
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Calculate CRC8 of input data.
 *
 *  @param[in] crc   Existing CRC value before processing new data.
 *  @param[in] data  Pointer to data to be hashed with CRC.
 *  @param[in] len   Size of data in bytes.
 *  @return CRC value.
 */
uint8_t crc8(uint8_t crc, const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* CRC8_H_ */
