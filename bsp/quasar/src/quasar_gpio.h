/** @file  quasar_gpio.h
 *  @brief This module controls GPIO related features.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_GPIO_H_
#define QUASAR_GPIO_H_

#ifdef __cplusplus
extern "C" {
#endif

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include "quasar_def.h"
#include "quasar_it.h"

/* TYPES **********************************************************************/
/** @brief Quasar BSP GPIO port selection.
 */
typedef enum quasar_gpio_port {
    /*! GPIO port A */
    QUASAR_GPIO_PORT_A,
    /*! GPIO port B */
    QUASAR_GPIO_PORT_B,
    /*! GPIO port C */
    QUASAR_GPIO_PORT_C,
    /*! GPIO port D */
    QUASAR_GPIO_PORT_D,
    /*! GPIO port E */
    QUASAR_GPIO_PORT_E,
    /*! GPIO port F */
    QUASAR_GPIO_PORT_F,
    /*! GPIO port G */
    QUASAR_GPIO_PORT_G,
    /*! GPIO port H */
    QUASAR_GPIO_PORT_H,
    /*! GPIO port I */
    QUASAR_GPIO_PORT_I
} quasar_gpio_port_t;

/** @brief Quasar BSP GPIO pin selection.
 */
typedef enum quasar_gpio_pin {
    /*! GPIO pin 0 */
    QUASAR_GPIO_PIN_0   = 0,
    /*! GPIO pin 1 */
    QUASAR_GPIO_PIN_1   = 1,
    /*! GPIO pin 2 */
    QUASAR_GPIO_PIN_2   = 2,
    /*! GPIO pin 3 */
    QUASAR_GPIO_PIN_3   = 3,
    /*! GPIO pin 4 */
    QUASAR_GPIO_PIN_4   = 4,
    /*! GPIO pin 5 */
    QUASAR_GPIO_PIN_5   = 5,
    /*! GPIO pin 6 */
    QUASAR_GPIO_PIN_6   = 6,
    /*! GPIO pin 7 */
    QUASAR_GPIO_PIN_7   = 7,
    /*! GPIO pin 8 */
    QUASAR_GPIO_PIN_8   = 8,
    /*! GPIO pin 9 */
    QUASAR_GPIO_PIN_9   = 9,
    /*! GPIO pin 10 */
    QUASAR_GPIO_PIN_10  = 10,
    /*! GPIO pin 11 */
    QUASAR_GPIO_PIN_11  = 11,
    /*! GPIO pin 12 */
    QUASAR_GPIO_PIN_12  = 12,
    /*! GPIO pin 13 */
    QUASAR_GPIO_PIN_13  = 13,
    /*! GPIO pin 14 */
    QUASAR_GPIO_PIN_14  = 14,
    /*! GPIO pin 15 */
    QUASAR_GPIO_PIN_15  = 15
} quasar_gpio_pin_t;

/** @brief Quasar BSP GPIO mode configuration selection.
 */
typedef enum quasar_gpio_mode {
    /*! GPIO mode configured as digital input. */
    QUASAR_GPIO_MODE_INPUT      = 0,
    /*! GPIO mode configured as digital output. */
    QUASAR_GPIO_MODE_OUTPUT     = 1,
    /*! GPIO mode configured for alternate functions. */
    QUASAR_GPIO_MODE_ALTERNATE  = 2,
    /*! GPIO mode configured for analog operations. */
    QUASAR_GPIO_MODE_ANALOG     = 3,
} quasar_gpio_mode_t;

/** @brief Quasar BSP GPIO type configuration selection.
 */
typedef enum quasar_gpio_type {
    /*! No GPIO type selected. */
    QUASAR_GPIO_TYPE_NONE = 0,
    /*! GPIO configured as push-pull type. */
    QUASAR_GPIO_TYPE_PP   = 0,
    /*! GPIO configured as open-drain type. */
    QUASAR_GPIO_TYPE_OD   = 1
} quasar_gpio_type_t;

/** @brief Quasar BSP GPIO speed configuration selection.
 */
