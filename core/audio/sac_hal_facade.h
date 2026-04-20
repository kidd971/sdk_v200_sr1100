/** @file  sac_hal_facade.h
 *
 *  @brief Facades for low-level platform-specific features required by SPARK Audio Core.
 *
 *  @note This header defines the interfaces for various hardware features used by the
 *  SPARK Audio Core library. The actual implementations are selected at compile time
 *  based on the target platform, allowing for flexibility and portability across
 *  different hardware.
 *
 *  The facade is designed to be a compile-time dependency only, with no
 *  support for runtime polymorphism. This ensures tight integration with the
 *  build system and minimal overhead.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_HAL_FACADE_H_
#define SAC_HAL_FACADE_H_

/* INCLUDES *******************************************************************/
#include "sac_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the audio endpoint interfaces.
 *
 *  @param[out] producer_iface  Producer audio endpoint interface.
 *  @param[out] consumer_iface  Consumer audio endpoint interface.
 */
void sac_facade_audio_endpoint_init(sac_endpoint_interface_t *producer_iface, sac_endpoint_interface_t *consumer_iface);

/** @brief Initialize the Clock Drift Compensation processing stage.
 *
 *  @param[in]  format  Audio sample format used in the CDC processing.
 *  @param[out] status  Status code.
 *  @return Reference to the initialized CDC processing stage.
 */
sac_processing_t *sac_facade_cdc_processing_init(sac_sample_format_t format, sac_status_t *status);

/** @brief Format the Clock Drift Compensation statistics as a string of characters.
 *
 *  @param[out] buffer  Buffer where to put the formatted string.
 *  @param[in]  size    Size of the buffer.
 *  @param[out] status  Status code.
 *  @return The formatted string length, excluding the NULL terminator.
 */
int sac_facade_cdc_format_stats(char *buffer, uint16_t size, sac_status_t *status);

/** @brief Set the Clock Drift Compensation queue size target.
 *
 *  @param[in]  pipeline    Pipeline instance.
 *  @param[in]  queue_size  Queue size to set.
 *  @param[out] status      Status code.
 */
void facade_app_audio_cdc_set_target_queue_size(sac_pipeline_t *pipeline, uint8_t queue_size, sac_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* SAC_HAL_FACADE_H_ */
