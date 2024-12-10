/** @file  sac_fallback_gate.h
 *  @brief SPARK Audio Core Fallback gate is used to gate a processing stage based on the fallback state.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_FALLBACK_GATE_H_
#define SAC_FALLBACK_GATE_H_

/* INCLUDES *******************************************************************/
#include "sac_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Gate function for a transmitting pipeline's processing stage. It is called by a processing stage that should
 *         be executed when the fallback state is on.
 *
 *         e.g. The compression processing stage can be activated when fallback is active to compress the audio stream.
 *
 *  @param[in]  instance  Process instance.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  header    SAC header.
 *  @param[in]  data_in   Audio payload.
 *  @param[in]  size      Audio payload size.
 *  @param[out] status    Status code.
 *  @return True if the fallback state is active.
 */
bool sac_fallback_gate_is_fallback_on(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in,
                                      uint16_t size, sac_status_t *status);

/** @brief Gate function for a transmitting pipeline's processing stage. It is called by a processing stage that should
 *         be executed when the fallback state is off.
 *
 *         e.g. The compression processing stage needs to update its instance with up-to-date audio while the fallback
 *         is off to ensure a seamless switch when activating the audio compression.
 *
 *  @param[in]  instance  Process instance.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  header    SAC header.
 *  @param[in]  data_in   Audio payload.
 *  @param[in]  size      Audio payload size.
 *  @param[out] status    Status code.
 *  @return True if the fallback state is inactive.
 */
bool sac_fallback_gate_is_fallback_off(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in,
                                       uint16_t size, sac_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* SAC_FALLBACK_GATE_H_ */
