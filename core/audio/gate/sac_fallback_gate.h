/** @file  sac_fallback_gate.h
 *  @brief SPARK Audio Core Fallback gate is used to gate a processing stage based on the fallback state.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
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
/** @brief Gate function to verify if a process is assigned to the current fallback mode.
 *
 *         e.g. The compression processing stage can be activated when fallback is active to compress the audio stream.
 *
 *  @param[in]  process   Pointer to process handle to verify.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  header    SAC header.
 *  @param[in]  data_in   Audio payload.
 *  @param[in]  size      Audio payload size.
 *  @param[out] status    Status code.
 *  @return True if the process is assigned to the current fallback mode.
 */

bool sac_fallback_gate_is_process_active(sac_processing_t *process, sac_pipeline_t *pipeline, sac_header_t *header,
                                         uint8_t *data_in, uint16_t size, sac_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* SAC_FALLBACK_GATE_H_ */
