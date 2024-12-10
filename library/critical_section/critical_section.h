/** @file  critical_section.h
 *  @brief This file contains functions for entering and exiting critical sections.
 *
 *         The critical section is a region of code where interrupts are temporarily
 *         disabled to ensure atomicity of certain operations. The implementation
 *         takes into account nested critical sections.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef CRITICAL_SECTION_H_
#define CRITICAL_SECTION_H_

/* INCLUDES *******************************************************************/
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Function for entering a critical region.
 *
 *  @note This function disables interrupts and increments the nesting level
 *        of critical sections. It is designed to handle nested critical sections.
 */
void CRITICAL_SECTION_ENTER(void);

/** @brief Function for leaving a critical region.
 *
 *  @note This function decrements the nesting level of critical sections.
 *        If the nesting level becomes zero, interrupts are re-enabled.
 */
void CRITICAL_SECTION_EXIT(void);

#ifdef __cplusplus
}
#endif

#endif /* CRITICAL_SECTION_H_ */
