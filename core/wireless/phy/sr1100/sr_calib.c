/** @file  sr_calib.c
 *  @brief sr_calib brief
 *
 *  sr_calib description
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is confidential and proprietary.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sr_calib.h"
#include "sr_utils.h"

/* CONSTANTS ******************************************************************/
#define DL_TUNE_VALUE_COUNT  32
#define VCRO_AVERAGING_COUNT 8
#define MSB_CODE_FREQ        256

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void put_radio_in_rx_power_state(radio_t *radio);
static void put_radio_in_dll_power_state(radio_t *radio);
static bool dl_tune(radio_t *radio, uint8_t *dl_tune_out);
static bool get_vcro_codes(radio_t *radio, uint32_t *target_vcro_table);

/* PUBLIC FUNCTIONS ***********************************************************/
void sr_calibrate(radio_t *radio, calib_vars_t *spectral_calib, nvm_t *nvm)
{
    chip_rate_cfg_t calibration_chip_rate = radio->chip_rate;

    spectral_calib->chip_id          = sr_nvm_get_serial_number_chip_id(nvm);
    spectral_calib->resistune        = sr_nvm_get_resistune(nvm);
    spectral_calib->vref_tune_offset = sr_nvm_get_vref_adjust_vref_tune_offset(nvm);
    spectral_calib->ireftune         = sr_nvm_get_ireftune(nvm);
    radio->vref_tune                 = spectral_calib->vref_tune_offset;
    radio->iref_tune                 = spectral_calib->ireftune;

    sr_access_write_reg16(radio->radio_id, REG16_V_I_TIME_REFS,
                          SET_VREFTUNE(radio->vref_tune) | SET_IREFTUNE(radio->iref_tune));

    /* Voltage settling time of minimum 100us */
    sr_utils_wait_delay(2);

    /* Calibrate using 20.48MHz when using 27.3MHz*/
    if (calibration_chip_rate == CHIP_RATE_27_30_MHZ) {
        calibration_chip_rate = CHIP_RATE_20_48_MHZ;
    }
    /* Setup CHIP rate and Clock source for calibration */
    sr_access_write_reg16(radio->radio_id, REG16_HARDDISABLES_IOCONFIG,
                          radio->std_spi | radio->outimped | radio->chip_rate | radio->irq_polarity |
                              calibration_chip_rate | radio->clock_source.pll_clk_source |
                              radio->clock_source.xtal_clk_source);

    /* DL tune for RX/TX */
    sr_calib_dl_tune_tx(radio, &spectral_calib->dl_tune);
    sr_calib_get_vcro_codes_tx(radio, spectral_calib);
    sr_calib_dl_tune_rx(radio, &spectral_calib->dl_tune);
    sr_calib_get_vcro_codes_rx(radio, spectral_calib);

    sr_access_write_reg16(radio->radio_id, REG16_V_I_TIME_REFS,
                          SET_VREFTUNE(radio->vref_tune) | SET_IREFTUNE(radio->iref_tune) |
                              SET_DLTUNING(spectral_calib->dl_tune));

    /* Resetup CHIP rate if 27.3MHz is chosen */
    if (radio->chip_rate == CHIP_RATE_27_30_MHZ) {
        sr_access_write_reg16(radio->radio_id, REG16_HARDDISABLES_IOCONFIG,
                              radio->std_spi | radio->outimped | radio->chip_rate | radio->irq_polarity |
                                  radio->clock_source.pll_clk_source | radio->clock_source.xtal_clk_source);
    }
}

bool sr_calib_dl_tune_rx(radio_t *radio, uint8_t *dl_tune_out)
{
    put_radio_in_rx_power_state(radio);
    return dl_tune(radio, dl_tune_out);
}

bool sr_calib_dl_tune_tx(radio_t *radio, uint8_t *dl_tune_out)
{
    put_radio_in_dll_power_state(radio);
    return dl_tune(radio, dl_tune_out);
}

bool sr_calib_get_vcro_codes_tx(radio_t *radio, calib_vars_t *spectral_calib)
{
    put_radio_in_dll_power_state(radio);
    return get_vcro_codes(radio, spectral_calib->vcro_table_tx);
}