typedef enum quasar_gpio_speed {
    /*! No GPIO speed configuration selected. */
    QUASAR_GPIO_SPEED_NONE      = 0,
    /*! GPIO configured for low speed operation. */
    QUASAR_GPIO_SPEED_LOW       = 0,
    /*! GPIO configured for medium speed operation. */
    QUASAR_GPIO_SPEED_MEDIUM    = 1,
    /*! GPIO configured for high speed operation. */
    QUASAR_GPIO_SPEED_HIGH      = 2,
    /*! GPIO configured for very high speed operation. */
    QUASAR_GPIO_SPEED_VERY_HIGH = 3
} quasar_gpio_speed_t;

/** @brief Quasar BSP GPIO pull up/down configuration selection.
 */
typedef enum quasar_gpio_pull {
    /*! No pull-up or pull down resistor configuration selected. */
    QUASAR_GPIO_PULL_NONE = 0,
    /*! GPIO configured with pull-up resistor. */
    QUASAR_GPIO_PULL_UP   = 1,
    /*! GPIO configured with pull-down resistor. */
    QUASAR_GPIO_PULL_DOWN = 2
} quasar_gpio_pull_t;

/** @brief Quasar BSP GPIO alternate function configuration selection.
 */
typedef enum quasar_gpio_alternate_function {
    /*! No alternate function selected. */
    QUASAR_GPIO_ALTERNATE_NONE = 0,
    /*! GPIO alternate function 0. */
    QUASAR_GPIO_ALTERNATE_AF0  = 0,
    /*! GPIO alternate function 1 */
    QUASAR_GPIO_ALTERNATE_AF1  = 1,
    /*! GPIO alternate function 2 */
    QUASAR_GPIO_ALTERNATE_AF2  = 2,
    /*! GPIO alternate function 3 */
    QUASAR_GPIO_ALTERNATE_AF3  = 3,
    /*! GPIO alternate function 4 */
    QUASAR_GPIO_ALTERNATE_AF4  = 4,
    /*! GPIO alternate function 5 */
    QUASAR_GPIO_ALTERNATE_AF5  = 5,
    /*! GPIO alternate function 6 */
    QUASAR_GPIO_ALTERNATE_AF6  = 6,
    /*! GPIO alternate function 7 */
    QUASAR_GPIO_ALTERNATE_AF7  = 7,
    /*! GPIO alternate function 8 */
    QUASAR_GPIO_ALTERNATE_AF8  = 8,
    /*! GPIO alternate function 9 */
    QUASAR_GPIO_ALTERNATE_AF9  = 9,
    /*! GPIO alternate function 10 */
    QUASAR_GPIO_ALTERNATE_AF10 = 10,
    /*! GPIO alternate function 11 */
    QUASAR_GPIO_ALTERNATE_AF11 = 11,
    /*! GPIO alternate function 12 */
    QUASAR_GPIO_ALTERNATE_AF12 = 12,
    /*! GPIO alternate function 13 */
    QUASAR_GPIO_ALTERNATE_AF13 = 13,
    /*! GPIO alternate function 14 */
    QUASAR_GPIO_ALTERNATE_AF14 = 14,
    /*! GPIO alternate function 15 */
    QUASAR_GPIO_ALTERNATE_AF15 = 15
} quasar_gpio_alternate_t;

/** @brief Quasar BSP GPIO configuration.
 */
typedef struct quasar_gpio_config {
    /*! GPIO port selection. */
    quasar_gpio_port_t port;
    /* GPIO pin selection. */
    quasar_gpio_pin_t pin;
    /*! GPIO mode selection. */
    quasar_gpio_mode_t mode;
    /*! GPIO type selection. */
    quasar_gpio_type_t type;
    /*! GPIO speed selection.*/
    quasar_gpio_speed_t speed;
    /*! GPIO pull-up/down configuration. */
    quasar_gpio_pull_t pull;
    /*! GPIO alternate function selection. */
    quasar_gpio_alternate_t alternate;
} quasar_gpio_config_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize GPIO peripheral.
 *
 *  @param[in] gpio_config  Selected GPIO configuration.
 */
