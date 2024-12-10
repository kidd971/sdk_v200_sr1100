/** @file  quasar_rgb.c
 *  @brief This module configures the RGB LED and provides functions to control each color.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_rgb.h"
#include "quasar_gpio.h"

/* TYPES **********************************************************************/
/** @brief Quasar BSP RGB LED peripherals selection.
 */
typedef enum quasar_rgb_peripherals {
    /*! User application RGB LED. */
    QUASAR_RGB_RED,
    /*! User application RGB LED. */
    QUASAR_RGB_GREEN,
    /*! User application RGB LED. */
    QUASAR_RGB_BLUE,
} quasar_rgb_peripherals_t;

/** @brief Structure for managing RGB LED.
 *
 *  This structure stores pointers to functions that set or clear the
 *  LED depending to the desired color.
 */
typedef struct rgb_status {
    /*! Allow storing whether the RGB is turned off or on. */
    bool is_rgb_set;
    /*! Configure the state (set or clear) of the red LED. */
    void (*rgb_red_configure)(quasar_rgb_peripherals_t rgb_peripheral);
    /*! Configure the state (set or clear) of the green LED. */
    void (*rgb_green_configure)(quasar_rgb_peripherals_t rgb_peripheral);
    /*! Configure the state (set or clear) of the blue LED. */
    void (*rgb_blue_configure)(quasar_rgb_peripherals_t rgb_peripheral);
} rgb_status_t;

/* PRIVATE GLOBALS ************************************************************/
/* Global private structure for managing RGB LED. */
static rgb_status_t rgb_status = {0};

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void rgb_init(quasar_rgb_peripherals_t rgb_peripheral);
static void rgb_deinit(quasar_rgb_peripherals_t rgb_peripheral);
static void rgb_set(quasar_rgb_peripherals_t rgb_peripheral);
static void rgb_clear(quasar_rgb_peripherals_t rgb_peripheral);
static quasar_gpio_config_t rgb_get_config(quasar_rgb_peripherals_t rgb_peripheral);

/* PUBLIC FUNCTIONS ***********************************************************/
void quasar_rgb_init(void)
{
    rgb_init(QUASAR_RGB_RED);
    rgb_init(QUASAR_RGB_BLUE);
    rgb_init(QUASAR_RGB_GREEN);

    /* By default, desired color is set to white. */
    rgb_status.is_rgb_set = false;
    rgb_status.rgb_red_configure = rgb_set;
    rgb_status.rgb_green_configure = rgb_set;
    rgb_status.rgb_blue_configure = rgb_set;

    quasar_rgb_clear();
}

void quasar_rgb_deinit(void)
{
    rgb_deinit(QUASAR_RGB_RED);
    rgb_deinit(QUASAR_RGB_BLUE);
    rgb_deinit(QUASAR_RGB_GREEN);
}

void quasar_rgb_configure_color(quasar_rgb_color_t rgb_color)
{
    switch (rgb_color) {
    case QUASAR_RGB_COLOR_RED:
        rgb_status.rgb_red_configure = rgb_set;
        rgb_status.rgb_green_configure = rgb_clear;
        rgb_status.rgb_blue_configure = rgb_clear;
        break;
    case QUASAR_RGB_COLOR_GREEN:
        rgb_status.rgb_red_configure = rgb_clear;
        rgb_status.rgb_green_configure = rgb_set;
        rgb_status.rgb_blue_configure = rgb_clear;
        break;
    case QUASAR_RGB_COLOR_BLUE:
        rgb_status.rgb_red_configure = rgb_clear;
        rgb_status.rgb_green_configure = rgb_clear;
        rgb_status.rgb_blue_configure = rgb_set;
        break;
    case QUASAR_RGB_COLOR_YELLOW:
        rgb_status.rgb_red_configure = rgb_set;
        rgb_status.rgb_green_configure = rgb_set;
        rgb_status.rgb_blue_configure = rgb_clear;
        break;
    case QUASAR_RGB_COLOR_CYAN:
        rgb_status.rgb_red_configure = rgb_clear;
        rgb_status.rgb_green_configure = rgb_set;
        rgb_status.rgb_blue_configure = rgb_set;
        break;
    case QUASAR_RGB_COLOR_MAGENTA:
        rgb_status.rgb_red_configure = rgb_set;
        rgb_status.rgb_green_configure = rgb_clear;
        rgb_status.rgb_blue_configure = rgb_set;
        break;
    case QUASAR_RGB_COLOR_WHITE:
        rgb_status.rgb_red_configure = rgb_set;
        rgb_status.rgb_green_configure = rgb_set;
        rgb_status.rgb_blue_configure = rgb_set;
        break;
    default:
        break;
    }
}

