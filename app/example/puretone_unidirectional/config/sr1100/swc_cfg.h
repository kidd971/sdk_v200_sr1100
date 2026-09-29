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
/* Pulse count for SR1100 -- pulses per transmitted symbol, 1 to 3, on every connection and in both
 * directions.
 *
 * The one TX energy knob outside the width/gain table, and the larger one: gain spans only 1.8 dB
 * end to end, while more pulses per symbol puts more energy into each one. Recorded in
 * MD/link_dropout_arms_ledger.md as the first candidate for range, then set aside only because
 * SPARK's demo also runs 1 -- it has never been measured on this link.
 *
 * It cannot be a rung-4-only setting. The fallback table does carry a tx_pulse_count, but the
 * node's rx_pulse_count is per connection and has no fallback counterpart, and it must match what
 * arrives. So it moves every rung, the data link and both ACKs together, and must be identical on
 * the coordinator and the node -- flash both ends.
 *
 * The Wireless Core has warnings for it (SWC_WARN_TX_PULSE_COUNT / _RX_PULSE_COUNT, "use at your
 * own risk"); they go to swc_warning_handler() and do not stop init. What more pulses cost in
 * airtime per 250 us slot is not documented in this package -- if a raised count crackles on the
 * top rungs with no obstruction, that is the ISI level-3 failure in another form.
 *
 * Overridable (-DSR1100_PULSE_COUNT=3) for a range arm; not assessed for emissions. */
#ifndef SR1100_PULSE_COUNT
#define SR1100_PULSE_COUNT 1
#endif

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
 * Level 1 is the value in force here, and the reason is dual radio: level 2 does not work with
 * two radios at all. Measured on the u5a5 with the headset application, a dual-radio node at
 * level 2 carries no audio -- only fallback mode 3 comes up, and only sometimes -- while the
 * same build at level 1 or level 0 runs normally. The pristine vendor tree at v2.4.0-rc2 runs
 * dual radio fine, and NODE_ISI_MITIG is the single line by which our swc_cfg.h differs from
 * theirs, so this is ours and not a vendor regression.
 *
 * The mechanism is the one described under level 3 below, arriving a level early. ISI
 * mitigation lengthens the preamble, the preamble belongs to the connection, and a dual-radio
 * schedule is tighter than a single-radio one -- so on two radios, level 2 behaves the way
 * level 3 behaves on one. That only mode 3 survives is the tell: it is the rung with the
 * smallest payload.
 *
 * Level 2 was previously in force, and it was the right choice at the time: on a SINGLE radio
 * it is what the near field needs, and level 1 was measured on this ladder with the
 * close-range dropouts coming back. What changed is not the measurement but the mechanism
 * available. Two antennas see different multipath, so dual radio attacks close-range
 * obstruction a different way than lengthening symbols does, and the pair of them at level 1
 * was measured to hold the near field -- 24 kHz stereo, dual radio, no dropouts under
 * obstruction. So the trade-off between near-field performance and dual radio dissolved
 * rather than having to be decided.
 *
 * This is why puretone_headset still defaults to level 2 and this application does not. That
 * line ships single radio, where level 2 is correct and there is nothing to conflict with.
 * The two defaults disagreeing is deliberate; do not "fix" it by aligning them.
 *
 * Level 2 was also briefly set to 1 once before, for an unrelated reason, while the crash
 * under sustained long-range obstruction was being chased -- the theory being that its longer
 * preamble did not fit beside a mode 3 the wider accumulator had grown to 100 B. That was the
 * right diagnosis and the wrong lever: taking mode 3 back to 54 B fixed the crash and let
 * level 2 stay at the time. Mentioned so the history is not mistaken for this decision.
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
 * find each other. A mismatched pair does not link, and that failure looks exactly like the
 * dual-radio one above, so flash both ends together when changing it. Overridable per build
 * (-DNODE_ISI_MITIG=SWC_ISI_MITIG_2) for an A/B arm.
 *
 * This default is what the dual-radio product needs, not what every preset runs. The u5a5
 * single-radio presets (unidir-u5a5-rjf-single-radio-isi2 and its sine siblings) set level 2 in
 * CMakePresets.json: an all-single-radio pair has nothing to conflict with, and level 2 is
 * what the near field wants there. It cannot be chosen per board automatically -- the DG is
 * always single radio and has no way to know which HS it will meet -- so it is chosen per
 * PAIR, by preset, and a u5a5 DG for a dual-radio HS has its own ISI-1 preset
 * (unidir-u5a5-rjf-DG-dual-radio). The consequence to remember: a u5a5 single-radio half no
 * longer links with a u535 half. */
