/** @file  max98091.c
 *  @brief Driver for the max98091 codec.
 *
 *  @copyright Copyright (C) 2018-2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "max98091.h"
#include <stddef.h>

/* PRIVATE GLOBALS ************************************************************/
static max98091_register_map_t reg_map = {0};
static max98091_biq_t biquad_bands;
static uint16_t ni_arr[] = {
    1,   /* 96k   */
    1,   /* 48k   */
    147, /* 44.1k */
    1,   /* 32k   */
    1,   /* 24k   */
    1,   /* 16k   */
    1,   /* 12k   */
    1    /*  8k   */
};
static uint16_t mi_arr[] = {
    2,   /* 96k   */
    2,   /* 48k   */
    320, /* 44.1k */
    3,   /* 32k   */
    4,   /* 24k   */
    6,   /* 16k   */
    8,   /* 12k   */
    12   /*  8k   */
};

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void init_reg_map_default(max98091_codec_cfg_t *codec_cfg);
static void shutdown_codec(max98091_i2c_hal_t *i2c_hal);
static void powerup_codec(max98091_i2c_hal_t *i2c_hal);
static void configure_interupt_mask(max98091_i2c_hal_t *i2c_hal);
static void configure_input_mode_and_source(max98091_i2c_hal_t *i2c_hal);
static void configure_clocks(max98091_i2c_hal_t *i2c_hal);
static void configure_audio_interface(max98091_i2c_hal_t *i2c_hal);
static void configure_dsp(max98091_i2c_hal_t *i2c_hal);
static void configure_equalizer(max98091_i2c_hal_t *i2c_hal);
static void configure_speakers(max98091_i2c_hal_t *i2c_hal);
static void configure_dynamic_range(max98091_i2c_hal_t *i2c_hal);
static void configure_bias_and_power(max98091_i2c_hal_t *i2c_hal);
static void configure_analog_mixer(max98091_i2c_hal_t *i2c_hal);
static void configure_biquad(max98091_i2c_hal_t *i2c_hal);
static void configure_sidetone(max98091_i2c_hal_t *i2c_hal);
static void configure_input(max98091_i2c_hal_t *i2c_hal);
static void configure_bias_voltage(max98091_i2c_hal_t *i2c_hal);
static void configure_zdv_and_volume(max98091_i2c_hal_t *i2c_hal);
static void enable_jack_input_output(max98091_i2c_hal_t *i2c_hal);
static void configure_microphone(max98091_i2c_hal_t *i2c_hal);
static void init_filters_coefficients(max98091_biq_t *biquad_bands);
static void configure_biquad_coefficients(max98091_i2c_hal_t *i2c_hal, max98091_biq_t *biquad);
static void activate_playback_filters(max98091_i2c_hal_t *i2c_hal, max98091_playback_filter_t filter_en);
static void activate_record_filter(max98091_i2c_hal_t *i2c_hal, bool filter_en);
static void configure_headphone_vol(max98091_i2c_hal_t *i2c_hal);
static void configure_speaker_vol(max98091_i2c_hal_t *i2c_hal);
static void configure_lineout_vol(max98091_i2c_hal_t *i2c_hal);
static void configure_quick_setup_configuration(max98091_i2c_hal_t *i2c_hal);
static jack_state_t get_jack_status(max98091_i2c_hal_t *i2c_hal);

/* PUBLIC FUNCTIONS ***********************************************************/
void max98091_init(max98091_i2c_hal_t *i2c_hal, max98091_codec_cfg_t *codec_cfg)
{
    if ((i2c_hal->write == NULL) || (i2c_hal->read == NULL)) {
        return;
    }

    if ((i2c_hal->i2c_addr != MAX98091A_I2C_ADDR) && (i2c_hal->i2c_addr != MAX98091B_I2C_ADDR)) {
        /* Default to MAX98091A_I2C_ADDR */
        i2c_hal->i2c_addr = MAX98091A_I2C_ADDR;
    }

    max98091_reset_codec(i2c_hal);

    init_reg_map_default(codec_cfg);

    configure_quick_setup_configuration(i2c_hal);

    /* Shutdown codec Step 1 */
    shutdown_codec(i2c_hal);

    /* Configure clocks */
    configure_clocks(i2c_hal);

    /* Configure interrupt mask */
    configure_interupt_mask(i2c_hal);

    /* Configure digital audio i2c_hal (DAI) */
    configure_input_mode_and_source(i2c_hal);

    /* Configure digital signal processing (DSP) */
    configure_dsp(i2c_hal);

    /* Load coefficients */
    configure_dynamic_range(i2c_hal);
    configure_biquad(i2c_hal);

    /* Configure Power and bias mode */
    configure_bias_and_power(i2c_hal);

    /* Configure analog Mixers */
    configure_analog_mixer(i2c_hal);
    configure_equalizer(i2c_hal);
    configure_bias_voltage(i2c_hal);

    /* Configure analog Gain and volume control */
    configure_headphone_vol(i2c_hal);
    configure_speaker_vol(i2c_hal);
    configure_lineout_vol(i2c_hal);
    configure_zdv_and_volume(i2c_hal);

    /* Configure misclelaneous functions */
    configure_audio_interface(i2c_hal);
    configure_speakers(i2c_hal);
    configure_sidetone(i2c_hal);
    configure_input(i2c_hal);
    enable_jack_input_output(i2c_hal);
    configure_microphone(i2c_hal);
    if (codec_cfg->playback_filter_enabled) {
        init_filters_coefficients(&biquad_bands);
        configure_biquad_coefficients(i2c_hal, &biquad_bands);
        activate_playback_filters(i2c_hal, MAX98091_PLAYBACK_FILTER_7_BANDS);
    } else {
        activate_playback_filters(i2c_hal, MAX98091_PLAYBACK_NO_FILTER);
    }

    if (codec_cfg->record_filter_enabled) {
        init_filters_coefficients(&biquad_bands);
        configure_biquad_coefficients(i2c_hal, &biquad_bands);
    }

    activate_record_filter(i2c_hal, codec_cfg->record_filter_enabled);

    /* Power up codec */
    powerup_codec(i2c_hal);
}

