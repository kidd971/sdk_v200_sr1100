/** @file  quasar_gpio.c
 *  @brief This module controls gpio related features.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_gpio.h"

/* MACROS *********************************************************************/
/*! Configure the GPIO mode. */
#define GPIO_MODE(PORT, PIN, MODE) (PORT->MODER = (PORT->MODER & (~(0b11 << (2 * PIN)))) | (MODE << (2 * PIN)))
/* Configure the GPIO type */
#define GPIO_TYPE(PORT, PIN, TYPE) (PORT->OTYPER = (PORT->OTYPER & (~(0b01 << (1 * PIN)))) | (TYPE << (1 * PIN)))
/* Configure the GPIO pull up/down */
#define GPIO_PULL(PORT, PIN, PULL) (PORT->PUPDR = (PORT->PUPDR & (~(0b11 << (2 * PIN)))) | (PULL << (2 * PIN)))
/* Configure the GPIO speed */
#define GPIO_SPEED(PORT, PIN, SPEED) (PORT->OSPEEDR = (PORT->OSPEEDR & (~(0b11 << (2 * PIN)))) | (SPEED << (2 * PIN)))
/* Configure the GPIO alternate function */
#define GPIO_FUNC(PORT, PIN, FUNC) (PORT->AFR[PIN/8] = (PORT->AFR[PIN/8] & (~(0b1111 << ((PIN % 8) * 4)))) | (FUNC << ((PIN % 8) * 4)))

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static GPIO_TypeDef *gpio_get_ref_port(quasar_gpio_port_t port);
static uint16_t gpio_get_ref_pin(quasar_gpio_pin_t pin);
static IRQn_Type gpio_get_selected_irq(quasar_gpio_pin_t gpio_pin);
static void gpio_select_port_for_exti_line(quasar_gpio_port_t gpio_port, quasar_gpio_pin_t gpio_pin);

/* PUBLIC FUNCTIONS ***********************************************************/
void quasar_gpio_init(quasar_gpio_config_t gpio_config)
{
    GPIO_TypeDef *port = gpio_get_ref_port(gpio_config.port);

    GPIO_MODE(port, gpio_config.pin, gpio_config.mode);
    GPIO_TYPE(port, gpio_config.pin, gpio_config.type);
    GPIO_PULL(port, gpio_config.pin, gpio_config.pull);
    GPIO_SPEED(port, gpio_config.pin, gpio_config.speed);
    GPIO_FUNC(port, gpio_config.pin, gpio_config.alternate);
}

void quasar_gpio_deinit(quasar_gpio_port_t gpio_port, quasar_gpio_pin_t gpio_pin)
{
    GPIO_TypeDef *port = gpio_get_ref_port(gpio_port);
    uint16_t pin = gpio_get_ref_pin(gpio_pin);

    HAL_GPIO_DeInit(port, pin);
}

void quasar_gpio_configure_irq(quasar_gpio_port_t gpio_port, quasar_gpio_pin_t gpio_pin, quasar_irq_priority_t irq_priority)
{
    IRQn_Type gpio_irq;

    /* Enable the rising edge event. */
    QUASAR_SET_BIT(EXTI->RTSR1, (1 << gpio_pin));
    /* Configure the mux to select the port of the GPIO. */
    gpio_select_port_for_exti_line(gpio_port, gpio_pin);

    /* Configure the priority and enable interrupt. */
    gpio_irq = gpio_get_selected_irq(gpio_pin);

    NVIC_SetPriority(gpio_irq, irq_priority);
    NVIC_EnableIRQ(gpio_irq);
}

void quasar_gpio_configure_rising_and_falling_edges_irq(quasar_gpio_port_t gpio_port, quasar_gpio_pin_t gpio_pin, quasar_irq_priority_t irq_priority)
{
    IRQn_Type gpio_irq;

    /* Enable the rising and falling edge event. */
    QUASAR_SET_BIT(EXTI->RTSR1, (1 << gpio_pin));
    QUASAR_SET_BIT(EXTI->FTSR1, (1 << gpio_pin));
    /* Configure the mux to select the port of the GPIO. */
    gpio_select_port_for_exti_line(gpio_port, gpio_pin);

    /* Configure the priority and enable interrupt. */
    gpio_irq = gpio_get_selected_irq(gpio_pin);

    NVIC_SetPriority(gpio_irq, irq_priority);
    NVIC_EnableIRQ(gpio_irq);
}

