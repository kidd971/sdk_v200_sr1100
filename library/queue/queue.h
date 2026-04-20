/** @file queue.h
 *  @brief Queue management.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUEUE_H_
#define QUEUE_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/*! Calculate number of bytes required for the queue. */
#define QUEUE_NB_BYTES_NEEDED(num_queues, num_nodes, data_size) \
    ((num_nodes) *                                              \
     (sizeof(queue_node_t) + data_size + (sizeof(queue_node_t *) * num_queues) + (sizeof(queue_t *) * num_queues)))

/* TYPES **********************************************************************/
/** @brief Node instance.
 */
typedef struct queue_node {
    /*! Pointer to the node’s payload. */
    uint8_t *data;
    /*! The free queue that owns this node. */
    struct queue *home_queue;
    /*! Pointer to the next node in the current queue. */
    struct queue_node **next;
    /*! Pointer to the list of queues in which this node currently resides. */
    struct queue **current_queue;
    /*! Number of bytes allocated for each node. */
    uint16_t data_size;
    /*! Number of times this node has been enqueued. This allows a node to be shared between different queues. It will
     *  not be returned to the home queue until all queues freed the node.
     */
    uint8_t copy_count;
    /*! Number of queues this node has been enqueued into. */
    uint8_t num_queues;
} queue_node_t;

/** @brief Queue instance.
 */
typedef struct queue {
    /*! Pointer to the first node in the queue. */
    queue_node_t *head;
    /*! Pointer to the last node in the queue. */
    queue_node_t *tail;
     /*! Current number of nodes in the queue. */
    uint16_t length;
    /*! Maximum number of nodes allowed in the queue. */
    uint16_t limit;
    /*! Indicates if this queue is a free queue type. */
    bool free_queue_type;
    /*! Pointer to the name of the queue. */
    const char *q_name;
    /*! Pointer to the previous queue in queues list. */
    struct queue *prev_queue;
} queue_t;

/** @brief Queue statistics instance.
 */
typedef struct queue_stats {
    /*! Current number of nodes in the queue. */
    uint16_t queue_length;
    /*! Maximum number of nodes allowed in the queue. */
    uint16_t queue_limit;
    /*! Pointer to the name of the queue. */
    char *queue_name;
    /*! Indicates if this queue is a free queue type. */
    bool queue_free_type;
} queue_stats_t;

/* MACROS *********************************************************************/
/** @brief Return pointer to data + offset for the specified node.
 *
 *  @param[in] _node    Specified node.
 *  @param[in] _offset  Offset added to the data pointer.
 */
#define queue_get_data_ptr(_node, _offset) (&(_node)->data[_offset])

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize queue management module.
 */
void queue_init(void);

/** @brief Initialize a new node pool.
 *
 *  @param[in] pool            Pool containing nodes and data.
 *  @param[in] new_free_queue  Queue where new nodes will be stored.
 *  @param[in] num_nodes       Number of nodes in this free pool.
 *  @param[in] data_size       Data size of each node.
 *  @param[in] num_queues      Total number of queues in the system.
 *  @param[in] queue_name      Queue name.
 *  @return Amount of memory consumed by this pool. This value rounded up to the nearest 4-byte multiple.
 */
uint32_t queue_init_pool(uint8_t *pool, queue_t *new_free_queue, uint16_t num_nodes, uint16_t data_size,
                         uint8_t num_queues, const char *queue_name);

/** @brief Initialize a new queue.
 *
 *  @param[in] queue   Queue to be initialized.
 *  @param[in] limit   Queue length limit.
 *  @param[in] q_name  Queue name.
 */
void queue_init_queue(queue_t *queue, uint16_t limit, const char *q_name);

/** @brief Get a free buffer from the queue.
 *
 *  @param[in] queue  Queue containing free nodes.
 *  @return Address of the free node, or NULL if queue is empty.
 */
queue_node_t *queue_get_free_node(queue_t *queue);

/** @brief Return node to its free queue.
 *
 *  @param[in] node  Node to be freed.
 */
void queue_free_node(queue_node_t *node);

/** @brief Get a node from a queue.
 *
 *  @param[in] queue  Desired queue.
 *  @return Address of the node.
 */
queue_node_t *queue_dequeue_node(queue_t *queue);

/** @brief Add a node to a queue.
 *
 *  @param[in] queue  Desired queue.
 *  @param[in] node   Address of the node.
 *  @return true if the node was successfully enqueued, false otherwise.
 */
bool queue_enqueue_node(queue_t *queue, queue_node_t *node);

/** @brief Replace the head of a queue.
 *
 *  @param[in] queue  Desired queue.
 *  @param[in] node   Address of the node.
 *  @return true if the head was successfully replaced, false otherwise.
 */
bool queue_enqueue_at_head(queue_t *queue, queue_node_t *node);

/** @brief Get the address of the head node without removing it from the queue.
 *
 *  @param[in] queue  Desired queue.
 *  @return Address of the node.
 */
queue_node_t *queue_get_node(queue_t *queue);

/** @brief Get the length of desired queue.
 *
 *  @param[in] queue  Desired queue.
 *  @return Length of the queue.
 */
uint16_t queue_get_length(queue_t *queue);

/** @brief Get the length limit of desired queue.
 *
 *  @param[in] queue  Desired queue.
 *  @return Limit of the queue.
 */
uint16_t queue_get_limit(queue_t *queue);

/** @brief Free any existing nodes in the queue.
 *
 *  @param[in] queue  Desired queue to flush.
 */
void queue_flush(queue_t *queue_to_flush);

/** @brief Unlink the queue from the linked list of queues.
 *
 *  @param[in] queue_to_flush  Desired queue.
 */
void queue_unlink(queue_t *queue);

/** @brief Get the queue statistics.
 *
 *  @param[in]  first        True for first queue stats, then false for the remaining.
 *  @param[out] queue_stats  Location for requested stats.
 *  @return True if valid stats returned, false if no more queues.
 */
bool queue_get_stats(bool first, queue_stats_t *queue_stats);

#ifdef __cplusplus
}
#endif

#endif /* QUEUE_H_ */
