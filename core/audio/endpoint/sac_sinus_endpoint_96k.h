/** @file  sac_sinus_endpoint_96k.h
 *  @brief SPARK Audio Core endpoint that produces a 1 kHz sine wave at 96 kHz / 24-bit stereo.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_SINUS_ENDPOINT_96K_H_
#define SAC_SINUS_ENDPOINT_96K_H_

/* INCLUDES *******************************************************************/
#include "sac_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Sinus 96k Endpoint's Produce action.
 *
 *  Fills the buffer with stereo 24-bit (unpacked, int32_t) samples of a 1 kHz sine wave
 *  sampled at 96 kHz. Left and right channels carry the same signal.
 *
 *  @param[in]  instance  Endpoint instance (not used).
 *  @param[out] samples   Produced samples.
 *  @param[in]  size      Size of samples to produce in bytes.
 *  @return Number of bytes produced.
 */
uint16_t ep_sinus_96k_produce(void *instance, uint8_t *samples, uint16_t size);

/** @brief Sinus 96k Endpoint's Consume action (no-op).
 *
 *  @param[in]  instance  Endpoint instance (not used).
 *  @param[in]  samples   Samples to consume (not used).
 *  @param[in]  size      Size in bytes (not used).
 *  @return 0.
 */
uint16_t ep_sinus_96k_consume(void *instance, uint8_t *samples, uint16_t size);

/** @brief Start the Sinus 96k endpoint. Resets the phase index to 0.
 *
 *  @param[in] instance  Endpoint instance (not used).
 */
void ep_sinus_96k_start(void *instance);

/** @brief Stop the Sinus 96k endpoint (no-op).
 *
 *  @param[in] instance  Endpoint instance (not used).
 */
void ep_sinus_96k_stop(void *instance);

#ifdef __cplusplus
}
#endif

#endif /* SAC_SINUS_ENDPOINT_96K_H_ */
