/** @file  at_cmd_core.c
 *  @brief Common AT command core implementation.
 */

/* INCLUDES *******************************************************************/
#include "at_cmd_core.h"
#include "at_cmd_core_facade.h"
#include "at_module.h"
#include "swc_api.h"
#include "swc_error.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <strings.h>   /* strncasecmp */

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static bool handler_ver(const char *args, char *resp, uint16_t resp_size);
static bool handler_help(const char *args, char *resp, uint16_t resp_size);
static bool handler_ping(const char *args, char *resp, uint16_t resp_size);
static bool handler_module_info(const char *args, char *resp, uint16_t resp_size);
static bool handler_uwb_conn_status(const char *args, char *resp, uint16_t resp_size);
static bool handler_uwb_pair(const char *args, char *resp, uint16_t resp_size);
static bool handler_fw_version(const char *args, char *resp, uint16_t resp_size);
static bool handler_module_reset(const char *args, char *resp, uint16_t resp_size);
static bool handler_conn_lm(const char *args, char *resp, uint16_t resp_size);
static bool handler_i2s_mux(const char *args, char *resp, uint16_t resp_size);
static bool handler_uwb_connect(const char *args, char *resp, uint16_t resp_size);
static bool handler_uwb_disconnect(const char *args, char *resp, uint16_t resp_size);
static bool handler_uwb_shutdown(const char *args, char *resp, uint16_t resp_size);
static bool handler_uwb_get_role(const char *args, char *resp, uint16_t resp_size);
static bool handler_vol(const char *args, char *resp, uint16_t resp_size);
static bool handler_play(const char *args, char *resp, uint16_t resp_size);
static bool handler_stop(const char *args, char *resp, uint16_t resp_size);
static bool handler_next_track(const char *args, char *resp, uint16_t resp_size);
static bool handler_pre_track(const char *args, char *resp, uint16_t resp_size);
static bool handler_battery(const char *args, char *resp, uint16_t resp_size);
static bool handler_vendor_cmd(const char *args, char *resp, uint16_t resp_size);
static void at_cmd_core_fallback(void);

/* PRIVATE VARIABLES **********************************************************/
static uint8_t              s_device_address   = 0xFF;
static at_uwb_conn_status_t s_uwb_conn_status  = AT_UWB_CONN_STATUS_STANDBY;
static at_device_role_t     s_device_role      = AT_DEVICE_ROLE_NODE;
static int32_t              s_vol              = 0;
static uint8_t              s_battery_level    = 0;
static void               (*s_pair_cb)(void)    = NULL;
static bool               (*s_link_status_cb)(void) = NULL;
static int32_t            (*s_link_margin_cb)(void) = NULL;
static void               (*s_i2s_mux_cb)(bool use_ext) = NULL;
static void               (*s_connect_cb)(void)         = NULL;
static void               (*s_cmd_tx_cb)(uint8_t cmd_type, uint8_t value) = NULL;
static void               (*s_vol_hw_cb)(uint8_t vol)                    = NULL;
static void               (*s_play_hw_cb)(void)                          = NULL;
static void               (*s_stop_hw_cb)(void)                          = NULL;
static void               (*s_next_track_hw_cb)(void)                    = NULL;
static void               (*s_pre_track_hw_cb)(void)                     = NULL;
static void               (*s_disconnect_cb)(void)      = NULL;
static void               (*s_shutdown_cb)(void)        = NULL;
static uint8_t            (*s_battery_cb)(void)         = NULL;
static bool                 s_i2s_mux_is_ext   = false; /* default: ON_BOARD; AT+I2S_MUX toggles to EXT */
static bool                 s_pair_requested        = false;
static bool                 s_reset_requested       = false;
static bool                 s_connect_requested    = false;
static uint32_t             s_connect_start_tick   = 0;
static bool                 s_link_quality_weak    = false;
static uint32_t             s_link_quality_last_check_tick = 0;
/* Down-edge debounce for the link status poll; see AT_UWB_DISCONNECT_DEBOUNCE_MS. */
static bool                 s_link_down_pending    = false;
static uint32_t             s_link_down_since_tick = 0;
static bool                 s_disconnect_requested = false;
static bool                 s_shutdown_requested   = false;

/* **** Vendor pass-through ****
 *
 * Three contexts touch this state, and the split between them is what keeps it lock-free:
 *
 *   AT command context (main loop) -- appends to the ring, owns s_vendor_tx_tail.
 *   Data timer context             -- transmits and retires entries, owns s_vendor_tx_head
 *                                     and every counter beside it.
 *   RX callback context            -- only ever writes s_vendor_peer_ack, a single byte.
 *
 * The RX side deliberately does NOT retire the acknowledged entry itself, even though that is
 * where the acknowledgement arrives. Doing so would make the head move from two contexts and
 * turn a single-producer/single-consumer ring into something that needs a lock. Parking the
 * byte and letting the transmit path notice it costs one packet of latency (10 ms) and buys
 * back the entire concurrency argument.
 *
 * One ring slot is left permanently empty so "full" and "empty" stay distinguishable without
 * a shared count, which would need a read-modify-write from both of the mutating contexts.
 */
#define VENDOR_TX_RING_SIZE  (AT_VENDOR_TX_PENDING_MAX + 1)

typedef struct {
    at_vendor_frame_t frame;
    bool              need_ack;
} vendor_tx_entry_t;

static vendor_tx_entry_t    s_vendor_tx_ring[VENDOR_TX_RING_SIZE];
static volatile uint8_t     s_vendor_tx_head         = 0; /* data timer owns */
static volatile uint8_t     s_vendor_tx_tail         = 0; /* AT context owns */
static uint8_t              s_vendor_tx_repeats_left = 0; /* data timer owns */
static uint16_t             s_vendor_tx_wait_packets = 0; /* data timer owns */
static uint8_t              s_vendor_tx_next_seq     = 0; /* AT context owns */
/* Last sequence the peer says it received from us. Written by the RX context, read by the
 * data timer. A plain byte write is atomic on this core, so no guard is needed. */
