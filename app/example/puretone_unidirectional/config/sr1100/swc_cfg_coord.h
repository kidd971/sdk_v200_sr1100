/** @file  swc_cfg_coord.h
 *  @brief Coordinator application-specific configuration constants for the SPARK Wireless Core.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SWC_CFG_COORD_H_
#define SWC_CFG_COORD_H_

/* CONSTANTS ******************************************************************/
/* Sets the output power configuration for transmitting audio data. */
#define TX_AUDIO_PULSE_WIDTH \
    {                        \
        1,                   \
        1,                   \
        1,                   \
        1,                   \
    }

#define TX_AUDIO_PULSE_GAIN \
    {                       \
        5,                  \
        5,                  \
        4,                  \
        2,                  \
    }

/* Sets the output power configuration for transmitting data and acknowledgment. */
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

/* Sets the output power configuration for transmitting acknowledgment data. */
#define TX_DATA_ACK_PULSE_WIDTH \
    {                           \
        6,                      \
        6,                      \
        6,                      \
        6,                      \
    }

#define TX_DATA_ACK_PULSE_GAIN \
    {                          \
        2,                     \
        2,                     \
        1,                     \
        0,                     \
    }

/* Sets the output power configuration for transmitting acknowledgment data. */
#define TX_ACK_PULSE_WIDTH \
    {                      \
        5,                 \
        5,                 \
        5,                 \
        5,                 \
    }

#define TX_ACK_PULSE_GAIN \
    {                     \
        2,                \
        2,                \
        1,                \
        1,                \
    }

/* Sets the offsets for output power configuration in fallback mode.
 *
 * One entry per SWC fallback level, so each list carries SWC_FALLBACK_MODE_COUNT values. The
 * fourth is the 24 kHz ADPCM rung and repeats the third: it sits below mode 3 on the ladder, so
 * it must be at least as robust, and copying the settings that rung already runs at keeps the
 * new rung's measurement about the sample rate rather than about a power change made at the same
 * time. Tuning it is a follow-up. */
#define TX_AUDIO_FB_BAND_1_PULSE_WIDTH \
    {                                  \
        2,                             \
        3,                             \
        3,                             \
        3,                             \
    }

#define TX_AUDIO_FB_BAND_1_PULSE_GAIN \
    {                                 \
        5,                            \
        4,                            \
        1,                            \
        1,                            \
    }

#define TX_AUDIO_FB_BAND_2_PULSE_WIDTH \
    {                                  \
        2,                             \
        3,                             \
        3,                             \
        3,                             \
    }

#define TX_AUDIO_FB_BAND_2_PULSE_GAIN \
    {                                 \
        5,                            \
        4,                            \
        1,                            \
        1,                            \
    }

#define TX_AUDIO_FB_BAND_3_PULSE_WIDTH \
    {                                  \
        2,                             \
        3,                             \
        4,                             \
        4,                             \
    }

#define TX_AUDIO_FB_BAND_3_PULSE_GAIN \
    {                                 \
        4,                            \
        3,                            \
        0,                            \
        0,                            \
    }

#define TX_AUDIO_FB_BAND_4_PULSE_WIDTH \
    {                                  \
        2,                             \
        4,                             \
        5,                             \
        5,                             \
    }

#define TX_AUDIO_FB_BAND_4_PULSE_GAIN \
    {                                 \
        3,                            \
        3,                            \
        0,                            \
        0,                            \
    }

/* CCA settings. */
#define SWC_CCA_AUDIO_FBK_1_TRY_COUNT 7
#define SWC_CCA_AUDIO_FBK_2_TRY_COUNT 13
#define SWC_CCA_AUDIO_FBK_3_TRY_COUNT 14
#define SWC_CCA_AUDIO_FBK_4_TRY_COUNT 14

#endif /* SWC_CFG_COORD_H_ */
