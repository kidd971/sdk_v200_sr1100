/** @file  sr_spectral.h
 *  @brief sr_spectral brief
 *
 *  sr_spectral description
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is confidential and proprietary.
 *  @author    SPARK FW Team.
 */
#ifndef SR_SPECTRAL_H_
#define SR_SPECTRAL_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "sr_calib.h"
#include "sr_def.h"
#include "wps_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
#define MAX_NUMBER_OF_PULSE_POS 9
#define MAX_PULSE_CFG           3

/* TYPES **********************************************************************/
/** @brief Pulse width cfg enumeration.
 */
typedef enum sr_spectral_pulse_width {
    /*! Pulse width of 0.9ns */
    PW_0_9_NS = 0,
    /*! Pulse width of 1.08ns */
    PW_1_0_8_NS,
    /*! Pulse width of 1.15ns */
    PW_1_1_5_NS,
    /*! Pulse width of 1.3ns */
    PW_1_3_NS,
    /*! Pulse width of 1.5ns */
    PW_1_5_NS,
    /*! Pulse width of 1.67ns */
    PW_1_6_7_NS,
    /*! Pulse width of 1.87ns */
    PW_1_8_7_NS,
    /*! Pulse width of 2.05ns */
    PW_2_0_5_NS,
} sr_spectral_pulse_width_t;

/** @brief TX pulse configuration selector.
 *
 *  @note CFG1 Should only be used when auto-reply
 *        is disable. This is because CFG1 is needed
 *        for RX auto reply.
 *  @note CFG1 and CFG2 are used here to block user
 *        from accessing the CFG1 in the radio, since
 *        its use for the RX.
 */
typedef enum sr_spectral_config_num {
    /*! First available CFG in 0x12 - 0x17 for TX */
    SR_SPECTRAL_TX_CFG1 = 1,
    /*! Second available CFG in 0x12 - 0x17 for TX */
    SR_SPECTRAL_TX_CFG2,
} sr_spectral_config_num_t;

/** @brief Pulse pattern and band frequency configuration.
 */
typedef struct channel_cfg_t {
    /*! Number of pulse for current band. */
    uint32_t pulse_count;
    /*! TX power for current band, 2bits value. */
    uint32_t tx_gain;
    /*! Number of empty pulse between each pulse, 0 mean every pulse is use, 1 mean every other pulse is used. */
    uint32_t pulse_spacing;
    /*! First active pulse, from 0 to 9. */
    uint32_t start_pos;
    /*! Band target frequency, will be adjusted based on calibration. */
    uint32_t center_freq;
    /*! Every pulse configuration. Choice available : CFG1 or 2
     *   This table do not count the pulse spacing, so if the cfg has 4 pulses,
     *   only the first four index should be populate with the CFG1 or CFG2.
     *   Right now, since freq shift is not implemented, only CFG1 is working.
     */
    sr_spectral_config_num_t pulse_cfg_selector[MAX_NUMBER_OF_PULSE_POS];
    /*! Table containing the pulse width for every pulse. Same restriction as
     *   the pulse_cfg_selector table. For a cfg of 3 pulse of pulse width 2,
     *   the user need to provided : uint8_t pulse_width_table[3] = {2, 2, 2};
     */
    uint8_t *pulse_width_table;
    /*! Number of pulse configuration. Only 1 is supported. */
    uint8_t pulse_cfg_num;
    /*! Integgain register value, from 0 to 3 */
    uint8_t integrators_gain;
    /*! Band frequency shift feature. Not yet implemented. */
    bool freq_shift;
} channel_cfg_t;

/** @brief SR1120 0x10 - 0x17 register pattern.
 */
typedef struct sr_reg_pattern {
    /*! 0x02 - REG16_V_I_TIME_REFS */
    uint16_t v_i_time_refs;
    /*! 0x07 - REG16_IF_BASEBAND_GAIN_LNA */
    uint16_t if_baseband_gain_lna;
    /*! 0x08 - REG16_RXBANDFRE_CFG1FREQ */
    uint16_t rxbandfre_cfg1freq;
    /*! 0x09 - REG16_CFG2FREQ_CFG3FREQ */
    uint16_t cfg2freq_cfg3freq;
    /*! 0x0A - REG16_CFG_WIDTHS_TXPWR_RANDPULSE */
    uint16_t cfg_widths_txpwr_randpulse;
    /*! 0x0B - REG16_TX_PULSE_POS */
    uint16_t tx_pulse_pos;
} sr_reg_pattern_t;

/** @brief Wrapper for the application RF channel structure.
 */
typedef struct sr_spectral {
    /*! Register 0x12 to 0x17 raw value */
    sr_reg_pattern_t reg_pattern;
    /*! Receiver integrators gain */
    uint8_t integgain;
} rf_channel_t;

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Configure the pulse pattern, band frequency and LNA frequency.
 *
 *  @note This will only setup one given band.
 *
 *  @note Depending on the giving input, this will populate the rf_channel_t
 *        structure to be sent directly to register 0x10 to 0x17 of the
 *        SR1120.
 *
 *  @param[in] spectral_calib  Calibration variable.
 *  @param[in] spectral_cfg    Band configuration table.
 *  @param[in] spectral        Register instance to populate.
 *  @return Error during spectrum configuration.
 */
sr_phy_error_t config_spectrum_advance(calib_vars_t *spectral_calib, channel_cfg_t *spectral_cfg, rf_channel_t *spectral);

/** @brief Configuration of spectrum based on tx_power preset.
 *
 *  @note Not yet implemented for SR1120
 *
 *  @param[in] spectral_calib  Calibration variable.
 *  @param[in] frequency       Target frequency,
 *  @param[in] tx_power        TX power preset.
 *  @param[in] rf_channel      Register instance to populate.
 */
void config_spectrum(calib_vars_t *spectral_calib, uint16_t frequency, tx_power_t tx_power, rf_channel_t *rf_channel);

/** @brief Get matching LNA frequency based on user target frequency.
 *
 *  @param[in] target_freq  Target frequency, in MHz
 *  @return Index in LNA peak table.
 */
uint8_t sr_get_lna_peak(uint32_t target_freq);

/** @brief Get matching DCRO frequency based on input frequency
 *
 *  @note This function try to find the closest frequency
 *        in the calibration table from the given one.
 *
 *  @param[in] spectral_calib    Calibration variable.
 *  @param[in] target_freq       Target user frequency.
 *  @param[in] calibration_mode  RX or TX calibration.
 *  @return Index of the DCRO table.
 */
uint8_t sr_find_matching_dcro(calib_vars_t *spectral_calib, uint32_t target_freq, spectral_calib_power_mode_t calibration_mode);

#ifdef __cplusplus
}
#endif
#endif /* SR_SPECTRAL_H_ */
