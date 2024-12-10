/** @file  max98091_reg.h
 *  @brief Register definition for the max98091 codec.
 *
 *  @copyright Copyright (C) 2018-2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef MAX98091_REG_H_
#define MAX98091_REG_H_

/* INCLUDES *******************************************************************/
#include <stdint.h>

/* CONSTANTS ******************************************************************/
/* REGISTERS ADDRESS */
#define MAX98091_REG_SOFTWARE_RESET       0x00
#define MAX98091_REG_DEVICE_STATUS        0x01
#define MAX98091_REG_JACK_STATUS          0x02
#define MAX98091_REG_INTERRUPT_MASKS      0x03
/* QUICK SETUP REG */
#define MAX98091_REG_QS_SYSTEM_CLOCK      0x04
#define MAX98091_REG_QS_SAMPLE_RATE       0x05
#define MAX98091_REG_QS_DAI_INTERFACE     0x06
#define MAX98091_REG_QS_DAC_PATH          0x07
#define MAX98091_REG_QS_MIC_DIRECT_TO_ADC 0x08
#define MAX98091_REG_QS_LINE_TO_ADC       0x09
#define MAX98091_REG_QS_ANALOG_MIC_LOOP   0x0A
#define MAX98091_REG_QS_ANALOG_LINE_LOOP  0x0B
/* ANALOG IN REG */
#define MAX98091_REG_LINE_INPUT_CONFIG    0x0D
#define MAX98091_REG_LINE_INPUT_LEVEL     0x0E
#define MAX98091_REG_INPUT_MODE           0x0F
#define MAX98091_REG_MIC1_INPUT_LEVEL     0x10
#define MAX98091_REG_MIC2_INPUT_LEVEL     0x11
/* MIC CONFIG */
#define MAX98091_REG_MIC_BIAS_VOLTAGE     0x12
#define MAX98091_REG_DIGITAL_MIC_ENABLE   0x13
#define MAX98091_REG_DIGITAL_MIC_CONFIG   0x14
/* ADC PATH CONFIG REG */
#define MAX98091_REG_LEFT_ADC_MIXER       0x15
#define MAX98091_REG_RIGHT_ADC_MIXER      0x16
#define MAX98091_REG_LEFT_REC_LEVEL       0x17
#define MAX98091_REG_RIGHT_REC_LEVEL      0x18
#define MAX98091_REG_REC_BIQUAD_LEVEL     0x19
#define MAX98091_REG_REC_SIDETONE         0x1A
/* CLOCK CONFIG REG */
#define MAX98091_REG_SYSTEM_CLOCK         0x1B
#define MAX98091_REG_CLOCK_MODE           0x1C
#define MAX98091_REG_CLOCK_RATIO_NI_MSB   0x1D
#define MAX98091_REG_CLOCK_RATIO_NI_LSB   0x1E
#define MAX98091_REG_CLOCK_RATIO_MI_MSB   0x1F
#define MAX98091_REG_CLOCK_RATIO_MI_LSB   0x20
#define MAX98091_REG_MASTER_MODE          0x21
/* INTERFACE CTRL REG */
#define MAX98091_REG_INTERFACE_FORMAT     0x22
#define MAX98091_REG_TDM_CONTROL          0x23
#define MAX98091_REG_TDM_FORMAT           0x24
#define MAX98091_REG_IO_CONFIGURATION     0x25
#define MAX98091_REG_FILTER_CONFIG        0x26
#define MAX98091_REG_DAI_PLAYBACK_LEVEL   0x27
#define MAX98091_REG_EQ_PLAYBACK_LEVEL    0x28
/* HEADPHONE CTRL REG */
#define MAX98091_REG_LEFT_HP_MIXER        0x29
#define MAX98091_REG_RIGHT_HP_MIXER       0x2A
#define MAX98091_REG_HP_CONTROL           0x2B
#define MAX98091_REG_LEFT_HP_VOLUME       0x2C
#define MAX98091_REG_RIGHT_HP_VOLUME      0x2D
/* SPEAKER CONFIG REG */
#define MAX98091_REG_LEFT_SPK_MIXER       0x2E
#define MAX98091_REG_RIGHT_SPK_MIXER      0x2F
#define MAX98091_REG_SPK_CONTROL          0x30
#define MAX98091_REG_LEFT_SPK_VOLUME      0x31
#define MAX98091_REG_RIGHT_SPK_VOLUME     0x32
/* DYNAMIC RANGE CONTROL CONFIG REG */
#define MAX98091_REG_DRC_TIMING           0x33
#define MAX98091_REG_DRC_COMPRESSOR       0x34
#define MAX98091_REG_DRC_EXPANDER         0x35
#define MAX98091_REG_DRC_GAIN             0x36
/* RECEIVER AND LINE OUTPUT REG */
#define MAX98091_REG_RCV_LOUTL_MIXER      0x37
#define MAX98091_REG_RCV_LOUTL_CONTROL    0x38
#define MAX98091_REG_RCV_LOUTL_VOLUME     0x39
#define MAX98091_REG_LOUTR_MIXER          0x3A
#define MAX98091_REG_LOUTR_CONTROL        0x3B
#define MAX98091_REG_LOUTR_VOLUME         0x3C
/* JACK DETECT AND ENABLE REG */
#define MAX98091_REG_JACK_DETECT          0x3D
#define MAX98091_REG_INPUT_ENABLE         0x3E
#define MAX98091_REG_OUTPUT_ENABLE        0x3F
#define MAX98091_REG_LEVEL_CONTROL        0x40
#define MAX98091_REG_DSP_FILTER_ENABLE    0x41
/* BIAS AND POWER MODE CONFIGURATION REG */
#define MAX98091_REG_BIAS_CONTROL         0x42
#define MAX98091_REG_DAC_CONTROL          0x43
#define MAX98091_REG_ADC_CONTROL          0x44
#define MAX98091_REG_DEVICE_SHUTDOWN      0x45
/* PLAYBACK PARAMETRIC EQUALIZER : BIQUAD FILTER COEFFICIENT REG */
/* BAND 1 */
#define MAX98091_REG_EQ_BAND1_B0_23_16    0x46
#define MAX98091_REG_EQ_BAND1_B0_15_08    0x47
#define MAX98091_REG_EQ_BAND1_B0_07_00    0x48
#define MAX98091_REG_EQ_BAND1_B1_23_16    0x49
#define MAX98091_REG_EQ_BAND1_B1_15_08    0x4A
#define MAX98091_REG_EQ_BAND1_B1_07_00    0x4B
#define MAX98091_REG_EQ_BAND1_B2_23_16    0x4C
#define MAX98091_REG_EQ_BAND1_B2_15_08    0x4D
#define MAX98091_REG_EQ_BAND1_B2_07_00    0x4E
#define MAX98091_REG_EQ_BAND1_A1_23_16    0x4F
#define MAX98091_REG_EQ_BAND1_A1_15_08    0x50
#define MAX98091_REG_EQ_BAND1_A1_07_00    0x51
#define MAX98091_REG_EQ_BAND1_A2_23_16    0x52
#define MAX98091_REG_EQ_BAND1_A2_15_08    0x53
#define MAX98091_REG_EQ_BAND1_A2_07_00    0x54
/* BAND 2 */
#define MAX98091_REG_EQ_BAND2_B0_23_16    0x55
#define MAX98091_REG_EQ_BAND2_B0_15_08    0x56
#define MAX98091_REG_EQ_BAND2_B0_07_00    0x57
#define MAX98091_REG_EQ_BAND2_B1_23_16    0x58
#define MAX98091_REG_EQ_BAND2_B1_15_08    0x59
#define MAX98091_REG_EQ_BAND2_B1_07_00    0x5A
#define MAX98091_REG_EQ_BAND2_B2_23_16    0x5B
#define MAX98091_REG_EQ_BAND2_B2_15_08    0x5C
#define MAX98091_REG_EQ_BAND2_B2_07_00    0x5D
#define MAX98091_REG_EQ_BAND2_A1_23_16    0x5E
#define MAX98091_REG_EQ_BAND2_A1_15_08    0x5F
#define MAX98091_REG_EQ_BAND2_A1_07_00    0x60
#define MAX98091_REG_EQ_BAND2_A2_23_16    0x61
#define MAX98091_REG_EQ_BAND2_A2_15_08    0x62
#define MAX98091_REG_EQ_BAND2_A2_07_00    0x63
/* BAND 3 */
#define MAX98091_REG_EQ_BAND3_B0_23_16    0x64
#define MAX98091_REG_EQ_BAND3_B0_15_08    0x65
#define MAX98091_REG_EQ_BAND3_B0_07_00    0x66
#define MAX98091_REG_EQ_BAND3_B1_23_16    0x67
#define MAX98091_REG_EQ_BAND3_B1_15_08    0x68
#define MAX98091_REG_EQ_BAND3_B1_07_00    0x69
#define MAX98091_REG_EQ_BAND3_B2_23_16    0x6A
#define MAX98091_REG_EQ_BAND3_B2_15_08    0x6B
#define MAX98091_REG_EQ_BAND3_B2_07_00    0x6C
#define MAX98091_REG_EQ_BAND3_A1_23_16    0x6D
#define MAX98091_REG_EQ_BAND3_A1_15_08    0x6E
#define MAX98091_REG_EQ_BAND3_A1_07_00    0x6F
#define MAX98091_REG_EQ_BAND3_A2_23_16    0x70
#define MAX98091_REG_EQ_BAND3_A2_15_08    0x71
#define MAX98091_REG_EQ_BAND3_A2_07_00    0x72
/* BAND 4 */
#define MAX98091_REG_EQ_BAND4_B0_23_16    0x73
#define MAX98091_REG_EQ_BAND4_B0_15_08    0x74
#define MAX98091_REG_EQ_BAND4_B0_07_00    0x75
#define MAX98091_REG_EQ_BAND4_B1_23_16    0x76
#define MAX98091_REG_EQ_BAND4_B1_15_08    0x77
#define MAX98091_REG_EQ_BAND4_B1_07_00    0x78
#define MAX98091_REG_EQ_BAND4_B2_23_16    0x79
#define MAX98091_REG_EQ_BAND4_B2_15_08    0x7A
#define MAX98091_REG_EQ_BAND4_B2_07_00    0x7B
#define MAX98091_REG_EQ_BAND4_A1_23_16    0x7C
#define MAX98091_REG_EQ_BAND4_A1_15_08    0x7D
#define MAX98091_REG_EQ_BAND4_A1_07_00    0x7E
#define MAX98091_REG_EQ_BAND4_A2_23_16    0x7F
#define MAX98091_REG_EQ_BAND4_A2_15_08    0x80
#define MAX98091_REG_EQ_BAND4_A2_07_00    0x81
/* BAND 5 */
#define MAX98091_REG_EQ_BAND5_B0_23_16    0x82
#define MAX98091_REG_EQ_BAND5_B0_15_08    0x83
#define MAX98091_REG_EQ_BAND5_B0_07_00    0x84
#define MAX98091_REG_EQ_BAND5_B1_23_16    0x85
#define MAX98091_REG_EQ_BAND5_B1_15_08    0x86
#define MAX98091_REG_EQ_BAND5_B1_07_00    0x87
#define MAX98091_REG_EQ_BAND5_B2_23_16    0x88
#define MAX98091_REG_EQ_BAND5_B2_15_08    0x89
#define MAX98091_REG_EQ_BAND5_B2_07_00    0x8A
#define MAX98091_REG_EQ_BAND5_A1_23_16    0x8B
#define MAX98091_REG_EQ_BAND5_A1_15_08    0x8C
#define MAX98091_REG_EQ_BAND5_A1_07_00    0x8D
#define MAX98091_REG_EQ_BAND5_A2_23_16    0x8E
#define MAX98091_REG_EQ_BAND5_A2_15_08    0x8F
#define MAX98091_REG_EQ_BAND5_A2_07_00    0x90
/* BAND 6 */
#define MAX98091_REG_EQ_BAND6_B0_23_16    0x91
#define MAX98091_REG_EQ_BAND6_B0_15_08    0x92
#define MAX98091_REG_EQ_BAND6_B0_07_00    0x93
#define MAX98091_REG_EQ_BAND6_B1_23_16    0x94
#define MAX98091_REG_EQ_BAND6_B1_15_08    0x95
#define MAX98091_REG_EQ_BAND6_B1_07_00    0x96
#define MAX98091_REG_EQ_BAND6_B2_23_16    0x97
#define MAX98091_REG_EQ_BAND6_B2_15_08    0x98
#define MAX98091_REG_EQ_BAND6_B2_07_00    0x99
#define MAX98091_REG_EQ_BAND6_A1_23_16    0x9A
#define MAX98091_REG_EQ_BAND6_A1_15_08    0x9B
#define MAX98091_REG_EQ_BAND6_A1_07_00    0x9C
#define MAX98091_REG_EQ_BAND6_A2_23_16    0x9D
#define MAX98091_REG_EQ_BAND6_A2_15_08    0x9E
#define MAX98091_REG_EQ_BAND6_A2_07_00    0x9F
/* BAND 7 */
#define MAX98091_REG_EQ_BAND7_B0_23_16    0xA0
#define MAX98091_REG_EQ_BAND7_B0_15_08    0xA1
#define MAX98091_REG_EQ_BAND7_B0_07_00    0xA2
#define MAX98091_REG_EQ_BAND7_B1_23_16    0xA3
#define MAX98091_REG_EQ_BAND7_B1_15_08    0xA4
#define MAX98091_REG_EQ_BAND7_B1_07_00    0xA5
#define MAX98091_REG_EQ_BAND7_B2_23_16    0xA6
#define MAX98091_REG_EQ_BAND7_B2_15_08    0xA7
#define MAX98091_REG_EQ_BAND7_B2_07_00    0xA8
#define MAX98091_REG_EQ_BAND7_A1_23_16    0xA9
#define MAX98091_REG_EQ_BAND7_A1_15_08    0xAA
#define MAX98091_REG_EQ_BAND7_A1_07_00    0xAB
#define MAX98091_REG_EQ_BAND7_A2_23_16    0xAC
#define MAX98091_REG_EQ_BAND7_A2_15_08    0xAD
#define MAX98091_REG_EQ_BAND7_A2_07_00    0xAE
/* RECORD BIQUAD FILTER COEFFICIENT REG */
#define MAX98091_REG_REC_BIQ_B0_23_16     0xAF
#define MAX98091_REG_REC_BIQ_B0_15_08     0xB0
#define MAX98091_REG_REC_BIQ_B0_07_00     0xB1
#define MAX98091_REG_REC_BIQ_B1_23_16     0xB2
#define MAX98091_REG_REC_BIQ_B1_15_08     0xB3
#define MAX98091_REG_REC_BIQ_B1_07_00     0xB4
#define MAX98091_REG_REC_BIQ_B2_23_16     0xB5
#define MAX98091_REG_REC_BIQ_B2_15_08     0xB6
#define MAX98091_REG_REC_BIQ_B2_07_00     0xB7
#define MAX98091_REG_REC_BIQ_A1_23_16     0xB8
#define MAX98091_REG_REC_BIQ_A1_15_08     0xB9
#define MAX98091_REG_REC_BIQ_A1_07_00     0xBA
#define MAX98091_REG_REC_BIQ_A2_23_16     0xBB
#define MAX98091_REG_REC_BIQ_A2_15_08     0xBC
#define MAX98091_REG_REC_BIQ_A2_07_00     0xBD
/* REVISION ID REG */
#define MAX98091_REG_REVISION_ID          0xFF

