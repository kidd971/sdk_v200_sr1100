/** @file  quasar_backend.c
 *  @brief Implement BSP validator facade prototype functions.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "bsp_validator_facade.h"
#include "quasar.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void facade_bsp_init(void)
{
    quasar_config_t quasar_cfg = {
        .clk_freq = QUASAR_CLK_160MHZ,
        .debug_enabled = false,
        .radio1_enabled = true,
        .adc_enabled = false,
        .quasar_vdd_selection = QUASAR_VDD_SELECTION_3V3,
    };

    if (SWC_RADIO_COUNT == 2) {
        quasar_cfg.radio2_enabled = true;
    } else {
        quasar_cfg.radio2_enabled = false;
    }
    quasar_init(quasar_cfg);
}

void facade_uart_init(void)
{
    quasar_gpio_config_t gpio_config_uart_tx = {
        .port = QUASAR_DEF_STLINK_UART_TX_PORT,
        .pin = QUASAR_DEF_STLINK_UART_TX_PIN,
        .mode = QUASAR_GPIO_MODE_ALTERNATE,
        .type = QUASAR_GPIO_TYPE_PP,
        .pull = QUASAR_GPIO_PULL_UP,
        .speed = QUASAR_GPIO_SPEED_LOW,
        .alternate = QUASAR_GPIO_ALTERNATE_AF8,
    };
    quasar_gpio_config_t gpio_config_uart_rx = {
        .port = QUASAR_DEF_STLINK_UART_RX_PORT,
        .pin = QUASAR_DEF_STLINK_UART_RX_PIN,
        .mode = QUASAR_GPIO_MODE_ALTERNATE,
        .type = QUASAR_GPIO_TYPE_OD,
        .pull = QUASAR_GPIO_PULL_UP,
        .speed = QUASAR_GPIO_SPEED_LOW,
        .alternate = QUASAR_GPIO_ALTERNATE_AF8,
    };

    quasar_uart_config_t uart_config = {
        .uart_selection = QUASAR_UART_SELECTION_UART4,
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

void facade_log_io(char *string)
{
    quasar_uart_transmit_string_irq(QUASAR_UART_SELECTION_UART4, string, strlen(string));
}

void facade_context_switch_trigger(void)
{
    quasar_radio_callback_context_switch();
}

void facade_set_context_switch_handler(void (*callback)(void))
{
    quasar_it_set_pendsv_callback(callback);
}
