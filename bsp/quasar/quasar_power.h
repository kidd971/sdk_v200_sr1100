/** @file quasar_power.h
 *  @brief This module provides functions to manage power features.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_POWER_H
#define QUASAR_POWER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "quasar_adc.h"

/* TYPES **********************************************************************/
/** @brief Board's VDD selection.
 */
typedef enum quasar_vdd_selection {
    QUASAR_VDD_SELECTION_1V8,
    QUASAR_VDD_SELECTION_3V3
} quasar_vdd_selection_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize power related features.
 */
void quasar_power_up(void);

/** @brief Initialize GPIOs related to enable pins of LDO assosiated with MCU's analog circuitry and USB and LEDs.
 *
 *  @note The GPIO used for LDO enable differs depending on the board revision. Both GPIOs are initialized in this
 *        function, and once the board revision is known, the GPIO that is not used for MCU LDO enable must be
 *        deinitialized.
 */
void quasar_power_init_gpios(void);

/** @brief Initialize the VDD select GPIO.
 *
 *  @param[in] quasar_vdd_selection  Select the board's VDD level.
 */
void quasar_power_set_vdd_level(quasar_vdd_selection_t quasar_vdd_selection);

/** @brief Enable the LDO that supplied LEDs with 3V3.
 */
void quasar_power_enable_ldo_led(void);

/** @brief Disable the LDO that supplied LEDs with 3V3.
 */
void quasar_power_disable_ldo_led(void);

/** @brief Enable the LDO that supplied MCU's USB and analog circuitry with 3V3.
 *
 *  @note Depending on the board revision the associated GPIO is used.
 *
 *  @param[in] board_revision  The board revision.
 */
void quasar_power_enable_ldo_mcu(quasar_revision_t board_revision);

/** @brief Disable the LDO that supplied MCU's USB and analog circuitry with 3V3.
 *
 *  @note Depending on the board revision the associated GPIO is used.
 *
 *  @param[in] board_revision  The board revision.
 */
void quasar_power_disable_ldo_mcu(quasar_revision_t board_revision);

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_POWER_H */