void quasar_gpio_enable_irq(quasar_gpio_pin_t gpio_pin)
{
    /* Mask the interrupt event line. */
    QUASAR_SET_BIT(EXTI->IMR1, (1 << gpio_pin));
}

void quasar_gpio_disable_irq(quasar_gpio_pin_t gpio_pin)
{
    /* Unmask the interrupt event line. */
    QUASAR_CLEAR_BIT(EXTI->IMR1, (1 << gpio_pin));
    /* Clear the interruption flag. */
    QUASAR_SET_BIT(EXTI->RPR1, (1 << gpio_pin));
}

void quasar_gpio_set_pending(quasar_gpio_pin_t gpio_pin)
{
    IRQn_Type gpio_irq = gpio_get_selected_irq(gpio_pin);

    __NVIC_SetPendingIRQ(gpio_irq);
}

void quasar_gpio_clock_enable(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();
}

void quasar_gpio_set(quasar_gpio_port_t port, quasar_gpio_pin_t pin)
{
    GPIO_TypeDef *ref_port = gpio_get_ref_port(port);

    ref_port->BSRR |= (1 << pin);
}

void quasar_gpio_clear(quasar_gpio_port_t port, quasar_gpio_pin_t pin)
{
    GPIO_TypeDef *ref_port = gpio_get_ref_port(port);

    ref_port->BRR |= (1 << pin);
}

void quasar_gpio_toggle(quasar_gpio_port_t port, quasar_gpio_pin_t pin)
{
    GPIO_TypeDef *ref_port = gpio_get_ref_port(port);

    if ((ref_port->ODR & (1 << pin)) != 0U) {
        quasar_gpio_clear(port, pin);
    } else {
        quasar_gpio_set(port, pin);
    }
}

bool quasar_gpio_read_state(quasar_gpio_port_t port, quasar_gpio_pin_t pin)
{
    GPIO_TypeDef *ref_port = gpio_get_ref_port(port);

    if (QUASAR_READ_BIT(ref_port->IDR, pin)) {
        return true;
    } else {
        return false;
    }
}

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
/** @brief Retrieves the pointer to the GPIO port specified by the port argument.
 *
 *  @param[in] port  Selected GPIO port.
 *  @return The pointer to the GPIO port.
 */
static GPIO_TypeDef *gpio_get_ref_port(quasar_gpio_port_t port)
{
    GPIO_TypeDef *ref_port;

    switch (port) {
    case QUASAR_GPIO_PORT_A:
        ref_port = GPIOA;
        break;
    case QUASAR_GPIO_PORT_B:
        ref_port = GPIOB;
        break;
    case QUASAR_GPIO_PORT_C:
        ref_port = GPIOC;
        break;
    case QUASAR_GPIO_PORT_D:
        ref_port = GPIOD;
        break;
    case QUASAR_GPIO_PORT_E:
        ref_port = GPIOE;
        break;
    case QUASAR_GPIO_PORT_F:
        ref_port = GPIOF;
        break;
    case QUASAR_GPIO_PORT_G:
        ref_port = GPIOG;
        break;
    case QUASAR_GPIO_PORT_H:
        ref_port = GPIOH;
        break;
    case QUASAR_GPIO_PORT_I:
        ref_port = GPIOI;
        break;
    default:
        ref_port = NULL;
        break;
    }

    return ref_port;
}

/** @brief Retrieves the pointer to the GPIO pin specified by the argument.
 *
 *  @param[in] pin  Selected GPIO pin.
 *  @return The reference HAL's GPIO pin.
 */
