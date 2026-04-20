/** @file  profiler_backend.c
 *  @brief Implement Profiler facade prototype functions.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include <string.h>
#include "profiler_facade.h"
#include "quasar.h"
#include "quasar_clock.h"
#include "quasar_profiler.h"

/* CONSTANTS ******************************************************************/
#define UART_TIMEOUT                        1000
#define IRQ_PRIORITY_TMER_PACKET_GENERATION (QUASAR_DEF_PRIO_PENDSV_IRQ + 1)
#define TIMER_SELECTION_PACKET_GENERATION   QUASAR_TIMER_SELECTION_TIMER6
#define SEC_TO_US                           ((double)1e6)

/* PRIVATE GLOBALS ************************************************************/
static uint32_t timestamp_begin;

/* PUBLIC FUNCTIONS ***********************************************************/
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
    quasar_bsp_status_t quasar_err = QUASAR_OK;

    quasar_uart_transmit_blocking(QUASAR_DEF_UART_SELECTION_DEBUG, (uint8_t *)string, strlen(string), UART_TIMEOUT,
                                  &quasar_err);
    ASSERT_QUASAR_BSP_STATUS(quasar_err);
}

void facade_log_error_string(char *string)
{
    /* Turn OFF LEDs to show nothing is happening. */
    facade_led_all_off();
    /* Set RGB to RED to indicate an error happened. */
    quasar_rgb_configure_color(QUASAR_RGB_COLOR_RED);
    quasar_rgb_set();

    /* Set to highest priority. */
    HAL_NVIC_SetPriority(OTG_HS_IRQn, 0, 0);
    facade_log_write(string);
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

void facade_profiler_start(uint32_t *timestamp_start_handle)
{
    if (timestamp_start_handle == NULL) {
        timestamp_begin = quasar_profiler_get_cycle_count();
        return;
    }

    *timestamp_start_handle = quasar_profiler_get_cycle_count();
}

uint32_t facade_profiler_stop(uint32_t *timestamp_start_handle)
{
    uint32_t timestamp_end = quasar_profiler_get_cycle_count();

    if (timestamp_start_handle == NULL) {
        return timestamp_end - timestamp_begin;
    }

    return timestamp_end - *timestamp_start_handle;
}

double facade_profiler_get_elapsed_us(uint32_t cycle_count)
{
    double clk_freq = (double)quasar_clock_get_system_clock_freq();

    return ((double)cycle_count / clk_freq) * SEC_TO_US;
}
