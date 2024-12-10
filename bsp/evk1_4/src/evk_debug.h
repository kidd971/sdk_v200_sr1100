/** @file  evk_debug.h
 *  @brief This module contains debug helpers.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef EVK_DEBUG_H_
#define EVK_DEBUG_H_

/* INCLUDES *******************************************************************/
#include "evk_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* MACROS *********************************************************************/
/* This assembly code recovers the stack pointer prior to exception and sets it
 * in R0 (or ARG0), the first argument passed to the "Hard_Fault_Handler"
 * function. To use this macro, make sure to define the Hard_Fault_Handler
 * function and make it available in the compilation unit where the macro will
 * be used.
 */
#define HARDFAULT_HANDLING_ASM(_x)  \
    (__asm volatile(                \
        "tst lr, #4\n"              \
        "ite eq\n"                  \
        "mrseq r0, msp\n"           \
        "mrsne r0, psp\n"           \
        "b Hard_Fault_Handler\n"    \
    ))

/* CONSTANTS ******************************************************************/
#ifndef DEBUG
#define DEBUG false /* Should be overwritten by project settings for debug builds */
#endif
#if defined(DEBUG) && DEBUG == true
#define STACK_DUMP_STRING_SIZE  400
#define FAULT_REGISTER_DUMP_STRING_SIZE  300
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Print a pre-formatted exception dump through the integrated ST-Link's UART interface.
 *
 *  @note The DEBUG preprocessor symbol must be defined and set to "true" for this function to perform any action.
 *
 *  @param stack_frame  Stack pointer pointing to the top of the stack frame to fetch information from.
 */
void evk_debug_exception_dump(uint32_t *stack_frame);

#ifdef __cplusplus
}
#endif

#endif /* EVK_DEBUG_H_ */