void max98091_reset_codec(max98091_i2c_hal_t *i2c_hal)
{
    max98091_reg_soft_reset_t soft_reset;

    soft_reset.bit.swreset = 1;

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_SOFTWARE_RESET, soft_reset.reg);
}

void max98091_hp_increase_volume(max98091_i2c_hal_t *i2c_hal)
{
    max98091_unmute_volume(i2c_hal);

    /* Validate the volume is not at maximum */
    if (reg_map.gain_set.left_hp_volume.bit.hpvoll < MAX98091_HP_MAX_VOLUME) {
        reg_map.gain_set.left_hp_volume.bit.hpvoll++;
        reg_map.gain_set.right_hp_volume.reg = reg_map.gain_set.left_hp_volume.reg;

        i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_LEFT_HP_VOLUME, reg_map.gain_set.left_hp_volume.reg);
        i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_RIGHT_HP_VOLUME, reg_map.gain_set.right_hp_volume.reg);
    } else {
        return;
    }
}

void max98091_hp_decrease_volume(max98091_i2c_hal_t *i2c_hal)
{
    max98091_unmute_volume(i2c_hal);

    /* Validate the volume is not at maximum */
    if (reg_map.gain_set.left_hp_volume.bit.hpvoll > MAX98091_HP_MIN_VOLUME) {
        reg_map.gain_set.left_hp_volume.bit.hpvoll--;
        reg_map.gain_set.right_hp_volume.reg = reg_map.gain_set.left_hp_volume.reg;

        i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_LEFT_HP_VOLUME, reg_map.gain_set.left_hp_volume.reg);
        i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_RIGHT_HP_VOLUME, reg_map.gain_set.right_hp_volume.reg);
    } else {
        return;
    }
}

void max98091_unmute_volume(max98091_i2c_hal_t *i2c_hal)
{
    reg_map.dai_playback_lvl.bit.dvm = MAX98091_VOLUME_UNMUTE;

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_DAI_PLAYBACK_LEVEL, reg_map.dai_playback_lvl.reg);
}

void max98091_mute_volume(max98091_i2c_hal_t *i2c_hal)
{
    reg_map.dai_playback_lvl.bit.dvm = MAX98091_VOLUME_MUTE;

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_DAI_PLAYBACK_LEVEL, reg_map.dai_playback_lvl.reg);
}

void max98091_hp_set_volume(max98091_i2c_hal_t *i2c_hal, max98091_hp_vol_t volume)
{
    max98091_unmute_volume(i2c_hal);

    if (volume > MAX98091_HP_MAX_VOLUME) {
        reg_map.gain_set.left_hp_volume.bit.hpvoll = MAX98091_HP_MAX_VOLUME;
        reg_map.gain_set.right_hp_volume.bit.hpvolr = MAX98091_HP_MAX_VOLUME;
    } else if (volume <= MAX98091_HP_MAX_VOLUME) {
        reg_map.gain_set.left_hp_volume.bit.hpvoll = volume;
        reg_map.gain_set.right_hp_volume.bit.hpvolr = volume;
    }
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_LEFT_HP_VOLUME, reg_map.gain_set.left_hp_volume.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_RIGHT_HP_VOLUME, reg_map.gain_set.right_hp_volume.reg);
}

bool max98091_is_microphone_present(max98091_i2c_hal_t *i2c_hal)
{
    if (get_jack_status(i2c_hal) == MAX98091_JACK_HEADSET) {
        return true;
    } else {
        return false;
    }
}

bool max98091_is_jack_present(max98091_i2c_hal_t *i2c_hal)
{
    if ((get_jack_status(i2c_hal) == MAX98091_JACK_HEADSET) ||
        (get_jack_status(i2c_hal) == MAX98091_JACK_HEADPHONE)) {
        return true;
    } else {
        return false;
    }
}

void max98091_enable_mic_trrs(max98091_i2c_hal_t *i2c_hal)
{
    reg_map.left_adc_mixer.bit.mixadl |= MAX98091_SET_MICROPHONE;
    reg_map.right_adc_mixer.bit.mixadr |= MAX98091_SET_MICROPHONE;

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_LEFT_ADC_MIXER, reg_map.left_adc_mixer.bit.mixadl);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_RIGHT_ADC_MIXER, reg_map.right_adc_mixer.bit.mixadr);
}

void max98091_disable_mic_trrs(max98091_i2c_hal_t *i2c_hal)
{
    reg_map.left_adc_mixer.bit.mixadl &= ~MAX98091_SET_MICROPHONE;
    reg_map.right_adc_mixer.bit.mixadr &= ~MAX98091_SET_MICROPHONE;

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_LEFT_ADC_MIXER, reg_map.left_adc_mixer.bit.mixadl);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_RIGHT_ADC_MIXER, reg_map.right_adc_mixer.bit.mixadr);
}

void max98091_enable_output(max98091_i2c_hal_t *i2c_hal)
{
    reg_map.output_en.bit.dalen  = true,
    reg_map.output_en.bit.daren  = true,
    reg_map.output_en.bit.hplen  = true,
    reg_map.output_en.bit.hpren  = true,
    reg_map.output_en.bit.rcvlen = true,
    reg_map.output_en.bit.rcvren = true,
    reg_map.output_en.bit.splen  = false,
    reg_map.output_en.bit.spren  = false,

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_OUTPUT_ENABLE, reg_map.output_en.reg);
}

void max98091_disable_output(max98091_i2c_hal_t *i2c_hal)
{
   i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_OUTPUT_ENABLE, 0x00);
}

