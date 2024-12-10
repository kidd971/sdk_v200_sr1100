/** @file  sac_cdc.c
 *  @brief Clock drift compensation processing stage using audio buffer load averaging for
 *         detecting the drift and interpolation for correcting it.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sac_cdc.h"
#include <stdio.h>

/* CONSTANTS ******************************************************************/
#define DECIMAL_FACTOR 100
#define CDC_DEFAULT_EXTRA_QUEUE_SIZE 3

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void detect_drift(sac_cdc_instance_t *cdc, sac_pipeline_t *pipeline, sac_header_t *header);
static uint16_t correct_drift(sac_cdc_instance_t *cdc, uint8_t *data_in, uint16_t size, uint8_t *data_out);
static void update_queue_avg(sac_cdc_instance_t *cdc, sac_pipeline_t *pipeline);
static void validate_sac_bit_depth(sac_bit_depth_t bit_depth, sac_status_t *status);

/* PUBLIC FUNCTIONS ***********************************************************/
void sac_cdc_init(void *instance, const char *name, sac_pipeline_t *pipeline, mem_pool_t *mem_pool,
                  sac_status_t *status)
{
    (void)name;

    sac_cdc_instance_t *cdc = instance;
    resampling_config_t resampling_config = {0};

    *status = SAC_OK;

    if (cdc == NULL) {
        *status = SAC_ERR_NULL_PTR;
        return;
    }

    validate_sac_bit_depth(cdc->sample_format.bit_depth, status);
    if (*status != SAC_OK) {
        return;
    }

    cdc->_internal.avg_sum = 0;
    cdc->_internal.avg_val = 0;
    cdc->_internal.avg_idx = 0;
    cdc->_internal.queue_avg_size = cdc->cdc_queue_avg_size;

    /* Allocate rolling average memory. */
    cdc->_internal.avg_arr = (uint16_t *)mem_pool_malloc(mem_pool, cdc->_internal.queue_avg_size * sizeof(uint16_t));
    if (cdc->_internal.avg_arr == NULL) {
        *status = SAC_ERR_NOT_ENOUGH_MEMORY;
        return;
    }
    memset(cdc->_internal.avg_arr, 0, cdc->_internal.queue_avg_size);

    /* Initialize resampling configuration. */
    if (cdc->sample_format.sample_encoding == SAC_SAMPLE_UNPACKED) {
        cdc->_internal.size_of_buffer_type = SAC_WORD_SIZE_BYTE;
    } else {
        /* SAC_SAMPLE_PACKED */
        if ((cdc->sample_format.bit_depth % SAC_BYTE_SIZE_BITS) != 0) {
            /* Bit depth not aligned to a byte. */
            *status = SAC_ERR_PROCESSING_STAGE_INIT;
            return;
        }
        cdc->_internal.size_of_buffer_type = cdc->sample_format.bit_depth / SAC_BYTE_SIZE_BITS;
    }

    resampling_config.nb_sample = (pipeline->consumer->cfg.audio_payload_size / cdc->_internal.size_of_buffer_type);
    resampling_config.nb_channel = pipeline->consumer->cfg.channel_count;
    resampling_config.resampling_length = cdc->cdc_resampling_length;
    resampling_config.buffer_type = cdc->sample_format.bit_depth - 1;

    /* Initialize the resampling instance. */
    if (resampling_init(&cdc->_internal.resampling_instance, &resampling_config) != RESAMPLING_NO_ERROR) {
        *status = SAC_ERR_PROCESSING_STAGE_INIT;
        return;
    }

    /* Configure threshold. */
    cdc->_internal.sample_amount = pipeline->consumer->cfg.audio_payload_size /
                                  (pipeline->consumer->cfg.channel_count * cdc->_internal.size_of_buffer_type);
    cdc->_internal.normal_queue_size = pipeline->consumer->cfg.queue_size * cdc->_internal.sample_amount * DECIMAL_FACTOR;
    cdc->_internal.max_queue_offset = 3 * DECIMAL_FACTOR; /* 3 samples. */

    /* Initialize the statistics. */
    memset(&cdc->_internal.sac_cdc_resampling_stats, 0, sizeof(cdc->_internal.sac_cdc_resampling_stats));

    /* Set consumer endpoint queue extra. */
    sac_set_extra_queue_size(pipeline->consumer, CDC_DEFAULT_EXTRA_QUEUE_SIZE, status);
    if (*status != SAC_OK) {
        return;
    }
}

