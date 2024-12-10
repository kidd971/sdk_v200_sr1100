/** @file  swc_cfg_coord.h
 *  @brief Coordinator application-specific configuration constants for the SPARK Wireless Core.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SWC_CFG_COORD_H_
#define SWC_CFG_COORD_H_

/* CONSTANTS ******************************************************************/
/* Specifies the device's network role as the Coordinator. */
#define NETWORK_ROLE SWC_ROLE_COORDINATOR
/* Define the PAN ID used for network identification. */
#define PAN_ID 0xBBB

/* The device roles are used for the pairing discovery list. */
#define DEVICE_ROLE_COORDINATOR 0
#define DEVICE_ROLE_NODE        1
/* The discovery list includes the Coordinator and the Node. */
#define PAIRING_DISCOVERY_LIST_SIZE 2
/* The application code prevents unwanted devices from pairing with this application. */
#define PAIRING_APP_CODE 0x0000000000000111
/* The timeout in seconds after which the pairing procedure will abort. */
#define PAIRING_TIMEOUT_IN_SECONDS 10

/* Sets the output power configuration for transmitting data and acknowledgement. */
#define TX_DATA_PULSE_COUNT 1
#define TX_DATA_PULSE_WIDTH 1
#define TX_DATA_PULSE_GAIN  1
#define TX_ACK_PULSE_COUNT  1
#define TX_ACK_PULSE_WIDTH  6
#define TX_ACK_PULSE_GAIN   0

/* Sets the offsets for output power configuration in fallback mode. */
#define TX_DATA_FB_PULSE_COUNT 1
#define TX_DATA_FB_PULSE_WIDTH 6
#define TX_DATA_FB_PULSE_GAIN  0

/* Sets the number of pulses for receiving data and acknowledgement. */
#define RX_DATA_PULSE_COUNT 1
#define RX_ACK_PULSE_COUNT  1

/* Defines the size of the SWC queues. */
#define TX_DATA_QUEUE_SIZE 2
#define RX_DATA_QUEUE_SIZE 2

/* Specifies the schedule configuration. */
// clang-format off
#define SCHEDULE                 \
    {                            \
        250, 250, 250, 250, 250, \
        250, 250, 250, 250,      \
    }
#define TX_TIMESLOTS                                                                          \
    {                                                                                         \
        MAIN_TIMESLOT(0), MAIN_TIMESLOT(1), MAIN_TIMESLOT(2),               MAIN_TIMESLOT(4), \
        MAIN_TIMESLOT(5), MAIN_TIMESLOT(6), MAIN_TIMESLOT(7),                                 \
    }

#define RX_TIMESLOTS                                                          \
    {                                                                         \
                                                            MAIN_TIMESLOT(3), \
                                                            MAIN_TIMESLOT(8), \
    }
// clang-format on

/* Defines the channels frequency band. */
#define CHANNEL_FREQ {164, 174, 184, 194}
#define CHANNEL_SEQUENCE {0, 1, 2, 3}

#endif /* SWC_CFG_COORD_H_ */