#ifndef NODE_ISI_MITIG
#define NODE_ISI_MITIG SWC_ISI_MITIG_1
#endif

/* Length of every timeslot, in us. A knob so that ISI 2's airtime question can be asked without
 * an edit. Measured 2026-09-29 on a u5a5 single-radio pair at ISI 2: at 250 the ladder could not
 * hold any rung above 4; at 280 it climbs to 1 and 2, so the large payloads were not fitting.
 * But a flat ~35-45% of attempts still failed on EVERY rung, 48 B to ~200 B alike, at 10 cm and
 * slightly worse at 1 m -- a per-attempt loss that slot length does not reach, and the reason
 * ISI 2 is not shipped on this link. Backing out the level-4 pulse width did not move it either
 * (level-4 power, FBK4_TX_POWER_REF=0, was built but not measured). ISI 1 on the same pair was
 * clean.
 *
 * Not free. The coordinator's 20 slots go from 3810/s to 3401/s at 280, and attempts per packet
 * are slot rate over packet rate (see sac_cfg.h): mode 4 drops from 6.4 to 5.7, mode 3 from 3.7
 * to 3.3. Must be identical on both ends -- the schedule is what they meet on. */
#ifndef SWC_TIMESLOT_US
#define SWC_TIMESLOT_US 250
#endif

/* Specifies the schedule configuration. */
// clang-format off
#define SCHEDULE                                                                                  \
    {                                                                                             \
        SWC_TIMESLOT_US, SWC_TIMESLOT_US, SWC_TIMESLOT_US, SWC_TIMESLOT_US, SWC_TIMESLOT_US,      \
        SWC_TIMESLOT_US, SWC_TIMESLOT_US, SWC_TIMESLOT_US, SWC_TIMESLOT_US, SWC_TIMESLOT_US,      \
        SWC_TIMESLOT_US, SWC_TIMESLOT_US, SWC_TIMESLOT_US, SWC_TIMESLOT_US, SWC_TIMESLOT_US,      \
        SWC_TIMESLOT_US, SWC_TIMESLOT_US, SWC_TIMESLOT_US, SWC_TIMESLOT_US, SWC_TIMESLOT_US,      \
        SWC_TIMESLOT_US,                                                                          \
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

/* CCA settings.
 *
 * try_count is how many times the radio re-checks for a clear channel before giving up, and the
 * give-up action on this connection is SWC_CCA_ABORT_TX -- the packet is dropped, not delayed.
 * At 2 tries the whole assessment lasts about 9 us, so a channel that is busy for longer than
 * that costs an audio packet.
 *
 * MEASURED AND RULED OUT as the cause of the close-range dropouts, so this is back at the SDK
 * example's 2 rather than the 14 SPARK's own audio demo runs. Do not re-run this experiment
 * without a reason: raising it was tried precisely because 2 looked out of place beside the
 * fallback levels in swc_cfg_coord.h, which ask for 7 / 13 / 14 / 14, and because the demo uses
 * 14. The statistics settled it -- under close-range obstruction the node's rx_rej climbs, not
 * the coordinator's cca_fail.
 *
 * That distinction is the useful part. CCA is about the channel being occupied and its failure
 * drops the packet before it is sent; a rejected packet was sent, arrived, and could not be
 * decoded through multipath. Close-range obstruction is the second kind by construction -- the
 * direct path is blocked and what reaches the antenna is reflections at different delays, which
 * is inter-symbol interference -- so it is answered by ISI mitigation and by nothing else here.
 * It also explains why every retransmission increase left the near field unchanged: each retry
 * of a smeared packet is smeared the same way.
 *
 * Overridable, so putting 14 back for a comparison costs no edit. */
#define MAIN_CHANNEL_SWC_CCA_AUDIO_RETRY_TIME 96 /* 4.688 us CCA intervals. */
#ifndef MAIN_CHANNEL_SWC_CCA_AUDIO_TRY_COUNT
#define MAIN_CHANNEL_SWC_CCA_AUDIO_TRY_COUNT 2
#endif

#define MAIN_CHANNEL_SWC_CCA_DATA_RETRY_TIME  160 /* 7.813 us CCA intervals. */
#define MAIN_CHANNEL_SWC_CCA_DATA_TRY_COUNT   15

#define MAIN_CHANNEL_SWC_CCA_FB_TRY_COUNT     15

#endif /* SWC_CFG_H_ */
