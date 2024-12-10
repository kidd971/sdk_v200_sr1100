/** @file  evk_power.h
 *  @brief This module controls the power features of the SPARK EVK board.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef EVK_POWER_H_
#define EVK_POWER_H_

/* INCLUDES *******************************************************************/
#include "evk_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize GPIOs for the voltage select feature.
 *
 * @param[in] vdd  Select whether the board runs on 1.8V or 3.3V.
 *      @li VDD_1V8
 *      @li VDD_3V3
 */
void evk_power_vdd_select_gpio_init(evk_vdd_t vdd);

/** @brief Select board voltage between 1.8V and 3.3V.
 *
 *  @param[in] vdd  Select whether the board runs on 1.8V or 3.3V.
 *      @li VDD_1V8
 *      @li VDD_3V3
 */
void evk_power_set_board_voltage(evk_vdd_t vdd);

/** @brief Put the system into shutdown and wake up after X ms is reached.
 *
 *  @param[in] ms  Milliseconds until wake up.
 *
 *  @note Upon wake up the system resets and starts from main.
 *        The maximum value is 30 seconds (30000).
 */
void evk_power_shutdown_and_wake_up(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* EVK_POWER_H_ */

