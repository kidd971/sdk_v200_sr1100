/** @file sr1100_def.h
 *  @brief SR1100 definitions
 *
 *  @copyright Copyright (C) 2020-2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is confidential and proprietary.
 *  @author    SPARK FW Team.
 */
#ifndef SR1100_DEF_H_
#define SR1100_DEF_H_

/* INCLUDES *******************************************************************/
#include <stdint.h>
#include "sr_access.h"
#include "sr_reg.h"

/* TYPES **********************************************************************/
/** @brief Radio's sleep level.
 *
 *  Sleep depth at which the radio will go when asleep.
 */
typedef enum sleep_lvl {
    /*! Radio sleep level IDLE with wake once disabled. */
    SLEEP_IDLE_NO_WAKEONCE = SLPDEPTH_WAKEONCE_0b00,
    /*! Radio sleep level IDLE */
    SLEEP_IDLE = SLPDEPTH_WAKEONCE_0b01,
    /*! Radio sleep level SHALLOW */
    SLEEP_SHALLOW = SLPDEPTH_WAKEONCE_0b10,
    /*! Radio sleep level DEEP */
    SLEEP_DEEP = SLPDEPTH_WAKEONCE_0b11,
} sleep_lvl_t;

/** @brief Interrupt polarity pin enumeration.
 */
typedef enum irq_polarity {
    /*! Interrupt pin active in LOW state */
    IRQ_ACTIVE_LOW = IRQPOLAR_0b0,
    /*! Interrupt pin active in HIGH state */
    IRQ_ACTIVE_HIGH = IRQPOLAR_0b1,
} irq_polarity_t;

/** @brief Transceiver chip rate enumeration.
 */
typedef enum chip_rate_cfg {
    /*! Enable 20.48MHz Chip rate */
    CHIP_RATE_20_48_MHZ = CHIP_RATE_0b00,
    /*! Enable 27.30MHz Chip rate */
    CHIP_RATE_27_30_MHZ = CHIP_RATE_0b10,
    /*! Enable 40.96MHz Chip rate */
    CHIP_RATE_40_96_MHZ = CHIP_RATE_0b01,
} chip_rate_cfg_t;

/** @brief Transceiver chip clock source.
 */
typedef enum chip_clk_src {
    /*! Disable external PLL. */
    CHIP_CLK_INTERNAL_OUTPUT_HIGH_IMPED = CHIP_CLK_0b00,
    /*! Enable integrated PLL and output signal. */
    CHIP_CLK_INTERNAL_OUTPUT_ENABLE = CHIP_CLK_0b01,
    /*! Enable external PLL. */
    CHIP_CLK_EXTERNAL_INTERNAL_DISABLE = CHIP_CLK_0b10,
    /*! Enable internal and external PLL. Default one is the external. */
    CHIP_CLK_EXTERNAL_INTERNAL_ENABLE = CHIP_CLK_0b11,
} chip_clk_src_t;

/** @brief Crystal oscillator slow clock source.
 */
typedef enum xtal_clk {
    /*! Disable external crystal clock source. */
    XTAL_CLK_INTERNAL_OUTPUT_HIGH_IMPED = XTAL_CLK_0b00,
    /*! Enable integrated crystal oscillator source. */
    XTAL_CLK_INTERNAL_ENABLE_AND_OUTPUT_ENABLE = XTAL_CLK_0b01,
    /*! Enable external crystal clock source. */
    XTAL_CLK_EXTERNAL_ENABLE = XTAL_CLK_0b10,
    /*! Enable internal and external crystal clk source. */
    XTAL_CLK_BOTH_ENABLE = XTAL_CLK_0b11,
} xtal_clk_t;

/** @brief Enable/Disable standard SPI operation.
 */
typedef enum std_spi {
    /*! Enable fast SPI mode, that doesn't comply with the de facto industry SPI communication */
    SPI_FAST = FASTMISO_0b1,
    /*! Enable standard SPI mode */
    SPI_STANDARD = FASTMISO_0b0,
} std_spi_t;

/** @brief Digital output driver impedance.
 */
typedef enum outimped {
    /*! OUTIMPED 0 */
    OUTIMPED_0 = OUTIMPED_0b00,
    /*! OUTIMPED 1 */
    OUTIMPED_1 = OUTIMPED_0b01,
    /*! OUTIMPED 2 */
    OUTIMPED_2 = OUTIMPED_0b10,
    /*! OUTIMPED 3 */
    OUTIMPED_3 = OUTIMPED_0b11,
} outimped_t;