void quasar_rgb_set(void)
{
    rgb_status.rgb_red_configure(QUASAR_RGB_RED);
    rgb_status.rgb_green_configure(QUASAR_RGB_GREEN);
    rgb_status.rgb_blue_configure(QUASAR_RGB_BLUE);
    rgb_status.is_rgb_set = true;
}

void quasar_rgb_clear(void)
{
    rgb_clear(QUASAR_RGB_RED);
    rgb_clear(QUASAR_RGB_GREEN);
    rgb_clear(QUASAR_RGB_BLUE);
    rgb_status.is_rgb_set = false;
}

void quasar_rgb_toggle(void)
{
    if (rgb_status.is_rgb_set) {
        quasar_rgb_clear();
    } else {
        quasar_rgb_set();
    }
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize the selected RGB peripheral.
 *
 *  @param[in] rgb_peripheral  Selected RGB peripheral.
 */
static void rgb_init(quasar_rgb_peripherals_t rgb_peripheral)
{
    quasar_gpio_config_t rgb_config = rgb_get_config(rgb_peripheral);

    quasar_gpio_init(rgb_config);
}

/** @brief Deinitialize the selected RGB peripheral.
 *
 *  @param[in] rgb_peripheral  Selected RGB peripheral.
 */
static void rgb_deinit(quasar_rgb_peripherals_t rgb_peripheral)
{
    quasar_gpio_config_t rgb_config = rgb_get_config(rgb_peripheral);

    quasar_gpio_deinit(rgb_config.port, rgb_config.pin);
}

static void rgb_set(quasar_rgb_peripherals_t rgb_peripheral)
{
    quasar_gpio_config_t rgb_config = rgb_get_config(rgb_peripheral);
    /* The LED lights on if the GPIO is pull-down. */
    quasar_gpio_clear(rgb_config.port, rgb_config.pin);
}

static void rgb_clear(quasar_rgb_peripherals_t rgb_peripheral)
{
    quasar_gpio_config_t rgb_config = rgb_get_config(rgb_peripheral);
    /* The LED lights off if the GPIO is pull-up. */
    quasar_gpio_set(rgb_config.port, rgb_config.pin);
}

/** @brief Get the configuration of the RGB peripheral.
 *
 *  All RGBs are controled by software with inverted logic.
 *
 *  @param[in] rgb_peripheral  Selected RGB peripheral.
 *  @return The RGB peripheral configuration.
 */
static quasar_gpio_config_t rgb_get_config(quasar_rgb_peripherals_t rgb_peripheral)
{
    quasar_gpio_config_t config;

    switch (rgb_peripheral) {
    /* The LED_RGB_RED is designated for application purposes. */
    case QUASAR_RGB_RED:
        config.port         = QUASAR_DEF_LED_RGB_RED_PORT;
        config.pin          = QUASAR_DEF_LED_RGB_RED_PIN;
        config.mode         = QUASAR_GPIO_MODE_OUTPUT;
        config.type         = QUASAR_GPIO_TYPE_OD;
        config.pull         = QUASAR_GPIO_PULL_UP;
        config.speed        = QUASAR_GPIO_SPEED_LOW;
        config.alternate    = QUASAR_GPIO_ALTERNATE_NONE;
        break;

    /* The LED_RGB_GREEN is designated for application purposes. */
    case QUASAR_RGB_GREEN:
        config.port         = QUASAR_DEF_LED_RGB_GREEN_PORT;
        config.pin          = QUASAR_DEF_LED_RGB_GREEN_PIN;
        config.mode         = QUASAR_GPIO_MODE_OUTPUT;
        config.type         = QUASAR_GPIO_TYPE_OD;
        config.pull         = QUASAR_GPIO_PULL_UP;
        config.speed        = QUASAR_GPIO_SPEED_LOW;
        config.alternate    = QUASAR_GPIO_ALTERNATE_NONE;
        break;

    /* The LED_RGB_BLUE is designated for application purposes. */
    case QUASAR_RGB_BLUE:
        config.port         = QUASAR_DEF_LED_RGB_BLUE_PORT;
        config.pin          = QUASAR_DEF_LED_RGB_BLUE_PIN;
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
