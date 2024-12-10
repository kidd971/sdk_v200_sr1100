/** @file xlayer_queue.c
 *  @brief Queue management.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
/* INCLUDES *******************************************************************/
#include "xlayer_queue.h"
#include "critical_section.h"
#include <string.h>

/* PRIVATE GLOBALS ************************************************************/
static xlayer_queue_t *last_xlayer_queue;

/* PUBLIC FUNCTIONS ***********************************************************/
void xlayer_queue_init_pool(uint8_t *pool, xlayer_queue_t *new_free_xlayer_queue, uint16_t num_nodes,
                            const char *xlayer_queue_name)
{
    xlayer_queue_node_t *node_ptr = (xlayer_queue_node_t *)pool;

    last_xlayer_queue = NULL;

    /* Initialize nodes */
    for (uint16_t i = 0; i < num_nodes; i++) {
        node_ptr->next = node_ptr + 1;
        node_ptr->home_xlayer_queue = new_free_xlayer_queue;
        node_ptr->copy_count = 1;
        node_ptr->xlayer.frame.source_address = 0;
        node_ptr->xlayer.frame.destination_address = 0;
        node_ptr->xlayer.frame.time_stamp = 0;
        node_ptr->xlayer.frame.retry_count = 0;
        node_ptr->xlayer.frame.max_frame_size = 0;
        node_ptr->xlayer.frame.user_payload = false;

        node_ptr++;
    }

    /* Initialize the free xlayer_queue */
    new_free_xlayer_queue->head = (xlayer_queue_node_t *)pool;
    new_free_xlayer_queue->tail = new_free_xlayer_queue->head + (num_nodes - 1);
    new_free_xlayer_queue->tail->next = NULL;
    new_free_xlayer_queue->size = num_nodes;
    new_free_xlayer_queue->max_size = num_nodes;
    new_free_xlayer_queue->q_name = xlayer_queue_name;
    new_free_xlayer_queue->free_xlayer_queue_type = true;

    /* Add xlayer_queue to xlayer_queue list */
    new_free_xlayer_queue->prev_xlayer_queue = last_xlayer_queue;
    last_xlayer_queue = new_free_xlayer_queue;
}

void xlayer_queue_init_pool_with_header_data(uint8_t *pool, xlayer_queue_t *new_free_xlayer_queue, uint16_t num_nodes,
                                             uint16_t data_size, const char *xlayer_queue_name)
{
    xlayer_queue_node_t *node_ptr = (xlayer_queue_node_t *)pool;
    uint8_t *data;

    last_xlayer_queue = NULL;
    data = (uint8_t *)pool + (sizeof(xlayer_queue_node_t) * num_nodes);

    /* Initialize nodes */
    for (uint16_t i = 0; i < num_nodes; i++) {
        node_ptr->next = node_ptr + 1;
        node_ptr->data = data;
        node_ptr->home_xlayer_queue = new_free_xlayer_queue;
        node_ptr->copy_count = 1;
        node_ptr->xlayer.frame.source_address = 0;
        node_ptr->xlayer.frame.destination_address = 0;
        node_ptr->xlayer.frame.time_stamp = 0;
        node_ptr->xlayer.frame.retry_count = 0;
        node_ptr->xlayer.frame.header_memory_size = data_size;
        node_ptr->xlayer.frame.header_memory = data;
        node_ptr->xlayer.frame.user_payload = false;
        node_ptr++;
        data += data_size;
    }
    /* Initialize the free xlayer_queue */
    new_free_xlayer_queue->head = (xlayer_queue_node_t *)pool;
    new_free_xlayer_queue->tail = new_free_xlayer_queue->head + (num_nodes - 1);
    new_free_xlayer_queue->tail->next = NULL;
    new_free_xlayer_queue->size = num_nodes;
    new_free_xlayer_queue->max_size = num_nodes;
    new_free_xlayer_queue->q_name = xlayer_queue_name;
    new_free_xlayer_queue->free_xlayer_queue_type = true;
    /* Add xlayer_queue to xlayer_queue list */
    new_free_xlayer_queue->prev_xlayer_queue = last_xlayer_queue;
    last_xlayer_queue = new_free_xlayer_queue;
}

