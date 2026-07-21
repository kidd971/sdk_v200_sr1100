/** @file  quasar_backend.c
 *  @brief Implement BSP validator facade prototype functions.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "bsp_validator_facade.h"
#include "quasar.h"
#include "quasar_def.h"
#include "quasar_gpio.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void facade_bsp_init(void)
{
    quasar_bsp_status_t quasar_err = QUASAR_OK;

    quasar_config_t quasar_cfg = {
        .clk_freq = QUASAR_CLK_160MHZ,
        .debug_enabled = false,
        /* Dual: both on. Single: only the physically selected transceiver (default radio 1). */
        .radio1_enabled = (SWC_RADIO_COUNT > 1) || (SWC_SINGLE_RADIO_ID == 1),
        .radio2_enabled = (SWC_RADIO_COUNT > 1) || (SWC_SINGLE_RADIO_ID == 2),
        .adc_enabled = false,
        .quasar_vdd_selection = QUASAR_VDD_SELECTION_1V8,
    };

    quasar_init(quasar_cfg, &quasar_err);
    ASSERT_QUASAR_BSP_STATUS(quasar_err);
}

void facade_debug_led_blink(facade_debug_led_t led, uint32_t count, uint32_t delay_loops)
{
    /*
     * Zero-dependency, cannot-hang liveness blink: raw register access only.
     * No HAL, no power/clock config, nothing that waits on a ready flag. Runs
     * at the very top of main() on whatever clock the reset state provides.
     *
     * Interpretation if this does NOT blink: the MCU is not executing main()
     * at all (wrong/again-old binary flashed, bad flash offset, or a fault
     * before main) - it is NOT a power or BSP-init problem, since the LEDs on
     * this board are externally powered and can all be lit.
     *
     * The three RGB channels are supported so radio 1 / radio 2 results are
     * distinguishable at a glance instead of by counting blinks:
     *   FACADE_DEBUG_LED_BLUE  = PD7  (RGB blue)  - boot markers + radio 1.
     *   FACADE_DEBUG_LED_GREEN = PB4  (RGB green) - radio 2.
     *   FACADE_DEBUG_LED_RED   = PH11 (RGB red).
     * The banks are enabled here so the function stays self-contained when
     * called pre-init. PB4 is the JTAG NJTRST pin; we debug over SWD, so
     * reconfiguring it as a plain output is safe. The busy-wait is made large so
     * the blink stays visible even if the core is already at 160 MHz.
     */
    GPIO_TypeDef *gpio;
    uint32_t pin;

    if (led == FACADE_DEBUG_LED_GREEN) {
        RCC->AHB2ENR1 |= RCC_AHB2ENR1_GPIOBEN;
        gpio = GPIOB;
        pin = 4U;
    } else if (led == FACADE_DEBUG_LED_RED) {
        RCC->AHB2ENR1 |= RCC_AHB2ENR1_GPIOHEN;
        gpio = GPIOH;
        pin = 11U;
    } else {
        RCC->AHB2ENR1 |= RCC_AHB2ENR1_GPIODEN;
        gpio = GPIOD;
        pin = 7U;
    }
    (void)RCC->AHB2ENR1; /* read-back: guarantee the clock is on before use */

    /* Drive the pin as a general-purpose output: MODER[2*pin+1 : 2*pin] = 0b01. */
    gpio->MODER = (gpio->MODER & ~(0x3UL << (pin * 2U))) | (0x1UL << (pin * 2U));

    for (uint32_t i = 0; i < count * 2U; i++) {
        gpio->ODR ^= (1UL << pin);
        for (volatile uint32_t d = 0; d < delay_loops; d++) {
        }
    }
    gpio->ODR &= ~(1UL << pin);
}

void facade_uart_init(void)
{
    /*
     * Log over LPUART1 (PA3=TX, PA2=RX, AF8) - the serial port routed on this
     * board. Note the swapped TX/RX pins. The default STLINK UART4 (PC10/PC11)
     * is not wired on the dual-radio board, so logging there produces nothing.
     */
    quasar_gpio_config_t gpio_config_uart_tx = {
        .port = QUASAR_DEF_EXPANSION_UART_TX_PORT,
        .pin = QUASAR_DEF_EXPANSION_UART_TX_PIN,
        .mode = QUASAR_GPIO_MODE_ALTERNATE,
        .type = QUASAR_GPIO_TYPE_PP,
        .pull = QUASAR_GPIO_PULL_UP,
        .speed = QUASAR_GPIO_SPEED_LOW,
        .alternate = QUASAR_GPIO_ALTERNATE_AF8,
    };
    quasar_gpio_config_t gpio_config_uart_rx = {
        .port = QUASAR_DEF_EXPANSION_UART_RX_PORT,
        .pin = QUASAR_DEF_EXPANSION_UART_RX_PIN,
        .mode = QUASAR_GPIO_MODE_ALTERNATE,
        .type = QUASAR_GPIO_TYPE_OD,
        .pull = QUASAR_GPIO_PULL_UP,
        .speed = QUASAR_GPIO_SPEED_LOW,
        .alternate = QUASAR_GPIO_ALTERNATE_AF8,
    };

    quasar_uart_config_t uart_config = {
        .uart_selection = QUASAR_DEF_UART_SELECTION_EXPANSION,
        .baud_rate = QUASAR_UART_BAUD_RATE_115200,
        .parity = QUASAR_UART_PARITY_NONE,
        .stop = QUASAR_UART_STOP_BITS_1B,
        .irq_priority = QUASAR_IRQ_PRIORITY_0,
        .gpio_config_tx = gpio_config_uart_tx,
        .gpio_config_rx = gpio_config_uart_rx,
    };
    quasar_uart_init(uart_config);
}

void facade_time_delay(uint32_t ms)
{
    quasar_timer_delay_ms(ms);
}

void facade_radio_cs_drive(uint8_t radio_index, bool level)
{
    GPIO_TypeDef *port = (radio_index == 0) ? QUASAR_DEF_RADIO_1_CS_PORT : QUASAR_DEF_RADIO_2_CS_PORT;
    quasar_gpio_pin_t pin = (radio_index == 0) ? QUASAR_DEF_RADIO_1_CS_PIN : QUASAR_DEF_RADIO_2_CS_PIN;

    if (level) {
        quasar_gpio_set(port, pin);
    } else {
        quasar_gpio_clear(port, pin);
    }
}

void facade_log_io(char *string)
{
    /*
     * Blocking TX (mirrors puretone's facade_expansion_uart_write) instead of
     * the IRQ-driven path: removes any dependency on the LPUART1 TX interrupt
     * firing, which is the most likely reason init succeeds but no bytes appear.
     */
    quasar_bsp_status_t err = QUASAR_OK;

    quasar_uart_transmit_blocking(QUASAR_DEF_UART_SELECTION_EXPANSION, (uint8_t *)string, (uint16_t)strlen(string), 1000,
                                  &err);
}
