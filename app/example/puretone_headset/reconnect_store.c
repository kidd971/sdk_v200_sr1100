/** @file  reconnect_store.c
 *  @brief Non-volatile store for the boot auto-reconnect pairing address.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "reconnect_store.h"
#include <string.h>
#include "puretone_headset_facade.h"

/* CONSTANTS ******************************************************************/
/* 'RCON' little-endian presence marker. Blank flash reads 0xFFFFFFFF != magic. */
#define RECONNECT_MAGIC     0x4E4F4352U
#define RECONNECT_VERSION   1U

/* TYPES **********************************************************************/
/** @brief On-flash record. Exactly 16 bytes = one flash quad-word, so the whole
 *         record is a single aligned program operation with no tail padding.
 */
typedef struct reconnect_record {
    uint32_t magic;                    /*!< RECONNECT_MAGIC presence check. */
    uint16_t version;                  /*!< Record format version.          */
    uint16_t reserved;                 /*!< Padding / future use (write 0).  */
    pairing_assigned_address_t addr;   /*!< The 4 persisted bytes.           */
    uint32_t crc32;                    /*!< CRC32 over the preceding 12 bytes.*/
} reconnect_record_t;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static uint32_t crc32_compute(const void *data, uint32_t len);

/* PUBLIC FUNCTIONS ***********************************************************/
bool reconnect_store_load(pairing_assigned_address_t *addr)
{
    reconnect_record_t record;

    if (addr == NULL) {
        return false;
    }

    if (!facade_nv_read(&record, sizeof(record))) {
        return false;
    }

    if ((record.magic != RECONNECT_MAGIC) || (record.version != RECONNECT_VERSION)) {
        return false;
    }

    if (record.crc32 != crc32_compute(&record, offsetof(reconnect_record_t, crc32))) {
        return false;
    }

    *addr = record.addr;
    return true;
}

bool reconnect_store_save(const pairing_assigned_address_t *addr)
{
    reconnect_record_t record;

    if (addr == NULL) {
        return false;
    }

    memset(&record, 0, sizeof(record));
    record.magic = RECONNECT_MAGIC;
    record.version = RECONNECT_VERSION;
    record.addr = *addr;
    record.crc32 = crc32_compute(&record, offsetof(reconnect_record_t, crc32));

    return facade_nv_write(&record, sizeof(record));
}

bool reconnect_store_clear(void)
{
    return facade_nv_erase();
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Compute a standard CRC32 (poly 0xEDB88320, reflected, init/final 0xFFFFFFFF).
 *
 *  Bitwise, no lookup table — the record is only 12 bytes, so speed is irrelevant
 *  and this keeps the module dependency-free.
 *
 *  @param[in] data  Buffer to checksum.
 *  @param[in] len   Number of bytes.
 *  @return The CRC32 value.
 */
static uint32_t crc32_compute(const void *data, uint32_t len)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFU;

    for (uint32_t i = 0; i < len; i++) {
        crc ^= bytes[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            uint32_t mask = -(crc & 1U);
            crc = (crc >> 1) ^ (0xEDB88320U & mask);
        }
    }

    return ~crc;
}