/* REGISTER VALUES */
/* SHUTDOWN */
#define MAX98091_DEV_SHUTDOWN             0x00
#define MAX98091_DEV_PWRUP                0x01
/* GAINS */
#define MAX98091_MIXER_GAIN_0DB           0x00
#define MAX98091_MIXER_GAIN_N6DB          0x01
#define MAX98091_MIXER_GAIN_N9_5DB        0x02
#define MAX98091_MIXER_GAIN_N12DB         0x03
/* HEADPHONES VOLUME */
#define MAX98091_HP_MIN_VOLUME            MAX98091_HP_VOL_M67DB
/* Real maximum is at 3dB, -7dB maximum is to avoid white noise */
#define MAX98091_HP_MAX_VOLUME            MAX98091_HP_VOL_M7DB
/* SPEAKER VOLUME */
#define MAX98091_SPK_MIN_VOLUME           0x18 /*+14dB*/
#define MAX98091_SPK_MAX_VOLUME           0x3F /*-48dB*/
/* LINEOUT VOLUME */
#define MAX98091_LOUT_MIN_VOLUME          MAX98091_LOUT_VOL_M62DB
#define MAX98091_LOUT_MAX_VOLUME          MAX98091_LOUT_VOL_8DB
/* MUTE */
#define MAX98091_UNMUTE                   0x00
#define MAX98091_MUTE                     0x01

