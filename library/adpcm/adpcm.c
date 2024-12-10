/** @file  adpcm.c
 *  @brief Definitions for ADPCM compression related function.
 *
 *  This implementation is based on the algorithm described in
 *  "Recommended Practices for Enhancing Digital Audio Compatibility in
 *  Multimedia Systems" by the IMA Digital Audio Focus and Technical
 *  Working Groups, revision 3.0.
 *  Reference: http://www.cs.columbia.edu/~hgs/audio/dvi/IMA_ADPCM.pdf
 *
 *  This implementation uses a state type to hold the encoder and
 *  decoder state information, thus allowing multiple instances of
 *  each to coexist.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "adpcm.h"

/* CONSTANTS ******************************************************************/
#define STEP_SIZE_TABLE_LENGTH 89

const uint16_t step_size_table[STEP_SIZE_TABLE_LENGTH] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

const int8_t index_table[] = {
    -1, -1, -1, -1, 2, 4, 6, 8,   /* Table of index changes */
    -1, -1, -1, -1, 2, 4, 6, 8
};

/* PUBLIC FUNCTIONS ***********************************************************/
void adpcm_init_state(adpcm_state_t *state)
{
    state->state.index = 0;
    state->state.predicted_sample = 0;
}

uint8_t adpcm_encode(int32_t original_sample, adpcm_state_t *state)
{
    int32_t predicted_sample = (int32_t)state->state.predicted_sample; /* output of ADPCM predictor */
    int32_t difference = 0;
    uint16_t temp_step_size = 0;
    uint16_t step_size = 0; /* quantizer step_size */
    uint8_t new_sample = 0;
    int16_t index = (int16_t)state->state.index; /* index into step_size_table */
    uint8_t mask = 0;

    step_size = step_size_table[index];

    /* find difference from predicted sample: */
    difference = original_sample - predicted_sample;

    if (difference >= 0) {  /* set sign bit and find absolute value of difference */
        new_sample = 0;     /* set sign bit(new_sample[3]) to 0 */
    } else {
        new_sample = 8;            /* set sign bit(new_sample[3]) to one */
        difference = -difference;  /* absolute value of negative difference */
    }

    mask = 4;                  /* used to set bits in new_sample */
    temp_step_size = step_size;  /* store quantizer step_size for later use */

    for (uint8_t i = 0; i < 3; i++) {       /* quantize difference down to four bits */
        if (difference >= temp_step_size) {  /* new_sample[2:0] = 4 * (difference/step_size) */
            new_sample |= mask;             /* perform division ... */
            difference -= temp_step_size;    /* ... through repeated subtraction */
        }
        temp_step_size >>= 1;  /* adjust comparator for next iteration */
        mask >>= 1;           /* adjust bit-set mask for next iteration */
    }
    /* 4-bit new_sample can be stored at this point */
    /* compute new sample estimate predicted_sample */
    difference = 0;             /* calculate difference = (new_sample + ½) * step_size/4 */
    if (new_sample & 4) {       /* perform multiplication through repetitive addition */
        difference += step_size;
    }
    if (new_sample & 2) {
        difference += step_size >> 1;
    }
    if (new_sample & 1) {
        difference += step_size >> 2;
    }
    difference += step_size >> 3;
    /* (new_sample + ½) * step_size/4 = new_sample * step_size/4 + step_size/8 */
    if (new_sample & 8) {  /* account for sign bit */
        difference = -difference;
    }
    /* adjust predicted sample based on calculated difference: */
    predicted_sample += difference;
    if (predicted_sample > INT16_MAX) {  /* check for overflow */
        predicted_sample = INT16_MAX;
    } else if (predicted_sample < INT16_MIN) {
        predicted_sample = INT16_MIN;
    }
    /* compute new step_size */
    /* adjust index into step_size lookup table using new_sample */
    index += index_table[new_sample];
    if (index < 0) {  /* check for index underflow */
        index = 0;
    } else if (index > (STEP_SIZE_TABLE_LENGTH - 1)) {  /* check for index overflow */
        index = (STEP_SIZE_TABLE_LENGTH - 1);
    }
    step_size = step_size_table[index];  /* find new quantizer step_size */

    state->state.index = (uint8_t)index;
    state->state.predicted_sample = (int16_t)predicted_sample;

    return new_sample;
}

int16_t adpcm_decode(uint8_t original_sample, adpcm_state_t *state)
{
    int32_t difference = 0;
    int32_t new_sample = (int32_t)state->state.predicted_sample;  /* Reuse the state variable, predicted sample == decoding result */
    int16_t index      = (int16_t)state->state.index;
    uint16_t step_size = step_size_table[index];  /* quantizer step_size */

    /* compute predicted sample estimate new_sample */
    /* calculate difference = (original_sample + ½) * step_size/4: */
    if (original_sample & 4) {  /* perform multiplication through repetitive addition */
        difference += step_size;
    }
    if (original_sample & 2) {
        difference += step_size >> 1;
    }
    if (original_sample & 1) {
        difference += step_size >> 2;
    }
    /* (original_sample + ½) * step_size/4 = original_sample * step_size/4 + step_size/8: */
    difference += step_size >> 3;
    if (original_sample & 8) {  /* account for sign bit */
        difference = -difference;
    }
    /* adjust predicted sample based on calculated difference: */
    new_sample += difference;
    if (new_sample > INT16_MAX) {  /* check for overflow */
        new_sample = INT16_MAX;
    } else if (new_sample < INT16_MIN) {
        new_sample = INT16_MIN;
    }
    /* 16-bit new_sample can be stored at this point */
    /* compute new step_size */
    /* adjust index into step_size lookup table using original_sample: */
    index += index_table[original_sample];
    if (index < 0) {  /* check for index underflow */
        index = 0;
    } else if (index > (STEP_SIZE_TABLE_LENGTH - 1)) {  /* check for index overflow */
        index = (STEP_SIZE_TABLE_LENGTH - 1);
    }

    state->state.index = (uint8_t)index;
    state->state.predicted_sample = (int16_t)new_sample;

    return (int16_t)new_sample;
}