static volatile uint8_t     s_vendor_peer_ack        = AT_VENDOR_SEQ_NONE;
/* Last (id, seq) delivered to the local SOC. Doubles as the value echoed back to the peer in
 * ack_seq, which is why sequence numbers skip 0: a peer that has received nothing sends 0,
 * and that must not read as an acknowledgement of a real command. */
static uint8_t              s_vendor_rx_last_id      = AT_VENDOR_ID_NONE;
static uint8_t              s_vendor_rx_last_seq     = AT_VENDOR_SEQ_NONE;
/* Outcome of an acknowledged command, handed to at_cmd_core_process() to emit. Written from
 * the data timer, cleared in the main loop. One slot is enough: a command cannot resolve
 * until it has occupied at least one packet, so outcomes are at least 10 ms apart while the
 * main loop turns over far faster. */
static volatile uint8_t     s_vendor_outcome_id      = AT_VENDOR_ID_NONE;
static volatile bool        s_vendor_outcome_acked   = false;

/* PUBLIC FUNCTIONS ***********************************************************/
void at_cmd_core_init(void)
{
    facade_expansion_uart_init(AT_CMD_CORE_BAUD_RATE);

    at_module_init(facade_expansion_uart_write,
                   facade_expansion_uart_read_byte,
                   facade_get_tick_ms);

    at_server_register("VER",    handler_ver);

    at_server_register("PING",        handler_ping);
    at_server_register("HELP",   handler_help);
    at_server_register("MODULE_RESET",    handler_module_reset);
    at_server_register("MODULE_INFO",    handler_module_info);

    at_server_register("LE_UWB_CONN_STATUS", handler_uwb_conn_status);


    at_server_register("LE_UWB_PAIR",        handler_uwb_pair);
    at_server_register("FW_VERSION",      handler_fw_version);
    at_server_register("CONN_LM",         handler_conn_lm);
    at_server_register("I2S_MUX",         handler_i2s_mux);
    at_server_register("LE_UWB_CONNECT",     handler_uwb_connect);
    at_server_register("LE_UWB_DISCONNECT",  handler_uwb_disconnect);
    at_server_register("LE_UWB_SHUTDOWN",    handler_uwb_shutdown);
    at_server_register("LE_UWB_GET_ROLE",    handler_uwb_get_role);
    at_server_register("VOL",             handler_vol);
    at_server_register("PLAY",            handler_play);
    at_server_register("STOP",            handler_stop);
    at_server_register("NEXT_TRACK",      handler_next_track);
    at_server_register("PRE_TRACK",       handler_pre_track);
    at_server_register("BATTERY",         handler_battery);
    at_server_register("VENDOR_CMD",      handler_vendor_cmd);

    at_module_set_fallback_handler(at_cmd_core_fallback);
}

/** @brief Fallback handler — prints AT+HELP output for any unknown/malformed input. */
static void at_cmd_core_fallback(void)
{
    char unused[1];
    handler_help("", unused, sizeof(unused));
}

void at_cmd_core_register_link_status_cb(bool (*cb)(void))
{
    s_link_status_cb = cb;
}

void at_cmd_core_register_link_margin_cb(int32_t (*cb)(void))
{
    s_link_margin_cb = cb;
}

void at_cmd_core_register_i2s_mux_cb(void (*cb)(bool use_ext))
{
    s_i2s_mux_cb = cb;
}

void at_cmd_core_register_cmd_tx_cb(void (*cb)(uint8_t cmd_type, uint8_t value))
{
    s_cmd_tx_cb = cb;
}

void at_cmd_core_register_vol_cb(void (*cb)(uint8_t vol))
{
    s_vol_hw_cb = cb;
}

void at_cmd_core_register_play_cb(void (*cb)(void))
{
    s_play_hw_cb = cb;
}

void at_cmd_core_notify_play_received(void)
{
    if (s_play_hw_cb != NULL) {
        s_play_hw_cb();
    }
    facade_expansion_uart_write("+EVENT: PLAY\r\n");
}

void at_cmd_core_register_stop_cb(void (*cb)(void))
{
    s_stop_hw_cb = cb;
}

void at_cmd_core_notify_stop_received(void)
{
    if (s_stop_hw_cb != NULL) {
        s_stop_hw_cb();
    }
    facade_expansion_uart_write("+EVENT: STOP\r\n");
}

void at_cmd_core_register_next_track_cb(void (*cb)(void))
{
    s_next_track_hw_cb = cb;
}

void at_cmd_core_notify_next_track_received(void)
{
    if (s_next_track_hw_cb != NULL) {
        s_next_track_hw_cb();
    }
    facade_expansion_uart_write("+EVENT: NEXT_TRACK\r\n");
}

void at_cmd_core_register_pre_track_cb(void (*cb)(void))
{
    s_pre_track_hw_cb = cb;
}

void at_cmd_core_notify_pre_track_received(void)
{
    if (s_pre_track_hw_cb != NULL) {
        s_pre_track_hw_cb();
    }
    facade_expansion_uart_write("+EVENT: PRE_TRACK\r\n");
}

void at_cmd_core_notify_vol_received(uint8_t vol)
{
    char event[24];

    s_vol = vol;
    if (s_vol_hw_cb != NULL) {
        s_vol_hw_cb(vol);
    }
    snprintf(event, sizeof(event), "+EVENT: VOL=%d\r\n", (int)vol);
    facade_expansion_uart_write(event);
}

/** @brief Retire the entry at the head of the vendor TX queue and report how it ended.
 *
 *  Data timer context only. The outcome is parked rather than printed: this runs in an
 *  interrupt and the UART write is a blocking call.
 */
