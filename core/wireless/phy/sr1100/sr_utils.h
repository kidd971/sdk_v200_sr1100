/** @file sr_utils.h
 *  @brief SR series driver utility macros and functions.
 *
 *  @copyright Copyright (C) 2018 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is confidential and proprietary.
 *  @author    SPARK FW Team.
 */
#ifndef SR_UTILS_H_
#define SR_UTILS_H_

/* INCLUDES *******************************************************************/
#include <stdint.h>
#include "swc_hal_facade.h"

/* MACROS *********************************************************************/
/*! Extract the nth (0 = 1st, 1 = 2nd,..) byte from an int */
#define EXTRACT_BYTE(x, n) (((x) >> (8 * (n))) & 0x00ff)

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Determine if value is positive or negative.
 *
 *  @param[in] value  Value to evaluate.
 *  @retval  1  Value is positive.
 *  @retval  0  Value is 0.
 *  @retval -1  Value is negative.
 */
static inline int8_t get_sign(int8_t value)
{
    int8_t sign;

    if (value > 0) {
        sign = 1;
    } else if (value < 0) {
        sign = -1;
    } else {
        sign = 0;
    }
    return sign;
}

/** @brief Serialize an uint16_t byte array into an uint8_t byte array
 *
 *  @param[in]  in_data   Input array in uint16_t
 *  @param[out] out_data  Output array in uint8_t
 */
static inline void serialize_uint16_to_uint8_array(uint16_t in_data, uint8_t *out_data)
{
    out_data[0] = EXTRACT_BYTE(in_data, 1);
    out_data[1] = EXTRACT_BYTE(in_data, 0);
}

/** @brief Serialize an uint32_t byte array into an uint8_t byte array
 *
 *  @param[in]  in_data   Input array in uint32_t
 *  @param[out] out_data  Output array in uint8_t
 */
static inline void serialize_uint32_to_uint8_array(uint32_t in_data, uint8_t *out_data)
{
    out_data[0] = EXTRACT_BYTE(in_data, 3);
    out_data[1] = EXTRACT_BYTE(in_data, 2);
    out_data[2] = EXTRACT_BYTE(in_data, 1);
    out_data[3] = EXTRACT_BYTE(in_data, 0);
}

/** @brief Add value at the first position of given array. Last index is discarded
 *
 *  @param[in] new_val  New value.
 *  @param[in] array    Array where the value is added.
 *  @param[in] size     Array's size.
 */
static inline void emplace_front_int32(int32_t new_val, int32_t *array, uint8_t size)
{
    /* Move data one position higher */
    for (uint8_t i = (size - 1); i > 0; --i) {
        array[i] = array[i - 1];
    }
    array[0] = new_val;
}

/** @brief Implements a delay mechanism based on a free-running timer.
 *
 *  @param[in] ms  The desired delay in milliseconds.
 */
static inline void sr_utils_wait_delay(uint32_t ms)
{
    uint32_t start_tick = swc_hal_get_tick_free_running_timer();
    uint32_t tick_frequency = swc_hal_get_free_running_timer_frequency_hz();
    uint32_t ticks_needed = (tick_frequency * ms) / 1000;

    /* Wait until the desired number of ticks has elapsed */
    while ((swc_hal_get_tick_free_running_timer() - start_tick) < ticks_needed) {}
}

/** @brief Get the current system time in milliseconds since system booted.
 *
 *  @return The current system time in milliseconds.
 */
static inline uint32_t sr_util_get_system_time_ms(void)
{
    return (swc_hal_get_tick_free_running_timer() * 1000) / swc_hal_get_free_running_timer_frequency_hz();
}

#endif /* SR_UTILS_H_ */
