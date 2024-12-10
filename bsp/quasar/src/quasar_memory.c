/** @file  quasar_memory.c
 *  @brief The memory module contains all functionalities related to the flash memory.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
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
static quasar_memory_error_t memory_verify_status(HAL_StatusTypeDef status);
static void memory_get_erase_info(uint32_t address, FLASH_EraseInitTypeDef *erase_info);

/* PUBLIC FUNCTIONS ***********************************************************/
void quasar_memory_read(uint32_t flash_address, const void *read_buffer, uint32_t buffer_size, quasar_memory_error_t *err)
{
    /* Reset error. */
    *err = QUASAR_MEMORY_ERROR_NO_ERROR;

    /* Null buffer check. */
    if (read_buffer == NULL) {
        *err = QUASAR_MEMORY_ERROR_ARGUMENT;
        return;
    }

    /* Does not check for buffer overflows when copying to destination (CWE-120). */
    memcpy((uint32_t *)read_buffer, (uint32_t *)flash_address, buffer_size); /* Flawfinder: ignore */
}

void quasar_memory_write(uint32_t flash_address, const void *write_buffer, uint32_t buffer_size, quasar_memory_error_t *err)
{
    HAL_StatusTypeDef status = 0;
    uint32_t *pdata  = (uint32_t *)write_buffer;
    uint32_t data_check[buffer_size / WORD_SIZE];
    uint32_t data_copy[buffer_size / WORD_SIZE];

    /* Reset error. */
    *err = QUASAR_MEMORY_ERROR_NO_ERROR;

    /* Null buffer check. */
    if (write_buffer == NULL) {
        *err = QUASAR_MEMORY_ERROR_ARGUMENT;
        return;
    }

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_SR_ERRORS);
    while (buffer_size > 0) {
        if (buffer_size >= QUAD_WORD_SIZE) {
            /* Write the data as 16-byte chunks. */
            status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, flash_address, (uint32_t)pdata);

            if (status != HAL_OK) {
                *err = QUASAR_MEMORY_ERROR_WRITE;
                HAL_FLASH_Lock();
                return;
            }

            /* Next write address is 16 bytes further. */
            flash_address += QUAD_WORD_SIZE;
            /* Decrement remaining number of bytes to write by 16. */
            buffer_size -= QUAD_WORD_SIZE;
            /* Jump to next 16 bytes to write. */
            pdata += (QUAD_WORD_SIZE / WORD_SIZE);
        } else {
            /* Less than 16 bytes to write. */
            status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, flash_address, (uint32_t)pdata);

            if (status != HAL_OK) {
                *err = QUASAR_MEMORY_ERROR_WRITE;
                HAL_FLASH_Lock();
                return;
            }

            buffer_size   = 0;
        }
    }
    HAL_FLASH_Lock();

    /* Verify write. */
    memcpy(data_copy, write_buffer, buffer_size);

    quasar_memory_read(flash_address, data_check, buffer_size, err);

    if (memcmp(data_check, data_copy, buffer_size) != 0) {
        *err = QUASAR_MEMORY_ERROR_WRITE;
    }
}

void quasar_memory_erase(uint32_t flash_address, quasar_memory_error_t *err)
{
    FLASH_EraseInitTypeDef erase_init;
    HAL_StatusTypeDef status;
    uint32_t page_error;

    memory_get_erase_info(flash_address, &erase_init);

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_SR_ERRORS);
    status = HAL_FLASHEx_Erase(&erase_init, &page_error);
    HAL_FLASH_Lock();

    *err = memory_verify_status(status);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Get the status from STM32 HAL and translate to error status.
 *
 *  @param[in] status  Status from STM32 HAL.
 *  @return The resulting quasar memory error status.
 */
static quasar_memory_error_t memory_verify_status(HAL_StatusTypeDef status)
{
    quasar_memory_error_t err;

    switch (status) {
    case HAL_OK:
        err = QUASAR_MEMORY_ERROR_NO_ERROR;
        break;
    case HAL_ERROR:
        err = QUASAR_MEMORY_ERROR;
        break;
    case HAL_BUSY:
        err = QUASAR_MEMORY_ERROR_BUSY;
        break;
    case HAL_TIMEOUT:
        err = QUASAR_MEMORY_ERROR_TIMEOUT;
        break;
    default:
        err = QUASAR_MEMORY_ERROR_UNKNOWN;
        break;
    }

    return err;
}

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
