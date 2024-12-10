/** @file  swc_cfg_node.h
 *  @brief Application specific configuration constants for the SPARK Wireless Core.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SWC_CFG_NODE_H_
#define SWC_CFG_NODE_H_

/* CONSTANTS ******************************************************************/
#define NETWORK_ROLE SWC_ROLE_NODE

/* Output power configuration */
#define TX_AUTO_REPLY_PULSE_COUNT 1
#define TX_AUTO_REPLY_PULSE_WIDTH 3
#define TX_AUTO_REPLY_PULSE_GAIN  3

/* Input power configuration */
#define RX_DATA_PULSE_COUNT 1 /* Pulses configuration of received data frames */

/* SWC queue size */
#define TX_DATA_QUEUE_SIZE 15
#define RX_DATA_QUEUE_SIZE 15

/* Schedule configuration */
#define SCHEDULE     {500}
#define TX_TIMESLOTS {AUTO_TIMESLOT(0)}
#define RX_TIMESLOTS {MAIN_TIMESLOT(0)}

/* Channels */
#define CHANNEL_FREQ     {164, 174, 184, 194}
#define CHANNEL_SEQUENCE {0, 1, 2, 3}

#endif /* SWC_CFG_NODE_H_ */