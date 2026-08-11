/** @file  swc_cfg.h
 *  @brief Configuration constants for the SPARK Wireless Core.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SWC_CFG_H_
#define SWC_CFG_H_

/* CONSTANTS ******************************************************************/
/* Defines the size of the SWC queues. */
#define SWC_QUEUE_SIZE 2
/* Maximum channel number. */
#define MAX_CHANNEL_NUMBER 5
/* Pulse count for SR1100. */
#define SR1100_PULSE_COUNT 1

/* Specifies the schedule configuration. */
// clang-format off
#define SCHEDULE                 \
    {                            \
        250, 250, 250, 250, 250, \
        250, 250, 250, 250, 250, \
        250, 250, 250, 250, 250, \
        250, 250, 250, 250, 250, \
        250,                     \
    }
#define COORD_TIMESLOTS                                                                                                       \
    {                                                                                                                         \
        SWC_MAIN_TIMESLOT(0),  SWC_MAIN_TIMESLOT(1),  SWC_MAIN_TIMESLOT(2),  SWC_MAIN_TIMESLOT(3),  SWC_MAIN_TIMESLOT(4),   \
        SWC_MAIN_TIMESLOT(5),  SWC_MAIN_TIMESLOT(6),  SWC_MAIN_TIMESLOT(7),  SWC_MAIN_TIMESLOT(8),  SWC_MAIN_TIMESLOT(9),   \
        SWC_MAIN_TIMESLOT(10), SWC_MAIN_TIMESLOT(11), SWC_MAIN_TIMESLOT(12), SWC_MAIN_TIMESLOT(13), SWC_MAIN_TIMESLOT(14),  \
        SWC_MAIN_TIMESLOT(15), SWC_MAIN_TIMESLOT(16), SWC_MAIN_TIMESLOT(17), SWC_MAIN_TIMESLOT(18), SWC_MAIN_TIMESLOT(19),  \
    }

#define NODE_TIMESLOTS                                                                                                        \
    {                                                                                                                         \
                                                                                          SWC_MAIN_TIMESLOT(20),              \
    }
// clang-format on

/* Channels. */
#define CHANNEL_FREQ     {164, 174, 184, 194}
#define CHANNEL_SEQUENCE {0, 1, 2, 3}

/* CCA settings. */
#define MAIN_CHANNEL_SWC_CCA_AUDIO_RETRY_TIME 96 /* 4.688 us CCA intervals. */
#define MAIN_CHANNEL_SWC_CCA_AUDIO_TRY_COUNT  2

#define MAIN_CHANNEL_SWC_CCA_DATA_RETRY_TIME  160 /* 7.813 us CCA intervals. */
#define MAIN_CHANNEL_SWC_CCA_DATA_TRY_COUNT   15

#define MAIN_CHANNEL_SWC_CCA_FB_TRY_COUNT     15

#endif /* SWC_CFG_H_ */
