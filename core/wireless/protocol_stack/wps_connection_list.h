/** @file wps_connection_list.h
 *  @brief wps_connection_list.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef WPS_CONNECTION_LIST_H_
#define WPS_CONNECTION_LIST_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Connection list node.
 */
typedef struct wps_connection_list_node {
    /*! Pointer to connection */
    void *connection;
    /*! Pointer to next node */
    struct wps_connection_list_node *next;
} wps_connection_list_node_t;

/** @brief Connection list.
 */
typedef struct wps_connection_list {
    /*! Pointer to head */
    wps_connection_list_node_t *head;
    /*! Pointer to tail */
    wps_connection_list_node_t *tail;
    /*! Queue size */
    uint16_t size;
} wps_connection_list_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize a new wps_connection_list.
 *
 *  @param[in] wps_connection_list  Queue to be initialized.
 */
void wps_connection_list_init(wps_connection_list_t *wps_connection_list);

/** @brief Initialize a new connection.
 *
 *  @param[out] list                  Queue.
 *  @param[in]  connection_list_node  Connection list node.
 *  @param[in]  connection            Connection.
 */
void wps_connection_list_append_conn(wps_connection_list_t *list, wps_connection_list_node_t *connection_list_node,
                                     void *connection);

/** @brief Iterrate through connections.
 *
 *  @param[in]  list      List.
 *  @param[in]  callback  Callback.
 *  @param[in]  arg       Argument.
 */
void wps_connection_list_iterate_connections(wps_connection_list_t *list,
                                             void (*callback)(wps_connection_list_node_t *, void *), void *arg);

/** @brief Get head of the list.
 *
 *  @param[in]  list  List.
 *  @return Head node of the list.
 */
wps_connection_list_node_t *wps_connection_list_get_head(wps_connection_list_t *list);

/** @brief Get next node.
 *
 *  @param[in]  current_node  Current node.
 *  @return Next node.
 */
wps_connection_list_node_t *wps_connection_list_get_next(wps_connection_list_node_t *current_node);

#ifdef __cplusplus
}
#endif

#endif /* WPS_CONNECTION_LIST_H_ */
