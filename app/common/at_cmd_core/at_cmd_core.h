/** @file  at_cmd_core.h
 *  @brief Common AT command core — shared across all applications.
 *
 *  Wraps at_module with the expansion UART backend and registers a set of
 *  built-in commands available in every application:
 *
 *    AT+VER         — firmware / SDK version string
 *    AT+STATUS      — system uptime in milliseconds
 *    AT+HELP        — list all registered AT commands
 *    AT+PING            — connectivity check, responds OK
 *    AT+MODULE_INFO?    — HW model, FW version, chip version, IC serial, device address
 *    AT+LE_UWB_CONN_STATUS? — current UWB connection status (0=Standby,1=Pairing,2=Connected)
 *    AT+LE_UWB_PAIR        — trigger UWB pairing (invokes registered pair callback)
 *
 *  Applications that need additional commands call at_server_register()
 *  directly after at_cmd_core_init().
 *
 *  Usage:
 *    1. Call at_cmd_core_init() once during application startup.
 *    2. Optionally register app-specific commands with at_server_register().
 *    3. Call at_cmd_core_process() every iteration of the main loop.
 */
#ifndef AT_CMD_CORE_H_
#define AT_CMD_CORE_H_

#include <stdbool.h>
#include <stdint.h>
#include "fw_version.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Baud rate used for the expansion UART AT channel. */
#define AT_CMD_CORE_BAUD_RATE  115200

/** @brief Version + compile timestamp identifying the exact binary.
 *
 *  Deliberately a macro rather than a string built inside at_cmd_core.c: __DATE__/__TIME__
 *  expand at the CALL site, so the timestamp comes from the application's translation unit
 *  (puretone_dongle.c / puretone_headset.c). An incremental build that recompiles only the
 *  application would otherwise leave at_cmd_core.o -- and therefore the reported timestamp --
 *  stale, which defeats the whole point of printing it. Expanding at the call site also keeps
 *  it identical to the HS crash-dump build line, which uses the same two macros.
 */
#define AT_CMD_CORE_BUILD_ID  FW_VERSION_STRING " " __DATE__ " " __TIME__

/** @brief UWB connection status codes reported by AT+LE_UWB_CONN_STATUS?. */
typedef enum {
    AT_UWB_CONN_STATUS_STANDBY    = 0, /*!< Idle, not yet started. */
    AT_UWB_CONN_STATUS_PAIRING    = 1, /*!< Pairing procedure in progress. */
    AT_UWB_CONN_STATUS_CONNECTED  = 2, /*!< Link established and running. */
    AT_UWB_CONN_STATUS_CONNECTING = 3, /*!< Connection attempt in progress (after AT+LE_UWB_CONNECT). */
} at_uwb_conn_status_t;

/** @brief Timeout for a UWB connection attempt, whoever opened it.
 *
 *  If the link is not established within this many milliseconds of the status going to
 *  CONNECTING, a +EVENT: LE_UWB_CONNECT_FAIL notification is sent.
 *
 *  Kept equal to the applications' RECONNECT_TIMEOUT_MS: both AT+LE_UWB_CONNECT (which resets
 *  the MCU into boot auto-reconnect) and a plain boot land in the same reconnect window, and
 *  a shorter value here made the host see CONNECT_FAIL while the application was still
 *  trying. The status is stamped just before the window opens, so CONNECT_FAIL is still
 *  reported before the application gives up and powers down with UWB_DISCONNECTED.
 */
#define AT_UWB_CONNECT_TIMEOUT_MS  10000

/** @brief How long the link must read down before +EVENT: LE_UWB_DISCONNECTED is sent.
 *
 *  Only the down edge is debounced; a link that comes back is reported immediately.
 *
 *  The Wireless Core calls a connection disconnected after 20 ms of lost frames, which is the
 *  right threshold for the audio pipeline but far too twitchy to hand a host as a connection
 *  event: any brief stall in the audio source empties the TX queue, the coordinator's audio
 *  timeslots go silent, and 20 ms later the status flips even though the peer never went
 *  anywhere. Without this the host saw DISCONNECTED/CONNECTED pairs on every pause.
 *
 *  Chosen an order of magnitude above the Wireless Core's own threshold so a real drop is
 *  still reported promptly -- the host learns about a peer that walked away within half a
 *  second, which is well inside human reaction time for an audio device.
 */
#define AT_UWB_DISCONNECT_DEBOUNCE_MS  400