/* TYPES **********************************************************************/
/* HEADPHONE VOLUME */
typedef enum max98091_hp_vol {
   MAX98091_HP_VOL_M67DB,
   MAX98091_HP_VOL_M63DB,
   MAX98091_HP_VOL_M59DB,
   MAX98091_HP_VOL_M55DB,
   MAX98091_HP_VOL_M51DB,
   MAX98091_HP_VOL_M47DB,
   MAX98091_HP_VOL_M43DB,
   MAX98091_HP_VOL_M40DB,
   MAX98091_HP_VOL_M37DB,
   MAX98091_HP_VOL_M34DB,
   MAX98091_HP_VOL_M31DB,
   MAX98091_HP_VOL_M28DB,
   MAX98091_HP_VOL_M25DB,
   MAX98091_HP_VOL_M22DB,
   MAX98091_HP_VOL_M19DB,
   MAX98091_HP_VOL_M17DB,
   MAX98091_HP_VOL_M15DB,
   MAX98091_HP_VOL_M13DB,
   MAX98091_HP_VOL_M11DB,
   MAX98091_HP_VOL_M9DB,
   MAX98091_HP_VOL_M7DB,
   MAX98091_HP_VOL_M5DB,
   MAX98091_HP_VOL_M4DB,
   MAX98091_HP_VOL_M3DB,
   MAX98091_HP_VOL_M2DB,
   MAX98091_HP_VOL_M1DB,
   MAX98091_HP_VOL_0DB,
   MAX98091_HP_VOL_1DB,
   MAX98091_HP_VOL_1_5DB,
   MAX98091_HP_VOL_2DB,
   MAX98091_HP_VOL_2_5DB,
   MAX98091_HP_VOL_3DB,
} max98091_hp_vol_t;

