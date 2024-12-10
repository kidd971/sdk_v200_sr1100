/** @file  evk_timer_ext.c
 *  @brief This module extension provides an easy way to implement timers.
 *
 *  @note The system frequency must not be less than 1MHz.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "evk_timer_ext.h"
#include "evk_clock.h"

/* PRIVATE GLOBALS ************************************************************/
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;
TIM_HandleTypeDef htim5;
TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim7;
TIM_HandleTypeDef htim8;
TIM_HandleTypeDef htim15;
TIM_HandleTypeDef htim16;
TIM_HandleTypeDef htim17;
TIM_HandleTypeDef htim20;

/* CONSTANT *******************************************************************/
#define TIME_BASE_MILLISECOND_DIVIDER    10000
#define TIME_BASE_MILLISECOND_MULTIPLIER 10
#define TIME_BASE_MICROSECOND_DIVIDER    1000000
#define TIME_BASE_MICROSECOND_MULTIPLIER 1

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void calculate_period_and_prescaler(uint16_t *prescaler, uint16_t *period, evk_timer_cfg_t timer_cfg);
static TIM_HandleTypeDef *get_selected_timer_handle(evk_timer_selection_t timer_selection);
static void init_selected_timer_handle(TIM_HandleTypeDef *timer_handle, evk_timer_selection_t timer_selection);
static IRQn_Type get_selected_timer_irq(evk_timer_selection_t timer_selection);

/* PUBLIC FUNCTIONS ***********************************************************/
void evk_timer_init(evk_timer_cfg_t timer_cfg)
{
    uint16_t prescaler = 0;
    uint16_t period = 0;
    uint8_t timer_selection = 0;
    TIM_HandleTypeDef *timer_handle;
    IRQn_Type timer_irq;

    /* Avoid invalid parameters. */
    if (timer_cfg.time_period == 0) {
        return;
    }

    /* Get the selected timer. */
    timer_selection = timer_cfg.timer_selection;

    /* Calculate the prescaler and period value. */
    calculate_period_and_prescaler(&prescaler, &period, timer_cfg);

    /* Configure the timer parameters. */
    timer_handle = get_selected_timer_handle(timer_selection);
    if (timer_handle == NULL) {
        return;
    }
    init_selected_timer_handle(timer_handle, timer_selection);
    timer_handle->Init.Prescaler = prescaler;
    timer_handle->Init.Period = period;
    timer_handle->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    timer_handle->Init.CounterMode = TIM_COUNTERMODE_UP;

    if (HAL_TIM_Base_Init(timer_handle) != HAL_OK) {
        while (1);
    }

    /* Configure the timer IRQ. */
    if (timer_cfg.irq_priority != PRIO_TIMER_NO_IRQ) {
        timer_irq = get_selected_timer_irq(timer_selection);
        HAL_NVIC_SetPriority(timer_irq, timer_cfg.irq_priority, 0);
        HAL_NVIC_EnableIRQ(timer_irq);
    }
}

void evk_timer_start(evk_timer_selection_t timer_selection)
{
    TIM_HandleTypeDef *timer_handle;

    timer_handle = get_selected_timer_handle(timer_selection);

    if (timer_handle != NULL) {
        HAL_TIM_Base_Start_IT(timer_handle);
    }
}

void evk_timer_stop(evk_timer_selection_t timer_selection)
{
    TIM_HandleTypeDef *timer_handle;

    timer_handle = get_selected_timer_handle(timer_selection);

    if (timer_handle != NULL) {
        HAL_TIM_Base_Stop_IT(timer_handle);
    }
}

void evk_timer_enable_interrupt(evk_timer_selection_t timer_selection)
{
    IRQn_Type timer_irq;

    timer_irq = get_selected_timer_irq(timer_selection);
    HAL_NVIC_EnableIRQ(timer_irq);
}

void evk_timer_disable_interrupt(evk_timer_selection_t timer_selection)
{
    IRQn_Type timer_irq;

    timer_irq = get_selected_timer_irq(timer_selection);
    HAL_NVIC_DisableIRQ(timer_irq);
}

void evk_timer_reset_count(evk_timer_selection_t timer_selection)
{
    TIM_HandleTypeDef *timer_handle;

    timer_handle = get_selected_timer_handle(timer_selection);

    if (timer_handle != NULL) {
        timer_handle->Instance->CNT = 0;
    }
}

uint32_t evk_timer_get_count(evk_timer_selection_t timer_selection)
{
    TIM_HandleTypeDef *timer_handle;
    uint32_t ret = 0;

    timer_handle = get_selected_timer_handle(timer_selection);

    if (timer_handle != NULL) {
        ret = timer_handle->Instance->CNT;
    }
    return ret;
}

