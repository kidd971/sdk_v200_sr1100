/** @file queue.c
 *  @brief Queue management.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
/* INCLUDES *******************************************************************/
#include "queue.h"
#include <string.h>
#include "critical_section.h"

/* CONSTANTS ******************************************************************/
#define QUEUE_NOT_FOUND -1
#define QUEUE_ARR_FULL  -2

/* PRIVATE GLOBALS ************************************************************/
static queue_t *last_queue;

/* PRIVATE FUNCTION PROTOTYPE *************************************************/
static int8_t get_next_queue_index(queue_t *queue, queue_node_t *node);
static int8_t get_empty_next_queue_index(queue_node_t *node);

/* PUBLIC FUNCTIONS ***********************************************************/
void queue_init(void)
{
    last_queue = NULL;
}

uint32_t queue_init_pool(uint8_t *pool, queue_t *new_free_queue, uint16_t num_nodes, uint16_t data_size,
                         uint8_t num_queues, const char *queue_name)
{
    queue_node_t *node_ptr = (queue_node_t *)pool;
    uint8_t *data = (uint8_t *)pool + (sizeof(queue_node_t) * num_nodes);
    uint8_t *offset_ptr = (uint8_t *)data + (data_size * num_nodes);
    queue_node_t **next_ptr = (queue_node_t **)offset_ptr;

    offset_ptr += sizeof(queue_node_t *) * num_queues * num_nodes;
    queue_t **curr_queue_ptr = (queue_t **)offset_ptr;
    uint32_t num_bytes_used = 0;

    /* Initialize nodes. */
    for (uint16_t i = 0; i < num_nodes; i++) {
        node_ptr->next = next_ptr;
        node_ptr->current_queue = curr_queue_ptr;
        node_ptr->current_queue[0] = new_free_queue;
        node_ptr->next[0] = node_ptr + 1;
        for (uint8_t j = 1; j < num_queues; j++) {
            node_ptr->next[j] = NULL;
            node_ptr->current_queue[j] = NULL;
        }
        node_ptr->data = data;
        node_ptr->home_queue = new_free_queue;
        node_ptr->data_size = data_size;
        node_ptr->copy_count = 0;
        node_ptr->num_queues = num_queues;
        node_ptr++;
        data += data_size;
        next_ptr += num_queues;
        curr_queue_ptr += num_queues;
    }
    /* Initialize the free queue. */
    new_free_queue->head = (queue_node_t *)pool;
    new_free_queue->tail = new_free_queue->head + (num_nodes - 1);
    new_free_queue->tail->next[0] = NULL;
    new_free_queue->length = num_nodes;
    new_free_queue->limit = num_nodes;
    new_free_queue->q_name = queue_name;
    new_free_queue->free_queue_type = true;
    /* Add queue to queue list. */
    new_free_queue->prev_queue = last_queue;
    last_queue = new_free_queue;
    num_bytes_used = (num_nodes * ((sizeof(queue_node_t) + data_size + (sizeof(queue_node_t *) * num_queues) +
                                    (sizeof(queue_t *) * num_queues))));
    /* Round up the number of bytes used to nearest 4-byte multiple. */
    num_bytes_used = ((num_bytes_used + 3) & 0xfffffffc);
    return num_bytes_used;
}

void queue_init_queue(queue_t *queue, uint16_t limit, const char *q_name)
{
    /* Initialize new queue. */
    CRITICAL_SECTION_ENTER();
    queue->head = NULL;
    queue->tail = NULL;
    queue->length = 0;
    queue->limit = limit;
    queue->q_name = q_name;
    queue->free_queue_type = false;
    /* Add queue to queue list. */
    queue->prev_queue = last_queue;
    last_queue = queue;
    CRITICAL_SECTION_EXIT();
}

queue_node_t *queue_get_free_node(queue_t *queue)
{
    return (queue->free_queue_type) ? queue_dequeue_node(queue) : NULL;
}

void queue_free_node(queue_node_t *node)
{
    if (node != NULL) {
        CRITICAL_SECTION_ENTER();
        if (node->copy_count == 0) {
            if (get_next_queue_index(node->home_queue, node) < 0) {
                for (uint8_t i = 0; i < node->num_queues; i++) {
                    node->next[i] = NULL;
                    node->current_queue[i] = NULL;
                }
                /* Return node to home queue. */
                queue_enqueue_node(node->home_queue, node);
            }
        }
        CRITICAL_SECTION_EXIT();
    }
}

queue_node_t *queue_dequeue_node(queue_t *queue)
{
    queue_node_t *head = NULL;

    CRITICAL_SECTION_ENTER();
    if (queue->length == 0) {
        /* The queue is empty. */
    } else {
        int index = get_next_queue_index(queue, queue->head);

        if (index >= 0) {
            if (queue->length == 1) {
                /* The queue has one node. */
                head = queue->head;
                queue->head = queue->tail = NULL;
                queue->length--;
            } else {
                /* The queue has more than one node. */
                head = queue->head;
                queue->head = queue->head->next[index];
                queue->length--;
            }
            head->next[index] = NULL;
            head->current_queue[index] = NULL;
        }
    }
    if ((head != NULL) && (queue->free_queue_type == false)) {
        /* Only decrement copy_count when dequeueing from normal queues. */
        head->copy_count--;
    }
    CRITICAL_SECTION_EXIT();

    return head;
}

