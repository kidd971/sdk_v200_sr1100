/** @file  sac_utils.h
 *  @brief Utility functions for the SPARK Audio Core.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_UTILS_H_
#define SAC_UTILS_H_

/* INCLUDES *******************************************************************/
#include "sac_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* MACROS *********************************************************************/
#define SAC_CHECK_STATUS(cond, status_ptr, status_code, ret) \
    do {                                                     \
        if (cond) {                                          \
            *(status_ptr) = (status_code);                   \
            ret;                                             \
        }                                                    \
    } while (0)

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Get the number of bits used to store a sample of a given format.
 *
 *  @param[in] sample_format  Sample format.
 *  @return The number of bits required to store a single sample.
 */
uint8_t sac_get_sample_size_from_format(sac_sample_format_t sample_format);

/** @brief Get the number of audio packets in a given amount of milliseconds.
 *
 *  @param[in] ms                  The number of milliseconds.
 *  @param[in] audio_payload_size  The size of the audio payload in bytes.
 *  @param[in] nb_channel          The number of audio channels.
 *  @param[in] sample_format       The format of the samples in the audio payload.
 *  @param[in] sampling_rate       The sampling rate of the samples in the audio payload in Hz.
 *  @return Number of audio packets, rounded down.
 */
uint16_t sac_get_nb_packets_in_x_ms(uint16_t ms, uint16_t audio_payload_size, uint8_t nb_channel,
                                    sac_sample_format_t sample_format, uint32_t sampling_rate);

#ifdef __cplusplus
}
#endif

#endif /* SAC_UTILS_H_ */