void evk_timer_set_period(evk_timer_selection_t timer_selection, uint16_t period)
{
    TIM_HandleTypeDef *timer_handle;

    timer_handle = get_selected_timer_handle(timer_selection);

    if (timer_handle != NULL) {
        timer_handle->Instance->ARR = (uint32_t)period - 1;
    }
}

void evk_timer_set_prescaler(evk_timer_selection_t timer_selection, uint16_t prescaler)
{
    TIM_HandleTypeDef *timer_handle;

    timer_handle = get_selected_timer_handle(timer_selection);

    if (timer_handle != NULL) {
        timer_handle->Instance->PSC = (uint32_t)prescaler - 1;
    }
}

uint32_t evk_timer_get_period(evk_timer_selection_t timer_selection)
{
    TIM_HandleTypeDef *timer_handle;

    timer_handle = get_selected_timer_handle(timer_selection);

    return timer_handle->Instance->ARR + 1;
}

uint32_t evk_timer_get_prescaler(evk_timer_selection_t timer_selection)
{
    TIM_HandleTypeDef *timer_handle;

    timer_handle = get_selected_timer_handle(timer_selection);

    return timer_handle->Instance->PSC + 1;
}

/* PRIVATE FUNCTION ***********************************************************/
/** @brief Calculate the period and prescaler based on the time period and the system clock.
 *
 *  Formula: update_event (Hz) = system_clock / ((prescaler + 1) * (period_arr + 1))
 *
 *  @param[out] prescaler  Calculated prescaler value.
 *  @param[out] period     Calculated period value.
 *  @param[in]  timer_cfg  Configuration for the time base and time period.
 */
static void calculate_period_and_prescaler(uint16_t *prescaler, uint16_t *period, evk_timer_cfg_t timer_cfg)
{
    uint32_t clock_frequency = 0;
    uint32_t divider = 0;
    uint8_t multiplier = 0;

    /* Get the system clock frequency. */
    clock_frequency = evk_get_system_clock_freq();

    switch (timer_cfg.time_base) {
    case EVK_TIMER_TIME_BASE_MILLISECOND:
        divider = TIME_BASE_MILLISECOND_DIVIDER;
        multiplier = TIME_BASE_MILLISECOND_MULTIPLIER;
        break;
    case EVK_TIMER_TIME_BASE_MICROSECOND:
        divider = TIME_BASE_MICROSECOND_DIVIDER;
        multiplier = TIME_BASE_MICROSECOND_MULTIPLIER;
        break;
    default:
        /* Time base not supported. */
        return;
        break;
    }

    /*
     * Calculate the period and prescaler. The divider and multiplier are used
     * to limit the period and prescaler within a 16-bit register.
     */
    *period = ((timer_cfg.time_period * multiplier) - 1);
    *prescaler = ((clock_frequency / divider) - 1);
}

/** @brief Return the handle from the selected timer.
 *
 *  @param[in] timer_selection  Available timer selection.
 *  @return Selected timer handle.
 */
static TIM_HandleTypeDef *get_selected_timer_handle(evk_timer_selection_t timer_selection)
{
    TIM_HandleTypeDef *timer_handle = {0};

    switch (timer_selection) {
    case EVK_TIMER_SELECTION_TIMER1:
        timer_handle = &htim1;
        break;
    case EVK_TIMER_SELECTION_TIMER2:
        timer_handle = &htim2;
        break;
    case EVK_TIMER_SELECTION_TIMER3:
        timer_handle = &htim3;
        break;
    case EVK_TIMER_SELECTION_TIMER4:
        timer_handle = &htim4;
        break;
    case EVK_TIMER_SELECTION_TIMER5:
        timer_handle = &htim5;
        break;
    case EVK_TIMER_SELECTION_TIMER6:
        timer_handle = &htim6;
        break;
    case EVK_TIMER_SELECTION_TIMER7:
        timer_handle = &htim7;
        break;
    case EVK_TIMER_SELECTION_TIMER8:
        timer_handle = &htim8;
        break;
    case EVK_TIMER_SELECTION_TIMER15:
        timer_handle = &htim15;
        break;
    case EVK_TIMER_SELECTION_TIMER16:
        timer_handle = &htim16;
        break;
    case EVK_TIMER_SELECTION_TIMER17:
        timer_handle = &htim17;
        break;
    case EVK_TIMER_SELECTION_TIMER20:
        timer_handle = &htim20;
        break;
    default:
        /* Unimplemented timer. */
        break;
    }

    return timer_handle;
}

