/** @file  quasar_led.c
 *  @brief This module configures LEDs and provides functions to control each of them.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_led.h"
#include "quasar_gpio.h"

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void led_init(quasar_led_peripherals_t led_peripheral);
static void led_deinit(quasar_led_peripherals_t led_peripheral);
static quasar_gpio_config_t led_get_config(quasar_led_peripherals_t led_peripheral);

/* PUBLIC FUNCTIONS ***********************************************************/
void quasar_led_init(void)
{
    led_init(QUASAR_LED_USB);
    led_init(QUASAR_LED_LINEIN);
    led_init(QUASAR_LED_HPJACK);
    led_init(QUASAR_LED_USER_1);
    led_init(QUASAR_LED_USER_2);
    led_init(QUASAR_LED_USER_3);
    led_init(QUASAR_LED_USER_4);

    quasar_led_clear(QUASAR_LED_USB);
    quasar_led_clear(QUASAR_LED_LINEIN);
    quasar_led_clear(QUASAR_LED_HPJACK);
    quasar_led_clear(QUASAR_LED_USER_1);
    quasar_led_clear(QUASAR_LED_USER_2);
    quasar_led_clear(QUASAR_LED_USER_3);
    quasar_led_clear(QUASAR_LED_USER_4);
}

void quasar_led_deinit(void)
{
    led_deinit(QUASAR_LED_USB);
    led_deinit(QUASAR_LED_LINEIN);
    led_deinit(QUASAR_LED_HPJACK);
    led_deinit(QUASAR_LED_USER_1);
    led_deinit(QUASAR_LED_USER_2);
    led_deinit(QUASAR_LED_USER_3);
    led_deinit(QUASAR_LED_USER_4);
}

void quasar_led_set(quasar_led_peripherals_t led_peripheral)
{
    quasar_gpio_config_t led_config = led_get_config(led_peripheral);
    /* The LED lights on if the GPIO is pull-down. */
    quasar_gpio_clear(led_config.port, led_config.pin);
}

void quasar_led_clear(quasar_led_peripherals_t led_peripheral)
{
    quasar_gpio_config_t led_config = led_get_config(led_peripheral);
    /* The LED lights off if the GPIO is pull-up. */
    quasar_gpio_set(led_config.port, led_config.pin);
}