/* LINEOUT VOLUME */
typedef enum max98091_lout_vol {
  MAX98091_LOUT_VOL_M62DB,
  MAX98091_LOUT_VOL_M58DB,
  MAX98091_LOUT_VOL_M54DB,
  MAX98091_LOUT_VOL_M50DB,
  MAX98091_LOUT_VOL_M46DB,
  MAX98091_LOUT_VOL_M42DB,
  MAX98091_LOUT_VOL_M38DB,
  MAX98091_LOUT_VOL_M35DB,
  MAX98091_LOUT_VOL_M32DB,
  MAX98091_LOUT_VOL_M29DB,
  MAX98091_LOUT_VOL_M26DB,
  MAX98091_LOUT_VOL_M23DB,
  MAX98091_LOUT_VOL_M20DB,
  MAX98091_LOUT_VOL_M17DB,
  MAX98091_LOUT_VOL_M14DB,
  MAX98091_LOUT_VOL_M12DB,
  MAX98091_LOUT_VOL_M10DB,
  MAX98091_LOUT_VOL_M8DB,
  MAX98091_LOUT_VOL_M6DB,
  MAX98091_LOUT_VOL_M4DB,
  MAX98091_LOUT_VOL_M2DB,
  MAX98091_LOUT_VOL_0DB,
  MAX98091_LOUT_VOL_1DB,
  MAX98091_LOUT_VOL_2DB,
  MAX98091_LOUT_VOL_3DB,
  MAX98091_LOUT_VOL_4DB,
  MAX98091_LOUT_VOL_5DB,
  MAX98091_LOUT_VOL_6DB,
  MAX98091_LOUT_VOL_6_5DB,
  MAX98091_LOUT_VOL_7DB,
  MAX98091_LOUT_VOL_7_5DB,
  MAX98091_LOUT_VOL_8DB,
} max98091_lout_vol_t;

/* PLAYBACK FILTYERS */
typedef enum max98091_playback_filter {
    MAX98091_PLAYBACK_NO_FILTER,
    MAX98091_PLAYBACK_FILTER_3_BANDS,
    MAX98091_PLAYBACK_FILTER_5_BANDS,
    MAX98091_PLAYBACK_FILTER_7_BANDS,
} max98091_playback_filter_t;

typedef union max98091_reg_soft_reset {
    struct {
        uint8_t unused:7;
        uint8_t swreset:1;
    } bit;
    uint8_t reg;
} max98091_reg_soft_reset_t;

typedef union max98091_reg_dev_status {
    struct {
        uint8_t drcclp:1;
        uint8_t drcact:1;
        uint8_t jdet:1;
        uint8_t unused:2;
        uint8_t ulk:1;
        uint8_t sld:1;
        uint8_t cld:1;
    } bit;
    uint8_t reg;
} max98091_reg_dev_status_t;

typedef union max98091_reg_jack_status {
    struct {
        uint8_t unused:1;
        uint8_t jksns:1;
        uint8_t lsns:1;
        uint8_t unused2:5;
    } bit;
    uint8_t reg;
} max98091_reg_jack_status_t;

typedef union max98091_reg_interrupt_mask {
    struct {
        uint8_t idrcclp:1;
        uint8_t idrcact:1;
        uint8_t ijdet:1;
        uint8_t unused:2;
        uint8_t iulk:1;
        uint8_t isld:1;
        uint8_t icld:1;
    } bit;
    uint8_t reg;
} max98091_reg_interrupt_mask_t;

/*QUICK SETUP REG*/
typedef union max98091_reg_qs_sys_clk {
    struct {
        uint8_t _256fs:1;
        uint8_t unused:1;
        uint8_t _11p2896m:1;
        uint8_t _12m:1;
        uint8_t _12p288m:1;
        uint8_t _13m:1;
        uint8_t _19p2m:1;
        uint8_t _26m:1;
    } bit;
    uint8_t reg;
} max98091_reg_qs_sys_clk_t;

typedef union max98091_reg_qs_sample_rate {
    struct {
        uint8_t sr_8k:1;
        uint8_t sr_16k:1;
        uint8_t sr_44k1:1;
        uint8_t sr_48k:1;
        uint8_t sr_32k:1;
        uint8_t sr_96k:1;
        uint8_t unused:2;
    } bit;
    uint8_t reg;
} max98091_reg_qs_sample_rate_t;

typedef union max98091_reg_qs_dai_interface {
    struct {
        uint8_t i2s_s:1;
        uint8_t i2s_m:1;
        uint8_t lj_s:1;
        uint8_t lj_m:1;
        uint8_t rj_s:1;
        uint8_t rj_m:1;
        uint8_t unused:2;
    } bit;
    uint8_t reg;
} max98091_reg_qs_dai_interface_t;

typedef union max98091_reg_qs_dac_path {
    struct {
        uint8_t unused:4;
        uint8_t dig2_lout:1;
        uint8_t dig2_spk:1;
        uint8_t dig2_ear:1;
        uint8_t dig2_hp:1;
    } bit;
    uint8_t reg;
} max98091_reg_qs_dac_path_t;

