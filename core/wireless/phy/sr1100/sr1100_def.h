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
#include "sr1120_V3_reg.h"
#include "sr_bit_macros.h"

/* CONSTANTS ******************************************************************/
/*! Limit address for 16-bit registers. */
#define REG_16BIT_ADDRESS_LIMIT 0x38

/*! Register operation configuration. */
#define REG_READ_BURST  BIT(7)
#define REG_WRITE       BIT(6)
#define REG_WRITE_BURST (BIT(7) | REG_WRITE)

/* MACROS *********************************************************************/
/*! Returns true if register at the provided adress is 16 bits, 0 otherwise. */
#define REG_IS_16_BITS(x) ((x) < REG_16BIT_ADDRESS_LIMIT ? 1 : 0)

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

/** @brief Transceiver chip rate enumeration.
 */
typedef enum spi_mode_cfg {
    /*! SPI mode. */
    SPI_MODE = QSPI_0b00,
    /*! QSPI with no dummy cycle mode. */
    QSPI_0_DUMMY_MODE = QSPI_0b01,
    /*! QSPI with 1 dummy cycle mode. */
    QSPI_1_DUMMY_MODE = QSPI_0b10,
    /*! Dummy cycles compatible with nRF microcontrollers mode. */
    QSPI_NRF_DUMMY_MODE = QSPI_0b11,
} spi_mode_cfg_t;

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

#define MAX_INTEGGAIN        15
#define DEFAULT_INTEGGAIN    8

/** @brief Radio's SFD length enumeration.
 */
typedef enum sfd_length {
    /*! 32 bits modulated with the plain OOK chip code */
    SFD_LENGTH_32_OOK = SET_SFDLENGTH(0b00),
    /*! 16 bits modulated into 1-bit PPM symbols */
    SFD_LENGTH_16_1BIT_PPM = SET_SFDLENGTH(0b01),
    /*! 32 bits modulated into 1-bit PPM symbols */
    SFD_LENGTH_32_1BIT_PPM = SET_SFDLENGTH(0b10),
    /*! 64 bits modulated into 1-bit PPM symbols */
    SFD_LENGTH_64_1BIT_PPM = SET_SFDLENGTH(0b11),
} sfd_length_t;

#define DEFAULT_PACKET_CONFIGURATION (ADDRFIELD_0b11 | ADDRLEN_0b1 | SIZEHDR_0b1 | SAVESIZE_0b1 | BIT_RETRYHDR)

/** @brief Frame outcome enumeration.
 */
