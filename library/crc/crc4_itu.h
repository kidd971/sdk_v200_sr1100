/** @file crc4_itu.h
 *  @brief 4-bit CRC implementation.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef CRC4_ITU_H_
#define CRC4_ITU_H_

/* INCLUDES *******************************************************************/
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Calculate CRC4 of input data.
 *
 *  @param[in] crc   Existing CRC value before process a new one.
 *  @param[in] data  Pointer to data to be hashed with CRC.
 *  @param[in] len   Size of data.
 *  @return CRC value.
 */
uint8_t crc4itu(uint8_t crc, uint8_t *data, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif /* CRC4_ITU_H_ */
