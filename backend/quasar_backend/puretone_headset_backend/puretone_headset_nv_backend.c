/** @file  puretone_headset_nv_backend.c
 *  @brief Reserved-page flash access for boot auto-reconnect (facade_nv_*).
 *
 *  Wraps the quasar_memory flash driver and hides the board-specific reserved
 *  page address (`_user_data_base`, provided by the linker script) behind the
 *  byte-oriented facade_nv_* API. The record format (magic / version / CRC)
 *  lives in the caller (reconnect_store); this file only moves raw bytes and
 *  handles the flash gotchas: erase-before-write, quad-word (16-byte) write
 *  granularity, and instruction-cache invalidation after programming.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "puretone_headset_facade.h"
#include "quasar_memory.h"
#include "stm32u5xx_hal.h"

/* CONSTANTS ******************************************************************/
/* Reserved user-data page size. Both chips (U535 and U5A5) use an 8 KB page
 * (FLASH_PAGE_SIZE == 0x2000); the linker reserves exactly the last page. */
#define NV_PAGE_SIZE   FLASH_PAGE_SIZE

/* Flash quad-word programming granularity (bytes). */
#define NV_QUAD_WORD   16U

/* Base of the reserved page, defined per board in the linker script. Referenced
 * by address only; the region itself is outside the FLASH memory region so the
 * linker never places code there. */
extern uint32_t _user_data_base;

/* PRIVATE FUNCTIONS **********************************************************/
static inline uint32_t nv_base_address(void)
{
    return (uint32_t)&_user_data_base;
}

/* PUBLIC FUNCTIONS ***********************************************************/
bool facade_nv_read(void *dst, uint32_t len)
{
    if ((dst == NULL) || (len == 0) || (len > NV_PAGE_SIZE)) {
        return false;
    }

    return quasar_memory_read(nv_base_address(), dst, len) == QUASAR_MEM_ERR_NONE;
}

bool facade_nv_write(const void *src, uint32_t len)
{
    /* len must be a multiple of the 16-byte quad-word: the driver programs a
     * full quad-word even for the tail chunk, so a non-multiple length would
     * make it read past the source buffer. */
    if ((src == NULL) || (len == 0) || (len > NV_PAGE_SIZE) || ((len % NV_QUAD_WORD) != 0)) {
        return false;
    }

    /* ECC: each quad-word can be programmed only once after an erase, so always
     * erase the page before writing. */
    if (quasar_memory_erase(nv_base_address()) != QUASAR_MEM_ERR_NONE) {
        return false;
    }

    if (quasar_memory_write(nv_base_address(), src, len) != QUASAR_MEM_ERR_NONE) {
        return false;
    }

    /* Drop any stale copy of the just-written region from the instruction cache. */
    return quasar_memory_invalidate_cache() == QUASAR_MEM_ERR_NONE;
}

bool facade_nv_erase(void)
{
    if (quasar_memory_erase(nv_base_address()) != QUASAR_MEM_ERR_NONE) {
        return false;
    }

    return quasar_memory_invalidate_cache() == QUASAR_MEM_ERR_NONE;
}
