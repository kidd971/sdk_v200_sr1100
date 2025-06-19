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

/* The timeout in second after which the pairing procedure will abort. */
#define PAIRING_TIMEOUT_IN_SECONDS 10
/* The pairing device role is used for the coordinator's pairing discovery list. */
#define PAIRING_DEVICE_ROLE 1
/* The application code prevents unwanted devices from pairing with this application. */
#define PAIRING_APP_CODE 0x0000000000000333

/* Output power configuration */
#define TX_DATA_PULSE_COUNT 1
#define TX_DATA_PULSE_WIDTH 7
#define TX_DATA_PULSE_GAIN  1
#define TX_ACK_PULSE_COUNT  1
#define TX_ACK_PULSE_WIDTH  7
#define TX_ACK_PULSE_GAIN   0

/* Input power configuration */
#define RX_ACK_PULSE_COUNT  1 /* Pulses configuration of received ACK frames */
#define RX_DATA_PULSE_COUNT 1 /* Pulses configuration of received data frames */

/* SWC queue size */
#define TX_DATA_QUEUE_SIZE 3
#define RX_DATA_QUEUE_SIZE 3

/* Schedule configuration */
#define SCHEDULE     {500, 500}
#define RX_TIMESLOTS {MAIN_TIMESLOT(0)}
#define TX_TIMESLOTS {MAIN_TIMESLOT(1)}

/* Channels */
#define CHANNEL_FREQ     {163, 171, 179, 187, 195}
#define CHANNEL_SEQUENCE {0, 1, 2, 3, 4}

#endif /* SWC_CFG_NODE_H_ */
