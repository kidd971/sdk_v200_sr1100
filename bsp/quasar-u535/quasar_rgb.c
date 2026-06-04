/** @file  quasar_rgb.c
 *  @brief This module configures the RGB LED and provides functions to control each color.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_rgb.h"
#include "quasar_def.h"
#include "quasar_gpio.h"

/* PRIVATE GLOBALS ************************************************************/
/* U535 only has BLUE available (PD7). RED and GREEN are NC on this board. */
static bool blue_is_active = false;
static bool rgb_is_set = false;

/* PUBLIC FUNCTIONS ***********************************************************/
void quasar_rgb_init(void)
{
#ifdef QUASAR_DEF_LED_RGB_BLUE_PORT
    quasar_gpio_config_t config = {
        .port = QUASAR_DEF_LED_RGB_BLUE_PORT,
        .pin = QUASAR_DEF_LED_RGB_BLUE_PIN,
        .mode = QUASAR_GPIO_MODE_OUTPUT,
        .type = QUASAR_GPIO_TYPE_OD,
        .pull = QUASAR_GPIO_PULL_UP,
        .speed = QUASAR_GPIO_SPEED_LOW,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(config);
    quasar_gpio_set(QUASAR_DEF_LED_RGB_BLUE_PORT, QUASAR_DEF_LED_RGB_BLUE_PIN);
#endif
    blue_is_active = false;
    rgb_is_set = false;
}

void quasar_rgb_deinit(void)
{
#ifdef QUASAR_DEF_LED_RGB_BLUE_PORT
    quasar_gpio_deinit(QUASAR_DEF_LED_RGB_BLUE_PORT, QUASAR_DEF_LED_RGB_BLUE_PIN);
#endif
}

void quasar_rgb_configure_color(quasar_rgb_color_t rgb_color)
{
    /* On U535 only BLUE is available. Enable blue for any color that includes blue. */
    switch (rgb_color) {
    case QUASAR_RGB_COLOR_BLUE:
    case QUASAR_RGB_COLOR_CYAN:
    case QUASAR_RGB_COLOR_MAGENTA:
    case QUASAR_RGB_COLOR_WHITE:
        blue_is_active = true;
        break;
    default:
        blue_is_active = false;
        break;
    }
}

void quasar_rgb_set(void)
{
#ifdef QUASAR_DEF_LED_RGB_BLUE_PORT
    if (blue_is_active) {
        quasar_gpio_clear(QUASAR_DEF_LED_RGB_BLUE_PORT, QUASAR_DEF_LED_RGB_BLUE_PIN);
    }
#endif
    rgb_is_set = true;
}

void quasar_rgb_clear(void)
{
#ifdef QUASAR_DEF_LED_RGB_BLUE_PORT
    quasar_gpio_set(QUASAR_DEF_LED_RGB_BLUE_PORT, QUASAR_DEF_LED_RGB_BLUE_PIN);
#endif
    rgb_is_set = false;
}

void quasar_rgb_toggle(void)
{
    if (rgb_is_set) {
        quasar_rgb_clear();
    } else {
        quasar_rgb_set();
    }
}
