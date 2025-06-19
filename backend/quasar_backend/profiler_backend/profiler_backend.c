/** @file  profiler_backend.c
 *  @brief Implement Profiler facade prototype functions.
 *
 *  @copyright Copyright (C) 2025 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include <string.h>
#include "profiler_facade.h"
#include "quasar.h"

/* CONSTANTS ******************************************************************/
#define IRQ_PRIORITY_TMER_PACKET_GENERATION (QUASAR_DEF_PRIO_PENDSV_IRQ + 1)
#define TIMER_SELECTION_PACKET_GENERATION   QUASAR_TIMER_SELECTION_TIMER6

/* PUBLIC FUNCTIONS ***********************************************************/
void facade_board_init(void)
{
    quasar_config_t quasar_cfg = {
        .clk_freq = QUASAR_CLK_160MHZ,
        .debug_enabled = true,
        .radio1_enabled = true,
        .radio2_enabled = false,
        .adc_enabled = false,
        .quasar_vdd_selection = QUASAR_VDD_SELECTION_3V3,
    };
    quasar_init(quasar_cfg);

    quasar_radio_1_set_debug_en();
}

void facade_delay(uint32_t ms_delay)
{
    quasar_timer_delay_ms(ms_delay);
}

void facade_log_init(void)
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
        .uart_selection = QUASAR_DEF_UART_SELECTION_DEBUG,
        .baud_rate = QUASAR_UART_BAUD_RATE_115200,
        .parity = QUASAR_UART_PARITY_NONE,
        .stop = QUASAR_UART_STOP_BITS_1B,
        .irq_priority = QUASAR_IRQ_PRIORITY_0,
        .gpio_config_tx = gpio_config_uart_tx,
        .gpio_config_rx = gpio_config_uart_rx,
    };
    quasar_uart_init(uart_config);
}

void facade_log_write(char *string)
{
    quasar_uart_transmit_blocking(QUASAR_DEF_UART_SELECTION_DEBUG, (uint8_t *)string, strlen(string), 1000);
}

void facade_context_switch_trigger(void)
{
    quasar_radio_callback_context_switch();
}

void facade_set_context_switch_handler(void (*callback)(void))
{
    quasar_it_set_pendsv_callback(callback);
}

void facade_packet_generation_timer_init(uint32_t period)
{
    quasar_timer_config_t timer_config = {
        .timer_selection = TIMER_SELECTION_PACKET_GENERATION,
        .time_base = QUASAR_TIMER_TIME_BASE_MICROSECOND,
        .time_period = period,
        .irq_priority = IRQ_PRIORITY_TMER_PACKET_GENERATION,
    };
    quasar_timer_init(&timer_config);
}

void facade_packet_generation_set_timer_callback(void (*irq_callback)(void))
{
    quasar_it_set_timer6_callback(irq_callback);
}

void facade_packet_generation_timer_start(void)
{
    quasar_timer_start(TIMER_SELECTION_PACKET_GENERATION);
}

void facade_packet_generation_timer_stop(void)
{
    quasar_timer_stop(TIMER_SELECTION_PACKET_GENERATION);
}

void facade_profiler_init(void)
{
    quasar_profiler_init();
}

void facade_profiler_start(void)
{
    quasar_profiler_start();
}

uint64_t facade_profiler_stop(void)
{
    return quasar_profiler_stop();
}

void facade_tx_conn_status(void)
{
    quasar_led_toggle(QUASAR_LED_USER_1);
}

void facade_rx_conn_status(void)
{
    quasar_led_toggle(QUASAR_LED_USER_2);
}