bool queue_enqueue_node(queue_t *queue, queue_node_t *node)
{
    bool ret = false;

    if (node != NULL) {
        /* Prevent NULL node from being enqueued. */
        CRITICAL_SECTION_ENTER();
        if (queue->length < queue->limit) {
            if (get_next_queue_index(queue, node) < 0) {
                /* Prevent from enqueuing the same node twice. */
                int index = get_empty_next_queue_index(node);

                if (index >= 0) {
                    node->current_queue[index] = queue;
                    node->next[index] = NULL;
                    if (queue->length == 0) {
                        /* The queue is empty. */
                        queue->head = node;
                    } else {
                        /* The queue has nodes. */
                        int tail_index = get_next_queue_index(queue, queue->tail);

                        queue->tail->next[tail_index] = node;
                    }
                    queue->tail = node;
                    queue->length++;
                    ret = true;
                    if (queue->free_queue_type == false) {
                        /* Node enqueued in a normal queue. */
                        node->copy_count++;
                    }
                }
            }
        }
        CRITICAL_SECTION_EXIT();
    }
    return ret;
}

bool queue_enqueue_at_head(queue_t *queue, queue_node_t *node)
{
    bool ret = false;

    if (node != NULL) {
        /* Prevent NULL node from being enqueued. */
        CRITICAL_SECTION_ENTER();
        if (queue->length < queue->limit) {
            if (get_next_queue_index(queue, node) < 0) {
                /* Prevent from enqueuing the same node twice. */
                int index = get_empty_next_queue_index(node);

                if (index >= 0) {
                    node->current_queue[index] = queue;
                    node->next[index] = NULL;
                    if (queue->length == 0) {
                        /* The queue is empty. */
                        queue->head = node;
                        queue->tail = node;
                    } else {
                        /* The queue has nodes. */
                        node->next[index] = queue->head;
                        queue->head = node;
                    }
                    queue->length++;
                    ret = true;
                    if (queue->free_queue_type == false) {
                        /* Node enqueued in a normal queue. */
                        node->copy_count++;
                    }
                }
            }
        }
        CRITICAL_SECTION_EXIT();
    }
    return ret;
}

queue_node_t *queue_get_node(queue_t *queue)
{
    return (queue->free_queue_type || (queue->length == 0)) ? NULL : queue->head;
}

uint16_t queue_get_length(queue_t *queue)
{
    return (queue == NULL) ? 0 : queue->length;
}

uint16_t queue_get_limit(queue_t *queue)
{
    return (queue == NULL) ? 0 : queue->limit;
}

void queue_flush(queue_t *queue_to_flush)
{
    queue_node_t *node = NULL;

    /* Cannot flush free queues. */
    if (!queue_to_flush->free_queue_type) {
        CRITICAL_SECTION_ENTER();
        if ((queue_to_flush == NULL) || (queue_to_flush->length == 0)) {
            /* Ignore if queue_to_flush invalid or empty. */
        } else {
            /* free each node. */
            node = queue_dequeue_node(queue_to_flush);
            while (node != NULL) {
                queue_free_node(node);
                node = queue_dequeue_node(queue_to_flush);
            }
        }
        CRITICAL_SECTION_EXIT();
    }
}

void queue_unlink(queue_t *queue_to_unlink)
{
    queue_t *q_ptr = last_queue;
    queue_t *prev_qptr = last_queue;

    /* Cannot unlink free queues. */
    if (!queue_to_unlink->free_queue_type) {
        CRITICAL_SECTION_ENTER();
        /* Starting at last_queue, look for the queue in the chain. */
        while ((q_ptr != queue_to_unlink) && (q_ptr != NULL)) {
            prev_qptr = q_ptr;
            q_ptr = q_ptr->prev_queue;
        }
        /* Make sure queue was found. */
        if (q_ptr != NULL) {
            if (q_ptr == last_queue) {
                /* If it's the last queue, just update last_queue. */
                last_queue = q_ptr->prev_queue;
            } else {
                /* Otherwise, remove this queue from the chain. */
                prev_qptr->prev_queue = q_ptr->prev_queue;
            }
        }
        CRITICAL_SECTION_EXIT();
    }
}

bool queue_get_stats(bool first, queue_stats_t *queue_stats)
{
    static queue_t *q_ptr;
    bool ret = false;

    if (first) {
        /* First, so point to the last queue. */
        q_ptr = last_queue;
    } else {
        /* Next, point to previous one. */
        if (q_ptr != NULL) {
            q_ptr = q_ptr->prev_queue;
        }
    }
    if (q_ptr != NULL) {
        queue_stats->queue_name = (char *)q_ptr->q_name;
        queue_stats->queue_length = q_ptr->length;
        queue_stats->queue_limit = q_ptr->limit;
        queue_stats->queue_free_type = q_ptr->free_queue_type;
        ret = true;
    }
    return ret;
}

/* PRIVATE FUNCTIONS ***********************************************************/
/** @brief Gets the index of the queue in the array of queues in the node's instance.
 *
 *  @param[in] queue  Desired queue.
 *  @param[in] node   Address of the node.
 *  @return -1 if queue not found, or index of the queue in the array.
 */
static int8_t get_next_queue_index(queue_t *queue, queue_node_t *node)
{
    if (node != NULL) {
        for (uint8_t i = 0; i < node->num_queues; i++) {
            if (node->current_queue[i] == queue) {
                return i;
            }
        }
    }
    /* Not found. */
    return QUEUE_NOT_FOUND;
}

/** @brief Gets the index of the first empty slot in the queue array in the node's instance.
 *
 *  @param[in] node  Address of the node.
 *  @return -2 if the array is full, or index of the queue in the array.
 */
static int8_t get_empty_next_queue_index(queue_node_t *node)
{
    if (node != NULL) {
        for (uint8_t i = 0; i < node->num_queues; i++) {
            if (node->current_queue[i] == NULL) {
                return i;
            }
        }
    }
    /* No empty slots node cannot be enqueued. */
    return QUEUE_ARR_FULL;
}
