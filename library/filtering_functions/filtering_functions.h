/** @file  filtering_functions.h
 *  @brief SPARK Audio Core ARM filtering functions.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef FILTERING_FUNCTIONS_H_
#define FILTERING_FUNCTIONS_H_

/* INCLUDES *******************************************************************/
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
#define FIR_MASK_16BITS 0x0000FFFF
#define FIR_MASK_24BITS 0x00FFFFFF

#define FIR_BITSHIFT_16BITS      16
#define FIR_BITSHIFT_24BITS      8

/* TYPES **********************************************************************/
/** @brief Error status returned by init functions in the library.
 */
typedef enum filtering_functions_error {
    FILTERING_FUNCTION_ERR_NONE = 0,
    FILTERING_FUNCTION_CFG_ERR,
} filtering_functions_error_t;

/** @brief FIR filter sample bit depth. */
typedef enum fir_bit_depth {
    /*! 16-bit FIR samples. */
    FIR_16BITS = 16,
    /*! 24-bit FIR samples. */
    FIR_24BITS = 24,
    /*! 32-bit FIR samples. */
    FIR_32BITS = 32,
} fir_bit_depth_t;

/** @brief FIR filter sample word size.
 */
typedef enum fir_sample_size_bytes {
    /*! 2-byte FIR samples. */
    FIR_2_BYTES = 2,
    /*! 3-byte FIR samples. */
    FIR_3_BYTES = 3,
    /*! 4-byte FIR samples. */
    FIR_4_BYTES = 4,
} fir_sample_size_bytes_t;

/** @brief FIR sample format.
 */
typedef struct fir_sample_format {
    /*! Bit resolution of an audio sample. */
    fir_bit_depth_t bit_depth;
    /*! Word size of an audio sample. */
    fir_sample_size_bytes_t sample_size_byte;
    /*! Mask to apply on input stream to separate samples. */
    uint32_t sample_mask;
    /*! Bit shift to apply on input samples to align data. */
    uint8_t sample_bitshift;
} fir_sample_format_t;

/** @brief Instance structure for the 32-bit FIR decimator.
 */
typedef struct fir_decimate_instance {
    /*! Decimation factor. */
    uint8_t divide_ratio;
    /*! Number of coefficients in the filter. */
    uint16_t num_taps;
    /*! Points to the coefficient array. The array is of length num_taps.*/
    const int32_t *p_coeffs;
    /*! Points to the state variable array. The array is of length num_taps+block_size-1. */
    int32_t *p_state;
    /*! Sample size of an input sample in bytes. */
    fir_sample_format_t input_sample_format;
    /*! Sample size of an output sample in bytes. */
    fir_sample_format_t output_sample_format;
} fir_decimate_instance_t;

/** @brief Instance structure for the 32-bit FIR interpolator.
 */
typedef struct fir_interpolate_instance {
    /*! Upsample factor. */
    uint8_t multiply_ratio;
    /*! Length of each polyphase filter component. */
    uint16_t phase_length;
    /*! Points to the coefficient array. The array is of length multiply_ratio*phase_length. */
    const int32_t *p_coeffs;
    /*! Points to the state variable array. The array is of length block_size+phase_length-1. */
    int32_t *p_state;
    /*! Sample size of an input sample in bytes. */
    fir_sample_format_t input_sample_format;
    /*! Sample size of an output sample in bytes. */
    fir_sample_format_t output_sample_format;
} fir_interpolate_instance_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief  Initialization function for the 16-bit FIR interpolator.
 *
 *  @param[in] instance        Points to an instance of the 16-bit FIR interpolator structure.
 *  @param[in] multiply_ratio  Upsample factor.
 *  @param[in] num_taps        Number of filter coefficients in the filter.
 *  @param[in] p_coeffs        Points to the filter coefficient buffer.
 *  @param[in] p_state         Points to the state buffer.
 *  @param[in] block_size      Number of input samples to process per call.
 *  @return FIR initialization error code.
 */
filtering_functions_error_t fir_interpolate_init(fir_interpolate_instance_t *instance, uint8_t multiply_ratio,
                                                 uint16_t num_taps, const int32_t *p_coeffs, int32_t *p_state,
                                                 uint32_t block_size);

/** @brief  Initialization function for the Q15 FIR decimator.
 *
 *  @param[in,out] instance      points to an instance of the Q15 FIR decimator structure.
 *  @param[in]     num_taps      number of coefficients in the filter.
 *  @param[in]     divide_ratio  decimation factor.
 *  @param[in]     p_coeffs      points to the filter coefficients.
 *  @param[in]     p_state       points to the state buffer.
 *  @param[in]     block_size    number of input samples to process per call.
 *  @return FIR initialization error code.
 */
filtering_functions_error_t fir_decimate_init(fir_decimate_instance_t *instance, uint16_t num_taps,
                                              uint8_t divide_ratio, const int32_t *p_coeffs, int32_t *p_state,
                                              uint32_t block_size);

/** @brief Processing function for the 16-bit FIR decimator.
 *
 *  @param[in]  instance       Points to an instance of the 16-bit FIR decimator structure.
 *  @param[in]  src            Points to the block of input data.
 *  @param[out] dst            Points to the block of output data.
 *  @param[in]  block_size     Number of input samples to process per call.
 *  @param[in]  channel        Channel index to use.
 *  @param[in]  channel_count  Number of channels in the input data.
 *
 * @par           Scaling and Overflow Behavior
 *                  The function is implemented using a 64-bit internal accumulator.
 *                  Both coefficients and state variables are represented in 1.15 format and multiplications yield a
 *                  2.30 result. The 2.30 intermediate results are accumulated in a 64-bit accumulator in 34.30 format.
 *                  There is no risk of internal overflow with this approach and the full precision of intermediate
 *                  multiplications is preserved. After all additions have been performed, the accumulator is truncated
 *                  to 34.15 format by discarding low 15 bits. Lastly, the accumulator is saturated to yield a result
 *                  in 1.15 format.
 *
 *@remark
 *            Refer to \ref arm_fir_decimate_fast_q15() for a faster but less precise implementation of this function.
 */
void fir_decimate(const fir_decimate_instance_t *instance, const uint8_t *src, uint8_t *dst, uint32_t block_size,
                  uint8_t channel, uint8_t channel_count);

/** @brief Processing function for the 16-bit FIR interpolator.
 *
 *  @param[in]  instance       Points to an instance of the 16-bit FIR decimator structure.
 *  @param[in]  src            Points to the block of input data.
 *  @param[out] dst            Points to the block of output data.
 *  @param[in]  block_size     Number of input samples to process per call.
 *  @param[in]  channel        Channel index to use.
 *  @param[in]  channel_count  Number of channels in the input data.
 *
 *  @par           Scaling and Overflow Behavior
 *                   The function is implemented using a 64-bit internal accumulator.
 *                   Both coefficients and state variables are represented in 1.15 format and multiplications yield a
 *                   2.30 result. The 2.30 intermediate results are accumulated in a 64-bit accumulator in 34.30 format.
 *                   There is no risk of internal overflow with this approach and the full precision of intermediate
 *                   multiplications is preserved. After all additions have been performed, the accumulator is truncated
 *                   to 34.15 format by discarding low 15 bits. Lastly, the accumulator is saturated to yield a result
 *                   in 1.15 format.
 */
void fir_interpolate(const fir_interpolate_instance_t *instance, const uint8_t *src, uint8_t *dst, uint32_t block_size,
                     uint8_t channel, uint8_t channel_count);

#ifdef __cplusplus
}
#endif

#endif /* FILTERING_FUNCTIONS_H_ */
