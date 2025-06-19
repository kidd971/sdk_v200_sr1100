/** @file  quasar_clock.h
 *  @brief This module controls clock related features.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_CLOCK_H_
#define QUASAR_CLOCK_H_

/* INCLUDES *******************************************************************/
#include "quasar_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/*! CDC PLL2 default FRACN coefficient value. */
#define QUASAR_PLL2_FRACN_DEFAULT_VALUE 4260
/*! CDC PLL2 maximum FRACN coefficient value. */
#define QUASAR_PLL2_FRACN_MAX_VALUE 8191 /* 0x1FFF */
/*! CDC PLL2 minimum FRACN coefficient value. */
#define QUASAR_PLL2_FRACN_MIN_VALUE 0

/* TYPES **********************************************************************/
/** @brief The Quasar's system clock selection.
 */
typedef enum quasar_clk_freq {
    /*! System clock at 160 MHz */
    QUASAR_CLK_160MHZ = 160000000,
} quasar_clk_freq_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the clocks.
 *
 *  @param quasar_clock_frequency  Clock frequency selection.
 */
void quasar_clock_init(quasar_clk_freq_t quasar_clk_freq);

/** @brief Get the system clock frequency value.
 *
 *  @return System clock frequency.
 */
uint32_t quasar_clock_get_system_clock_freq(void);

/** @brief Set PLL2 FRACN coefficient.
 *
 *  @param fracn The coefficient to set.
 */
void quasar_clock_set_pll2_fracn(uint32_t fracn);

/** @brief Get current PLL2 FRACN coefficient.
 *
 *  @return current PLL2 FRACN coefficient.
 */
uint32_t quasar_clock_get_pll2_fracn(void);

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_CLOCK_H_ */
