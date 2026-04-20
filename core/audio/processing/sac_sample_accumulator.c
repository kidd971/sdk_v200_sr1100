/** @file  sac_sample_accumulator.c
 *  @brief SPARK Audio Core sample accumulator processing stage.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sac_sample_accumulator.h"
#include <string.h>
#include "sac_error.h"
#include "sac_stats.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void sac_sample_accumulator_init(void *instance, const char *name, sac_pipeline_t *pipeline, mem_pool_t *mem_pool,
                                 sac_status_t *status)
{
    (void)name;

    if ((instance == NULL) || (pipeline == NULL) || (mem_pool == NULL)) {
        *status = SAC_ERR_NULL_PTR;
        return;
    }

    sac_sample_accumulator_instance_t *acc_inst = instance;

    if (acc_inst == NULL) {
        *status = SAC_ERR_PROCESSING_STAGE_INIT;
        return;
    }

    if (pipeline->cfg.max_payload_size < acc_inst->max_accumulator_size) {
        *status = SAC_ERR_PIPELINE_PAYLOAD_NOT_BIG_ENOUGH;
        return;
    }

    *status = SAC_OK;

    /* Init accumulator memory. */
    acc_inst->_internal.accumulator_used_size = 0;
    acc_inst->_internal.accumulator_size = 0;
    acc_inst->_internal.prod_enqueue_count = 0;
    acc_inst->_internal.accumulator = (uint8_t *)mem_pool_malloc(mem_pool, acc_inst->max_accumulator_size);
    acc_inst->_internal.underflow_count = sac_pipeline_get_consumer_buffer_underflow_count(pipeline, status);
}

uint32_t sac_sample_accumulator_ctrl(void *instance, sac_pipeline_t *pipeline, uint8_t cmd, uint32_t arg,
                                     sac_status_t *status)
{
    (void)pipeline;
    (void)arg;

    uint32_t ret = 0;
    sac_sample_accumulator_instance_t *acc_inst = instance;

    *status = SAC_OK;

    switch ((sac_sample_accumulator_cmd_t)cmd) {
    case SAC_SAMPLE_ACCUMULATOR_GET_USED_SIZE:
        ret = acc_inst->_internal.accumulator_used_size;
        break;
    default:
        *status = SAC_ERR_INVALID_CMD;
    }
    return ret;
}