void max98091_reset_codec_irq(max98091_i2c_hal_t *i2c_hal)
{
    uint8_t codec_status = 0;

    i2c_hal->read(i2c_hal->i2c_addr, MAX98091_REG_DEVICE_STATUS, &codec_status);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize the audio codec's register structure.
 *
 *  @param[in] codec_cfg       Configuration structure for the codec.
 */
static void init_reg_map_default(max98091_codec_cfg_t *codec_cfg)
{
    bool mic_en = false;

    /* INIT MAX98091 */
    /* clk */
    reg_map.qs_sys.reg = 0;         /* Disable quick setup... */
    reg_map.qs_sample_rate.reg = 0; /* ...configuration       */

    reg_map.sys_clk.bit.psclk = 0b01;     /* fPCLK = fMCLK */
    reg_map.clk_mode.bit.freq = 0;        /* Disable exact integer mode */
    reg_map.clk_mode.bit.use_mi = 1;      /* Set Mi Manually */
    reg_map.master_mode.bit.mas = 0b1;    /* Master mode */
    if (codec_cfg->word_size == MAX98091_AUDIO_16BITS) {
        reg_map.master_mode.bit.bsel = 0b001; /* Bit clock freq is  32 x Fs (16 bits per frame) */
    } else {
        reg_map.master_mode.bit.bsel = 0b011; /* Bit clock freq is  64 x Fs (32 bits per frame) */
    }

    if (codec_cfg->sampling_rate == MAX98091_AUDIO_96KHZ) {
        reg_map.filter_config.bit.dhf = 1; /* LRCLK is more than 48kHz. 4x FIR interpolation filter used */
        reg_map.adc_ctrl.bit.osr128 = 0; /* 64 x Fs */
    } else {
        reg_map.filter_config.bit.dhf = 0; /* LRCLK is less than 48kHz. 8x FIR interpolation filter used */
        reg_map.adc_ctrl.bit.osr128 = 1; /* 128 x Fs */
    }

    /* SEE README FOR INFO */
    uint16_t ni = ni_arr[codec_cfg->sampling_rate];

    reg_map.clk_ratio_ni_msb.bit.ni = (ni >> 8) & 0xFF;
    reg_map.clk_ratio_ni_lsb.bit.ni = (ni) & 0xFF;

    uint16_t mi = mi_arr[codec_cfg->sampling_rate];

    reg_map.clk_ratio_mi_msb.bit.mi = (mi >> 8) & 0xFF;
    reg_map.clk_ratio_mi_lsb.bit.mi = (mi) & 0xFF;

    /* Interrupt */
    reg_map.interrupt_mask.reg = 0x04; /* Jack detection flag */

    /* DAI: RIGHT JUSTIFIED STANDARD */
    reg_map.interface_format.bit.rj = 0b1; /*  right justified */
    reg_map.interface_format.bit.wci = 0b0; /* left channel on lrclk high */
    reg_map.interface_format.bit.bci = 0b0; /* Data is valid on rising edge of bclk */
    reg_map.interface_format.bit.dly = 0b0; /* No delay */
    reg_map.interface_format.bit.ws = (codec_cfg->word_size & 0b11); /* Word size */
    reg_map.tdm_ctrl.bit.tmd = 0b0; /* disable tdm */

    /* Set all gains to minimum values by default */
    reg_map.gain_set.hp_ctrl.bit.mixhplg = MAX98091_MIXER_GAIN_N12DB;
    reg_map.gain_set.hp_ctrl.bit.mixhprg = MAX98091_MIXER_GAIN_N12DB;
    reg_map.gain_set.spk_ctrl.bit.mxisplg = MAX98091_MIXER_GAIN_N12DB;
    reg_map.gain_set.spk_ctrl.bit.mxisprg = MAX98091_MIXER_GAIN_N12DB;
    reg_map.gain_set.rcv_loutl_ctrl.bit.mixrcvlg = MAX98091_MIXER_GAIN_N12DB;
    reg_map.gain_set.loutr_ctrl.bit.mixrcvrg = MAX98091_MIXER_GAIN_N12DB;
    reg_map.gain_set.left_hp_volume.bit.hplm = MAX98091_MUTE;
    reg_map.gain_set.left_hp_volume.bit.hpvoll = MAX98091_HP_MIN_VOLUME;
    reg_map.gain_set.right_hp_volume.bit.hprm = MAX98091_MUTE;
    reg_map.gain_set.right_hp_volume.bit.hpvolr = MAX98091_HP_MIN_VOLUME;
    reg_map.gain_set.left_spk_volume.bit.splm = MAX98091_MUTE;
    reg_map.gain_set.left_spk_volume.bit.spvoll = MAX98091_SPK_MIN_VOLUME;
    reg_map.gain_set.right_spk_volume.bit.sprm = MAX98091_MUTE;
    reg_map.gain_set.right_spk_volume.bit.spvolr = MAX98091_SPK_MIN_VOLUME;
    reg_map.gain_set.rcv_loutl_volume.bit.rcvlm = MAX98091_MUTE;
    reg_map.gain_set.rcv_loutl_volume.bit.rcvlvol = MAX98091_LOUT_MIN_VOLUME;
    reg_map.gain_set.loutr_volume.bit.rcvrm = MAX98091_MUTE;
    reg_map.gain_set.loutr_volume.bit.rcvrvol = MAX98091_LOUT_MIN_VOLUME;

    if (codec_cfg->record_enabled) {
        reg_map.io_config.bit.sdoen = 0b1; /* enable serial data out */

        reg_map.line_in_config.bit.in3seen = 0b1; /* IN3 single ended on LINE A */
        reg_map.line_in_config.bit.in4seen = 0b1; /* IN4 single ended on LINE B */
        reg_map.left_adc_mixer.reg = 0b00001000; /* Left ADC mix on LINE A */
        reg_map.right_adc_mixer.reg = 0b00010000; /* Right ADC mix on LINE B */

        reg_map.input_en.bit.adlen = 0b1; /* Enable left ADC */
        reg_map.input_en.bit.adren = 0b1; /* Enable right ADC */
        reg_map.input_en.bit.lineaen = 0b1; /* Enable LINE A */
        reg_map.input_en.bit.lineben = 0b1; /* Enable LINE B */
        reg_map.input_en.bit.mben = 0b1; /* Enable MIC1 bias */
        reg_map.mic_bias_voltage.reg = 0b11; /* 00: 2.2V, 01: 2.4V, 10: 2.55V, 11: 2.8V */

        /* digital mic */
        reg_map.dig_mic_en.bit.micclk = 2; /* fDMC = fPCLK/4 (3.072MHz) */
        reg_map.dig_mic_config.bit.dmic_comp = 3; /* Based on table 17 in codec datasheet */
        reg_map.dig_mic_config.bit.dmic_freq = 0b00;
        if (mic_en) { /* Enable digital microphone */
            reg_map.dig_mic_en.bit.digmicl = 1;
            reg_map.dig_mic_en.bit.digmicr = 1;
        }

        /* input lvl */
        reg_map.line_in_lvl.reg = 0x3F; /* -6dB */
        reg_map.in_mode.reg = 0x00; /* default */
        reg_map.mic1_in_lvl.reg = 0x54; /* MIC 1 enabled , 20 dB gain */
        reg_map.mic2_in_lvl.reg = 0x14; /* default */
    }
    if (codec_cfg->playback_enabled) {
        reg_map.io_config.bit.sdien = 0b1; /* enable serial data in */

        reg_map.left_hp_mixer.bit.mixhpl = 0b100000; /* left adc to left hp */
        reg_map.right_hp_mixer.bit.mixhpr = 0b100000; /* right adc to right hp */
        reg_map.gain_set.hp_ctrl.bit.mixhplsel = 0x00; /* dac only source */
        reg_map.gain_set.hp_ctrl.bit.mixhprsel = 0x00;
        reg_map.rcv_loutl_mixer.bit.mixrcvl = 1; /* left adc to left line out */
        reg_map.loutr_mixer.bit.linmod = 1; /* Line Output mode. Left and right channels are programmed independently */
        reg_map.loutr_mixer.bit.mixrcvr = 2; /* right adc to right line out */

        reg_map.output_en.bit.dalen = 0b1; /* Enable left DAC */
        reg_map.output_en.bit.daren = 0b1; /* Enable right DAC */
        reg_map.output_en.bit.hplen = 0b1; /* Enable left Headphone */
        reg_map.output_en.bit.hpren = 0b1; /* Enable right Headphone */
        reg_map.output_en.bit.rcvlen = 0b1; /* Enable left line out */
        reg_map.output_en.bit.rcvren = 0b1; /* Enable right line out */

        /* analog gain and volumes */
        reg_map.gain_set.hp_ctrl.bit.mixhplg = MAX98091_MIXER_GAIN_0DB;
        reg_map.gain_set.hp_ctrl.bit.mixhprg = MAX98091_MIXER_GAIN_0DB;
        reg_map.gain_set.rcv_loutl_ctrl.bit.mixrcvlg = MAX98091_MIXER_GAIN_0DB;
        reg_map.gain_set.loutr_ctrl.bit.mixrcvrg = MAX98091_MIXER_GAIN_0DB;

        reg_map.gain_set.left_hp_volume.bit.hplm = MAX98091_UNMUTE;
        reg_map.gain_set.left_hp_volume.bit.hpvoll = MAX98091_HP_VOL_1DB;
        reg_map.gain_set.right_hp_volume.bit.hprm = MAX98091_UNMUTE;
        reg_map.gain_set.right_hp_volume.bit.hpvolr = MAX98091_HP_VOL_1DB;
        reg_map.gain_set.rcv_loutl_volume.bit.rcvlm = MAX98091_UNMUTE;
        reg_map.gain_set.rcv_loutl_volume.bit.rcvlvol = MAX98091_LOUT_VOL_M14DB;
        reg_map.gain_set.loutr_volume.bit.rcvrm = MAX98091_UNMUTE;
        reg_map.gain_set.loutr_volume.bit.rcvrvol = MAX98091_LOUT_VOL_M14DB;
    }

    /* dsp */
    reg_map.left_rec_lvl.reg = 0x03; /*0db*/
    reg_map.right_rec_lvl.reg = 0x03; /*0db*/
    /* No playback filters */
    reg_map.dsp_filter_en.bit.eq3banden = 0;
    reg_map.dsp_filter_en.bit.eq5banden = 0;
    reg_map.dsp_filter_en.bit.eq7banden = 0;
    /* no record filters */
    reg_map.dsp_filter_en.bit.recbqen = 0;
    /* dsp filter config */
    reg_map.filter_config.bit.mode = 1; /* Audio mode */
    reg_map.filter_config.bit.ahpf = 0; /* Disable the Record Path DC-Blocking Filter */
    reg_map.filter_config.bit.dhpf = 0; /* Disable the Playback Path DC-Blocking Filter */

    reg_map.drc_gain.bit.drcg = 0; /* PLAYBACK DRC Make-Up Gain : 0dB */
    reg_map.drc_compressor.bit.drccmp = 0; /* PLAYBACK DRC Compression Ratio : 1:1 */
    reg_map.drc_compressor.bit.drcthc = 0; /* PLAYBACK DRC Compression Threshold : 0dB */
    reg_map.drc_expander.bit.drcexp = 0; /* PLAYBACK DRC Expansion Ratio : 1:1 */
    reg_map.drc_expander.bit.drcthe = 0; /* PLAYBACK DRC Expansion Threshold : 35dB */
    reg_map.drc_timing.bit.drcatk = 0; /* PLAYBACK DRC Attack Time Configuration : 0.125ms */
    reg_map.drc_timing.bit.drcen = 0; /* PLAYBACK DRC DISABLE */
    reg_map.drc_timing.bit.drcrls = 0; /* PLAYBACK DRC Release Time Configuration : 8s */

    /*power and bias*/
    reg_map.bias_ctrl.bit.bias_mode = 0; /* BIAS derived from resistive division */
    reg_map.dac_ctrl.bit.dachp = 0; /* DAC settings optimized for lowest power consumption */
    reg_map.dac_ctrl.bit.perfmode = 0; /* High performance headphone playback mode */
    reg_map.adc_ctrl.bit.adchp = 0; /* ADC is optimized for low power operation */
    reg_map.adc_ctrl.bit.adcdither = 1; /* ADC Quantizer Dither Enabled */

    /*MISC*/
    reg_map.lvl_ctrl.bit.not_vsen = 1;  /* Volume changes are smoothed by stepping through intermediate levels */
    reg_map.lvl_ctrl.bit.not_vs2en = 1; /* Each volume change waits until the previous */
                                                /* volume step has been applied to the output. Allows volume smoothing */
                                                /* to function with zero-crossing timeout */
    reg_map.lvl_ctrl.bit.not_zden = 0;  /* Volume changes made only at zero crossings or after approximately 100ms */

    /* Jack detection */
    reg_map.jack_detect.bit.jdeten = 1;
}

/** @brief Put the max98091 codec in a shutdown state.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
static void shutdown_codec(max98091_i2c_hal_t *i2c_hal)
{
    reg_map.shutdown.bit.not_shdn = MAX98091_DEV_SHUTDOWN;

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_DEVICE_SHUTDOWN, reg_map.shutdown.reg);
}

/** @brief Power up the max98091 codec.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
static void powerup_codec(max98091_i2c_hal_t *i2c_hal)
{
    reg_map.shutdown.bit.not_shdn = MAX98091_DEV_PWRUP;

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_DEVICE_SHUTDOWN, reg_map.shutdown.reg);
}

/** @brief Setup the max98091 codec interrupt mask.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
static void configure_interupt_mask(max98091_i2c_hal_t *i2c_hal)
{
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_INTERRUPT_MASKS, reg_map.interrupt_mask.reg);
}

/** @brief Setup the max98091 codec input and source configuration register.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
static void configure_input_mode_and_source(max98091_i2c_hal_t *i2c_hal)
{
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_INPUT_MODE, reg_map.in_mode.reg);
}

/** @brief Setup the max98091 codec clocks registers.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
static void configure_clocks(max98091_i2c_hal_t *i2c_hal)
{
    /* Clock Mode Configuration Register */
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_SYSTEM_CLOCK, reg_map.sys_clk.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_CLOCK_MODE, reg_map.clk_mode.reg);

    /* Manual Clock Ratio Configuration Register */
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_CLOCK_RATIO_NI_MSB, reg_map.clk_ratio_ni_msb.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_CLOCK_RATIO_NI_LSB, reg_map.clk_ratio_ni_lsb.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_CLOCK_RATIO_MI_MSB, reg_map.clk_ratio_mi_msb.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_CLOCK_RATIO_MI_LSB, reg_map.clk_ratio_mi_lsb.reg);

    /* Master Mode Clock Configuration Register */
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_MASTER_MODE, reg_map.master_mode.reg);
}

