/** @file  sr_calib.h
 *  @brief sr_calib brief
 *
 *  sr1100_calib description
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is confidential and proprietary.
 *  @author    SPARK FW Team.
 */
#ifndef SR1100_CALIB_H_
#define SR1100_CALIB_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "sr_def.h"
#include "sr_nvm.h"
#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Calibration mode enum.
 */
typedef enum spectral_calib_power_mode {
    /*! Put radio in RX mode for RX calibration */
    CALIBRATION_RX = 0,
    /*! Put radio in DLL mode for TX calibration */
    CALIBRATION_TX
} spectral_calib_power_mode_t;

/** @brief Calibration variable for spectral configuration.
 */
typedef struct spectral_calib_vars {
    /*! Chip ID of the transceiver */
    uint64_t chip_id;
    /*! Resistance tuning value to calibrate the PLL and band gap*/
    uint8_t resistune;
    /*! Vref tune offset power tuning */
    int8_t vref_tune_offset;
    /*! Resistance tuning value to calibrate the PLL and band gap*/
    uint8_t ireftune;
    /*! DL tune value after calibration */
    uint8_t dl_tune;
    /*! Internal */
    /*! VCRO RX frequency, in MHz */
    uint32_t vcro_table_rx[DCRO_MAX_COUNT];
    /*! VCRO TX frequency, in MHz */
    uint32_t vcro_table_tx[DCRO_MAX_COUNT];
} calib_vars_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/

/** @brief Calibrate the radio.
 *
 *  @note Calibration variables should already be allocated
 *        before calling this function.
 *
 *  @param[in] radio           Radio's instance.
 *  @param[in] nvm             NVM instance, not yet implemented.
 *  @param[in] spectral_calib  Output calibration values for spectral module.
 */
void sr_calibrate(radio_t *radio, calib_vars_t *spectral_calib, nvm_t *nvm);

/** @brief Tune delay line in RX mode.
 *
 *  @param[in]  radio        Radio's instance.
 *  @param[out] dl_tune_out  DL tune value after calibration.
 *  @retval false  Tuning of delay line failed
 *  @retval true   Delay line tuning success
 */
bool sr_calib_dl_tune_rx(radio_t *radio, uint8_t *dl_tune_out);

/** @brief Tune delay line in TX mode.
 *
 *  @param[in]  radio        Radio's instance.
 *  @param[out] dl_tune_out  DL tune value after calibration.
 *  @retval false  Tuning of delay line failed
 *  @retval true   Delay line tuning success
 */
bool sr_calib_dl_tune_tx(radio_t *radio, uint8_t *dl_tune_out);

/** @brief Populate VCRO TX table.
 *
 *  @param[in] radio  Radio's instance.
 *  @param[in] spectral_calib
 *  @retval false  Failed to populate VCRO table TX.
 *  @retval true   VCRO table properly fill.
 */
bool sr_calib_get_vcro_codes_tx(radio_t *radio, calib_vars_t *spectral_calib);

/** @brief Populate VCRO RX table.
 *
 *  @param[in] radio  Radio's instance.
 *  @param[in] spectral_calib
 *  @retval false  Failed to populate VCRO table TX.
 *  @retval true   VCRO table properly fill.
 */
bool sr_calib_get_vcro_codes_rx(radio_t *radio, calib_vars_t *spectral_calib);

#ifdef __cplusplus
}
#endif
#endif /* SR1100_CALIB_H_ */
