/** @file  evk_dac.h
 *  @brief This module controls DAC features of SPARK EVK board.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef EVK_DAC_H_
#define EVK_DAC_H_

/* INCLUDES *******************************************************************/
#include "evk_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize DAC1 peripheral for conversion on Channel 1 (GPIOA PIN4).
 */
void evk_exp_dac1_ch1_init(void);

/** @brief Converts provided value to analog voltage on GPIOA PIN4.
 *
 *  @param[in] data  Value to convert to analog voltage.
 */
void evk_exp_dac1_ch1_convert(uint16_t data);

#ifdef __cplusplus
}
#endif

#endif /* EVK_DAC_H_ */

