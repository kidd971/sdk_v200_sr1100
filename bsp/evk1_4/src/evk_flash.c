/** @file  evk_flash.c
 *  @brief This module is composed of all functionalities related to the MCU's flash memory.
 *
 *  @copyright Copyright (C) 2020-2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "evk_flash.h"
#include "stm32g4xx_hal.h"
#include "stm32g4xx_hal_flash_ex.h"

/* PUBLIC FUNCTIONS ***********************************************************/
uint8_t evk_flash_prog(uint32_t address, const void *buffer, uint32_t size)
{
    HAL_StatusTypeDef status = 0;
    uint64_t *pdata = (uint64_t *)buffer;

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_SR_ERRORS);
    while (size > 0) {
        if (size >= sizeof(uint64_t)) {
            /* Write the data as 8-byte chunks */
            status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, *pdata);
            /* Next write address is 8 bytes further */
            address += sizeof(uint64_t);
            /* Decrement remaining number of bytes to write by 8 */
            size -= sizeof(uint64_t);
            /* Jump to next 8 bytes to write */
            pdata++;
        } else { /* less than 8 bytes to write */
            status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, *pdata);
            size = 0;
        }
    }
    HAL_FLASH_Lock();

    return (uint8_t)status;
}

uint8_t evk_flash_erase(uint32_t block_count, uint32_t block)
{
    FLASH_EraseInitTypeDef erase_init;
    uint32_t page_error;
    HAL_StatusTypeDef status;

    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.Banks     = FLASH_BANK_2;
    erase_init.Page      = (FLASH_PAGE_NB - block_count) + block;
    erase_init.NbPages   = 1;

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_SR_ERRORS);
    status = HAL_FLASHEx_Erase(&erase_init, &page_error);
    HAL_FLASH_Lock();

    return (uint8_t)status;
}
