/** @file  backend.c
 *  @brief Implement the abstraction layer over hardware-specific functionality for USB operations.
 *         Designed to simplify the integration of USB features by abstracting direct hardware interactions
 *         through the TinyUSB library, this file includes interfaces for device enumeration, serial number
 *         retrieval, and more, facilitating portable and hardware-agnostic USB implementations.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "stm32u5xx.h"
#include "tiny_usb_facade.h"


/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void int_to_unicode(uint32_t value, uint8_t *pbuf, uint8_t len);

/* PUBLIC FUNCTIONS ***********************************************************/
size_t tuh_get_serial_id(uint8_t desc_str1[], size_t max_chars) {

    (void)max_chars;
    size_t serial_len = 12;
    uint8_t str[12 + 1];
    uint32_t uid0, uid1, uid2;

    uid0 = *(uint32_t *) (UID_BASE);
    uid1 = *(uint32_t *) (UID_BASE + 0x4);
    uid2 = *(uint32_t *) (UID_BASE + 0x8);

    uid0 += uid2;

    /* Convert UID to ASCII */
    if (uid0 != 0) {
        int_to_unicode(uid0, &str[0], 8);
        int_to_unicode(uid1, &str[8], 4);
    }

    /* Convert ASCII string into UTF-16 */
    for ( volatile size_t i = 0; i < serial_len; i++ ) {
        desc_str1[i * 2] = str[i];
    }

    return serial_len;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Convert Hex 32Bits value into char.
 *
 *  @param[in] value  Value to set.
 *  @param[in] pbuf   String buffer to write.
 *  @param[in] len    Size of string buffer.
 */
static void int_to_unicode(uint32_t value, uint8_t *pbuf, uint8_t len)
{
    uint8_t idx = 0;

    for (idx = 0; idx < len; idx++) {
        if ((value >> 28) < 0xA) {
            pbuf[idx] = (value >> 28) + '0';
        } else {
            pbuf[idx] = (value >> 28) + 'A' - 10;
        }

        value = value << 4;

        /* Mark end of string. */
        pbuf[idx + 1] = 0;
    }
}