/** @brief Radio's TX power level enumeration in dBFs.
 */
typedef enum tx_power {
    /*! TX power -0.0 dBFs */
    MINUS_0_0_DBFS = 4,
    /*! TX power -0.6 dBFs */
    MINUS_0_6_DBFS = 5,
    /*! TX power -1.2 dBFs */
    MINUS_1_2_DBFS = 6,
    /*! TX power -1.8 dBFs */
    MINUS_1_8_DBFS = 7,

    /* TX power preset */
    TX_PWR_LOW_OUTPUT_POWER_RANGING = 0,
    TX_PWR_HIGH_OUTPUT_POWER_RANGING,
} tx_power_t;

/** @brief Radio's Forward error correction enumeration.
 */
typedef enum fec_level {
    /*! Forward error correction ratio 1.00 */
    FEC_LVL_0 = SET_FEC_RATE0(0),
    /*! Forward error correction ratio 1.25 */
    FEC_LVL_1 = SET_FEC_RATE0(0b001),
    /*! Forward error correction ratio 1.375 */
    FEC_LVL_2 = SET_FEC_RATE0(0b010),
    /*! Forward error correction ratio 1.50 */
    FEC_LVL_3 = SET_FEC_RATE0(0b011),
    /*! Forward error correction ratio 1.625 */
    FEC_LVL_4 = SET_FEC_RATE0(0b100),
    /*! Forward error correction ratio 1.75 */
    FEC_LVL_5 = SET_FEC_RATE0(0b101),
    /*! Forward error correction ratio 1.875 */
    FEC_LVL_6 = SET_FEC_RATE0(0b110),
    /*! Forward error correction ratio 2.00 */
    FEC_LVL_7 = SET_FEC_RATE0(0b111),
} fec_level_t;

#define FEC_TYPE_TO_RAW(fec_level) GET_FEC_RATE0(fec_level)

/** @brief Radio's modulation type enumeration.
 */
typedef enum modulation {
    /*! Frame modulation OOK (On-off keying) */
    MODULATION_OOK = SET_CHIPCODE0(0b001),
    /*! Frame modulation IOOK (Inverse On-off keying) */
    MODULATION_IOOK = SET_CHIPCODE0(0b000),
    /*! Frame modulation PPM (Pulse-position modulation) */
    MODULATION_PPM = SET_CHIPCODE0(0b010),
    /*! Frame modulation 2BITPPM(2-bit Pulse-position modulation) */
    MODULATION_2BITPPM = SET_CHIPCODE0(0b011),
    /*! Frame modulation PPM fast */
    MODULATION_PPM_FAST = SET_CHIPCODE0(0b100),
    /*! Frame modulation 2BITPPM fast */
    MODULATION_2BITPPM_FAST = SET_CHIPCODE0(0b101),
    /*! Frame modulation PPM utlrafast */
    MODULATION_PPM_ULTRAFAST = SET_CHIPCODE0(0b110),
    /*! Frame modulation 2BITPPM utlrafast */
    MODULATION_2BITPPM_ULTRAFAST = SET_CHIPCODE0(0b111),
} modulation_t;

/** @brief Radio's chip repetition type enumeration.
 */
typedef enum chip_repetition {
    /*! CHIP repetition 1X */
    CHIP_REPET_1 = SET_CHIPREPE0(0),
    /*! CHIP repetition 2X */
    CHIP_REPET_2 = SET_CHIPREPE0(0b01),
    /*! CHIP repetition 3X */
    CHIP_REPET_3 = SET_CHIPREPE0(0b10),
    /*! CHIP repetition 4X */
    CHIP_REPET_4 = SET_CHIPREPE0(0b11),
} chip_repetition_t;

#define CHIP_REPET_TO_RAW(chip_repetition) (GET_CHIPREPE0(chip_repetition) + 1)

/** @brief Inter-symbol interference mitigation enum
 */
typedef enum isi_mitig {
    /*! Inter-symbol interference mitigation level 0 */
    ISI_MITIG_0 = 0,
    /*! Inter-symbol interference mitigation level 1 */
    ISI_MITIG_1 = 1,
    /*! Inter-symbol interference mitigation level 2 */
    ISI_MITIG_2 = 2,
    /*! Inter-symbol interference mitigation level 3 */
    ISI_MITIG_3 = 3,
} isi_mitig_t;

#define ISI_TYPE_TO_RAW(isi) GET_ISIMITIG0(isi)

