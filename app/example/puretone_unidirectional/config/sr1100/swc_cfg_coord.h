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

/* Level 4 output power -- the 24 kHz rung's, and the only entry in this table that was not
 * measured. It shipped as a copy of level 3, on the reasoning that a rung below another should be
 * at least as robust; "at least as" is not the same as "enough", and levels 1 to 3 here are
 * identical to what SPARK's own audio demo runs while level 4 is not.
 *
 * 1 selects the demo's level 4: pulse width 6 across all bands, which is the widest entry in
 * their whole table, at gain 1/1/1/0. They keep adding energy on the way down; this table stops
 * adding at level 3.
 *
 * Worth knowing what this can and cannot do. The close-range failure is multipath -- rx_rej
 * climbs while cca_fail does not -- and more energy does not unsmear a reflection, since the
 * reflections scale with it. What it can do is push the residual signal-to-noise after smearing
 * back over the decode threshold, so it helps if the near field is marginal rather than hopeless.
 * If it changes nothing, the failure is pure smearing and ISI mitigation is the only answer. */
#ifndef FBK4_TX_POWER_REF
#define FBK4_TX_POWER_REF 0
#endif

#if FBK4_TX_POWER_REF
#define TX_AUDIO_FB_L4_WIDTH_B1 6
#define TX_AUDIO_FB_L4_WIDTH_B2 6
#define TX_AUDIO_FB_L4_WIDTH_B3 6
#define TX_AUDIO_FB_L4_WIDTH_B4 6
#define TX_AUDIO_FB_L4_GAIN_B1  1
#define TX_AUDIO_FB_L4_GAIN_B2  1
#define TX_AUDIO_FB_L4_GAIN_B3  1
#define TX_AUDIO_FB_L4_GAIN_B4  0
#else
#define TX_AUDIO_FB_L4_WIDTH_B1 3
#define TX_AUDIO_FB_L4_WIDTH_B2 3
#define TX_AUDIO_FB_L4_WIDTH_B3 4
#define TX_AUDIO_FB_L4_WIDTH_B4 5
#define TX_AUDIO_FB_L4_GAIN_B1  1
#define TX_AUDIO_FB_L4_GAIN_B2  1
#define TX_AUDIO_FB_L4_GAIN_B3  0
#define TX_AUDIO_FB_L4_GAIN_B4  0
#endif

/* Sets the offsets for output power configuration in fallback mode. One entry per SWC fallback
 * level, so each list carries SWC_FALLBACK_MODE_COUNT values. Levels 1 to 3 are identical to
 * SPARK's own audio demo; level 4 comes from the switch above. */
#define TX_AUDIO_FB_BAND_1_PULSE_WIDTH \
    {                                  \
        2,                             \
        3,                             \
        3,                             \
        TX_AUDIO_FB_L4_WIDTH_B1,\
    }

#define TX_AUDIO_FB_BAND_1_PULSE_GAIN \
    {                                  \
        5,                             \
        4,                             \
        1,                             \
        TX_AUDIO_FB_L4_GAIN_B1,\
    }

#define TX_AUDIO_FB_BAND_2_PULSE_WIDTH \
    {                                  \
        2,                             \
        3,                             \
        3,                             \
        TX_AUDIO_FB_L4_WIDTH_B2,\
    }

#define TX_AUDIO_FB_BAND_2_PULSE_GAIN \
    {                                  \
        5,                             \
        4,                             \
        1,                             \
        TX_AUDIO_FB_L4_GAIN_B2,\
    }

#define TX_AUDIO_FB_BAND_3_PULSE_WIDTH \
    {                                  \
        2,                             \
        3,                             \
        4,                             \
        TX_AUDIO_FB_L4_WIDTH_B3,\
    }

#define TX_AUDIO_FB_BAND_3_PULSE_GAIN \
    {                                  \
        4,                             \
        3,                             \
        0,                             \
        TX_AUDIO_FB_L4_GAIN_B3,\
    }

#define TX_AUDIO_FB_BAND_4_PULSE_WIDTH \
    {                                  \
        2,                             \
        4,                             \
        5,                             \
        TX_AUDIO_FB_L4_WIDTH_B4,\
    }

#define TX_AUDIO_FB_BAND_4_PULSE_GAIN \
    {                                  \
        3,                             \
        3,                             \
        0,                             \
        TX_AUDIO_FB_L4_GAIN_B4,\
    }

/* CCA settings. */
#define SWC_CCA_AUDIO_FBK_1_TRY_COUNT 7
#define SWC_CCA_AUDIO_FBK_2_TRY_COUNT 13
#define SWC_CCA_AUDIO_FBK_3_TRY_COUNT 14
#define SWC_CCA_AUDIO_FBK_4_TRY_COUNT 14

#endif /* SWC_CFG_COORD_H_ */
