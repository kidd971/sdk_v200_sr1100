/** @file  quasar_timer.c
 *  @brief This module provides functions to control and configure basic timers.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_timer_ext.h"
#include "quasar_clock.h"
#include "quasar_it.h"

/* CONSTANTS ******************************************************************/
#define TIME_BASE_MILLISECOND_DIVIDER    10000
#define TIME_BASE_MILLISECOND_MULTIPLIER 10
#define TIME_BASE_MICROSECOND_DIVIDER    1000000
#define TIME_BASE_MICROSECOND_MULTIPLIER 1

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void timer_enable_clock(quasar_timer_selection_t timer_selection);
static void timer_disable_clock(quasar_timer_selection_t timer_selection);
static void calculate_max_count_and_prescaler(uint16_t *prescaler, uint16_t *max_count, quasar_timer_config_t timer_config);
static IRQn_Type timer_get_selected_irq(quasar_timer_selection_t timer_selection);
static void timer_configure_basic_parameters(quasar_timer_selection_t timer_selection, uint16_t prescaler, uint16_t period);

/* PUBLIC FUNCTIONS ***********************************************************/
void quasar_timer_init(quasar_timer_config_t *timer_config)
{
    uint16_t prescaler = 0;
    uint16_t max_count = 0;
    IRQn_Type timer_irq;

    /* Avoid invalid parameters. */
    if (timer_config->time_period == 0) {
        return;
    }

    /* Activate the clock timer. */
    timer_enable_clock(timer_config->timer_selection);

    /* Calculate the prescaler and max_count value. */
    calculate_max_count_and_prescaler(&prescaler, &max_count, *timer_config);

    /* Configure the maximum count (period) before an update occurs into the timer structure and the prescaler. */
    timer_configure_basic_parameters(timer_config->timer_selection, prescaler, max_count);

    /* Configure the timer IRQ. */
    if (timer_config->irq_priority != QUASAR_IRQ_PRIORITY_NONE) {
        timer_irq = timer_get_selected_irq(timer_config->timer_selection);

        NVIC_SetPriority(timer_irq, timer_config->irq_priority);
        NVIC_EnableIRQ(timer_irq);
    }
}

void quasar_timer_deinit(quasar_timer_config_t timer_config)
{
    quasar_timer_stop(timer_config.timer_selection);

    /* Configure the timer IRQ. */
    if (timer_config.irq_priority != QUASAR_IRQ_PRIORITY_NONE) {
        quasar_timer_disable_interrupt(timer_config.timer_selection);
    }
    timer_disable_clock(timer_config.timer_selection);
}

void quasar_timer_enable_interrupt(quasar_timer_selection_t timer_selection)
{
    IRQn_Type timer_irq = timer_get_selected_irq(timer_selection);

    NVIC_EnableIRQ(timer_irq);
}

void quasar_timer_disable_interrupt(quasar_timer_selection_t timer_selection)
{
    IRQn_Type timer_irq = timer_get_selected_irq(timer_selection);

    NVIC_DisableIRQ(timer_irq);
}

void quasar_timer_start(quasar_timer_selection_t timer_selection)
{
    TIM_TypeDef *timer_instance = quasar_timer_get_instance(timer_selection);

    /* Enable the timer counter. */
    QUASAR_SET_BIT(timer_instance->CR1, TIM_CR1_CEN);
    /* Reset the count. */
    QUASAR_SET_BIT(timer_instance->EGR, TIM_EGR_UG);
    /* Enable end of cycle interruptions. */
    QUASAR_SET_BIT(timer_instance->DIER, TIM_DIER_UIE);
}

void quasar_timer_stop(quasar_timer_selection_t timer_selection)
{
    TIM_TypeDef *timer_instance = quasar_timer_get_instance(timer_selection);

    /* Disable the timer counter. */
    QUASAR_CLEAR_BIT(timer_instance->CR1, TIM_CR1_CEN);
    /* Disable end of cycle interruptions. */
    QUASAR_CLEAR_BIT(timer_instance->DIER, TIM_DIER_UIE);
}

TIM_TypeDef *quasar_timer_get_instance(quasar_timer_selection_t timer_selection)
{
    TIM_TypeDef *timer_instance = NULL;

    switch (timer_selection) {
    case QUASAR_TIMER_SELECTION_TIMER1:
        timer_instance = TIM1;
        break;
    case QUASAR_TIMER_SELECTION_TIMER2:
        timer_instance = TIM2;
        break;
    case QUASAR_TIMER_SELECTION_TIMER3:
        timer_instance = TIM3;
        break;
    case QUASAR_TIMER_SELECTION_TIMER4:
        timer_instance = TIM4;
        break;
    case QUASAR_TIMER_SELECTION_TIMER5:
        timer_instance = TIM5;
        break;
    case QUASAR_TIMER_SELECTION_TIMER6:
        timer_instance = TIM6;
        break;
    case QUASAR_TIMER_SELECTION_TIMER7:
        timer_instance = TIM7;
        break;
    case QUASAR_TIMER_SELECTION_TIMER8:
        timer_instance = TIM8;
        break;
    case QUASAR_TIMER_SELECTION_TIMER15:
        timer_instance = TIM15;
        break;
    case QUASAR_TIMER_SELECTION_TIMER16:
        timer_instance = TIM16;
        break;
    case QUASAR_TIMER_SELECTION_TIMER17:
        timer_instance = TIM17;
        break;
    default:
        /* Unimplemented timer. */
        break;
    }

    return timer_instance;
}