typedef enum frame_outcome {
    /*! Frame received */
    FRAME_RECEIVED,
    /* Frame received has an invalid data size. */
    FRAME_RECEIVED_DATA_SIZE_INVALID,
    /* Frame received has an invalid header size. */
    FRAME_RECEIVED_HEADER_SIZE_INVALID,
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

/** @brief Start Frame Delimiter configuration.
 */
typedef struct sfd_cfg {
    /*! Start Frame Delimiter, 16 or 32 bits.*/
    uint32_t sfd;
    /*! Start Frame Delimiter detection bit mismatch's extra cost, 3 bits range value */
    uint8_t sfd_bit_cost;
    /*! Start Frame Delimiter detection tolerance, 5 bits range value */
    uint8_t sfd_tolerance;
    /*! Start Frame Delimiter length, either SFD_LENGTH_16 or SFD_LENGTH_32 */
    sfd_length_t sfd_length;
} sfd_cfg_t;

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

/** @brief Legacy table containing the 16 originals SFD.
 */
static const uint32_t sfd_table_legacy[] = {
    0x5ea6c11d, 0x09ae74e5, 0x0a2fb635, 0x0ade3365, 0x0b1ae937, 0x0cbad627, 0x0ce2a76d, 0x0e6ae45b,
    0xe129ab17, 0xe126eac6, 0xe1225779, 0xe620a5db, 0xe92e8c4e, 0xe5a0af32, 0x0daf91ac, 0x0ca2fb72,
};

/** @brief Table containing the SFD with the best orthogonal properties and PDR performance.
 */
static const uint32_t sfd_table[] = {
    0xFCC8D605, 0xFD01993A, 0xFD23E281, 0xFD427129, 0xFE2D841A, 0xFE0328D5, 0xFE18D116, 0xFD9E0115, 0xFF062B60,
    0xFE301B51, 0xFD689388, 0xFD2D3222, 0xFD265542, 0xFF8B9021, 0xFD54A641, 0xFF286710, 0xFD6148C6, 0xFE4401F6,
    0xFDA009E9, 0xFF601C25, 0xFFC14432, 0xFF16601A, 0xFFA42C82, 0xFEA44649, 0xFFCC202C, 0xFE4D06A1, 0xFE64E850,
    0xFE4B7240, 0xE1225779, 0xFFA08346, 0x0DAF91AC, 0x0CE2A76D, 0xFE705682, 0x09AE74E5, 0xFFC4C181, 0xFFE61210,
    0xF95038F1, 0xFCB86426, 0xFC079709, 0xE616A695, 0xFB0941F1, 0xFA03672A, 0xFC8758A2, 0xFC5A2A25, 0xF8C49BE0,
    0xF8937431, 0xE5B640F1, 0xC9123DCD, 0xFA473C03, 0xF92C2C59, 0xFA0C4FC2, 0xFC3A234A, 0xF9464A55, 0xF900FE26,
    0xFA55131A, 0xF8F0299A, 0xFC762530, 0xC99C3656, 0xD4E27D81, 0xF8872696, 0xFC129672, 0xF9D662A0, 0xFA5285C5,
    0xFC05F065, 0xE0563E9A, 0xD613B15C, 0xD48E0FB1, 0xD742B790, 0xC989CB72, 0xE535B849, 0xD926D2AA, 0xD4C6CA6A,
    0xC502FD72, 0xC6963F41, 0xF5710336, 0xD631E1AA, 0xF48C91D9, 0x49B9DB88, 0xD6194CDA, 0x06C7DF48, 0xD9281DE6,
    0x797B3209, 0xD328FB82, 0xC639C971, 0xF6AC3930, 0x3636536A, 0x3311BBC3, 0xC81B39F2, 0xD4889F56, 0x1A52CBF1,
    0xE053CB6C, 0xD0477B25, 0x93C06BE9, 0xC65A3999, 0xC84D7AC9, 0x162CAF69, 0xC35957B0, 0xC1ED1B1A, 0x06383D7E,
    0xE589B2F0, 0xE45AA3B2, 0xCC5D4936, 0xF60A96CC, 0x6309FA3A, 0xF4B21596, 0xF5C6B08A, 0xC04ADEE5, 0x305EDF90,
    0xC301DBD5, 0x5E266CB1, 0xF5432D4A, 0xE159DC19, 0xE1E6D780, 0x245F372A, 0xF63B0D22, 0xF53A30AC, 0x960BE395,
    0x31BCD45A, 0xE13DC26A, 0xE117663C, 0x607753F0, 0xF3B02D15, 0xF325AE0C, 0xF22690F5, 0xF4724399, 0xF0CE6E09,
    0xF371D340, 0x1602FAEE};

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
#if SR1100
    /*! SPI mode setting */
    spi_mode_cfg_t spi_mode_cfg;
#endif
    /*! Main debug (ASIC) */
    uint8_t main_debug;
    /*! RSSI offset */
    int8_t rssi_offset;
} radio_t;

/* DEFINE *********************************************************************/

/*! 20 MHz PHY Integgain table. */
#define INTEGGAIN_20_48_PC1 9
#define INTEGGAIN_20_48_PC2 8
#define INTEGGAIN_20_48_PCX 7
/*! 27 MHz PHY Integgain table. */
#define INTEGGAIN_27_30_PC1 11
#define INTEGGAIN_27_30_PC2 9
#define INTEGGAIN_27_30_PCX 8
/*! 40 MHz PHY Integgain table. */
#define INTEGGAIN_40_96_PC1 13
#define INTEGGAIN_40_96_PC2 10
#define INTEGGAIN_40_96_PCX 9
/*! Max pulse count. */
#define CHIP_RATE_COUNT 3
#define MIN_PULSE_COUNT 1
#define MAX_PULSE_COUNT 3

/* Get the frequency of the PLL depending on the chip rate. */
#define PLL_FREQ_HZ(chip_rate)                           \
    ((chip_rate) == (CHIP_RATE_40_96_MHZ) ? (40960000) : \
                                            ((chip_rate) == (CHIP_RATE_27_30_MHZ) ? (27306666) : (20480000)))

/* Get the frequency of the PLL in kHz depending on the chip rate. */
#define PLL_FREQ_KHZ(chip_rate) \
    ((chip_rate) == (CHIP_RATE_40_96_MHZ) ? (40960) : ((chip_rate) == (CHIP_RATE_27_30_MHZ) ? (27306) : (20480)))