#define MAX_INTEGGAIN     15
#define DEFAULT_INTEGGAIN 8

/** @brief Integgain entry for lookup table
 */

typedef struct integgain_entry {
    /*! Radio chip rate */
    chip_rate_cfg_t chip_rate;
    /*! Channel pulse count */
    uint8_t pulse_count;
    /*! Integgain value */
    uint8_t integgain;
} integgain_entry_t;

static const integgain_entry_t integgain_lookup_table[] = {
    /* Chip Rate         Pulse count  Integgain value */
    {CHIP_RATE_20_48_MHZ, 1, 7},  {CHIP_RATE_20_48_MHZ, 2, 5},  {CHIP_RATE_20_48_MHZ, 3, 3},
    {CHIP_RATE_20_48_MHZ, 4, 3},  {CHIP_RATE_20_48_MHZ, 5, 3},

    {CHIP_RATE_40_96_MHZ, 1, 12}, {CHIP_RATE_40_96_MHZ, 2, 10}, {CHIP_RATE_40_96_MHZ, 3, 10},
};
#define INTEGGAIN_ENTRY_COUNT (sizeof(integgain_lookup_table) / sizeof(integgain_entry_t))

/** @brief Radio's syncword length enumeration.
 */
typedef enum syncword_length {
    /*! 32 bits modulated with the plain OOK chip code */
    SYNCWORD_LENGTH_32_OOK = SET_SWLENGTH(0b00),
    /*! 16 bits modulated into 1-bit PPM symbols */
    SYNCWORD_LENGTH_16_1BIT_PPM = SET_SWLENGTH(0b01),
    /*! 32 bits modulated into 1-bit PPM symbols */
    SYNCWORD_LENGTH_32_1BIT_PPM = SET_SWLENGTH(0b10),
    /*! 64 bits modulated into 1-bit PPM symbols */
    SYNCWORD_LENGTH_64_1BIT_PPM = SET_SWLENGTH(0b11),
} syncword_length_t;

#define DEFAULT_PACKET_CONFIGURATION \
    (ADDRFIELD_0b11 | ADDRLEN_0b1 | SIZEHDR_0b1 | SAVESIZE_0b1 | BIT_RETRYHDR)

/** @brief Frame outcome enumeration.
 */
typedef enum frame_outcome {
    /*! Frame received */
    FRAME_RECEIVED,
    /*! Frame lost */
    FRAME_LOST,
    /*! Frame rejected */
    FRAME_REJECTED,
    /*! Frame sent and acknowledged */
    FRAME_SENT_ACK,
    /*! Frame sent and ack is lost*/
    FRAME_SENT_ACK_LOST,
    /*! Frame sent and ack is rejected */
    FRAME_SENT_ACK_REJECTED,
    /*! No frame sent or received */
    FRAME_WAIT
} frame_outcome_t;

/** @brief Frame configuration.
 */
typedef struct {
    /*! RF modulation */
    modulation_t modulation;
    /*! Chip repetition */
    chip_repetition_t chip_repet;
    /*! Forward Error Correction level */
    fec_level_t fec;
} frame_cfg_t;

/** @brief Synchronization word configuration.
 */
typedef struct syncword_cfg {
    /*! Synchronization word, 16 or 32 bits.*/
    uint32_t syncword;
    /*! Synchronization word detection bit mismatch's extra cost, 3 bits range value */
    uint8_t syncword_bit_cost;
    /*! Synchronization word detection tolerance, 5 bits range value */
    uint8_t syncword_tolerance;
    /*! Synchronization word length, either SYNCWORD_LENGTH_16 or SYNCWORD_LENGTH_32 */
    syncword_length_t syncword_length;
} syncword_cfg_t;

/** @brief Interleave frame data feature.
 *
 *  @note Seek register 0x11 for more info.
 */
typedef enum interleav_cfg {
    /*! Enable INTERLEAV feature */
    INTERLEAV_DISABLE = INTRLEAV_0b0,
    /*! Enable INTERLEAV feature */
    INTERLEAV_ENABLE = INTRLEAV_0b1,
} interleav_cfg_t;

static const uint32_t sync_word_table[] = {
    0x5ea6c11d, 0x09ae74e5, 0x0a2fb635, 0x0ade3365, 0x0b1ae937, 0x0cbad627, 0x0ce2a76d, 0x0e6ae45b,
    0xe129ab17, 0xe126eac6, 0xe1225779, 0xe620a5db, 0xe92e8c4e, 0xe5a0af32, 0x0daf91ac, 0x0ca2fb72,
};

