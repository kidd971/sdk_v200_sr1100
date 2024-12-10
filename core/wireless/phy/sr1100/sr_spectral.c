/** @file  sr_spectral.c
 *  @brief Spectral configuration based on calibration value.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is confidential and proprietary.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sr_spectral.h"

/* CONSTANTS ******************************************************************/
#define TX_POWER_MAX         7
#define LNA_PEAK_TABLE_SIZE  16
#define DCRO_FREQ_RESOLUTION 1
#define VGA3GAIN_OPT         0x06
#define LNA_BIAS_OPT         0x08
#define TX_FREQ_OFFSET       2

/* PRIVATE GLOBALS ************************************************************/
/** @brief LNA peak table, from datasheet under register 0x11.
 */
static const uint16_t lna_peak_table[] = {
    9110 / DCRO_FREQ_RESOLUTION, 8720 / DCRO_FREQ_RESOLUTION, 8390 / DCRO_FREQ_RESOLUTION,
    8090 / DCRO_FREQ_RESOLUTION, 7850 / DCRO_FREQ_RESOLUTION, 7610 / DCRO_FREQ_RESOLUTION,
    7400 / DCRO_FREQ_RESOLUTION, 7190 / DCRO_FREQ_RESOLUTION, 6950 / DCRO_FREQ_RESOLUTION,
    6770 / DCRO_FREQ_RESOLUTION, 6620 / DCRO_FREQ_RESOLUTION, 6470 / DCRO_FREQ_RESOLUTION,
    6320 / DCRO_FREQ_RESOLUTION, 6200 / DCRO_FREQ_RESOLUTION, 6080 / DCRO_FREQ_RESOLUTION,
    5960 / DCRO_FREQ_RESOLUTION,
};

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void setup_pulse_selector(sr_reg_pattern_t *spectral, uint8_t pulse_pos, uint8_t pulse_cfg);
static void setup_pulse_cfg_width(sr_reg_pattern_t *reg_pattern, sr_spectral_config_num_t cfg_num, uint8_t pulse_width);
static void setup_channel_tx_pulse_cfg_freq(sr_reg_pattern_t *reg_pattern, sr_spectral_config_num_t cfg_num, uint8_t freq_dcro,
                                            bool freq_shift);
static void setup_channel_tx_power(sr_reg_pattern_t *reg_pattern, uint8_t tx_power);
static void setup_channel_rx_rf_filter_freq(sr_reg_pattern_t *reg_pattern, uint8_t freq_dcro);
static void setup_channel_rx_lna_freq(sr_reg_pattern_t *reg_pattern, uint8_t lna_freq_code);
static void setup_channel_vreftune(sr_reg_pattern_t *reg_pattern, uint8_t vreftune_nvm_value);
static void setup_channel_ireftune(sr_reg_pattern_t *reg_pattern, uint8_t ireftune_nvm_value);
static void setup_channel_dltune(sr_reg_pattern_t *reg_pattern, uint8_t ireftune_nvm_value);
static void validate_input(channel_cfg_t *spectral_cfg, sr_phy_error_t *error);
static bool assert_valid_tx_power(uint8_t tx_power);
static bool assert_valid_pulse_position(uint8_t pulse_position);
static bool assert_valid_pulse_cfg_number(uint8_t number_of_cfg);
static void default_spectral(rf_channel_t *spectral);

/* PUBLIC FUNCTIONS ***********************************************************/
sr_phy_error_t config_spectrum_advance(calib_vars_t *spectral_calib, channel_cfg_t *spectral_cfg, rf_channel_t *spectral)