/** @brief Fallback rung at which the link is reported WEAK, and at which it is reported CRITICAL.
 *
 *  The quality report is driven by the fallback rung, not by link margin, and the reason is
 *  measured rather than preferred. Range on v240 is 32.9 m unobstructed against 3.5 m blocked
 *  -- a 19.5 dB step -- so inside the 5x5 m room the PRD specifies, distance never takes the
 *  link down and blocking always does, in one step. There is no gradual slope for a margin
 *  threshold to sit on: the margin reads near its ceiling everywhere in the usable envelope
 *  and then the link is gone. The rung is the only quantity that moves in between, because
 *  the ladder descends on the coordinator's transmit queue backing up, which happens before
 *  the node misses its first packet.
 *
 *  WEAK at rung 2 and CRITICAL at rung 3 rather than one rung later each: with 33 m of
 *  unobstructed reach, any descent indoors is already evidence of an obstruction rather than
 *  of distance, and waiting for the bottom rung means waiting for the step to finish. The
 *  cost of being early is a Bluetooth path warmed and not used.
 *
 *  The ladder can descend one rung per 10 Hz sample, so rung 2 to rung 4 is ~200 ms. That is
 *  the whole warning budget this product can offer, which is why the host must keep Bluetooth
 *  ready rather than start preparing it on WEAK. See MD/uwb_quality_indicator_decision_spec.md.
 */
#define AT_UWB_QUALITY_WEAK_RUNG      2
#define AT_UWB_QUALITY_CRITICAL_RUNG  3

/** @brief Interval (ms) between quality evaluations while connected.
 *
 *  The rung itself does not wait for this -- a rung change is reported through
 *  at_cmd_core_notify_fb_rung_change() as it happens. This interval only paces the dropout
 *  backstop, which counts events per unit time and so needs a window.
 */
#define AT_UWB_QUALITY_CHECK_INTERVAL_MS   1000

/** @brief How long after CONNECTED before quality is evaluated at all.
 *
 *  Matched to the coordinator's LADDER_SETTLE_MS. A link that has just come up has a transmit
 *  queue full of warm-up rather than evidence, and the ladder is held for that long on purpose;
 *  reporting the rung during the hold would report whatever it was before the outage.
 */
#define AT_UWB_QUALITY_SETTLE_MS   2000

/** @brief How long an improvement must hold before it is reported, per level.
 *
 *  Deliberately asymmetric with the immediate report of a degradation. Going down costs the
 *  host one source switch; coming back up too eagerly costs it another one straight after,
 *  and the bottom of the ladder oscillates -- it climbs, fails and falls again. That used to
 *  be hidden by FALLBACK_PIN_AT_BOTTOM in sac_cfg.h, which is off by default since 2026-09-17
 *  precisely because a pinned rung left this recovery path unreachable, so this window is now
 *  the only thing damping it. One level per window, so a recovery from CRITICAL to GOOD takes
 *  two.
 */
#define AT_UWB_QUALITY_RECOVER_MS  5000

/** @brief Link quality reported by AT+CONN_QUALITY? and +EVENT: LE_UWB_QUALITY.
 *
 *  Ordered worst-last on purpose: the state machine compares these to decide whether a new
 *  reading is a degradation (report at once) or an improvement (make it hold first), and
 *  steps one level at a time when recovering.
 */
typedef enum {
    AT_LINK_QUALITY_UNKNOWN  = 0, /*!< Not connected, or inside the post-connect settle window.
                                   *   Reported as N/A; no event is sent for it. */
    AT_LINK_QUALITY_GOOD     = 1, /*!< Rung 0-1. Nothing to do. */
    AT_LINK_QUALITY_WEAK     = 2, /*!< Rung 2. Bluetooth should already be ready to sound. */
    AT_LINK_QUALITY_CRITICAL = 3, /*!< Rung 3+, or audio has dropped out. Switch. */
} at_link_quality_t;

/** @brief Device role codes reported by AT+LE_UWB_GET_ROLE?. */
typedef enum {
    AT_DEVICE_ROLE_NODE        = 0, /*!< Node (leaf) device. */
    AT_DEVICE_ROLE_COORDINATOR = 1, /*!< Coordinator device. */
} at_device_role_t;

/** @brief Command codes carried in the user_data_t cmd_type field over the UWB data channel.
 *
 *  THE VALUES 1-4 ARE ON THE WIRE AND CANNOT BE CHANGED. An ODM host MCU is already
 *  integrated against them on the HS->DG path, and its firmware release schedule is not ours
 *  to control, so a renumbering opens a window in which the two sides disagree. The failure
 *  mode in that window is not "nothing happens" -- it is silently executing the wrong
 *  command: ask for PLAY, get STOP. In the field that looks like a hardware fault, and the
 *  numbering is the last thing anyone would suspect.
 *
 *  These deliberately do NOT match the 0x01-0x05 table this file's handlers used to pass to
 *  the DG->HS forwarding callback (VOL=1, PLAY=2, STOP=3, NEXT=4, PRE=5). That table was
 *  written for a forwarding path that was never enabled -- no application ever registered
 *  the callback -- so it never reached a wire. Where a documented-but-dead encoding conflicts
 *  with an undocumented-but-shipping one, the shipping one is the fact. See
 *  MD/at_cmd_bidir_decision_spec.md section 5.
 *
 *  AT_CMD_VOL is the one value still free to choose: volume is not forwarded over UWB by
 *  either side today (the DG applies AT+VOL to its own back channel; see
 *  at_cmd_core_register_vol_cb()), so no wire has ever carried it.
 */
