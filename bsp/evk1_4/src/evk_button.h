/** @file  evk_button.h
 *  @brief This module controls button feature of the SPARK EVK board.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef EVK_BUTTON_H_
#define EVK_BUTTON_H_

/* INCLUDES *******************************************************************/
#include "evk_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the GPIOs for the buttons.
 */
void evk_buttons_init(void);

/** @brief Read the input from the user button.
 *
 *  @param[in] btn  Button number.
 *  @retval true   Button is pressed.
 *  @retval false  Button is not pressed.
 */
bool evk_read_btn_state(evk_btn_t btn);

#ifdef __cplusplus
}
#endif

#endif /* EVK_BUTTON_H_ */