static void vendor_tx_retire(uint8_t head, uint8_t id, bool acked, bool report)
{
    s_vendor_tx_repeats_left = 0;
    s_vendor_tx_wait_packets = 0;

    if (report) {
        s_vendor_outcome_acked = acked;
        s_vendor_outcome_id    = id; /* written last: it is what the main loop tests on */
    }

    /* Releasing the slot must come after everything that reads it. This store is the single
     * point at which the AT context becomes free to overwrite this entry. */
    s_vendor_tx_head = (uint8_t)((head + 1) % VENDOR_TX_RING_SIZE);
}

void at_cmd_core_vendor_tx_fill(at_vendor_frame_t *frame)
{
    uint8_t head;
    vendor_tx_entry_t *entry;

    if (frame == NULL) {
        return;
    }

    *frame = (at_vendor_frame_t){0};

    /* Echoed in EVERY packet, including the ones carrying no command. An acknowledgement that
     * only went out once would be exactly as losable as the command it is acknowledging. */
    frame->ack_seq = s_vendor_rx_last_seq;

    head = s_vendor_tx_head;
    if (head == s_vendor_tx_tail) {
        return; /* nothing queued; id stays AT_VENDOR_ID_NONE */
    }
    entry = &s_vendor_tx_ring[head];

    if (entry->need_ack) {
        /* Resolve first, so an acknowledgement that arrived since the last packet stops the
         * retransmission immediately instead of costing one more copy.
         *
         * The wait_packets test is not redundant: without it, a stale s_vendor_peer_ack left
         * over from an earlier command could match a newly queued one before it has ever been
         * transmitted, and the host would be told ACK for a command that never reached the
         * air. That needs sequence numbers to wrap while the link is down -- 255 commands, so
         * minutes of outage with a host still pushing -- but "we confirmed delivery of
         * something we never sent" is the one answer this mechanism must never give, so it is
         * cheaper to make it structurally impossible than to argue about the odds. The peer
         * cannot have acknowledged a command we have not sent, so requiring one transmission
         * first is simply the truth. */
        if (s_vendor_tx_wait_packets > 0 && s_vendor_peer_ack == entry->frame.seq) {
            uint8_t id = entry->frame.id;

            vendor_tx_retire(head, id, true, true);
            /* Fall through with no command this packet. The next one starts on the next tick;
             * spending 10 ms here keeps this function to a single retire per call, which is
             * what keeps the one-outcome-slot argument true. */
            return;
        }
        if (++s_vendor_tx_wait_packets >= AT_VENDOR_ACK_TIMEOUT_PACKETS) {
            uint8_t id = entry->frame.id;

            vendor_tx_retire(head, id, false, true);
            return;
        }
        /* Retransmit in every packet until one of the two above happens. */
        *frame = entry->frame;
        frame->ack_seq = s_vendor_rx_last_seq;
        return;
    }

    /* Best effort: a fixed number of copies, then gone, with nobody told either way. */
    if (s_vendor_tx_repeats_left == 0) {
        s_vendor_tx_repeats_left = AT_VENDOR_TX_REPEAT;
    }
    *frame = entry->frame;
    frame->ack_seq = s_vendor_rx_last_seq;

    if (--s_vendor_tx_repeats_left == 0) {
        vendor_tx_retire(head, entry->frame.id, false, false);
    }
}

void at_cmd_core_notify_vendor_received(const at_vendor_frame_t *frame)
{
    /* "+EVENT: VENDOR_CMD:" (19) + id (3) + ",\"" (2) + hex (2 per byte) + "\"\r\n" (3) + NUL. */
    char event[32 + (AT_VENDOR_PAYLOAD_MAX * 2)];
    int written;
    uint8_t len;

    if (frame == NULL) {
        return;
    }

    /* Record the peer's acknowledgement even when this packet carries no command of its own,
     * which is the usual case: the ack is a level echoed in every packet. Nothing else is
     * done here -- the transmit path owns the queue and will notice. */
    s_vendor_peer_ack = frame->ack_seq;

    if (frame->id == AT_VENDOR_ID_NONE) {
        return;
    }

    /* Reserved for the module; never surfaced to the local SOC. Nothing sends these today,
     * so this branch exists purely for the future: when a later firmware starts using the
     * reserved range, units already in the field must ignore that traffic rather than
     * forward it to their host as an invented vendor command. A reservation honoured only by
     * the sender would fail in precisely the case it is meant to cover. */
    if (frame->id >= AT_VENDOR_ID_RESERVED) {
        return;
    }

    /* Drop the repeats of a command already delivered. Sequence numbers never take the value
     * AT_VENDOR_SEQ_NONE, and s_vendor_rx_last_seq starts there, so the first frame after
     * boot always differs and no separate "have we received anything" flag is needed. */
    if (frame->id == s_vendor_rx_last_id && frame->seq == s_vendor_rx_last_seq) {
        return;
    }
    s_vendor_rx_last_id  = frame->id;
    s_vendor_rx_last_seq = frame->seq;

    /* Clamp rather than trust. len arrives from the air, so a corrupted byte -- or a peer
     * built with a larger AT_VENDOR_PAYLOAD_MAX -- must not walk off the end of data[]. */
    len = (frame->len > AT_VENDOR_PAYLOAD_MAX) ? AT_VENDOR_PAYLOAD_MAX : frame->len;

    written = snprintf(event, sizeof(event), "+EVENT: VENDOR_CMD:%u", (unsigned)frame->id);
    if (len > 0 && written > 0 && (size_t)written < sizeof(event)) {
        written += snprintf(event + written, sizeof(event) - (size_t)written, ",\"");
        for (uint8_t i = 0; i < len && written > 0 && (size_t)written < sizeof(event); i++) {
            written += snprintf(event + written, sizeof(event) - (size_t)written,
                                "%02X", frame->data[i]);
        }
        if (written > 0 && (size_t)written < sizeof(event)) {
            written += snprintf(event + written, sizeof(event) - (size_t)written, "\"");
        }
    }
    if (written > 0 && (size_t)written < sizeof(event)) {
        snprintf(event + written, sizeof(event) - (size_t)written, "\r\n");
    }
    facade_expansion_uart_write(event);
}

