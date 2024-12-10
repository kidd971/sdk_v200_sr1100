/** @file  sac_cdc_pll.c
 *  @brief Clock drift compensation processing stage using audio buffer load averaging for
 *         detecting the drift and audio pll adjustment for correcting it.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sac_cdc_pll.h"
#include <stdio.h>
#include <stdlib.h>
#include "sac_utils.h"

/* CONSTANTS ******************************************************************/
#define DECIMAL_FACTOR               1000
#define INTEGRATOR_FACTOR            5
#define DRIFT_THRESHOLD              (DECIMAL_FACTOR / 4)
#define MAX_PLL_FRACN_OFFSET         (DECIMAL_FACTOR / 2)
#define ERROR_DIVISOR                (DECIMAL_FACTOR / 3)
#define QUEUE_AVERAGE_TIME_SEC       1
#define QUEUE_ARRAY_SIZE             2000
#define CDC_DEFAULT_EXTRA_QUEUE_SIZE 3
/* Queue level thresholds. */
#define CDC_QUEUE_HIGH_LEVEL_THRESHOLD(queue_limit) ((queue_limit) - 2)
#define CDC_QUEUE_LOW_LEVEL_THRESHOLD 1

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void adjust_latency(sac_cdc_pll_instance_t *cdc);
static void update_queue_avg(sac_cdc_pll_instance_t *cdc, sac_pipeline_t *pipeline);
static void validate_sac_bit_depth(sac_bit_depth_t bit_depth, sac_status_t *status);
static void reset_queue_avg(sac_cdc_pll_instance_t *cdc, sac_pipeline_t *pipeline);
static void device_clock_incr_pll2_fracn(sac_cdc_pll_instance_t *cdc);
static void device_clock_decr_pll2_fracn(sac_cdc_pll_instance_t *cdc);

/* PUBLIC FUNCTIONS ***********************************************************/
void sac_cdc_pll_init(void *instance, const char *name, sac_pipeline_t *pipeline, mem_pool_t *mem_pool,
                      sac_status_t *status)
{
    (void)name;

    sac_cdc_pll_instance_t *cdc = instance;

    *status = SAC_OK;

    SAC_CHECK_STATUS(cdc == NULL, status, SAC_ERR_NULL_PTR, return);

    /* Validate CDC PLL HAL. */
    SAC_CHECK_STATUS(cdc->cdc_pll_hal.fracn_default_value == 0, status, SAC_ERR_PROCESSING_STAGE_INIT, return);
    SAC_CHECK_STATUS(cdc->cdc_pll_hal.fracn_max_value == 0, status, SAC_ERR_PROCESSING_STAGE_INIT, return);
    SAC_CHECK_STATUS(cdc->cdc_pll_hal.fracn_min_value > cdc->cdc_pll_hal.fracn_max_value, status,
                     SAC_ERR_PROCESSING_STAGE_INIT, return);
    SAC_CHECK_STATUS(cdc->cdc_pll_hal.get_fracn == NULL, status, SAC_ERR_PROCESSING_STAGE_INIT, return);
    SAC_CHECK_STATUS(cdc->cdc_pll_hal.set_fracn == NULL, status, SAC_ERR_PROCESSING_STAGE_INIT, return);

    validate_sac_bit_depth(cdc->sample_format.bit_depth, status);
    if (*status != SAC_OK) {
        return;
    }

    /* Reset internal values. */
    cdc->_internal.error = 0;
    cdc->_internal.pll_fracn_offset = 0;

    /* Initialize configuration. */
    if (cdc->sample_format.sample_encoding == SAC_SAMPLE_UNPACKED) {
        cdc->_internal.size_of_buffer_type = SAC_WORD_SIZE_BYTE;
    } else {
        /* SAC_SAMPLE_PACKED */
        SAC_CHECK_STATUS((cdc->sample_format.bit_depth % SAC_BYTE_SIZE_BITS) != 0, status,
                         SAC_ERR_PROCESSING_STAGE_INIT, return);
        cdc->_internal.size_of_buffer_type = cdc->sample_format.bit_depth / SAC_BYTE_SIZE_BITS;
    }

    /* Configure threshold. */
    cdc->_internal.sample_amount = pipeline->consumer->cfg.audio_payload_size /
                                   (pipeline->consumer->cfg.channel_count * cdc->_internal.size_of_buffer_type);
    cdc->_internal.target_queue_size = pipeline->consumer->cfg.queue_size * cdc->_internal.sample_amount *
                                       DECIMAL_FACTOR;

    /* Allocate rolling average memory. */
    cdc->_internal.avg_arr = (uint8_t *)mem_pool_malloc(mem_pool, QUEUE_ARRAY_SIZE * sizeof(uint8_t));
    SAC_CHECK_STATUS(cdc->_internal.avg_arr == NULL, status, SAC_ERR_NOT_ENOUGH_MEMORY, return);
    reset_queue_avg(cdc, pipeline);

    /* Initialize the statistics. */
    memset(&cdc->_internal.sac_cdc_pll_stats, 0, sizeof(cdc->_internal.sac_cdc_pll_stats));

    /* Set consumer endpoint queue extra. */
    sac_set_extra_queue_size(pipeline->consumer, CDC_DEFAULT_EXTRA_QUEUE_SIZE, status);
    if (*status != SAC_OK) {
        return;
    }
}

