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
 *     "Append only" means AFTER vendor_data, at the very end -- never in the gap before
 *     vendor_id, however tempting that looks now that the vendor block is cut off there.
 *     Inserting there shifts vendor_id/seq/len/data by the width of the new field, so a peer
 *     running the older build reads the new field AS vendor_id and hands the ODM's SOC a
 *     fabricated vendor command built from the bytes that follow. Silent misinterpretation,
 *     not a missing field -- and pointed straight at the one interface whose contents this
 *     firmware cannot sanity-check, because they are opaque by design.
 *
 *     The real answer is usually not to add a field at all. Vendor pass-through exists so
 *     that new information can cross this link without a wire format change; anything
 *     optional, occasional, or ODM-specific belongs there. Reach for a new field only for
 *     something the module itself needs in every packet.
 *
 *     If it really is that, append it after vendor_data and extend user_data_tx_size() to
 *     cover it. Note the cost honestly: a field beyond the vendor block can only be reached
 *     by transmitting the vendor block too, so an always-present field there makes every
 *     packet full length and gives up the saving user_data_tx_size() exists for. That is the
 *     price of the layout, and it is the right way round -- it costs airtime, whereas the
 *     tempting alternative costs correctness.
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
#include <stddef.h>
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
 *  There is no framing, no length prefix and no sequence number at this level: a packet is
 *  simply the first N bytes of this struct, and the receiver relies on rule 3 to read the
 *  rest as absent. Fields are of two kinds, and which kind a field is determines whether it
 *  survives packet loss:
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
    /*! Last vendor sequence number the SENDER of this packet received from the peer, echoed
     *  in every packet. This is the acknowledgement for commands sent with the ack flag.
     *  A level rather than a one-shot: an acknowledgement transmitted once would be exactly
     *  as losable as the command it acknowledges. 0 = nothing received yet.
     *
     *  Sits ahead of the vendor command fields because it is the only one of them that has
     *  to go out in every packet; everything below is omitted when there is no command. */
    uint8_t vendor_ack;

    /* ---- Everything below here is sent ONLY when a vendor command is present. ---- */

    /*! Vendor pass-through: ODM-defined command id, or AT_VENDOR_ID_NONE (0) for "none".
     *  Opaque to this firmware -- see at_cmd_core.h. When the packet is truncated before this
     *  field, the receiver's zero-initialized copy reads 0, which already means "none" -- so
     *  a short packet needs no flag to say it is short. */
    uint8_t vendor_id;
    /*! Sequence number for the vendor command, used by the receiver to drop the repeats. */
    uint8_t vendor_seq;
    /*! Vendor payload length, 0..AT_VENDOR_PAYLOAD_MAX. */
    uint8_t vendor_len;
    /*! Vendor payload. Only the first vendor_len bytes are transmitted. */
    uint8_t vendor_data[AT_VENDOR_PAYLOAD_MAX];
} user_data_t;

_Static_assert(sizeof(user_data_t) <= MAX_DATA_PAYLOAD_SIZE,
               "user_data_t no longer fits the data connection payload");
_Static_assert(offsetof(user_data_t, vendor_data) + AT_VENDOR_PAYLOAD_MAX == sizeof(user_data_t),
               "the vendor payload must stay last -- see user_data_tx_size()");

/** @brief How many bytes of a packet to actually transmit.
 *
 *  The vendor block is the large part of this struct and is empty in almost every packet, so
 *  it is not transmitted unless there is something in it. Sending the full struct regardless
 *  would put the whole vendor payload on the air 100 times a second to carry nothing.
 *
 *  This matters more than it looks. These packets go out every 10 ms in both directions, and
 *  the data connection shares its timeslots with the back-channel audio, so payload length is
 *  airtime taken from an allocation that is already tight -- the same budget the fb=0 park
 *  investigation traced its margin problems to. Adding the vendor block unconditionally would
 *  have taken this packet from 4 bytes to 14 forever; this keeps the common case at 5.
 *
 *  Three lengths result:
 *    - no vendor command  -> through vendor_ack, 5 bytes (one more than before the vendor
 *      block existed; the acknowledgement is a level and has to be in every packet)
 *    - command, no payload -> through vendor_len, 8 bytes
 *    - command with payload -> 8 + vendor_len, at most 14
 *
 *  The receiver needs no length flag: it zero-initializes its copy and wireless_read_data()
 *  fills only what arrived, so a truncated packet leaves vendor_id at 0, which already means
 *  "no vendor command". The rule that every field's zero means "absent" is what makes
 *  variable-length transmission free.
 *
 *  @param[in] packet  Fully built packet.
 *  @return Number of leading bytes to hand to wireless_send_data().
 */
static inline uint8_t user_data_tx_size(const user_data_t *packet)
{
    uint8_t len;

    if (packet->vendor_id == AT_VENDOR_ID_NONE) {
        return (uint8_t)offsetof(user_data_t, vendor_id);
    }

    /* Clamp before trusting it as a length. vendor_len is validated where the command is
     * parsed, but this is the value that decides how far into the struct we read. */
    len = (packet->vendor_len > AT_VENDOR_PAYLOAD_MAX) ? AT_VENDOR_PAYLOAD_MAX : packet->vendor_len;

    return (uint8_t)(offsetof(user_data_t, vendor_data) + len);
}

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
