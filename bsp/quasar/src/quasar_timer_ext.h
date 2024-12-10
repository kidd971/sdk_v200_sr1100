/** @file  quasar_timer.h
 *  @brief This module provides functions to control and configure basic timers.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_TIMER_EXT_H_
#define QUASAR_TIMER_EXT_H_

/* INCLUDES *******************************************************************/
#include "quasar_def.h"
#include "quasar_it.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief List of all available timers.
 */
typedef enum quasar_timer_selection {
    /*! Select the 16-bits timer 1. */
    QUASAR_TIMER_SELECTION_TIMER1,
    /*! Select the 32-bits timer 2. */
    QUASAR_TIMER_SELECTION_TIMER2,
    /*! Select the 32-bits timer 3. */
    QUASAR_TIMER_SELECTION_TIMER3,
    /*! Select the 32-bits timer 4. */
    QUASAR_TIMER_SELECTION_TIMER4,
    /*! Select the 32-bits timer 5. */
    QUASAR_TIMER_SELECTION_TIMER5,
    /*! Select the 16-bits timer 6. */
    QUASAR_TIMER_SELECTION_TIMER6,
    /*! Select the 16-bits timer 7. */
    QUASAR_TIMER_SELECTION_TIMER7,
    /*! Select the 16-bits timer 8. */
    QUASAR_TIMER_SELECTION_TIMER8,
    /*! Select the 16-bits timer 15. */
    QUASAR_TIMER_SELECTION_TIMER15,
    /*! Select the 16-bits timer 16. */
    QUASAR_TIMER_SELECTION_TIMER16,
    /*! Select the 16-bits timer 17. */
    QUASAR_TIMER_SELECTION_TIMER17
} quasar_timer_selection_t;

/** @brief Available time base for a timer.
 */
typedef enum quasar_timer_time_base {
    /*! Time base is in milliseconds. */
    QUASAR_TIMER_TIME_BASE_MILLISECOND,
    /*! Time base is in microseconds. */
    QUASAR_TIMER_TIME_BASE_MICROSECOND,
} quasar_timer_time_base_t;

/** @brief Configuration settings for a timer.
 */
typedef struct quasar_timer_config {
    /*! Available timers. */
    quasar_timer_selection_t timer_selection;
    /*! Select the time base for the time period. */
    quasar_timer_time_base_t time_base;
    /*! Select the time period based on the selected time base. */
    uint16_t time_period;
    /*! Available IRQ priority. */
    quasar_irq_priority_t irq_priority;
} quasar_timer_config_t;

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Initialize the timer as a basic timer.
 *
 *  @param[in] timer_config  Configuration of the timer.
 */
void quasar_timer_init(quasar_timer_config_t *timer_config);

/** @brief Deinitialize the timer.
 *
 *  @param[in] timer_config  Configuration of the timer.
 */
void quasar_timer_deinit(quasar_timer_config_t timer_config);

/** @brief Enable the selected timer interrupt.
 *
 *  @note The interrupt are enabled by default in the timer initialization.
 *
 *  @param[in] timer_selection  Available timer selection.
 */
void quasar_timer_enable_interrupt(quasar_timer_selection_t timer_selection);

/** @brief Disable the selected timer interrupt.
 *
 *  @note The interrupt are enabled by default in the timer initialization.
 *
 *  @param[in] timer_selection  Available timer selection.
 */
void quasar_timer_disable_interrupt(quasar_timer_selection_t timer_selection);

/** @brief Start the selected timer.
 *
 *  @param[in] timer_selection  Selected timer to start.
 */
void quasar_timer_start(quasar_timer_selection_t timer_selection);

/** @brief Stop the selected timer.
 *
 *  @param[in] timer_selection  Selected timer to stop.
 */
void quasar_timer_stop(quasar_timer_selection_t timer_selection);

/** @brief Return the instance to the selected timer instance.
 *
 *  @param[in] timer_selection  Selected timer to get associated instance.
 *  @return Selected timer instance.
 */
TIM_TypeDef *quasar_timer_get_instance(quasar_timer_selection_t timer_selection);

/** @brief Manually set the period (Auto-Reload Register) register value.
 *
 *  This function is used when the timer needs to be fine tuned.
 *  This function automatically adds the -1 to the period value.
 *
 *  @param[in] timer_selection  Available timer selection.
 *  @param[in] period           Period (Auto-Reload Register) value.
 */
void quasar_timer_set_period(quasar_timer_selection_t timer_selection, uint16_t period);

/** @brief Get the selected timer period (Auto-Reload Register) register value.
 *
 *  This function automatically adds the +1 to the period value.
 *
 *  @param[in] timer_selection  Available timer selection.
 *  @return Period register value.
 */
uint32_t quasar_timer_get_period(quasar_timer_selection_t timer_selection);

/** @brief Manually set the prescaler register value.
 *
 *  This function is used when the timer needs to be fine tuned.
 *  This function automatically adds the -1 to the prescaler value.
 *
 *  @param[in] timer_selection  Available timer selection.
 *  @param[in] prescaler        Prescaler register value.
 */
void quasar_timer_set_prescaler(quasar_timer_selection_t timer_selection, uint16_t prescaler);

/** @brief Get the selected timer prescaler register value.
 *
 *  This function automatically adds the +1 to the prescaler value.
 *
 *  @param[in] timer_selection  Available timer selection.
 *  @return Prescaler register value.
 */
uint32_t quasar_timer_get_prescaler(quasar_timer_selection_t timer_selection);

/** @brief Reset the selected timer count value.
 *
 *  @param[in] timer_selection  Available timer selection.
 */
void quasar_timer_reset_count(quasar_timer_selection_t timer_selection);

/** @brief Get the selected timer count value.
 *
 *  @param[in] timer_selection  Available timer selection.
 *  @return Timer count value.
 */
uint32_t quasar_timer_get_count(quasar_timer_selection_t timer_selection);

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_TIMER_EXT_H_ */
