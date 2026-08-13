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
 * lengthen the preamble accordingly, so they cost airtime.
 *
 * Level 1 is what SPARK's shipping demo runs on both roles. Level 2 helped close-range
 * obstruction on the older configuration: obstruction is a non-line-of-sight case, so the direct
 * path is gone and what arrives is reflections, packets landing corrupted rather than missing --
 * rx_rej climbing while rx_ok holds is the signature -- and that is what ISI mitigation is for.
 *
 * Back at 1 to measure it against the current ladder, where the trade has changed: the wider
 * accumulator took mode 3's payload from 54 B to 100 B, so a longer preamble now has to fit
 * beside a bigger bottom-rung packet than when level 2 was chosen. Slot pressure is also the
 * leading suspect for the crash under sustained long-range obstruction, and this is the cheapest
 * way to take some off.
 *
 * Level 3 does not work here, and not by a small margin: it crackles with no obstruction at
 * all. The preamble belongs to the connection, not to a fallback mode, so it has to fit the
 * largest payload on the ladder inside a 250 us slot, and at level 3 it no longer reliably
 * does. Note the failure is audible corruption at the TOP of the ladder, not the red LED at
 * init that an unaffordable setting gives -- it boots and links, then sounds wrong.
 *
 * Level 2 only pays off alongside the wider accumulator on the bottom rungs. Either one alone
 * left the dropouts unchanged: ISI raises the odds of decoding a single attempt through
 * multipath, the accumulator supplies enough attempts for those odds to cash in.
 *
 * Must be identical on the coordinator and the node: it changes the preamble both ends use to
 * find each other. Overridable per build (-DNODE_ISI_MITIG=SWC_ISI_MITIG_1) for an A/B arm. */
#ifndef NODE_ISI_MITIG
#define NODE_ISI_MITIG SWC_ISI_MITIG_1
#endif

/* Specifies the schedule configuration. */
// clang-format off
#define SCHEDULE                 \
    {                            \
        250, 250, 250, 250, 250, \
        250, 250, 250, 250, 250, \
        250, 250, 250, 250, 250, \
        250, 250, 250, 250, 250, \
        250,                     \
    }
#define COORD_TIMESLOTS                                                                                                       \
    {                                                                                                                         \
        SWC_MAIN_TIMESLOT(0),  SWC_MAIN_TIMESLOT(1),  SWC_MAIN_TIMESLOT(2),  SWC_MAIN_TIMESLOT(3),  SWC_MAIN_TIMESLOT(4),   \
        SWC_MAIN_TIMESLOT(5),  SWC_MAIN_TIMESLOT(6),  SWC_MAIN_TIMESLOT(7),  SWC_MAIN_TIMESLOT(8),  SWC_MAIN_TIMESLOT(9),   \
        SWC_MAIN_TIMESLOT(10), SWC_MAIN_TIMESLOT(11), SWC_MAIN_TIMESLOT(12), SWC_MAIN_TIMESLOT(13), SWC_MAIN_TIMESLOT(14),  \
        SWC_MAIN_TIMESLOT(15), SWC_MAIN_TIMESLOT(16), SWC_MAIN_TIMESLOT(17), SWC_MAIN_TIMESLOT(18), SWC_MAIN_TIMESLOT(19),  \
    }

#define NODE_TIMESLOTS                                                                                                        \
    {                                                                                                                         \
                                                                                          SWC_MAIN_TIMESLOT(20),              \
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

#define MAIN_CHANNEL_SWC_CCA_FB_TRY_COUNT     15

#endif /* SWC_CFG_H_ */
