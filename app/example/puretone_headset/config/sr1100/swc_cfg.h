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
 * Shipping value is SWC_ISI_MITIG_0, which is also what the zero-initialised node_cfg used to
 * give implicitly; naming it here makes it a knob instead of an accident. Higher levels insert
 * pauses between symbols and lengthen the preamble accordingly (swc_api.c:510), so they cost
 * airtime -- the same budget that already refused FEC 2.00 inside a 250 us timeslot.
 *
 * Level 2 is what close-range obstruction wanted. Obstruction is a non-line-of-sight case: the
 * direct path is gone and what arrives is reflections, so packets land corrupted rather than not
 * at all -- rx_rej climbing while rx_ok holds is the signature -- and that is precisely what ISI
 * mitigation is for. Level 1, which is what SPARK's reference demo ships, was not enough.
 *
 * Level 3 does not work here, and not by a small margin: it crackles with no obstruction at all.
 * The preamble belongs to the connection, not to a fallback mode, so it has to fit the largest
 * payload on the ladder -- mode 0's 242 B -- inside a 250 us slot. At level 3 it no longer
 * reliably does. Note the failure is audible corruption at the TOP of the ladder, not the red
 * LED at init that an unaffordable FEC gave: this one fits well enough to boot and link.
 *
 * Level 2 only pays off alongside the 4.6x accumulator on mode 3. Either one alone left the
 * dropouts unchanged: ISI raises the odds of decoding a single attempt through multipath, the
 * accumulator supplies enough attempts for those odds to cash in. Three attempts at good odds
 * and six attempts at bad odds both lose.
 *
 * Must be identical on the dongle and the headset: it changes the preamble both ends use to
 * find each other. */
#define NODE_ISI_MITIG SWC_ISI_MITIG_1

/* Specifies the schedule configuration. */
// clang-format off
#define SCHEDULE                 \
    {                            \
        250, 250, 250, 250, 250, \
        250, 250, 250, 250,      \
    }
#define COORD_TIMESLOTS                                                                           \
    {                                                                                             \
        MAIN_TIMESLOT(0), MAIN_TIMESLOT(1), MAIN_TIMESLOT(2),                   MAIN_TIMESLOT(4), \
        MAIN_TIMESLOT(5), MAIN_TIMESLOT(6), MAIN_TIMESLOT(7),                                     \
    }

#define NODE_TIMESLOTS                                                          \
    {                                                                           \
                                                              MAIN_TIMESLOT(3), \
                                                              MAIN_TIMESLOT(8), \
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