/** @brief Setup the max98091 codec Digital Audio Interface (DAI) TDM Control Registers.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
static void configure_audio_interface(max98091_i2c_hal_t *i2c_hal)
{
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_INTERFACE_FORMAT, reg_map.interface_format.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_IO_CONFIGURATION, reg_map.io_config.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_TDM_CONTROL, reg_map.tdm_ctrl.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_TDM_FORMAT, reg_map.tdm_format.reg);
}

/** @brief Setup the max98091 codec digital signal processing registers.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
static void configure_dsp(max98091_i2c_hal_t *i2c_hal)
{
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_FILTER_CONFIG, reg_map.filter_config.reg);
}

/** @brief Setup the max98091 codec Parametric Equalizer Playback Level Configuration Register.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
static void configure_equalizer(max98091_i2c_hal_t *i2c_hal)
{
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_PLAYBACK_LEVEL, reg_map.eq_playback_lvl.reg);
}

/** @brief Setup the max98091 codec Left & Right Speaker Mixer Configuration Register
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
static void configure_speakers(max98091_i2c_hal_t *i2c_hal)
{
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_LEFT_SPK_MIXER, reg_map.left_spk_mixer.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_RIGHT_SPK_MIXER, reg_map.right_spk_mixer.reg);
}

/** @brief Setup the max98091 codec dynamic range control register.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
static void configure_dynamic_range(max98091_i2c_hal_t *i2c_hal)
{
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_DRC_TIMING, reg_map.drc_timing.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_DRC_COMPRESSOR, reg_map.drc_compressor.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_DRC_EXPANDER, reg_map.drc_expander.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_DRC_GAIN, reg_map.drc_gain.reg);
}

/** @brief Setup the max98091 codec bias and poer mode configuration register.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
static void configure_bias_and_power(max98091_i2c_hal_t *i2c_hal)
{
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_BIAS_CONTROL, reg_map.bias_ctrl.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_DAC_CONTROL, reg_map.dac_ctrl.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_ADC_CONTROL, reg_map.adc_ctrl.reg);
}

/** @brief Setup the max98091 codec analog mixer configuration register.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
static void configure_analog_mixer(max98091_i2c_hal_t *i2c_hal)
{
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_LINE_INPUT_CONFIG, reg_map.line_in_config.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_LEFT_ADC_MIXER, reg_map.left_adc_mixer.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_RIGHT_ADC_MIXER, reg_map.right_adc_mixer.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_LEFT_HP_MIXER, reg_map.left_hp_mixer.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_RIGHT_HP_MIXER, reg_map.right_hp_mixer.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_RCV_LOUTL_MIXER, reg_map.rcv_loutl_mixer.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_LOUTR_MIXER, reg_map.loutr_mixer.reg);
}

/** @brief Setup the max98091 codec primary record path Biquad digital preamplifier level configuration register.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
static void configure_biquad(max98091_i2c_hal_t *i2c_hal)
{
   i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_REC_BIQUAD_LEVEL, reg_map.rec_biq_lvl.reg);
}

/** @brief Setup the max98091 codec record path sidetone configuration register.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
static void configure_sidetone(max98091_i2c_hal_t *i2c_hal)
{
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_REC_SIDETONE, reg_map.rec_sidetone.reg);
}

/** @brief Setup the max98091 codec inputs.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
static void configure_input(max98091_i2c_hal_t *i2c_hal)
{
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_LINE_INPUT_LEVEL, reg_map.line_in_lvl.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_MIC1_INPUT_LEVEL, reg_map.mic1_in_lvl.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_MIC2_INPUT_LEVEL, reg_map.mic2_in_lvl.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_LEFT_REC_LEVEL, reg_map.left_rec_lvl.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_RIGHT_REC_LEVEL, reg_map.right_rec_lvl.reg);
}

/** @brief Setup the max98091 codec mic Bias voltage register.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
static void configure_bias_voltage(max98091_i2c_hal_t *i2c_hal)
{
     i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_MIC_BIAS_VOLTAGE, reg_map.mic_bias_voltage.reg);
}

/** @brief Setup the max98091 codec volume control.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
static void configure_zdv_and_volume(max98091_i2c_hal_t *i2c_hal)
{
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_LEVEL_CONTROL, reg_map.lvl_ctrl.reg);
}

/** @brief Enable jack input and output.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
static void enable_jack_input_output(max98091_i2c_hal_t *i2c_hal)
{
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_OUTPUT_ENABLE, reg_map.output_en.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_INPUT_ENABLE, reg_map.input_en.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_JACK_DETECT, reg_map.jack_detect.reg);
}

/** @brief Configure and enable microphone.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
static void configure_microphone(max98091_i2c_hal_t *i2c_hal)
{
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_DIGITAL_MIC_CONFIG, reg_map.dig_mic_config.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_DIGITAL_MIC_ENABLE, reg_map.dig_mic_en.reg);
}

/** @brief Initialize the BiQuad Band structure.
 *
 *  @param[in] biquad_bands  BiQuad Band structure.
 */