typedef union max98091_reg_qs_mic_direct_adc {
    struct {
        uint8_t unused1:1;
        uint8_t in56_dadc:1;
        uint8_t in34_dadc:1;
        uint8_t in12_dadc:1;
        uint8_t unused2:2;
        uint8_t in34_mic2:1;
        uint8_t in12_mic1:1;
    } bit;
    uint8_t reg;
} max98091_reg_qs_mic_direct_adc_t;

typedef union max98091_reg_qs_line_to_adc {
    struct {
        uint8_t unused:3;
        uint8_t in65d_b:1;
        uint8_t in34d_a:1;
        uint8_t in56s_ab:1;
        uint8_t in34s_ab:1;
        uint8_t in12s_ab:1;
    } bit;
    uint8_t reg;
} max98091_reg_qs_line_to_adc_t;

typedef union max98091_reg_qs_analog_mic_loop {
    struct {
        uint8_t in34_m2loutr:1;
        uint8_t in34_m2ear:1;
        uint8_t in34_m2spkr:1;
        uint8_t in34_m2hpr:1;
        uint8_t in12_m1loutl:1;
        uint8_t in12_m1ear:1;
        uint8_t in12_m1spkl:1;
        uint8_t in12_m1hpl:1;
    } bit;
    uint8_t reg;
} max98091_reg_qs_analog_mic_loop_t;

typedef union max98091_reg_qs_analog_line_loop {
    struct {
        uint8_t in34s_ablout:1;
        uint8_t in65d_bear:1;
        uint8_t in65d_bspkr:1;
        uint8_t in34s_abhp:1;
        uint8_t in12s_ablout:1;
        uint8_t in34d_aear:1;
        uint8_t in34d_aspkl:1;
        uint8_t in12s_abhp:1;
    } bit;
    uint8_t reg;
} max98091_reg_qs_analog_line_loop_t;

/* ANALOG IN REG */
typedef union max98091_reg_line_in_config {
    struct {
        uint8_t in6seen:1;
        uint8_t in5seen:1;
        uint8_t in4seen:1;
        uint8_t in3seen:1;
        uint8_t in2seen:1;
        uint8_t in1seen:1;
        uint8_t in65diff:1;
        uint8_t in34diff:1;
    } bit;
    uint8_t reg;
} max98091_reg_line_in_config_t;

typedef union max98091_reg_line_in_lvl {
    struct {
        uint8_t linbpga:3;
        uint8_t linapga:3;
        uint8_t mixg246:1;
        uint8_t mixg135:1;
    } bit;
    uint8_t reg;
} max98091_reg_line_in_lvl_t;

typedef union max98091_reg_in_mode {
    struct {
        uint8_t ext_mic:2;
        uint8_t unused:4;
        uint8_t extbufb:1;
        uint8_t extbufa:1;
    } bit;
    uint8_t reg;
} max98091_reg_in_mode_t;

typedef union max98091_reg_mic1_in_lvl {
    struct {
        uint8_t pgam1:5;
        uint8_t pa1en:2;
        uint8_t unused:1;
    } bit;
    uint8_t reg;
} max98091_reg_mic1_in_lvl_t;

typedef union max98091_reg_mic2_in_lvl {
    struct {
        uint8_t pgam2:5;
        uint8_t pa2en:2;
        uint8_t unused:1;
    } bit;
    uint8_t reg;
} max98091_reg_mic2_in_lvl_t;

/* MIC CONFIG */
typedef union max98091_reg_mic_bias_voltage {
    struct {
        uint8_t mbvsel:2;
        uint8_t unused:6;
    } bit;
    uint8_t reg;
} max98091_reg_mic_bias_voltage_t;

typedef union max98091_reg_dig_mic_en {
    struct {
        uint8_t digmicl:1;
        uint8_t digmicr:1;
        uint8_t unused1:2;
        uint8_t micclk:3;
        uint8_t unused2:1;
    } bit;
    uint8_t reg;
} max98091_reg_dig_mic_en_t;

typedef union max98091_reg_dig_mic_config {
    struct {
        uint8_t dmic_freq:2;
        uint8_t unused:2;
        uint8_t dmic_comp:4;
    } bit;
    uint8_t reg;
} max98091_reg_dig_mic_config_t;

/* ADC PATH CONFIG REG */
typedef union max98091_reg_left_adc_mixer {
    struct {
        uint8_t mixadl:7;
        uint8_t unused:1;
    } bit;
    uint8_t reg;
} max98091_reg_left_adc_mixer_t;

typedef union max98091_reg_right_adc_mixer {
    struct {
        uint8_t mixadr:7;
        uint8_t unused:1;
    } bit;
    uint8_t reg;
} max98091_reg_right_adc_mixer_t;

typedef union max98091_reg_left_rec_lvl {
    struct {
        uint8_t avl:4;
        uint8_t avlg:3;
        uint8_t unused:1;
    } bit;
    uint8_t reg;
} max98091_reg_left_rec_lvl_t;

typedef union max98091_reg_right_rec_lvl {
    struct {
        uint8_t avr:4;
        uint8_t avrg:3;
        uint8_t unused:1;
    } bit;
    uint8_t reg;
} max98091_reg_right_rec_lvl_t;

typedef union max98091_reg_rec_biq_lvl {
    struct {
        uint8_t avbq:4;
        uint8_t unused:4;
    } bit;
    uint8_t reg;
} max98091_reg_rec_biq_lvl_t;

typedef union max98091_reg_rec_sidestone {
    struct {
        uint8_t dvst:5;
        uint8_t unused:1;
        uint8_t dsts:2;
    } bit;
    uint8_t reg;
} max98091_reg_rec_sidestone_t;

/* CLOCK CONFIG REG */
typedef union max98091_reg_sys_clk {
    struct {
        uint8_t unused1:4;
        uint8_t psclk:2;
        uint8_t unused2:2;
    } bit;
    uint8_t reg;
} max98091_reg_sys_clk_t;

