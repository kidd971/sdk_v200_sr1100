/** @file  swc_cfg.h
 *  @brief Application specific configuration constants for the SPARK Wireless Core.
 *
 *  @copyright Copyright (C) 2025 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SWC_CFG_H_
#define SWC_CFG_H_

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/* Output power configuration */
#define TX_DATA_PULSE_COUNT 1
#define TX_DATA_PULSE_WIDTH 6
#define TX_DATA_PULSE_GAIN  1
#define TX_ACK_PULSE_COUNT  1
#define TX_ACK_PULSE_WIDTH  6
#define TX_ACK_PULSE_GAIN   1

/* Input power configuration */
#define RX_ACK_PULSE_COUNT  1
#define RX_DATA_PULSE_COUNT 1

/* SWC queue size */
#define TX_DATA_QUEUE_SIZE 2
#define RX_DATA_QUEUE_SIZE 2

/* Schedule configuration */
#define TIMESLOT_SIZE     1000
#define SCHEDULE          {TIMESLOT_SIZE}
#define TIMESLOTS         {MAIN_TIMESLOT(0)}

#define BIDIR_SCHEDULE    {TIMESLOT_SIZE, TIMESLOT_SIZE}
#define BIDIR_TIMESLOTS_0 {MAIN_TIMESLOT(0)}
#define BIDIR_TIMESLOTS_1 {MAIN_TIMESLOT(1)}

/* Channels */
#define CHANNEL_FREQ     {163, 171, 179, 187, 195}
#define CHANNEL_SEQUENCE {0, 1, 2, 3, 4}

/* Packet transmission frequency in us over the air, ensuring alignment with the radio's schedules. */
#define PACKET_GENERATION_FEQUENCY_US (TIMESLOT_SIZE / 2)

/* The maximum payload size depends on the radio type. */
#define PAYLOAD_SIZE_COUNT    8
#define MIN_PAYLOAD_SIZE_BYTE 2
#define MAX_PAYLOAD_SIZE_BYTE 250
/* Number of processing time measurements for each payload size. */
#define AVERAGE_LENGTH 1000
/* Maximum consecutive transmission fail on the last packet before considering that the other device disconnected. */
#define MAX_LAST_PKT_FAIL 4

/* Network parameters. */
#define COORDINATOR_ADDRESS 0x34
#define NODE_ADDRESS        0x12
#define PAN_ID              0x66

#ifdef __cplusplus
}
#endif

#endif /* SWC_CFG_H_ */
