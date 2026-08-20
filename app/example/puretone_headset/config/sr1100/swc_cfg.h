/** @file  swc_cfg.h
 *  @brief Configuration constants for the SPARK Wireless Core.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SWC_CFG_H_
#define SWC_CFG_H_

/* CONSTANTS ******************************************************************/
/* Defines the size of the SWC queues. */
#define SWC_QUEUE_SIZE 2
/* Maximum channel number. */
#define MAX_CHANNEL_NUMBER 5
/* Pulse count for SR1100. */
#define SR1100_PULSE_COUNT 1

/* Inter-symbol interference mitigation level, applied to both roles through swc_node_cfg_t.
 *
 * Naming it here makes it a knob instead of an accident: swc_node_cfg_t was zero-initialised,
 * which gave SWC_ISI_MITIG_0 implicitly. Higher levels insert pauses between symbols and
 * lengthen the preamble accordingly, so they cost airtime -- the same budget that already
 * refused FEC 2.00 inside a 250 us timeslot.
 *
 * Level 2 is what close-range obstruction wants. Obstruction is a non-line-of-sight case: the
 * direct path is gone and what arrives is reflections, so packets land corrupted rather than
 * not at all -- rx_rej climbing while rx_ok holds is the signature -- and that is precisely
 * what ISI mitigation is for. Level 1, which is what SPARK's reference demo ships, was measured
 * and was not enough.
 *
 * Level 3 does not work here, and not by a small margin: it crackles with no obstruction at
 * all. The preamble belongs to the connection, not to a fallback mode, so it has to fit the
 * largest payload on the ladder -- mode 0's 242 B -- inside a 250 us slot, and at level 3 it no
 * longer reliably does. Note the failure is audible corruption at the TOP of the ladder, not
 * the red LED at init that an unaffordable FEC gives: this one fits well enough to boot and
 * link, so it will pass a bring-up check and fail in listening.
 *
 * Level 2 only pays off alongside a wider accumulator on the bottom rung. Either one alone left
 * the dropouts unchanged: ISI raises the odds of decoding a single attempt through multipath,
 * the accumulator supplies enough attempts for those odds to cash in. Three attempts at good
 * odds and six attempts at bad odds both lose.
 *
 * UNVERIFIED HERE, AND KNOWN BAD ON THE OTHER LINE. puretone_unidirectional ran level 2 beside a
 * mode 3 that the same accumulator change had grown to 100 B, and it crashed under sustained
 * long-range obstruction -- reproducibly, and cured by taking mode 3 back to 54 B. This app's
 * mode 3 is also 100 B (see MAIN_CHANNEL_FBK_3_ACC_MUL), so it carries the same pair. Nobody has
 * flashed this app since that was learned.
 *
 * Left as is deliberately: the presets are moving to puretone_unidirectional, and the fix that
 * worked there -- move the retransmission down to a fifth rung -- has nowhere to go here, because
 * this ladder stops at mode 3. Taking mode 3 back to 2.3x would simply give up the retransmission
 * rather than relocate it. So the choice is a real trade either way, and it is not worth paying on
 * a line that is being retired.
 *
 * If this app is flashed again for anything more than a bench smoke test, decide first: either
 * mode 3 back to 2.3x (safe, no retransmission gain) or this level back to 1 (keeps the gain,
 * gives up the near-field improvement). Do not assume the pair is fine because it builds.
 *
 * DUAL RADIO DOES NOT WORK AT THIS LEVEL. Measured on the u5a5: a dual-radio node at level 2
 * carries no audio -- only fallback mode 3 comes up, and only sometimes -- while the same
 * build at level 1 or 0 runs normally. The mechanism is the one described for level 3 above,
 * arriving a level early: a dual-radio schedule is tighter, so on two radios level 2 behaves
 * the way level 3 behaves on one, and that only the smallest-payload rung survives is the
 * tell. It is not a vendor problem -- the pristine v2.4.0-rc2 tree runs dual radio fine, and
 * this line is the only place our swc_cfg.h differs from theirs.
 *
 * So the dual-radio presets in this application are broken at the default and need
 * -DNODE_ISI_MITIG=SWC_ISI_MITIG_1 to run at all. The default stays at 2 because this line
 * ships SINGLE radio, where level 2 is what the near field needs. puretone_unidirectional
 * defaults to 1 for the opposite reason: it is going dual radio, and two antennas see
 * different multipath, so dual radio at level 1 was measured to hold the near field on its
 * own. The two applications disagreeing is deliberate -- do not align them.
 *
 * Must be identical on the dongle and the headset: it changes the preamble both ends use to
 * find each other. A mismatched pair does not link, and that failure is indistinguishable
 * from the dual-radio one above. Overridable per build (-DNODE_ISI_MITIG=SWC_ISI_MITIG_1) so
 * an A/B arm can be cut without editing this file -- which is how the level last drifted
 * without anyone meaning it to. */
#ifndef NODE_ISI_MITIG
#define NODE_ISI_MITIG SWC_ISI_MITIG_2
#endif

/* Specifies the schedule configuration. */
// clang-format off
#define SCHEDULE                 \
    {                            \
        250, 250, 250, 250, 250, \
        250, 250, 250, 250,      \
    }
#define COORD_TIMESLOTS                                                                                               \
    {                                                                                                                 \
        SWC_MAIN_TIMESLOT(0), SWC_MAIN_TIMESLOT(1), SWC_MAIN_TIMESLOT(2),                       SWC_MAIN_TIMESLOT(4), \
        SWC_MAIN_TIMESLOT(5), SWC_MAIN_TIMESLOT(6), SWC_MAIN_TIMESLOT(7),                                             \
    }

#define NODE_TIMESLOTS                                                                          \
    {                                                                                           \
                                                                          SWC_MAIN_TIMESLOT(3), \
                                                                          SWC_MAIN_TIMESLOT(8), \
    }
// clang-format on

/* Channels. */
#define CHANNEL_FREQ     {164, 174, 184, 194}
#define CHANNEL_SEQUENCE {0, 1, 2, 3}

/* CCA settings. */
#define MAIN_CHANNEL_SWC_CCA_AUDIO_RETRY_TIME 96 /* 4.688 us CCA intervals. */
#define MAIN_CHANNEL_SWC_CCA_AUDIO_TRY_COUNT  2

#define MAIN_CHANNEL_SWC_CCA_DATA_RETRY_TIME  160 /* 7.813 us CCA intervals. */
#define MAIN_CHANNEL_SWC_CCA_DATA_TRY_COUNT   15

#define BACK_CHANNEL_SWC_CCA_DATA_RETRY_TIME  160 /* 7.813 us CCA intervals. */
#define BACK_CHANNEL_SWC_CCA_DATA_TRY_COUNT   14

#define BACK_CHANNEL_SWC_CCA_AUDIO_RETRY_TIME 128 /* 6.25 us CCA intervals. */
#define BACK_CHANNEL_SWC_CCA_AUDIO_TRY_COUNT  8

#define MAIN_CHANNEL_SWC_CCA_FB_TRY_COUNT     15
#define BACK_CHANNEL_SWC_CCA_FB_TRY_COUNT     14

#endif /* SWC_CFG_H_ */
