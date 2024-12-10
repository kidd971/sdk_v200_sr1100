/** @file  critical_section.c
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

/* INCLUDES *******************************************************************/
#include "critical_section.h"

/* PRIVATE GLOBALS ************************************************************/
static volatile uint32_t m_in_critical_region;

/* PUBLIC FUNCTIONS ***********************************************************/
void CRITICAL_SECTION_ENTER(void)
{
    __asm volatile("cpsid i" : : : "memory");
    ++m_in_critical_region;
}

void CRITICAL_SECTION_EXIT(void)
{
    if (--m_in_critical_region == 0) {
        __asm volatile("cpsie i" : : : "memory");
    }
}
