/** @file  evk_debug.c
 *  @brief This module contains debug helpers.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* Includes *******************************************************************/
#include "evk_debug.h"
#include <stdio.h>
#include "evk_uart.h"

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
#if defined(DEBUG) && DEBUG == true
static uint16_t stack_dump_format(uint32_t *stack, char *out_string, uint16_t string_size);
static uint16_t fault_register_format(char *out_string, uint16_t string_size);
static uint16_t memfault_format(char *out_string, uint16_t string_size);
static uint16_t busfault_format(char *out_string, uint16_t string_size);
static uint16_t usagefault_format(char *out_string, uint16_t string_size);
#endif /* DEBUG */

/* PUBLIC FUNCTIONS ***********************************************************/
void evk_debug_exception_dump(uint32_t *stack_frame)
{
    #if defined(DEBUG) && DEBUG == true
    char fault_string[FAULT_REGISTER_DUMP_STRING_SIZE];
    char stack_string[STACK_DUMP_STRING_SIZE];
    uint16_t out_size;

    out_size = fault_register_format(fault_string, FAULT_REGISTER_DUMP_STRING_SIZE);
    evk_uart_swd_write_blocking((uint8_t *)fault_string, out_size);
    out_size = stack_dump_format(stack_frame, stack_string, STACK_DUMP_STRING_SIZE);
    evk_uart_swd_write_blocking((uint8_t *)stack_string, out_size);
    #else
    (void)stack_frame;
    #endif
}

/* PRIVATE FUNCTIONS **********************************************************/
#if defined(DEBUG) && DEBUG == true
/** @brief Format the provided string with stack dump data.
 *
 *  @param stack        Stack pointer pointing to the top of the stack frame to fetch information from.
 *  @param out_string   Pre-allocated char array which will be formatted with stack dump data.
 *  @param string_size  Size of the pre-allocated out_string. Used to prevent buffer overrun in case of insufficient space.
 *
 *  @return Used length of the pre-allocated out_string containing formatted stack data.
 */
static uint16_t stack_dump_format(uint32_t *stack, char *out_string, uint16_t string_size)
{
    enum {r0, r1, r2, r3, r12, lr, pc, psr};
    uint16_t used_string_size = 0;
    const char *suggestion_title = "Here are some commands you could execute to find the source of the error:\n\r";
    const char *suggestion1 = "arm-none-eabi-objdump -S path_to_compiled_elf.elf | grep -B 5 -A 5 %8lx:\n\r";
    const char *suggestion2 = "arm-none-eabi-addr2line -a %8lx -e path_to_compiled_elf.elf\n\r";

    used_string_size += snprintf(
                            out_string,
                            string_size,
                            "Stack dump:\n\r"
                            "r0  = 0x%08lx\n\r"
                            "r1  = 0x%08lx\n\r"
                            "r2  = 0x%08lx\n\r"
                            "r3  = 0x%08lx\n\r"
                            "r12 = 0x%08lx\n\r"
                            "lr  = 0x%08lx\n\r"
                            "pc  = 0x%08lx\n\r"
                            "psr = 0x%08lx\n\r",
                            stack[r0],
                            stack[r1],
                            stack[r2],
                            stack[r3],
                            stack[r12],
                            stack[lr],
                            stack[pc],
                            stack[psr]
                        );

    used_string_size += snprintf(out_string + used_string_size, string_size - used_string_size, suggestion_title);
    used_string_size += snprintf(out_string + used_string_size, string_size - used_string_size, suggestion1, stack[pc]);
    used_string_size += snprintf(out_string + used_string_size, string_size - used_string_size, suggestion2, stack[pc]);

    return used_string_size;
}

/** @brief Format the provided string with fault registers data.
 *
 *  @param out_string   Pre-allocated char array which will be formatted with fault registers data.
 *  @param string_size  Size of the pre-allocated out_string. Used to prevent buffer overrun in case of insufficient space.
 *
 *  @return Used length of the pre-allocated out_string containing formatted fault registers data.
 */
