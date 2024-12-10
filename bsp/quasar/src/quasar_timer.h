/** @file  quasar_timer.h
 *  @brief This module provides BSP API functions for everything related to free running timers and delays.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_TIMER_H_
#define QUASAR_TIMER_H_

/* INCLUDES *******************************************************************/
#include "quasar_def.h"
#include "quasar_it.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Get timebase milliseconds tick value.
 *
 *  @return Tick value.
 */
uint32_t quasar_timer_get_ms_tick(void);

/** @brief Blocking delay with a 1ms resolution.
 *
 *  @param[in] delay  Delay in milli seconds to wait.
 */
void quasar_timer_delay_ms(uint32_t delay);

/** @brief Initialize the free running ms timer.
 *
 *  @param[in] irq_priority  Free running ms timer priority.
 */
void quasar_timer_free_running_ms_init(quasar_irq_priority_t irq_priority);

/** @brief Get the current free running ms timer tick count.
 *
 *  @return The free running ms timer tick count.
 */
uint64_t quasar_timer_free_running_ms_get_tick_count(void);

/** @brief Initialize the timer for dual-radio support.
 *
 *  @param[in] irq_priority  Priority of the multi radio timer.
 */
void quasar_timer_multi_radio_init(quasar_irq_priority_t irq_priority);

/** @brief Initialize the blocking delay ms timer.
 *
 *  @param[in] irq_priority  Blocking delay ms timer priority.
 */
void quasar_timer_blocking_delay_init(quasar_irq_priority_t irq_priority);

/** @brief Get the multi-radio timer frequency from the system clock and prescaler.
 *
 *  @return  Multi-radio timer frequency.
 */
uint32_t quasar_timer_multi_radio_get_freq_hz(void);

/** @brief Set the multi-radio timer callback.
 *
 *  @param[in] callback  Multi-radio callback.
 */
void quasar_timer_multi_radio_set_callback(void (*callback)(void));

/** @brief Set the timer period for dual-radio support.
 *
 *  @param[in] period  Set the timer Period.
 */
void quasar_timer_multi_radio_set_period(uint16_t period);

/** @brief Set the timer prescaler for dual-radio support.
 *
 *  @param[in] period  Set the timer Period.
 */
void quasar_timer_multi_radio_set_prescaler(uint16_t prescaler);

/** @brief Get prescaler value of the multi-radio timer.
 *
 *  @return prescaler value.
 */
uint32_t quasar_timer_multi_radio_get_prescaler(void);

/** @brief Start the dual-radio timer.
 */
void quasar_timer_multi_radio_start(void);

/** @brief Stop the dual-radio timer.
 */
void quasar_timer_multi_radio_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_TIMER_H_ */

