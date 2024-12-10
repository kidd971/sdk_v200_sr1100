/** @file fixed_point.c
 *  @brief SR1000 Fixed point library for basic operation (+, -, *, /) on QX.Y
 *         number format. User have control over the precision bits (Y) and the
 *         integer value bits(X).
 *
 *  @copyright Copyright (C) 2020 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "fixed_point.h"

/* CONSTANTS ******************************************************************/
#define FIXED_POINT_DEFAULT_PRECISION    16
#define FIXED_POINT_DEFAULT_INTEGER_BITS 15
#define FIXED_POINT_MAX_32_BITS_VALUE    ((int64_t)((1U << (32U - 1U)) - 1U))
#define FIXED_POINT_MIN_32_BITS_VALUE    (-1 - FIXED_POINT_MAX_32_BITS_VALUE)

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static bool no_bits_defined(uint8_t bits_value1, uint8_t bits_value2);
static bool not_enough_fixed_point_bits(uint8_t total_nb_bits);
static void apply_default_configuration(fixed_point_format_t *fixed_point_format);
static float saturate_value_float(int32_t integer_bits, float value);
static int64_t saturate_value32(int64_t value);
static int32_t clip64_to_32(int64_t x);

/* PUBLIC FUNCTIONS ***********************************************************/
fixed_point_format_t fixed_point_initialization(uint8_t precision_bits, uint8_t integer_bits)
{
    fixed_point_format_t fixed_point_format;

    if ((no_bits_defined(precision_bits, integer_bits)) ||
         not_enough_fixed_point_bits(precision_bits + integer_bits + FIXED_POINT_SIGN_BIT)) {
        apply_default_configuration(&fixed_point_format);
    } else if (integer_bits == 0) {
        fixed_point_format.precision    = precision_bits;
        fixed_point_format.integer_bits = FIXED_POINT_TOTAL_NUMBER_OF_BITS - precision_bits - FIXED_POINT_SIGN_BIT;
    } else if (precision_bits == 0) {
        fixed_point_format.precision    = FIXED_POINT_TOTAL_NUMBER_OF_BITS - integer_bits - FIXED_POINT_SIGN_BIT;
        fixed_point_format.integer_bits = integer_bits;
    } else {
        fixed_point_format.precision    = precision_bits;
        fixed_point_format.integer_bits = integer_bits;
    }

    return fixed_point_format;
}

q_num_t fixed_point_float_to_q_conv(fixed_point_format_t *fixed_point_format, float real_number)
{
    q_num_t conv_result = 0;

    real_number = saturate_value_float(fixed_point_format->integer_bits, real_number);

    conv_result = real_number * (1 << fixed_point_format->precision);

    return conv_result;
}

q_num_t fixed_point_int_to_q_conv(fixed_point_format_t *fixed_point_format, int32_t real_number)
{
    q_num_t conv_result = 0;

    conv_result = real_number << fixed_point_format->precision;

    return conv_result;
}

float fixed_point_q_to_float_conv(fixed_point_format_t *fixed_point_format, q_num_t q_number)
{
    float conv_result = 0;

    conv_result = (float)q_number / (1 << fixed_point_format->precision);

    return conv_result;
}

int32_t fixed_point_q_to_int_conv(fixed_point_format_t *fixed_point_format, q_num_t q_number)
{
    int32_t conv_result = 0;

    conv_result = q_number >> fixed_point_format->precision;

    return conv_result;
}

q_num_t fixed_point_add(q_num_t q_num1, q_num_t q_num2)
{
    q_num_t result;
    int64_t result_tmp;

    result_tmp = clip64_to_32((int64_t)q_num1 + (int64_t)q_num2);

    result = (q_num_t)result_tmp;

    return result;
}

q_num_t fixed_point_sub(q_num_t q_num1, q_num_t q_num2)
{
    return fixed_point_add(q_num1, -q_num2);
}

q_num_t fixed_point_multiply(fixed_point_format_t *fixed_point_format, q_num_t q_num1, q_num_t q_num2)
{
    q_num_t result;
    int64_t result_tmp;

    result_tmp = (int64_t)q_num1 * (int64_t)q_num2;

    result_tmp = result_tmp >> fixed_point_format->precision;

    result_tmp = saturate_value32(result_tmp);

    result = (q_num_t)result_tmp;

    return result;
}

q_num_t fixed_point_division(fixed_point_format_t *fixed_point_format, q_num_t q_num1, q_num_t q_num2)
{
    int32_t result;
    int64_t nominator_scale;
    int64_t result_tmp;

    nominator_scale = (int64_t)q_num1 << fixed_point_format->precision;

    result_tmp = nominator_scale / (int64_t)q_num2;

    result_tmp = saturate_value32(result_tmp);

    result = (int32_t)result_tmp;

    return result;
}

fixed_point_mean_format_t fixed_point_mean_init(fixed_point_format_t *fixed_point_format, uint16_t mean_size)
{
    fixed_point_mean_format_t fixed_point_mean_format;

    fixed_point_mean_format.max_mean_size          = mean_size;
    fixed_point_mean_format.mean_accumulated_value = 0;
    fixed_point_mean_format.mean_index             = 0;
    fixed_point_mean_format.mean_precision_bits    = fixed_point_format->precision;

    return fixed_point_mean_format;
}

