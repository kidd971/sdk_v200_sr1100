/** @file  swc_utils.h
 *  @brief SPARK Wireless Core Utilities.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SWC_UTILS_H_
#define SWC_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* MACROS *********************************************************************/
/*! Get the number of elements in an array of any types. */
#define SWC_ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))
/*! Concatenate two 8-bit values into a single 16-bit value. */
#define SWC_CONCAT_8B_TO_16B(MSB, LSB) ((MSB) << 8 | (LSB))
/*! Extract the nth (0 = 1st, 1 = 2nd,..) byte from an int value.
 *  byte_postion is byte position to be extracted.
 *  value is the int value where an specific byte will be extracted.
 */
#define SWC_EXTRACT_BYTE(value, byte_position) (((value) >> (8 * (byte_position))) & 0x00ff)

/*! Single bit mask. */
#define SWC_BIT(n) (1 << (n))
/*! Bit mask to identify Auto reply timeslot (auto reply timeslot). */
#define SWC_BIT_AUTO_REPLY_TIMESLOT SWC_BIT(7)
/*! User MACRO to identify primary timeslot. */
#define SWC_MAIN_TIMESLOT(x) ((x) & 0x7F)
/*! User MACRO to identify data in auto-reply timeslot (auto reply timeslot). */
#define SWC_AUTO_TIMESLOT(x) ((x) | SWC_BIT_AUTO_REPLY_TIMESLOT)

#ifdef __cplusplus
}
#endif

#endif /* SWC_UTILS_H_ */
