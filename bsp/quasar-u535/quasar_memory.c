/** @file  quasar_memory.c
 *  @brief The memory module contains all functionalities related to the flash memory.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_memory.h"
#include <string.h>
#include "stm32u5xx_hal.h"
#include "stm32u5xx_hal_flash_ex.h"

/* CONSTANTS ******************************************************************/
#define QUAD_WORD_SIZE (4 * WORD_SIZE)
#define WORD_SIZE      4

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void memory_get_erase_info(uint32_t address, FLASH_EraseInitTypeDef *erase_info);
static quasar_memory_error_t memory_verify_status(HAL_StatusTypeDef status);

/* PUBLIC FUNCTIONS ***********************************************************/
quasar_memory_error_t quasar_memory_read(uint32_t flash_address, const void *read_buffer, uint32_t buffer_size)
{
    /* Null buffer check. */
    if (read_buffer == NULL) {
        return QUASAR_MEM_ERR_ARGUMENT;
    }

    /* Does not check for buffer overflows when copying to destination (CWE-120). */
    memcpy((uint32_t *)read_buffer, (uint32_t *)flash_address, buffer_size); /* Flawfinder: ignore */

    return QUASAR_MEM_ERR_NONE;
}

quasar_memory_error_t quasar_memory_write(uint32_t flash_address, const void *write_buffer, uint32_t buffer_size)
{
    HAL_StatusTypeDef status = 0;
    uint32_t *pdata = (uint32_t *)write_buffer;

    /* Null buffer check. */
    if (write_buffer == NULL) {
        return QUASAR_MEM_ERR_ARGUMENT;
    }

    uint16_t remaining_size = buffer_size;
    uint32_t current_address = flash_address;

    status = HAL_FLASH_Unlock();
    if (status != HAL_OK) {
        return memory_verify_status(status);
    }

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_SR_ERRORS);
    while (remaining_size > 0) {
        if (remaining_size >= QUAD_WORD_SIZE) {
            /* Write the data as 16-byte chunks. */
            status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, current_address, (uint32_t)pdata);

            if (status != HAL_OK) {
                HAL_FLASH_Lock();
                return memory_verify_status(status);
            }

            /* Next write address is 16 bytes further. */
            current_address += QUAD_WORD_SIZE;
            /* Decrement remaining number of bytes to write by 16. */
            remaining_size -= QUAD_WORD_SIZE;
            /* Jump to next 16 bytes to write. */
            pdata += (QUAD_WORD_SIZE / WORD_SIZE);
        } else {
            /* Less than 16 bytes to write. */
            status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, current_address, (uint32_t)pdata);

            if (status != HAL_OK) {
                HAL_FLASH_Lock();
                return memory_verify_status(status);
            }

            remaining_size = 0;
        }
    }
    status = HAL_FLASH_Lock();
    if (status != HAL_OK) {
        return memory_verify_status(status);
    }

    return QUASAR_MEM_ERR_NONE;
}

quasar_memory_error_t quasar_memory_erase(uint32_t address)
{
    FLASH_EraseInitTypeDef erase_init = {0};
    HAL_StatusTypeDef status = HAL_OK;
    uint32_t page_error = 0;

    memory_get_erase_info(address, &erase_init);

    status = HAL_FLASH_Unlock();
    if (status != HAL_OK) {
        return memory_verify_status(status);
    }

    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_SR_ERRORS);
    status = HAL_FLASHEx_Erase(&erase_init, &page_error);
    if (status != HAL_OK) {
        /* Best effort lock. */
        HAL_FLASH_Lock();
        return memory_verify_status(status);
    }

    status = HAL_FLASH_Lock();
    if (status != HAL_OK) {
        return memory_verify_status(status);
    }

    return QUASAR_MEM_ERR_NONE;
}

quasar_memory_error_t quasar_memory_invalidate_cache(void)
{
    HAL_StatusTypeDef status = HAL_ICACHE_Invalidate();

    if (status != HAL_OK) {
        return memory_verify_status(status);
    }

    return QUASAR_MEM_ERR_NONE;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Get erase info from address.
 *
 *  @param[in] address     Flash address.
 *  @param[in] erase_info  Fash erase information.
 */
static void memory_get_erase_info(uint32_t address, FLASH_EraseInitTypeDef *erase_info)
{
    /* Only use 1 page for user data. */
    erase_info->NbPages = 1;
    erase_info->TypeErase = FLASH_TYPEERASE_PAGES;

    if (address < (FLASH_BASE + FLASH_BANK_SIZE)) {
        erase_info->Banks = FLASH_BANK_1;
        erase_info->Page = (address - FLASH_BASE) / FLASH_PAGE_SIZE;
    } else {
        erase_info->Banks = FLASH_BANK_2;
        erase_info->Page = (address - (FLASH_BASE + FLASH_BANK_SIZE)) / FLASH_PAGE_SIZE;
    }
}

/** @brief Get the status from STM32 HAL and translate to error status.
 *
 *  @param[in] status  Status from STM32 HAL.
 *  @return The resulting quasar memory error status.
 */
static quasar_memory_error_t memory_verify_status(HAL_StatusTypeDef status)
{
    quasar_memory_error_t err = QUASAR_MEM_ERR_NONE;

    switch (status) {
    case HAL_OK:
        err = QUASAR_MEM_ERR_NONE;
        break;
    case HAL_ERROR:
        err = QUASAR_MEM_ERR;
        break;
    case HAL_BUSY:
        err = QUASAR_MEM_ERR_BUSY;
        break;
    case HAL_TIMEOUT:
        err = QUASAR_MEM_ERR_TIMEOUT;
        break;
    default:
        err = QUASAR_MEM_ERR_UNKNOWN;
        break;
    }

    return err;
}
