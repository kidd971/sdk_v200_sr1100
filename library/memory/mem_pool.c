/** @file  mem_pool.c
 *  @brief Memory management for the SDK.
 *
 *  @copyright Copyright (C) 2020 SPARK Microsystems International Inc.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "mem_pool.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void mem_pool_init(mem_pool_t *mem_pool, uint8_t *pool, size_t meme_pool_size)
{
    mem_pool->mem_pool_begin = pool;
    mem_pool->capacity = meme_pool_size;
    mem_pool->free_bytes = mem_pool->capacity;
    mem_pool->mem_pool_it = mem_pool->mem_pool_begin;
    mem_pool->mem_pool_end = mem_pool->mem_pool_begin + mem_pool->capacity;
}

void *mem_pool_malloc(mem_pool_t *mem_pool, size_t wanted_size)
{
    void *ptr_ret = NULL;

    if (wanted_size & (sizeof(void *) - 1)) {
        wanted_size += (sizeof(uint32_t) - (wanted_size & (sizeof(void *) - 1)));
    }

    if (wanted_size <= mem_pool->free_bytes) {
        ptr_ret = mem_pool->mem_pool_it;
        memset(mem_pool->mem_pool_it, 0, wanted_size);
        mem_pool->mem_pool_it += wanted_size;
        mem_pool->free_bytes -= wanted_size;
    }

    return ptr_ret;
}

void mem_pool_free(mem_pool_t *mem_pool)
{
    mem_pool->free_bytes = mem_pool->capacity;
    mem_pool->mem_pool_it = mem_pool->mem_pool_begin;
    mem_pool->mem_pool_end = mem_pool->mem_pool_begin + mem_pool->capacity;
}

uint32_t mem_pool_get_allocated_bytes(mem_pool_t *mem_pool)
{
    return (mem_pool->capacity - mem_pool->free_bytes);
}
