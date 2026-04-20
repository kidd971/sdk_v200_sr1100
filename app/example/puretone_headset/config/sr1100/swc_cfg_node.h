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
#define TX_AUDIO_PULSE_WIDTH \
    {                        \
        3,                   \
        3,                   \
        3,                   \
        3,                   \
    }

#define TX_AUDIO_PULSE_GAIN \
    {                       \
        4,                  \
        4,                  \
        3,                  \
        2,                  \
    }

/* Sets the output power configuration for transmitting data and acknowledgement. */
#define TX_DATA_PULSE_WIDTH \
    {                       \
        6,                  \
        6,                  \
        6,                  \
        6,                  \
    }

#define TX_DATA_PULSE_GAIN \
    {                      \
        2,                 \
        2,                 \
        1,                 \
        0,                 \
    }

/* Sets the output power configuration for transmitting acknowledgement data. */
#define TX_DATA_ACK_PULSE_WIDTH \
    {                           \
        6,                      \
        6,                      \
        6,                      \
        7,                      \
    }

#define TX_DATA_ACK_PULSE_GAIN \
    {                          \
        3,                     \
        3,                     \
        2,                     \
        2,                     \
    }

/* Sets the output power configuration for transmitting acknowledgement data. */
#define TX_ACK_PULSE_WIDTH \
    {                      \
        5,                 \
        5,                 \
        5,                 \
        5,                 \
    }

#define TX_ACK_PULSE_GAIN \
    {                     \
        1,                \
        1,                \
        1,                \
        1,                \
    }

/* Sets the offsets for output power configuration in fallback mode. */
#define TX_AUDIO_FB_PULSE_WIDTH \
    {                           \
        4,                      \
        4,                      \
        4,                      \
        4,                      \
    }

#define TX_AUDIO_FB_PULSE_GAIN \
    {                          \
        2,                     \
        2,                     \
        1,                     \
        0,                     \
    }

/* CCA settings. */
#define SWC_CCA_AUDIO_FBK_TRY_COUNT 14

#endif /* SWC_CFG_NODE_H_ */