void quasar_led_toggle(quasar_led_peripherals_t led_peripheral)
{
    quasar_gpio_config_t led_config = led_get_config(led_peripheral);

    quasar_gpio_toggle(led_config.port, led_config.pin);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize the selected LED peripheral.
 *
 *  @param[in] led_peripheral  Selected LED peripheral.
 */
static void led_init(quasar_led_peripherals_t led_peripheral)
{
    quasar_gpio_config_t led_config = led_get_config(led_peripheral);

    quasar_gpio_init(led_config);
}

/** @brief Deinitialize the selected LED peripheral.
 *
 *  @param[in] led_peripheral  Selected LED peripheral.
 */
static void led_deinit(quasar_led_peripherals_t led_peripheral)
{
    quasar_gpio_config_t led_config = led_get_config(led_peripheral);

    quasar_gpio_deinit(led_config.port, led_config.pin);
}

/** @brief Get the configuration of the LED peripheral.
 *
 *  All LEDs are controlled by software with inverted logic.
 *
 *  @param[in] led_peripheral  Selected LED peripheral.
 *  @return The LED peripheral configuration.
 */
static quasar_gpio_config_t led_get_config(quasar_led_peripherals_t led_peripheral)
{
    quasar_gpio_config_t config;

    switch (led_peripheral) {
    /* The LED_USB lights on if the USB port is the source of the audio. */
    case QUASAR_LED_USB:
        config.port         = QUASAR_DEF_LED_USB_PORT;
        config.pin          = QUASAR_DEF_LED_USB_PIN;
        config.mode         = QUASAR_GPIO_MODE_OUTPUT;
        config.type         = QUASAR_GPIO_TYPE_OD;
        config.pull         = QUASAR_GPIO_PULL_UP;
        config.speed        = QUASAR_GPIO_SPEED_LOW;
        config.alternate    = QUASAR_GPIO_ALTERNATE_NONE;
        break;

    /*
     *  The LED_LINEIN lights on if the Line-In jack is enabled as the audio output.
     *
     *  LED must : lights on if GPIO_LINEIN_DETECT (PC4) is high
     *             lights off if GPIO_LINEIN_DETECT (PC4) is low
     *
     *  The GPIO_LINEIN_DETECT is high if the Line in is present and goes low if it is not present.
     *  PC4 -> GPIO_LINEIN_DETECT, configured as GPIO in input mode.
     */
    case QUASAR_LED_LINEIN:
        config.port         = QUASAR_DEF_LED_LINEIN_PORT;
        config.pin          = QUASAR_DEF_LED_LINEIN_PIN;
        config.mode         = QUASAR_GPIO_MODE_OUTPUT;
        config.type         = QUASAR_GPIO_TYPE_OD;
        config.pull         = QUASAR_GPIO_PULL_UP;
        config.speed        = QUASAR_GPIO_SPEED_LOW;
        config.alternate    = QUASAR_GPIO_ALTERNATE_NONE;
        break;

    /* The LED_HPJACK lights on if the headphone jack is enabled ad the audio output. */
    case QUASAR_LED_HPJACK:
        config.port         = QUASAR_DEF_LED_HP_JACK_PORT;
        config.pin          = QUASAR_DEF_LED_HP_JACK_PIN;
        config.mode         = QUASAR_GPIO_MODE_OUTPUT;
        config.type         = QUASAR_GPIO_TYPE_OD;
        config.pull         = QUASAR_GPIO_PULL_UP;
        config.speed        = QUASAR_GPIO_SPEED_LOW;
        config.alternate    = QUASAR_GPIO_ALTERNATE_NONE;
        break;

    /* The LED_USER1 is designated for application purposes. */
    case QUASAR_LED_USER_1:
        config.port         = QUASAR_DEF_LED_USER_1_PORT;
        config.pin          = QUASAR_DEF_LED_USER_1_PIN;
        config.mode         = QUASAR_GPIO_MODE_OUTPUT;
        config.type         = QUASAR_GPIO_TYPE_OD;
        config.pull         = QUASAR_GPIO_PULL_UP;
        config.speed        = QUASAR_GPIO_SPEED_LOW;
        config.alternate    = QUASAR_GPIO_ALTERNATE_NONE;
        break;

    /* The LED_USER2 is designated for application purposes. */
    case QUASAR_LED_USER_2:
        config.port         = QUASAR_DEF_LED_USER_2_PORT;
        config.pin          = QUASAR_DEF_LED_USER_2_PIN;
        config.mode         = QUASAR_GPIO_MODE_OUTPUT;
        config.type         = QUASAR_GPIO_TYPE_OD;
        config.pull         = QUASAR_GPIO_PULL_UP;
        config.speed        = QUASAR_GPIO_SPEED_LOW;
        config.alternate    = QUASAR_GPIO_ALTERNATE_NONE;
        break;

    /* The LED_USER3 is designated for application purposes. */
    case QUASAR_LED_USER_3:
        config.port         = QUASAR_DEF_LED_USER_3_PORT;
        config.pin          = QUASAR_DEF_LED_USER_3_PIN;
        config.mode         = QUASAR_GPIO_MODE_OUTPUT;
        config.type         = QUASAR_GPIO_TYPE_OD;
        config.pull         = QUASAR_GPIO_PULL_UP;
        config.speed        = QUASAR_GPIO_SPEED_LOW;
        config.alternate    = QUASAR_GPIO_ALTERNATE_NONE;
        break;

    /* The LED_USER4 is designated for application purposes. */
    case QUASAR_LED_USER_4:
        config.port         = QUASAR_DEF_LED_USER_4_PORT;
        config.pin          = QUASAR_DEF_LED_USER_4_PIN;
        config.mode         = QUASAR_GPIO_MODE_OUTPUT;
        config.type         = QUASAR_GPIO_TYPE_OD;
        config.pull         = QUASAR_GPIO_PULL_UP;
        config.speed        = QUASAR_GPIO_SPEED_LOW;
        config.alternate    = QUASAR_GPIO_ALTERNATE_NONE;
        break;

    default:
        break;
    }

    return config;
}
