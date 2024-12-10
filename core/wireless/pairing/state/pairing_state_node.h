/** @file  pairing_state_node.h
 *  @brief This file handles the function related to the node's pairing states.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef PAIRING_STATE_NODE_H_
#define PAIRING_STATE_NODE_H_

/* INCLUDES *******************************************************************/
#include "pairing_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC GLOBALS *************************************************************/
/** @brief Initialize the node state machine.
 */
void pairing_state_node_init(void);

/** @brief Callback function when a message is successfully sent.
 */
void sent_message_node_callback(void);

/** @brief Callback function when a message is received.
 *
 *  @param[in] received_message  The received message.
 *  @param[in] message_size      The received message size.
 */
void received_message_node_callback(uint8_t *received_message, uint8_t message_size);

#ifdef __cplusplus
}
#endif

#endif /* PAIRING_STATE_NODE_H_ */
