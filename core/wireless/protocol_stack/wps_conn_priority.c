/** @file  wps_conn_priority.h
 *  @brief Wireless Protocol Stack connection priority module.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "wps_mac.h"

/* CONSTANTS ******************************************************************/
/** @brief Number of extra points added to `notify_missed_credits_count` value for connection with the highest priority.
 */
#define NOTIFY_MISSED_CREDITS_HIGH_CONN_EXTRA_POINTS 3

/** @brief The connection priority ID to know when the highest basic connection priority should be returned.
 */
#define USE_HIGHEST_CONNECTION_PRIORITY  0xFF

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static uint8_t get_highest_conn_index_based_on_priority(wps_connection_t **connections, const uint8_t *connection_priorities,
                                                        uint8_t connection_count);

static uint8_t get_highest_main_conn_index_based_on_priority_and_credits(wps_connection_t **connections,
                                                                         const uint8_t *connection_priorities,
                                                                         uint8_t connection_count,
                                                                         uint8_t depth);

static uint8_t get_highest_auto_conn_index_based_on_priority_and_credits(wps_connection_t **connections,
                                                                         const uint8_t *connection_priorities,
                                                                         uint8_t connection_count);

/* PUBLIC FUNCTIONS ***********************************************************/
uint8_t wps_conn_priority_get_highest_main_conn_index(wps_connection_t **connections, const uint8_t *connection_priorities,
                                                      uint8_t connection_count)
{
    wps_connection_t *first_connection = connections[0];

    if (first_connection->credit_flow_ctrl.enabled == false) {
        return get_highest_conn_index_based_on_priority(connections, connection_priorities, connection_count);
    } else {
        uint8_t high_priority_id = get_highest_main_conn_index_based_on_priority_and_credits(connections, connection_priorities,
                                                                                             connection_count, connection_count - 1);

        if (high_priority_id == USE_HIGHEST_CONNECTION_PRIORITY) {
            high_priority_id = get_highest_conn_index_based_on_priority(connections, connection_priorities, connection_count);
        }

        return high_priority_id;
    }
}

uint8_t wps_conn_priority_get_highest_auto_conn_index(wps_connection_t **connections, const uint8_t *connection_priorities,
                                                      uint8_t connection_count)
{
    wps_connection_t *first_connection = connections[0];

    if (first_connection->credit_flow_ctrl.enabled == false) {
        return get_highest_conn_index_based_on_priority(connections, connection_priorities, connection_count);
    } else {
        return get_highest_auto_conn_index_based_on_priority_and_credits(connections, connection_priorities,
                                                                         connection_count);
    }
}

/* PRIVATE STATE FUNCTIONS ****************************************************/
/** @brief Get the index of the highest priority connection.
 *
 *  @param[in] connections            Connection table.
 *  @param[in] connection_priorities  Connection priorities table.
 *  @param[in] connection_count       Connection count.
 *  @return Connection index with the highest priority.
 */
static uint8_t get_highest_conn_index_based_on_priority(wps_connection_t **connections, const uint8_t *connection_priorities,
                                                        uint8_t connection_count)
{
    xlayer_t *free_xlayer;
    xlayer_queue_node_t *node;
    uint8_t min_prio  = WPS_MAX_CONN_PRIORITY + 1;
    uint8_t min_index = 0;

    for (uint8_t i = 0; i < connection_count; i++) {
        if (connections[i]->currently_enabled) {
            node = xlayer_queue_get_node(&connections[i]->xlayer_queue);
        } else {
            node = NULL;
        }
        if (node == NULL) {
            free_xlayer = NULL;
        } else {
            free_xlayer = &node->xlayer;
        }
        if (free_xlayer != NULL) {
            if (connection_priorities[i] < min_prio) {
                min_prio = connection_priorities[i];
                min_index = i;
            }
            if (min_prio == 0) {
                break;
            }
        }
    }

    return min_index;
}