typedef union max98091_reg_clk_mode {
    struct {
        uint8_t use_mi:1;
        uint8_t unused:3;
        uint8_t freq:4;
    } bit;
    uint8_t reg;
} max98091_reg_clk_mode_t;

typedef union max98091_reg_clk_ratio_ni_msb {
    struct {
        uint8_t ni:7;
        uint8_t unused:1;
    } bit;
    uint8_t reg;
} max98091_reg_clk_ratio_ni_msb_t;

typedef union max98091_reg_clk_ratio_ni_lsb {
    struct {
        uint8_t ni:8;
    } bit;
    uint8_t reg;
} max98091_reg_clk_ratio_ni_lsb_t;

typedef union max98091_reg_clk_ratio_mi_msb {
    struct {
        uint8_t mi:8;
    } bit;
    uint8_t reg;
} max98091_reg_clk_ratio_mi_msb_t;

typedef union max98091_reg_clk_ratio_mi_lsb {
    struct {
        uint8_t mi:8;
    } bit;
    uint8_t reg;
} max98091_reg_clk_ratio_mi_lsb_t;

typedef union max98091_reg_master_mode {
    struct {
        uint8_t bsel:3;
        uint8_t unused:4;
        uint8_t mas:1;
    } bit;
    uint8_t reg;
} max98091_reg_master_mode_t;

/* INTERFACE CTRL REG */
typedef union max98091_reg_interface_format {
    struct {
        uint8_t ws:2;
        uint8_t dly:1;
        uint8_t bci:1;
        uint8_t wci:1;
        uint8_t rj:1;
        uint8_t unused:2;
    } bit;
    uint8_t reg;
} max98091_reg_interface_format_t;

typedef union max98091_reg_tdm_ctrl {
    struct {
        uint8_t tmd:1;
        uint8_t fsw:1;
        uint8_t unused:6;
    } bit;
    uint8_t reg;
} max98091_reg_tdm_ctrl_t;

typedef union max98091_reg_tdm_format {
    struct {
        uint8_t slotdly:4;
        uint8_t slotr:2;
        uint8_t slotl:2;
    } bit;
    uint8_t reg;
} max98091_reg_tdm_format_t;

typedef union max98091_reg_io_config {
    struct {
        uint8_t sdien:1;
        uint8_t sdoen:1;
        uint8_t hizoff:1;
        uint8_t dmono:1;
        uint8_t lben:1;
        uint8_t lten:1;
        uint8_t unused:2;
    } bit;
    uint8_t reg;
} max98091_reg_io_config_t;

typedef union max98091_reg_filter_config {
    struct {
        uint8_t unused:4;
        uint8_t dhf:1;
        uint8_t dhpf:1;
        uint8_t ahpf:1;
        uint8_t mode:1;
    } bit;
    uint8_t reg;
} max98091_reg_filter_config_t;

typedef union max98091_reg_dai_playback_lvl {
    struct {
        uint8_t dv:4;
        uint8_t dvg:2;
        uint8_t unused:1;
        uint8_t dvm:1;
    } bit;
    uint8_t reg;
} max98091_reg_dai_playback_lvl_t;

typedef union max98091_reg_eq_playback_lvl {
    struct {
        uint8_t dveq:4;
        uint8_t not_eqclp:1;
        uint8_t unused:3;
    } bit;
    uint8_t reg;
} max98091_reg_eq_playback_lvl_t;

/* HEADPHONE CTRL REG */
typedef union max98091_reg_left_hp_mixer {
    struct {
        uint8_t mixhpl:6;
        uint8_t unused:2;
    } bit;
    uint8_t reg;
} max98091_reg_left_hp_mixer_t;

typedef union max98091_reg_right_hp_mixer {
    struct {
        uint8_t mixhpr:6;
        uint8_t unused:2;
    } bit;
    uint8_t reg;
} max98091_reg_right_hp_mixer_t;

typedef union max98091_reg_hp_ctrl {
    struct {
        uint8_t mixhplg:2;
        uint8_t mixhprg:2;
        uint8_t mixhplsel:1;
        uint8_t mixhprsel:1;
        uint8_t unused:2;
    } bit;
    uint8_t reg;
} max98091_reg_hp_ctrl_t;

typedef union max98091_reg_left_hp_volume {
    struct {
        uint8_t hpvoll:5;
        uint8_t unused:2;
        uint8_t hplm:1;
    } bit;
    uint8_t reg;
} max98091_reg_left_hp_volume_t;

typedef union max98091_reg_right_hp_volume {
    struct {
        uint8_t hpvolr:5;
        uint8_t unused:2;
        uint8_t hprm:1;
    } bit;
    uint8_t reg;
} max98091_reg_right_hp_volume_t;

/* SPEAKER CONFIG REG */
typedef union max98091_reg_left_spk_mixer {
    struct {
        uint8_t mixspl:6;
        uint8_t unused:2;
    } bit;
    uint8_t reg;
} max98091_reg_left_spk_mixer_t;

typedef union max98091_reg_right_spk_mixer {
    struct {
        uint8_t mixspr:6;
        uint8_t spk_slave:1;
        uint8_t unused:1;
    } bit;
    uint8_t reg;
} max98091_reg_right_spk_mixer_t;

typedef union max98091_reg_spk_ctrl {
    struct {
        uint8_t mxisplg:2;
        uint8_t mxisprg:2;
        uint8_t unused:4;
    } bit;
    uint8_t reg;
} max98091_reg_spk_ctrl_t;

typedef union max98091_reg_left_spk_volume {
    struct {
        uint8_t spvoll:6;
        uint8_t unused:1;
        uint8_t splm:1;
    } bit;
    uint8_t reg;
} max98091_reg_left_spk_volume_t;

typedef union max98091_reg_right_spk_volume {
    struct {
        uint8_t spvolr:6;
        uint8_t unused:1;
        uint8_t sprm:1;
    } bit;
    uint8_t reg;
} max98091_reg_right_spk_volume_t;