void quasar_timer_set_period(quasar_timer_selection_t timer_selection, uint16_t period)
{
    TIM_TypeDef *timer_instance = quasar_timer_get_instance(timer_selection);

    /* Configure the maximum count of a cycle (period). */
    if (timer_instance != NULL) {
        timer_instance->ARR = (uint32_t)period - 1;
    }
}

uint32_t quasar_timer_get_period(quasar_timer_selection_t timer_selection)
{
    TIM_TypeDef *timer_instance = quasar_timer_get_instance(timer_selection);

    return timer_instance->ARR + 1;
}

void quasar_timer_set_prescaler(quasar_timer_selection_t timer_selection, uint16_t prescaler)
{
    TIM_TypeDef *timer_instance = quasar_timer_get_instance(timer_selection);

    if (timer_instance != NULL) {
        timer_instance->PSC = (uint32_t)prescaler - 1;
    }
}

uint32_t quasar_timer_get_prescaler(quasar_timer_selection_t timer_selection)
{
    TIM_TypeDef *timer_instance = quasar_timer_get_instance(timer_selection);

    return timer_instance->PSC + 1;
}

void quasar_timer_reset_count(quasar_timer_selection_t timer_selection)
{
    TIM_TypeDef *timer_instance = quasar_timer_get_instance(timer_selection);

    if (timer_instance != NULL) {
        timer_instance->CNT = 0;
    }
}

