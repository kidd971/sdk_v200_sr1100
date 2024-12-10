/** @file  quasar_pwm.h
 *  @brief This module provides functions to control and configure PWM.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_PWM_H_
#define QUASAR_PWM_H_

/* INCLUDES *******************************************************************/
#include "quasar_timer_ext.h"
#include "quasar_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Available channel for a timer.
 *
 *  Refer to the reference manual to ensure that the channel
 *  is available for the selected timer.
 */
typedef enum quasar_pwm_channel {
    /*! Timer channel 1 */
    QUASAR_PWM_CHANNEL_1 = 1,
    /*! Timer channel 2 */
    QUASAR_PWM_CHANNEL_2 = 2,
    /*! Timer channel 3 */
    QUASAR_PWM_CHANNEL_3 = 3,
    /*! Timer channel 4 */
    QUASAR_PWM_CHANNEL_4 = 4
} quasar_pwm_channel_t;

/** @brief Configuration settings for a PWM.
 *
 *  Refer to the reference manual to ensure that the timer
 *  can generate PWM.
 */
typedef struct quasar_pwm_config {
    /*! Configuration of the timer used for PWM. */
    quasar_timer_config_t timer_config;
    /*! Timer channel used for the PWM ouput. */
    quasar_pwm_channel_t timer_channel;
    /*! PWM output's duty cycle percentage (0 - 100). */
    uint8_t duty_cycle;
    /*! GPIO used for the PWM ouput. */
    quasar_gpio_config_t gpio_config;
} quasar_pwm_config_t;

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Initialize the GPIO and the timer linked to the PWM as well as the PWM itself.
 *
 *  @param[in] pwm_config  Configuration of the PWM.
 */
void quasar_pwm_init(quasar_pwm_config_t *pwm_config);

/** @brief Deinitialize the GPIO and the timer linked to the PWM as well as the PWM itself.
 *
 *  @param[in] pwm_config  Configuration of the PWM.
 */
void quasar_pwm_deinit(quasar_pwm_config_t pwm_config);

/** @brief Configure the duty cycle of the PWM.
 *
 *  @param[in] pwm_config               Configuration of the PWM.
 *  @param[in] new_duty_cycle_percent   Duty cycle to set.
 */
void quasar_pwm_set_duty_cycle(quasar_pwm_config_t *pwm_config, uint8_t new_duty_cycle_percent);

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_PWM_H_ */