typedef enum {
    AT_CMD_NONE       = 0, /*!< No command in this packet. Must stay 0: user_data_t is
                            *   zero-initialized and reads 0 when no packet has arrived, so
                            *   0 has to mean "absent" rather than being a real command. */
    AT_CMD_NEXT_TRACK = 1, /*!< On the wire, do not change. */
    AT_CMD_PRE_TRACK  = 2, /*!< On the wire, do not change. */
    AT_CMD_PLAY       = 3, /*!< On the wire, do not change. */
    AT_CMD_STOP       = 4, /*!< On the wire, do not change. */
    AT_CMD_VOL        = 5, /*!< Reserved, not yet forwarded over UWB. Value still free. */
} at_cmd_code_t;

/** @brief Maximum vendor pass-through payload, in bytes.
 *
 *  Sized to what is left in the 10 ms packet, not to a requirement, because there was no
 *  requirement to size it to -- that is the point of a pass-through. Raising it means either
 *  taking bytes from a future field or raising the connection's max_payload_size, and the
 *  latter lengthens the frame and eats airtime in a data timeslot that is already tight. If a
 *  vendor command ever needs more than this, chunk it across packets using the sequence
 *  number rather than growing the frame.
 *
 *  8 is the largest value that fits MAX_DATA_PAYLOAD_SIZE without raising it: 5 bytes of
 *  always-present fields, 3 of vendor header, 8 of payload. It is an upper bound, not a
 *  reservation -- user_data_tx_size() transmits offsetof(vendor_data) + vendor_len, so a
 *  one-byte command still puts nine bytes on the air whatever this is set to. Raising the
 *  ceiling therefore costs nothing until somebody actually uses the room.
 *
 *  Chosen generously on purpose, because the two limits behave differently over time:
 *
 *    - THIS one cannot be raised compatibly. An older peer clamps a longer payload down to
 *      its own maximum, which is a defence against overrunning data[] and no defence at all
 *      against misreading it -- the ODM receives a truncated command and parses it as if it
 *      were whole. So it has to be right before an ODM integrates, like at_cmd_code_t.
 *    - MAX_DATA_PAYLOAD_SIZE can be raised later without breaking anything, because an older
 *      peer simply truncates what it does not understand. That is the escape hatch if a
 *      future always-present field ever needs room beyond this 16-byte packet.
 *
 *  So spend the free space here now and leave the other limit for later, not the reverse.
 */
#define AT_VENDOR_PAYLOAD_MAX  8

/** @brief Reserved vendor command id meaning "no vendor command in this packet".
 *
 *  Must stay 0 and must never be handed to an ODM as a usable id. The packet struct is
 *  zero-initialized at both ends and reads as all zeros when nothing has been received, so if
 *  0 were a legal command id an empty packet would be indistinguishable from that command.
 */
#define AT_VENDOR_ID_NONE      0

/** @brief Highest vendor command id an ODM may allocate.
 *
 *  1..223 belong to the ODM; the module neither interprets nor reserves anything inside that
 *  range. AT+VENDOR_CMD rejects anything above it with RESERVED_ID.
 */
#define AT_VENDOR_ID_ODM_MAX   223

/** @brief First vendor command id reserved for the module's own use.
 *
 *  224..255 (32 ids) are held back for future in-band module traffic -- payload chunking, a
 *  link-level probe, whatever turns out to be needed. None are used today.
 *
 *  Reserved NOW because this is the only moment it is free. The ODM allocates ids from its
 *  own documents; once it has handed some out, taking any back means coordinating a change
 *  across two host firmwares we do not control. Giving away all 255 and asking for some back
 *  later is a trade with no good version of itself.
 *
 *  The reservation is only real because this build ALREADY ignores these ids on receive.
 *  Rejecting them at AT+VENDOR_CMD stops an ODM adopting them; dropping them in
 *  at_cmd_core_notify_vendor_received() is what stops a shipped unit from forwarding a future
 *  firmware's internal traffic to its host as a bogus vendor event. A reservation that only
 *  the sender honours would be worth nothing the first time the two ends run different
 *  builds -- which is exactly when the reserved ids would start appearing.
 */
#define AT_VENDOR_ID_RESERVED  224

