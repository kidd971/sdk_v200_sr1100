/** @file  pseudo_data.h
 *  @brief Pseudo random data generator and validator with the help of a CRC.
 *         Often use to validate application.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef PSEUDO_DATA_H_
#define PSEUDO_DATA_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Fill an array with pseudo generated data and CRC
 *
 *  @param[in] data   Pointer to array to put data.
 *  @param[in] size  Size of array
 */
void pseudo_data_generate(uint8_t *data, size_t size);

/** @brief Validate the CRC of a packet received generated with pseudo data.
 *
 *  @param[in] data   Pointer to array to put data.
 *  @param[in] size  Size of array
 *  @return  True if CRC is match, False otherwise.
 */
bool pseudo_data_validate(uint8_t *data, size_t size);

/** @brief Validate if CRC is present.
 *
 *  @param[in] size  Size of payload received
 *  @return  True if CRC is present, False otherwise.
 */
bool pseudo_data_is_crc_populated(size_t size);

#ifdef __cplusplus
}
#endif

#endif /* PSEUDO_DATA_H_ */