static uint16_t fault_register_format(char *out_string, uint16_t string_size)
{
    uint16_t string_used_size = 0;

    string_used_size += snprintf(out_string, string_size, "Hardfault occured!\n\r");
    string_used_size += snprintf(out_string + string_used_size, string_size - string_used_size, "SCB->HFSR = 0x%08lx\n\r", SCB->HFSR);

    /* Check fault type */
    if (SCB->HFSR & SCB_HFSR_VECTTBL_Msk) {
        string_used_size += snprintf(out_string + string_used_size, string_size - string_used_size, "A BusFault occurred by reading the vector table for exception processing.\n\r");
    }
    if (SCB->HFSR & SCB_HFSR_FORCED_Msk) {
        /* Forced Hardfault means that it was triggered by a bus fault, a memory fault, or a usage fault */
        string_used_size += snprintf(out_string + string_used_size, string_size - string_used_size, "Exception was caused by escalation of a configurable fault (FORCED).\n\r");
        string_used_size += snprintf(out_string + string_used_size, string_size - string_used_size, "SCB->CFSR = 0x%08lx\n\r", SCB->CFSR);
        if (SCB->CFSR & SCB_CFSR_MEMFAULTSR_Msk) {
            string_used_size += memfault_format(out_string + string_used_size, string_size - string_used_size);
        }
        if (SCB->CFSR & SCB_CFSR_BUSFAULTSR_Msk) {
            string_used_size += busfault_format(out_string + string_used_size, string_size - string_used_size);
        }
        if (SCB->CFSR & SCB_CFSR_USGFAULTSR_Msk) {
            string_used_size += usagefault_format(out_string + string_used_size, string_size - string_used_size);
        }
    }

    return string_used_size;
}

/** @brief Format the provided string with specific memfault data.
 *
 *  @param out_string   Pre-allocated char array which will be formatted with memfault data.
 *  @param string_size  Size of the pre-allocated out_string. Used to prevent buffer overrun in case of insufficient space.
 *
 *  @return Used length of the pre-allocated out_string containing formatted memfault data.
 */
static uint16_t memfault_format(char *out_string, uint16_t string_size)
{
    uint32_t cfsr = SCB->CFSR;
    int string_used_size = 0;
    const char *section_title = "MemManage fault: ";
    const char *mlsperr = "Fault occurred during floating point lazy stack preservation (MLSPERR)\n\r";
    const char *mstkerr = "Fault occurred on stacking for exception entry (MSTKERR)\n\r";
    const char *munstkerr = "Fault occurred on unstacking on exception return (MUNSTKERR)\n\r";
    const char *daccviol = "Data access violation (DACCVIOL)\n\r";
    const char *iaccviol = "MPU or Execute Never (XN) default memory map access violation (IACCVIOL)\n\r";
    const char *fault_address = "Address which caused the fault (SCB->MMFAR): 0x%08lx\n\r";

    string_used_size += snprintf(out_string, string_size, section_title);

    if (cfsr & SCB_CFSR_MLSPERR_Msk) {
        string_used_size += snprintf(out_string + string_used_size, string_size - string_used_size, mlsperr);
    }
    if (cfsr & SCB_CFSR_MSTKERR_Msk) {
        string_used_size += snprintf(out_string + string_used_size, string_size - string_used_size, mstkerr);
    }
    if (cfsr & SCB_CFSR_MUNSTKERR_Msk) {
        string_used_size += snprintf(out_string + string_used_size, string_size - string_used_size, munstkerr);
    }
    if (cfsr & SCB_CFSR_DACCVIOL_Msk) {
        string_used_size += snprintf(out_string + string_used_size, string_size - string_used_size, daccviol);
    }
    if (cfsr & SCB_CFSR_IACCVIOL_Msk) {
        string_used_size += snprintf(out_string + string_used_size, string_size - string_used_size, iaccviol);
    }
    if (cfsr & SCB_CFSR_MMARVALID_Msk) {
        string_used_size += snprintf(out_string + string_used_size, string_size - string_used_size, fault_address, SCB->MMFAR);
    }

    return string_used_size;
}

/** @brief Format the provided string with specific busfault data.
 *
 *  @param out_string   Pre-allocated char array which will be formatted with busfault data.
 *  @param string_size  Size of the pre-allocated out_string. Used to prevent buffer overrun in case of insufficient space.
 *
 *  @return Used length of the pre-allocated out_string containing formatted busfault data.
 */