/** @brief Reserved sequence number meaning "no vendor frame" / "nothing to acknowledge".
 *
 *  Sequence numbers run 1..255 and skip 0 on wrap for the same reason ids do: the ack field
 *  is zero in every packet from a peer that has not received anything, and that must not be
 *  mistaken for an acknowledgement of sequence 0.
 */
#define AT_VENDOR_SEQ_NONE     0

/** @brief How many consecutive packets a best-effort vendor command is transmitted in.
 *
 *  Applies only to commands sent WITHOUT the acknowledgement flag. Three is a starting point
 *  chosen from "loss probability cubed", not from a measurement of this link's actual packet
 *  loss under fallback. If best-effort vendor commands are seen to go missing in the field,
 *  measure the loss rate before raising this -- a larger number without a measurement just
 *  moves the same unknown further away. Commands that genuinely must arrive should ask for
 *  an acknowledgement instead of buying more repeats.
 */
#define AT_VENDOR_TX_REPEAT    3

/** @brief How many packets an acknowledged vendor command is retransmitted for before it is
 *         declared failed. At the 10 ms data period this is the timeout in centiseconds.
 *
 *  100 packets = ~1 s. Chosen to outlast a brief RF dropout but to fail well inside a human's
 *  patience, because the whole point of asking for an acknowledgement is to be TOLD. A
 *  timeout long enough to cover a real disconnection would just be a slower way of never
 *  answering.
 */
#define AT_VENDOR_ACK_TIMEOUT_PACKETS  100

/** @brief How many vendor commands may be queued for transmission at once.
 *
 *  A best-effort command occupies the vendor field for AT_VENDOR_TX_REPEAT packets (~30 ms);
 *  an acknowledged one holds it until acked or timed out (up to ~1 s). Beyond this depth
 *  AT+VENDOR_CMD returns an error rather than silently dropping, so the host can back off
 *  instead of believing it succeeded.
 */
#define AT_VENDOR_TX_PENDING_MAX  4

/** @brief The vendor block as it travels in the periodic data packet.
 *
 *  The module assigns no meaning to id or data. Both are defined by the two host SOCs
 *  between themselves, which is the whole purpose: adding a command becomes their release,
 *  not ours. The module only guarantees delivery semantics -- see
 *  at_cmd_core_vendor_tx_fill() for exactly what those are.
 *
 *  data is ONE opaque byte string, not a parameter list. The module cannot offer multiple
 *  parameters honestly: with a single length field it would have to concatenate them on the
 *  way out and could not tell the receiver where to split them again, so the peer would
 *  receive one blob where two were sent. Per-parameter length bytes would fix that at a cost
 *  of one byte each out of six. Any internal structure therefore belongs to the ODM's
 *  definition of that id, where both of its host SOCs already agree on it.
 */
typedef struct {
    uint8_t id;                        /*!< 1-255, allocated by the ODM. 0 means "none". */
    uint8_t seq;                       /*!< 1-255, assigned by the core. 0 means "none". */
    uint8_t len;                       /*!< 0..AT_VENDOR_PAYLOAD_MAX. */
    uint8_t data[AT_VENDOR_PAYLOAD_MAX];  /*!< Opaque to the module. */
    uint8_t ack_seq;                   /*!< Piggybacked: the last seq THIS device received
                                        *   from the peer, echoed in every packet whether or
                                        *   not this packet carries a command. Being a level
                                        *   rather than a one-shot is what makes the
                                        *   acknowledgement itself survive packet loss. */
} at_vendor_frame_t;

/**
 * @brief Initialize the AT command core.
 *
 * Initializes the expansion UART, the AT module, and registers the built-in
 * commands (VER, STATUS, HELP).
 */
void at_cmd_core_init(void);

/**
 * @brief Drive the AT command state machine.
 *
 * Must be called every iteration of the application main loop.
 */
void at_cmd_core_process(void);

/**
 * @brief Update the UWB connection status reported by AT+LE_UWB_CONN_STATUS?.
 *
 * Call this whenever the application state machine transitions state.
 * Default value before any call is AT_UWB_CONN_STATUS_STANDBY.
 *
 * @param[in] status  New connection status.
 */
void at_cmd_core_set_uwb_conn_status(at_uwb_conn_status_t status);

/**
 * @brief Set the local device address reported by AT+MODULE_INFO?.
 *
 * Call this after swc_setup() with the node's local_address.
 * If not called, address is reported as 0xFF.
 *
 * @param[in] addr  Local device address from swc_node_cfg_t.local_address.
 */
void at_cmd_core_set_device_address(uint8_t addr);