int64_t fixed_point_mean_add(fixed_point_mean_format_t *fixed_point_mean_format, q_num_t real_number)
{
    fixed_point_mean_format->mean_index++;

    if (fixed_point_mean_format->mean_index <= fixed_point_mean_format->max_mean_size) {
        fixed_point_mean_format->mean_accumulated_value += real_number;
    }
    return fixed_point_mean_format->mean_accumulated_value;
}

void fixed_point_mean_reset(fixed_point_mean_format_t *fixed_point_mean_format)
{
    fixed_point_mean_format->mean_accumulated_value = 0;
    fixed_point_mean_format->mean_index             = 0;
}

q_num_t fixed_point_mean_calculate(fixed_point_mean_format_t *fixed_point_mean_format, uint16_t size)
{
    q_num_t result;
    int64_t nominator_scale;
    int64_t result_tmp;
    q_num_t mean_size_scale = 0;

    if (size != 0) {
        mean_size_scale = (q_num_t)size << fixed_point_mean_format->mean_precision_bits;
    } else {
        mean_size_scale = (q_num_t)fixed_point_mean_format->max_mean_size << fixed_point_mean_format->mean_precision_bits;
    }

    nominator_scale = fixed_point_mean_format->mean_accumulated_value << fixed_point_mean_format->mean_precision_bits;

    result_tmp = nominator_scale / (int64_t)mean_size_scale;

    result_tmp = saturate_value32(result_tmp);

    result = (q_num_t)result_tmp;

    return result;
}

q_num_t fixed_point_get_precision_q(fixed_point_format_t *fixed_point_format)
{
    return 1 << fixed_point_format->precision;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Check if one or more bits is defined in 2 given value .
 *
 *  @param[in] bits_value1  First value to check.
 *  @param[in] bits_value2  Second value to check.
 *  @retval True   Both value have no bits set
 *  @retval False  One or both value have one or more bits set
 */
static bool no_bits_defined(uint8_t bits_value1, uint8_t bits_value2)
{
    if (bits_value1 == 0 && bits_value2 == 0) {
        return 1;
    }
    return 0;
}

/** @brief Check if the user set parameter are valid.
 *
 *  This will check if the sum of the precision_bits, the
 *  integer_bits and the signed representation is greater
 *  than 32.
 *
 *  @param[in] total_nb_bits  The sum, in bits, of the 3 parameters.
 *  @retval True   Sum is higher than 32.
 *  @retval False  Sum is lower of equal to 32.
 */
static bool not_enough_fixed_point_bits(uint8_t total_nb_bits)
{
    if (total_nb_bits > FIXED_POINT_TOTAL_NUMBER_OF_BITS) {
        return 1;
    }
    return 0;
}

/** @brief Apply the default configuration.
 *
 *  @note This will apply the following parameters :
 *        - Precision bits : 16 -> 0.000015259.
 *        - Integer value  : 15 -> 32768.
 *
 *  @param[in] fixed_point_format  Fixed point format initialization.
 */
static void apply_default_configuration(fixed_point_format_t *fixed_point_format)
{
    fixed_point_format->integer_bits = FIXED_POINT_DEFAULT_INTEGER_BITS;
    fixed_point_format->precision    = FIXED_POINT_DEFAULT_PRECISION;
}

/** @brief Saturate given float value.
 *
 *  @note Value are saturate using 32-bit max signed value.
 *
 *  @param[in] integer_bits  Number of bits for max value.
 *  @param[in] value         Value to clip, in float.
 *  @return Saturated value.
 */
static float saturate_value_float(int32_t integer_bits, float value)
{
    const int32_t max = (int32_t)(((1U << integer_bits) - 1U));
    const int32_t min = -1 - max;

    if (value > max) {
        return max;

    } else if (value < min) {
        return min;
    }

    return value;
}

/** @brief Saturate given value.
 *
 *  @note Value are saturate using 32-bit max signed value.
 *
 *  @param[in] value  64-bit value to clip.
 *  @return Saturated value.
 */
static int64_t saturate_value32(int64_t value)
{
    if (value > FIXED_POINT_MAX_32_BITS_VALUE) {
        return FIXED_POINT_MAX_32_BITS_VALUE;
    } else if (value < FIXED_POINT_MIN_32_BITS_VALUE) {
        return FIXED_POINT_MIN_32_BITS_VALUE;
    }
    return value;
}

/** @brief Clip 64-bit value to 32-bit max value (Optimized).
 *
 *  @param[in] value  64-bit value to clip.
 *  @return Clipped value.
 */
static int32_t clip64_to_32(int64_t x)
{
    if ((int32_t)(x >> 32) != ((int32_t)x >> 31)) {
        return (0x7FFFFFFF ^ ((int32_t)(x >> 63)));
    } else {
        return (int32_t)x;
    }
}
