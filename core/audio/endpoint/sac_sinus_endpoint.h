/** @file  sac_sinus_endpoint.h
 *  @brief SPARK Audio Core endpoint used to produce a pre-recorded sine wave.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_SINUS_ENDPOINT_H_
#define SAC_SINUS_ENDPOINT_H_

/* INCLUDES *******************************************************************/
#include "sac_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Sinus frequency selection.
 */
typedef enum sine_freq {
    /*! 1 period of a 1 kHz tone sampled at 48 kHz, 16-bit samples, mono. */
    SINE_FREQ_1K,
    /*! 2 periods of a 2 kHz tone sampled at 48 kHz, 16-bit samples, mono. */
    SINE_FREQ_2K,
    /*! 3 periods of a 3 kHz tone sampled at 48 kHz, 16-bit samples, mono. */
    SINE_FREQ_3K,
} sine_freq_t;

/** @brief Sinus Endpoint Instance.
 */
typedef struct sinus_instance {
    /*! Frequency of the produced sine wave */
    sine_freq_t sine_freq;
} sinus_instance_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Sinus Endpoint's Produce action.
 *
 *  @param[in]  instance  Endpoint instance.
 *  @param[out] samples   Produced samples.
 *  @param[in]  size      Size of samples to produce in bytes.
 *  @return Number of bytes produced.
 */
uint16_t ep_sinus_produce(void *instance, uint8_t *samples, uint16_t size);

/** @brief Sinus Endpoint's Consume action.
 *
 *  @param[in]  instance  Endpoint instance.
 *  @param[out] samples   Consumed samples.
 *  @param[in]  size      Size of samples to consume in bytes.
 *  @return Number of bytes consumed.
 */
uint16_t ep_sinus_consume(void *instance, uint8_t *samples, uint16_t size);

/** @brief Start the Sinus endpoint.
 *
 *  @param[in] instance  Endpoint instance.
 */
void ep_sinus_start(void *instance);

/** @brief Stop the Sinus endpoint.
 *
 *  @param[in] instance  Endpoint instance.
 */
void ep_sinus_stop(void *instance);

#ifdef __cplusplus
}
#endif

#endif /* SAC_SINUS_ENDPOINT_H_ */
