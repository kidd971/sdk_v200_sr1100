/** @license   SPDX-License-Identifier: Apache-2.0
 *
 *             Licensed under the Apache License, Version 2.0 (the License); you may
 *             not use this file except in compliance with the License.
 *             You may obtain a copy of the License at
 *
 *             www.apache.org/licenses/LICENSE-2.0
 *
 *             Unless required by applicable law or agreed to in writing, software
 *             distributed under the License is distributed on an AS IS BASIS, WITHOUT
 *             WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *             See the License for the specific language governing permissions and
 *             limitations under the License.
 *
 *  @file  fir_interpolate.c
 *  @brief Q15 FIR interpolation.
 *
 *  Target Processor: Cortex-M
 *
 *  Based on arm_fir_interpolate_q15.c CMSIS DSP Library V1.9.0.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *             Copyright (C) 2010-2021 ARM Limited or its affiliates. All rights reserved.
 *
 *  @author    Derivative: SPARK FW Team.
 *             Original work: ARM Limited.
 */

/* INCLUDES *******************************************************************/
#include <string.h>
#include "filtering_functions.h"

/* PUBLIC FUNCTIONS ***********************************************************/
filtering_functions_error_t fir_interpolate_init(fir_interpolate_instance_t *instance, uint8_t multiply_ratio,
                                                 uint16_t num_taps, const int32_t *p_coeffs, int32_t *p_state,
                                                 uint32_t block_size)
{
    /* The filter length must be a multiple of the interpolation factor */
    if ((num_taps % multiply_ratio) != 0) {
        return FILTERING_FUNCTION_CFG_ERR;
    }

    /* Assign coefficient pointer */
    instance->p_coeffs = p_coeffs;

    /* Assign Interpolation factor */
    instance->multiply_ratio = multiply_ratio;

    /* Assign polyPhaseLength */
    instance->phase_length = num_taps / multiply_ratio;

    /* Clear state buffer and size of buffer is always phase_length + block_size - 1 */
    memset(p_state, 0, (block_size + ((uint32_t)instance->phase_length - 1U)) * sizeof(int32_t));

    /* Assign state pointer */
    instance->p_state = p_state;

    return FILTERING_FUNCTION_ERR_NONE;
}

