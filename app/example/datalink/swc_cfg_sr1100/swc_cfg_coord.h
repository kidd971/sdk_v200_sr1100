/** @file  swc_cfg_coord.h
 *  @brief Application specific configuration constants for the SPARK Wireless Core.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SWC_CFG_COORD_H_
#define SWC_CFG_COORD_H_

/* CONSTANTS ******************************************************************/
#define NETWORK_ROLE SWC_ROLE_COORDINATOR

/* Output power configuration */
//3M
// #define TX_DATA_PULSE_COUNT 1  //1
// #define TX_DATA_PULSE_WIDTH 3  //7
// #define TX_DATA_PULSE_GAIN  5  //3

//5M
#define TX_DATA_PULSE_COUNT 1
#define TX_DATA_PULSE_WIDTH 3
#define TX_DATA_PULSE_GAIN  5

/* Input power configuration */
#define RX_ACK_PULSE_COUNT 1 /* Pulses configuration of received ACK frames */

/* SWC queue size */
#define TX_DATA_QUEUE_SIZE 2

/* Schedule configuration */
// clang-format off
//#define SCHEDULE     {250} // 200
#define SCHEDULE     {230}  // FR 4347

#define TX_TIMESLOTS {MAIN_TIMESLOT(0)}

/* Channels */
// #define CHANNEL_FREQ     {164, 174, 184, 194}
// #define CHANNEL_SEQUENCE {0, 1, 2, 3}
// 5 bands, spacing 10
#define CHANNEL_FREQ     {162, 172, 182, 192, 202}
#define CHANNEL_SEQUENCE {0, 1, 2, 3,4}


// clang-format on

/* CCA settings */
#define SWC_CCA_RETRY_TIME 512
#define SWC_CCA_THRESHOLD  25
#define SWC_CCA_TRY_COUNT  2

#endif /* SWC_CFG_COORD_H_ */
