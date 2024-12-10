/** @file  swc_utils.h
 *  @brief SPARK Wireless Core Utilities.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
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
/*! Get the number of elements in an array of any types */
#define SWC_ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))
/*! Concatenate two 8-bit values into a single 16-bit value */
#define SWC_CONCAT_8B_TO_16B(MSB, LSB) (MSB << 8 | LSB)

#ifdef __cplusplus
}
#endif

#endif /* SWC_UTILS_H_ */
