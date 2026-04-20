/** @file  swc_cfg_node.h
 *  @brief Application specific configuration constants for the SPARK Wireless Core.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SWC_CFG_NODE_H_
#define SWC_CFG_NODE_H_

/* CONSTANTS ******************************************************************/

/* Sets the output power configuration for transmitting data. */
#define TX_DATA_PULSE_COUNT 1
#define TX_DATA_PULSE_WIDTH 7
#define TX_DATA_PULSE_GAIN  1

/* Sets the output power configuration for transmitting acknowledgment data. */
#define TX_ACK_PULSE_COUNT 1
#define TX_ACK_PULSE_WIDTH 6
#define TX_ACK_PULSE_GAIN  0

/* Input power configuration */
#define RX_ACK_PULSE_COUNT  1 /* Pulses configuration of received ACK frames */
#define RX_DATA_PULSE_COUNT 1 /* Pulses configuration of received data frames */

#endif /* SWC_CFG_NODE_H_ */