/* DYNAMIC RANGE CONTROL CONFIG REG */
typedef union max98091_reg_drc_timing {
    struct {
        uint8_t drcatk:3;
        uint8_t unused:1;
        uint8_t drcrls:3;
        uint8_t drcen:1;
    } bit;
    uint8_t reg;
} max98091_reg_drc_timing_t;

typedef union max98091_reg_drc_compressor {
    struct {
        uint8_t drcthc:5;
        uint8_t drccmp:3;
    } bit;
    uint8_t reg;
} max98091_reg_drc_compressor_t;

typedef union max98091_reg_drc_expander {
    struct {
        uint8_t drcthe:5;
        uint8_t drcexp:3;
    } bit;
    uint8_t reg;
} max98091_reg_drc_expander_t;

typedef union max98091_reg_drc_gain {
    struct {
        uint8_t drcg:5;
        uint8_t unused:3;
    } bit;
    uint8_t reg;
} max98091_reg_drc_gain_t;

/* RECEIVER AND LINE OUTPUT REG */
typedef union max98091_reg_rcv_loutl_mixer {
    struct {
        uint8_t mixrcvl:6;
        uint8_t unused:2;
    } bit;
    uint8_t reg;
} max98091_reg_rcv_loutl_mixer_t;

typedef union max98091_reg_rcv_loutl_ctrl {
    struct {
        uint8_t mixrcvlg:2;
        uint8_t unused:6;
    } bit;
    uint8_t reg;
} max98091_reg_rcv_loutl_ctrl_t;

typedef union max98091_reg_rcv_loutl_volume {
    struct {
        uint8_t rcvlvol:5;
        uint8_t unused:2;
        uint8_t rcvlm:1;
    } bit;
    uint8_t reg;
} max98091_reg_rcv_loutl_volume_t;

typedef union max98091_reg_loutr_mixer {
    struct {
        uint8_t mixrcvr:6;
        uint8_t unused:1;
        uint8_t linmod:1;
    } bit;
    uint8_t reg;
} max98091_reg_loutr_mixer_t;

typedef union max98091_reg_loutr_ctrl {
    struct {
        uint8_t mixrcvrg:2;
        uint8_t unused:6;
    } bit;
    uint8_t reg;
} max98091_reg_loutr_ctrl_t;

typedef union max98091_reg_loutr_volume {
    struct {
        uint8_t rcvrvol:5;
        uint8_t unused:2;
        uint8_t rcvrm:1;
    } bit;
    uint8_t reg;
} max98091_reg_loutr_volume_t;

/* JACK DETECT AND ENABLE REG */
typedef union max98091_reg_jack_detect {
    struct {
        uint8_t jdeb:2;
        uint8_t unused:4;
        uint8_t jdwk:1;
        uint8_t jdeten:1;
    } bit;
    uint8_t reg;
} max98091_reg_jack_detect_t;

typedef union max98091_reg_input_en {
    struct {
        uint8_t adlen:1;
        uint8_t adren:1;
        uint8_t lineben:1;
        uint8_t lineaen:1;
        uint8_t mben:1;
        uint8_t unused:3;
    } bit;
    uint8_t reg;
} max98091_reg_input_en_t;

typedef union max98091_reg_output_en {
    struct {
        uint8_t dalen:1;
        uint8_t daren:1;
        uint8_t rcvren:1;
        uint8_t rcvlen:1;
        uint8_t splen:1;
        uint8_t spren:1;
        uint8_t hplen:1;
        uint8_t hpren:1;
    } bit;
    uint8_t reg;
} max98091_reg_output_en_t;

typedef union max98091_reg_lvl_ctrl {
    struct {
        uint8_t not_vsen:1;
        uint8_t not_vs2en:1;
        uint8_t not_zden:1;
        uint8_t unused:5;
    } bit;
    uint8_t reg;
} max98091_reg_lvl_ctrl_t;

typedef union max98091_reg_dsp_filter_en {
    struct {
        uint8_t eq7banden:1;
        uint8_t eq5banden:1;
        uint8_t eq3banden:1;
        uint8_t recbqen:1;
        uint8_t unused:4;
    } bit;
    uint8_t reg;
} max98091_reg_dsp_filter_en_t;

/* BIAS AND POWER MODE CONFIGURATION REG */
typedef union max98091_reg_bias_ctrl {
    struct {
        uint8_t bias_mode:1;
        uint8_t unused:7;
    } bit;
    uint8_t reg;
} max98091_reg_bias_ctrl_t;

typedef union max98091_reg_dac_ctrl {
    struct {
        uint8_t dachp:1;
        uint8_t perfmode:1;
        uint8_t unused:6;
    } bit;
    uint8_t reg;
} max98091_reg_dac_ctrl_t;

typedef union max98091_reg_adc_ctrl {
    struct {
        uint8_t adchp:1;
        uint8_t adcdither:1;
        uint8_t osr128:1;
        uint8_t unused:5;
    } bit;
    uint8_t reg;
} max98091_reg_adc_ctrl_t;

typedef union max98091_reg_device_shutdown {
    struct {
        uint8_t unused:7;
        uint8_t not_shdn:1;
    } bit;
    uint8_t reg;
} max98091_reg_device_shutdown_t;

typedef union max98091_biq_coef {
    struct {
        uint8_t lsb0_7;
        uint8_t lsb8_15;
        uint8_t msb16_23;
        uint8_t unused;
    } bit;
    uint32_t reg;
} max98091_biq_coef_t;

typedef enum jack_state {
    MAX98091_JACK_UNKNOWN = 0x04,
    MAX98091_JACK_UNPLUGGED = 0x06,
    MAX98091_JACK_HEADPHONE = 0x00,
    MAX98091_JACK_HEADSET = 0x02,
} jack_state_t;