{
    uint8_t current_position             = 0;
    uint8_t current_pulse_cfg            = 0;
    uint8_t current_number_of_cfg_needed = 0;
    uint8_t freq_dcro                    = 0;
    uint8_t lna_freq_code                = 0;
    bool cfg2_in_use                     = false;
    bool cfg3_in_use                     = false;
    sr_phy_error_t error                 = SR_SPECTRAL_ERROR_NONE;
    struct spectral_configuration {
        sr_spectral_config_num_t pulse_cfg_num;
        uint8_t pulse_width;
    } cfg_to_fill = {0};


    /* Assert good input */
    validate_input(spectral_cfg, &error);

    if (error != SR_SPECTRAL_ERROR_NONE) {
        return error;
    }

    /* Setup Integgain */
    spectral->integgain = spectral_cfg->integrators_gain;
    /* Initialize spectral configuration */
    default_spectral(spectral);

    uint8_t *pulse_width_table  = spectral_cfg->pulse_width_table;
    uint32_t target_center_freq = spectral_cfg->center_freq;

    if (spectral_cfg->freq_shift) {
        current_number_of_cfg_needed = MAX_PULSE_CFG;
    } else {
        current_number_of_cfg_needed = spectral_cfg->pulse_cfg_num;
    }

    current_position = spectral_cfg->start_pos;

    /* Setup channel pulse selector */
    for (uint8_t pulse_idx = 0; pulse_idx < spectral_cfg->pulse_count; pulse_idx++) {
        /* Since table index start at 0 and register value start at 1, increment pulse config by one */
        current_pulse_cfg = spectral_cfg->pulse_cfg_selector[pulse_idx] + 1;

        setup_pulse_selector(&spectral->reg_pattern, current_position, current_pulse_cfg);

        /* Pulse spacing of 0 need to be the next one and not the same */
        current_position += spectral_cfg->pulse_spacing + 1;
    }

    /* Setup frequency and pulse width for every channel pulse - TX configuration */
    freq_dcro = sr_find_matching_dcro(spectral_calib, target_center_freq + TX_FREQ_OFFSET, CALIBRATION_TX);
    for (uint8_t index_cfg = 0; index_cfg < current_number_of_cfg_needed; index_cfg++) {
        setup_channel_tx_pulse_cfg_freq(&spectral->reg_pattern, spectral_cfg->pulse_cfg_selector[index_cfg], freq_dcro, false);
        setup_pulse_cfg_width(&spectral->reg_pattern, spectral_cfg->pulse_cfg_selector[index_cfg], pulse_width_table[index_cfg]);
        if (spectral_cfg->pulse_cfg_selector[index_cfg] == SR_SPECTRAL_TX_CFG1) {
            cfg2_in_use = true;
            cfg_to_fill.pulse_cfg_num = SR_SPECTRAL_TX_CFG2;
            cfg_to_fill.pulse_width   = pulse_width_table[index_cfg];
        } else {
            cfg3_in_use = true;
            cfg_to_fill.pulse_cfg_num = SR_SPECTRAL_TX_CFG1;
            cfg_to_fill.pulse_width   = pulse_width_table[index_cfg];
        }
    }

    /* If one of both configuration is not use, fix ASIC BUG */
    if (!(cfg2_in_use && cfg3_in_use)) {
        setup_channel_tx_pulse_cfg_freq(&spectral->reg_pattern, cfg_to_fill.pulse_cfg_num, freq_dcro, false);
        setup_pulse_cfg_width(&spectral->reg_pattern, cfg_to_fill.pulse_cfg_num, cfg_to_fill.pulse_width);
    }

    setup_channel_tx_power(&spectral->reg_pattern, spectral_cfg->tx_gain);

    /* Setup RF filter and LNA frequencies - RX configuration */
    freq_dcro     = sr_find_matching_dcro(spectral_calib, target_center_freq, CALIBRATION_RX);
    lna_freq_code = sr_get_lna_peak(target_center_freq);
    setup_channel_rx_rf_filter_freq(&spectral->reg_pattern, freq_dcro);
    setup_channel_rx_lna_freq(&spectral->reg_pattern, lna_freq_code);
    setup_channel_vreftune(&spectral->reg_pattern, spectral_calib->vref_tune_offset);
    setup_channel_ireftune(&spectral->reg_pattern, spectral_calib->ireftune);
    setup_channel_dltune(&spectral->reg_pattern, spectral_calib->dl_tune);

    return error;
}

void config_spectrum(calib_vars_t *calib_vars, uint16_t frequency, tx_power_t tx_power, rf_channel_t *rf_channel)
{
    (void)calib_vars;
    (void)frequency;
    (void)tx_power;
    (void)rf_channel;
}

