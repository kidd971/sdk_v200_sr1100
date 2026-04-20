/** @file  swc_cfg.h
 *  @brief Application specific configuration constants for the SPARK Wireless Core.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SWC_CFG_H_
#define SWC_CFG_H_

/* CONSTANTS ******************************************************************/
/* SWC queue size */
#define SWC_QUEUE_SIZE 2

/* Schedule configuration */
#define SCHEDULE                {250, 250, 250, 250}
#define TX_TO_NODE1_TIMESLOTS   {MAIN_TIMESLOT(0)}
#define RX_FROM_NODE1_TIMESLOTS {MAIN_TIMESLOT(1)}
#define TX_TO_NODE2_TIMESLOTS   {MAIN_TIMESLOT(2)}
#define RX_FROM_NODE2_TIMESLOTS {MAIN_TIMESLOT(3)}

/* Channels */
#define CHANNEL_FREQ     {163, 171, 179, 187, 195}
#define CHANNEL_SEQUENCE {0, 1, 2, 3, 4}

#endif /* SWC_CFG_H_ */
