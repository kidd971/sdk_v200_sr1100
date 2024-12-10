/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * This file is part of the TinyUSB stack.
 */

#ifndef _BOARD_API_H_
#define _BOARD_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "tusb.h"

// Define the default baudrate
#ifndef CFG_BOARD_UART_BAUDRATE
#define CFG_BOARD_UART_BAUDRATE 115200   ///< Default baud rate
#endif

//--------------------------------------------------------------------+
// Board Porting API
// For simplicity, only one LED and one Button are used
//--------------------------------------------------------------------+

// Initialize on-board peripherals : led, button, uart and USB
void board_init(void);

// Init board after tinyusb is initialized
void board_init_after_tusb(void) TU_ATTR_WEAK;

// Turn LED on or off
void board_led_write(bool state);

// Control led pattern using phase duration in ms.
// For each phase, LED is toggle then repeated, board_led_task() is required to be called
//void board_led_pattern(uint32_t const phase_ms[], uint8_t count);

// Get the current state of button
// a '1' means active (pressed), a '0' means inactive.
uint32_t board_button_read(void);

// Get board unique ID for USB serial number. Return number of bytes. Note max_len is typically 16
TU_ATTR_WEAK size_t board_get_unique_id(uint8_t id[], size_t max_len);

// Get characters from UART. Return number of read bytes
int board_uart_read(uint8_t *buf, int len);

// Send characters to UART. Return number of sent bytes
int board_uart_write(void const *buf, int len);

#if CFG_TUSB_OS == OPT_OS_NONE
// Get current milliseconds, must be implemented when no RTOS is used
uint32_t board_millis(void);

#elif CFG_TUSB_OS == OPT_OS_FREERTOS
static inline uint32_t board_millis(void) {
  return ( ( ((uint64_t) xTaskGetTickCount()) * 1000) / configTICK_RATE_HZ );
}

#elif CFG_TUSB_OS == OPT_OS_MYNEWT
static inline uint32_t board_millis(void) {
  return os_time_ticks_to_ms32( os_time_get() );
}

#elif CFG_TUSB_OS == OPT_OS_PICO
#include "pico/time.h"
static inline uint32_t board_millis(void) {
  return to_ms_since_boot(get_absolute_time());
}

#elif CFG_TUSB_OS == OPT_OS_RTTHREAD
static inline uint32_t board_millis(void) {
  return (((uint64_t)rt_tick_get()) * 1000 / RT_TICK_PER_SECOND);
}

#else
  #error "board_millis() is not implemented for this OS"
#endif

//--------------------------------------------------------------------+
// Helper functions
//--------------------------------------------------------------------+

// stdio getchar() is blocking, this is non-blocking version
int board_getchar(void);

#ifdef __cplusplus
}
#endif

#endif