bool sr_calib_get_vcro_codes_rx(radio_t *radio, calib_vars_t *spectral_calib)
{
    put_radio_in_rx_power_state(radio);
    return get_vcro_codes(radio, spectral_calib->vcro_table_rx);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Delay line tuning.
 *
 *  @note This will increment the value of DLTUNING in
 *        register 0x0C until the flag DL_LAGS in the same
 *        register is set.
 *
 *  @param[in]  radio        Radio's instance.
 *  @param[out] dl_tune_out  DL tune value after calibration.
 *  @retval false  Delay line failed.
 *  @retval true   Delay line success.
 */
static bool dl_tune(radio_t *radio, uint8_t *dl_tune_out)
{
    uint16_t dl_tune;
    uint8_t i = 0;

    for (i = 0; i < DL_TUNE_VALUE_COUNT; i++) {
        sr_access_write_reg16(radio->radio_id, REG16_V_I_TIME_REFS,
                              SET_VREFTUNE(radio->vref_tune) | SET_IREFTUNE(radio->iref_tune) | SET_DLTUNING(i));
        /*
         * Stop tuning when the delay line starts
         * to lag the symbol rate in frequency (slower).
         */
        dl_tune = sr_access_read_reg16(radio->radio_id, REG16_V_I_TIME_REFS);
        if (!GET_DL_LAGS(dl_tune)) {
            break;
        }
    }

    if (i == DL_TUNE_VALUE_COUNT) {
        return false;
    }

    *dl_tune_out = GET_DLTUNING(dl_tune);

    return true;
}

/** @brief Fill the VCRO table.
 *
 *  @param[in] radio              Radio's instance.
 *  @param[in] target_vcro_table  Table to populate
 *  @retval false  Failed to populate VCRO table.
 *  @retval true   VCRO table properly filled.
 */
static bool get_vcro_codes(radio_t *radio, uint32_t *target_vcro_table)
{
    uint8_t vcro;

    for (uint8_t dcro_code = 0; dcro_code < DCRO_MAX_COUNT; dcro_code++) {
        /* Initialize VCRO table */
        uint32_t vcro_temp = 0;

        for (uint8_t vcro_average = 0; vcro_average < VCRO_AVERAGING_COUNT; vcro_average++) {
            sr_access_write_reg8(radio->radio_id, REG8_DCRO_CONFIG, dcro_code);
            vcro = sr_access_read_reg8(radio->radio_id, REG8_DCRO_CONFIG);

            vcro_temp += vcro + MSB_CODE_FREQ;
        }

        vcro_temp            = (vcro_temp / VCRO_AVERAGING_COUNT);
        *target_vcro_table++ = ((vcro_temp * 2048) / 100);
    }
    return true;
}

/** @brief Put the radio in RX static power state.
 *
 *  @note This consist of :
 *          - Reset main commands   (0x01)
 *          - Reset timer/sleep cfg (0x04 - 0x05)
 *          - Disabling the buffer load interrupt (0x26)
 *          - Default Debug control (0x27)
 *          - Default main feature with RADIODIR to 1 (0x28)
 *          - Set integgain to 3    (0x0C)
 *
 *  @param[in] radio  SR hardware abstraction layer instance.
 */
static void put_radio_in_rx_power_state(radio_t *radio)
{
    uint8_t pwr_status;

    sr_access_write_reg16(radio->radio_id, REG16_FRAMEPROC_PHASEDATA, RX_MODE);
    sr_access_write_reg16(radio->radio_id, REG16_TIMERCFG_SLEEPCFG, 0x00);
    sr_access_write_reg16(radio->radio_id, REG16_IF_BASEBAND_GAIN_LNA, REG16_IF_BASEBAND_GAIN_LNA_DEFAULT);

    do {
        sr_access_write_reg8(radio->radio_id, REG8_ACTIONS, 0x00);
        pwr_status = sr_access_read_reg8(radio->radio_id, REG8_POWER_STATE);
    } while (!GET_AWAKE(pwr_status));
}

/** @brief Put the radio in Delay line static power state.
 *
 *  @note This consist of :
 *          - Reset main commands   (0x01)
 *          - Reset timer/sleep cfg (0x04 - 0x05)
 *          - Disabling the buffer load interrupt (0x26)
 *          - Default Debug control (0x27)
 *          - Default main feature  (0x28)
 *          - Set integgain to 3    (0x0C)
 *
 *  @param[in] radio  SR hardware abstraction layer instance.
 */
static void put_radio_in_dll_power_state(radio_t *radio)
{
    uint8_t pwr_status;

    sr_access_write_reg16(radio->radio_id, REG16_FRAMEPROC_PHASEDATA, TX_MODE);
    sr_access_write_reg16(radio->radio_id, REG16_TIMERCFG_SLEEPCFG, 0x00);
    sr_access_write_reg16(radio->radio_id, REG16_IF_BASEBAND_GAIN_LNA, REG16_IF_BASEBAND_GAIN_LNA_DEFAULT);

    do {
        sr_access_write_reg8(radio->radio_id, REG8_ACTIONS, 0x00);
        pwr_status = sr_access_read_reg8(radio->radio_id, REG8_POWER_STATE);
    } while (!GET_AWAKE(pwr_status));
}
