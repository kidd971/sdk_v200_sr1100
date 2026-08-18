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
#define TX_DATA_PULSE_COUNT 1
#define TX_ACK_PULSE_COUNT  1

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
        7,                      \
    }

#define TX_DATA_ACK_PULSE_GAIN \
    {                          \
        3,                     \
        3,                     \
        2,                     \
        2,                     \
    }

/* Output power for the audio connection's acknowledgment -- the node's half of the audio link.
 *
 * This is the one power setting in the app that does not move with the fallback ladder. The
 * coordinator's audio power has four levels and steps up as conditions worsen; this is a single
 * value used at every level. TX_AUDIO_FB_PULSE_WIDTH / TX_AUDIO_FB_PULSE_GAIN are defined below
 * and never referenced by puretone_unidirectional_node.c -- the sibling app uses its pair at
 * puretone_headset.c:930, this one does not -- so there is no per-level ACK power here even in
 * principle.
 *
 * ACK_TX_POWER_MAX=1 takes it to the rail: width 7, the widest the field allows, at gain 0, which
 * swc_api.h defines as maximum amplitude -- the field runs 0 (max, 0 dB) to 3 (min, -1.8 dB), so
 * smaller is more. Default stays at the shipped 5 / 1 and the arm is a -D.
 *
 * Why this is a candidate. The drop-out is reported as the link going outright rather than
 * degrading, at the same distance whether the audio ladder is locked to mode 4 or free-running.
 * Free-running sweeps the coordinator's audio power across the whole ladder while locked sits at
 * the bottom of it, so a limit that does not move between those two is a limit that does not move
 * with the ladder -- and this is the only power in the link that answers that description. It is
 * also the right shape for a link that stops rather than fades: lose the acknowledgment and every
 * retransmission of a packet that did arrive is spent for nothing.
 *
 * Raising it is not free. The acknowledgment goes out in the same timeslot the coordinator just
 * used, so a louder ACK is more energy into the same near-field reflections the ISI work is
 * about. If the near field regresses while the distance improves, that is the trade and not a
 * contradiction.
 *
 * Not a shippable setting. Maximum width at maximum amplitude on all four bands is a lab arm for
 * bounding an answer; emissions compliance has not been considered here. */
#ifndef ACK_TX_POWER_MAX
#define ACK_TX_POWER_MAX 0
#endif

#if ACK_TX_POWER_MAX
#define TX_ACK_PULSE_WIDTH \
    {                      \
        7,                 \
        7,                 \
        7,                 \
        7,                 \
    }

#define TX_ACK_PULSE_GAIN \
    {                     \
        0,                \
        0,                \
        0,                \
        0,                \
    }
#else
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
#endif

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