uint32_t sac_cdc_pll_ctrl(void *instance, sac_pipeline_t *pipeline, uint8_t cmd, uint32_t arg, sac_status_t *status)
{
    sac_cdc_pll_instance_t *cdc = instance;
    sac_cdc_pll_cmd_t cdc_cmd = cmd;

    switch (cdc_cmd) {
    case SAC_CDC_PLL_CMD_INCREASE:
        device_clock_incr_pll2_fracn(cdc);
        break;
    case SAC_CDC_PLL_CMD_DECREASE:
        device_clock_decr_pll2_fracn(cdc);
        break;
    case SAC_CDC_PLL_CMD_SET_TARGET_QUEUE_SIZE:
        if (arg <= pipeline->consumer->cfg.queue_size && arg > 0) {
            cdc->_internal.target_queue_size = arg * cdc->_internal.sample_amount * DECIMAL_FACTOR;
        } else {
            *status = SAC_ERR_INVALID_ARG;
        }
        break;
    default:
        *status = SAC_ERR_INVALID_CMD;
    }

    return 0;
}

uint16_t sac_cdc_pll_process(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in,
                             uint16_t size, uint8_t *data_out, sac_status_t *status)
{
    (void)data_in;
    (void)data_out;
    (void)header;
    (void)size;

    static uint8_t tx_queue_level_high_count;
    static int32_t error_accumulator;
    sac_cdc_pll_instance_t *cdc = instance;
    uint32_t current_pll_fracn;

    *status = SAC_OK;
    current_pll_fracn = cdc->cdc_pll_hal.get_fracn();

    update_queue_avg(cdc, pipeline);

    if (cdc->_internal.queue_level_high) {
        /* Queue level is high. Increase consumption speed to avoid overflow. */
        cdc->cdc_pll_hal.set_fracn((cdc->cdc_pll_hal.fracn_default_value) + MAX_PLL_FRACN_OFFSET);
        cdc->_internal.pll_fracn_offset = MAX_PLL_FRACN_OFFSET;
        return 0;
    }

    /* Calculate average queue length only if audio link is stable. */
    if (header->tx_queue_level_high != 0) {
        if ((tx_queue_level_high_count > (pipeline->consumer->cfg.queue_size - 2)) &&
            (abs(cdc->_internal.pll_fracn_offset) > 0)) {
            /* Remove offset when multiple packets have the tx_queue_level_high flag. */
            cdc->cdc_pll_hal.set_fracn(current_pll_fracn - cdc->_internal.pll_fracn_offset);
            cdc->_internal.pll_fracn_offset = 0;
        }
        tx_queue_level_high_count++;
        return 0;
    } else {
        tx_queue_level_high_count = 0;
        if (cdc->_internal.queue_level_low) {
            /* Queue level is low. Decrease consumption speed to avoid underflow. */
            cdc->cdc_pll_hal.set_fracn((cdc->cdc_pll_hal.fracn_default_value) - MAX_PLL_FRACN_OFFSET);
            cdc->_internal.pll_fracn_offset = -MAX_PLL_FRACN_OFFSET;
            return 0;
        }

        if (cdc->_internal.avg_idx == 0) {
            /* Average ready and queue level is valid. */
            if (abs(cdc->_internal.avg_val_delta) < DRIFT_THRESHOLD) {
                /* Current PLL FRACN value has low drift.
                 * Calculate Integrator when drift is stable to reduce static error.
                 */
                error_accumulator += cdc->_internal.error;
                if ((cdc->_internal.pll_fracn_offset > 0) &&
                    (error_accumulator > (INTEGRATOR_FACTOR * DECIMAL_FACTOR))) {
                    cdc->_internal.pll_fracn_offset = 0;
                    error_accumulator = 0;
                } else if ((cdc->_internal.pll_fracn_offset < 0) &&
                           (error_accumulator < (-INTEGRATOR_FACTOR * DECIMAL_FACTOR))) {
                    cdc->_internal.pll_fracn_offset = 0;
                    error_accumulator = 0;
                }
            } else {
                error_accumulator = 0;
            }

            adjust_latency(cdc);
        }
    }

    /* CDC does not alter data, return 0 to indicate no data processing was done. */
    return 0;
}

