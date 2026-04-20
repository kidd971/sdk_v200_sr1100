/** @file circular_queue.h
 *  @brief Circular queue functions.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

#ifndef CIRCULAR_QUEUE_H
#define CIRCULAR_QUEUE_H

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Structure for circular queue.
 */
typedef struct circular_queue {
    /* Iterators */
    void *enqueue_it; /*!< Enqueue iterator */
    void *dequeue_it; /*!< Dequeue iterator */

    /* Buffer */
    void *buffer_begin; /*!< Buffer begin iterator */
    void *buffer_end;   /*!< Buffer end iterator   */
    uint32_t item_size;
    uint32_t capacity;   /*!< Buffer's capacity     */
    uint32_t free_space; /*!< Buffer's free space   */
} circular_queue_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Circular queue initialization.
 *
 *  Assign circular queue's iterators for each frame.
 *
 *  @param[in] queue    Circular queue instance.
 *  @param[in] buffer   Circular queue buffer.
 *  @param[in] capacity Circular queue buffer size.
 *  @param[in] size     Size in bytes of the data element stored in the circular queue buffer.
 */
void circular_queue_init(circular_queue_t *queue, void *buffer, uint32_t capacity, uint32_t size);

/** @brief Circular queue front.
 *
 *  @param[in] queue  Circular queue instance.
 *  @return Buffer's front (oldest value). If queue is empty, return NULL.
 */
void *circular_queue_front(circular_queue_t *queue);

/** @brief Circular queue front without any fail check.
 *
 *  @param[in] queue  Circular queue instance.
 *  @return Buffer's front (oldest value).
 */
void *circular_queue_front_raw(circular_queue_t *queue);

/** @brief Circular queue free slot.
 *
 *  @param[in] queue  Circular queue instance.
 *  @return The next slot to be enqueue. If no slot is free, return NULL.
 */
void *circular_queue_get_free_slot(circular_queue_t *queue);

/** @brief Circular queue free slot without any fail check.
 *
 *  @param[in] queue  Circular queue instance.
 *  @return The next slot to be enqueue.
 */
void *circular_queue_get_free_slot_raw(circular_queue_t *queue);

/** @brief Circular queue enqueue.
 *
 *  @param[in] queue  Circular queue instance.
 *  @retval True  Slot has been successfully enqueue.
 *  @retval False Queue is full, slot has not been enqueue.
 */
bool circular_queue_enqueue(circular_queue_t *queue);

/** @brief Circular queue enqueue without any fail check.
 *
 *  @param[in] queue  Circular queue instance.
 */
void circular_queue_enqueue_raw(circular_queue_t *queue);

/** @brief Circular queue dequeue.
 *
 *  @param[in] queue  Circular queue instance.
 *  @retval True  Slot has been successfully dequeue.
 *  @retval False Queue is empty, nothing to dequeue.
 */
bool circular_queue_dequeue(circular_queue_t *queue);

/** @brief Circular queue dequeue without any fail check.
 *
 *  @param[in] queue  Circular queue instance.
 */
void circular_queue_dequeue_raw(circular_queue_t *queue);

/** @brief Circular queue size.
 *
 *  @param[in] queue  Circular queue instance.
 *  @return Queue size.
 */
uint32_t circular_queue_size(circular_queue_t *queue);

/** @brief Circular queue capacity.
 *
 *  @param[in] queue  Circular queue instance.
 *  @return Queue capacity.
 */
uint32_t circular_queue_capacity(circular_queue_t *queue);

/** @brief Circular queue free space.
 *
 *   @param[in] queue Circular queue instance.
 *   @return Number of free elements in the queue.
 */
uint32_t circular_queue_free_space(circular_queue_t *queue);

/** @brief Circular queue empty?
 *
 *  @param[in] queue  Circular queue instance.
 *  @retval true   Queue is empty.
 *  @retval false  Queue is not empty.
 */
bool circular_queue_is_empty(circular_queue_t *queue);

/** @brief Circular queue full?
 *
 *  @param[in] queue  Circular queue instance.
 *  @retval true   Queue is full.
 *  @retval false  Queue is not full.
 */
bool circular_queue_is_full(circular_queue_t *queue);

#ifdef __cplusplus
}
#endif
#endif  // CIRCULAR_QUEUE_H