uint32_t sac_cdc_ctrl(void *instance, sac_pipeline_t *pipeline, uint8_t cmd, uint32_t arg, sac_status_t *status)
{
    uint32_t ret = 0;
    sac_cdc_instance_t *cdc_inst = instance;

    *status = SAC_OK;

    switch ((sac_cdc_cmd_t)cmd) {
    case SAC_CDC_SET_TARGET_QUEUE_SIZE:
        if (arg <= pipeline->consumer->cfg.queue_size && arg > 0) {
            cdc_inst->_internal.normal_queue_size = arg * cdc_inst->_internal.sample_amount * DECIMAL_FACTOR;
        } else {
            *status = SAC_ERR_INVALID_ARG;
        }
        break;
    default:
        *status = SAC_ERR_INVALID_CMD;
    }
    return ret;
}

uint16_t sac_cdc_process(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in,
                         uint16_t size, uint8_t *data_out, sac_status_t *status)
{
    sac_cdc_instance_t *cdc = instance;
    uint16_t original_sample_count = size / cdc->_internal.size_of_buffer_type;
    uint16_t new_sample_count;

    *status = SAC_OK;

    detect_drift(cdc, pipeline, header);

    new_sample_count = correct_drift(cdc, data_in, size, data_out);
    if (new_sample_count > original_sample_count) {
        cdc->_internal.sac_cdc_resampling_stats.cdc_inflated_packets_count++;
    } else if (new_sample_count < original_sample_count) {
        cdc->_internal.sac_cdc_resampling_stats.cdc_deflated_packets_count++;
    }

    return (new_sample_count * cdc->_internal.size_of_buffer_type);
}

