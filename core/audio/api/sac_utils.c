/** @file  sac_utils.c
 *  @brief Utility functions for the SPARK Audio Core.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sac_utils.h"

/* PUBLIC FUNCTIONS ***********************************************************/
uint8_t sac_get_sample_size_from_format(sac_sample_format_t sample_format)
{
    if (sample_format.sample_encoding == SAC_SAMPLE_UNPACKED) {
        return SAC_WORD_SIZE_BITS;
    } else {
        return sample_format.bit_depth;
    }
}

uint16_t sac_get_nb_packets_in_x_ms(uint16_t ms, uint16_t audio_payload_size, uint8_t nb_channel,
                                    sac_sample_format_t sample_format, uint32_t sampling_rate)
{
    uint16_t sample_count = (audio_payload_size * SAC_BYTE_SIZE_BITS) / sac_get_sample_size_from_format(sample_format);

    return ((ms / 1000.0) / ((sample_count / nb_channel) / (float)sampling_rate));
}
