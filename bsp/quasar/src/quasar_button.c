/** @file  quasar_button.c
 *  @brief This module configures buttons and provides functions to control each of them.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_button.h"
#include "quasar_gpio.h"

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
void button_init(quasar_button_selection_t button_selection, quasar_irq_priority_t irq_priority);
void button_deinit(quasar_button_selection_t button_selection);
static quasar_gpio_config_t button_get_config(quasar_button_selection_t button_selection);

/* PUBLIC FUNCTIONS ***********************************************************/
void quasar_button_init(void)
{
    button_init(QUASAR_BUTTON_USER_1, QUASAR_IRQ_PRIORITY_NONE);
    button_init(QUASAR_BUTTON_USER_2, QUASAR_IRQ_PRIORITY_NONE);
    button_init(QUASAR_BUTTON_USER_3, QUASAR_IRQ_PRIORITY_NONE);
    button_init(QUASAR_BUTTON_USER_4, QUASAR_IRQ_PRIORITY_NONE);
}

void quasar_button_deinit(void)
{
    button_deinit(QUASAR_BUTTON_USER_1);
    button_deinit(QUASAR_BUTTON_USER_2);
    button_deinit(QUASAR_BUTTON_USER_3);
    button_deinit(QUASAR_BUTTON_USER_4);
}

void quasar_button_configure_irq(quasar_button_selection_t button_selection, quasar_irq_priority_t irq_priority)
{
    quasar_gpio_config_t button_config = button_get_config(button_selection);

    /* Configure interrupt and enable it. */
    quasar_gpio_configure_irq(button_config.port, button_config.pin, irq_priority);
    quasar_gpio_enable_irq(button_config.pin);
}

void quasar_button_enable_irq(quasar_button_selection_t button_selection)
{
    quasar_gpio_config_t button_config = button_get_config(button_selection);

    quasar_gpio_enable_irq(button_config.pin);
}

void quasar_button_disable_irq(quasar_button_selection_t button_selection)
{
    quasar_gpio_config_t button_config = button_get_config(button_selection);

    quasar_gpio_disable_irq(button_config.pin);
}

void quasar_button_set_button1_callback(void (*irq_callback)(void))
{
    quasar_it_set_exti10_irq_callback(irq_callback);
}

void quasar_button_set_button2_callback(void (*irq_callback)(void))
{
    quasar_it_set_exti12_irq_callback(irq_callback);
}

void quasar_button_set_button3_callback(void (*irq_callback)(void))
{
    quasar_it_set_rising_edge_exti15_irq_callback(irq_callback);
}

void quasar_button_set_button4_callback(void (*irq_callback)(void))
{
    quasar_it_set_exti0_irq_callback(irq_callback);
}

bool quasar_button_read_state(quasar_button_selection_t button_selection)
{
    quasar_gpio_config_t button_config = button_get_config(button_selection);

    bool button_state = quasar_gpio_read_state(button_config.port, button_config.pin);

    return button_state;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize the selected button peripheral.
 *
 *  @param[in] button_selection  Selected button peripheral.
 *  @param[in] irq_priority      IRQ priority is optional; if the button does not trigger an IRQ, assign QUASAR_IRQ_PRIORITY_NONE.
 */
void button_init(quasar_button_selection_t button_selection, quasar_irq_priority_t irq_priority)
{
    quasar_gpio_config_t button_config = button_get_config(button_selection);

    /* Initialize the GPIO associated with the button. */
    quasar_gpio_init(button_config);
    if (irq_priority != QUASAR_IRQ_PRIORITY_NONE) {
        /* Configure interrupt and enable it. */
        quasar_gpio_configure_irq(button_config.port, button_config.pin, irq_priority);
        quasar_gpio_enable_irq(button_config.pin);
    }
}

/** @brief Deinitialize the selected button peripheral.
 *
 *  @param[in] button_selection  Selected button peripheral.
 */
void button_deinit(quasar_button_selection_t button_selection)
{
    quasar_gpio_config_t button_config = button_get_config(button_selection);

    quasar_gpio_deinit(button_config.port, button_config.pin);
}

/** @brief Get the configuration of the button peripheral.
 *
 *  All buttons are connected to VDD and have capacitors for debouncing.
 *  The external interrupt (EXTI) linked to the button corresponds
 *  to the GPIO pin to which the button is connected.
 *
 *  @param[in] button_selection  Selected button peripheral.
 *  @return The button peripheral configuration.
 */
static quasar_gpio_config_t button_get_config(quasar_button_selection_t button_selection)
{
    quasar_gpio_config_t config;

    switch (button_selection) {
    case QUASAR_BUTTON_USER_1:
        config.port         = QUASAR_DEF_BUTTON_USER_1_PORT;
        config.pin          = QUASAR_DEF_BUTTON_USER_1_PIN;
        config.mode         = QUASAR_GPIO_MODE_INPUT;
        config.type         = QUASAR_GPIO_TYPE_NONE;
        config.pull         = QUASAR_GPIO_PULL_DOWN;
        config.speed        = QUASAR_GPIO_SPEED_LOW;
        config.alternate    = QUASAR_GPIO_ALTERNATE_NONE;
        break;
    case QUASAR_BUTTON_USER_2:
        config.port         = QUASAR_DEF_BUTTON_USER_2_PORT;
        config.pin          = QUASAR_DEF_BUTTON_USER_2_PIN;
        config.mode         = QUASAR_GPIO_MODE_INPUT;
        config.type         = QUASAR_GPIO_TYPE_NONE;
        config.pull         = QUASAR_GPIO_PULL_DOWN;
        config.speed        = QUASAR_GPIO_SPEED_LOW;
        config.alternate    = QUASAR_GPIO_ALTERNATE_NONE;
        break;
    case QUASAR_BUTTON_USER_3:
        config.port         = QUASAR_DEF_BUTTON_USER_3_PORT;
        config.pin          = QUASAR_DEF_BUTTON_USER_3_PIN;
        config.mode         = QUASAR_GPIO_MODE_INPUT;
        config.type         = QUASAR_GPIO_TYPE_NONE;
        config.pull         = QUASAR_GPIO_PULL_DOWN;
        config.speed        = QUASAR_GPIO_SPEED_LOW;
        config.alternate    = QUASAR_GPIO_ALTERNATE_NONE;
        break;
    case QUASAR_BUTTON_USER_4:
        config.port         = QUASAR_DEF_BUTTON_USER_4_PORT;
        config.pin          = QUASAR_DEF_BUTTON_USER_4_PIN;
        config.mode         = QUASAR_GPIO_MODE_INPUT;
        config.type         = QUASAR_GPIO_TYPE_NONE;
        config.pull         = QUASAR_GPIO_PULL_DOWN;
        config.speed        = QUASAR_GPIO_SPEED_LOW;
        config.alternate    = QUASAR_GPIO_ALTERNATE_NONE;
        break;
    default:
        break;
    }

    return config;
}
