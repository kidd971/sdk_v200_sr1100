/** @file  swc_cfg_coord.h
 *  @brief Application specific configuration constants for the SPARK Wireless Core.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SWC_CFG_COORD_H_
#define SWC_CFG_COORD_H_

/* CONSTANTS ******************************************************************/

/* Sets the output power configuration for transmitting data. */
#define TX_DATA_PULSE_COUNT 1
#define TX_DATA_PULSE_WIDTH 3
#define TX_DATA_PULSE_GAIN  3

/* Input power configuration */
#define RX_AUTO_REPLY_PULSE_COUNT 1 /* Pulses configuration of received auto-reply frames */

#endif /* SWC_CFG_COORD_H_ */
