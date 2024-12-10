/** @file  swc_def.h
 *  @brief SPARK Wireless Core definitions.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SWC_DEF_H_
#define SWC_DEF_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>

/* TYPES **********************************************************************/

/** @brief Node's Role.
 */
typedef enum swc_role {
    /*! Node acts as the Coordinator */
    SWC_ROLE_COORDINATOR,
    /*! Node has no special function */
    SWC_ROLE_NODE
} swc_role_t;

/** @brief Node's Sleep Level.
 */
typedef enum swc_sleep_level {
    /*! Idle sleep level */
    SWC_SLEEP_IDLE,
    /*! Shallow sleep level */
    SWC_SLEEP_SHALLOW,
    /*! Deep sleep level */
    SWC_SLEEP_DEEP
} swc_sleep_level_t;

/** @brief Node's ISI mitigation level.
 */
typedef enum swc_isi_mitig {
    /*! Inter-symbol interference mitigation level 0 */
    SWC_ISI_MITIG_0,
    /*! Inter-symbol interference mitigation level 1 */
    SWC_ISI_MITIG_1,
    /*! Inter-symbol interference mitigation level 2 */
    SWC_ISI_MITIG_2,
    /*! Inter-symbol interference mitigation level 3 */
    SWC_ISI_MITIG_3,
} swc_isi_mitig_t;

/** @brief Node's chip rate.
 */
typedef enum swc_chip_rate {
    /*! Enable 20.48 MHz Chip rate */
    SWC_CHIP_RATE_20_48_MHZ,
    /*! Enable 27.30 MHz Chip rate */
    SWC_CHIP_RATE_27_30_MHZ,
    /*! Enable 40.96 MHz Chip rate */
    SWC_CHIP_RATE_40_96_MHZ
} swc_chip_rate_t;

/** @brief Radio's Interrupt Request Polarity.
 */
typedef enum swc_irq_polarity {
    /*! Interrupt pin active in high state */
    SWC_IRQ_ACTIVE_HIGH,
    /*! Interrupt pin active in low state */
    SWC_IRQ_ACTIVE_LOW
} swc_irq_polarity_t;

/** @brief Radio's SPI mode.
 */
typedef enum swc_spi_mode {
    /*! SPI timings are standard */
    SWC_SPI_STANDARD,
    /*! SPI timings are optimized for high capacitive load on the bus */
    SWC_SPI_FAST
} swc_spi_mode_t;

/** @brief Radio's digital output driver impedance.
 */
typedef enum swc_outimped {
    /*! output impedance 0 */
    SWC_OUTIMPED_0,
    /*! output impedance 1 */
    SWC_OUTIMPED_1,
    /*! output impedance 2 */
    SWC_OUTIMPED_2,
    /*! output impedance 3 */
    SWC_OUTIMPED_3
} swc_outimped_t;

/** @brief Connection's Modulation.
 */
typedef enum swc_modulation {
    /*! OOK modulation */
    SWC_MOD_OOK,
    /*! inverted OOK modulation */
    SWC_MOD_IOOK,
    /*! PPM modulation */
    SWC_MOD_PPM,
    /*! 2 bit PPM modulation */
    SWC_MOD_2BITPPM,
} swc_modulation_t;

/** @brief Connection's Modulation.
 */
typedef enum swc_chip_repetition {
    /*! 1 chip repetition */
    SWC_CHIP_REPET_1,
    /*! 2 chip repetition */
    SWC_CHIP_REPET_2,
    /*! 3 chip repetition */
    SWC_CHIP_REPET_3,
    /*! 4 chip repetition */
    SWC_CHIP_REPET_4,
} swc_chip_repetition_t;

/** @brief Connection's Forward Error Correction Ratio.
 *
 *  @note FEC Ratio is (FEC_RATIO + 9) / 8.
 */
typedef enum swc_fec_ratio {
    /*! FEC ratio 1.00 */
    SWC_FEC_1_0_0_0,
    /*! FEC ratio 1.25 */
    SWC_FEC_1_2_5_0,
    /*! FEC ratio 1.375 */
    SWC_FEC_1_3_7_5,
    /*! FEC ratio 1.50 */
    SWC_FEC_1_5_0_0,
    /*! FEC ratio 1.625 */
    SWC_FEC_1_6_2_5,
    /*! FEC ratio 1.75 */
    SWC_FEC_1_7_5_0,
    /*! FEC ratio 1.875 */
    SWC_FEC_1_8_7_5,
    /*! FEC ratio 2.00 */
    SWC_FEC_2_0_0_0,
} swc_fec_ratio_t;

/** @brief Clear Channel Assessment Fail Action.
 */
typedef enum swc_cca_fail_action {
    /*! Force transmission */
    SWC_CCA_FORCE_TX,
    /*! Abort transmission */
    SWC_CCA_ABORT_TX
} swc_cca_fail_action_t;

/** @brief Wireless core events.
 */
typedef enum swc_event {
    /*! No event */
    SWC_EVENT_NONE,
    /*! The connection is established between nodes */
    SWC_EVENT_CONNECT,
    /*! The connection is broken between nodes */
    SWC_EVENT_DISCONNECT,
    /*! An error occured on the wireless core */
    SWC_EVENT_ERROR
} swc_event_t;

typedef enum swc_concurrency_mode {
    /*! Concurrency mode for high performance MCU */
    SWC_CONCURRENCY_MODE_HIGH_PERFORMANCE,
    /*! Concurrency mode for low performance MCU */
    SWC_CONCURRENCY_MODE_LOW_PERFORMANCE
} swc_concurrency_mode_t;

#endif /* SWC_DEF_H_ */
