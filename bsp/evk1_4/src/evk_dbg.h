/** @file  evk_dbg.h
 *
 *  @brief This module controls debug GPIO of SPARK EVK board.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef EVK_DBG_H_
#define EVK_DBG_H_

/* INCLUDES *******************************************************************/
#include "evk_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initializes the debug pins on the expansion board.
 */
void evk_dbg_debug_pin_init(void);

/** @brief Set a GPIO.
 *
 *  @param[in] gpio  GPIO ID number.
 */
void evk_dbg_on(evk_dbg_t gpio);

/** @brief Toggle io OFF.
 *
 *  @param[in] gpio  GPIO ID number.
 */
void evk_dbg_off(evk_dbg_t gpio);

/** @brief Toggle a GPIO.
 *
 *  @param[in] gpio  GPIO ID number.
 */
void evk_dbg_toggle(evk_dbg_t gpio);

/** @brief Pulse a GPIO.
 *
 *  @param[in] gpio  GPIO ID number.
 */
void evk_dbg_pulse(evk_dbg_t gpio);

#ifdef __cplusplus
}
#endif

#endif /* EVK_DBG_H_ */