/** @brief Get the index of the highest priority for main connection base on priority order and credits information.
 *
 *  @param[in] connections            Connection table.
 *  @param[in] connection_priorities  Connection priorities table.
 *  @param[in] connection_count       Connection count.
 *  @param[in] depth                  Recursive search depth
 *  @return Main connection index with the highest priority.
 */
static uint8_t get_highest_main_conn_index_based_on_priority_and_credits(wps_connection_t **connections,
                                                                         const uint8_t *connection_priorities,
                                                                         uint8_t connection_count,
                                                                         uint8_t depth)
{
    uint8_t high_priority_conn_id = get_highest_conn_index_based_on_priority(connections, connection_priorities,
                                                                             connection_count);
    wps_connection_t *wps_conn = connections[high_priority_conn_id];

    if (wps_conn->credit_flow_ctrl.credits_count > 0) {
        return high_priority_conn_id;
    } else if (wps_conn->credit_flow_ctrl.skipped_frames_count >= CREDIT_FLOW_CTRL_SKIPPED_FRAMES_THRESHOLD) {
        return high_priority_conn_id;
    } else if (depth > 0) {
        if (wps_conn->credit_flow_ctrl.skipped_frames_count < UINT8_MAX) {
            wps_conn->credit_flow_ctrl.skipped_frames_count++;
        }

        uint8_t new_connection_priorities[WPS_MAX_CONN_PER_TIMESLOT];

        memcpy(new_connection_priorities, connection_priorities, sizeof(new_connection_priorities));
        /* Use a different connection, `high_priority_conn_id` connection will not be taken into account. */
        new_connection_priorities[high_priority_conn_id] = WPS_MAX_CONN_PRIORITY + 1;
        high_priority_conn_id = get_highest_main_conn_index_based_on_priority_and_credits(connections, new_connection_priorities,
                                                                                          connection_count, depth - 1);

        return high_priority_conn_id;
    } else {
        if (wps_conn->credit_flow_ctrl.skipped_frames_count < UINT8_MAX) {
            wps_conn->credit_flow_ctrl.skipped_frames_count++;
        }

        return USE_HIGHEST_CONNECTION_PRIORITY;
    }
}

/** @brief Get the index of the highest priority for auto-reply connection base on priority order and credits
 *         information. The main goal is to select the oldest connection that sent credit information.
 *
 *  @param[in] connections            Connection table.
 *  @param[in] connection_priorities  Connection priorities table.
 *  @param[in] connection_count       Connection count.
 *  @return Auto reply connection index with the highest priority.
 */
static uint8_t get_highest_auto_conn_index_based_on_priority_and_credits(wps_connection_t **connections,
                                                                         const uint8_t *connection_priorities,
                                                                         uint8_t connection_count)
{
    uint8_t max_notify_missed_credits_count = 0;
    uint8_t high_notify_conn_id = 0;
    uint8_t high_priority_conn_id = get_highest_conn_index_based_on_priority(connections, connection_priorities,
                                                                             connection_count);

    for (uint8_t idx = 0; idx < connection_count; idx++) {
        if (connections[idx]->currently_enabled) {
            connections[idx]->credit_flow_ctrl.notify_missed_credits_count++;
        }
    }

    /* The highest priority connection only gets extra points if it has some data to transmit */
    if (xlayer_queue_get_node(&connections[high_priority_conn_id]->xlayer_queue) != NULL) {
        connections[high_priority_conn_id]->credit_flow_ctrl.notify_missed_credits_count +=
            NOTIFY_MISSED_CREDITS_HIGH_CONN_EXTRA_POINTS;
    }

    /* Find connection with highest `notify_missed_credits_count` value */
    for (uint8_t idx = 0; idx < connection_count; idx++) {
        if (connections[idx]->currently_enabled) {
            if (connections[idx]->credit_flow_ctrl.notify_missed_credits_count > max_notify_missed_credits_count) {
                max_notify_missed_credits_count = connections[idx]->credit_flow_ctrl.notify_missed_credits_count;
                high_notify_conn_id = idx;
            }
        }
    }

    return high_notify_conn_id;
}