/** @brief Initialize the selected timer handle.
 *
 *  @param[in] timer_handle     Initialize the timer handle.
 *  @param[in] timer_selection  Available timer selection.
 */
static void init_selected_timer_handle(TIM_HandleTypeDef *timer_handle, evk_timer_selection_t timer_selection)
{
    switch (timer_selection) {
    case EVK_TIMER_SELECTION_TIMER1:
        __HAL_RCC_TIM1_CLK_ENABLE();
        timer_handle->Instance = TIM1;
        break;
    case EVK_TIMER_SELECTION_TIMER2:
        __HAL_RCC_TIM2_CLK_ENABLE();
        timer_handle->Instance = TIM2;
        break;
    case EVK_TIMER_SELECTION_TIMER3:
        __HAL_RCC_TIM3_CLK_ENABLE();
        timer_handle->Instance = TIM3;
        break;
    case EVK_TIMER_SELECTION_TIMER4:
        __HAL_RCC_TIM4_CLK_ENABLE();
        timer_handle->Instance = TIM4;
        break;
    case EVK_TIMER_SELECTION_TIMER5:
        __HAL_RCC_TIM5_CLK_ENABLE();
        timer_handle->Instance = TIM5;
        break;
    case EVK_TIMER_SELECTION_TIMER6:
        __HAL_RCC_TIM6_CLK_ENABLE();
        timer_handle->Instance = TIM6;
        break;
    case EVK_TIMER_SELECTION_TIMER7:
        __HAL_RCC_TIM7_CLK_ENABLE();
        timer_handle->Instance = TIM7;
        break;
    case EVK_TIMER_SELECTION_TIMER8:
        __HAL_RCC_TIM8_CLK_ENABLE();
        timer_handle->Instance = TIM8;
        break;
    case EVK_TIMER_SELECTION_TIMER15:
        __HAL_RCC_TIM15_CLK_ENABLE();
        timer_handle->Instance = TIM15;
        break;
    case EVK_TIMER_SELECTION_TIMER16:
        __HAL_RCC_TIM16_CLK_ENABLE();
        timer_handle->Instance = TIM16;
        break;
    case EVK_TIMER_SELECTION_TIMER17:
        __HAL_RCC_TIM17_CLK_ENABLE();
        timer_handle->Instance = TIM17;
        break;
    case EVK_TIMER_SELECTION_TIMER20:
        __HAL_RCC_TIM20_CLK_ENABLE();
        timer_handle->Instance = TIM20;
        break;
    default:
        /* Unimplemented timer. */
        break;
    }
}

/** @brief Return the selected timer's global interrupt.
 *
 *  @param[in] timer_selection  Available timer selection.
 *  @return Selected timer global interrupt.
 */
static IRQn_Type get_selected_timer_irq(evk_timer_selection_t timer_selection)
{
    IRQn_Type timer_irq = {0};

    switch (timer_selection) {
    case EVK_TIMER_SELECTION_TIMER1:
        timer_irq = TIM1_UP_TIM16_IRQn;
        break;
    case EVK_TIMER_SELECTION_TIMER2:
        timer_irq = TIM2_IRQn;
        break;
    case EVK_TIMER_SELECTION_TIMER3:
        timer_irq = TIM3_IRQn;
        break;
    case EVK_TIMER_SELECTION_TIMER4:
        timer_irq = TIM4_IRQn;
        break;
    case EVK_TIMER_SELECTION_TIMER5:
        timer_irq = TIM5_IRQn;
        break;
    case EVK_TIMER_SELECTION_TIMER6:
        timer_irq = TIM6_DAC_IRQn;
        break;
    case EVK_TIMER_SELECTION_TIMER7:
        timer_irq = TIM7_DAC_IRQn;
        break;
    case EVK_TIMER_SELECTION_TIMER8:
        timer_irq = TIM8_UP_IRQn;
        break;
    case EVK_TIMER_SELECTION_TIMER15:
        timer_irq = TIM1_BRK_TIM15_IRQn;
        break;
    case EVK_TIMER_SELECTION_TIMER16:
        timer_irq = TIM1_UP_TIM16_IRQn;
        break;
    case EVK_TIMER_SELECTION_TIMER17:
        timer_irq = TIM1_TRG_COM_TIM17_IRQn;
        break;
    case EVK_TIMER_SELECTION_TIMER20:
        timer_irq = TIM20_UP_IRQn;
        break;
    default:
        /* Unimplemented timer. */
        break;
    }

    return timer_irq;
}
