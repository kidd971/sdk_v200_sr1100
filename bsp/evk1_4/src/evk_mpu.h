/** @file  evk_mpu.h
 *  @brief This module is used to enable the Memory Protection Unit.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef EVK_MPU_H_
#define EVK_MPU_H_

/* INCLUDES *******************************************************************/
#include "evk_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Enables the MPU to protect memory regions:
 *          - Small region at 0x0000 0000 to catch any NULL pointer dereferencing,
 *          - Small region at the end of the stack to catch stack overflows.
 */
void evk_mpu_enable(void);

#ifdef __cplusplus
}
#endif

#endif /* EVK_MPU_H_ */
