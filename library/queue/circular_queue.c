/** @file circular_queue.c
 *  @brief Cross layer circular queue
 *
 *  @copyright Copyright (C) 2020-2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "circular_queue.h"
#include <stddef.h>
#include "critical_section.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void circular_queue_init(circular_queue_t *queue, void *buffer, uint32_t capacity, uint32_t size)
{
    queue->buffer_begin = (char *)buffer;
    queue->buffer_end   = (char *)buffer + capacity * size;
    queue->enqueue_it   = queue->buffer_begin;
    queue->dequeue_it   = queue->buffer_begin;
    queue->item_size    = size;
    queue->capacity     = capacity;
    queue->free_space   = capacity;
}

void *circular_queue_front_raw(circular_queue_t *queue)
{
    return queue->dequeue_it;
}

void *circular_queue_front(circular_queue_t *queue)
{
    void *ret;

    CRITICAL_SECTION_ENTER();
    if (circular_queue_is_empty((queue))) {
        ret = NULL;
    } else {
        ret = circular_queue_front_raw(queue);
    }
    CRITICAL_SECTION_EXIT();

    return ret;
}

void *circular_queue_get_free_slot_raw(circular_queue_t *queue)
{
    return queue->enqueue_it;
}

void *circular_queue_get_free_slot(circular_queue_t *queue)
{
    void *ret;

    CRITICAL_SECTION_ENTER();
    if (circular_queue_is_full(queue)) {
        ret = NULL;
    } else {
        ret = circular_queue_get_free_slot_raw(queue);
    }
    CRITICAL_SECTION_EXIT();

    return ret;
}

void circular_queue_enqueue_raw(circular_queue_t *queue)
{
    queue->enqueue_it = (void *)((char *)queue->enqueue_it + queue->item_size);

    if (queue->enqueue_it >= queue->buffer_end) {
        queue->enqueue_it = queue->buffer_begin;
    }
}

bool circular_queue_enqueue(circular_queue_t *queue)
{
    bool success = true;

    CRITICAL_SECTION_ENTER();
    if (!circular_queue_is_full(queue)) {
        queue->free_space -= 1;
        circular_queue_enqueue_raw(queue);
    } else {
        success = false;
    }
    CRITICAL_SECTION_EXIT();

    return success;
}

void circular_queue_dequeue_raw(circular_queue_t *queue)
{
    queue->dequeue_it = (void *)((char *)queue->dequeue_it + queue->item_size);

    if (queue->dequeue_it >= queue->buffer_end) {
        queue->dequeue_it = queue->buffer_begin;
    }
}

bool circular_queue_dequeue(circular_queue_t *queue)
{
    bool success = true;

    CRITICAL_SECTION_ENTER();
    if (!circular_queue_is_empty(queue)) {
        circular_queue_dequeue_raw(queue);
        queue->free_space += 1;
    } else {
        success = false;
    }
    CRITICAL_SECTION_EXIT();

    return success;
}

uint32_t circular_queue_size(circular_queue_t *queue)
{
    return queue->capacity - queue->free_space;
}

uint32_t circular_queue_capacity(circular_queue_t *queue)
{
    return queue->capacity;
}

uint32_t circular_queue_free_space(circular_queue_t *queue)
{
    return queue->free_space;
}

bool circular_queue_is_empty(circular_queue_t *queue)
{
    return (circular_queue_size(queue) == 0 ? true : false);
}

bool circular_queue_is_full(circular_queue_t *queue)
{
    return (queue->free_space == 0 ? true : false);
}
