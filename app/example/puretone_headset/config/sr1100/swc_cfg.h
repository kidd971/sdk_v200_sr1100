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
        250, 250, 250, 250,      \
    }
#define COORD_TIMESLOTS                                                                           \
    {                                                                                             \
        MAIN_TIMESLOT(0), MAIN_TIMESLOT(1), MAIN_TIMESLOT(2),                   MAIN_TIMESLOT(4), \
        MAIN_TIMESLOT(5), MAIN_TIMESLOT(6), MAIN_TIMESLOT(7),                                     \
    }

#define NODE_TIMESLOTS                                                          \
    {                                                                           \
                                                              MAIN_TIMESLOT(3), \
                                                              MAIN_TIMESLOT(8), \
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

#define BACK_CHANNEL_SWC_CCA_DATA_RETRY_TIME  160 /* 7.813 us CCA intervals. */
#define BACK_CHANNEL_SWC_CCA_DATA_TRY_COUNT   14

#define BACK_CHANNEL_SWC_CCA_AUDIO_RETRY_TIME 128 /* 6.25 us CCA intervals. */
#define BACK_CHANNEL_SWC_CCA_AUDIO_TRY_COUNT  8

#define MAIN_CHANNEL_SWC_CCA_FB_TRY_COUNT     15
#define BACK_CHANNEL_SWC_CCA_FB_TRY_COUNT     14

#endif /* SWC_CFG_H_ */
