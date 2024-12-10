/** @file  sr_pwr_up.h
 *  @brief SR1100 power up sequence.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is confidential and proprietary.
 *  @author    SPARK FW Team.
 */
#ifndef SR_PWR_UP_H_
#define SR_PWR_UP_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "sr_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Power up radio.
 *
 *  @param[in] radio  Radio's instance.
 *  @param[in] reset  Reset_flag.
 *  @param[in] err    Pointer to the error code.
 */
static inline void sr_pwr_up(radio_t *radio, bool reset, sr_phy_error_t *err)
{
    /* 1 second delay after board power-up to allow radio crystal stabilization. */
    while (sr_util_get_system_time_ms() < POWER_UP_TIME);

    if (reset) {
        sr_access_reset_reset_pin(radio->radio_id);
        sr_utils_wait_delay(10);
        sr_access_set_reset_pin(radio->radio_id);
        sr_utils_wait_delay(10);
    }

    sr_access_write_reg16(radio->radio_id, REG16_HARDDISABLES_IOCONFIG,
                          radio->std_spi | radio->outimped | CHIP_RATE_20_48_MHZ | radio->irq_polarity |
                              radio->clock_source.pll_clk_source | radio->clock_source.xtal_clk_source);
    sr_access_write_reg16(radio->radio_id, REG16_PREAMB_DEBUG, REG16_PREAMB_DEBUG_OPT | SET_SUMRXADC(radio->sumrxadc));

    uint16_t crc_30_16_value = sr_access_read_reg16(radio->radio_id, REG16_CRC_30_16);

    if (crc_30_16_value != REG16_CRC_30_16_DEFAULT) {
        *err = PHY_MODEL_NOT_FOUND;
    }
}

#ifdef __cplusplus
}
#endif

#endif /* SR_PWR_UP_H_ */
