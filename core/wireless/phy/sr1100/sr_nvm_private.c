/** @file sr1000_nvm_private.c
 *   @brief SR1000 non-volatile memory private module.
 *
 *   Functions related to writing the NVM and to its protocol.
 *
 *  @copyright Copyright (C) 2020 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is confidential and proprietary.
 *   @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sr_nvm_private.h"
#include "sr_access.h"
#include "sr_reg.h"

/* CONSTANTS ******************************************************************/
#define NVM_POST_WRITE_DELAY_MS 150

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void sr_nvm_set_byte(uint8_t radio_id, nvm_vdd_hal_t *vdd, uint8_t addr, uint8_t byte);

/* PUBLIC FUNCTIONS ***********************************************************/
void sr_nvm_write(radio_t *radio, nvm_vdd_hal_t *vdd, uint8_t *buf, uint8_t addr_start, uint8_t addr_end)
{
    uint8_t idx          = 0;
    uint8_t addr_current = addr_start;
    uint8_t read_status;

    /* Wake up the radio */
    sr_access_write_reg8(radio->radio_id, REG8_ACTIONS, 0x00);
    sr_access_write_reg16(radio->radio_id, REG16_IF_BASEBAND_GAIN_LNA, REG16_IF_BASEBAND_GAIN_LNA_DEFAULT);

    do {
        read_status = sr_access_read_reg8(radio->radio_id, REG8_POWER_STATE);
    } while (!GET_AWAKE(read_status));

    sr_nvm_power_up(radio);

    while (addr_current <= addr_end) {
        sr_nvm_set_byte(radio->radio_id, vdd, addr_current++, buf[idx++]);
    }
    sr_nvm_power_down(radio);

    /* Delay to ensure subsequent read works. Value found by experiment on V8B dies */
    sr_utils_wait_delay(NVM_POST_WRITE_DELAY_MS);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Burn a byte into a NVM location. Assumes that the radio and NVM are powered up.
 *
 *  @param[in] radio_id  Radio number.
 *  @param[in] vdd       VDD control HAL instance.
 *  @param[in] addr      Address of the memory location.
 *  @param[in] byte      Byte value.
 *  @return None.
 */
static void sr_nvm_set_byte(uint8_t radio_id, nvm_vdd_hal_t *vdd, uint8_t addr, uint8_t byte)
{
    uint8_t bit_array[16];
    uint8_t dummy_rx[16];
    int8_t array_index = 0;

    /* Fill up array with bit/addr pairs for each 1 bit */
    for (uint8_t i = 0; i <= NVM_LAST_BIT_POS; i++) {
        if (byte & BIT(i)) {
            bit_array[array_index++] = ((i << 1) | 0x20);
            bit_array[array_index++] = addr;
        }
    }
    /* Write the bit array to the NVM */
    if (array_index > 0) {
        array_index -= 2;
        vdd->enable_vdd();
        while (array_index >= 0) {
            sr_access_spi_transfer_blocking(radio_id, &bit_array[array_index], dummy_rx, 2);
            array_index -= 2;
        }
        vdd->disable_vdd();
    }
}
