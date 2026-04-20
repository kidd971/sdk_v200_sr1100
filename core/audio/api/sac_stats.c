/** @file  sac_stats.c
 *  @brief SPARK Audio Core statistics.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sac_stats.h"
#include <inttypes.h>
#include <stdio.h>
#include "sac_utils.h"

/* PUBLIC FUNCTIONS ***********************************************************/
sac_statistics_t *sac_pipeline_update_stats(sac_pipeline_t *pipeline, sac_status_t *status)
{
    *status = SAC_OK;

    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_NULL_PTR, return NULL);

    pipeline->_statistics.producer_buffer_load = sac_pipeline_get_producer_buffer_load(pipeline, status);
    if (*status != SAC_OK) {
        return NULL;
    }

    pipeline->_statistics.consumer_buffer_load = sac_pipeline_get_consumer_buffer_load(pipeline, status);
    if (*status != SAC_OK) {
        return NULL;
    }

    return &pipeline->_statistics;
}

int sac_pipeline_format_stats(sac_pipeline_t *pipeline, char *buffer, uint16_t size, sac_status_t *status)
{
    int string_length = 0;

    *status = SAC_OK;

    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_NULL_PTR, return 0);
    SAC_CHECK_STATUS(buffer == NULL, status, SAC_ERR_NULL_PTR, return 0);

    const char *producer_str = "Producer";
    const char *consumer_str = "Consumer";
    const char *buffer_load_str = "Buffer Load";
    const char *buffer_size_str = "Buffer Size";
    const char *producer_buffer_overflow_count_str = "Producer Overflow Count";
    const char *consumer_buffer_overflow_count_str = "Buffer Overflow Count";
    const char *consumer_buffer_underflow_count_str = "Buffer Underflow Count";
    const char *producer_packets_corrupted_count_str = "Corrupted Packets Count";

    string_length = snprintf(buffer, size,
                             "<<< %s >>>\r\n"
                             "%s\r\n"
                             "  %s:\t\t\t%10" PRIu32 "\r\n"
                             "  %s:\t\t\t%10" PRIu16 "\r\n"
                             "  %s:\t%10" PRIu32 "\r\n"
                             "%s\r\n"
                             "  %s:\t\t\t%10" PRIu32 "\r\n"
                             "  %s:\t\t\t%10" PRIu16 "\r\n"
                             "  %s:\t%10" PRIu32 "\r\n"
                             "  %s:\t%10" PRIu32 "\r\n"
                             "  %s:\t%10" PRIu32 "\r\n",
                             pipeline->name, producer_str, buffer_load_str, pipeline->_statistics.producer_buffer_load,
                             buffer_size_str, pipeline->_statistics.producer_buffer_size,
                             producer_packets_corrupted_count_str,
                             pipeline->_statistics.producer_packets_corrupted_count, consumer_str, buffer_load_str,
                             pipeline->_statistics.consumer_buffer_load, buffer_size_str,
                             pipeline->_statistics.consumer_buffer_size, producer_buffer_overflow_count_str,
                             pipeline->_statistics.producer_buffer_overflow_count, consumer_buffer_overflow_count_str,
                             pipeline->_statistics.consumer_buffer_overflow_count, consumer_buffer_underflow_count_str,
                             pipeline->_statistics.consumer_buffer_underflow_count);

    return string_length;
}

uint32_t sac_pipeline_get_producer_buffer_load(sac_pipeline_t *pipeline, sac_status_t *status)
{
    *status = SAC_OK;

    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_NULL_PTR, return 0);

    return pipeline->producer->_internal.queue->length;
}

uint32_t sac_pipeline_get_consumer_buffer_load(sac_pipeline_t *pipeline, sac_status_t *status)
{
    *status = SAC_OK;

    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_NULL_PTR, return 0);

    return pipeline->consumer->_internal.queue->length;
}

uint32_t sac_pipeline_get_consumer_buffer_overflow_count(sac_pipeline_t *pipeline, sac_status_t *status)
{
    *status = SAC_OK;

    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_NULL_PTR, return 0);

    return pipeline->_statistics.consumer_buffer_overflow_count;
}

uint32_t sac_pipeline_get_consumer_buffer_underflow_count(sac_pipeline_t *pipeline, sac_status_t *status)
{
    *status = SAC_OK;

    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_NULL_PTR, return 0);

    return pipeline->_statistics.consumer_buffer_underflow_count;
}

uint32_t sac_pipeline_get_producer_buffer_overflow_count(sac_pipeline_t *pipeline, sac_status_t *status)
{
    *status = SAC_OK;

    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_NULL_PTR, return 0);

    return pipeline->_statistics.producer_buffer_overflow_count;
}

uint32_t sac_pipeline_get_producer_packets_corrupted_count(sac_pipeline_t *pipeline, sac_status_t *status)
{
    *status = SAC_OK;

    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_NULL_PTR, return 0);

    return pipeline->_statistics.producer_packets_corrupted_count;
}

uint32_t sac_pipeline_get_consumer_queue_peak_buffer_load(sac_pipeline_t *pipeline, sac_status_t *status)
{
    *status = SAC_OK;

    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_NULL_PTR, return 0);

    return pipeline->_statistics.consumer_queue_peak_buffer_load;
}

void sac_pipeline_reset_stats(sac_pipeline_t *pipeline, sac_status_t *status)
{
    uint32_t consume_size = 0;
    uint32_t produce_size = 0;

    *status = SAC_OK;

    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_NULL_PTR, return);

    produce_size = pipeline->_statistics.producer_buffer_size;
    consume_size = pipeline->_statistics.consumer_buffer_size;

    memset(&pipeline->_statistics, 0, sizeof(sac_statistics_t));

    pipeline->_statistics.producer_buffer_size = produce_size;
    pipeline->_statistics.consumer_buffer_size = consume_size;
}