/**
 * @brief Set the device role reported by AT+LE_UWB_GET_ROLE?.
 *
 * Call once during application startup (before the main loop).
 * Node applications pass AT_DEVICE_ROLE_NODE; coordinator applications
 * pass AT_DEVICE_ROLE_COORDINATOR.
 *
 * @param[in] role  Device role.
 */
void at_cmd_core_set_device_role(at_device_role_t role);

/**
 * @brief Send +EVENT: BUILD: <version> <date> <time> role=<DG|HS> to the external MCU.
 *
 * Boot banner. Call once immediately before at_cmd_core_notify_uwb_ready(), after
 * at_cmd_core_set_device_role() so the role is already known.
 *
 * Two things it buys on a customer board where no ST-Link is wired: the host can confirm
 * which binary is actually running (a mismatched timestamp means the wrong image was
 * flashed), and a line that repeats every ~10 s means the module is resetting rather than
 * sitting in the boot-reconnect idle state -- the two are otherwise indistinguishable over
 * the AT port, since a reconnect timeout on the DG is silent.
 *
 * Deliberately a separate line from UWB_READY, which stays byte-for-byte as it was so a
 * host matching that line exactly keeps working.
 *
 * @param[in] build_id  Build identifier, normally AT_CMD_CORE_BUILD_ID. NULL is ignored.
 */
void at_cmd_core_notify_build(const char *build_id);

/**
 * @brief Send +EVENT: LE_UWB_READY to the external MCU.
 *
 * Call once after SWC initialization is complete and the device is ready
 * to connect or accept pairing.
 */
void at_cmd_core_notify_uwb_ready(void);

/**
 * @brief Send +EVENT: LE_UWB_UNPAIRED to the external MCU.
 *
 * Call from the application's unpair path, after the persisted pairing address has been
 * erased. Tells the host the difference the link events cannot express: UWB_DISCONNECTED
 * means the peer is unreachable but still remembered, so AT+LE_UWB_CONNECT is worth sending;
 * UWB_UNPAIRED means the stored address is gone, so only AT+LE_UWB_PAIR can get the link back.
 *
 * Additive: the status is left alone, so the normal poll still emits UWB_DISCONNECTED
 * afterwards and a host that only knows that event is unaffected.
 */
void at_cmd_core_notify_unpaired(void);

/**
 * @brief Send +EVENT: LE_UWB_PAIRING and move the status to Pairing for the procedure.
 *
 * Call when entering pairing. Link polling is suspended while the status is Pairing, so the
 * torn-down wireless core is not misreported as a dropped link. Always pair this with
 * at_cmd_core_notify_pairing_result(), which is what releases the status again.
 *
 * The event is what lets the host show a pairing indication: the pairing call blocks for the
 * whole window with the status parked, so UWB_PAIRING and the eventual UWB_PAIRED /
 * UWB_PAIR_FAIL are the only two things the host hears about the procedure.
 */
void at_cmd_core_notify_pairing_started(void);

/**
 * @brief Report the outcome of pairing and release the status.
 *
 * Sends +EVENT: LE_UWB_PAIRED or +EVENT: LE_UWB_PAIR_FAIL and returns the status to Standby. On
 * success the link still has to come up; the usual poll reports that with UWB_CONNECTED.
 *
 * @param[in] success  true if the peer was paired, false on timeout / abort / failure.
 */
void at_cmd_core_notify_pairing_result(bool success);

/**
 * @brief Set the status to Standby and send +EVENT: LE_UWB_DISCONNECTED then +EVENT: LE_UWB_STANDBY.
 *
 * Call immediately before powering the module down (facade_enter_standby()), and only there.
 * The normal status machine cannot report this: entering Standby does not return, so
 * at_cmd_core_process() never runs again and the host would only see the UART fall silent,
 * which is indistinguishable from a crash.
 *
 * UWB_STANDBY is what separates this from an ordinary drop. UWB_DISCONNECTED on its own
 * comes from the link poll and means the peer is gone while the module stays up and
 * re-syncs by itself, so the host should keep talking to it. Here it means the opposite:
 * this UART stops answering on the next line, and only a reset (SoC NRST, or a WKUP button)
 * brings it back. Both writes block until the bytes are on the wire, so they survive the
 * power-down that follows.
 */
void at_cmd_core_notify_standby(void);

/**
 * @brief Register a connection status getter polled by at_cmd_core_process().
 *
 * The callback returns true when the UWB link is up, false otherwise.
 * at_cmd_core_process() polls this every main-loop iteration and sends
 * +EVENT: LE_UWB_CONNECTED or +EVENT: LE_UWB_DISCONNECTED only when the status
 * changes. Polling is skipped while status is AT_UWB_CONN_STATUS_PAIRING.
 *
 * Typical usage (call once after app_init()):
 *   at_cmd_core_register_link_status_cb(my_get_link_status);
 *
 * @param[in] cb  Function returning true if link is up. May be NULL to unregister.
 */
