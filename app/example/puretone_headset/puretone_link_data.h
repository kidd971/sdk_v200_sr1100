/** @file  puretone_link_data.h
 *  @brief Wire format of the 10 ms user-data packet exchanged over the UWB data connection.
 *
 *  Shared by puretone_headset.c (node) and puretone_dongle.c (coordinator). It exists
 *  because those two files used to carry two hand-copied definitions of this struct, kept in
 *  step by hand. Editing one and not the other compiles cleanly, links cleanly, and then
 *  misreads every field after the divergence point -- the symptom is parameters arriving as
 *  garbage, with each side looking correct in isolation. Both ends must agree byte for byte,
 *  so there is exactly one definition.
 *
 *  Rules for changing this struct:
 *
 *  1. APPEND ONLY. Never reorder or remove a field. wireless_read_data() copies the common
 *     prefix when the two ends disagree on length, which is what lets a mixed-version pair
 *     degrade to "the fields both builds know about" instead of failing. Reordering turns
 *     that graceful degradation into silent misinterpretation.
 *
 *  2. EVERY FIELD IS uint8_t (or bool, which is one byte). The struct is memcpy'd straight
 *     onto the air with no packing attribute, so it must have no padding. A uint16_t or
 *     uint32_t member would introduce alignment padding whose contents are undefined.
 *
 *  3. ZERO MEANS ABSENT. Both ends zero-initialize their local copy, and a receiver with a
 *     longer struct than the sender leaves the trailing fields at zero. So the zero value of
 *     every new field must mean "nothing here" -- never a legitimate value, and never a
 *     benign-looking default. See at_cmd_bidir_decision_spec.md section 6 for why this
 *     matters most for alarm-carrying fields: reporting "I don't know" as "everything is
 *     fine" is the failure this rule exists to prevent.
 *
 *  4. IT MUST STILL FIT IN MAX_DATA_PAYLOAD_SIZE. The static assert below enforces it.
 *     Raising that limit is not a free edit: it lengthens the frame and eats airtime in a
 *     data timeslot that is already tight.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 */
#ifndef PURETONE_LINK_DATA_H_
#define PURETONE_LINK_DATA_H_

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "at_cmd_core.h"  /* AT_VENDOR_PAYLOAD_MAX: the vendor payload budget is an AT-level limit */

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Maximum payload the data connection is configured for, in bytes.
 *
 *  This is the max_payload_size given to both data connections at init. Changing it changes
 *  the frame length on air, so it is a scheduling decision, not a struct-sizing convenience.
 */
#define MAX_DATA_PAYLOAD_SIZE 16

/** @brief The 10 ms status/command packet, identical in both directions.
 *
 *  Every field is sent in every packet; there is no framing, no length prefix and no
 *  sequence number at this level. Fields are therefore of two kinds, and which kind a field
 *  is determines whether it survives packet loss:
 *
 *    - LEVEL fields (button_state, link_margin, battery_pct) carry current state and are
 *      resent every packet, so a lost packet self-corrects 10 ms later.
 *    - EDGE fields (cmd_type) are set once, transmitted once and cleared, so a lost packet
 *      loses the event outright. There is no ack, no retry and no sequence number on this
 *      path, and the sender clears the field before it knows whether the send succeeded.
 *
 *  Prefer a level field for anything new. An edge field is only acceptable where a human is
 *  in the loop to retry it -- which is exactly why the media keys can live with being edge
 *  triggered and a fall alarm cannot.
 */
typedef struct user_data {
    /*! A boolean indicating the button's state. */
    bool button_state;
    /*! The link margin to monitor link quality. */
    uint8_t link_margin;
    /*! Pending command, an at_cmd_code_t: 0=none, 1=next_track, 2=pre_track, 3=play, 4=stop.
     *  Edge triggered: cleared by the sender immediately after packing. The values are on the
     *  wire and frozen; see at_cmd_code_t in at_cmd_core.h for why.
     */
    uint8_t cmd_type;
    /*! Battery level of the node (0-100%). */
    uint8_t battery_pct;
    /*! Vendor pass-through: ODM-defined command id, or AT_VENDOR_ID_NONE (0) for "none".
     *  Opaque to this firmware -- see at_cmd_core.h. */
    uint8_t vendor_id;
    /*! Sequence number for the vendor command, used by the receiver to drop the repeats. */
    uint8_t vendor_seq;
    /*! Vendor payload length, 0..AT_VENDOR_PAYLOAD_MAX. */
    uint8_t vendor_len;
    /*! Vendor payload. */
    uint8_t vendor_data[AT_VENDOR_PAYLOAD_MAX];
    /*! Last vendor sequence number the SENDER of this packet received from the peer, echoed
     *  in every packet. This is the acknowledgement for commands sent with the ack flag.
     *  A level rather than a one-shot: an acknowledgement transmitted once would be exactly
     *  as losable as the command it acknowledges. 0 = nothing received yet. */
    uint8_t vendor_ack;
} user_data_t;

_Static_assert(sizeof(user_data_t) <= MAX_DATA_PAYLOAD_SIZE,
               "user_data_t no longer fits the data connection payload");

/** @brief Copy the outgoing vendor block into a packet being built.
 *
 *  Always writes all five vendor fields. Note that this is NOT conditional on there being a
 *  command to send: the acknowledgement field has to go out in every packet, including the
 *  overwhelming majority that carry no command, or acknowledgements would be as easy to lose
 *  as the commands they confirm.
 *
 *  Lives here, next to the struct, rather than being written out in each application: this is
 *  the mapping between the AT-level frame and the wire layout, and a mapping that exists in
 *  two hand-written copies is the exact mistake that merging the two struct definitions was
 *  meant to end.
 *
 *  @param[in,out] packet  Packet under construction.
 */
static inline void user_data_pack_vendor(user_data_t *packet)
{
    at_vendor_frame_t frame;

    at_cmd_core_vendor_tx_fill(&frame);

    packet->vendor_id  = frame.id;
    packet->vendor_seq = frame.seq;
    packet->vendor_len = frame.len;
    packet->vendor_ack = frame.ack_seq;
    memcpy(packet->vendor_data, frame.data, sizeof(packet->vendor_data));
}

/** @brief Hand a received packet's vendor block to the AT core, which de-duplicates the
 *         command, delivers it to the local SOC, and records the peer's acknowledgement.
 *
 *  Called for every received packet, with no "is there a command" test: a packet carrying no
 *  command still carries the peer's acknowledgement of ours.
 *
 *  @param[in] packet  Received packet.
 */
static inline void user_data_deliver_vendor(const user_data_t *packet)
{
    at_vendor_frame_t frame;

    frame.id      = packet->vendor_id;
    frame.seq     = packet->vendor_seq;
    frame.len     = packet->vendor_len;
    frame.ack_seq = packet->vendor_ack;
    memcpy(frame.data, packet->vendor_data, sizeof(frame.data));

    at_cmd_core_notify_vendor_received(&frame);
}

#ifdef __cplusplus
}
#endif

#endif /* PURETONE_LINK_DATA_H_ */