void at_cmd_core_set_battery_level(uint8_t level)
{
    s_battery_level = level;
}

void at_cmd_core_register_battery_cb(uint8_t (*cb)(void))
{
    s_battery_cb = cb;
}

void at_cmd_core_register_connect_cb(void (*cb)(void))
{
    s_connect_cb = cb;
}

void at_cmd_core_register_disconnect_cb(void (*cb)(void))
{
    s_disconnect_cb = cb;
}

void at_cmd_core_register_shutdown_cb(void (*cb)(void))
{
    s_shutdown_cb = cb;
}

void at_cmd_core_notify_build(const char *build_id)
{
    /* Holds "+EVENT: BUILD: " + AT_CMD_CORE_BUILD_ID (version + release tag + __DATE__ +
     * __TIME__, ~32 chars) + " role=DG\r\n", with room to spare if the strings grow. */
    char buf[96];

    if (build_id == NULL) {
        return;
    }

    snprintf(buf, sizeof(buf), "+EVENT: BUILD: %s role=%s\r\n", build_id,
             (s_device_role == AT_DEVICE_ROLE_NODE) ? "HS" : "DG");
    facade_expansion_uart_write(buf);
}

void at_cmd_core_notify_uwb_ready(void)
{
    facade_expansion_uart_write("+EVENT: LE_UWB_READY\r\n");
}

void at_cmd_core_notify_unpaired(void)
{
    /* Deliberately does NOT touch s_uwb_conn_status. The link really is going down, so the
     * poll should still emit its UWB_DISCONNECTED as it always did -- a host that only knows
     * the old event keeps working. This line is the extra bit of information on top: the
     * peer was not merely unreachable, the stored address is gone, so reconnecting is not
     * something AT+LE_UWB_CONNECT can do. Only AT+LE_UWB_PAIR can. */
    facade_expansion_uart_write("+EVENT: LE_UWB_UNPAIRED\r\n");
}

void at_cmd_core_notify_pairing_started(void)
{
    /* Puts the status machine on hold: link polling is meaningless while the wireless core is
     * torn down for pairing, and without this the poll reported a dropped link and emitted a
     * spurious UWB_DISCONNECTED on the way in. */
    s_uwb_conn_status = AT_UWB_CONN_STATUS_PAIRING;

    /* Announce the window. pairing_node_start() / pairing_coordinator_start() block for up to
     * PAIRING_TIMEOUT_IN_SECONDS, and the status is parked at PAIRING for all of it, so
     * without this line the host sees nothing at all between the command and the result --
     * no way to light a "pairing" indication for the one state where the user is waiting and
     * watching. Sent before the blocking call so it is on the wire when the window opens. */
    facade_expansion_uart_write("+EVENT: LE_UWB_PAIRING\r\n");
}

void at_cmd_core_notify_pairing_result(bool success)
{
    /* Leave PAIRING either way, otherwise the status machine stays parked for good. Success
     * hands over to STANDBY rather than claiming CONNECTED: the addresses are assigned but the
     * link still has to come up, and the poll emits UWB_CONNECTED once it really does. */
    s_uwb_conn_status = AT_UWB_CONN_STATUS_STANDBY;
    facade_expansion_uart_write(success ? "+EVENT: LE_UWB_PAIRED\r\n" : "+EVENT: LE_UWB_PAIR_FAIL\r\n");
}

void at_cmd_core_notify_standby(void)
{
    /* The status machine in at_cmd_core_process() is what normally emits this, but every
     * caller powers the MCU down on the next line and never comes back, so the event has to
     * be sent from here -- and has to be flushed, since the write only queues. Without the
     * flush at the end of this function the host would see the UART fall silent instead of
     * being told why, which is the exact failure these two lines exist to prevent. */
    s_uwb_conn_status = AT_UWB_CONN_STATUS_STANDBY;
    facade_expansion_uart_write("+EVENT: LE_UWB_DISCONNECTED\r\n");

    /* Then say which kind of disconnect this is. UWB_DISCONNECTED alone used to imply the
     * power-down, because it was the only path that could emit it on the node -- the link
     * poll was reading the application's own "am I paired" flag and so never saw a drop.
     * With the poll fixed, a peer walking away emits the same line while the module stays
     * up and re-syncs by itself, which is the opposite of what the host should do here:
     * this UART is about to stop answering, and only a reset brings it back. */
    facade_expansion_uart_write("+EVENT: LE_UWB_STANDBY\r\n");

    /* Last thing before the caller powers the module down: put the queue on the wire. */
    facade_expansion_uart_flush();
}

void at_cmd_core_register_pair_cb(void (*cb)(void))
{
    s_pair_cb = cb;
}

void at_cmd_core_set_uwb_conn_status(at_uwb_conn_status_t status)
{
    /* Stamp the start of every connecting window, not just the one AT+LE_UWB_CONNECT opens.
     * The boot auto-reconnect path sets CONNECTING directly, so s_connect_start_tick used to
     * stay at 0 and the timeout below compared against the time since boot -- CONNECT_FAIL
     * then fired a fixed AT_UWB_CONNECT_TIMEOUT_MS after power-on rather than after the
     * attempt actually started. */
    if ((status == AT_UWB_CONN_STATUS_CONNECTING) && (s_uwb_conn_status != AT_UWB_CONN_STATUS_CONNECTING)) {
        s_connect_start_tick = facade_get_tick_ms();
    }

    s_uwb_conn_status = status;
}

void at_cmd_core_set_device_address(uint8_t addr)
{
    s_device_address = addr;
}

void at_cmd_core_set_device_role(at_device_role_t role)
{
    s_device_role = role;
}