typedef struct max98091_biq_band {
    max98091_biq_coef_t coef_b0;
    max98091_biq_coef_t coef_b1;
    max98091_biq_coef_t coef_b2;
    max98091_biq_coef_t coef_a1;
    max98091_biq_coef_t coef_a2;
} max98091_biq_band_t;

typedef struct max98091_biq {
    max98091_biq_band_t band1;
    max98091_biq_band_t band2;
    max98091_biq_band_t band3;
    max98091_biq_band_t band4;
    max98091_biq_band_t band5;
    max98091_biq_band_t band6;
    max98091_biq_band_t band7;
    max98091_biq_band_t record;
} max98091_biq_t;

typedef struct max98091_gain_set_reg {
    max98091_reg_hp_ctrl_t hp_ctrl; /*0x2B*/
    max98091_reg_left_hp_volume_t left_hp_volume; /*0x2C*/
    max98091_reg_right_hp_volume_t right_hp_volume; /*0x2D*/
    max98091_reg_spk_ctrl_t spk_ctrl; /*0x30*/
    max98091_reg_left_spk_volume_t left_spk_volume; /*0x31*/
    max98091_reg_right_spk_volume_t right_spk_volume; /*0x32*/
    max98091_reg_rcv_loutl_ctrl_t rcv_loutl_ctrl; /*0x38*/
    max98091_reg_rcv_loutl_volume_t rcv_loutl_volume; /*0x39*/
    max98091_reg_loutr_ctrl_t loutr_ctrl; /*0x3B*/
    max98091_reg_loutr_volume_t loutr_volume; /*0x3C*/
} max98091_gain_set_reg_t;

typedef struct max98091_register_map {
    max98091_reg_interrupt_mask_t interrupt_mask; /*0x03*/
    max98091_reg_qs_sys_clk_t qs_sys; /*0x04*/
    max98091_reg_qs_sample_rate_t qs_sample_rate; /*0x05*/
    max98091_reg_line_in_config_t line_in_config; /*0x0D*/
    max98091_reg_line_in_lvl_t line_in_lvl; /*0x0E*/
    max98091_reg_in_mode_t in_mode; /*0x0F*/
    max98091_reg_mic1_in_lvl_t mic1_in_lvl; /*0x10*/
    max98091_reg_mic2_in_lvl_t mic2_in_lvl; /*0x11*/
    max98091_reg_mic_bias_voltage_t mic_bias_voltage; /*0x12*/
    max98091_reg_dig_mic_en_t dig_mic_en; /*0x13*/
    max98091_reg_dig_mic_config_t dig_mic_config; /*0x14*/
    max98091_reg_left_adc_mixer_t left_adc_mixer; /*0x15*/
    max98091_reg_right_adc_mixer_t right_adc_mixer; /*0x16*/
    max98091_reg_left_rec_lvl_t left_rec_lvl; /*0x17*/
    max98091_reg_right_rec_lvl_t right_rec_lvl; /*0x18*/
    max98091_reg_rec_biq_lvl_t rec_biq_lvl; /*0x19*/
    max98091_reg_rec_sidestone_t rec_sidetone; /*0x1A*/
    max98091_reg_sys_clk_t sys_clk; /*0x1B*/
    max98091_reg_clk_mode_t clk_mode; /*0x1C*/
    max98091_reg_clk_ratio_ni_msb_t clk_ratio_ni_msb; /*0x1D*/
    max98091_reg_clk_ratio_ni_lsb_t clk_ratio_ni_lsb; /*0x1E*/
    max98091_reg_clk_ratio_mi_msb_t clk_ratio_mi_msb; /*0x1F*/
    max98091_reg_clk_ratio_mi_lsb_t clk_ratio_mi_lsb; /*0x20*/
    max98091_reg_master_mode_t master_mode; /*0x21*/
    max98091_reg_interface_format_t interface_format; /*0x22*/
    max98091_reg_tdm_ctrl_t tdm_ctrl; /*0x23*/
    max98091_reg_tdm_format_t tdm_format; /*0x24*/
    max98091_reg_io_config_t io_config; /*0x25*/
    max98091_reg_filter_config_t filter_config; /*0x26*/
    max98091_reg_dai_playback_lvl_t dai_playback_lvl; /*0x27*/
    max98091_reg_eq_playback_lvl_t eq_playback_lvl; /*0x28*/
    max98091_reg_left_hp_mixer_t left_hp_mixer; /*0x29*/
    max98091_reg_right_hp_mixer_t right_hp_mixer; /*0x2A*/
    max98091_reg_left_spk_mixer_t left_spk_mixer; /*0x2E*/
    max98091_reg_right_spk_mixer_t right_spk_mixer; /*0x2F*/
    max98091_reg_drc_timing_t drc_timing; /*0x33*/
    max98091_reg_drc_compressor_t drc_compressor; /*0x34*/
    max98091_reg_drc_expander_t drc_expander; /*0x35*/
    max98091_reg_drc_gain_t drc_gain; /*0x36*/
    max98091_reg_rcv_loutl_mixer_t rcv_loutl_mixer; /*0x37*/
    max98091_reg_loutr_mixer_t loutr_mixer; /*0x3A*/
    max98091_reg_jack_detect_t jack_detect; /*0x3D*/
    max98091_reg_input_en_t input_en; /*0x3E*/
    max98091_reg_output_en_t output_en; /*0x3F*/
    max98091_reg_lvl_ctrl_t lvl_ctrl; /*0x40*/
    max98091_reg_dsp_filter_en_t dsp_filter_en; /*0x41*/
    max98091_biq_t biquad;
    max98091_reg_bias_ctrl_t bias_ctrl; /*0x42*/
    max98091_reg_dac_ctrl_t dac_ctrl; /*0x43*/
    max98091_reg_adc_ctrl_t adc_ctrl; /*0x44*/
    max98091_reg_device_shutdown_t shutdown; /* 0x45 */
    max98091_gain_set_reg_t gain_set;
} max98091_register_map_t;

#ifdef __cplusplus
}
#endif

#endif /* MAX98091_REG_H_ */