void quasar_gpio_init(quasar_gpio_config_t gpio_config);

/** @brief Deinitialize GPIO peripheral.
 *
 *  @param[in] gpio_port     The GPIO port to set.
 *  @param[in] gpio_pin      Selected GPIO pin.
 */
void quasar_gpio_deinit(quasar_gpio_port_t gpio_port, quasar_gpio_pin_t gpio_pin);

/** @brief Configure the GPIO's global interrupt on rising edge.
 *
 *  @note This function should not be used with a QUASAR_IRQ_PRIORITY_NONE.
 *
 *  @param[in] port          The GPIO port to set.
 *  @param[in] gpio_pin      Selected GPIO pin.
 *  @param[in] irq_priority  IRQ priority.
 */
void quasar_gpio_configure_irq(quasar_gpio_port_t gpio_port, quasar_gpio_pin_t gpio_pin, quasar_irq_priority_t irq_priority);

/** @brief Configure the GPIO's global interrupt on both edges.
 *
 *  @note This function should not be used with a QUASAR_IRQ_PRIORITY_NONE.
 *
 *  @param[in] port          The GPIO port to set.
 *  @param[in] gpio_pin      Selected GPIO pin.
 *  @param[in] irq_priority  IRQ priority.
 */
void quasar_gpio_configure_rising_and_falling_edges_irq(quasar_gpio_port_t gpio_port, quasar_gpio_pin_t gpio_pin, quasar_irq_priority_t irq_priority);

/** @brief Enable the GPIO's global interrupt.
 *
 *  @note This function should only be use if the GPIO has a interrupt configured.
 *
 *  @param[in] gpio_pin  Selected GPIO pin.
 */
void quasar_gpio_enable_irq(quasar_gpio_pin_t gpio_pin);

/** @brief Disable the GPIO's global interrupt.
 *
 *  @note This function should only be use if the GPIO has a interrupt configured.
 *
 *  @param[in] gpio_pin  Selected GPIO pin.
 */
void quasar_gpio_disable_irq(quasar_gpio_pin_t gpio_pin);

/** @brief Set the specified GPIO pin's interrupt as pending.
 *
 *  This function manually forces an interrupt to enter the pending state for a specified GPIO pin.
 *
 *  @note This function doesn't configure the interrupt itself; the interrupt for the specified GPIO
 *        pin should be already configured properly before calling this function.
 *
 *  @param[in] gpio_pin  Selected GPIO pin.
 */
void quasar_gpio_set_pending(quasar_gpio_pin_t gpio_pin);

/** @brief Enable all GPIO's peripheral clock.
 */
void quasar_gpio_clock_enable(void);

/** @brief Set GPIO output.
 *
 *  @note The GPIO must have been initialized in output mode.
 *
 *  @param[in] port  The GPIO port to set.
 *  @param[in] pin   The GPIO pin to set.
 */
void quasar_gpio_set(quasar_gpio_port_t port, quasar_gpio_pin_t pin);

/** @brief Clear GPIO output.
 *
 *  @note The GPIO must have been initialized in output mode.
 *
 *  @param[in] port  The GPIO port to clear.
 *  @param[in] pin   The GPIO pin to clear.
 */
void quasar_gpio_clear(quasar_gpio_port_t port, quasar_gpio_pin_t pin);

/** @brief Toggle GPIO output.
 *
 *  @note The GPIO must have been initialized in output mode.
 *
 *  @param[in] port  The GPIO port to toggle.
 *  @param[in] pin   The GPIO pin to toggle.
 */
void quasar_gpio_toggle(quasar_gpio_port_t port, quasar_gpio_pin_t pin);

/** @brief Read GPIO state.
 *
 *  @note The GPIO must have been initialized in input mode.
 *
 *  @param[in] port  The GPIO port to read.
 *  @param[in] pin   The output GPIO pin to read.
 *  @return Return true if the input value is high, false otherwise.
 */
bool quasar_gpio_read_state(quasar_gpio_port_t port, quasar_gpio_pin_t pin);

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_GPIO_H_ */
