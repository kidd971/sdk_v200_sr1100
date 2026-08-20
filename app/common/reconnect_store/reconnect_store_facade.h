/** @file  reconnect_store_facade.h
 *  @brief Hardware interface required by the reconnect store.
 *
 *  Three byte-oriented calls onto one reserved flash page. Each application
 *  backend implements them; the page address itself (`_user_data_base`) comes
 *  from the linker script and never appears above this line.
 *
 *  This header exists so reconnect_store.c depends on a contract rather than on
 *  one application's facade. It used to include puretone_headset_facade.h, which
 *  was the only thing tying an otherwise board-agnostic file to a single
 *  application -- and the reason a second application could not use it without
 *  copying it.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 */
#ifndef RECONNECT_STORE_FACADE_H_
#define RECONNECT_STORE_FACADE_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Read the boot auto-reconnect record from the reserved flash page.
 *
 *  Copies @p len bytes from the reserved user-data page (`_user_data_base`,
 *  board-specific address handled by the linker) into @p dst. Raw bytes only --
 *  the record format (magic / version / CRC) lives in the caller.
 *
 *  @param[out] dst  Destination buffer.
 *  @param[in]  len  Number of bytes to read (must be <= reserved page size).
 *  @return true on success; false on argument error.
 */
bool facade_nv_read(void *dst, uint32_t len);

/** @brief Erase the reserved page and write the boot auto-reconnect record.
 *
 *  Erases the reserved user-data page, programs @p len bytes at `_user_data_base`,
 *  then invalidates the instruction cache. @p len must be a multiple of 16 bytes
 *  (flash quad-word granularity) and fit in one page.
 *
 *  @param[in] src  Source buffer.
 *  @param[in] len  Number of bytes to write (multiple of 16, <= page size).
 *  @return true on success; false on argument error or a flash fault.
 */
bool facade_nv_write(const void *src, uint32_t len);

/** @brief Erase the reserved boot auto-reconnect page.
 *
 *  Leaves the page in the blank (0xFF) state so the next read fails the magic
 *  check and the device falls through to pairing.
 *
 *  @return true on success; false on a flash fault.
 */
bool facade_nv_erase(void);

#ifdef __cplusplus
}
#endif

#endif /* RECONNECT_STORE_FACADE_H_ */
