/** @file  evk_timer.c
 *  @brief This module controls timer features of the SPARK EVK board.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "evk_timer.h"
#include "evk_clock.h"
#include "evk_it.h"
#include "evk_timer_ext.h"

/* PRIVATE GLOBALS ************************************************************/
static volatile uint64_t free_running_ms_timer_tick_counter;
static volatile uint64_t free_running_quarter_ms_timer_tick_counter;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void free_running_timer_ms_tick_callback(void);
static void free_running_timer_quarter_ms_tick_callback(void);

/* PUBLIC FUNCTIONS ***********************************************************/
void evk_timer_free_running_ms_init(uint8_t irq_priority)
{
    evk_it_set_timer4_callback(free_running_timer_ms_tick_callback);

    evk_timer_cfg_t timer_cfg = {
        .timer_selection = EVK_TIMER_SELECTION_TIMER4,
        .time_base = EVK_TIMER_TIME_BASE_MILLISECOND,
        .time_period = 1,
        .irq_priority = irq_priority,
    };
    evk_timer_init(timer_cfg);

    evk_timer_start(EVK_TIMER_SELECTION_TIMER4);
}

void evk_timer_free_running_quarter_ms_init(uint8_t irq_priority)
{
    evk_it_set_timer6_callback(free_running_timer_quarter_ms_tick_callback);

    evk_timer_cfg_t timer_cfg = {
        .timer_selection = EVK_TIMER_SELECTION_TIMER6,
        .time_base = EVK_TIMER_TIME_BASE_MICROSECOND,
        .time_period = 250,
        .irq_priority = irq_priority,
    };
    evk_timer_init(timer_cfg);

    evk_timer_start(EVK_TIMER_SELECTION_TIMER6);
}

void evk_timer_delay_ms(uint32_t delay_ms)
{
    HAL_Delay(delay_ms);
}

uint32_t evk_timer_get_ms_tick(void)
{
    return HAL_GetTick();
}

uint64_t evk_timer_free_running_ms_get_tick_count(void)
{
    return free_running_ms_timer_tick_counter;
}

uint64_t evk_timer_free_running_quarter_ms_get_tick_count(void)
{
    return free_running_quarter_ms_timer_tick_counter;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Callback for the free running timer tick.
 */
static void free_running_timer_ms_tick_callback(void)
{
    free_running_ms_timer_tick_counter++;
}

/** @brief Callback for the free running quarter timer tick.
 */
static void free_running_timer_quarter_ms_tick_callback(void)
{
    free_running_quarter_ms_timer_tick_counter++;
}