static void init_filters_coefficients(max98091_biq_t *biquad_bands)
{
    /*  IIR filter coefficients at 48kHz sampling freq */
    /* [4bits]int [20bits]dec */

    biquad_bands->band1.coef_b0.reg = LS_172_B0;
    biquad_bands->band1.coef_b1.reg = LS_172_B1;
    biquad_bands->band1.coef_b2.reg = LS_172_B2;
    biquad_bands->band1.coef_a1.reg = LS_172_A1;
    biquad_bands->band1.coef_a2.reg = LS_172_A2;

    biquad_bands->band2.coef_b0.reg = FLAT_B0;
    biquad_bands->band2.coef_b1.reg = FLAT_B1;
    biquad_bands->band2.coef_b2.reg = FLAT_B2;
    biquad_bands->band2.coef_a1.reg = FLAT_A1;
    biquad_bands->band2.coef_a2.reg = FLAT_A2;

    biquad_bands->band3.coef_b0.reg = FLAT_B0;
    biquad_bands->band3.coef_b1.reg = FLAT_B1;
    biquad_bands->band3.coef_b2.reg = FLAT_B2;
    biquad_bands->band3.coef_a1.reg = FLAT_A1;
    biquad_bands->band3.coef_a2.reg = FLAT_A2;

    biquad_bands->band4.coef_b0.reg = FLAT_B0;
    biquad_bands->band4.coef_b1.reg = FLAT_B1;
    biquad_bands->band4.coef_b2.reg = FLAT_B2;
    biquad_bands->band4.coef_a1.reg = FLAT_A1;
    biquad_bands->band4.coef_a2.reg = FLAT_A2;

    biquad_bands->band5.coef_b0.reg = FLAT_B0;
    biquad_bands->band5.coef_b1.reg = FLAT_B1;
    biquad_bands->band5.coef_b2.reg = FLAT_B2;
    biquad_bands->band5.coef_a1.reg = FLAT_A1;
    biquad_bands->band5.coef_a2.reg = FLAT_A2;

    biquad_bands->band6.coef_b0.reg = FLAT_B0;
    biquad_bands->band6.coef_b1.reg = FLAT_B1;
    biquad_bands->band6.coef_b2.reg = FLAT_B2;
    biquad_bands->band6.coef_a1.reg = FLAT_A1;
    biquad_bands->band6.coef_a2.reg = FLAT_A2;

    biquad_bands->band7.coef_b0.reg = FLAT_B0;
    biquad_bands->band7.coef_b1.reg = FLAT_B1;
    biquad_bands->band7.coef_b2.reg = FLAT_B2;
    biquad_bands->band7.coef_a1.reg = FLAT_A1;
    biquad_bands->band7.coef_a2.reg = FLAT_A2;

    biquad_bands->record.coef_b0.reg = LP_12k_B0;
    biquad_bands->record.coef_b1.reg = LP_12k_B1;
    biquad_bands->record.coef_b2.reg = LP_12k_B2;
    biquad_bands->record.coef_a1.reg = LP_12k_A1;
    biquad_bands->record.coef_a2.reg = LP_12k_A2;
}

