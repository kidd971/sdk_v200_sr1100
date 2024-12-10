/** @file wps_connection_list.c
 *  @brief Connection list.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
/* INCLUDES *******************************************************************/
#include "wps_connection_list.h"

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void append_node(wps_connection_list_t *wps_connection_list, wps_connection_list_node_t *node);

/* PUBLIC FUNCTIONS ***********************************************************/
void wps_connection_list_init(wps_connection_list_t *wps_connection_list)
{
    /* Initialize new wps_connection_list */
    wps_connection_list->head = NULL;
    wps_connection_list->tail = NULL;
    wps_connection_list->size = 0;
}

void wps_connection_list_append_conn(wps_connection_list_t *list, wps_connection_list_node_t *connection_list_node,
                                     void *connection)
{
    connection_list_node->connection = connection;
    append_node(list, connection_list_node);
}

void wps_connection_list_iterate_connections(wps_connection_list_t *list,
                                             void (*callback)(wps_connection_list_node_t *, void *), void *arg)
{
    wps_connection_list_node_t *current = list->head;

    while (current != NULL) {
        callback(current, arg);
        current = current->next;
    }
}

wps_connection_list_node_t *wps_connection_list_get_head(wps_connection_list_t *list)
{
    return list->head;
}

wps_connection_list_node_t *wps_connection_list_get_next(wps_connection_list_node_t *current_node)
{
    return current_node->next;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Add a node to a wps_connection_list.
 *
 *  @param[in] wps_connection_list  Desired wps_connection_list.
 *  @param[in] node              Address of the node.
 */
static void append_node(wps_connection_list_t *wps_connection_list, wps_connection_list_node_t *node)
{
    if (node != NULL) { /* Prevent NULL node from being enlistd */
        if (wps_connection_list->size == 0) {
            /* The wps_connection_list is empty */
            wps_connection_list->head = node;
        } else {
            /* The wps_connection_list has nodes */
            wps_connection_list->tail->next = node;
        }
        wps_connection_list->tail = node;
        wps_connection_list->size++;
    }
}
