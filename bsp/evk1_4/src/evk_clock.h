/** @file  evk_clock.h
 *  @brief This module control clock related features.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef EVK_CLOCK_H_
#define EVK_CLOCK_H_

/* INCLUDES *******************************************************************/
#include "evk_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the external PLL oscillator clock.
 *
 *  This external PLL can be optionally used to drive the radio's PLL.
 */
void evk_clock_ext_osc_init(void);

/** @brief Initialize the external XTAL clock.
 *
 *  This external XTAL clock can be optionally used to drive the radio's XTAL clock.
 */
void evk_clock_xtal_init(void);

/** @brief Initialize RTC with wake up timer.
 */
void evk_clock_rtc_for_wakeup_init(void);

/** @brief Enable all GPIO's peripheral clock.
 */
void evk_clock_all_gpio_init(void);

/** @brief Set the system clock frequency.
 *
 *  @param evk_clk_freq  Clock frequency selection.
 */
void evk_set_system_clock(evk_clk_freq_t evk_clk_freq);

/** @brief Get the system clock frequency value.
 *
 *  @return System clock frequency.
 */
uint32_t evk_get_system_clock_freq(void);

/** @brief De-initialize the peripheral USB clock.
 */
void evk_usb_clock_deinit(void);

/** @brief De-initialize the external PLL oscillator clock.
 */
void evk_clock_ext_osc_deinit(void);

/** @brief Set the PLL select pin to enable the external oscillator clock.
 *
 *  This function turns on the 20.48MHz oscillator.
 *  The oscillator provides an optional external PLL for the radio.
 *  When the oscillator is on, the radio should deactivate its internal PLL.
 */
void evk_enable_ext_osc_clk(void);

/** @brief Reset the PLL select pin to disable the external oscillator clock.
 *
 *  This function turns off the 20.48MHz oscillator.
 *  When the oscillator is off, the radio should activate its internal PLL.
 */
void evk_disable_ext_osc_clk(void);

/** @brief De-initialize the external XTAL clock.
 */
void evk_clock_xtal_deinit(void);

/** @brief Enable the XTAL clock.
 *
 *  This signal is a PWM with a 50% duty cycle and a 32.768kHz frequency.
 *  It provides an optional external XTAL clock for the radio.
 *  When the XTAL clock is on, the radio should deactivate its internal XTAL clock.
 */
void evk_enable_xtal_clk(void);

/** @brief Disable the XTAL clock.
 *
 *  This function turns off the XTAL clock PWM signal.
 *  When the XTAL clock is off, the radio should activate its internal XTAL clock.
 */
void evk_disable_xtal_clk(void);

/** @brief Select mux PLL 170Mhz input for system clock.
 *
 *  @note This function is made to work in tandem with evk_clk_set_hsi_as_sysclk.
 */
void evk_clock_set_pll_170Mhz_as_sysclk(void);

/** @brief Select mux HSI input for system clock.
 *
 *  @note This function is made to work in tandem with evk_clk_set_pll_170Mhz_as_sysclk.
 */
void evk_clock_set_hsi_as_sysclk(void);

/** @brief Select mux HSI input for system clock and make sure no spi transfer
 *         are on going.
 *
 *  @note Protected version for the 170Mhz and hsi clock switching version.
 */
void evk_clock_set_hsi_as_sysclk_protected(void);

/** @brief Start RTC  wake up timer.
 *
 *  @param[in] ms  Milliseconds until wake up.
 *
 *  @note Upon wake up the system resets and starts from main.
 *        The maximum value is 30 seconds (30000).
 */
void evk_clock_start_rtc_for_wakeup(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* EVK_CLOCK_H_ */