void at_cmd_core_register_link_status_cb(bool (*cb)(void));

/**
 * @brief Register a callback invoked when AT+LE_UWB_PAIR is received.
 *
 * The callback should initiate the application's pairing procedure.
 * If no callback is registered the command still returns OK but does nothing.
 *
 * @param[in] cb  Function to call on AT+LE_UWB_PAIR. May be NULL to unregister.
 */
void at_cmd_core_register_pair_cb(void (*cb)(void));

/**
 * @brief Register a callback invoked when AT+I2S_MUX is received.
 *
 * The callback receives the new selection (false = ON_BOARD, true = EXT) and
 * should apply it to hardware (e.g. quasar_audio_set_i2s_mux_selection()).
 * The command toggles the selection on each call and replies with the new state.
 * If no callback is registered the command still toggles the tracked state
 * but does not touch hardware.
 *
 * @param[in] cb  Function accepting new selection. May be NULL to unregister.
 */
void at_cmd_core_register_i2s_mux_cb(void (*cb)(bool use_ext));

/**
 * @brief Register a link margin getter polled by AT+CONN_LM?.
 *
 * The callback should call swc_connection_update_stats() on a connection that receives
 * unconditionally -- the data connection, not the audio one, whose margin goes to zero
 * whenever the source pauses -- and return whole dB (the stats are in tenths).
 * Set to NULL to unregister (e.g. on unpair); the command then returns N/A.
 *
 * Diagnostic only. The quality report deliberately does not read this: see
 * at_cmd_core_register_fb_rung_cb() and MD/uwb_quality_indicator_decision_spec.md.
 *
 * @param[in] cb  Function returning link margin in dB. May be NULL to unregister.
 */
void at_cmd_core_register_link_margin_cb(int32_t (*cb)(void));

/**
 * @brief Register a getter for the current audio fallback rung.
 *
 * Drives +EVENT: LE_UWB_QUALITY and AT+CONN_QUALITY?. The callback should return
 * sac_fallback_get_current_mode() for the application's main-channel fallback instance, and
 * must be safe to call at any time -- including before the audio core exists, where it should
 * return 0 rather than reach into an uninitialised instance.
 *
 * Both roles register this. The coordinator owns the ladder; the node mirrors it from the
 * received packet header, which is a rung behind only for as long as no packet arrives.
 *
 * @param[in] cb  Function returning the current rung index. May be NULL to unregister.
 */
void at_cmd_core_register_fb_rung_cb(uint8_t (*cb)(void));

/**
 * @brief Register a getter for the cumulative audio dropout count.
 *
 * The backstop under the rung: any increase means the listener has already heard a gap, so
 * the report goes to CRITICAL whatever the rung says. The callback should return the
 * consumer buffer underflow count of the pipeline that feeds the output -- on this product
 * the one carrying the mute-on-underflow stage, since that is the count that turns into
 * audible silence. Free-running; this module differences it.
 *
 * Only the receiving side can register this. The coordinator has no consumer and therefore no
 * backstop, which is why its report is a statement about the ladder rather than about what
 * anyone heard.
 *
 * @param[in] cb  Function returning a cumulative dropout count. May be NULL to unregister.
 */
void at_cmd_core_register_dropout_count_cb(uint32_t (*cb)(void));

/**
 * @brief Report that the fallback rung has changed.
 *
 * Wire this to sac_fallback_instance_t::fallback_state_change_callback so a rung change is
 * acted on when it happens instead of at the next poll. On a link whose entire warning budget
 * is a couple of hundred milliseconds, a polling interval is most of it.
 *
 * Runs in the audio processing context, so it only marks the state dirty; the event is emitted
 * from at_cmd_core_process() like every other one.
 *
 * @param[in] rung  The rung just switched to. Recorded for the next evaluation.
 */
void at_cmd_core_notify_fb_rung_change(uint8_t rung);

/**
 * @brief Register a callback invoked when an AT command requires sending a
 *        control command to the remote device over the UWB data channel.
 *
 * The callback receives an at_cmd_code_t and its value, and is responsible for getting it
 * into the application's outgoing data packet (queue it for the next transmission; do not
 * transmit from here -- this runs in AT command context).
 *
 * Registered by whichever side wants the command to travel rather than be applied locally.
 * The HS does not use this: it registers the hardware callbacks instead and implements them
 * as "queue for the DG", which predates this hook and is what is on the wire today.
 *
 * @param[in] cb  Function accepting (cmd_type, value). May be NULL to unregister.
 */
void at_cmd_core_register_cmd_tx_cb(void (*cb)(uint8_t cmd_type, uint8_t value));

