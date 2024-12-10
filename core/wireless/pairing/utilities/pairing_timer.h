/** @file  pairing_timer.h
 *  @brief This file handles the time management for pairing module such as the timeout.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef PAIRING_TIMER_H_
#define PAIRING_TIMER_H_

/* INCLUDES *******************************************************************/
#include "swc_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the time manager module.
 *
 *  @param[in] timeout_sec  The duration in seconds after which the timeout occurs.
 */
void pairing_timer_init(uint16_t timeout_sec);

/** @brief Get the current tick count from the HAL free running timer.
 *
 *  @return The current tick count from the HAL free running timer.
 */
uint32_t pairing_timer_get_current_timer_tick_count(void);

/** @brief Calculate if the pairing timeout was reached.
 *
 *  @return the pairing timeout is reached if true, false otherwise.
 */
bool pairing_timer_is_timeout(void);

/** @brief Blocking delay in milliseconds.
 *
 *  @param[in] delay_ms  Delay period in milliseconds.
 */
void pairing_timer_blocking_delay_ms(uint16_t delay_ms);

#ifdef __cplusplus
}
#endif

#endif /* PAIRING_TIMER_H_ */