uint8_t sr_get_lna_peak(uint32_t target_freq)
{
    uint32_t upper_difference = 0;
    uint32_t lower_difference = 0;
    uint8_t lna_index         = 0;

    for (uint8_t i = 0; i < LNA_PEAK_TABLE_SIZE; i++) {
        if ((target_freq == lna_peak_table[i]) || (target_freq > lna_peak_table[0])) {
            /* freq is equal or greater than the first entry */
            lna_index = i;
            break;
        } else if (target_freq > lna_peak_table[i]) {
            upper_difference = lna_peak_table[i - 1] - target_freq;
            lower_difference = target_freq - lna_peak_table[i];
            if (upper_difference < lower_difference) {
                lna_index = i - 1;
            } else {
                lna_index = i;
            }
            break;
        } else {
            lna_index = LNA_PEAK_TABLE_SIZE - 1;
        }
    }

    return lna_index;
}

uint8_t sr_find_matching_dcro(calib_vars_t *spectral_calib, uint32_t target_freq, spectral_calib_power_mode_t calibration_mode)
{
    uint32_t upper_difference = 0;
    uint32_t lower_difference = 0;
    uint32_t *vcro_table      = NULL;
    uint8_t dcro_index        = 0;

    switch (calibration_mode) {
    case CALIBRATION_RX:
        vcro_table = spectral_calib->vcro_table_rx;
        break;
    case CALIBRATION_TX:
        vcro_table = spectral_calib->vcro_table_tx;
        break;
    }

    for (uint8_t i = 0; i < DCRO_MAX_COUNT; i++) {
        if ((target_freq == vcro_table[i]) || (target_freq > vcro_table[0])) {
            /* freq is equal or higher than the first entry */
            dcro_index = i;
            break;
        } else if (target_freq > vcro_table[i]) {
            upper_difference = vcro_table[i] - target_freq;
            lower_difference = target_freq - vcro_table[i - 1];
            if (upper_difference > lower_difference) {
                dcro_index = i;
            } else {
                dcro_index = i - 1;
            }
            break;
        } else {
            dcro_index = 0;
        }
    }

    return dcro_index;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Setup pulse position based on configuration.
 *
 *  @note Pulse selection is based on starting position,
 *        pulse spacing and pulse number.
 *        Given starting position = 2, spacing 1 and number 3,
 *        pulse number 2, 4, 6 will be used.
 *
 *  @param[in] spectral   Register instance to populate.
 *  @param[in] pulse_pos  Pulse position, from 0 to 9.
 *  @param[in] pulse_cfg  Pulse configuration number, CFG1 - CFG2 - CFG3.
 */
static void setup_pulse_selector(sr_reg_pattern_t *spectral, uint8_t pulse_pos, uint8_t pulse_cfg)
{
    switch (pulse_pos) {
    case 1:
        spectral->tx_pulse_pos |= SET_POS1PULSE(pulse_cfg);
        break;
    case 2:
        spectral->tx_pulse_pos |= SET_POS2PULSE(pulse_cfg);
        break;
    case 3:
        spectral->tx_pulse_pos |= SET_POS3PULSE(pulse_cfg);
        break;
    case 4:
        spectral->tx_pulse_pos |= SET_POS4PULSE(pulse_cfg);
        break;
    case 5:
        spectral->tx_pulse_pos |= SET_POS5PULSE(pulse_cfg);
        break;
    case 6:
        spectral->tx_pulse_pos |= SET_POS6PULSE(pulse_cfg);
        break;
    case 7:
        spectral->tx_pulse_pos |= SET_POS7PULSE(pulse_cfg);
        break;
    case 8:
        break;
    case 9:
        spectral->tx_pulse_pos |= SET_POS9PULSE(pulse_cfg);
        break;
    default:
        break;
    }
}

/** @brief Setup pulse width configuration.
 *
 *  @param[in] reg_pattern  Register instance to populate.
 *  @param[in] cfg_num      Pulse configuration number, CFG1 - CFG2 - CFG3.
 *  @param[in] pulse_width  Target pulse width, 3 bits value.
 */
static void setup_pulse_cfg_width(sr_reg_pattern_t *reg_pattern, sr_spectral_config_num_t cfg_num, uint8_t pulse_width)
{
    switch (cfg_num) {
    case SR_SPECTRAL_TX_CFG1:
        reg_pattern->cfg_widths_txpwr_randpulse |= SET_CFG2WIDTH(pulse_width);
        break;
    case SR_SPECTRAL_TX_CFG2:
        reg_pattern->cfg_widths_txpwr_randpulse |= SET_CFG3WIDTH(pulse_width);
        break;
    default:
        break;
    }
    /* Force cfg1width to the selected width */
    reg_pattern->cfg_widths_txpwr_randpulse |= SET_CFG1WIDTH(pulse_width);
}

/** @brief Setup band frequency from DCRO table index.
 *
 *  @param[in] reg_pattern  Register instance to populate.
 *  @param[in] cfg_num      Pulse configuration number, CFG1 - CFG2 - CFG3.
 *  @param[in] freq_dcro    DCRO table index, 0 to 64.
 *  @param[in] freq_shift   Frequency shift enable.
 */
static void setup_channel_tx_pulse_cfg_freq(sr_reg_pattern_t *reg_pattern, sr_spectral_config_num_t cfg_num, uint8_t freq_dcro,
                                            bool freq_shift)
{
    (void)freq_shift;

    switch (cfg_num) {
    case SR_SPECTRAL_TX_CFG2:
        reg_pattern->cfg2freq_cfg3freq |= SET_CFG3FREQ(freq_dcro);
        break;
    case SR_SPECTRAL_TX_CFG1:
        reg_pattern->cfg2freq_cfg3freq |= SET_CFG2FREQ(freq_dcro);
        break;
    default:
        break;
    }
}

/** @brief Setup TX power in register structure.
 *
 *  @param[in] reg_pattern  Register instance to populate.
 *  @param[in] tx_power     TX power, 2 bits value.
 */
static void setup_channel_tx_power(sr_reg_pattern_t *reg_pattern, uint8_t tx_power)
{
    reg_pattern->cfg_widths_txpwr_randpulse |= SET_TX_POWER(tx_power);
}

/** @brief Setup CFG1 frequency in register structure.
 *
 *  @param[in] reg_pattern  Register instance to populate.
 *  @param[in] freq_dcro    DCRO frequency index, from DCRO table.
 */
static void setup_channel_rx_rf_filter_freq(sr_reg_pattern_t *reg_pattern, uint8_t freq_dcro)
{
    reg_pattern->rxbandfre_cfg1freq |= SET_CFG1FREQ(freq_dcro) | SET_RXBANDFRE(freq_dcro);
}

/** @brief Setup the LNA frequency in register structure.
 *
 *  @param[in] reg_pattern    Register instance to populate.
 *  @param[in] lna_freq_code  LNA freq table index.
 */
static void setup_channel_rx_lna_freq(sr_reg_pattern_t *reg_pattern, uint8_t lna_freq_code)
{
    reg_pattern->if_baseband_gain_lna = (reg_pattern->if_baseband_gain_lna & ~BITS_LNA_FREQ) |
                                        SET_LNA_FREQ(lna_freq_code);
}

/** @brief Setup VREFtune value into the register structure.
 *
 *  @param[in] reg_pattern         Register instance to populate.
 *  @param[in] vreftune_nvm_value  VREFtune value, taken from the NVM.
 */
static void setup_channel_vreftune(sr_reg_pattern_t *reg_pattern, uint8_t vreftune_nvm_value)
{
    reg_pattern->v_i_time_refs |= SET_VREFTUNE(vreftune_nvm_value);
}

/** @brief Setup IREFtune value into the register structure.
 *
 *  @param[in] reg_pattern         Register instance to populate.
 *  @param[in] ireftune_nvm_value  IREFtune value, taken from the NVM.
 */
static void setup_channel_ireftune(sr_reg_pattern_t *reg_pattern, uint8_t ireftune_nvm_value)
{
    reg_pattern->v_i_time_refs |= SET_IREFTUNE(ireftune_nvm_value);
}

/** @brief Setup IREFtune value into the register structure.
 *
 *  @param[in] reg_pattern         Register instance to populate.
 *  @param[in] dl_tune_from_calib  DL tune value, taken from the calibration.
 */
static void setup_channel_dltune(sr_reg_pattern_t *reg_pattern, uint8_t dl_tune_from_calib)
{
    reg_pattern->v_i_time_refs |= SET_DLTUNING(dl_tune_from_calib);
}

/** @brief Validate user input before configuring spectrum.
 *
 *  @param[in] spectral_cfg  Input configuration.
 *  @param[in] error         Error.
 */
static void validate_input(channel_cfg_t *spectral_cfg, sr_phy_error_t *error)
{
    uint8_t pulse_pos;

    pulse_pos = 0;
    /* Validate pulse cfg number */
    if (!assert_valid_pulse_cfg_number(spectral_cfg->pulse_cfg_num)) {
        *error = SR_SPECTRAL_ERROR_INVALID_PULSE_CFG;
    }
    /* Validate TX power */
    if (!assert_valid_tx_power(spectral_cfg->tx_gain)) {
        *error = SR_SPECTRAL_ERROR_INVALID_TX_POWER;
    }
    /* Validate spacing input */
    for (uint8_t pulse_idx = 0; pulse_idx < spectral_cfg->pulse_count; pulse_idx++) {
        pulse_pos += spectral_cfg->pulse_spacing + 1;
        if (!assert_valid_pulse_position(pulse_pos)) {
            *error = SR_SPECTRAL_ERROR_INVALID_SPACING;
        }
    }
}

/** @brief Assert TX power value.
 *
 *  @param[in] tx_power  Input TX power
 *  @retval true  TX power is valid.
 *  @retval false TX power is unsupported.
 */
static bool assert_valid_tx_power(uint8_t tx_power)
{
    if (tx_power <= TX_POWER_MAX) {
        return true;
    }
    return false;
}

/** @brief Assert valid pulse position.
 *
 *  @param[in] pulse_position Current pulse position
 *  @retval true  Pulse position is within 0 and 9.
 *  @retval false Pulse position is greater than 9.
 */
static bool assert_valid_pulse_position(uint8_t pulse_position)
{
    if (pulse_position <= MAX_NUMBER_OF_PULSE_POS) {
        return true;
    }
    return false;
}

/** @brief Assert number of pulse configuration.
 *
 *  @note Register only have 3 possible configuration.
 *
 *  @param[in] number_of_cfg  Band number of pulse configuration.
 *  @retval true  Pulse configuration number is lower or equal to 3
 *  @retval false Pulse configuration is greater than the number of available one.
 */
static bool assert_valid_pulse_cfg_number(uint8_t number_of_cfg)
{
    if (number_of_cfg <= MAX_PULSE_CFG) {
        return true;
    }
    return false;
}

/** @brief Initialize register structure to default value.
 *
 *  @param[in] spectral  Register instance to populate.
 */
static void default_spectral(rf_channel_t *spectral)
{
    spectral->reg_pattern.v_i_time_refs = 0x0000;
    spectral->reg_pattern.if_baseband_gain_lna =
        (REG16_IF_BASEBAND_GAIN_LNA_DEFAULT & ~BITS_INTEGGAIN & ~BITS_VGA3GAIN & ~BITS_LNA_BIAS) |
        SET_INTEGGAIN(spectral->integgain) | SET_VGA3GAIN(VGA3GAIN_OPT) |
        SET_LNA_BIAS(LNA_BIAS_OPT);
    spectral->reg_pattern.rxbandfre_cfg1freq         = 0x0000;
    spectral->reg_pattern.cfg2freq_cfg3freq          = 0x0000;
    spectral->reg_pattern.cfg_widths_txpwr_randpulse = RANDPULS_DEFAULT;
    spectral->reg_pattern.tx_pulse_pos               = 0x0000;
}