/** @brief Radio internal or external clock source.
 */
typedef struct clock_source {
    /*! Enable external PLL clock source */
    chip_clk_src_t pll_clk_source;
    /*! Enable external XTAL clock source */
    xtal_clk_t xtal_clk_source;
} clock_source_t;

/** @brief Radio instance.
 */
typedef struct radio {
    /*! Radio Number */
    uint8_t radio_id;
    /*! Interrupt polarity */
    irq_polarity_t irq_polarity;
    /*! VREF tune */
    int8_t vref_tune;
    /*! IREF tune */
    uint8_t iref_tune;
    /*! Standard SPI operations */
    std_spi_t std_spi;
    /*! Radio PLL and XTAL clock source (Internal or external) */
    clock_source_t clock_source;
    /*! Digital output driver impedance */
    outimped_t outimped;
    /*! Chip rate */
    chip_rate_cfg_t chip_rate;
    /*! Summation of ADC samples */
    bool sumrxadc;
} radio_t;

/* DEFINE *********************************************************************/
#define PLL_FREQ_HZ(chip_rate)                   \
    ((chip_rate) == (CHIP_RATE_40_96_MHZ) ?      \
         (40960000) :                            \
         ((chip_rate) == (CHIP_RATE_27_30_MHZ) ? \
              (27300000) :                       \
              ((chip_rate) == (CHIP_RATE_20_48_MHZ) ? (20480000) : (20480000))))
#define PLL_FREQ_KHZ(chip_rate)                  \
    ((chip_rate) == (CHIP_RATE_40_96_MHZ) ?      \
         (40960) :                               \
         ((chip_rate) == (CHIP_RATE_27_30_MHZ) ? \
              (27300) :                          \
              ((chip_rate) == (CHIP_RATE_20_48_MHZ) ? (20480) : (20480))))
#define PLL_RATIO(chip_rate)                \
    ((chip_rate) == (CHIP_RATE_40_96_MHZ) ? \
         (1250) :                           \
         ((chip_rate) == (CHIP_RATE_27_30_MHZ) ? (833) : ((chip_rate) == (CHIP_RATE_20_48_MHZ) ? (625) : (625))))
#define DCRO_MAX_COUNT           64
#define MAX_FRAMESIZE            255
#define BROADCAST_ADDRESS        0xFF
#define PHASE_OFFSET_BYTE_COUNT  16
#define NB_PHASES                4
#define NB_PULSES                9
#define MAX_PULSE_WIDTH          7
#define POWER_UP_TIME            1000
#define MS_TO_S                  1000

#define TIMEOUT_VAL2RAW(val)    ((val - 1) / 8)
#define PWRUPDELAY_VAL2RAW(val) (val / 8)

/*! Minimum CCAINTERV value for the SR1120 */
#define CCAINTERV_MIN_VALUE     32
#define CCAINTERV_VAL2RAW(val)  ((val < CCAINTERV_MIN_VALUE) ? 0 : ((val / CCAINTERV_MIN_VALUE) - 1))

#define RX_MODE                 RADIODIR_0b1
#define TX_MODE                 RADIODIR_0b0

/*! Base preamble and debug options. */
#define REG16_PREAMB_DEBUG_OPT (SET_MAINDEBUG(5) | SET_MAXSIGLVL(3))

/*! @brief Optimized preamble length for max payload size
 *
 *  @note This is optimized for ISI_MITIG = 0 and SYNCWORD_LEN = 32 bits.
 *
 *  The preamble length is determined by the following formula:
 *
 *  preamble_bits = ((OPTIMIZED_PREAMBLE_LEN * 4 * chip_multiplier) + ((48 / chip_per_symbol) + 1)) * chip_per_symbol
 *
 *  Where chip_multiplier = 2 if SYNCWORD_LEN = 32 bits
 *
 *  and chip_per_symbol = 2 if isi_mitig = 0
 *
 *  With OPTIMIZED_PREAMBLE_LEN = 20, Preamble length is 210 bits
 */
#define OPTIMIZED_PREAMBLE_LEN 20

/*! Base prelude options. */
#define REG16_PRELUDE_OPT                                                             \
    ((REG16_PRELUDE_DEFAULT & ~BITS_PREATRKBW & ~BITS_PREAMBTHR) | SET_PREATRKBW(3) | \
     SET_PREAMBTHR(10))

#endif /* SR1100_DEF_H_ */