/** @brief Configure BiQuad Coefficients in the codec.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
static void configure_biquad_coefficients(max98091_i2c_hal_t *i2c_hal, max98091_biq_t *biquad)
{
    /* BAND 1 */
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND1_B0_23_16, biquad->band1.coef_b0.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND1_B0_15_08, biquad->band1.coef_b0.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND1_B0_07_00, biquad->band1.coef_b0.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND1_B1_23_16, biquad->band1.coef_b1.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND1_B1_15_08, biquad->band1.coef_b1.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND1_B1_07_00, biquad->band1.coef_b1.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND1_B2_23_16, biquad->band1.coef_b2.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND1_B2_15_08, biquad->band1.coef_b2.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND1_B2_07_00, biquad->band1.coef_b2.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND1_A1_23_16, biquad->band1.coef_a1.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND1_A1_15_08, biquad->band1.coef_a1.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND1_A1_07_00, biquad->band1.coef_a1.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND1_A2_23_16, biquad->band1.coef_a2.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND1_A2_15_08, biquad->band1.coef_a2.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND1_A2_07_00, biquad->band1.coef_a2.bit.lsb0_7);

    /* BAND 2 */
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND2_B0_23_16, biquad->band2.coef_b0.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND2_B0_15_08, biquad->band2.coef_b0.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND2_B0_07_00, biquad->band2.coef_b0.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND2_B1_23_16, biquad->band2.coef_b1.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND2_B1_15_08, biquad->band2.coef_b1.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND2_B1_07_00, biquad->band2.coef_b1.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND2_B2_23_16, biquad->band2.coef_b2.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND2_B2_15_08, biquad->band2.coef_b2.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND2_B2_07_00, biquad->band2.coef_b2.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND2_A1_23_16, biquad->band2.coef_a1.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND2_A1_15_08, biquad->band2.coef_a1.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND2_A1_07_00, biquad->band2.coef_a1.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND2_A2_23_16, biquad->band2.coef_a2.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND2_A2_15_08, biquad->band2.coef_a2.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND2_A2_07_00, biquad->band2.coef_a2.bit.lsb0_7);

    /* BAND 3 */
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND3_B0_23_16, biquad->band3.coef_b0.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND3_B0_15_08, biquad->band3.coef_b0.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND3_B0_07_00, biquad->band3.coef_b0.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND3_B1_23_16, biquad->band3.coef_b1.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND3_B1_15_08, biquad->band3.coef_b1.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND3_B1_07_00, biquad->band3.coef_b1.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND3_B2_23_16, biquad->band3.coef_b2.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND3_B2_15_08, biquad->band3.coef_b2.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND3_B2_07_00, biquad->band3.coef_b2.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND3_A1_23_16, biquad->band3.coef_a1.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND3_A1_15_08, biquad->band3.coef_a1.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND3_A1_07_00, biquad->band3.coef_a1.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND3_A2_23_16, biquad->band3.coef_a2.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND3_A2_15_08, biquad->band3.coef_a2.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND3_A2_07_00, biquad->band3.coef_a2.bit.lsb0_7);

    /*BAND 4*/
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND4_B0_23_16, biquad->band4.coef_b0.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND4_B0_15_08, biquad->band4.coef_b0.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND4_B0_07_00, biquad->band4.coef_b0.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND4_B1_23_16, biquad->band4.coef_b1.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND4_B1_15_08, biquad->band4.coef_b1.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND4_B1_07_00, biquad->band4.coef_b1.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND4_B2_23_16, biquad->band4.coef_b2.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND4_B2_15_08, biquad->band4.coef_b2.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND4_B2_07_00, biquad->band4.coef_b2.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND4_A1_23_16, biquad->band4.coef_a1.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND4_A1_15_08, biquad->band4.coef_a1.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND4_A1_07_00, biquad->band4.coef_a1.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND4_A2_23_16, biquad->band4.coef_a2.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND4_A2_15_08, biquad->band4.coef_a2.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND4_A2_07_00, biquad->band4.coef_a2.bit.lsb0_7);

    /*BAND 5*/
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND5_B0_23_16, biquad->band5.coef_b0.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND5_B0_15_08, biquad->band5.coef_b0.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND5_B0_07_00, biquad->band5.coef_b0.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND5_B1_23_16, biquad->band5.coef_b1.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND5_B1_15_08, biquad->band5.coef_b1.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND5_B1_07_00, biquad->band5.coef_b1.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND5_B2_23_16, biquad->band5.coef_b2.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND5_B2_15_08, biquad->band5.coef_b2.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND5_B2_07_00, biquad->band5.coef_b2.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND5_A1_23_16, biquad->band5.coef_a1.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND5_A1_15_08, biquad->band5.coef_a1.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND5_A1_07_00, biquad->band5.coef_a1.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND5_A2_23_16, biquad->band5.coef_a2.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND5_A2_15_08, biquad->band5.coef_a2.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND5_A2_07_00, biquad->band5.coef_a2.bit.lsb0_7);

    /* BAND 6 */
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND6_B0_23_16, biquad->band6.coef_b0.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND6_B0_15_08, biquad->band6.coef_b0.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND6_B0_07_00, biquad->band6.coef_b0.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND6_B1_23_16, biquad->band6.coef_b1.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND6_B1_15_08, biquad->band6.coef_b1.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND6_B1_07_00, biquad->band6.coef_b1.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND6_B2_23_16, biquad->band6.coef_b2.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND6_B2_15_08, biquad->band6.coef_b2.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND6_B2_07_00, biquad->band6.coef_b2.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND6_A1_23_16, biquad->band6.coef_a1.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND6_A1_15_08, biquad->band6.coef_a1.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND6_A1_07_00, biquad->band6.coef_a1.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND6_A2_23_16, biquad->band6.coef_a2.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND6_A2_15_08, biquad->band6.coef_a2.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND6_A2_07_00, biquad->band6.coef_a2.bit.lsb0_7);

    /* BAND 7 */
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND7_B0_23_16, biquad->band7.coef_b0.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND7_B0_15_08, biquad->band7.coef_b0.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND7_B0_07_00, biquad->band7.coef_b0.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND7_B1_23_16, biquad->band7.coef_b1.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND7_B1_15_08, biquad->band7.coef_b1.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND7_B1_07_00, biquad->band7.coef_b1.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND7_B2_23_16, biquad->band7.coef_b2.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND7_B2_15_08, biquad->band7.coef_b2.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND7_B2_07_00, biquad->band7.coef_b2.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND7_A1_23_16, biquad->band7.coef_a1.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND7_A1_15_08, biquad->band7.coef_a1.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND7_A1_07_00, biquad->band7.coef_a1.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND7_A2_23_16, biquad->band7.coef_a2.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND7_A2_15_08, biquad->band7.coef_a2.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_EQ_BAND7_A2_07_00, biquad->band7.coef_a2.bit.lsb0_7);

    /* RECORD */
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_REC_BIQ_B0_23_16, biquad->record.coef_b0.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_REC_BIQ_B0_15_08, biquad->record.coef_b0.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_REC_BIQ_B0_07_00, biquad->record.coef_b0.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_REC_BIQ_B1_23_16, biquad->record.coef_b1.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_REC_BIQ_B1_15_08, biquad->record.coef_b1.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_REC_BIQ_B1_07_00, biquad->record.coef_b1.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_REC_BIQ_B2_23_16, biquad->record.coef_b2.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_REC_BIQ_B2_15_08, biquad->record.coef_b2.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_REC_BIQ_B2_07_00, biquad->record.coef_b2.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_REC_BIQ_A1_23_16, biquad->record.coef_a1.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_REC_BIQ_A1_15_08, biquad->record.coef_a1.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_REC_BIQ_A1_07_00, biquad->record.coef_a1.bit.lsb0_7);

    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_REC_BIQ_A2_23_16, biquad->record.coef_a2.bit.msb16_23);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_REC_BIQ_A2_15_08, biquad->record.coef_a2.bit.lsb8_15);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_REC_BIQ_A2_07_00, biquad->record.coef_a2.bit.lsb0_7);
}

