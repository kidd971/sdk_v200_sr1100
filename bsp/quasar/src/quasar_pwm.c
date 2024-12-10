/** @file  quasar_pwm.c
 *  @brief This module provides functions to control and configure PWM.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_pwm.h"

/* CONSTANTS ******************************************************************/
#define QUASAR_PWM_PERCENT_DIVIDER   100.0

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void pwm_configure_parameters(quasar_pwm_config_t *pwm_config);
static void pwm_set_duty_cycle(TIM_TypeDef *timer_instance, quasar_pwm_config_t *pwm_config, uint8_t duty_cycle);
static void pwm_configure_channel(TIM_TypeDef *timer_instance, quasar_pwm_channel_t timer_channel);
static void pwm_unconfigure_channel(TIM_TypeDef *timer_instance, quasar_pwm_channel_t timer_channel);
static uint32_t pwm_convert_duty_cycle_to_ccr(uint8_t duty_cycle, uint16_t max_count);

/* PUBLIC FUNCTIONS ***********************************************************/
void quasar_pwm_init(quasar_pwm_config_t *pwm_config)
{
    quasar_gpio_init(pwm_config->gpio_config);
    quasar_timer_init(&(pwm_config->timer_config));
    pwm_configure_parameters(pwm_config);
}

void quasar_pwm_deinit(quasar_pwm_config_t pwm_config)
{
    TIM_TypeDef *timer_instance = quasar_timer_get_instance(pwm_config.timer_config.timer_selection);

    pwm_unconfigure_channel(timer_instance, pwm_config.timer_channel);
    quasar_timer_deinit(pwm_config.timer_config);
    quasar_gpio_deinit(pwm_config.gpio_config.port, pwm_config.gpio_config.pin);
}

