/** @file  evk_mpu.c
 *  @brief This module is used to enable the Memory Protection Unit.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "evk_mpu.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void evk_mpu_enable(void)
{
    /* Stack end address is a symbol provided by the linker script. */
    extern char _stack_end_address;

    /* Disable MPU. */
    MPU->CTRL = 0;
    /* Protect first 512 bytes addresses of flash to catch NULL ptr dereferences.
     * Use MPU region 7 (highest priority).
     */
    MPU->RBAR = 0x0U | MPU_RBAR_VALID_Msk | MPU_REGION_NUMBER7;
    MPU->RASR = MPU_REGION_SIZE_512B << MPU_RASR_SIZE_Pos |
                MPU_REGION_NO_ACCESS << MPU_RASR_AP_Pos |
                MPU_RASR_C_Msk |
                MPU_RASR_ENABLE_Msk;
    /* Protect 32 bytes around the end of stack to catch stack overflows (approximative since the address mask forces 32-byte alignment).
     * Mask the 5 least significant bits of target address as they are used for VALID bit (0x10) & REGION NUMBER (0xF).
     * Use MPU region 6 (second highest priority).
     */
    MPU->RBAR = (((uint32_t)&_stack_end_address) & MPU_RBAR_ADDR_Msk) |
                MPU_RBAR_VALID_Msk |
                MPU_REGION_NUMBER6;
    MPU->RASR = MPU_REGION_SIZE_32B << MPU_RASR_SIZE_Pos |
                MPU_REGION_NO_ACCESS << MPU_RASR_AP_Pos |
                MPU_RASR_S_Msk |
                MPU_RASR_C_Msk |
                MPU_RASR_ENABLE_Msk;
    /* Enable MPU with default memory mapping (PRIVDEFENA) and our user defined regions. */
    MPU->CTRL = MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_ENABLE_Msk;
    /* Instruction & memory barrier required to ensure that the pipeline is flushed and no
     * further operations are executed until the memory access that enables the MPU completes.
     */
    __ISB();
    __DSB();
}