/** @brief Activate the playback filtes in the codec.
 *
 *  @param[in] i2c_hal    Hardware Abstraction Layer structure i2c_hal.
 *  @param[in] filter_en  Register structure for the codec.
 */
static void activate_playback_filters(max98091_i2c_hal_t *i2c_hal, max98091_playback_filter_t filter_en)
{
    max98091_reg_dsp_filter_en_t biquad_enable = {0};

    i2c_hal->read(i2c_hal->i2c_addr, MAX98091_REG_DSP_FILTER_ENABLE, &biquad_enable.reg);
    switch (filter_en) {
    case MAX98091_PLAYBACK_FILTER_3_BANDS:
        /* 3 Bands */
        biquad_enable.bit.eq3banden = 1;
        biquad_enable.bit.eq5banden = 0;
        biquad_enable.bit.eq7banden = 0;
        break;
    case MAX98091_PLAYBACK_FILTER_5_BANDS:
        /* 5 Bands */
        biquad_enable.bit.eq3banden = 0;
        biquad_enable.bit.eq5banden = 1;
        biquad_enable.bit.eq7banden = 0;
        break;
    case MAX98091_PLAYBACK_FILTER_7_BANDS:
        /* 7 Bands */
        biquad_enable.bit.eq3banden = 0;
        biquad_enable.bit.eq5banden = 0;
        biquad_enable.bit.eq7banden = 1;
        break;
    default:
        /* No filters */
        biquad_enable.bit.eq3banden = 0;
        biquad_enable.bit.eq5banden = 0;
        biquad_enable.bit.eq7banden = 0;
        break;
    }
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_DSP_FILTER_ENABLE, biquad_enable.reg);
}

