/** @file  quasar_debug.c
 *  @brief Debug module for Quasar BSP, providing debug IOs control and UART communication.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_debug.h"
#include "quasar_def.h"
#include "quasar_uart.h"

/* CONSTANTS ******************************************************************/
#define LOG_UART_TIMEOUT_MS 1000

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void debug_io_init(quasar_debug_io_peripheral_t quasar_debug_io_peripheral);
static void debug_io_deinit(quasar_debug_io_peripheral_t quasar_debug_io_peripheral);
static void debug_uart_init(void);
static void debug_uart_deinit(void);
static quasar_gpio_config_t debug_io_get_default_config(quasar_debug_io_peripheral_t quasar_debug_io_peripheral);
static quasar_uart_config_t debug_uart_get_default_config(void);

/* PUBLIC FUNCTIONS ***********************************************************/
void quasar_debug_init(void)
{
    debug_io_init(QUASAR_DEBUG_IO_1);
    debug_io_init(QUASAR_DEBUG_IO_2);
    debug_io_init(QUASAR_DEBUG_IO_3);
    debug_io_init(QUASAR_DEBUG_IO_4);
    debug_io_init(QUASAR_DEBUG_IO_5);
    debug_uart_init();
}

void quasar_debug_deinit(void)
{
    debug_io_deinit(QUASAR_DEBUG_IO_1);
    debug_io_deinit(QUASAR_DEBUG_IO_2);
    debug_io_deinit(QUASAR_DEBUG_IO_3);
    debug_io_deinit(QUASAR_DEBUG_IO_4);
    debug_io_deinit(QUASAR_DEBUG_IO_5);
    debug_uart_deinit();
}

void quasar_debug_io_set(quasar_debug_io_peripheral_t quasar_debug_io_peripheral)
{
    quasar_gpio_config_t debug_config = debug_io_get_default_config(quasar_debug_io_peripheral);

    quasar_gpio_set(debug_config.port, debug_config.pin);
}

void quasar_debug_io_clear(quasar_debug_io_peripheral_t quasar_debug_io_peripheral)
{
    quasar_gpio_config_t debug_config = debug_io_get_default_config(quasar_debug_io_peripheral);

    quasar_gpio_clear(debug_config.port, debug_config.pin);
}

void quasar_debug_io_toggle(quasar_debug_io_peripheral_t quasar_debug_io_peripheral)
{
    quasar_gpio_config_t debug_config = debug_io_get_default_config(quasar_debug_io_peripheral);

    quasar_gpio_toggle(debug_config.port, debug_config.pin);
}

