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

#define COORD_TIMESLOTS                                                                                                    \
    {                                                                                                                      \
        SWC_MAIN_TIMESLOT(0),  SWC_MAIN_TIMESLOT(1),  SWC_MAIN_TIMESLOT(2),  SWC_MAIN_TIMESLOT(3),  SWC_MAIN_TIMESLOT(4),  \
        SWC_MAIN_TIMESLOT(5),  SWC_MAIN_TIMESLOT(6),  SWC_MAIN_TIMESLOT(7),  SWC_MAIN_TIMESLOT(8),  SWC_MAIN_TIMESLOT(9),  \
        SWC_MAIN_TIMESLOT(10), SWC_MAIN_TIMESLOT(11), SWC_MAIN_TIMESLOT(12), SWC_MAIN_TIMESLOT(13), SWC_MAIN_TIMESLOT(14), \
        SWC_MAIN_TIMESLOT(15), SWC_MAIN_TIMESLOT(16), SWC_MAIN_TIMESLOT(17), SWC_MAIN_TIMESLOT(18), SWC_MAIN_TIMESLOT(19), \
        SWC_MAIN_TIMESLOT(20), SWC_MAIN_TIMESLOT(21), SWC_MAIN_TIMESLOT(22), SWC_MAIN_TIMESLOT(23), SWC_MAIN_TIMESLOT(24), \
        SWC_MAIN_TIMESLOT(25), SWC_MAIN_TIMESLOT(26), SWC_MAIN_TIMESLOT(27), SWC_MAIN_TIMESLOT(28), SWC_MAIN_TIMESLOT(29), \
        SWC_MAIN_TIMESLOT(30), SWC_MAIN_TIMESLOT(31), SWC_MAIN_TIMESLOT(32), SWC_MAIN_TIMESLOT(33), SWC_MAIN_TIMESLOT(34), \
        SWC_MAIN_TIMESLOT(35), SWC_MAIN_TIMESLOT(36), SWC_MAIN_TIMESLOT(37), SWC_MAIN_TIMESLOT(38), SWC_MAIN_TIMESLOT(39), \
        SWC_MAIN_TIMESLOT(40), SWC_MAIN_TIMESLOT(41), SWC_MAIN_TIMESLOT(42), SWC_MAIN_TIMESLOT(43), SWC_MAIN_TIMESLOT(44), \
        SWC_MAIN_TIMESLOT(45), SWC_MAIN_TIMESLOT(46), SWC_MAIN_TIMESLOT(47),                                               \
    }

#define NODE_TIMESLOTS                                                                              \
    {                                                                                               \
                                                                             SWC_MAIN_TIMESLOT(48), \
    }
// clang-format on

/* Defines the channels frequency band. */
#define CHANNEL_FREQ     {163, 171, 179, 187, 195}
#define CHANNEL_SEQUENCE {0, 1, 2, 3, 4}

/* CCA settings. */
#define SWC_CCA_RETRY_TIME          204 /* 9.96 us CCA intervals. */
#define SWC_CCA_AUDIO_TRY_COUNT     4   /* 29.88 us total CCA time. */
#define SWC_CCA_AUDIO_FBK_TRY_COUNT 6   /* 49.80 us total CCA time. */
#define SWC_CCA_DATA_TRY_COUNT      6   /* 49.80 us total CCA time. */

#endif /* SWC_CFG_H_ */
