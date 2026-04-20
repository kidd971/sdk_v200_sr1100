/** @file  swc_cfg_node.h
 *  @brief Node application-specific configuration constants for the SPARK Wireless Core.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SWC_CFG_NODE_H_
#define SWC_CFG_NODE_H_

/* CONSTANTS ******************************************************************/
/* Sets the output power configuration for transmitting audio data. */
#define TX_AUDIO_PULSE_COUNT 1
#define TX_AUDIO_PULSE_WIDTH 1
#define TX_AUDIO_PULSE_GAIN  1

/* Sets the output power configuration for transmitting data and acknowledgement. */
#define TX_DATA_PULSE_COUNT 1
#define TX_DATA_PULSE_WIDTH 2
#define TX_DATA_PULSE_GAIN  1

/* Sets the output power configuration for transmitting acknowledgement data. */
#define TX_ACK_PULSE_COUNT 1
#define TX_ACK_PULSE_WIDTH 6
#define TX_ACK_PULSE_GAIN  0

/* Sets the offsets for output power configuration in fallback mode. */
#define TX_AUDIO_FB_PULSE_COUNT 1
#define TX_AUDIO_FB_PULSE_WIDTH 5
#define TX_AUDIO_FB_PULSE_GAIN  2

/* Sets the number of pulses of received data and acknowledgement. */
#define RX_DATA_PULSE_COUNT 1
#define RX_ACK_PULSE_COUNT  1

#endif /* SWC_CFG_NODE_H_ */