void quasar_debug_uart_transmit_blocking(uint8_t *data, uint16_t size)
{
    quasar_uart_transmit_blocking(QUASAR_DEF_UART_SELECTION_DEBUG, data, size, LOG_UART_TIMEOUT_MS);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize the selected debug io peripheral.
 *
 *  @param[in] quasar_debug_io_peripheral  Selected debug io peripheral.
 */
static void debug_io_init(quasar_debug_io_peripheral_t quasar_debug_io_peripheral)
{
    quasar_gpio_config_t debug_config = debug_io_get_default_config(quasar_debug_io_peripheral);

    quasar_gpio_init(debug_config);
}

/** @brief Deinitialize the selected debug io peripheral.
 *
 *  @param[in] quasar_debug_io_peripheral  Selected debug io peripheral.
 */
static void debug_io_deinit(quasar_debug_io_peripheral_t quasar_debug_io_peripheral)
{
    quasar_gpio_config_t debug_config = debug_io_get_default_config(quasar_debug_io_peripheral);

    quasar_gpio_deinit(debug_config.port, debug_config.pin);
}

/** @brief Initialize the debug UART (ST-Link) in blocking mode.
 */
static void debug_uart_init(void)
{
    quasar_uart_config_t config = debug_uart_get_default_config();

    quasar_uart_init(config);
}

/** @brief Deinitialize the debug UART (ST-Link).
 */
static void debug_uart_deinit(void)
{
    quasar_uart_config_t uart_config = debug_uart_get_default_config();

    quasar_uart_deinit(uart_config);
}

/** @brief Get configuration of the debug IO.
 *
 *  @param[in] quasar_debug_io_peripheral  Selected debug io peripheral.
 *  @return The Quasar GPIO configuration.
 */
static quasar_gpio_config_t debug_io_get_default_config(quasar_debug_io_peripheral_t quasar_debug_io_peripheral)
{
    quasar_gpio_config_t config;

    switch (quasar_debug_io_peripheral) {
    case QUASAR_DEBUG_IO_1:
        config.port = QUASAR_DEF_DEBUG_IO_0_PORT;
        config.pin = QUASAR_DEF_DEBUG_IO_0_PIN;
        config.mode = QUASAR_GPIO_MODE_OUTPUT;
        config.type = QUASAR_GPIO_TYPE_PP;
        config.pull = QUASAR_GPIO_PULL_NONE;
        config.speed = QUASAR_GPIO_SPEED_LOW;
        config.alternate = QUASAR_GPIO_ALTERNATE_NONE;
        break;

    case QUASAR_DEBUG_IO_2:
        config.port = QUASAR_DEF_DEBUG_IO_1_PORT;
        config.pin = QUASAR_DEF_DEBUG_IO_1_PIN;
        config.mode = QUASAR_GPIO_MODE_OUTPUT;
        config.type = QUASAR_GPIO_TYPE_PP;
        config.pull = QUASAR_GPIO_PULL_NONE;
        config.speed = QUASAR_GPIO_SPEED_LOW;
        config.alternate = QUASAR_GPIO_ALTERNATE_NONE;
        break;

    case QUASAR_DEBUG_IO_3:
        config.port = QUASAR_DEF_DEBUG_IO_2_PORT;
        config.pin = QUASAR_DEF_DEBUG_IO_2_PIN;
        config.mode = QUASAR_GPIO_MODE_OUTPUT;
        config.type = QUASAR_GPIO_TYPE_PP;
        config.pull = QUASAR_GPIO_PULL_NONE;
        config.speed = QUASAR_GPIO_SPEED_LOW;
        config.alternate = QUASAR_GPIO_ALTERNATE_NONE;
        break;

    case QUASAR_DEBUG_IO_4:
        config.port = QUASAR_DEF_DEBUG_IO_3_PORT;
        config.pin = QUASAR_DEF_DEBUG_IO_3_PIN;
        config.mode = QUASAR_GPIO_MODE_OUTPUT;
        config.type = QUASAR_GPIO_TYPE_PP;
        config.pull = QUASAR_GPIO_PULL_NONE;
        config.speed = QUASAR_GPIO_SPEED_LOW;
        config.alternate = QUASAR_GPIO_ALTERNATE_NONE;
        break;

    case QUASAR_DEBUG_IO_5:
        config.port = QUASAR_DEF_DEBUG_IO_4_PORT;
        config.pin = QUASAR_DEF_DEBUG_IO_4_PIN;
        config.mode = QUASAR_GPIO_MODE_OUTPUT;
        config.type = QUASAR_GPIO_TYPE_PP;
        config.pull = QUASAR_GPIO_PULL_NONE;
        config.speed = QUASAR_GPIO_SPEED_LOW;
        config.alternate = QUASAR_GPIO_ALTERNATE_NONE;
        break;

    default:
        break;
    }

    return config;
}

/** @brief Get configuration of the UART used for debugging via the ST-Link debugger.
 *
 *  @return The Quasar UART configuration.
 */
static quasar_uart_config_t debug_uart_get_default_config(void)
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
        .irq_priority = QUASAR_IRQ_PRIORITY_NONE,
        .gpio_config_tx = gpio_config_uart_tx,
        .gpio_config_rx = gpio_config_uart_rx,
    };

    return uart_config;
}