/* Get the PLL ratio needed to go from xtal clock frequency to requested chip rate. */
#define XTAL_TO_PLL_RATIO(chip_rate) \
    ((chip_rate) == (CHIP_RATE_40_96_MHZ) ? (1250) : ((chip_rate) == (CHIP_RATE_27_30_MHZ) ? (833.3333) : (625)))
#define SUMRXADC(chip_rate)        (chip_rate == CHIP_RATE_20_48_MHZ ? SET_SUMRXADC(false) : SET_SUMRXADC(true))
#define DCRO_MAX_COUNT             64
#define MAX_FRAMESIZE              255
#define BROADCAST_ADDRESS          0xFF
#define PHASE_OFFSET_BYTE_COUNT    16
#define NB_PHASES                  4
#define NB_PULSES                  9
#define MAX_PULSE_WIDTH            7
#define POWER_UP_TIME              1000
#define MS_TO_US                   1000

#define CHIP_RATE_FACTOR_20_48_MHZ 20480000
#define CHIP_RATE_FACTOR_27_3_MHZ  27300000
#define CHIP_RATE_FACTOR_40_96_MHZ 40960000

#define TIMEOUT_VAL2RAW(val)       ((val - 1) / 8)
#define PWRUPDELAY_VAL2RAW(val)    (val / 8)

/*! Minimum CCAINTERV value for the SR1120 */
#define CCAINTERV_MIN_VALUE    32
#define CCAINTERV_VAL2RAW(val) ((val < CCAINTERV_MIN_VALUE) ? 0 : ((val / CCAINTERV_MIN_VALUE) - 1))
#define CCAINTERV_MAX_VALUE    ((GET_CCAINTERV(UINT16_MAX) + 1) * CCAINTERV_MIN_VALUE)

/*! CCA retry time converter */
#define CCA_RETRY_TIME_PLL_TO_REG(value_pll) ((value_pll / 32) - 1)

/*! CCA ON time pll cycles to register converter.
 *
 *  The +7 value was added in the macro to force the ppl cycles used by the 27.30MHz chip rate to be bigger or equal to
 *  the standard cca ON time.
 */
#define CCA_ON_TIME_PLL_TO_REG(value_pll) (((value_pll + 7) / 8) - 1)

/*! CCA ON time register to pll cycles converter. */
#define CCA_ON_TIME_REG_TO_PLL(value_reg) ((value_reg + 1) * 8)

/*! CCA maximum retry count value depending on the selected fail action */
#define CCA_MAX_RETRY_COUNT(cca_fail_action) \
    ((cca_fail_action == CCA_FAIL_ACTION_TX) ? GET_MAXRETRY(UINT16_MAX) : GET_MAXRETRY(UINT16_MAX) + 1)

/*! Radio direction modes. */
#define RX_MODE RADIODIR_0b1
#define TX_MODE RADIODIR_0b0

/*! Optimized MAXSIGLVL register value for RF performances. */
#define MAXSIGLVL_OPTIMIZED_REG_VAL SET_MAXSIGLVL(1)

/*! MAIN DEBUG value to have both RX and TX info output on SYNC pin. */
#define MAIN_DEBUG_VAL_RX_TX_INFO_ON_SYNC_PIN 5

/*! MAIN DEBUG value to have maximum info.
 *  Receiver status is available through the S_IN PIN.
 *  Transmitter status is available through the SYNC PIN.
 */
#define MAIN_DEBUG_VAL_MAX_INFO 4

/*! @brief Optimized preamble length for max payload size
 *
 *  @note This is optimized for ISI_MITIG = 0 and SFD_LEN = 32 bits.
 *
 *  The preamble length is determined by the following formula:
 *
 *  preamble_bits = ((OPTIMIZED_PREAMBLE_LEN * 4 * chip_multiplier) + ((48 / chip_per_symbol) + 1)) * chip_per_symbol
 *
 *  Where chip_multiplier = 2 if SFD_LEN = 32 bits
 *
 *  and chip_per_symbol = 2 if isi_mitig = 0
 *
 *  With OPTIMIZED_PREAMBLE_LEN = 20, Preamble length is 210 bits
 */
#define OPTIMIZED_PREAMBLE_LEN 20

/*! Optimized prelude options. */
#define REG16_PRELUDE_OPT \
    (SET_SWBITTOL(2) | SET_SOFTSWTHR(10) | SET_PREAMBTHR(10) | SET_PREATRKBW(2) | SET_PREADETBW(2))

#endif /* SR1100_DEF_H_ */