void quasar_pwm_set_duty_cycle(quasar_pwm_config_t *pwm_config, uint8_t new_duty_cycle_percent)
{
    TIM_TypeDef *timer_instance = quasar_timer_get_instance(pwm_config->timer_config.timer_selection);

    pwm_set_duty_cycle(timer_instance, pwm_config, new_duty_cycle_percent);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Configure parameters for the PWM.
 *
 *  @param[in] pwm_config  Pointer to the configuration of the PWM.
 */
static void pwm_configure_parameters(quasar_pwm_config_t *pwm_config)
{
    TIM_TypeDef *timer_instance = quasar_timer_get_instance(pwm_config->timer_config.timer_selection);

    /* Disable the slave mode to use the internal clock of the APB. */
    QUASAR_CLEAR_BIT(timer_instance->SMCR, TIM_SMCR_SMS);

    /* Select the edge-aligned mode (counts up to the value in ARR and then resets to zero). */
    QUASAR_CLEAR_BIT(timer_instance->CR1, TIM_CR1_CMS);

    pwm_configure_channel(timer_instance, pwm_config->timer_channel);

    pwm_set_duty_cycle(timer_instance, pwm_config, pwm_config->duty_cycle);
}

/** @brief Configure the duty cycle of the PWM.
 *
 *  @param[in] timer_instance  The timer instance linked to the PWM.
 *  @param[in] pwm_config      Pointer to the configuration of the PWM.
 *  @param[in] duty_cycle      Duty cycle to set.
 */
static void pwm_set_duty_cycle(TIM_TypeDef *timer_instance, quasar_pwm_config_t *pwm_config, uint8_t duty_cycle)
{
    uint32_t ccr_value;
    uint32_t max_count;

    if (duty_cycle > 100) {
        duty_cycle = 100;
    }

    /* Set the new duty cycle into the PWM configuration and convert it to CCR value. */
    pwm_config->duty_cycle = duty_cycle;
    max_count = timer_instance->ARR;
    ccr_value = pwm_convert_duty_cycle_to_ccr(pwm_config->duty_cycle, max_count);

    /* Configure the new duty cycle. */
    switch (pwm_config->timer_channel) {
    case QUASAR_PWM_CHANNEL_1:
        timer_instance->CCR1 = ccr_value;
        break;
    case QUASAR_PWM_CHANNEL_2:
        timer_instance->CCR2 = ccr_value;
        break;
    case QUASAR_PWM_CHANNEL_3:
        timer_instance->CCR3 = ccr_value;
        break;
    case QUASAR_PWM_CHANNEL_4:
        timer_instance->CCR4 = ccr_value;
        break;
    default:
        /* Unimplemented timer. */
        break;
    }
}

/** @brief Configure the PWM channel.
 *
 *  @param[in] timer_instance  The timer instance linked to the PWM.
 *  @param[in] timer_channel   The timer channel.
 */
static void pwm_configure_channel(TIM_TypeDef *timer_instance, quasar_pwm_channel_t timer_channel)
{
    switch (timer_channel) {
    case QUASAR_PWM_CHANNEL_1:
        /* Select mode 1 asymmetric PWM for comparison output. */
        QUASAR_CLEAR_BIT(timer_instance->CCMR1, TIM_CCMR1_OC1M_0);
        QUASAR_SET_BIT(timer_instance->CCMR1, TIM_CCMR1_OC1M_1);
        QUASAR_SET_BIT(timer_instance->CCMR1, TIM_CCMR1_OC1M_2);
        QUASAR_SET_BIT(timer_instance->CCMR1, TIM_CCMR1_OC1M_3);

        /* Configure the comparison for the selected channel as output. */
        QUASAR_CLEAR_BIT(timer_instance->CCMR1, TIM_CCMR1_CC1S);

        /* Enable the auto-reload register for the selected channel. */
        QUASAR_SET_BIT(timer_instance->CCMR1, TIM_CCMR1_OC1PE);

        /* Set signal polarity to high level (high at the beginning of each cycle). */
        QUASAR_CLEAR_BIT(timer_instance->CCER, TIM_CCER_CC1P);

        /* Enable output for the selected channel. */
        QUASAR_SET_BIT(timer_instance->CCER, TIM_CCER_CC1E);
        break;

    case QUASAR_PWM_CHANNEL_2:
        /* Select mode 1 asymmetric PWM for comparison output. */
        QUASAR_CLEAR_BIT(timer_instance->CCMR1, TIM_CCMR1_OC2M_0);
        QUASAR_SET_BIT(timer_instance->CCMR1, TIM_CCMR1_OC2M_1);
        QUASAR_SET_BIT(timer_instance->CCMR1, TIM_CCMR1_OC2M_2);
        QUASAR_SET_BIT(timer_instance->CCMR1, TIM_CCMR1_OC2M_3);

        /* Configure the comparison for the selected channel as output. */
        QUASAR_CLEAR_BIT(timer_instance->CCMR1, TIM_CCMR1_CC2S);

        /* Enable the auto-reload register for the selected channel. */
        QUASAR_SET_BIT(timer_instance->CCMR1, TIM_CCMR1_OC2PE);

        /* Set signal polarity to high level (high at the beginning of each cycle). */
        QUASAR_CLEAR_BIT(timer_instance->CCER, TIM_CCER_CC2P);

        /* Enable output for the selected channel. */
        QUASAR_SET_BIT(timer_instance->CCER, TIM_CCER_CC2E);
        break;

    case QUASAR_PWM_CHANNEL_3:
        /* Select mode 1 asymmetric PWM for comparison output.  */
        QUASAR_CLEAR_BIT(timer_instance->CCMR2, TIM_CCMR2_OC3M_0);
        QUASAR_SET_BIT(timer_instance->CCMR2, TIM_CCMR2_OC3M_1);
        QUASAR_SET_BIT(timer_instance->CCMR2, TIM_CCMR2_OC3M_2);
        QUASAR_SET_BIT(timer_instance->CCMR2, TIM_CCMR2_OC3M_3);

        /* Configure the comparison for the selected channel as output.*/
        QUASAR_CLEAR_BIT(timer_instance->CCMR2, TIM_CCMR2_CC3S);

        /* Enable the auto-reload register for the selected channel.*/
        QUASAR_SET_BIT(timer_instance->CCMR2, TIM_CCMR2_OC3PE);

        /* Set signal polarity to high level (high at the beginning of each cycle). */
        QUASAR_CLEAR_BIT(timer_instance->CCER, TIM_CCER_CC3P);

        /* Enable output for the selected channel. */
        QUASAR_SET_BIT(timer_instance->CCER, TIM_CCER_CC3E);
        break;

    case QUASAR_PWM_CHANNEL_4:
        /* Select mode 1 asymmetric PWM for comparison output. */
        QUASAR_CLEAR_BIT(timer_instance->CCMR2, TIM_CCMR2_OC4M_0);
        QUASAR_SET_BIT(timer_instance->CCMR2, TIM_CCMR2_OC4M_1);
        QUASAR_SET_BIT(timer_instance->CCMR2, TIM_CCMR2_OC4M_2);
        QUASAR_SET_BIT(timer_instance->CCMR2, TIM_CCMR2_OC4M_3);

        /* Configure the comparison for the selected channel as output. */
        QUASAR_CLEAR_BIT(timer_instance->CCMR2, TIM_CCMR2_CC4S);

        /* Enable the auto-reload register for the selected channel. */
        QUASAR_SET_BIT(timer_instance->CCMR2, TIM_CCMR2_OC4PE);

        /* Set signal polarity to high level (high at the beginning of each cycle). */
        QUASAR_CLEAR_BIT(timer_instance->CCER, TIM_CCER_CC4P);

        /* Enable output for the selected channel. */
        QUASAR_SET_BIT(timer_instance->CCER, TIM_CCER_CC4E);
        break;
    default:
        /* Unimplemented timer. */
        break;
    }
}

/** @brief Restore register bits for the timer channel to their default reset values.
 *
 *  @param[in] timer_instance  The timer instance linked to the PWM.
 *  @param[in] timer_channel   The timer channel.
 */
static void pwm_unconfigure_channel(TIM_TypeDef *timer_instance, quasar_pwm_channel_t timer_channel)
{
    switch (timer_channel) {
    case QUASAR_PWM_CHANNEL_1:
        QUASAR_CLEAR_BIT(timer_instance->CCMR1, TIM_CCMR1_OC1M_1);
        QUASAR_CLEAR_BIT(timer_instance->CCMR1, TIM_CCMR1_OC1M_2);
        QUASAR_CLEAR_BIT(timer_instance->CCMR1, TIM_CCMR1_OC1M_3);
        QUASAR_CLEAR_BIT(timer_instance->CCMR1, TIM_CCMR1_OC1PE);
        QUASAR_CLEAR_BIT(timer_instance->CCER, TIM_CCER_CC1E);
        timer_instance->CCR1 = 0;
        break;

    case QUASAR_PWM_CHANNEL_2:
        QUASAR_CLEAR_BIT(timer_instance->CCMR1, TIM_CCMR1_OC2M_1);
        QUASAR_CLEAR_BIT(timer_instance->CCMR1, TIM_CCMR1_OC2M_2);
        QUASAR_CLEAR_BIT(timer_instance->CCMR1, TIM_CCMR1_OC2M_3);
        QUASAR_CLEAR_BIT(timer_instance->CCMR1, TIM_CCMR1_OC2PE);
        QUASAR_CLEAR_BIT(timer_instance->CCER, TIM_CCER_CC2E);
        timer_instance->CCR2 = 0;
        break;

    case QUASAR_PWM_CHANNEL_3:
        QUASAR_CLEAR_BIT(timer_instance->CCMR2, TIM_CCMR2_OC3M_1);
        QUASAR_CLEAR_BIT(timer_instance->CCMR2, TIM_CCMR2_OC3M_2);
        QUASAR_CLEAR_BIT(timer_instance->CCMR2, TIM_CCMR2_OC3M_3);
        QUASAR_CLEAR_BIT(timer_instance->CCMR2, TIM_CCMR2_OC3PE);
        QUASAR_CLEAR_BIT(timer_instance->CCER, TIM_CCER_CC3E);
        timer_instance->CCR3 = 0;
        break;

    case QUASAR_PWM_CHANNEL_4:
        QUASAR_CLEAR_BIT(timer_instance->CCMR2, TIM_CCMR2_OC4M_1);
        QUASAR_CLEAR_BIT(timer_instance->CCMR2, TIM_CCMR2_OC4M_2);
        QUASAR_CLEAR_BIT(timer_instance->CCMR2, TIM_CCMR2_OC4M_3);
        QUASAR_CLEAR_BIT(timer_instance->CCMR2, TIM_CCMR2_OC4PE);
        QUASAR_CLEAR_BIT(timer_instance->CCER, TIM_CCER_CC4E);
        timer_instance->CCR4 = 0;
        break;
    default:
        /* Unimplemented timer. */
        break;
    }
}

/** @brief Convert a duty cycle to its corresponding CCR (Capture/Compare Register) value.
 *
 *  @param[in] duty_cycle  The duty cycle percent (0-100).
 *  @param[in] max_count   The maximum count value of the timer.
 *  @return The calculated CCR value for the given duty cycle.
 */
static uint32_t pwm_convert_duty_cycle_to_ccr(uint8_t duty_cycle, uint16_t max_count)
{
    return (uint32_t)((duty_cycle / QUASAR_PWM_PERCENT_DIVIDER) * max_count);
}
