/** @file  reconnect_store.h
 *  @brief Non-volatile store for the boot auto-reconnect pairing address.
 *
 *  Persists the 4-byte pairing_assigned_address_t across power cycles so a
 *  previously paired device can re-establish its link on boot without re-running
 *  the pairing procedure (see boot_auto_reconnect_design.md).
 *
 *  Board-agnostic: the record format (magic / version / CRC32) lives here; the
 *  actual flash access goes through the byte-oriented facade_nv_* API, which
 *  hides the board-specific reserved page address.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef RECONNECT_STORE_H_
#define RECONNECT_STORE_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "pairing_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Load the persisted pairing address from flash.
 *
 *  Reads the reserved page and validates magic, version and CRC32. A blank
 *  (never-written) page reads as 0xFF and fails the magic check, so a factory
 *  device naturally reports "no record".
 *
 *  @param[out] addr  Filled with the stored address on success; untouched otherwise.
 *  @return true if a valid record was found and copied into @p addr; false if
 *          there is no valid record (blank, corrupt, or wrong version).
 */
bool reconnect_store_load(pairing_assigned_address_t *addr);

/** @brief Persist the pairing address to flash.
 *
 *  Erases and rewrites the reserved page with a fresh record. Call after a
 *  successful pairing.
 *
 *  @param[in] addr  Address to persist.
 *  @return true on success; false on a flash fault or bad argument.
 */
bool reconnect_store_save(const pairing_assigned_address_t *addr);

/** @brief Erase any persisted pairing address.
 *
 *  Call on unpair so the next boot does not reconnect to the device the user
 *  just removed.
 *
 *  @return true on success; false on a flash fault.
 */
bool reconnect_store_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* RECONNECT_STORE_H_ */
