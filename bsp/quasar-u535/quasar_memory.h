/** @file  quasar_memory.h
 *  @brief The memory module contains all functionalities related to the flash memory.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_MEMORY_H_
#define QUASAR_MEMORY_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/* Current __COUNTER__ value. */
enum {
    QUASAR_MEMORY_COUNTER_BASE = __COUNTER__
};

/* MACROS *********************************************************************/
/** @brief Create a local counter macro.
 */
#define QUASAR_MEMORY_COUNTER (__COUNTER__ - QUASAR_MEMORY_COUNTER_BASE)

/** @brief Automatically increase error count each time invoked. Starts at -(0 + 1) = -1.
 */
#define QUASAR_MEMORY_GENERATE_ERR_CODE (-(QUASAR_MEMORY_COUNTER + 1))

/* TYPES **********************************************************************/
/** @brief Memory error status.
 */
typedef enum quasar_memory_error {
    /*! No error was detected. */
    QUASAR_MEM_ERR_NONE = 0,
    /*! Write flash error. */
    QUASAR_MEM_ERR = QUASAR_MEMORY_GENERATE_ERR_CODE,
    /*! Flash is busy. */
    QUASAR_MEM_ERR_BUSY = QUASAR_MEMORY_GENERATE_ERR_CODE,
    /*! Flash write/read timeout. */
    QUASAR_MEM_ERR_TIMEOUT = QUASAR_MEMORY_GENERATE_ERR_CODE,
    /*! Flash function argument error. */
    QUASAR_MEM_ERR_ARGUMENT = QUASAR_MEMORY_GENERATE_ERR_CODE,
    /*! There is an unknown error occurring. */
    QUASAR_MEM_ERR_UNKNOWN = QUASAR_MEMORY_GENERATE_ERR_CODE,
} quasar_memory_error_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Read data from the flash memory and put it in a buffer.
 *
 *  @param[in] flash_address  Flash memory address to write.
 *  @param[in] read_buffer    The read data buffer.
 *  @param[in] buffer_size    The size of the buffer to be read.
 *  @return The quasar error code.
 */
quasar_memory_error_t quasar_memory_read(uint32_t flash_address, const void *read_buffer, uint32_t buffer_size);

/** @brief Write a buffer containing data to the flash memory.
 *
 *  @note The block must be previously erased.
 *
 *  @param[in] flash_address  Flash memory address to write.
 *  @param[in] write_buffer   The write data buffer.
 *  @param[in] buffer_size    The size of the buffer to be written.
 *  @return The quasar error code.
 */
quasar_memory_error_t quasar_memory_write(uint32_t flash_address, const void *write_buffer, uint32_t buffer_size);

/** @brief Erase memory blocks of the MCU's flash memory.
 *
 *  @note The state of an erased block is undefined. A block must be erased before being programmed.
 *
 *  @param[in] address  The address of the flash memory.
 *  @return The quasar error code.
 */
quasar_memory_error_t quasar_memory_erase(uint32_t address);

/** @brief Invalidate the cache (syncs the cache).
 *
 *  @return The quasar error code.
 */
quasar_memory_error_t quasar_memory_invalidate_cache(void);

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_MEMORY_H_ */