/**
 * @brief Register a callback invoked when volume should be applied to hardware.
 *
 * Called when AT+VOL=N is received from the local SOC, or when a volume value arrives from
 * the peer over the UWB data channel. The callback receives the new level (0-100) and should
 * apply it to the audio hardware (e.g. via sac_processing_ctrl / sac_volume).
 *
 * BOTH roles register this, and it is not the mistake it looks like. This comment used to
 * say "not used on the DG side -- DG forwards the command over UWB instead", but the DG has
 * always registered it and applies AT+VOL to its own back channel (the HS->DG audio it
 * receives). That is the behaviour ODM hosts see today, so it is the behaviour that stays;
 * the comment was what was wrong. Volume is consequently NOT forwarded over UWB by either
 * side, which is why AT_CMD_VOL is still marked reserved.
 *
 * The consequence to keep in mind when wiring the DG->HS path: registering a cmd_tx callback
 * on the DG would make AT+VOL both forward to the HS and change the DG's own back channel.
 * Those are two different speakers. Decide which one AT+VOL means before enabling it.
 *
 * @param[in] cb  Function accepting volume level 0-100. May be NULL to unregister.
 */
void at_cmd_core_register_vol_cb(void (*cb)(uint8_t vol));

/**
 * @brief Register a callback invoked when play/pause should be applied to hardware.
 *
 * Used on the HS side. Called when AT+PLAY is received from the local SOC
 * OR when a CMD_PLAY packet arrives from the DG over the UWB data channel.
 * Not used on the DG side — DG forwards the command over UWB instead.
 *
 * @param[in] cb  Function to call on play/pause. May be NULL to unregister.
 */
void at_cmd_core_register_play_cb(void (*cb)(void));

/**
 * @brief Notify that a play/pause event was received (from UWB or local button).
 *
 * Calls the registered play hardware callback and sends +EVENT: PLAY to the
 * local SOC over the expansion UART.
 * Call from the app's RX data handler when CMD_PLAY is received, or from
 * the button handler when a local play/pause button is pressed.
 */
void at_cmd_core_notify_play_received(void);

/**
 * @brief Register a callback invoked when stop should be applied to hardware.
 *
 * Used on the HS side. Called when AT+STOP is received from the local SOC
 * OR when a CMD_STOP packet arrives from the DG over the UWB data channel.
 *
 * @param[in] cb  Function to call on stop. May be NULL to unregister.
 */
void at_cmd_core_register_stop_cb(void (*cb)(void));

/**
 * @brief Notify that a stop event was received (from UWB or local SOC).
 *
 * Calls the registered stop hardware callback and sends +EVENT: STOP to the
 * local SOC over the expansion UART.
 */
void at_cmd_core_notify_stop_received(void);

/** @brief Register a callback invoked when next track should be applied to hardware (HS side). */
void at_cmd_core_register_next_track_cb(void (*cb)(void));

/** @brief Notify that a next track event was received. Calls hw_cb and sends +EVENT: NEXT_TRACK to SOC. */
void at_cmd_core_notify_next_track_received(void);

/** @brief Register a callback invoked when previous track should be applied to hardware (HS side). */
void at_cmd_core_register_pre_track_cb(void (*cb)(void));

/** @brief Notify that a previous track event was received. Calls hw_cb and sends +EVENT: PRE_TRACK to SOC. */
void at_cmd_core_notify_pre_track_received(void);

/**
 * @brief Apply a volume value received from the remote device over UWB.
 *
 * Call this from the app's RX data handler when a CMD_VOL packet is received.
 * Updates the internal volume cache, calls the registered vol hardware callback,
 * and sends +EVENT: VOL=<n> to the local SOC over the expansion UART.
 *
 * @param[in] vol  Volume level 0-100.
 */
void at_cmd_core_notify_vol_received(uint8_t vol);

/**
 * @brief Update the cached battery level reported by AT+BATTERY?.
 *
 * Call this from the app's RX data handler whenever a CMD_BATTERY packet
 * is received from the HS over the UWB data channel.
 *
 * @param[in] level  Battery level 0–100 (percent).
 */
void at_cmd_core_set_battery_level(uint8_t level);

/**
 * @brief Register a callback that reads the local battery level (HS side only).
 *
 * When registered, AT+BATTERY? calls this callback directly instead of
 * returning the cached value. Use on HS (node) to report its own battery.
 * DG (coordinator) should NOT register this — it returns the cached value
 * received from HS over the UWB data channel.
 *
 * @param[in] cb  Function returning battery level 0–100. May be NULL to unregister.
 */
void at_cmd_core_register_battery_cb(uint8_t (*cb)(void));

