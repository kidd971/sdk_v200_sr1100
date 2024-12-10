/** @file  evk_timer.h
 *  @brief This module controls timer features of the SPARK EVK board.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef EVK_TIMER_H_
#define EVK_TIMER_H_

/* INCLUDES *******************************************************************/
#include "evk_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the free running millisecond timer.
 *
 *  @param[in] irq_priority  Free running timer millisecond interrupt priority.
 */
void evk_timer_free_running_ms_init(uint8_t irq_priority);

/** @brief Initialize the free running quarter millisecond timer.
 *
 *  @param[in] irq_priority  Timer interrupt priority.
 */
void evk_timer_free_running_quarter_ms_init(uint8_t irq_priority);

/** @brief Blocking delay with a 1 millisecond resolution.
 *
 *  @param[in] delay_ms  Delay in milliseconds to wait.
 */
void evk_timer_delay_ms(uint32_t delay_ms);

/** @brief Get timebase tick value.
 *
 *  @return Tick value.
 */
uint32_t evk_timer_get_ms_tick(void);

/** @brief Get the free running timer tick count with a 1 millisecond resolution.
 *
 *  @return Tick count.
 */
uint64_t evk_timer_free_running_ms_get_tick_count(void);

/** @brief Get the free running timer tick count with a 250 microseconds resolution.
 *
 *  @return Tick count.
 */
uint64_t evk_timer_free_running_quarter_ms_get_tick_count(void);

#ifdef __cplusplus
}
#endif

#endif /* EVK_TIMER_H_ */