void at_cmd_core_process(void)
{
    at_module_process();

    /* Invoke pair callback deferred — after at_module_process() has sent OK. */
    if (s_pair_requested) {
        s_pair_requested = false;
        if (s_pair_cb != NULL) {
            s_pair_cb();
        }
    }

    /* Reset MCU deferred — after at_module_process() has sent OK. "Sent" now means "queued",
     * so the OK has to be flushed onto the wire before the reset discards the FIFO along with
     * the rest of RAM. */
    if (s_reset_requested) {
        s_reset_requested = false;
        facade_expansion_uart_flush();
        facade_system_reset();
    }

    /* Report how an acknowledged vendor command ended. Deferred to here because the transmit
     * path that decides it runs in the data timer interrupt and the UART write blocks. */
    if (s_vendor_outcome_id != AT_VENDOR_ID_NONE) {
        char event[40];
        uint8_t id = s_vendor_outcome_id;
        bool acked = s_vendor_outcome_acked;

        s_vendor_outcome_id = AT_VENDOR_ID_NONE;
        snprintf(event, sizeof(event), "+EVENT: VENDOR_CMD_%s:%u\r\n",
                 acked ? "ACK" : "FAIL", (unsigned)id);
        facade_expansion_uart_write(event);
    }



    /* Invoke connect callback deferred — after at_module_process() has sent OK. */
    if (s_connect_requested) {
        s_connect_requested = false;
        if (s_connect_cb != NULL) {
            s_connect_cb();
        }
        /* If the app entered CONNECTING state, record the start tick for timeout. */
        if (s_uwb_conn_status == AT_UWB_CONN_STATUS_CONNECTING) {
            s_connect_start_tick = facade_get_tick_ms();
        }
    }

    /* Invoke disconnect callback deferred — after at_module_process() has sent OK. */
    if (s_disconnect_requested) {
        s_disconnect_requested = false;
        if (s_disconnect_cb != NULL) {
            s_disconnect_cb();
        }
    }

    /* Invoke shutdown callback then assert hardware shutdown — deferred. */
    if (s_shutdown_requested) {
        s_shutdown_requested = false;
        if (s_shutdown_cb != NULL) {
            s_shutdown_cb(); /* app cleanup: stop timers, disconnect SWC */
        }
        facade_expansion_uart_flush(); /* the OK and any events, before the radio goes */
        facade_uwb_shutdown(); /* assert radio shutdown pin(s) */
    }

    if (s_link_status_cb == NULL || s_uwb_conn_status == AT_UWB_CONN_STATUS_PAIRING) {
        return;
    }

    /* Connecting window: wait for link-up or timeout → CONNECT_FAIL. */
    if (s_uwb_conn_status == AT_UWB_CONN_STATUS_CONNECTING) {
        if (s_link_status_cb()) {
            s_uwb_conn_status = AT_UWB_CONN_STATUS_CONNECTED;
            facade_expansion_uart_write("+EVENT: LE_UWB_CONNECTED\r\n");
        } else if (facade_get_tick_ms() - s_connect_start_tick >= AT_UWB_CONNECT_TIMEOUT_MS) {
            s_uwb_conn_status = AT_UWB_CONN_STATUS_STANDBY;
            facade_expansion_uart_write("+EVENT: LE_UWB_CONNECT_FAIL\r\n");
        }
        return;
    }

    /* Asymmetric on purpose: the down edge waits out AT_UWB_DISCONNECT_DEBOUNCE_MS, the up
     * edge is reported the moment it is seen. A link that is merely idle -- no audio flowing,
     * so the coordinator's audio timeslots carry nothing -- reads as down within 20 ms even
     * though the peer is still there and still answering on the data connection, and the host
     * used to get a DISCONNECTED/CONNECTED pair every time playback paused.
     *
     * AT+LE_UWB_CONN_STATUS? reads the same variable, so it stays CONNECTED for the length of
     * the window too. That is deliberate: the query and the event must not disagree. */
    if (s_link_status_cb()) {
        s_link_down_pending = false;
        if (s_uwb_conn_status != AT_UWB_CONN_STATUS_CONNECTED) {
            s_uwb_conn_status = AT_UWB_CONN_STATUS_CONNECTED;
            facade_expansion_uart_write("+EVENT: LE_UWB_CONNECTED\r\n");
        }
    } else if (s_uwb_conn_status == AT_UWB_CONN_STATUS_CONNECTED) {
        uint32_t down_now = facade_get_tick_ms();

        if (!s_link_down_pending) {
            s_link_down_pending = true;
            s_link_down_since_tick = down_now;
        } else if ((down_now - s_link_down_since_tick) >= AT_UWB_DISCONNECT_DEBOUNCE_MS) {
            s_link_down_pending = false;
            s_uwb_conn_status = AT_UWB_CONN_STATUS_STANDBY;
            s_link_quality_weak = false; /* reset on disconnect so WEAK can fire again */
            facade_expansion_uart_write("+EVENT: LE_UWB_DISCONNECTED\r\n");
        }
    }

    /* Link quality monitor: poll link margin periodically and notify on threshold crossing. */
    if (s_uwb_conn_status == AT_UWB_CONN_STATUS_CONNECTED && s_link_margin_cb != NULL) {
        uint32_t now = facade_get_tick_ms();
        if (now - s_link_quality_last_check_tick >= AT_UWB_LINK_QUALITY_CHECK_INTERVAL_MS) {
            s_link_quality_last_check_tick = now;
            int32_t margin = s_link_margin_cb();
            if (!s_link_quality_weak && margin < AT_UWB_LINK_QUALITY_WEAK_THRESHOLD_DB) {
                s_link_quality_weak = true;
                facade_expansion_uart_write("+EVENT: LE_UWB_QUALITY:WEAK\r\n");
            } else if (s_link_quality_weak && margin >= AT_UWB_LINK_QUALITY_GOOD_THRESHOLD_DB) {
                s_link_quality_weak = false;
                facade_expansion_uart_write("+EVENT: LE_UWB_QUALITY:GOOD\r\n");
            }
        }
    }
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief AT+VER — return SDK version string. */
static bool handler_ver(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    snprintf(resp, resp_size, "+VER: SPARK SDK SR1100 " FW_VERSION_STRING);
    return true;
}

/** @brief AT+FW_VERSION? — return firmware version string. */
static bool handler_fw_version(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    snprintf(resp, resp_size, "+FW_VERSION: " FW_VERSION_STRING);
    return true;
}

/** @brief AT+PING — connectivity check, always responds OK. */
static bool handler_ping(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    snprintf(resp, resp_size, "OK");
    return true;
}

/** @brief AT+MODULE_INFO? — HW model, FW version, chip version, IC serial number, device address. */
static bool handler_module_info(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    swc_error_t err = SWC_ERR_NONE;

    uint8_t  hw_model   = swc_node_get_radio_product_model(&err);
    uint8_t  chip_ver   = swc_node_get_radio_product_version(&err);
    uint64_t serial     = swc_node_get_radio_serial_number(&err);

    snprintf(resp, resp_size,
             "+MODULE_INFO: HW=%u,FW=v2.3.0,Chip=%u,SN=%08lX%08lX,Addr=0x%02X",
             hw_model,
             chip_ver,
             (unsigned long)(serial >> 32),
             (unsigned long)(serial & 0xFFFFFFFF),
             s_device_address);
    return true;
}

/** @brief AT+LE_UWB_CONN_STATUS? — current UWB connection status.
 *
 *  Answers "<n> (<NAME>)". The number is the machine-readable half and is what a host should
 *  parse; the name is there so a person reading a terminal does not have to keep the mapping
 *  in their head, which is where transcription mistakes come from. Both halves come from the
 *  same switch, so they cannot drift apart the way a number and a comment in a document do.
 */
static bool handler_uwb_conn_status(const char *args, char *resp, uint16_t resp_size)
{
    const char *name;

    (void)args;

    switch (s_uwb_conn_status) {
    case AT_UWB_CONN_STATUS_STANDBY:    name = "STANDBY";    break;
    case AT_UWB_CONN_STATUS_PAIRING:    name = "PAIRING";    break;
    case AT_UWB_CONN_STATUS_CONNECTED:  name = "CONNECTED";  break;
    case AT_UWB_CONN_STATUS_CONNECTING: name = "CONNECTING"; break;
    default:                            name = "UNKNOWN";    break;
    }

    snprintf(resp, resp_size, "+LE_UWB_CONN_STATUS: %d (%s)", (int)s_uwb_conn_status, name);
    return true;
}

/** @brief AT+LE_UWB_PAIR — trigger UWB pairing via registered callback. */
static bool handler_uwb_pair(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    s_pair_requested = true;
    snprintf(resp, resp_size, "OK");
    return true;
}

/** @brief AT+MODULE_RESET — reset the UWB module MCU. */
static bool handler_module_reset(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    s_reset_requested = true;
    snprintf(resp, resp_size, "OK");
    return true;
}

/** @brief AT+CONN_LM? — report UWB link margin in dB. */
static bool handler_conn_lm(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    if (s_link_margin_cb == NULL) {
        snprintf(resp, resp_size, "+CONN_LM: N/A");
        return true;
    }
    snprintf(resp, resp_size, "+CONN_LM: %ddB", (int)s_link_margin_cb());
    return true;
}

/** @brief AT+I2S_MUX — toggle I2S MUX between ON_BOARD and EXT codec. */
static bool handler_i2s_mux(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    s_i2s_mux_is_ext = !s_i2s_mux_is_ext;
    if (s_i2s_mux_cb != NULL) {
        s_i2s_mux_cb(s_i2s_mux_is_ext);
    }
    snprintf(resp, resp_size, "+I2S_MUX: %s", s_i2s_mux_is_ext ? "EXT" : "ON_BOARD");
    return true;
}

/** @brief AT+LE_UWB_SHUTDOWN — disconnect and assert hardware shutdown pin on UWB radio(s). */
static bool handler_uwb_shutdown(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    s_shutdown_requested = true;
    snprintf(resp, resp_size, "OK");
    return true;
}

/** @brief AT+VOL=[0-100] / AT+VOL? — set or query headphone volume. */
static bool handler_vol(const char *args, char *resp, uint16_t resp_size)
{
    if (args[0] == '?') {
        snprintf(resp, resp_size, "+VOL: %d", (int)s_vol);
        return true;
    }

    int vol;
    if (sscanf(args, "=%d", &vol) != 1 || vol < 0 || vol > 100) {
        snprintf(resp, resp_size, "ERROR");
        return true;
    }
    s_vol = vol;
    if (s_cmd_tx_cb != NULL) {
        s_cmd_tx_cb(AT_CMD_VOL, (uint8_t)vol); /* forward over UWB */
    }
    if (s_vol_hw_cb != NULL) {
        s_vol_hw_cb((uint8_t)vol); /* HS: apply to hardware */
    }
    snprintf(resp, resp_size, "OK");
    return true;
}

/** @brief AT+STOP — forward over UWB if a tx callback is registered, and apply locally if a
 *         hardware callback is registered. Which of the two happens is the application's
 *         choice, not this file's; see handler_play() for why both are offered.
 */
static bool handler_stop(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    if (s_cmd_tx_cb != NULL) {
        s_cmd_tx_cb(AT_CMD_STOP, 0); /* forward to the peer over UWB */
    }
    if (s_stop_hw_cb != NULL) {
        s_stop_hw_cb(); /* HS: apply locally */
    }
    snprintf(resp, resp_size, "OK");
    return true;
}

/** @brief AT+NEXT_TRACK — forward over UWB and/or apply locally; see handler_play(). */
static bool handler_next_track(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    if (s_cmd_tx_cb != NULL) {
        s_cmd_tx_cb(AT_CMD_NEXT_TRACK, 0);
    }
    if (s_next_track_hw_cb != NULL) {
        s_next_track_hw_cb();
    }
    snprintf(resp, resp_size, "OK");
    return true;
}

/** @brief AT+PRE_TRACK — forward over UWB and/or apply locally; see handler_play(). */
static bool handler_pre_track(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    if (s_cmd_tx_cb != NULL) {
        s_cmd_tx_cb(AT_CMD_PRE_TRACK, 0);
    }
    if (s_pre_track_hw_cb != NULL) {
        s_pre_track_hw_cb();
    }
    snprintf(resp, resp_size, "OK");
    return true;
}

/** @brief AT+PLAY — forward Play/Pause over UWB and/or apply it locally.
 *
 *  Both callbacks are offered on both roles on purpose. The original comment here said
 *  "DG forwards, HS applies locally", but the shipping HS does neither of those literally:
 *  it registers the *hardware* callback and implements it as "queue this for the DG", because
 *  the forwarding callback was never wired up. Rather than encode a role split this file
 *  cannot enforce, it now just calls whatever the application registered and lets the
 *  application decide. See MD/at_cmd_bidir_decision_spec.md section 2.
 */
static bool handler_play(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    if (s_cmd_tx_cb != NULL) {
        s_cmd_tx_cb(AT_CMD_PLAY, 0); /* forward to the peer over UWB */
    }
    if (s_play_hw_cb != NULL) {
        s_play_hw_cb(); /* HS: apply locally */
    }
    snprintf(resp, resp_size, "OK");
    return true;
}

/** @brief AT+BATTERY? — query cached HS battery level (0-100%). */
static bool handler_battery(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    uint8_t level = (s_battery_cb != NULL) ? s_battery_cb() : s_battery_level;
    snprintf(resp, resp_size, "+BATTERY: %d", (int)level);
    return true;
}

/** @brief Decode one hex digit, or -1 if it is not one. */
static int hex_nibble(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

/** @brief AT+VENDOR_CMD=<id>[,"<hex payload>"[,"ACK"]] — queue an ODM command for the peer.
 *
 *  ids 1-223 belong to the ODM; 224-255 are reserved for the module (AT_VENDOR_ID_RESERVED).
 *  The module does not interpret id or payload, and deliberately has no table of valid ids:
 *  the point of this command is that the two host SOCs can agree on new commands without a
 *  module firmware release. Everything below is transport validation only -- is it
 *  well-formed, does it fit, is there room to queue it.
 *
 *  Parameters are POSITIONAL AND FIXED, not a variable-length list. There is exactly one
 *  payload parameter, and it is one opaque byte string; see at_vendor_frame_t for why the
 *  module cannot offer several parameters without lying about what arrives at the other end.
 *
 *  The payload is hex rather than raw bytes because this is a line-oriented ASCII channel: a
 *  raw 0x0D, 0x0A or 0x00 in the middle of a command would end the line early, and defining
 *  an escape scheme for one command is a worse trade than doubling its length.
 *
 *  The quotes are required, and they are not decoration. AT convention is that an unquoted
 *  numeric parameter is decimal and a quoted one is a string -- so without them a reader of
 *  AT+VENDOR_CMD=17,12 has no way to know that 17 is decimal and 12 is hex. Requiring the
 *  quotes borrows the convention's own disambiguator instead of inventing a rule that has to
 *  be memorised from a document.
 */
static bool handler_vendor_cmd(const char *args, char *resp, uint16_t resp_size)
{
    at_vendor_frame_t frame = {0};
    const char *p = args;
    unsigned id = 0;
    bool need_ack = false;
    uint8_t next_tail;

    if (*p != '=') {
        snprintf(resp, resp_size, "SYNTAX");
        return false;
    }
    p++;

    /* Command id, decimal. */
    if (*p < '0' || *p > '9') {
        snprintf(resp, resp_size, "SYNTAX");
        return false;
    }
    while (*p >= '0' && *p <= '9') {
        id = (id * 10u) + (unsigned)(*p - '0');
        if (id > 255u) {
            snprintf(resp, resp_size, "BAD_ID");
            return false;
        }
        p++;
    }
    if (id == AT_VENDOR_ID_NONE) {
        /* Not a usable id: 0 is how a packet says "no vendor command". */
        snprintf(resp, resp_size, "BAD_ID");
        return false;
    }
    if (id >= AT_VENDOR_ID_RESERVED) {
        /* Held back for the module's own future in-band traffic. Reported separately from
         * BAD_ID because the two need different reactions: BAD_ID means the host built a
         * malformed command, RESERVED_ID means the command is well formed but the host is
         * allocating from a range that is not its to allocate from. */
        snprintf(resp, resp_size, "RESERVED_ID");
        return false;
    }
    frame.id = (uint8_t)id;

    /* Optional payload, quoted hex. */
    if (*p == ',') {
        p++;
        if (*p != '"') {
            snprintf(resp, resp_size, "SYNTAX");
            return false;
        }
        p++;
        while (*p != '"') {
            int hi = hex_nibble(p[0]);
            int lo = (p[0] != '\0') ? hex_nibble(p[1]) : -1;

            if (hi < 0 || lo < 0) {
                /* Covers a non-hex character, an odd number of digits, and an unterminated
                 * string in one test: a byte is two digits, and half a byte is not something
                 * to guess at. */
                snprintf(resp, resp_size, "SYNTAX");
                return false;
            }
            if (frame.len >= AT_VENDOR_PAYLOAD_MAX) {
                snprintf(resp, resp_size, "TOO_LONG");
                return false;
            }
            frame.data[frame.len++] = (uint8_t)((hi << 4) | lo);
            p += 2;
        }
        p++; /* closing quote */
    }

    /* Optional delivery mode, a quoted keyword rather than a numeric flag.
     *
     * AT+VENDOR_CMD=1,"02",1 would be shorter, but a bare trailing 1 tells a reader nothing
     * -- it is the same defect as writing the payload without quotes, one parameter further
     * along. Quoted keyword parameters are ordinary AT (AT+CGDCONT=1,"IP",...), so this costs
     * nothing but four characters and removes a thing to look up.
     *
     * Numeric 0/1 is deliberately NOT also accepted: two spellings of one parameter would put
     * the ambiguity straight back. */
    if (*p == ',') {
        p++;
        if (strncasecmp(p, "\"ACK\"", 5) == 0) {
            need_ack = true;
            p += 5;
        } else if (strncasecmp(p, "\"NOACK\"", 7) == 0) {
            need_ack = false;
            p += 7;
        } else {
            snprintf(resp, resp_size, "SYNTAX");
            return false;
        }
    }

    if (*p != '\0') {
        /* Anything left over is a third payload parameter, a stray character, or a second
         * quoted string. All of them mean the host expects a shape this command does not
         * have, and guessing which would be worse than saying so. */
        snprintf(resp, resp_size, "SYNTAX");
        return false;
    }

    /* Queue it. Full is reported rather than dropped: the host is issuing commands faster
     * than the 10 ms link can carry them, and it can only back off if it is told. */
    next_tail = (uint8_t)((s_vendor_tx_tail + 1) % VENDOR_TX_RING_SIZE);
    if (next_tail == s_vendor_tx_head) {
        snprintf(resp, resp_size, "BUSY");
        return false;
    }

    /* Sequence numbers run 1..255 and skip 0, which is reserved for "nothing". The ack field
     * is zero in every packet from a peer that has received nothing, and that must never be
     * mistaken for an acknowledgement of a real command. */
    if (++s_vendor_tx_next_seq == AT_VENDOR_SEQ_NONE) {
        s_vendor_tx_next_seq = 1;
    }
    frame.seq = s_vendor_tx_next_seq;

    s_vendor_tx_ring[s_vendor_tx_tail].frame    = frame;
    s_vendor_tx_ring[s_vendor_tx_tail].need_ack = need_ack;
    /* Publish only after the slot is fully written -- the consumer runs from the data timer
     * and may look the instant this store lands. */
    s_vendor_tx_tail = next_tail;

    snprintf(resp, resp_size, "OK");
    return true;
}

/** @brief AT+LE_UWB_GET_ROLE? — report device role, "<n> (<NAME>)" as AT+LE_UWB_CONN_STATUS?.
 *
 *  The name keeps the product-side word alongside the wireless-stack one. Everything outside
 *  this firmware -- schematics, presets, release packages, the PRD -- says HS and DG, while
 *  the stack says node and coordinator. Printing only one of the two pairs forces whoever is
 *  reading to translate, and they are the two halves people most often get backwards.
 */
static bool handler_uwb_get_role(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    snprintf(resp, resp_size, "+LE_UWB_GET_ROLE: %d (%s)", (int)s_device_role,
             s_device_role == AT_DEVICE_ROLE_NODE ? "NODE/HS" : "COORDINATOR/DG");
    return true;
}

/** @brief AT+LE_UWB_DISCONNECT — end the session.
 *
 *  The puretone apps implement this as a power-down: the UWB radio's shutdown pin is
 *  asserted and the MCU enters STM32 Standby, so the module draws almost nothing and stops
 *  responding to AT entirely. It leaves Standby only through a reset (NRST, or
 *  AT+LE_UWB_CONNECT before the module goes down), which boots into auto-reconnect. Other
 *  apps may implement the callback differently.
 */
static bool handler_uwb_disconnect(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    s_disconnect_requested = true;
    snprintf(resp, resp_size, "OK");
    return true;
}

/** @brief AT+LE_UWB_CONNECT — re-establish the UWB link from the stored pairing address.
 *
 *  The puretone apps implement this as an MCU reset into boot auto-reconnect: reconnecting
 *  in place is not supported (the SAC pipelines cannot be restarted once stopped). The host
 *  sees OK, then +EVENT: LE_UWB_READY, then +EVENT: LE_UWB_CONNECTED. No-op while already
 *  connected. Other apps may implement the callback differently.
 */
static bool handler_uwb_connect(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    s_connect_requested = true;
    snprintf(resp, resp_size, "OK");
    return true;
}

/** @brief AT+HELP — list all registered AT commands. */
static bool handler_help(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    (void)resp_size;

    /* Query commands end with '?'; action commands do not. */
    static const char * const cmds[] = {
        "  AT+HELP\r\n",
        "  AT+PING\r\n",
        "  AT+VER?\r\n",
        "  AT+FW_VERSION?\r\n",
        "  AT+MODULE_INFO?\r\n",
        "  AT+LE_UWB_CONN_STATUS? (0=STANDBY, 1=PAIRING, 2=CONNECTED, 3=CONNECTING)\r\n",
        "  AT+LE_UWB_PAIR\r\n",
        "  AT+MODULE_RESET\r\n",
        //"  AT+CONN_LM? (internal)\r\n",
        "  AT+I2S_MUX\r\n",
        "  AT+LE_UWB_CONNECT\r\n",
        "  AT+LE_UWB_DISCONNECT\r\n",
        "  AT+LE_UWB_SHUTDOWN\r\n",
        "  AT+LE_UWB_GET_ROLE? (0=NODE/HS, 1=COORDINATOR/DG)\r\n",
        "  AT+VOL=[0-100]\r\n",
        "  AT+VOL?\r\n",
        "  AT+PLAY\r\n",
        "  AT+STOP\r\n",
        "  AT+NEXT_TRACK\r\n",
        "  AT+PRE_TRACK\r\n",
        "  AT+BATTERY?\r\n",
        "  AT+VENDOR_CMD=<id 1-223>[,\"<hex>\"[,\"ACK\"]]\r\n",
    };

    facade_expansion_uart_write("+HELP:\r\n");
    for (uint8_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        facade_expansion_uart_write((char *)cmds[i]);
    }

    resp[0] = '\0'; /* at_module will append OK\r\n */
    return true;
}
