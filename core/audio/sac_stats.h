/** @file  sac_stats.h
 *  @brief SPARK Audio Core statistics.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_STATS_H_
#define SAC_STATS_H_

/* INCLUDES *******************************************************************/
#include "sac_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Update the SPARK Audio Core pipeline statistics.
 *
 *  After calling this function, the statistics inside the
 *  pipeline instance will be updated. The function also
 *  returns a reference to these internal statistics so they
 *  can be used by the caller.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[out] status    Status code.
 *  @return Reference to the statistics.
 */
sac_statistics_t *sac_pipeline_update_stats(sac_pipeline_t *pipeline, sac_status_t *status);

/** @brief Format the pipeline statistics as a string of characters.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[out] buffer    Buffer where to put the formatted string.
 *  @param[in]  size      Size of the buffer.
 *  @param[out] status    Status code.
 *  @return The formatted string length, excluding the NULL terminator.
 */
int sac_pipeline_format_stats(sac_pipeline_t *pipeline, char *buffer, uint16_t size, sac_status_t *status);

/** @brief Get producer buffer load.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[out] status    Status code.
 *  @return buffer producer load value
 */
uint32_t sac_pipeline_get_producer_buffer_load(sac_pipeline_t *pipeline, sac_status_t *status);

/** @brief Get consumer buffer load.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[out] status    Status code.
 *  @return Consumer buffer load value.
 */
uint32_t sac_pipeline_get_consumer_buffer_load(sac_pipeline_t *pipeline, sac_status_t *status);

/** @brief Get consumer buffer overflow count.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[out] status    Status code.
 *  @return Consumer buffer overflow counter.
 */
uint32_t sac_pipeline_get_consumer_buffer_overflow_count(sac_pipeline_t *pipeline, sac_status_t *status);

/** @brief Get consumer buffer underflow count.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[out] status    Status code.
 *  @return Consumer buffer underflow counter.
 */
uint32_t sac_pipeline_get_consumer_buffer_underflow_count(sac_pipeline_t *pipeline, sac_status_t *status);

/** @brief Get producer buffer overflow count.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[out] status    Status code.
 *  @return Producer buffer overflow counter.
 */
uint32_t sac_pipeline_get_producer_buffer_overflow_count(sac_pipeline_t *pipeline, sac_status_t *status);

/** @brief Get producer packets corrupted.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[out] status    Status code.
 *  @return Producer packets corrupted.
 */
uint32_t sac_pipeline_get_producer_packets_corrupted_count(sac_pipeline_t *pipeline, sac_status_t *status);

/** @brief Get consumer queue peak load.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[out] status    Status code.
 *  @return Consumer queue peak load.
 */
uint32_t sac_pipeline_get_consumer_queue_peak_buffer_load(sac_pipeline_t *pipeline, sac_status_t *status);

/** @brief Reset the SPARK Audio Core stats.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[out] status    Status code.
 */
void sac_pipeline_reset_stats(sac_pipeline_t *pipeline, sac_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* SAC_STATS_H_ */
