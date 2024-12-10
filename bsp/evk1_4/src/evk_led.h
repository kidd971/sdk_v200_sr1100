/** @file  evk_led.h
 *  @brief This module controls LED features of SPARK EVK board.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef EVK_LED_H_
#define EVK_LED_H_

/* INCLUDES *******************************************************************/
#include "evk_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initializes the on-board LEDs.
 */
void evk_led_init(void);

/** @brief Turn on LED.
 *
 *  @param[in] led  LED ID number.
 */
void evk_led_on(evk_led_t led);

/** @brief Turn off LED.
 *
 *  @param[in] led  LED ID number.
 */
void evk_led_off(evk_led_t led);

/** @brief Toggle LED.
 *
 *  @param[in] led  LED ID number.
 */
void evk_led_toggle(evk_led_t led);

#ifdef __cplusplus
}
#endif

#endif /* EVK_LED_H_ */

