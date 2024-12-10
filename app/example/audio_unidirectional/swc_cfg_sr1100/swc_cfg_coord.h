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

/* The pairing device roles are used for the Coordinator's pairing discovery list. */
#define DEVICE_ROLE_COORDINATOR 0
#define DEVICE_ROLE_NODE        1
/* The discovery list includes the Coordinator and the Node. */
#define PAIRING_DISCOVERY_LIST_SIZE 2
/* The application code prevents unwanted devices from pairing with this application. */
#define PAIRING_APP_CODE 0x0000000000000222
/* Specifies the timeout in seconds after which the pairing procedure will abort. */
#define PAIRING_TIMEOUT_IN_SECONDS 10

/* Sets the output power configuration for transmitting data. */
#define TX_DATA_PULSE_COUNT 1
#define TX_DATA_PULSE_WIDTH 5
#define TX_DATA_PULSE_GAIN  4

/* Sets the offsets for output power configuration in fallback mode. */
#define TX_DATA_FB_PULSE_COUNT 1
#define TX_DATA_FB_PULSE_WIDTH 6
#define TX_DATA_FB_PULSE_GAIN  4

/* Sets the number of pulses for receiving acknowledgement data. */
#define RX_ACK_PULSE_COUNT 1

/* Defines the size of the SWC queues. */
#define TX_DATA_QUEUE_SIZE 2
#define RX_DATA_QUEUE_SIZE 2

/* Specifies the schedule configuration. */
// clang-format off
#define SCHEDULE                 \
    {                            \
        200, 200, 200, 200, 200, \
        200, 200, 200, 200, 200, \
        200, 200, 200, 200, 200, \
        200, 200, 200, 200, 200, \
        200, 200, 200, 200, 200, \
        200, 200, 200, 200, 200, \
        200, 200, 200, 200, 200, \
        200, 200, 200, 200, 200, \
        200, 200, 200, 200, 200, \
        200, 200, 200, 200, 200, \
    }

#define TX_AUDIO_TIMESLOTS                                                                             \
    {                                                                                                  \
        MAIN_TIMESLOT(0),  MAIN_TIMESLOT(1),  MAIN_TIMESLOT(2),  MAIN_TIMESLOT(3),  MAIN_TIMESLOT(4),  \
        MAIN_TIMESLOT(5),  MAIN_TIMESLOT(6),  MAIN_TIMESLOT(7),  MAIN_TIMESLOT(8),  MAIN_TIMESLOT(9),  \
        MAIN_TIMESLOT(10), MAIN_TIMESLOT(11), MAIN_TIMESLOT(12), MAIN_TIMESLOT(13), MAIN_TIMESLOT(14), \
        MAIN_TIMESLOT(15), MAIN_TIMESLOT(16), MAIN_TIMESLOT(17), MAIN_TIMESLOT(18), MAIN_TIMESLOT(19), \
        MAIN_TIMESLOT(20), MAIN_TIMESLOT(21), MAIN_TIMESLOT(22), MAIN_TIMESLOT(23), MAIN_TIMESLOT(24), \
        MAIN_TIMESLOT(25), MAIN_TIMESLOT(26), MAIN_TIMESLOT(27), MAIN_TIMESLOT(28), MAIN_TIMESLOT(29), \
        MAIN_TIMESLOT(30), MAIN_TIMESLOT(31), MAIN_TIMESLOT(32), MAIN_TIMESLOT(33), MAIN_TIMESLOT(34), \
        MAIN_TIMESLOT(35), MAIN_TIMESLOT(36), MAIN_TIMESLOT(37), MAIN_TIMESLOT(38), MAIN_TIMESLOT(39), \
        MAIN_TIMESLOT(40), MAIN_TIMESLOT(41), MAIN_TIMESLOT(42), MAIN_TIMESLOT(43), MAIN_TIMESLOT(44), \
        MAIN_TIMESLOT(45), MAIN_TIMESLOT(46), MAIN_TIMESLOT(47), MAIN_TIMESLOT(48),                    \
    }

#define TX_DATA_TIMESLOTS                                                                              \
    {                                                                                                  \
                                                                                    MAIN_TIMESLOT(49), \
    }

#define RX_DATA_TIMESLOTS                                                                              \
    {                                                                                                  \
                                                                                    AUTO_TIMESLOT(49), \
    }
// clang-format on

/* Defines the channels frequency band. */
#define CHANNEL_FREQ {163, 171, 179, 187, 195}
#define CHANNEL_SEQUENCE {0, 1, 2, 3, 4}

#endif /* SWC_CFG_COORD_H_ */
