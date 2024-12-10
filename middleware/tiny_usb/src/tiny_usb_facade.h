/** @file  tiny_usb_facade.h
 * @brief Provides an abstraction layer over hardware-specific functionality for USB operations.
 *        Designed to simplify the integration of USB features by abstracting direct hardware interactions
 *        through the TinyUSB library, this file includes interfaces for device enumeration, serial number
 *        retrieval, and more, facilitating portable and hardware-agnostic USB implementations.
 *
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef TINY_USB_FACADE_H_
#define TINY_USB_FACADE_H_

/* INCLUDES *******************************************************************/
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Retrieve the unique ID of the board to use as a USB serial number.
 *
 * This function is designed to fetch a unique identifier for the board which
 * can then be utilized as a USB serial number. The unique ID is copied into
 * the provided array up to a maximum length specified by the caller. This
 * function is particularly useful for USB device enumeration and identification
 * purposes, ensuring that each device can be uniquely identified by its serial number.
 *
 * @param[out] id       A pointer to the buffer where the unique ID,encoded in UTF-16, should be stored.
 * @param[in]  max_len  The maximum number of bytes to copy into the `id` buffer.
 *
 * @return size_t The number of bytes actually written into the `id` buffer. If the unique ID is
 *                shorter than `max_len`, only the actual bytes of the ID are copied, and the
 *                function returns the number of bytes copied. If an error occurs, the function
 *                may return 0, indicating that no valid ID was retrieved.
 */
size_t tuh_get_serial_id(uint8_t id[], size_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* TINY_USB_FACADE_H_ */
