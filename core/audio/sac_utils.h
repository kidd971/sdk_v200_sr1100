/** @file  sac_utils.h
 *  @brief Utility functions for the SPARK Audio Core.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_UTILS_H_
#define SAC_UTILS_H_

/* INCLUDES *******************************************************************/
#include "sac_api.h"
#include "sac_packing.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/*! SAC system latency in number of packets. 1 for DMA transfer, 2 for wireless queue. */
#define SAC_SYSTEM_LATENCY_PKT 3

/* MACROS *********************************************************************/
/* Check if a condition is met. If so, apply the status code to the status pointer and return from the function. */
#define SAC_CHECK_STATUS(cond, status_ptr, status_code, ret) \
    do {                                                     \
        if (cond) {                                          \
            *(status_ptr) = (status_code);                   \
            ret;                                             \
        }                                                    \
    } while (0)

/*! Calculate the audio payload size based on the audio sample settings. */
#define SAC_CALCULATE_PAYLOAD_SIZE(samp_nb_per_ch, ch_nb, bits) (((samp_nb_per_ch) * (ch_nb) * (bits) + 7) / 8)

/*! Calculate the audio payload size based on the number of samples per channel per packet, the sample rate and the
 *  audio peripheral latency (ADC, DAC, USB, etc).
 */
#define SAC_CALCULATE_LATENCY_QUEUE_SIZE(target_ms, periph_latency_ms, nb_samp_per_ch, samp_rate_hz) \
    ((((samp_rate_hz) * ((target_ms) - (periph_latency_ms))) / ((nb_samp_per_ch) * 1000)) - SAC_SYSTEM_LATENCY_PKT)

/*! Calculate the number of samples per channel contained in a packet. */
#define SAC_CALCULATE_SAMPLE_COUNT(nb_byte, nb_ch, bytes_per_samp) ((nb_byte) / ((nb_ch) * (bytes_per_samp)))

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

/** @brief Get the number of milliseconds in a given number of audio packets.
 *
 *  @param[in] nb_packet           The number of packets.
 *  @param[in] audio_payload_size  The size of the audio payload in bytes.
 *  @param[in] nb_channel          The number of audio channels.
 *  @param[in] sample_format       The format of the samples in the audio payload.
 *  @param[in] sampling_rate       The sampling rate of the samples in the audio payload in Hz.
 *  @return Number of milliseconds, rounded down.
 */
uint16_t sac_get_ms_in_x_packets(uint16_t nb_packet, uint16_t audio_payload_size, uint8_t nb_channel,
                                 sac_sample_format_t sample_format, uint32_t sampling_rate);

/** @brief Configure a packing instance based on producer and consumer sample formats.
 *
 *  @note Determines the appropriate packing mode for converting between the producer and consumer sample formats.
 *        Handles all combinations of pack (unpacked to packed), unpack (packed to unpacked), and scale (packed to
 *        packed) operations.
 *
 *  @param[out] instance         Packing instance to configure.
 *  @param[in]  producer_format  Sample format of the audio producer.
 *  @param[in]  consumer_format  Sample format of the audio consumer.
 *  @param[out] status           SAC status code. Set to SAC_ERR_BIT_DEPTH on unsupported combination.
 *  @retval true   Packing is required; instance has been configured.
 *  @retval false  No packing required (formats are compatible) or unsupported (check status).
 */
bool sac_configure_packing(sac_packing_instance_t *instance, sac_sample_format_t producer_format,
                           sac_sample_format_t consumer_format, sac_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* SAC_UTILS_H_ */
