/** @file  sac_sinus_endpoint_96k.c
 *  @brief SPARK Audio Core endpoint that produces a 1 kHz sine wave at 96 kHz / 24-bit stereo.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sac_sinus_endpoint_96k.h"

/* CONSTANTS ******************************************************************/
/* One period of 1 kHz sampled at 96 kHz: 96 samples, 24-bit amplitude (±8388607). */
#define SINE_TABLE_LEN 96

/* Values: round(sin(2π × n / 96) × 8388607) for n = 0..95 */
static const int32_t sine_1khz_96ks_24bit[SINE_TABLE_LEN] = {
          0,   548962,  1094832,  1636007,  2170852,  2695926,  3210028,  3706818,
    4194303,  4660038,  5105108,  5530428,  5931642,  6305617,  6653610,  6973068,
    7262604,  7525135,  7747217,  7928939,  8100512,  8225760,  8315131,  8369014,
    8388607,  8369014,  8315131,  8225760,  8100512,  7928939,  7747217,  7525135,
    7262604,  6973068,  6653610,  6305617,  5931642,  5530428,  5105108,  4660038,
    4194303,  3706818,  3210028,  2695926,  2170852,  1636007,  1094832,   548962,
          0,  -548962, -1094832, -1636007, -2170852, -2695926, -3210028, -3706818,
   -4194303, -4660038, -5105108, -5530428, -5931642, -6305617, -6653610, -6973068,
   -7262604, -7525135, -7747217, -7928939, -8100512, -8225760, -8315131, -8369014,
   -8388607, -8369014, -8315131, -8225760, -8100512, -7928939, -7747217, -7525135,
   -7262604, -6973068, -6653610, -6305617, -5931642, -5530428, -5105108, -4660038,
   -4194303, -3706818, -3210028, -2695926, -2170852, -1636007, -1094832,  -548962,
};

/* PRIVATE GLOBALS ************************************************************/
static uint32_t s_phase = 0;

/* PUBLIC FUNCTIONS ***********************************************************/
uint16_t ep_sinus_96k_produce(void *instance, uint8_t *samples, uint16_t size)
{
    (void)instance;
    int32_t *out = (int32_t *)samples;
    uint32_t frame_count = size / (sizeof(int32_t) * 2); /* stereo: 2 × int32_t per frame */

    for (uint32_t i = 0; i < frame_count; i++) {
        int32_t val = sine_1khz_96ks_24bit[s_phase];
        out[i * 2]     = val; /* Left  */
        out[i * 2 + 1] = val; /* Right */
        s_phase = (s_phase + 1 >= SINE_TABLE_LEN) ? 0 : s_phase + 1;
    }

    return size;
}

uint16_t ep_sinus_96k_consume(void *instance, uint8_t *samples, uint16_t size)
{
    (void)instance;
    (void)samples;
    (void)size;

    return 0;
}

void ep_sinus_96k_start(void *instance)
{
    (void)instance;

    s_phase = 0;
}

void ep_sinus_96k_stop(void *instance)
{
    (void)instance;
}