void xlayer_queue_init_queue(xlayer_queue_t *xlayer_queue, uint16_t max_size, const char *q_name)
{
    /* Initialize new xlayer_queue */
    xlayer_queue->head   = NULL;
    xlayer_queue->tail   = NULL;
    xlayer_queue->size = 0;
    xlayer_queue->max_size  = max_size;
    xlayer_queue->q_name = q_name;
    xlayer_queue->free_xlayer_queue_type = false;
    /* Add xlayer_queue to xlayer_queue list */
    xlayer_queue->prev_xlayer_queue = last_xlayer_queue;
    last_xlayer_queue = xlayer_queue;
}

xlayer_queue_node_t *xlayer_queue_get_free_node(xlayer_queue_t *xlayer_queue)
{
    return (xlayer_queue->free_xlayer_queue_type) ? xlayer_queue_dequeue_node(xlayer_queue) : NULL;
}

void xlayer_queue_free_node(xlayer_queue_node_t *node)
{
    if (node != NULL) {
        CRITICAL_SECTION_ENTER();
        if (node->copy_count == 1) {
            CRITICAL_SECTION_EXIT();
            xlayer_queue_enqueue_node(node->home_xlayer_queue, node);
        } else {
            node->copy_count--;
            CRITICAL_SECTION_EXIT();
        }
    }
}

xlayer_queue_node_t *xlayer_queue_dequeue_node(xlayer_queue_t *xlayer_queue)
{
    xlayer_queue_node_t *head = NULL;

    CRITICAL_SECTION_ENTER();
    if (xlayer_queue->size == 0) {
        /* The xlayer_queue is empty */
    } else if (xlayer_queue->size == 1) {
        /* The xlayer_queue has one node */
        head = xlayer_queue->head;
        xlayer_queue->size--;
    } else {
        /* The xlayer_queue has more than one node */
        head = xlayer_queue->head;
        xlayer_queue->head = xlayer_queue->head->next;
        xlayer_queue->size--;
    }
    CRITICAL_SECTION_EXIT();

    return head;
}

bool xlayer_queue_enqueue_node(xlayer_queue_t *xlayer_queue, xlayer_queue_node_t *node)
{
    bool ret = false;

    if (node != NULL) { /* Prevent NULL node from being enqueued */
        CRITICAL_SECTION_ENTER();
        if (xlayer_queue->size < xlayer_queue->max_size) {
            if (xlayer_queue->size == 0) {
                /* The xlayer_queue is empty */
                xlayer_queue->head = node;
            } else {
                /* The xlayer_queue has nodes */
                xlayer_queue->tail->next = node;
            }
            xlayer_queue->tail = node;
            xlayer_queue->size++;
            ret = true;
        }
        CRITICAL_SECTION_EXIT();
    }
    return ret;
}

bool xlayer_queue_enqueue_at_head(xlayer_queue_t *xlayer_queue, xlayer_queue_node_t *node)
{
    bool ret = false;

    if (node != NULL) { /* Prevent NULL node from being enqueued */
        CRITICAL_SECTION_ENTER();
        if (xlayer_queue->size < xlayer_queue->max_size) {
            if (xlayer_queue->size == 0) {
                /* The xlayer_queue is empty */
                xlayer_queue->head = node;
            } else {
                /* The xlayer_queue has nodes */
                node->next = xlayer_queue->head;
                xlayer_queue->head = node;
            }
            xlayer_queue->size++;
            ret = true;
        }
        CRITICAL_SECTION_EXIT();
    }
    return ret;
}

xlayer_queue_node_t *xlayer_queue_get_node(xlayer_queue_t *xlayer_queue)
{
    return (xlayer_queue->free_xlayer_queue_type || (xlayer_queue->size == 0)) ? NULL : xlayer_queue->head;
}

uint16_t xlayer_queue_get_size(xlayer_queue_t *xlayer_queue)
{
    return (xlayer_queue == NULL) ? 0 : xlayer_queue->size;
}

