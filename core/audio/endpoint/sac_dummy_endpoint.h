/** @file  sac_dummy_endpoint.h
 *  @brief SPARK Audio Core dummy endpoints.
 *
 *  This file contains functions designed to control a dummy endpoint for audio data handling. The main actions provided
 *  by these functions do not perform any operations themselves. Instead, they serve as placeholders, providing function
 *  pointers to 'dummy' functions. These dummy functions are used primarily to facilitate the action of clearing the
 *  queue of audio samples without actually processing them. This setup helps in maintaining the structure necessary for
 *  potentially more complex operations while using simple, non-functional placeholders.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_DUMMY_ENDPOINT_H_
#define SAC_DUMMY_ENDPOINT_H_

/* INCLUDES *******************************************************************/
#include "sac_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Dummy endpoint produce action.
 *
 *  @param[in]  instance  Endpoint instance.
 *  @param[out] samples   Produced samples.
 *  @param[in]  size      Size of samples to produce in bytes.
 *  @return Number of bytes produced.
 */
uint16_t ep_dummy_produce(void *instance, uint8_t *samples, uint16_t size);

/** @brief Dummy endpoint consume action.
 *
 *  @param[in]  instance  Endpoint instance.
 *  @param[out] samples   Consumed samples.
 *  @param[in]  size      Size of samples to consume in bytes.
 *  @return Number of bytes consumed.
 */
uint16_t ep_dummy_consume(void *instance, uint8_t *samples, uint16_t size);

/** @brief Dummy endpoint start.
 *
 *  @param[in] instance  Endpoint instance.
 */
void ep_dummy_start(void *instance);

/** @brief Dummy endpoint stop.
 *
 *  @param[in] instance  Endpoint instance.
 */
void ep_dummy_stop(void *instance);

#ifdef __cplusplus
}
#endif

#endif /* SAC_MIXER_ENDPOINT_H_ */
