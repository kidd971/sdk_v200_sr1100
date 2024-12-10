/** @file  quasar_button.h
 *  @brief This module configures buttons and provides functions to control each of them.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_BUTTON_H_
#define QUASAR_BUTTON_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include "quasar_it.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Quasar BSP button peripherals selection.
 */
typedef enum quasar_button_selection {
    /*! User application button. */
    QUASAR_BUTTON_USER_1,
    /*! User application button. */
    QUASAR_BUTTON_USER_2,
    /*! User application button. */
    QUASAR_BUTTON_USER_3,
    /*! User application button. */
    QUASAR_BUTTON_USER_4,
} quasar_button_selection_t;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
/** @brief Initialize button peripherals.
 */
void quasar_button_init(void);

/** @brief Deinitialize button peripherals.
 */
void quasar_button_deinit(void);

/** @brief Configure and enable the interrupt for the selected button.
 *
 *  @param[in] button_selection  Selected button.
 *  @param[in] irq_priority      IRQ priority.
 */
void quasar_button_configure_irq(quasar_button_selection_t button_selection, quasar_irq_priority_t irq_priority);

/** @brief Enable the interrupt for the selected button.
 *
 *  @param[in] button_selection  Selected button.
 */
void quasar_button_enable_irq(quasar_button_selection_t button_selection);

/** @brief Disable the interrupt for the selected button.
 *
 *  @param[in] button_selection  Selected button.
 */
void quasar_button_disable_irq(quasar_button_selection_t button_selection);

/** @brief Set button 1 interrupt callback.
 *
 *  @param[in] irq_callback  Button 1 callback.
 */
void quasar_button_set_button1_callback(void (*irq_callback)(void));

/** @brief Set button 2 interrupt callback.
 *
 *  @param[in] irq_callback  Button 2 callback.
 */
void quasar_button_set_button2_callback(void (*irq_callback)(void));

/** @brief Set button 3 interrupt callback.
 *
 *  @note The same EXTI is also used for USB detection.
 *
 *  @param[in] irq_callback  Button 3 callback.
 */
void quasar_button_set_button3_callback(void (*irq_callback)(void));

/** @brief Set button 4 interrupt callback.
 *
 *  @param[in] irq_callback  Button 4 callback.
 */
void quasar_button_set_button4_callback(void (*irq_callback)(void));

/** @brief Read button state.
 *
 *  @note The GPIO must have been initialized in input mode.
 *        Since the button is connected to VDD and the GPIO has a pull-down
 *        resistor, the return value is true if the button is pressed, false
 *        otherwise.
 *
 *  @param[in] button_peripheral  Selected button peripheral.
 *  @return Return true if the button is pressed, flase otherwise.
 */
bool quasar_button_read_state(quasar_button_selection_t button_peripheral);

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_BUTTON_H_ */