/**
 * @brief Build the vendor block for the next outgoing data packet.
 *
 * Call once per periodic data callback, before packing, and copy every field of the result
 * into the packet. Always fills the frame: with nothing queued it yields id
 * AT_VENDOR_ID_NONE, which is what almost every packet carries, but ack_seq is still set and
 * still has to be transmitted.
 *
 * DELIVERY SEMANTICS, which are the only thing the module promises about vendor traffic.
 * There are two modes, chosen per command by the host:
 *
 *   BEST EFFORT (default). The command goes out in AT_VENDOR_TX_REPEAT consecutive packets
 *   and is then dropped from the queue. The receiver discards the duplicates. Three
 *   back-to-back copies take the loss probability to its cube with no timers and no state,
 *   at a bounded, known cost of ~30 ms of the vendor field. Delivery is NOT guaranteed and
 *   the sender is never told either way. Right for anything the host resends periodically
 *   anyway, and for anything a human will retry.
 *
 *   ACKNOWLEDGED. The command is retransmitted in EVERY packet until the peer echoes its
 *   sequence number back in ack_seq, or until AT_VENDOR_ACK_TIMEOUT_PACKETS have passed.
 *   Either way the host is told, by +EVENT: VENDOR_CMD_ACK:<id> or
 *   +EVENT: VENDOR_CMD_FAIL:<id>. This is real delivery confirmation, not more repeats: it
 *   reports what happened rather than making failure less likely. The cost is that the
 *   command holds the vendor field until it resolves, so queued commands behind it wait.
 *
 * Ordering is preserved in both modes: the queue is FIFO and a command resolves before the
 * next one starts.
 *
 * Safe to call from the timer/interrupt context that builds the packet. This function is the
 * ONLY place the queue head moves -- acknowledgements arriving on the RX side just park a
 * byte for it to read -- so there is exactly one consumer despite two contexts being involved.
 *
 * @param[out] frame  Filled with the vendor block to transmit. Never left untouched.
 */
void at_cmd_core_vendor_tx_fill(at_vendor_frame_t *frame);

/**
 * @brief Handle a received vendor block: deliver a new command, and record the peer's ack.
 *
 * Call from the application's RX data handler for every received packet, passing the packet's
 * vendor fields verbatim. Blocks with id AT_VENDOR_ID_NONE still carry a meaningful ack_seq,
 * so call this unconditionally rather than testing the id first.
 *
 * De-duplication happens here: the repeated copies described in at_cmd_core_vendor_tx_fill()
 * arrive as identical (id, seq) pairs and only the first is emitted as
 * "+EVENT: VENDOR_CMD:<id>,"<hex payload>"". Gaps in seq are deliberately NOT reported. In
 * best-effort mode nothing could be done about them, and in acknowledged mode the sender's
 * own timeout is the authority on whether a command arrived -- a receiver guessing from gaps
 * would produce a second, contradictory answer.
 *
 * @param[in] frame  Vendor fields from the received packet. NULL is ignored.
 */
void at_cmd_core_notify_vendor_received(const at_vendor_frame_t *frame);

/**
 * @brief Register a callback invoked when AT+LE_UWB_SHUTDOWN is received.
 *
 * The callback should perform software cleanup (stop timers, call swc_disconnect,
 * stop audio pipelines) before the hardware shutdown pin is asserted by the core.
 * The core always calls facade_uwb_shutdown() after this callback returns,
 * regardless of whether a callback is registered.
 * If the device is already disconnected the callback should be a no-op.
 *
 * @param[in] cb  Function to call on AT+LE_UWB_SHUTDOWN. May be NULL to unregister.
 */
void at_cmd_core_register_shutdown_cb(void (*cb)(void));

/**
 * @brief Register a callback invoked when AT+LE_UWB_DISCONNECT is received.
 *
 * The callback should terminate the active UWB connection and stop all
 * associated timers/pipelines (i.e. call unpair_device()). The pairing
 * address is preserved so that AT+LE_UWB_CONNECT can reconnect afterwards.
 * If the device is already disconnected the callback should be a no-op.
 *
 * @param[in] cb  Function to call on AT+LE_UWB_DISCONNECT. May be NULL to unregister.
 */
void at_cmd_core_register_disconnect_cb(void (*cb)(void));

/**
 * @brief Register a callback invoked when AT+LE_UWB_CONNECT is received.
 *
 * The callback should attempt to re-establish the UWB connection using the
 * previously assigned pairing addresses, without entering the pairing procedure.
 * If the device is already connected the callback should be a no-op.
 * If no pairing address is available the callback should do nothing.
 *
 * @param[in] cb  Function to call on AT+LE_UWB_CONNECT. May be NULL to unregister.
 */
void at_cmd_core_register_connect_cb(void (*cb)(void));

#ifdef __cplusplus
}
#endif

#endif /* AT_CMD_CORE_H_ */
