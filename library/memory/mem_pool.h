/** @file  mem_pool.h
 *  @brief Memory management for the SDK.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef MEM_POOL_H_
#define MEM_POOL_H_

/* INCLUDES *******************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Memory pool structure.
 */
typedef struct {
    /*! Pointer to the beginning of the memory pool. */
    uint8_t *mem_pool_begin;
    /*! Total capacity of the memory pool in bytes. */
    uint32_t capacity;
    /*! Number of free bytes available in the memory pool. */
    uint32_t free_bytes;
    /*! Pointer to the end of the memory pool. */
    uint8_t *mem_pool_end;
    /*! Iterator pointer for memory allocation within the pool. */
    uint8_t *mem_pool_it;
} mem_pool_t;

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Memory pool module initialization.
 *
 *  @param[in] mem_pool       Memory pool handler.
 *  @param[in] pool           Memory pool pointer to array.
 *  @param[in] mem_pool_size  Memory pool size of array.
 */
void mem_pool_init(mem_pool_t *mem_pool, uint8_t *pool, size_t mem_pool_size);

/** @brief Memory pool allocation.
 *
 *  @param[in] mem_pool     Memory pool handler.
 *  @param[in] wanted_size  User wanted size.
 *  @return Pointer to first element of asked memory.
 */
void *mem_pool_malloc(mem_pool_t *mem_pool, size_t wanted_size);

/** @brief Free every bloc of memory previously allocated.
 *
 *  @param[in] mem_pool  Memory pool handler.
 */
void mem_pool_free(mem_pool_t *mem_pool);

/** @brief Get the number of bytes allocated from the pool.
 *
 *  @param[in] mem_pool  Memory pool handle.
 *  @return Number of bytes allocated.
 */
uint32_t mem_pool_get_allocated_bytes(const mem_pool_t *mem_pool);

#ifdef __cplusplus
}
#endif

#endif /* MEM_POOL_H_ */
