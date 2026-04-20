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
#define SWC_QUEUE_SIZE 3

/* Schedule configuration */
#define SCHEDULE        {500, 500}
#define COORD_TIMESLOTS {MAIN_TIMESLOT(0)}
#define NODE_TIMESLOTS  {MAIN_TIMESLOT(1)}

/* Channels */
#define CHANNEL_FREQ     {163, 171, 179, 187, 195}
#define CHANNEL_SEQUENCE {0, 1, 2, 3, 4}

#endif /* SWC_CFG_H_ */
