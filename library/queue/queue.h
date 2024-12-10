/** @file queue.h
 *  @brief Queue management.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
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
#define QUEUE_LIMIT_UNLIMITED  0xffff
#define QUEUE_NB_BYTES_NEEDED(num_nodes, data_size) ((num_nodes) * ((sizeof(queue_node_t) + (data_size))))

/* TYPES **********************************************************************/
typedef struct queue_node {
    uint8_t           *data;
    struct queue      *home_queue;
    struct queue_node *next;
    /*! Number of bytes allocated for each node. */
    uint16_t data_size;
    /*! Number of times this node has been enqueued. This allows a node to be shared between different queues. It will
     *  not be returned to the home queue until all queues freed the node.
     */
    uint8_t copy_count;
} queue_node_t;

typedef struct queue {
    queue_node_t *head;
    queue_node_t *tail;
    uint16_t     length;
    uint16_t     limit;
    bool         free_queue_type;
    const char   *q_name;
    struct queue *prev_queue;
} queue_t;

typedef struct queue_stats {
    uint16_t queue_length;
    uint16_t queue_limit;
    char     *queue_name;
    bool     queue_free_type;
 } queue_stats_t;

typedef struct queue_critical_cfg {
    void (*enter_critical)(void);
    void (*exit_critical)(void);
} queue_critical_cfg_t;

/* MACROS *********************************************************************/
/** @brief Return pointer to data + offset for the specified node.
 *
 *  @param[in] _node    Specified node.
 *  @param[in] _offset  Offset added to the data pointer.
 */
#define queue_get_data_ptr(_node, _offset) (&(_node)->data[_offset])

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize queue management module.
 *
 *  @param[in] critical  Pointer to critical section enter and exit functions.
 */
void queue_init(queue_critical_cfg_t critical);

/** @brief Initialize a new node pool.
 *
 *  @param[in] pool            Pool containing nodes and data.
 *  @param[in] new_free_queue  Queue where new nodes will be stored.
 *  @param[in] num_nodes       Number of nodes in this free pool.
 *  @param[in] data_size       Data size of each node.
 *  @param[in] queue_name      Queue name.
 *  @return Amount of memory consumed by this pool. This value rounded up to the nearest 4-byte multiple.
 */
uint32_t queue_init_pool(uint8_t *pool, queue_t *new_free_queue, uint16_t num_nodes,
                         uint16_t data_size, const char *queue_name);

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