static uint16_t gpio_get_ref_pin(quasar_gpio_pin_t pin)
{
    /**
     *  GPIO_PIN_0      ((uint16_t)0x0001)       -> 0000 0000 0000 0001     1 << 0
     *  GPIO_PIN_1      ((uint16_t)0x0002)       -> 0000 0000 0000 0010     1 << 1
     *  GPIO_PIN_2      ((uint16_t)0x0004)       -> 0000 0000 0000 0100     1 << 2
     *  GPIO_PIN_3      ((uint16_t)0x0008)       -> 0000 0000 0000 1000      ...
     *  GPIO_PIN_4      ((uint16_t)0x0010)       -> 0000 0000 0001 0000
     *   ...
     *  GPIO_PIN_15     ((uint16_t)0x8000)       -> 1000 0000 0000 0000     1 << 15
     */
    if (pin <= QUASAR_GPIO_PIN_15) {
        return (uint16_t)(1 << pin);
    }
    return 0;
}

/** @brief Return the selected GPIO's global interrupt.
 *
 *  @param[in] gpio_pin  Selected timer to get associated global interrupt.
 *  @return Selected GPIO global interrupt.
 */
static IRQn_Type gpio_get_selected_irq(quasar_gpio_pin_t gpio_pin)
{
    IRQn_Type gpio_irq = {0};

    switch (gpio_pin) {
    case QUASAR_GPIO_PIN_0:
        gpio_irq = EXTI0_IRQn;
        break;
    case QUASAR_GPIO_PIN_1:
        gpio_irq = EXTI1_IRQn;
        break;
    case QUASAR_GPIO_PIN_2:
        gpio_irq = EXTI2_IRQn;
        break;
    case QUASAR_GPIO_PIN_3:
        gpio_irq = EXTI3_IRQn;
        break;
    case QUASAR_GPIO_PIN_4:
        gpio_irq = EXTI4_IRQn;
        break;
    case QUASAR_GPIO_PIN_5:
        gpio_irq = EXTI5_IRQn;
        break;
    case QUASAR_GPIO_PIN_6:
        gpio_irq = EXTI6_IRQn;
        break;
    case QUASAR_GPIO_PIN_7:
        gpio_irq = EXTI7_IRQn;
        break;
    case QUASAR_GPIO_PIN_8:
        gpio_irq = EXTI8_IRQn;
        break;
    case QUASAR_GPIO_PIN_9:
        gpio_irq = EXTI9_IRQn;
        break;
    case QUASAR_GPIO_PIN_10:
        gpio_irq = EXTI10_IRQn;
        break;
    case QUASAR_GPIO_PIN_11:
        gpio_irq = EXTI11_IRQn;
        break;
    case QUASAR_GPIO_PIN_12:
        gpio_irq = EXTI12_IRQn;
        break;
    case QUASAR_GPIO_PIN_13:
        gpio_irq = EXTI13_IRQn;
        break;
    case QUASAR_GPIO_PIN_14:
        gpio_irq = EXTI14_IRQn;
        break;
    case QUASAR_GPIO_PIN_15:
        gpio_irq = EXTI15_IRQn;
        break;
    default:
        /* Trigger an exception. */
        break;
    }

    return gpio_irq;
}

/** @brief Select the port for the interrupt line pin.
 *
 *  @note Once a port is selected for a pin number, that pin number is
 *        reserved across all other ports. For instance, if port C is
 *        chosen for pin 13, pin 13 on any other port becomes unavailable.
 *
 *  @param[in] port  The GPIO port to set.
 *  @param[in] pin   The GPIO pin to set.
 */