sac_cdc_pll_stats_t sac_cdc_pll_get_stats(sac_cdc_pll_instance_t *cdc)
{
    sac_cdc_pll_stats_t cdc_stats = {
        .target_queue_size = cdc->_internal.target_queue_size / cdc->_internal.sample_amount,
        .avg_queue_size = cdc->_internal.avg_val / cdc->_internal.sample_amount,
        .queue_size_error = cdc->_internal.error / (int32_t)cdc->_internal.sample_amount,
        .queue_size_avg_delta = cdc->_internal.avg_val_delta / (int32_t)cdc->_internal.sample_amount,
        .current_pll_value = cdc->cdc_pll_hal.get_fracn(),
        .pll_fracn_offset = cdc->_internal.pll_fracn_offset,
    };

    return cdc_stats;
}

int sac_cdc_pll_format_stats(sac_cdc_pll_instance_t *cdc, char *buffer, uint16_t size)
{
    int string_length = 0;

    sac_cdc_pll_stats_t cdc_stats = sac_cdc_pll_get_stats(cdc);

    string_length = snprintf(buffer, size,
                             "\n<< CDC STATS >>\r\n"
                             "  %s:\t\t%10lu\r\n"
                             "  %s:\t\t%10lu\r\n"
                             "  %s:\t\t\t%10li\r\n"
                             "  %s:\t\t\t%10li\r\n"
                             "  %s:\t\t%10lu\r\n"
                             "  %s:\t\t%10li\r\n",
                             "Target queue size", cdc_stats.target_queue_size, "Avg queue size",
                             cdc_stats.avg_queue_size, "Error", cdc_stats.queue_size_error, "Avg delta",
                             cdc_stats.queue_size_avg_delta, "Current PLL value", cdc_stats.current_pll_value,
                             "PLL fracn offset", cdc_stats.pll_fracn_offset);

    return string_length;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Tune queue size to the target level.
 *
 *  @param[in] cdc  CDC instance.
 */
static void adjust_latency(sac_cdc_pll_instance_t *cdc)
{
    int16_t current_pll_fracn_offset;
    int16_t adjust_pll_fracn_offset;
    uint32_t current_pll_fracn;

    /* Save the current offset. */
    current_pll_fracn_offset = cdc->_internal.pll_fracn_offset;

    /* Calculate the new pll offset. */
    cdc->_internal.pll_fracn_offset = cdc->_internal.error / ERROR_DIVISOR;
    if (cdc->_internal.pll_fracn_offset > MAX_PLL_FRACN_OFFSET) {
        cdc->_internal.pll_fracn_offset = MAX_PLL_FRACN_OFFSET;
    } else if (cdc->_internal.pll_fracn_offset < -MAX_PLL_FRACN_OFFSET) {
        cdc->_internal.pll_fracn_offset = -MAX_PLL_FRACN_OFFSET;
    }

    /* Calculate the required adjustement and apply it. */
    adjust_pll_fracn_offset = cdc->_internal.pll_fracn_offset - current_pll_fracn_offset;
    current_pll_fracn = cdc->cdc_pll_hal.get_fracn();
    cdc->cdc_pll_hal.set_fracn(current_pll_fracn + adjust_pll_fracn_offset);
}

/** @brief Update the queue level average and queue level delta.
 *
 *  @param[in] cdc       CDC instance.
 *  @param[in] pipeline  Pipeline instance.
 */
static void update_queue_avg(sac_cdc_pll_instance_t *cdc, sac_pipeline_t *pipeline)
{
    uint16_t current_queue_length = pipeline->_internal.samples_buffered_size /
                                    (pipeline->consumer->cfg.channel_count * cdc->_internal.size_of_buffer_type *
                                     cdc->_internal.sample_amount);
    uint16_t avg_idx = cdc->_internal.avg_idx;

    /* Check is queue level is high. */
    cdc->_internal.queue_level_high = current_queue_length > CDC_QUEUE_HIGH_LEVEL_THRESHOLD(
                                                                 queue_get_limit(pipeline->consumer->_internal.queue));
    cdc->_internal.queue_level_low = current_queue_length <= CDC_QUEUE_LOW_LEVEL_THRESHOLD;

    /* Update Rolling Avg. */
    cdc->_internal.avg_sum -= cdc->_internal.avg_arr[avg_idx]; /* Remove oldest value. */
    cdc->_internal.avg_arr[avg_idx] = current_queue_length;
    cdc->_internal.avg_sum += cdc->_internal.avg_arr[avg_idx]; /* Add new value. */
    cdc->_internal.avg_val = cdc->_internal.sample_amount *
                             ((cdc->_internal.avg_sum * DECIMAL_FACTOR) / QUEUE_ARRAY_SIZE);
    cdc->_internal.error = cdc->_internal.avg_val - cdc->_internal.target_queue_size;

    if (++avg_idx >= QUEUE_ARRAY_SIZE) {
        avg_idx = 0;
        /* Update Delta Avg. */
        cdc->_internal.avg_val_delta = cdc->_internal.avg_val - cdc->_internal.prev_avg_val;
        cdc->_internal.prev_avg_val = cdc->_internal.avg_val;
    }
    cdc->_internal.avg_idx = avg_idx;
}

/** @brief Reset the queue level average and queue level delta.
 *
 *  @param[in] cdc       CDC instance.
 *  @param[in] pipeline  Pipeline instance.
 */
static void reset_queue_avg(sac_cdc_pll_instance_t *cdc, sac_pipeline_t *pipeline)
{
    cdc->_internal.avg_idx = 0;
    cdc->_internal.avg_val = cdc->_internal.target_queue_size;
    cdc->_internal.prev_avg_val = cdc->_internal.target_queue_size;
    cdc->_internal.avg_val_delta = 0;
    for (uint32_t i = 0; i < QUEUE_ARRAY_SIZE; i++) {
        cdc->_internal.avg_arr[i] = pipeline->consumer->cfg.queue_size;
    }
    cdc->_internal.avg_sum = pipeline->consumer->cfg.queue_size * QUEUE_ARRAY_SIZE;
}

/** @brief Validate if bit depth value is supported by the SAC.
 *
 *  @param[in]  bit_depth  Bit depth to validate.
 *  @param[out] status     Status code.
 */
static void validate_sac_bit_depth(sac_bit_depth_t bit_depth, sac_status_t *status)
{
    if ((bit_depth != SAC_16BITS) && (bit_depth != SAC_18BITS) && (bit_depth != SAC_20BITS) &&
        (bit_depth != SAC_24BITS) && (bit_depth != SAC_32BITS)) {
        *status = SAC_ERR_BIT_DEPTH;
    }
}

/** @brief Increment the PLL2 fractional part by 1.
 */
void device_clock_incr_pll2_fracn(sac_cdc_pll_instance_t *cdc)
{
    uint32_t current_fracn = cdc->cdc_pll_hal.get_fracn();

    cdc->cdc_pll_hal.set_fracn(current_fracn + 1);
}

/** @brief Decrement the PLL2 fractional part by 1.
 */
void device_clock_decr_pll2_fracn(sac_cdc_pll_instance_t *cdc)
{
    uint32_t current_fracn = cdc->cdc_pll_hal.get_fracn();

    cdc->cdc_pll_hal.set_fracn(current_fracn - 1);
}