void fir_interpolate(const fir_interpolate_instance_t *instance, const uint8_t *src, uint8_t *dst, uint32_t block_size,
                     uint8_t channel, uint8_t channel_count)
{
    int32_t *p_state = instance->p_state;         /* State pointer */
    const int32_t *p_coeffs = instance->p_coeffs; /* Coefficient pointer */
    int32_t *p_state_cur;                         /* Points to the current sample of the state */
    int32_t *ptr1;                                /* Temporary pointer for state buffer */
    const int32_t *ptr2;                          /* Temporary pointer for coefficient buffer */
    int64_t sum0;                                 /* Accumulators */
    uint32_t i, j, k, block_count, tap_count;     /* Loop counters */
    uint32_t phase_len = instance->phase_length;  /* Length of each polyphase filter component */

    const uint8_t sample_size_in_byte = instance->input_sample_format.sample_size_byte;
    const uint8_t input_bitshift = instance->input_sample_format.sample_bitshift;
    const uint32_t input_mask = instance->input_sample_format.sample_mask;
    const uint8_t sample_size_out_byte = instance->output_sample_format.sample_size_byte;
    const uint8_t output_bitshift = instance->output_sample_format.sample_bitshift;
    const uint8_t multiply_ratio = instance->multiply_ratio;

    int64_t acc0, acc1, acc2, acc3;
    int32_t x0, x1, x2, x3;
    int32_t c0, c1, c2, c3;

    src += channel * sample_size_in_byte;
    dst += channel * sample_size_out_byte;

    /* instance->p_state buffer contains previous frame (phase_len - 1) samples */
    /* p_state_cur points to the location where the new input data should be written */
    p_state_cur = instance->p_state + (phase_len - 1U);

    /* Loop unrolling: Compute 4 outputs at a time */
    block_count = block_size >> 2U;

    while (block_count > 0U) {

        /* Copy new input sample into the state buffer */
        *p_state_cur++ = (*((int32_t *)src) & input_mask) << input_bitshift;
        src += (channel_count * sample_size_in_byte);
        *p_state_cur++ = (*((int32_t *)src) & input_mask) << input_bitshift;
        src += (channel_count * sample_size_in_byte);
        *p_state_cur++ = (*((int32_t *)src) & input_mask) << input_bitshift;
        src += (channel_count * sample_size_in_byte);
        *p_state_cur++ = (*((int32_t *)src) & input_mask) << input_bitshift;
        src += (channel_count * sample_size_in_byte);

        /* Address modifier index of coefficient buffer */
        j = 1U;

        /* Loop over the Interpolation factor. */
        i = multiply_ratio;

        while (i > 0U) {

            /* Set accumulator to zero */
            acc0 = 0;
            acc1 = 0;
            acc2 = 0;
            acc3 = 0;

            /* Initialize state pointer */
            ptr1 = p_state;

            /* Initialize coefficient pointer */
            ptr2 = p_coeffs + (multiply_ratio - j);

            /* Loop over the polyPhase length. Unroll by a factor of 4.
             * Repeat until we've computed numTaps-(4*multiply_ratio) coefficients.
             */
            tap_count = phase_len >> 2U;

            x0 = *(ptr1++);
            x1 = *(ptr1++);
            x2 = *(ptr1++);

            while (tap_count > 0U) {
                /* Read the input sample */
                x3 = *(ptr1++);

                /* Read the coefficient */
                c0 = *(ptr2);

                /* Perform the multiply-accumulate */
                acc0 += (int64_t)x0 * c0;
                acc1 += (int64_t)x1 * c0;
                acc2 += (int64_t)x2 * c0;
                acc3 += (int64_t)x3 * c0;

                /* Read the coefficient */
                c1 = *(ptr2 + multiply_ratio);

                /* Read the input sample */
                x0 = *(ptr1++);

                /* Perform the multiply-accumulate */
                acc0 += (int64_t)x1 * c1;
                acc1 += (int64_t)x2 * c1;
                acc2 += (int64_t)x3 * c1;
                acc3 += (int64_t)x0 * c1;

                /* Read the coefficient */
                c2 = *(ptr2 + multiply_ratio * 2);

                /* Read the input sample */
                x1 = *(ptr1++);

                /* Perform the multiply-accumulate */
                acc0 += (int64_t)x2 * c2;
                acc1 += (int64_t)x3 * c2;
                acc2 += (int64_t)x0 * c2;
                acc3 += (int64_t)x1 * c2;

                /* Read the coefficient */
                c3 = *(ptr2 + multiply_ratio * 3);

                /* Read the input sample */
                x2 = *(ptr1++);

                /* Perform the multiply-accumulate */
                acc0 += (int64_t)x3 * c3;
                acc1 += (int64_t)x0 * c3;
                acc2 += (int64_t)x1 * c3;
                acc3 += (int64_t)x2 * c3;

                /* Upsampling is done by stuffing multiply_ratio-1 zeros between each sample.
                 * So instead of multiplying zeros with coefficients,
                 * Increment the coefficient pointer by interpolation factor times.
                 */
                ptr2 += 4 * multiply_ratio;

                /* Decrement loop counter */
                tap_count--;
            }

            /* If the polyPhase length is not a multiple of 4, compute the remaining filter taps */
            tap_count = phase_len % 0x4U;

            while (tap_count > 0U) {
                /* Read the input sample */
                x3 = *(ptr1++);

                /* Read the coefficient */
                c0 = *(ptr2);

                /* Perform the multiply-accumulate */
                acc0 += (int64_t)x0 * c0;
                acc1 += (int64_t)x1 * c0;
                acc2 += (int64_t)x2 * c0;
                acc3 += (int64_t)x3 * c0;

                /* Increment the coefficient pointer by interpolation factor times */
                ptr2 += multiply_ratio;

                /* Update states for next sample processing */
                x0 = x1;
                x1 = x2;
                x2 = x3;

                /* Decrement loop counter */
                tap_count--;
            }

            /* The result is in the accumulator, store in the destination buffer. */
            acc0 = acc0 >> (31 + output_bitshift);
            acc1 = acc1 >> (31 + output_bitshift);
            acc2 = acc2 >> (31 + output_bitshift);
            acc3 = acc3 >> (31 + output_bitshift);
            for (k = 0; k < sample_size_out_byte; k++) {
                dst[k] = ((uint8_t *)&acc0)[k];
                (dst + ((multiply_ratio * channel_count) * sample_size_out_byte))[k] = ((uint8_t *)&acc1)[k];
                (dst + (2 * ((multiply_ratio * channel_count) * sample_size_out_byte)))[k] = ((uint8_t *)&acc2)[k];
                (dst + (3 * ((multiply_ratio * channel_count) * sample_size_out_byte)))[k] = ((uint8_t *)&acc3)[k];
            }
            dst += channel_count * sample_size_out_byte;

            /* Increment the address modifier index of coefficient buffer */
            j++;

            /* Decrement loop counter */
            i--;
        }

        /* Advance the state pointer by 1
         * to process the next group of interpolation factor number samples
         */
        p_state = p_state + 4;

        dst += (multiply_ratio * (channel_count * sample_size_out_byte)) * 3;

        /* Decrement loop counter */
        block_count--;
    }

    /* Loop unrolling: Compute remaining outputs */
    block_count = block_size % 0x4U;

    while (block_count > 0U) {

        /* Copy new input sample into the state buffer */
        *p_state_cur++ = (*((int32_t *)src) & input_mask) << input_bitshift;
        src += (channel_count * sample_size_in_byte);

        /* Address modifier index of coefficient buffer */
        j = 1U;

        /* Loop over the interpolation factor */
        i = multiply_ratio;
        while (i > 0U) {
            /* Set accumulator to zero */
            sum0 = 0;

            /* Initialize state pointer */
            ptr1 = p_state;

            /* Initialize coefficient pointer */
            ptr2 = p_coeffs + (multiply_ratio - j);

            /* Loop over the polyPhase length.
             * Repeat until we've computed numTaps-(4*multiply_ratio) coefficients.
             */

            /* Loop unrolling: Compute 4 outputs at a time */
            tap_count = phase_len >> 2U;

            while (tap_count > 0U) {
                /* Perform the multiply-accumulate */
                sum0 += (int64_t)*ptr1++ * *ptr2;

                /* Upsampling is done by stuffing multiply_ratio-1 zeros between each sample.
                 * So instead of multiplying zeros with coefficients,
                 * Increment the coefficient pointer by interpolation factor times.
                 */
                ptr2 += multiply_ratio;

                sum0 += (int64_t)*ptr1++ * *ptr2;
                ptr2 += multiply_ratio;

                sum0 += (int64_t)*ptr1++ * *ptr2;
                ptr2 += multiply_ratio;

                sum0 += (int64_t)*ptr1++ * *ptr2;
                ptr2 += multiply_ratio;

                /* Decrement loop counter */
                tap_count--;
            }

            /* Loop unrolling: Compute remaining outputs */
            tap_count = phase_len % 0x4U;

            while (tap_count > 0U) {
                /* Perform the multiply-accumulate */
                sum0 += (int64_t)*ptr1++ * *ptr2;

                /* Upsampling is done by stuffing multiply_ratio-1 zeros between each sample.
                 * So instead of multiplying zeros with coefficients,
                 * Increment the coefficient pointer by interpolation factor times.
                 */
                ptr2 += multiply_ratio;

                /* Decrement loop counter */
                tap_count--;
            }

            /* The result is in the accumulator, store in the destination buffer. */
            sum0 = sum0 >> (31 + output_bitshift);
            for (k = 0; k < sample_size_out_byte; k++) {
                dst[k] = ((uint8_t *)&sum0)[k];
            }
            dst += channel_count * sample_size_out_byte;

            /* Increment the address modifier index of coefficient buffer */
            j++;

            /* Decrement the loop counter */
            i--;
        }

        /* Advance the state pointer by 1
         * to process the next group of interpolation factor number samples
         */
        p_state = p_state + 1;

        /* Decrement the loop counter */
        block_count--;
    }

    /* Processing is complete.
     * Now copy the last phase_len - 1 samples to the satrt of the state buffer.
     * This prepares the state buffer for the next function call.
     */

    /* Points to the start of the state buffer */
    p_state_cur = instance->p_state;

    /* Loop unrolling: Compute 4 outputs at a time */
    tap_count = (phase_len - 1U) >> 2U;

    /* Copy data */
    while (tap_count > 0U) {
        *p_state_cur++ = *p_state++;
        *p_state_cur++ = *p_state++;
        *p_state_cur++ = *p_state++;
        *p_state_cur++ = *p_state++;

        /* Decrement loop counter */
        tap_count--;
    }

    /* Loop unrolling: Compute remaining outputs */
    tap_count = (phase_len - 1U) % 0x04U;

    /* Copy data */
    while (tap_count > 0U) {
        *p_state_cur++ = *p_state++;

        /* Decrement loop counter */
        tap_count--;
    }
}
