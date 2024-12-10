/** @file  quasar_memory.h
 *  @brief The memory module contains all functionalities related to the flash memory.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_MEMORY_H_
#define QUASAR_MEMORY_H_

/* INCLUDES *******************************************************************/
#include "quasar_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Memory error status.
 */
typedef enum quasar_memory_error {
    /*! No error was detected. */
    QUASAR_MEMORY_ERROR_NO_ERROR = 0,
    /*! Write flash error. */
    QUASAR_MEMORY_ERROR,
    /*! Flash is busy. */
    QUASAR_MEMORY_ERROR_BUSY,
    /*! Flash write/read timeout. */
    QUASAR_MEMORY_ERROR_TIMEOUT,
    /*! Flash write error. */
    QUASAR_MEMORY_ERROR_WRITE,
    /*! Flash function argument error. */
    QUASAR_MEMORY_ERROR_ARGUMENT,
    /*! There is an unknown error occurring. */
    QUASAR_MEMORY_ERROR_UNKNOWN,
} quasar_memory_error_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Read data from the flash memory and put it in a buffer.
 *
 *  @param[in] flash_address  Flash memory address to write.
 *  @param[in] read_buffer    The read data buffer.
 *  @param[in] buffer_size    The size of the buffer to be read.
 *  @param[in] err            Memory error status.
 */
void quasar_memory_read(uint32_t flash_address, const void *read_buffer, uint32_t buffer_size, quasar_memory_error_t *err);

/** @brief Write a buffer containing data to the flash memory.
 *
 *  @note The block must be previously erased.
 *
 *  @param[in] flash_address  Flash memory address to write.
 *  @param[in] write_buffer   The write data buffer.
 *  @param[in] buffer_size    The size of the buffer to be written.
 *  @param[in] err            Memory error status.
 */
void quasar_memory_write(uint32_t flash_address, const void *write_buffer, uint32_t buffer_size, quasar_memory_error_t *err);

/** @brief Erase a memory block of the MCU's flash memory.
 *
 *  @note  The state of an erased block is undefined.
 *         A block must be erased before being programmed.
 *
 *  @param[in] flash_address  Flash memory address to erase.
 *  @param[in] err            Memory error status.
 */
void quasar_memory_erase(uint32_t flash_address, quasar_memory_error_t *err);

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_MEMORY_H_ */

