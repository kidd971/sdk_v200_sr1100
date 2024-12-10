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
 *  @file  fir_decimate.c
 *  @brief Q15 FIR Decimator.
 *
 *  Target Processor: Cortex-M
 *
 *  Based on arm_fir_decimate_q15.c CMSIS DSP Library V1.9.0.
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
filtering_functions_error_t fir_decimate_init(fir_decimate_instance_t *instance, uint16_t num_taps,
                                              uint8_t divide_ratio, const int32_t *p_coeffs, int32_t *p_state,
                                              uint32_t block_size)
{
    /* The size of the input block must be a multiple of the decimation factor */
    if ((block_size % divide_ratio) != 0) {
        return FILTERING_FUNCTION_CFG_ERR;
    }

    /* Assign filter taps */
    instance->num_taps = num_taps;

    /* Assign coefficient pointer */
    instance->p_coeffs = p_coeffs;

    /* Clear the state buffer. The size is always (block_size + num_taps - 1) */
    memset(p_state, 0, (num_taps + (block_size - 1U)) * sizeof(int32_t));

    /* Assign state pointer */
    instance->p_state = p_state;

    /* Assign Decimation Factor */
    instance->divide_ratio = divide_ratio;

    return FILTERING_FUNCTION_ERR_NONE;
}

void fir_decimate(const fir_decimate_instance_t *instance, const uint8_t *src, uint8_t *dst, uint32_t block_size,
                  uint8_t channel, uint8_t channel_count)
{
    int32_t *p_state = instance->p_state;         /* State pointer */
    const int32_t *p_coeffs = instance->p_coeffs; /* Coefficient pointer */
    int32_t *p_state_cur;                         /* Points to the current sample of the state */
    int32_t *px0;                                 /* Temporary pointer for state buffer */
    const int32_t *pb;                            /* Temporary pointer for coefficient buffer */
    int32_t x0, c0;                               /* Temporary variables to hold state and coefficient values */
    int64_t acc0;                                 /* Accumulator */
    uint32_t num_taps = instance->num_taps;       /* Number of filter coefficients in the filter */
    uint32_t i, tap_count, block_count, out_block_size = block_size / instance->divide_ratio; /* Loop counters */

    const uint8_t sample_size_in_byte = instance->input_sample_format.sample_size_byte;
    const uint8_t input_bitshift = instance->input_sample_format.sample_bitshift;
    const uint32_t input_mask = instance->input_sample_format.sample_mask;
    const uint8_t sample_size_out_byte = instance->output_sample_format.sample_size_byte;
    const uint8_t output_bitshift = instance->output_sample_format.sample_bitshift;
    const uint8_t divide_ratio = instance->divide_ratio;

    src += channel * sample_size_in_byte;
    dst += channel * sample_size_out_byte;

    /* instance->p_state buffer contains previous frame (num_taps - 1) samples */
    /* p_state_cur points to the location where the new input data should be written */
    p_state_cur = instance->p_state + (num_taps - 1U);

    /* Total number of output samples to be computed */
    block_count = out_block_size;

    while (block_count > 0U) {
        /* Copy decimation factor number of new input samples into the state buffer */
        i = divide_ratio;

        do {
            *p_state_cur++ = (*((int32_t *)src) & input_mask) << input_bitshift;
            src += (channel_count * sample_size_in_byte);
        } while (--i);

        /* Set accumulator to zero */
        acc0 = 0;

        /* Initialize state pointer for all the samples */
        px0 = p_state;

        /* Initialize coeff pointer */
        pb = p_coeffs;

        /* Initialize tap_count with number of taps */
        tap_count = num_taps;

        while (tap_count > 0U) {
            /* Read coefficients */
            c0 = *pb++;

            /* Fetch state variables for acc0, acc1 */
            x0 = *px0++;

            /* Perform the multiply-accumulate */
            acc0 += (int64_t)x0 * c0;

            /* Decrement loop counter */
            tap_count--;
        }

        /* Advance the state pointer by the decimation factor
         * to process the next group of decimation factor number samples
         */
        p_state += divide_ratio;

        /* The result is in the accumulator, store in the destination buffer. */
        acc0 = acc0 >> (31 + output_bitshift);
        for (uint8_t j = 0; j < instance->output_sample_format.sample_size_byte; j++) {
            dst[j] = ((uint8_t *)&acc0)[j];
        }
        dst += (channel_count * sample_size_out_byte);

        /* Decrement loop counter */
        block_count--;
    }

    /* Points to the start of the state buffer */
    p_state_cur = instance->p_state;

    /* Initialize tap_count with number of taps */
    tap_count = (num_taps - 1U);

    /* Copy data */
    while (tap_count > 0U) {
        *p_state_cur++ = *p_state++;

        /* Decrement loop counter */
        tap_count--;
    }
}
