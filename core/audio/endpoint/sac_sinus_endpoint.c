/** @file  sac_sinus_endpoint.c
 *  @brief SPARK Audio Core endpoint used to produce a pre-recorded sine wave.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sac_sinus_endpoint.h"

/* CONSTANTS ******************************************************************/
static const int16_t sin_1khz_48ks_16bits_mono[48] = {
    0, 4276, 8480, 12539, 16383, 19947,
    23169, 25995, 28377, 30272, 31650, 32486,
    32767, 32486, 31650, 30272, 28377, 25995,
    23169, 19947, 16383, 12539, 8480, 4276,
    0, -4276, -8480, -12539, -16383, -19947,
    -23169, -25995, -28377, -30272, -31650, -32486,
    -32767, -32486, -31650, -30272, -28377, -25995,
    -23169, -19947, -16383, -12539, -8480, -4276
};

static const int16_t sin_2khz_48ks_16bits_mono[48] = {
    0, 8480, 16383, 23169, 28377, 31650,
    32767, 31650, 28377, 23169, 16383, 8480,
    0, -8480, -16383, -23169, -28377, -31650,
    -32767, -31650, -28377, -23169, -16383, -8480,
    0, 8480, 16383, 23169, 28377, 31650,
    32767, 31650, 28377, 23169, 16383, 8480,
    0, -8480, -16383, -23169, -28377, -31650,
    -32767, -31650, -28377, -23169, -16383, -8480
};

static const int16_t sin_3khz_48ks_16bits_mono[48] = {
    0, 12539, 23169, 30272, 32767, 30272, 23169, 12539,
    0, -12539, -23169, -30272, -32767, -30272, -23169, -12539,
    0, 12539, 23169, 30272, 32767, 30272, 23169, 12539,
    0, -12539, -23169, -30272, -32767, -30272, -23169, -12539,
    0, 12539, 23169, 30272, 32767, 30272, 23169, 12539,
    0, -12539, -23169, -30272, -32767, -30272, -23169, -12539
};

/* PUBLIC FUNCTIONS ***********************************************************/
uint16_t ep_sinus_produce(void *instance, uint8_t *samples, uint16_t size)
{
    sinus_instance_t *inst = (sinus_instance_t *)instance;

    switch (inst->sine_freq) {
    case SINE_FREQ_1K:
        size = sizeof(sin_1khz_48ks_16bits_mono);
        memcpy(samples, sin_1khz_48ks_16bits_mono, size);
        break;
    case SINE_FREQ_2K:
        size = sizeof(sin_2khz_48ks_16bits_mono);
        memcpy(samples, sin_2khz_48ks_16bits_mono, size);
        break;
    case SINE_FREQ_3K:
        size = sizeof(sin_3khz_48ks_16bits_mono);
        memcpy(samples, sin_3khz_48ks_16bits_mono, size);
        break;
    default:
        break;
    }

    return size;
}

uint16_t ep_sinus_consume(void *instance, uint8_t *samples, uint16_t size)
{
    (void)instance;
    (void)samples;
    (void)size;

    return 0;
}

void ep_sinus_start(void *instance)
{
    (void)instance;
}

void ep_sinus_stop(void *instance)
{
    (void)instance;
}