static void gpio_select_port_for_exti_line(quasar_gpio_port_t gpio_port, quasar_gpio_pin_t gpio_pin)
{
    /**
     *  Refer to the reference manual :
     *
     *      EXTI_EXTICR1            EXTI_EXTICR2
     *
     *      [0:7]   : PX0           [0:7]   : PX4
     *      [8:15]  : PX1           [8:15]  : PX5
     *      [16:23] : PX2           [16:23] : PX6
     *      [24:31] : PX3           [24:31] : PX7
     *
     *      EXTI_EXTICR3            EXTI_EXTICR4
     *
     *      [0:7]   : PX8           [0:7]   : PX12
     *      [8:15]  : PX9           [8:15]  : PX13
     *      [16:23] : PX10          [16:23] : PX14
     *      [24:31] : PX11          [24:31] : PX15
     */

    switch (gpio_pin) {
    case QUASAR_GPIO_PIN_0:
        QUASAR_WRITE_BITS(EXTI->EXTICR[0], EXTI_EXTICR1_EXTI0_Msk, EXTI_EXTICR1_EXTI0_Pos, gpio_port);
        break;
    case QUASAR_GPIO_PIN_1:
        QUASAR_WRITE_BITS(EXTI->EXTICR[0], EXTI_EXTICR1_EXTI1_Msk, EXTI_EXTICR1_EXTI1_Pos, gpio_port);
        break;
    case QUASAR_GPIO_PIN_2:
        QUASAR_WRITE_BITS(EXTI->EXTICR[0], EXTI_EXTICR1_EXTI2_Msk, EXTI_EXTICR1_EXTI2_Pos, gpio_port);
        break;
    case QUASAR_GPIO_PIN_3:
        QUASAR_WRITE_BITS(EXTI->EXTICR[0], EXTI_EXTICR1_EXTI3_Msk, EXTI_EXTICR1_EXTI3_Pos, gpio_port);
        break;
    case QUASAR_GPIO_PIN_4:
        QUASAR_WRITE_BITS(EXTI->EXTICR[1], EXTI_EXTICR2_EXTI4_Msk, EXTI_EXTICR2_EXTI4_Pos, gpio_port);
        break;
    case QUASAR_GPIO_PIN_5:
        QUASAR_WRITE_BITS(EXTI->EXTICR[1], EXTI_EXTICR2_EXTI5_Msk, EXTI_EXTICR2_EXTI5_Pos, gpio_port);
        break;
    case QUASAR_GPIO_PIN_6:
        QUASAR_WRITE_BITS(EXTI->EXTICR[1], EXTI_EXTICR2_EXTI6_Msk, EXTI_EXTICR2_EXTI6_Pos, gpio_port);
        break;
    case QUASAR_GPIO_PIN_7:
        QUASAR_WRITE_BITS(EXTI->EXTICR[1], EXTI_EXTICR2_EXTI7_Msk, EXTI_EXTICR2_EXTI7_Pos, gpio_port);
        break;
    case QUASAR_GPIO_PIN_8:
        QUASAR_WRITE_BITS(EXTI->EXTICR[2], EXTI_EXTICR3_EXTI8_Msk, EXTI_EXTICR3_EXTI8_Pos, gpio_port);
        break;
    case QUASAR_GPIO_PIN_9:
        QUASAR_WRITE_BITS(EXTI->EXTICR[2], EXTI_EXTICR3_EXTI9_Msk, EXTI_EXTICR3_EXTI9_Pos, gpio_port);
        break;
    case QUASAR_GPIO_PIN_10:
        QUASAR_WRITE_BITS(EXTI->EXTICR[2], EXTI_EXTICR3_EXTI10_Msk, EXTI_EXTICR3_EXTI10_Pos, gpio_port);
        break;
    case QUASAR_GPIO_PIN_11:
        QUASAR_WRITE_BITS(EXTI->EXTICR[2], EXTI_EXTICR3_EXTI11_Msk, EXTI_EXTICR3_EXTI11_Pos, gpio_port);
        break;
    case QUASAR_GPIO_PIN_12:
        QUASAR_WRITE_BITS(EXTI->EXTICR[3], EXTI_EXTICR4_EXTI12_Msk, EXTI_EXTICR4_EXTI12_Pos, gpio_port);
        break;
    case QUASAR_GPIO_PIN_13:
        QUASAR_WRITE_BITS(EXTI->EXTICR[3], EXTI_EXTICR4_EXTI13_Msk, EXTI_EXTICR4_EXTI13_Pos, gpio_port);
        break;
    case QUASAR_GPIO_PIN_14:
        QUASAR_WRITE_BITS(EXTI->EXTICR[3], EXTI_EXTICR4_EXTI14_Msk, EXTI_EXTICR4_EXTI14_Pos, gpio_port);
        break;
    case QUASAR_GPIO_PIN_15:
        QUASAR_WRITE_BITS(EXTI->EXTICR[3], EXTI_EXTICR4_EXTI15_Msk, EXTI_EXTICR4_EXTI15_Pos, gpio_port);
        break;
    default:
        /* Trigger an exception. */
        break;
    }
}
