/** @file  swc_cfg_coord.h
 *  @brief Application specific configuration constants for the SPARK Wireless Core.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SWC_CFG_COORD_H_
#define SWC_CFG_COORD_H_

/* CONSTANTS ******************************************************************/
#define NETWORK_ROLE SWC_ROLE_COORDINATOR

/* Output power configuration */
#define TX_DATA_PULSE_COUNT 1
#define TX_DATA_PULSE_WIDTH 7
#define TX_DATA_PULSE_GAIN  0
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
#define TX_TIMESLOTS {MAIN_TIMESLOT(0)}
#define RX_TIMESLOTS {MAIN_TIMESLOT(1)}

/* Channels */
#define CHANNEL_FREQ     {164, 174, 184, 194}
#define CHANNEL_SEQUENCE {0, 1, 2, 3}

#endif /* SWC_CFG_COORD_H_ */
