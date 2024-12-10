/** @file  quasar_power.c
 *  @brief This module provides functions to manage power features.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_power.h"
#include "quasar_def.h"

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
void power_init_vdd_select_gpio(void);
void quasar_power_set_vdd_level(quasar_vdd_selection_t quasar_vdd_selection);

/* PUBLIC FUNCTIONS ***********************************************************/
void quasar_power_up(void)
{
    /* Enable the port G power. */
    HAL_PWREx_EnableVddIO2();

    /* Switch to SMPS regulator instead of LDO */
    if (HAL_PWREx_ConfigSupply(PWR_SMPS_SUPPLY) != HAL_OK) {
        while (1);
    }
}

void quasar_power_init_gpios(void)
{
    /* Configuration of the GPIO that drives the pin to enable the 3v3 LDO for MCU's USB and analog circuitry. */

    /* Both GPIOs are set to power up the ADC circuitry to allow getting the board revision. */
    /*  REV A board use PD4 as LDO enable GPIO. */
    quasar_gpio_config_t gpio_config_ldo_mcu_reva = {
        .port = QUASAR_DEF_LDO_MCU_EN_PORT_REVA,
        .pin = QUASAR_DEF_LDO_MCU_EN_PIN_REVA,
        .mode = QUASAR_GPIO_MODE_OUTPUT,
        .type = QUASAR_GPIO_TYPE_OD,
        .pull = QUASAR_GPIO_PULL_UP,
        .speed = QUASAR_GPIO_SPEED_LOW,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config_ldo_mcu_reva);
    quasar_gpio_clear(gpio_config_ldo_mcu_reva.port, gpio_config_ldo_mcu_reva.pin);

    /*  REV B board use PB15 as LDO enable GPIO. */
    quasar_gpio_config_t gpio_config_ldo_mcu_revb = {
        .port = QUASAR_DEF_LDO_MCU_EN_PORT_REVB,
        .pin = QUASAR_DEF_LDO_MCU_EN_PIN_REVB,
        .mode = QUASAR_GPIO_MODE_OUTPUT,
        .type = QUASAR_GPIO_TYPE_OD,
        .pull = QUASAR_GPIO_PULL_UP,
        .speed = QUASAR_GPIO_SPEED_LOW,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(gpio_config_ldo_mcu_revb);
    quasar_gpio_clear(gpio_config_ldo_mcu_revb.port, gpio_config_ldo_mcu_revb.pin);

    /* Configuration of the GPIO that drives the pin to enable the 3v3 LDO for LEDs. */
    quasar_gpio_config_t gpio_config_ldo_led = {
        .port       = QUASAR_DEF_LDO_LED_EN_PORT,
        .pin        = QUASAR_DEF_LDO_LED_EN_PIN,
        .mode       = QUASAR_GPIO_MODE_OUTPUT,
        .type       = QUASAR_GPIO_TYPE_OD,
        .pull       = QUASAR_GPIO_PULL_UP,
        .speed      = QUASAR_GPIO_SPEED_LOW,
        .alternate  = QUASAR_GPIO_ALTERNATE_NONE
    };
    quasar_gpio_init(gpio_config_ldo_led);
    quasar_gpio_clear(gpio_config_ldo_led.port, gpio_config_ldo_led.pin);

    quasar_gpio_config_t gpio_config_vdd_select = {
        .port       = QUASAR_DEF_VDD_SEL_PORT,
        .pin        = QUASAR_DEF_VDD_SEL_PIN,
        .mode       = QUASAR_GPIO_MODE_OUTPUT,
        .type       = QUASAR_GPIO_TYPE_PP,
        .pull       = QUASAR_GPIO_PULL_NONE,
        .speed      = QUASAR_GPIO_SPEED_LOW,
        .alternate  = QUASAR_GPIO_ALTERNATE_NONE
    };
    quasar_gpio_init(gpio_config_vdd_select);
}

void quasar_power_set_vdd_level(quasar_vdd_selection_t quasar_vdd_selection)
{
    if (quasar_vdd_selection == QUASAR_VDD_SELECTION_1V8) {
        quasar_gpio_clear(QUASAR_DEF_VDD_SEL_PORT, QUASAR_DEF_VDD_SEL_PIN);
    } else if (quasar_vdd_selection == QUASAR_VDD_SELECTION_3V3) {
        quasar_gpio_set(QUASAR_DEF_VDD_SEL_PORT, QUASAR_DEF_VDD_SEL_PIN);
    }
}

void quasar_power_enable_ldo_led(void)
{
    quasar_gpio_set(QUASAR_DEF_LDO_LED_EN_PORT, QUASAR_DEF_LDO_LED_EN_PIN);
}

void quasar_power_disable_ldo_led(void)
{
    quasar_gpio_clear(QUASAR_DEF_LDO_LED_EN_PORT, QUASAR_DEF_LDO_LED_EN_PIN);
}

void quasar_power_enable_ldo_mcu(quasar_revision_t board_revision)
{
    if (board_revision == QUASAR_REVA) {
        quasar_gpio_set(QUASAR_DEF_LDO_MCU_EN_PORT_REVA, QUASAR_DEF_LDO_MCU_EN_PIN_REVA);
    } else if (board_revision == QUASAR_REVB) {
        quasar_gpio_set(QUASAR_DEF_LDO_MCU_EN_PORT_REVB, QUASAR_DEF_LDO_MCU_EN_PIN_REVB);
    } else {
        /* Board revision unsupported. */
        while (1);
    }
}

void quasar_power_disable_ldo_mcu(quasar_revision_t board_revision)
{
    if (board_revision == QUASAR_REVA) {
        quasar_gpio_clear(QUASAR_DEF_LDO_MCU_EN_PORT_REVA, QUASAR_DEF_LDO_MCU_EN_PIN_REVA);
    } else if (board_revision == QUASAR_REVB) {
        quasar_gpio_clear(QUASAR_DEF_LDO_MCU_EN_PORT_REVB, QUASAR_DEF_LDO_MCU_EN_PIN_REVB);
    } else {
        /* Board revision unsupported. */
        while (1);
    }
}
