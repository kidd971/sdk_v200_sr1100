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
        200, 200, 200, 200,      \
    }

#define COORD_TIMESLOTS                                                                                \
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
        MAIN_TIMESLOT(45), MAIN_TIMESLOT(46), MAIN_TIMESLOT(47),                                       \
    }

#define NODE_TIMESLOTS                                                                                 \
    {                                                                                                  \
                                                                 MAIN_TIMESLOT(48),                    \
    }
// clang-format on

/* Defines the channels frequency band. */
#define CHANNEL_FREQ     {163, 171, 179, 187, 195}
#define CHANNEL_SEQUENCE {0, 1, 2, 3, 4}

/* CCA settings */
#define SWC_CCA_RETRY_TIME          204 /* 9.96 us CCA intervals. */
#define SWC_CCA_AUDIO_TRY_COUNT     4   /* 29.88 us total CCA time. */
#define SWC_CCA_AUDIO_FBK_TRY_COUNT 6   /* 49.80 us total CCA time. */
#define SWC_CCA_DATA_TRY_COUNT      6   /* 49.80 us total CCA time. */

#endif /* SWC_CFG_H_ */
