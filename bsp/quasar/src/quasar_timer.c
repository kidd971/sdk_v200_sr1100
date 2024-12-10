/** @file  quasar_timer.c
 *  @brief This module provides BSP API functions for everything related to free running timers and delays.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_it.h"
#include "quasar_timer.h"
#include "quasar_timer_ext.h"
#include "quasar_clock.h"

/* PRIVATE GLOBALS ************************************************************/
static volatile uint64_t free_running_ms_timer_tick_counter;
static volatile uint64_t free_running_quarter_ms_timer_tick_counter;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void free_running_timer_ms_tick_callback(void);

/* PUBLIC FUNCTIONS ***********************************************************/
uint32_t quasar_timer_get_ms_tick(void)
{
    return HAL_GetTick();
}

void quasar_timer_delay_ms(uint32_t delay)
{
    HAL_Delay(delay);
}

void quasar_timer_free_running_ms_init(quasar_irq_priority_t irq_priority)
{
    quasar_it_set_timer8_callback(free_running_timer_ms_tick_callback);

    quasar_timer_config_t timer_cfg = {
        .timer_selection = QUASAR_DEF_TIMER_SELECTION_FREE_RUNNING_MS,
        .time_base = QUASAR_TIMER_TIME_BASE_MILLISECOND,
        .time_period = 1,
        .irq_priority = irq_priority,
    };
    quasar_timer_init(&timer_cfg);

    quasar_timer_start(QUASAR_DEF_TIMER_SELECTION_FREE_RUNNING_MS);
}

uint64_t quasar_timer_free_running_ms_get_tick_count(void)
{
    return free_running_ms_timer_tick_counter;
}

void quasar_timer_multi_radio_init(quasar_irq_priority_t irq_priority)
{
    quasar_timer_config_t timer_cfg = {
        .timer_selection = QUASAR_DEF_TIMER_SELECTION_MULTI_RADIO,
        .time_base = QUASAR_TIMER_TIME_BASE_MICROSECOND,
        .time_period = 0xFFFE, /* Dummy value, period is set dynamically. */
        .irq_priority = irq_priority,
    };
    quasar_timer_init(&timer_cfg);
}

void quasar_timer_blocking_delay_init(quasar_irq_priority_t irq_priority)
{
    quasar_it_set_timer2_callback(HAL_IncTick);

    quasar_timer_config_t timer_cfg = {
        .timer_selection = QUASAR_DEF_TIMER_SELECTION_BLOCKING_DELAY,
        .time_base = QUASAR_TIMER_TIME_BASE_MILLISECOND,
        .time_period = 1,
        .irq_priority = irq_priority,
    };
    quasar_timer_init(&timer_cfg);

    quasar_timer_start(QUASAR_DEF_TIMER_SELECTION_BLOCKING_DELAY);
}

void quasar_timer_multi_radio_set_callback(void (*callback)(void))
{
    quasar_it_set_timer4_callback(callback);
}

void quasar_timer_multi_radio_set_period(uint16_t period)
{
    quasar_timer_set_period(QUASAR_DEF_TIMER_SELECTION_MULTI_RADIO, period);
}

void quasar_timer_multi_radio_set_prescaler(uint16_t prescaler)
{
    quasar_timer_set_prescaler(QUASAR_DEF_TIMER_SELECTION_MULTI_RADIO, prescaler);
}

uint32_t quasar_timer_multi_radio_get_prescaler(void)
{
    return quasar_timer_get_prescaler(QUASAR_DEF_TIMER_SELECTION_MULTI_RADIO);
}

uint32_t quasar_timer_multi_radio_get_freq_hz(void)
{
    uint32_t clock_frequency = 0;
    uint32_t prescaler = 0;

    /* Get the system clock frequency. */
    clock_frequency = quasar_clock_get_system_clock_freq();
    prescaler = quasar_timer_get_prescaler(QUASAR_DEF_TIMER_SELECTION_MULTI_RADIO);

    return (clock_frequency / prescaler);
}

void quasar_timer_multi_radio_start(void)
{
    quasar_timer_start(QUASAR_DEF_TIMER_SELECTION_MULTI_RADIO);
}

void quasar_timer_multi_radio_stop(void)
{
    quasar_timer_stop(QUASAR_DEF_TIMER_SELECTION_MULTI_RADIO);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Callback for the free running timer tick.
 */
static void free_running_timer_ms_tick_callback(void)
{
    free_running_ms_timer_tick_counter++;
}

/* ST HAL WEAK FUNCTIONS IMPLEMENTATION ***************************************/
/** @brief This function configures the TIM2 as a time base source.
 *         The time source is configured to have 1ms time base with a dedicated
 *         Tick interrupt priority.
 *
 *  @note This function is called automatically at the beginning of program after
 *        reset by HAL_Init() or at any time when clock is configured, by HAL_RCC_ClockConfig().
 *
 *  @param[in] TickPriority  Tick interrupt priority.
 *  @retval HAL status.
 */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    /* It is assumed that TickPriority is a valid value. */
    quasar_timer_blocking_delay_init(TickPriority);

    /* It is assumed that the initialization is successful. */
    return HAL_OK;
}

/** @brief Suspend Tick increment.
 *
 *  @note Disable the tick increment by disabling TIM2 update interrupt.
 */
void HAL_SuspendTick(void)
{
    /* Disable TIM2 update Interrupt */
    quasar_timer_disable_interrupt(QUASAR_DEF_TIMER_SELECTION_BLOCKING_DELAY);
}

/** @brief Resume Tick increment.
 *
 *  @note Enable the tick increment by Enabling TIM2 update interrupt.
 */
void HAL_ResumeTick(void)
{
    /* Enable TIM2 Update interrupt. */
    quasar_timer_enable_interrupt(QUASAR_DEF_TIMER_SELECTION_BLOCKING_DELAY);
}