uint32_t quasar_timer_get_count(quasar_timer_selection_t timer_selection)
{
    TIM_TypeDef *timer_instance = quasar_timer_get_instance(timer_selection);
    uint32_t ret = 0;

    if (timer_instance != NULL) {
        ret = timer_instance->CNT;
    }
    return ret;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Enables the clock for the selected timer.
 *
 *  @param[in] timer_selection  Selected timer to enable the clock.
 */
static void timer_enable_clock(quasar_timer_selection_t timer_selection)
{
    switch (timer_selection) {
    case QUASAR_TIMER_SELECTION_TIMER1:
        __HAL_RCC_TIM1_CLK_ENABLE();
        break;
    case QUASAR_TIMER_SELECTION_TIMER2:
        __HAL_RCC_TIM2_CLK_ENABLE();
        break;
    case QUASAR_TIMER_SELECTION_TIMER3:
        __HAL_RCC_TIM3_CLK_ENABLE();
        break;
    case QUASAR_TIMER_SELECTION_TIMER4:
        __HAL_RCC_TIM4_CLK_ENABLE();
        break;
    case QUASAR_TIMER_SELECTION_TIMER5:
        __HAL_RCC_TIM5_CLK_ENABLE();
        break;
    case QUASAR_TIMER_SELECTION_TIMER6:
        __HAL_RCC_TIM6_CLK_ENABLE();
        break;
    case QUASAR_TIMER_SELECTION_TIMER7:
        __HAL_RCC_TIM7_CLK_ENABLE();
        break;
    case QUASAR_TIMER_SELECTION_TIMER8:
        __HAL_RCC_TIM8_CLK_ENABLE();
        break;
    case QUASAR_TIMER_SELECTION_TIMER15:
        __HAL_RCC_TIM15_CLK_ENABLE();
        break;
    case QUASAR_TIMER_SELECTION_TIMER16:
        __HAL_RCC_TIM16_CLK_ENABLE();
        break;
    case QUASAR_TIMER_SELECTION_TIMER17:
        __HAL_RCC_TIM17_CLK_ENABLE();
        break;
    default:
        /* Unimplemented timer. */
        break;
    }
}

/** @brief Disables the clock for the selected timer.
 *
 *  @param[in] timer_selection  Selected timer to disable the clock.
 */
static void timer_disable_clock(quasar_timer_selection_t timer_selection)
{
    switch (timer_selection) {
    case QUASAR_TIMER_SELECTION_TIMER1:
        __HAL_RCC_TIM1_CLK_DISABLE();
        break;
    case QUASAR_TIMER_SELECTION_TIMER2:
        __HAL_RCC_TIM2_CLK_DISABLE();
        break;
    case QUASAR_TIMER_SELECTION_TIMER3:
        __HAL_RCC_TIM3_CLK_DISABLE();
        break;
    case QUASAR_TIMER_SELECTION_TIMER4:
        __HAL_RCC_TIM4_CLK_DISABLE();
        break;
    case QUASAR_TIMER_SELECTION_TIMER5:
        __HAL_RCC_TIM5_CLK_DISABLE();
        break;
    case QUASAR_TIMER_SELECTION_TIMER6:
        __HAL_RCC_TIM6_CLK_DISABLE();
        break;
    case QUASAR_TIMER_SELECTION_TIMER7:
        __HAL_RCC_TIM7_CLK_DISABLE();
        break;
    case QUASAR_TIMER_SELECTION_TIMER8:
        __HAL_RCC_TIM8_CLK_DISABLE();
        break;
    case QUASAR_TIMER_SELECTION_TIMER15:
        __HAL_RCC_TIM15_CLK_DISABLE();
        break;
    case QUASAR_TIMER_SELECTION_TIMER16:
        __HAL_RCC_TIM16_CLK_DISABLE();
        break;
    case QUASAR_TIMER_SELECTION_TIMER17:
        __HAL_RCC_TIM17_CLK_DISABLE();
        break;
    default:
        /* Unimplemented timer. */
        break;
    }
}

/** @brief Calculate the maximum count and the prescaler based on the time period and the system clock.
 *
 *  Formula: update_event (Hz) = system_clock / ((prescaler + 1) * (max_count + 1))
 *
 *  @param[out] prescaler    Calculated prescaler value.
 *  @param[out] max_count    Calculated maximum count value.
 *  @param[in]  timer_config Configuration for the time base and time period.
 */
static void calculate_max_count_and_prescaler(uint16_t *prescaler, uint16_t *max_count, quasar_timer_config_t timer_config)
{
    uint32_t clock_frequency = 0;
    uint32_t divider = 0;
    uint8_t multiplier = 0;

    /* Get the system clock frequency. */
    clock_frequency = quasar_clock_get_system_clock_freq();

    switch (timer_config.time_base) {
    case QUASAR_TIMER_TIME_BASE_MILLISECOND:
        divider = TIME_BASE_MILLISECOND_DIVIDER;
        multiplier = TIME_BASE_MILLISECOND_MULTIPLIER;
        break;
    case QUASAR_TIMER_TIME_BASE_MICROSECOND:
        divider = TIME_BASE_MICROSECOND_DIVIDER;
        multiplier = TIME_BASE_MICROSECOND_MULTIPLIER;
        break;
    default:
        /* Time base not supported. */
        return;
        break;
    }

    /*
     * Calculate the max count and the prescaler. The divider and multiplier are used
     * to limit the max count and the prescaler within a 16-bit register.
     */
    *max_count = ((timer_config.time_period * multiplier) - 1);
    *prescaler = ((clock_frequency / divider) - 1);
}

/** @brief Return the selected timer's global interrupt.
 *
 *  @param[in] timer_selection  Selected timer to get associated global interrupt.
 *  @return Selected timer global interrupt.
 */
static IRQn_Type timer_get_selected_irq(quasar_timer_selection_t timer_selection)
{
    IRQn_Type timer_irq = {0};

    switch (timer_selection) {
    case QUASAR_TIMER_SELECTION_TIMER1:
        timer_irq = TIM1_UP_IRQn;
        break;
    case QUASAR_TIMER_SELECTION_TIMER2:
        timer_irq = TIM2_IRQn;
        break;
    case QUASAR_TIMER_SELECTION_TIMER3:
        timer_irq = TIM3_IRQn;
        break;
    case QUASAR_TIMER_SELECTION_TIMER4:
        timer_irq = TIM4_IRQn;
        break;
    case QUASAR_TIMER_SELECTION_TIMER5:
        timer_irq = TIM5_IRQn;
        break;
    case QUASAR_TIMER_SELECTION_TIMER6:
        timer_irq = TIM6_IRQn;
        break;
    case QUASAR_TIMER_SELECTION_TIMER7:
        timer_irq = TIM7_IRQn;
        break;
    case QUASAR_TIMER_SELECTION_TIMER8:
        timer_irq = TIM8_UP_IRQn;
        break;
    case QUASAR_TIMER_SELECTION_TIMER15:
        timer_irq = TIM15_IRQn;
        break;
    case QUASAR_TIMER_SELECTION_TIMER16:
        timer_irq = TIM16_IRQn;
        break;
    case QUASAR_TIMER_SELECTION_TIMER17:
        timer_irq = TIM17_IRQn;
        break;
    default:
        /* Unimplemented timer. */
        break;
    }

    return timer_irq;
}

/** @brief Configure the basic parameters of a timer.
 *
 *  @param[in]  timer_selection  Selected timer to configure parameters.
 *  @param[out] prescaler        Calculated prescaler value.
 *  @param[out] period           Calculated period value.
 */
static void timer_configure_basic_parameters(quasar_timer_selection_t timer_selection, uint16_t prescaler, uint16_t max_count)
{
    TIM_TypeDef *timer_instance = quasar_timer_get_instance(timer_selection);

    /* Configure the clock divider. */
    timer_instance->PSC = prescaler;

    /* Configure the maximum count of a cycle. */
    timer_instance->ARR = max_count;

    /* The counter will increment. */
    QUASAR_CLEAR_BIT(timer_instance->CR1, TIM_CR1_DIR);

    /* Reset the count. */
    QUASAR_SET_BIT(timer_instance->EGR, TIM_EGR_UG);
}