/** @brief Activate the recording filtes in the codec.
 *
 *  @param[in] i2c_hal    Hardware Abstraction Layer structure i2c_hal.
 *  @param[in] filter_en  Register structure for the codec.
 */
static void activate_record_filter(max98091_i2c_hal_t *i2c_hal, bool filter_en)
{
    max98091_reg_dsp_filter_en_t biquad_enable = {0};

    i2c_hal->read(i2c_hal->i2c_addr, MAX98091_REG_DSP_FILTER_ENABLE, &biquad_enable.reg);
    if (filter_en) {
        biquad_enable.bit.recbqen = 1;
    } else {
        biquad_enable.bit.recbqen = 0;
    }
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_DSP_FILTER_ENABLE, biquad_enable.reg);
}

/** @brief Configure the headphone volume control register and gain.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
static void configure_headphone_vol(max98091_i2c_hal_t *i2c_hal)
{
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_HP_CONTROL, reg_map.gain_set.hp_ctrl.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_LEFT_HP_VOLUME, reg_map.gain_set.left_hp_volume.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_RIGHT_HP_VOLUME, reg_map.gain_set.right_hp_volume.reg);
}

/** @brief Configure the speaker volume control register and gain.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
static void configure_speaker_vol(max98091_i2c_hal_t *i2c_hal)
{
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_SPK_CONTROL, reg_map.gain_set.spk_ctrl.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_LEFT_SPK_VOLUME, reg_map.gain_set.left_spk_volume.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_RIGHT_SPK_VOLUME, reg_map.gain_set.right_spk_volume.reg);
}

/** @brief Configure the lineout volume control register and gain.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
static void configure_lineout_vol(max98091_i2c_hal_t *i2c_hal)
{
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_RCV_LOUTL_CONTROL, reg_map.gain_set.rcv_loutl_ctrl.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_RCV_LOUTL_VOLUME, reg_map.gain_set.rcv_loutl_volume.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_LOUTR_CONTROL, reg_map.gain_set.loutr_ctrl.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_LOUTR_VOLUME, reg_map.gain_set.loutr_volume.reg);
}

/** @brief Configure quick configuration mode.
 *
 *  This enables/disables fine control over clocks configuration.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
static void configure_quick_setup_configuration(max98091_i2c_hal_t *i2c_hal)
{
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_QS_SYSTEM_CLOCK, reg_map.qs_sys.reg);
    i2c_hal->write(i2c_hal->i2c_addr, MAX98091_REG_QS_SAMPLE_RATE, reg_map.qs_sample_rate.reg);
}

/** @brief Return the jack status.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 *  @return jack status enum.
 */
static jack_state_t get_jack_status(max98091_i2c_hal_t *i2c_hal)
{
    uint8_t status = 0;

    i2c_hal->read(i2c_hal->i2c_addr, MAX98091_REG_JACK_STATUS, &status);

    return (jack_state_t)status;
}
