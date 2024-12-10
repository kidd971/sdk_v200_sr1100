/** @file  evk_flash.h
 *  @brief This module is composed of all functionalities related to the MCU's flash memory.
 *
 *  @copyright Copyright (C) 2020-2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef EVK_FLASH_H_
#define EVK_FLASH_H_

/* INCLUDES *******************************************************************/
#include "evk_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Program a region in a block of the MCU's flash memory.
 *
 *  The block must be previously erased.
 *
 *  @param[in] address  Flash memory address to write.
 *  @param[in] buffer   Buffer to write.
 *  @param[in] size     Size of the buffer.
 *
 *  @return HAL status code.
 */
uint8_t evk_flash_prog(uint32_t address, const void *buffer, uint32_t size);

/** @brief Erase a memory block of the MCU's flash memory.
 *
 *  The state of an erased block is undefined.
 *  A block must be erased before being programmed.
 *
 *  @param[in] block_count  Number of erasable blocks on the device.
 *  @param[in] block        Block index.
 *
 *  @return HAL status code.
 */
uint8_t evk_flash_erase(uint32_t block_count, uint32_t block);

#ifdef __cplusplus
}
#endif

#endif /* EVK_FLASH_H_ */

