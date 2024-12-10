/** @file  evk_timer_ext.h
 *  @brief This module extension provides an easy way to implement timers.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef EVK_TIMER_EXT_H_
#define EVK_TIMER_EXT_H_

/* INCLUDES *******************************************************************/
#include "evk_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief List of all available timers.
 */
typedef enum evk_timer_selection {
    /*!< Select the 16-bits timer 1. */
    EVK_TIMER_SELECTION_TIMER1  = 1,
    /*!< Select the 32-bits timer 2. */
    EVK_TIMER_SELECTION_TIMER2  = 2,
    /*!< Select the 32-bits timer 3. */
    EVK_TIMER_SELECTION_TIMER3  = 3,
    /*!< Select the 32-bits timer 4. */
    EVK_TIMER_SELECTION_TIMER4  = 4,
    /*!< Select the 32-bits timer 5. */
    EVK_TIMER_SELECTION_TIMER5  = 5,
    /*!< Select the 16-bits timer 6. */
    EVK_TIMER_SELECTION_TIMER6  = 6,
    /*!< Select the 16-bits timer 7. */
    EVK_TIMER_SELECTION_TIMER7  = 7,
    /*!< Select the 16-bits timer 8. */
    EVK_TIMER_SELECTION_TIMER8  = 8,
    /*!< Select the 16-bits timer 15. */
    EVK_TIMER_SELECTION_TIMER15 = 15,
    /*!< Select the 16-bits timer 16. */
    EVK_TIMER_SELECTION_TIMER16 = 16,
    /*!< Select the 16-bits timer 17. */
    EVK_TIMER_SELECTION_TIMER17 = 17,
    /*!< Select the 16-bits timer 20. */
    EVK_TIMER_SELECTION_TIMER20 = 20,
} evk_timer_selection_t;

/** @brief Available time base for a timer.
 */
typedef enum evk_timer_time_base {
    /*!< Time base is in milliseconds. */
    EVK_TIMER_TIME_BASE_MILLISECOND,
    /*!< Time base is in microseconds. */
    EVK_TIMER_TIME_BASE_MICROSECOND,
} evk_timer_time_base_t;

/** @brief Configuration settings for a timer.
 */
typedef struct evk_timer_cfg {
    /*!< Available timers. */
    evk_timer_selection_t timer_selection;
    /*!< Select the time base for the time period. */
    evk_timer_time_base_t time_base;
    /*!< Select the time period based on the selected time base. */
    uint16_t time_period;
    /*!< Available IRQ priority. */
    uint32_t irq_priority;
} evk_timer_cfg_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize a timer.
 *
 *  @param[in] timer_cfg  The configuration settings for a timer.
 */
void evk_timer_init(evk_timer_cfg_t timer_cfg);

/** @brief Start the selected timer.
 *
 *  @param[in] timer_selection  Available timer selection.
 */
void evk_timer_start(evk_timer_selection_t timer_selection);

/** @brief Stop the selected timer.
 *
 *  @param[in] timer_selection  Available timer selection.
 */
void evk_timer_stop(evk_timer_selection_t timer_selection);

/** @brief Enable the selected timer interrupt.
 *
 *  @note The interrupt are enabled by default in the timer initialization.
 *
 *  @param[in] timer_selection  Available timer selection.
 */
void evk_timer_enable_interrupt(evk_timer_selection_t timer_selection);

/** @brief Disable the selected timer interrupt.
 *
 *  @note The interrupt are enabled by default in the timer initialization.
 *
 *  @param[in] timer_selection  Available timer selection.
 */
void evk_timer_disable_interrupt(evk_timer_selection_t timer_selection);

/** @brief Reset the selected timer count value.
 *
 *  @param[in] timer_selection  Available timer selection.
 */
void evk_timer_reset_count(evk_timer_selection_t timer_selection);

/** @brief Get the selected timer count value.
 *
 *  @param[in] timer_selection  Available timer selection.
 *  @return Timer count value.
 */
uint32_t evk_timer_get_count(evk_timer_selection_t timer_selection);

/** @brief Manually set the period (Auto-Reload Register) register value.
 *
 *  This function is used when the timer needs to be fine tuned.
 *  This function automatically adds the -1 to the period value.
 *
 *  @param[in] timer_selection  Available timer selection.
 *  @param[in] period           Period (Auto-Reload Register) value.
 */
void evk_timer_set_period(evk_timer_selection_t timer_selection, uint16_t period);

/** @brief Manually set the prescaler register value.
 *
 *  This function is used when the timer needs to be fine tuned.
 *  This function automatically adds the -1 to the prescaler value.
 *
 *  @param[in] timer_selection  Available timer selection.
 *  @param[in] prescaler        Prescaler register value.
 */
void evk_timer_set_prescaler(evk_timer_selection_t timer_selection, uint16_t prescaler);

/** @brief Get the selected timer period (Auto-Reload Register) register value.
 *
 *  This function automatically adds the +1 to the period value.
 *
 *  @param[in] timer_selection  Available timer selection.
 *  @return Period register value.
 */
uint32_t evk_timer_get_period(evk_timer_selection_t timer_selection);

/** @brief Get the selected timer prescaler register value.
 *
 *  This function automatically adds the +1 to the prescaler value.
 *
 *  @param[in] timer_selection  Available timer selection.
 *  @return Prescaler register value.
 */
uint32_t evk_timer_get_prescaler(evk_timer_selection_t timer_selection);

#ifdef __cplusplus
}
#endif

#endif /* EVK_TIMER_EXT_H_ */