uint16_t sac_sample_accumulator_process(void *instance, sac_pipeline_t *pipeline, sac_header_t *header,
                                        uint8_t *data_in, uint16_t size, uint8_t *data_out, sac_status_t *status)
{
    sac_sample_accumulator_instance_t *acc_inst = instance;
    uint16_t input_size = size;
    uint16_t ret = 0;
    uint32_t current_underflow_count;
    uint32_t current_target_size = 0;

    *status = SAC_OK;

    if (acc_inst->get_accumulator_size) {
        current_target_size = acc_inst->get_accumulator_size(pipeline);
        if (current_target_size > acc_inst->max_accumulator_size) {
            /* ERROR! */
            return 0;
        }
        if (current_target_size < acc_inst->_internal.accumulator_used_size) {
            /* Since output size = input size, we'll copy data from accumulator only. */
            uint16_t extra_accumulator_bytes = acc_inst->_internal.accumulator_used_size - current_target_size;

            /* Copy accumulator data into output. */
            memcpy(data_out, acc_inst->_internal.accumulator, current_target_size);
            /* Move left-over data to start of accumulator. */
            memcpy(acc_inst->_internal.accumulator, &acc_inst->_internal.accumulator[current_target_size],
                   extra_accumulator_bytes);
            /* Put input data into accumulator. */
            memcpy(&acc_inst->_internal.accumulator[extra_accumulator_bytes], data_in, size);
            acc_inst->_internal.accumulator_used_size = extra_accumulator_bytes + size;

            return current_target_size;
        }
        acc_inst->_internal.accumulator_size = current_target_size;
    } else {
        acc_inst->_internal.accumulator_size = acc_inst->max_accumulator_size;
    }

    /* Calculate number of missing bytes required to generate a packet of size accumulator_size. */
    uint32_t required_size = acc_inst->_internal.accumulator_size - acc_inst->_internal.accumulator_used_size;

    if (acc_inst->_internal.prod_enqueue_count > 0) {
        /* Ignore packets enqueued by this processing stage. */
        acc_inst->_internal.prod_enqueue_count--;
        pipeline->_internal.pending_packets--;
        return 0;
    }

    current_underflow_count = sac_pipeline_get_consumer_buffer_underflow_count(pipeline, status);
    if ((current_underflow_count != acc_inst->_internal.underflow_count) && (current_underflow_count != 0)) {
        /* Underflow count changed but was not reset. Flush accumulator (packet was lost). */
        acc_inst->_internal.accumulator_used_size = 0;
        required_size = acc_inst->_internal.accumulator_size;
    }
    acc_inst->_internal.underflow_count = current_underflow_count;

    if (required_size > input_size) {
        /* There isn't enough data accumulated to produce a packet. */
        *status = SAC_WARN_NO_PACKET_TO_PRODUCE;
        ret = 0;
    } else {
        /* Read the required bytes from the input data. */
        memcpy(data_out, acc_inst->_internal.accumulator, acc_inst->_internal.accumulator_used_size);
        memcpy(&data_out[acc_inst->_internal.accumulator_used_size], data_in, required_size);
        input_size -= required_size;
        acc_inst->_internal.accumulator_used_size = 0;
        ret = acc_inst->_internal.accumulator_size;
    }

    /* Save current read pointer. */
    uint8_t *current_read_ptr = &data_in[size - input_size];

    /* Enqueue packet in producer to be processed later. */
    queue_node_t *node = NULL;
    uint8_t extra_packet_nb = 0;
    /* Check if another packet is available */
    extra_packet_nb = (input_size >= acc_inst->_internal.accumulator_size) ?
                          (input_size / acc_inst->_internal.accumulator_size) :
                          0;
    /* Enqueue extra packet from the newest to oldest at head of the producer queue. */
    for (uint8_t i = extra_packet_nb; i > 0; i--) {
        node = queue_get_free_node(pipeline->producer->_internal.free_queue);
        if (node == NULL) {
            pipeline->_statistics.producer_buffer_overflow_count++;
            *status = SAC_WARN_PRODUCER_Q_FULL;
            input_size -= acc_inst->_internal.accumulator_size;
            continue;
        }
        /* Copy current packet header. */
        *sac_node_get_header(node) = *header;
        /* Set the node's payload size. */
        sac_node_set_payload_size(node, acc_inst->_internal.accumulator_size);
        /* Enqueue the extra packet at the head of the producer queue since this packet is older. */
        sac_node_data_memcpy(node, current_read_ptr + ((i - 1) * acc_inst->_internal.accumulator_size),
                             acc_inst->_internal.accumulator_size, status);
        if (*status != SAC_OK) {
            return 0;
        }
        queue_enqueue_at_head(pipeline->producer->_internal.queue, node);

        /* Track that a packet was enqueued so that we can ignore it once it enters this processing stage. */
        acc_inst->_internal.prod_enqueue_count++;
        pipeline->_internal.pending_packets++;

        /* Decrement the enqueued packet size from the input size. */
        input_size -= acc_inst->_internal.accumulator_size;
    }

    /* Copy leftover data in accumulator. */
    memcpy(&acc_inst->_internal.accumulator[acc_inst->_internal.accumulator_used_size], &data_in[size - input_size],
           input_size);
    acc_inst->_internal.accumulator_used_size += input_size;

    return ret;
}

uint16_t sac_sample_accumulator_flush(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in,
                                      uint16_t size, uint8_t *data_out, sac_status_t *status)
{
    (void)pipeline;
    (void)header;
    (void)data_in;
    (void)size;
    (void)data_out;

    sac_sample_accumulator_instance_t *acc_inst = instance;

    *status = SAC_OK;

    /* Flush accumulator. */
    acc_inst->_internal.accumulator_used_size = 0;

    return 0;
}