uint16_t xlayer_queue_get_max_size(xlayer_queue_t *xlayer_queue)
{
    return (xlayer_queue == NULL) ? 0 : xlayer_queue->max_size;
}

uint16_t xlayer_queue_get_free_space(xlayer_queue_t *xlayer_queue)
{
    return xlayer_queue_get_max_size(xlayer_queue) - xlayer_queue_get_size(xlayer_queue);
}

void xlayer_queue_flush(xlayer_queue_t *xlayer_queue_to_flush)
{
    xlayer_queue_node_t *node;

    /* Cannot flush free xlayer_queues */
    if (!xlayer_queue_to_flush->free_xlayer_queue_type) {
        CRITICAL_SECTION_ENTER();
        if ((xlayer_queue_to_flush == NULL) || (xlayer_queue_to_flush->size == 0)) {
            /* Ignore if xlayer_queue_to_flush invalid or empty */
        } else {
            /* free each node */
            node = xlayer_queue_dequeue_node(xlayer_queue_to_flush);
            while (node != NULL) {
                xlayer_queue_free_node(node);
                node = xlayer_queue_dequeue_node(xlayer_queue_to_flush);
            }
        }
        CRITICAL_SECTION_EXIT();
    }
}

void xlayer_queue_unlink(xlayer_queue_t *xlayer_queue_to_unlink)
{
    xlayer_queue_t *q_ptr = last_xlayer_queue;
    xlayer_queue_t *prev_qptr = last_xlayer_queue;

    /* Cannot unlink free xlayer_queues */
    if (!xlayer_queue_to_unlink->free_xlayer_queue_type) {
        CRITICAL_SECTION_ENTER();
        /* Starting at last_xlayer_queue, look for the xlayer_queue in the chain */
        while ((q_ptr != xlayer_queue_to_unlink) && (q_ptr != NULL)) {
            prev_qptr = q_ptr;
            q_ptr = q_ptr->prev_xlayer_queue;
        }
        /* Make sure xlayer_queue was found */
        if (q_ptr != NULL) {
            if (q_ptr == last_xlayer_queue) {
                /* If it's the last xlayer_queue, just update last_xlayer_queue */
                last_xlayer_queue = q_ptr->prev_xlayer_queue;
            } else {
                /* Otherwise, remove this xlayer_queue from the chain */
                prev_qptr->prev_xlayer_queue = q_ptr->prev_xlayer_queue;
            }
        }
        CRITICAL_SECTION_EXIT();
    }
}

bool xlayer_queue_get_stats(bool first, xlayer_queue_stats_t *xlayer_queue_stats)
{
    static xlayer_queue_t *q_ptr;
    bool ret = false;

    if (first) {
        /* First, so point to the last xlayer_queue */
        q_ptr = last_xlayer_queue;
    } else {
        /* Next, point to previous one */
        if (q_ptr != NULL) {
            q_ptr = q_ptr->prev_xlayer_queue;
        }
    }
    if (q_ptr != NULL) {
        xlayer_queue_stats->xlayer_queue_name      = (char *)q_ptr->q_name;
        xlayer_queue_stats->xlayer_queue_size      = q_ptr->size;
        xlayer_queue_stats->xlayer_queue_max_size  = q_ptr->max_size;
        xlayer_queue_stats->xlayer_queue_free_type = q_ptr->free_xlayer_queue_type;
        ret = true;
    }
    return ret;
}

void xlayer_queue_inc_copy_count(xlayer_queue_node_t *node)
{
    CRITICAL_SECTION_ENTER();
    node->copy_count++;
    CRITICAL_SECTION_EXIT();
}

void xlayer_queue_set_tx_frame_buffer(xlayer_frame_t *frame, uint8_t header_size, uint8_t *buffer)
{
    frame->header_begin_it = &buffer[header_size + XLAYER_QUEUE_SPI_COMM_ADDITIONAL_BYTES - 1];
    frame->payload_begin_it = &buffer[header_size + XLAYER_QUEUE_SPI_COMM_ADDITIONAL_BYTES];
    frame->header_end_it = frame->header_begin_it;
    frame->header_memory = buffer;
}
