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
/* Follows SR1100_PULSE_COUNT rather than restating it: the coordinator receives this connection
 * with rx_pulse_count = SR1100_PULSE_COUNT, so a hard-coded 1 here breaks the data link -- and with
 * it link_is_up() -- the moment that knob is raised.
 *
 * TX_ACK_PULSE_COUNT is referenced by nothing: the audio ACK takes its pulse count from
 * SR1100_PULSE_COUNT directly (puretone_unidirectional_node.c). Kept only so it cannot disagree. */
#define TX_DATA_PULSE_COUNT SR1100_PULSE_COUNT
#define TX_ACK_PULSE_COUNT  SR1100_PULSE_COUNT

/* UNUSED in this application -- changing it changes nothing in the binary.
 *
 * The node's own audio TX power, inherited from puretone_headset, where the headset also sends
 * audio back to the dongle. This node never transmits audio: on the audio connection it only
 * receives, and the one thing it sends there is the acknowledgment, whose power is
 * TX_ACK_PULSE_WIDTH / TX_ACK_PULSE_GAIN below. The DG's TX_AUDIO_PULSE_WIDTH in swc_cfg_coord.h
 * shares the name and IS live -- it is the audio power on the top rung. Do not tune this one
 * expecting that one to move. */
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
/** @brief Transmit at the widest pulse the field allows, on every band and every fallback level.
 *
 *  Widens ONLY. The gain fields are left exactly as they were -- that is the whole point of
 *  this switch existing separately from FBK4_TX_POWER_REF=2 and ACK_TX_POWER_MAX=1, both of
 *  which take width to 7 AND gain to 0 and are documented here as lab arms rather than
 *  shippable settings.
 *
 *  This is a product decision, not a measurement. It should not be read as a fix: the two
 *  arms above put width at 7 with gain at 0 -- strictly louder than this -- and moved the
 *  drop-out distance not at all, over two runs each. Nothing here is expected to change that
 *  number, and if it does, the arms' results need re-examining rather than believing.
 *
 *  What it does change, and what to watch. The shipped table steps power UP as the link
 *  degrades: width 2 at level 1, 6 at level 4. That graduation is deliberate -- a good link
 *  does not need the energy, and energy it does not need goes into near-field reflections.
 *  Flattening the table to 7 transmits at full width when the peer is close and the link is
 *  fine, which is the condition the ISI mitigation work exists to survive. If near-field
 *  obstruction regresses after this, this switch is the first thing to turn off.
 *
 *  Off by default since 2026-09-29, -DTX_PULSE_WIDTH_MAX=1 for the arm: an unmeasured raise does
 *  not belong in the base. Not a fix for anything -- see the coordinator's TX_PULSE_WIDTH_MAX in
 *  swc_cfg_coord.h. At 0 the audio ACK is width 5.
 *
 *  Emissions compliance has not been assessed for this table.
 */
#ifndef TX_PULSE_WIDTH_MAX
#define TX_PULSE_WIDTH_MAX 0
#endif

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
#if TX_PULSE_WIDTH_MAX
#define TX_ACK_PULSE_WIDTH \
    {                      \
        7,                 \
        7,                 \
        7,                 \
        7,                 \
    }
#else
#define TX_ACK_PULSE_WIDTH \
    {                      \
        5,                 \
        5,                 \
        5,                 \
        5,                 \
    }
#endif

#define TX_ACK_PULSE_GAIN \
    {                     \
        1,                \
        1,                \
        1,                \
        1,                \
    }
#endif

/* UNUSED in this application, for the same reason as TX_AUDIO_PULSE_WIDTH above: the per-rung
 * power of an audio TX this node does not have (puretone_headset.c:952 is the user it came from).
 * It could not serve as a per-rung ACK power either -- swc_connection_set_fallback_channels() is
 * TX-connection only, and the node's audio connection is RX. */
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