int sac_cdc_format_stats(sac_cdc_instance_t *cdc, char *buffer, uint16_t size)
{
    int string_length = 0;
    const char *cdc_inflated_packets_count_str = "CDC Inflated Packets Count";
    const char *cdc_deflated_packets_count_str = "CDC Deflated Packets Count";

    string_length = snprintf(buffer, size,
                             "\n<< CDC STATS >>\r\n"
                             "  %s:\t%10lu\r\n"
                             "  %s:\t%10lu\r\n",
                             cdc_inflated_packets_count_str,
                             cdc->_internal.sac_cdc_resampling_stats.cdc_inflated_packets_count,
                             cdc_deflated_packets_count_str,
                             cdc->_internal.sac_cdc_resampling_stats.cdc_deflated_packets_count);

    return string_length;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Detect an audio clock drift based on the average audio queue load.
 *
 *  @param[in] cdc       CDC instance.
 *  @param[in] pipeline  Pipeline instance.
 *  @param[in] header    Audio packet's header.
 */
static void detect_drift(sac_cdc_instance_t *cdc, sac_pipeline_t *pipeline, sac_header_t *header)
{
    /* Calculate average queue length only if audio link is stable. */
    if (header->tx_queue_level_high == 0) {
        update_queue_avg(cdc, pipeline);
    }

    if ((header->tx_queue_level_high == 1) &&
        (resample_get_state(&cdc->_internal.resampling_instance) == RESAMPLING_IDLE)) {
        cdc->_internal.wait_for_queue_full = true;
    }

    if (resample_get_state(&cdc->_internal.resampling_instance) == RESAMPLING_WAIT_QUEUE_FULL) {
        if (header->tx_queue_level_high == 0) {
            cdc->_internal.resampling_instance.status = RESAMPLING_IDLE;
            cdc->_internal.wait_for_queue_full = false;
        }
    } else if (resample_get_state(&cdc->_internal.resampling_instance) == RESAMPLING_IDLE) {
        /* Verify if queue is increasing or depleting. */
        if (cdc->_internal.wait_for_queue_full) {
            cdc->_internal.resampling_instance.status = RESAMPLING_WAIT_QUEUE_FULL;
        } else {
            if (cdc->_internal.count > cdc->_internal.queue_avg_size) {
                if (cdc->_internal.avg_val > (cdc->_internal.normal_queue_size + cdc->_internal.max_queue_offset)) {
                    resampling_start(&cdc->_internal.resampling_instance, RESAMPLING_REMOVE_SAMPLE);
                    cdc->_internal.count = 0;
                } else if (cdc->_internal.avg_val < (cdc->_internal.normal_queue_size - cdc->_internal.max_queue_offset)) {
                    resampling_start(&cdc->_internal.resampling_instance, RESAMPLING_ADD_SAMPLE);
                    cdc->_internal.count = 0;
                }
            } else {
                /* Give time to the avg to stabilize before checking. */
                cdc->_internal.count++;
            }
        }
    }
}

/** @brief Correct the audio clock drift using interpolation.
 *
 *  @param[in] cdc       CDC instance.
 *  @param[in] data_in   Audio payload to process.
 *  @param[in] size      Size in bytes of the audio payload.
 *  @param[in] data_out  Audio payload that has been processed.
 *  @return Number of samples after the correction has been applied.
 */
static uint16_t correct_drift(sac_cdc_instance_t *cdc, uint8_t *data_in, uint16_t size, uint8_t *data_out)
{
    uint16_t sample_count = size / cdc->_internal.size_of_buffer_type;

    sample_count = resample(&cdc->_internal.resampling_instance, data_in, data_out, sample_count);

    return sample_count;
}

/** @brief Update the rolling average of the audio buffer load.
 *
 *  Values in the average are the number of samples multiplied
 *  by DECIMAL_FACTOR to have a proper granularity.
 *
 *  @param[in] cdc       CDC instance.
 *  @param[in] pipeline  Pipeline instance.
 */
static void update_queue_avg(sac_cdc_instance_t *cdc, sac_pipeline_t *pipeline)
{
    uint16_t current_queue_length = pipeline->_internal.samples_buffered_size / (pipeline->consumer->cfg.channel_count * cdc->_internal.size_of_buffer_type);
    uint16_t avg_idx = cdc->_internal.avg_idx;

    /* Update Rolling Avg */
    cdc->_internal.avg_sum -= cdc->_internal.avg_arr[avg_idx]; /* Remove oldest value. */
    cdc->_internal.avg_arr[avg_idx] = current_queue_length;
    cdc->_internal.avg_sum += cdc->_internal.avg_arr[avg_idx]; /* Add new value. */
    if (++avg_idx >= cdc->_internal.queue_avg_size) {
        avg_idx = 0;
    }
    cdc->_internal.avg_val = (cdc->_internal.avg_sum * DECIMAL_FACTOR) / cdc->_internal.queue_avg_size;
    cdc->_internal.avg_idx = avg_idx;
}

/** @brief Validate if bit depth value is supported by the SAC.
 *
 *  @param[in]  bit_depth  Bit depth to validate.
 *  @param[out] status     Status code.
 */
static void validate_sac_bit_depth(sac_bit_depth_t bit_depth, sac_status_t *status)
{
    if ((bit_depth != SAC_16BITS) &&
        (bit_depth != SAC_18BITS) &&
        (bit_depth != SAC_20BITS) &&
        (bit_depth != SAC_24BITS) &&
        (bit_depth != SAC_32BITS)) {
        *status = SAC_ERR_BIT_DEPTH;
    }
}

