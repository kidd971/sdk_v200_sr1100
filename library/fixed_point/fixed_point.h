/** @file fixed_point.h
 *  @brief SR1000 Fixed point library for basic operation (+, -, *, /) on QX.Y
 *         number format. User have control over the integer value bits (2^X) and the
 *         precision bits (2^-Y).
 *
 *  @copyright Copyright (C) 2020 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef FIXED_POINT_H_
#define FIXED_POINT_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
#define FIXED_POINT_TOTAL_NUMBER_OF_BITS 32
#define FIXED_POINT_SIGN_BIT             1

/* TYPES **********************************************************************/
typedef int32_t q_num_t;

/** @brief Fixed point format structure.
 */
typedef struct fixed_point_format {
    uint8_t precision;    /*!< Number of precision bits, between 1 and 31 */
    uint8_t integer_bits; /*!< Number of bits for the integer */
} fixed_point_format_t;

/** @brief Fixed point mean parameters structure.
 */
typedef struct fixed_point_mean_format {
    uint16_t max_mean_size;         /*!< Maximum mean size */
    uint16_t mean_index;            /*!< Current mean value index */
    int64_t mean_accumulated_value; /*!< Current mean accumulated value */
    uint8_t mean_precision_bits;    /*!< Fixed point precision for division calculation in mean */
} fixed_point_mean_format_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the Fixed point library.
 *
 *  @param[in] precision_bits  Number of bits for the precision part of the Q format(2^-X).
 *  @param[in] integer_bits    Number of bits for the integer value part of the Q format(2^Y).
 *  @return Fixed point parameters.
 */
fixed_point_format_t fixed_point_initialization(uint8_t precision_bits, uint8_t integer_bits);

/** @brief Convert float number to Q representation.
 *
 *  This function convert a number represented using the
 *  floating point representation to the QX.Y format, where
 *  X is the integer part and Y is the precision. Theses are setup
 *  with fixed_point_initialization().
 *
 *  @param[in] fixed_point_format  Fixed point parameters.
 *  @param[in] real_number         Number to be converted, in float.
 *  @return Number in QX.Y format.
 */
q_num_t fixed_point_float_to_q_conv(fixed_point_format_t *fixed_point_format, float real_number);

/** @brief Convert Q representation number to float number.
 *
 *  @param[in] fixed_point_format  Fixed point parameters.
 *  @param[in] q_number            Number to be converted, in QX.Y format.
 *  @return Converted number, in floating point representation.
 */
float fixed_point_q_to_float_conv(fixed_point_format_t *fixed_point_format, q_num_t q_number);

/** @brief Convert Q representation number to 32 bits number.
 *
 *  @param[in] fixed_point_format  Fixed point parameters.
 *  @param[in] q_number            Number to be converted, in QX.Y format.
 *  @return Converted integer number.
 */
int32_t fixed_point_q_to_int_conv(fixed_point_format_t *fixed_point_format, q_num_t q_number);

/** @brief Convert 32 bit number to Q representation number.
 *
 *  @param[in] fixed_point_format  Fixed point parameters.
 *  @param[in] real_number         Real integer number.
 *  @return Converted QX.Y format number.
 */
q_num_t fixed_point_int_to_q_conv(fixed_point_format_t *fixed_point_format, int32_t real_number);

/** @brief Add two Q represented number together.
 *
 *  @note Result is 32 bits saturated.
 *
 *  @param[in] q_num1  First QX.Y format number to be added.
 *  @param[in] q_num2  Second QX.Y format number to be added.
 *  @return Sum, in QX.Y format.
 */
q_num_t fixed_point_add(q_num_t q_num1, q_num_t q_num2);

/** @brief Subtract two Q represented number.
 *
 *  @note Result is 32 bits saturated.
 *
 *  @param[in] q_num1  Minuend, in QX.Y format.
 *  @param[in] q_num2  Subtrahend, in QX.Y format.
 *  @return Difference, in QX.Y format.
 */
q_num_t fixed_point_sub(q_num_t q_num1, q_num_t q_num2);

/** @brief Multiply two Q represented number.
 *
 *  @note Result is 32 bits saturated.
 *
 *  @param[in] fixed_point_format  Fixed point parameters.
 *  @param[in] q_num1              First factor, in QX.Y format.
 *  @param[in] q_num2              Second factor, in QX.Y format.
 *  @return Product, in QX.Y format.
 */
q_num_t fixed_point_multiply(fixed_point_format_t *fixed_point_format, q_num_t q_num1, q_num_t q_num2);

/** @brief Divided two Q represented number.
 *
 *  @note Result is 32 bits saturated.
 *
 *  @param[in] fixed_point_format  Fixed point parameters.
 *  @param[in] q_num1              Dividend, in QX.Y format.
 *  @param[in] q_num2              Divisor, in QX.Y format.
 *  @return Quotient, in QX.Y format.
 */
q_num_t fixed_point_division(fixed_point_format_t *fixed_point_format, q_num_t q_num1, q_num_t q_num2);

/** @brief Get the precision bits value.
 *
 *  Get the precision bits value in QX.Y format
 *  of the current initialize fixed point library.
 *
 *  @param[in] fixed_point_format  Fixed point parameters.
 *  @return Precision, in QX.Y format.
 */
q_num_t fixed_point_get_precision_q(fixed_point_format_t *fixed_point_format);

/** @brief Initialize the Fixed point arithmetic mean .
 *
 *  @param[in] fixed_point_format  Fixed point parameters.
 *  @param[in] max_mean_size       Maximum mean size for the application.
 *  @return Fixed point mean parameters.
 */
fixed_point_mean_format_t fixed_point_mean_init(fixed_point_format_t *fixed_point_format, uint16_t mean_size);

/** @brief Add one element to the already initialized mean.
 *
 *  @param[in] fixed_point_mean_format  Fixed point mean parameters.
 *  @param[in] real_number              Number to be added, in QX.Y format.
 *  @return Accumulated mean value.
 */
int64_t fixed_point_mean_add(fixed_point_mean_format_t *fixed_point_mean_format, q_num_t real_number);

/** @brief Reset the mean for another calculation.
 *
 *  @note This should be call after every fixed_point_mean_calculate()
 *        to reset the accumulated mean value and the current index.
 *
 *  @param[in] fixed_point_mean_format  Fixed point mean parameters.
 */
void fixed_point_mean_reset(fixed_point_mean_format_t *fixed_point_mean_format);

/** @brief Calculate the mean based on previously added values.
 *
 *  @param[in] fixed_point_mean_format  Fixed point mean parameters.
 *  @param[in] size                     Size of the mean, 0 for the size define in fixed_point_mean_init().
 *  @return Mean result, in QX.Y format.
 */
q_num_t fixed_point_mean_calculate(fixed_point_mean_format_t *fixed_point_mean_format, uint16_t size);

#ifdef __cplusplus
}
#endif

#endif /* FIXED_POINT_H_ */