static uint16_t busfault_format(char *out_string, uint16_t string_size)
{
    uint32_t cfsr = SCB->CFSR;
    int string_used_size = 0;
    const char *section_title = "Bus fault: ";
    const char *lsperr = "Bus fault occurred during floating point lazy stack preservation (LSPERR)\n\r";
    const char *stkerr = "Bus fault occurred on stacking for exception entry (STKERR)\n\r";
    const char *unstkerr = "Bus fault occurred on unstacking on exception return (UNSTKERR)\n\r";
    const char *impreciseerr = "Return address is not related to fault, e.g. fault caused before (IMPRECISERR)\n\r";
    const char *preciseerr = "Return address instruction caused the fault (PRECISERR)\n\r";
    const char *ibuserr = "Fault on instruction fetch (IBUSERR)\n\r";
    const char *fault_address = "Address which caused the fault (SCB->BFAR): 0x%08lx\n\r";

    string_used_size += snprintf(out_string, string_size, section_title);

    if (cfsr & SCB_CFSR_LSPERR_Msk) {
        string_used_size += snprintf(out_string + string_used_size, string_size - string_used_size, lsperr);
    }
    if (cfsr & SCB_CFSR_STKERR_Msk) {
        string_used_size += snprintf(out_string + string_used_size, string_size - string_used_size, stkerr);
    }
    if (cfsr & SCB_CFSR_UNSTKERR_Msk) {
        string_used_size += snprintf(out_string + string_used_size, string_size - string_used_size, unstkerr);
    }
    if (cfsr & SCB_CFSR_IMPRECISERR_Msk) {
        string_used_size += snprintf(out_string + string_used_size, string_size - string_used_size, impreciseerr);
    }
    if (cfsr & SCB_CFSR_PRECISERR_Msk) {
        string_used_size += snprintf(out_string + string_used_size, string_size - string_used_size, preciseerr);
    }
    if (cfsr & SCB_CFSR_IBUSERR_Msk) {
        string_used_size += snprintf(out_string + string_used_size, string_size - string_used_size, ibuserr);
    }
    if (cfsr & SCB_CFSR_BFARVALID_Msk) {
        string_used_size += snprintf(out_string + string_used_size, string_size - string_used_size, fault_address, SCB->BFAR);
    }

    return string_used_size;
}

/** @brief Format the provided string with specific usagefault data.
 *
 *  @param out_string   Pre-allocated char array which will be formatted with usagefault data.
 *  @param string_size  Size of the pre-allocated out_string. Used to prevent buffer overrun in case of insufficient space.
 *
 *  @return Used length of the pre-allocated out_string containing formatted usagefault data.
 */
static uint16_t usagefault_format(char *out_string, uint16_t string_size)
{
    uint32_t cfsr = SCB->CFSR;
    int string_used_size = 0;
    const char *section_title = "Usage fault: ";
    const char *divbyzero = "SDIV or UDIV instruction executed with divisor 0 (DIVBYZERO)\n\r";
    const char *unaligned = "LDM, STM, LDRD, STRD on unaligned address executed, or single load or store executed when enabled to trap (UNALIGNED)\n\r";
    const char *nocp = "Access to unsupported (e.g. not available or not enabled) coprocessor (NOCP)\n\r";
    const char *invpc = "Illegal or invalid EXC_RETURN value load to PC (INVPC)\n\r";
    const char *invstate = "Execution in invalid state. E.g. Thumb bit not set in EPSR, or invalid IT state in EPSR (INVSTATE)\n\r";
    const char *undefinstr = "Execution of undefined instruction (UNDEFINSTR)\n\r";

    string_used_size += snprintf(out_string, string_size, section_title);

    if (cfsr & SCB_CFSR_DIVBYZERO_Msk) {
        string_used_size += snprintf(out_string + string_used_size, string_size - string_used_size, divbyzero);
    }
    if (cfsr & SCB_CFSR_UNALIGNED_Msk) {
        string_used_size += snprintf(out_string + string_used_size, string_size - string_used_size, unaligned);
    }
    if (cfsr & SCB_CFSR_NOCP_Msk) {
        string_used_size += snprintf(out_string + string_used_size, string_size - string_used_size, nocp);
    }
    if (cfsr & SCB_CFSR_INVPC_Msk) {
        string_used_size += snprintf(out_string + string_used_size, string_size - string_used_size, invpc);
    }
    if (cfsr & SCB_CFSR_INVSTATE_Msk) {
        string_used_size += snprintf(out_string + string_used_size, string_size - string_used_size, invstate);
    }
    if (cfsr & SCB_CFSR_UNDEFINSTR_Msk) {
        string_used_size += snprintf(out_string + string_used_size, string_size - string_used_size, undefinstr);
    }

    return string_used_size;
}
#endif /* DEBUG */
