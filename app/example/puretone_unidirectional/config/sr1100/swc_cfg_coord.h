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

/* Level 4 output power -- the 24 kHz rung's.
 *
 * Now defaults to what SPARK's own audio demo runs at its bottom rung: pulse width 6 across all
 * four bands at gain 1/1/1/0. Levels 1 to 3 of this table were already identical to theirs, value
 * for value; level 4 was the one entry with no counterpart, because it was filled in by copying
 * level 3 when the 24 kHz rung was added. Copying the reference removes the last place this table
 * diverges from a configuration that is known to work on this hardware.
 *
 * The node's audio ACK power needed no such change -- it is already width 5 gain 1 on every band,
 * which is exactly the demo's ack_power for this mode.
 *
 * Note the gain field runs backwards: swc_api.h defines 0 as maximum amplitude (0 dB) and 3 as
 * minimum (-1.8 dB), so smaller is louder. That is why the demo's level 4 is not uniformly louder
 * than the old values -- it widens every band but moves band 3's gain from 0 to 1, trading a
 * little amplitude on the band that had the most for width everywhere.
 *
 * Selecting it is not a fix for anything measured. It was tried as an arm and moved the drop-out
 * distance not at all, over two runs, with the ladder locked to mode 4 and free-running. It is the
 * default because matching the reference is worth more than an unmeasured value of our own, not
 * because it changed a result.
 *
 *   0  the value this rung shipped with -- a copy of level 3, width 3/3/4/5, gain 1/1/0/0.
 *      Kept so the comparison can be run in reverse without archaeology.
 *   1  SPARK's demo level 4. The default.
 *   2  the rail: width 7, the widest the field allows, at gain 0 on every band.
 *
 * 2 exists because 1 changed nothing, and one more step of width would have been worth a few
 * tenths of a dB against three steps that did nothing -- so the useful experiment was the ceiling
 * rather than another step. If the loudest this radio can transmit at level 4 leaves the distance
 * where it is, level-4 output power is not what sets it, and the remaining candidates are the ones
 * that do not move with the ladder: SR1100_PULSE_COUNT, still at 1 of a possible 3, and the ACK
 * power above.
 *
 * 2 is a lab arm and not shippable -- maximum width at maximum amplitude on all four bands, with
 * emissions compliance not considered. */
#ifndef FBK4_TX_POWER_REF
#define FBK4_TX_POWER_REF 1
#endif

#if FBK4_TX_POWER_REF == 2
#define TX_AUDIO_FB_L4_WIDTH_B1 7
#define TX_AUDIO_FB_L4_WIDTH_B2 7
#define TX_AUDIO_FB_L4_WIDTH_B3 7
#define TX_AUDIO_FB_L4_WIDTH_B4 7
#define TX_AUDIO_FB_L4_GAIN_B1  0
#define TX_AUDIO_FB_L4_GAIN_B2  0
#define TX_AUDIO_FB_L4_GAIN_B3  0
#define TX_AUDIO_FB_L4_GAIN_B4  0
#elif FBK4_TX_POWER_REF
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
