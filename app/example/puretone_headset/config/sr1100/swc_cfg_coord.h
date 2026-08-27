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
        6,                      \
    }

#define TX_DATA_ACK_PULSE_GAIN \
    {                          \
        2,                     \
        2,                     \
        1,                     \
        0,                     \
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
        2,                \
        2,                \
        1,                \
        1,                \
    }

/* Sets the offsets for output power configuration in fallback mode.
 *
 * One row per band, one entry per FALLBACK mode -- not per rung. There are three entries because
 * SWC_FALLBACK_MODE_COUNT is 3 (puretone_dongle.c:102), and the loop that consumes them indexes
 * [band][j] for j in 0..2. The app's fb= numbering counts the main mode as 0, so the mapping is
 * off by one and the LAST entry of each row is the one the bottom rung uses:
 *
 *   entry 0 -> fb=1     entry 1 -> fb=2     entry 2 -> fb=3  <- bottom rung
 *
 * fb=0 does not appear here at all; it takes TX_AUDIO_PULSE_WIDTH / _GAIN above.
 *
 * Width is 0..7 (0 narrow, 7 large). Gain is 0..7 and INVERTED -- 0 is max, 0 dB, and larger
 * numbers attenuate. That inversion is why the shipped table already descends toward the bottom
 * rung, 5 -> 4 -> 1 on bands 1 and 2: it is already spending power where the link is worst.
 *
 * The values are per-band and deliberately not flat -- band 4 was tuned hotter than band 1 -- so
 * flattening them discards that tuning. This is the raw output power knob and the whole ladder
 * radiates through the antenna the certified build was measured with, so re-check emissions
 * before shipping a change here. Note also that only the coordinator's TX audio lives in this
 * file; the headset's ACK path is in swc_cfg_node.h and is not affected by anything below. */
#define TX_AUDIO_FB_BAND_1_PULSE_WIDTH \
    {                                  \
        2,                             \
        3,                             \
        3,                             \
    }

#define TX_AUDIO_FB_BAND_1_PULSE_GAIN \
    {                                 \
        5,                            \
        4,                            \
        1,                            \
    }

#define TX_AUDIO_FB_BAND_2_PULSE_WIDTH \
    {                                  \
        2,                             \
        3,                             \
        3,                             \
    }

#define TX_AUDIO_FB_BAND_2_PULSE_GAIN \
    {                                 \
        5,                            \
        4,                            \
        1,                            \
    }

#define TX_AUDIO_FB_BAND_3_PULSE_WIDTH \
    {                                  \
        2,                             \
        3,                             \
        4,                             \
    }

#define TX_AUDIO_FB_BAND_3_PULSE_GAIN \
    {                                 \
        4,                            \
        3,                            \
        0,                            \
    }

#define TX_AUDIO_FB_BAND_4_PULSE_WIDTH \
    {                                  \
        2,                             \
        4,                             \
        5,                             \
    }

#define TX_AUDIO_FB_BAND_4_PULSE_GAIN \
    {                                 \
        3,                            \
        3,                            \
        0,                            \
    }

/* CCA settings. */
#define SWC_CCA_AUDIO_FBK_1_TRY_COUNT 7
#define SWC_CCA_AUDIO_FBK_2_TRY_COUNT 13
#define SWC_CCA_AUDIO_FBK_3_TRY_COUNT 14
#define SWC_CCA_AUDIO_FBK_4_TRY_COUNT 15

#endif /* SWC_CFG_COORD_H_ */
