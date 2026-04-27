/** @file  at_module.c
 *  @brief AT command module implementation.
 */

/* INCLUDES *******************************************************************/
#include "at_module.h"
#include <string.h>
#include <strings.h>   /* strcasecmp / strncasecmp */

/* PRIVATE TYPES **************************************************************/
/** @brief One entry in the AT server command table. */
typedef struct {
    char                name[AT_CMD_NAME_MAX];
    at_server_handler_t handler;
} at_server_entry_t;

/** @brief One slot in the AT client pending-command queue. */
typedef struct {
    char                 cmd[AT_LINE_MAX_LEN];
    at_client_callback_t callback;
    uint32_t             timeout_ms;
    uint32_t             send_tick;
    bool                 in_use;
} at_client_slot_t;

/** @brief Client state machine states. */
typedef enum {
    CLIENT_IDLE,      /*!< Nothing pending, ready to send next queued command. */
    CLIENT_WAITING,   /*!< Command sent, waiting for OK / ERROR from remote.   */
} at_client_state_t;

/* PRIVATE GLOBALS ************************************************************/
/* UART function pointers */
static void     (*s_tx_fn)(char *str);
static uint8_t  (*s_rx_fn)(void);
static uint32_t (*s_tick_fn)(void);

/* Line accumulator */
static char     s_line_buf[AT_LINE_MAX_LEN];
static uint16_t s_line_idx;

/* AT Server */
static at_server_entry_t s_server_table[AT_SERVER_MAX_COMMANDS];
static uint8_t           s_server_count;

/* Fallback handler — called for any unrecognised or malformed input. */
static void (*s_fallback_fn)(void) = NULL;

/* AT Client */
static at_client_slot_t  s_queue[AT_CLIENT_QUEUE_SIZE];
static uint8_t           s_queue_head;   /* index of the active (sent) slot  */
static uint8_t           s_queue_tail;   /* index where next enqueue goes     */
static uint8_t           s_queue_count;  /* number of slots currently in use  */
static at_client_state_t s_client_state;
static char              s_client_resp[AT_CLIENT_RESP_MAX_LEN]; /* accumulated resp  */

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void     line_accumulate(uint8_t byte);
static void     line_dispatch(const char *line);
static void     server_process(const char *line);
static void     client_send_head(void);
static void     client_handle_final(bool success);
static void     client_append_resp(const char *line);
static uint8_t  queue_next(uint8_t idx);

/* PUBLIC FUNCTIONS ***********************************************************/
void at_module_init(void     (*tx_fn)(char *str),
                    uint8_t  (*rx_fn)(void),
                    uint32_t (*tick_fn)(void))
{
    s_tx_fn   = tx_fn;
    s_rx_fn   = rx_fn;
    s_tick_fn = tick_fn;

    memset(s_line_buf,    0, sizeof(s_line_buf));
    memset(s_server_table, 0, sizeof(s_server_table));
    memset(s_queue,       0, sizeof(s_queue));

    s_line_idx    = 0;
    s_server_count = 0;
    s_queue_head  = 0;
    s_queue_tail  = 0;
    s_queue_count = 0;
    s_client_state = CLIENT_IDLE;
    s_client_resp[0] = '\0';
}

void at_module_process(void)
{
    uint8_t byte;

    /* 1. Drain RX FIFO — assemble lines. */
    while ((byte = s_rx_fn()) != 0) {
        line_accumulate(byte);
    }

    /* 2. Check client timeout. */
    if (s_client_state == CLIENT_WAITING) {
        at_client_slot_t *slot = &s_queue[s_queue_head];
        uint32_t elapsed = s_tick_fn() - slot->send_tick;

        if (elapsed >= slot->timeout_ms) {
            client_handle_final(false);
        }
    }

    /* 3. If client is idle and queue has items, send the next one. */
    if (s_client_state == CLIENT_IDLE && s_queue_count > 0) {
        client_send_head();
    }
}

bool at_server_register(const char *cmd_name, at_server_handler_t handler)
{
    if (s_server_count >= AT_SERVER_MAX_COMMANDS) {
        return false;
    }

    strncpy(s_server_table[s_server_count].name, cmd_name, AT_CMD_NAME_MAX - 1);
    s_server_table[s_server_count].name[AT_CMD_NAME_MAX - 1] = '\0';
    s_server_table[s_server_count].handler = handler;
    s_server_count++;

    return true;
}

void at_server_list_commands(char *buf, uint16_t buf_size)
{
    if (!buf || buf_size == 0) {
        return;
    }
    buf[0] = '\0';
    uint16_t pos = 0;
    for (uint8_t i = 0; i < s_server_count; i++) {
        if (i > 0 && pos < buf_size - 1) {
            buf[pos++] = ',';
            buf[pos]   = '\0';
        }
        uint16_t space = buf_size - 1 - pos;
        uint16_t nlen  = (uint16_t)strlen(s_server_table[i].name);
        uint16_t copy  = (nlen < space) ? nlen : space;
        memcpy(buf + pos, s_server_table[i].name, copy);
        pos += copy;
        buf[pos] = '\0';
        if (pos >= buf_size - 1) {
            break;
        }
    }
}

void at_module_set_fallback_handler(void (*cb)(void))
{
    s_fallback_fn = cb;
}

bool at_client_send(const char          *cmd,
                    uint32_t             timeout_ms,
                    at_client_callback_t callback)
{
    if (s_queue_count >= AT_CLIENT_QUEUE_SIZE) {
        return false;
    }

    at_client_slot_t *slot = &s_queue[s_queue_tail];
    strncpy(slot->cmd, cmd, AT_LINE_MAX_LEN - 1);
    slot->cmd[AT_LINE_MAX_LEN - 1] = '\0';
    slot->callback   = callback;
    slot->timeout_ms = timeout_ms;
    slot->send_tick  = 0;
    slot->in_use     = true;

    s_queue_tail = queue_next(s_queue_tail);
    s_queue_count++;

    return true;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Accumulate one received byte into the line buffer.
 *
 *  Dispatches on \r or \n to handle all common line endings:
 *    \r      (CR only)   - TeraTerm default Enter key
 *    \r\n    (CR+LF)     - most terminals with "CR+LF" mode
 *    \n      (LF only)   - Linux style
 *
 *  When \r\n arrives, \r triggers dispatch; the following \n sees
 *  idx == 0 and is a no-op, preventing a spurious empty dispatch.
 */
static void line_accumulate(uint8_t byte)
{
    if (byte == '\r') {
        if (s_line_idx > 0) {
            s_line_buf[s_line_idx] = '\0';
            line_dispatch(s_line_buf);
            s_line_idx = 0;
        }
        return;
    }

    if (byte == '\n') {
        if (s_line_idx > 0) {
            s_line_buf[s_line_idx] = '\0';
            line_dispatch(s_line_buf);
            s_line_idx = 0;
        }
        return;
    }

    if (s_line_idx < AT_LINE_MAX_LEN - 1) {
        s_line_buf[s_line_idx++] = (char)byte;
    }
    /* else: line overflow — drop byte, wait for next \r/\n to reset. */
}

/** @brief Route a complete line to the correct handler.
 *
 *  Routing rules:
 *    "AT..."          → server (incoming command from remote)
 *    "OK"             → client final: success
 *    "ERROR"          → client final: failure
 *    "+..."           → client response data (if client is waiting)
 *    anything else    → ignored (future: URC callback)
 */
static void line_dispatch(const char *line)
{
    /* Incoming AT command → always handled by server. */
    if (strncasecmp(line, "AT", 2) == 0) {
        server_process(line);
        return;
    }

    /* Final response tokens. */
    if (strcasecmp(line, "OK") == 0) {
        client_handle_final(true);
        return;
    }

    if (strcasecmp(line, "ERROR") == 0) {
        client_handle_final(false);
        return;
    }

    /* Response data line (e.g. "+CGMI: ...") — buffer for active client cmd. */
    if (line[0] == '+' && s_client_state == CLIENT_WAITING) {
        client_append_resp(line);
        return;
    }

    /*
     * Unrecognised line while client is idle → treat as malformed AT command
     * and reply ERROR so the remote gets feedback.
     * While client is WAITING, unknown lines may be unsolicited response data;
     * buffer them like '+' lines rather than replying ERROR.
     */
    if (s_client_state == CLIENT_WAITING) {
        client_append_resp(line);
    } else {
        if (s_fallback_fn != NULL) {
            s_fallback_fn();
        } else {
            s_tx_fn("+CME ERROR: INVALID_CMD\r\n");
        }
    }
}

/** @brief Parse and execute an incoming AT command from the remote.
 *
 *  Supported forms:
 *    AT            → bare ping, reply OK
 *    AT+NAME       → execute (args = "")
 *    AT+NAME?      → read    (args = "?")
 *    AT+NAME=val   → set     (args = "=val")
 *    AT+NAME=?     → test    (args = "=?")
 */
static void server_process(const char *line)
{
    /* Bare "AT" ping. */
    if (strcasecmp(line, "AT") == 0) {
        s_tx_fn("OK\r\n");
        return;
    }

    /* All other commands must begin with "AT+". */
    if (strncasecmp(line, "AT+", 3) != 0) {
        s_tx_fn("+CME ERROR: INVALID_CMD\r\n");
        return;
    }

    const char *name_start = line + 3;

    /* Find end of command name: stop at '=', '?', or end-of-string. */
    const char *p = name_start;
    while (*p && *p != '=' && *p != '?') {
        p++;
    }

    /* Copy name into a local buffer for case-insensitive lookup. */
    char name[AT_CMD_NAME_MAX];
    uint16_t name_len = (uint16_t)(p - name_start);
    if (name_len >= AT_CMD_NAME_MAX) {
        name_len = AT_CMD_NAME_MAX - 1;
    }
    strncpy(name, name_start, name_len);
    name[name_len] = '\0';

    /* args = everything from p onward ("", "?", "=val", "=?"). */
    const char *args = p;

    /* Look up handler in the server table. */
    for (uint8_t i = 0; i < s_server_count; i++) {
        if (strcasecmp(s_server_table[i].name, name) == 0) {
            char resp[AT_LINE_MAX_LEN];
            resp[0] = '\0';

            bool ok = s_server_table[i].handler(args, resp, sizeof(resp));

            if (ok) {
                if (resp[0] != '\0') {
                    s_tx_fn(resp);
                    s_tx_fn("\r\n");
                }
                s_tx_fn("OK\r\n");
            } else {
                s_tx_fn("+CME ERROR: ");
                s_tx_fn(resp[0] != '\0' ? resp : "CMD_FAILED");
                s_tx_fn("\r\n");
            }
            return;
        }
    }

    /* No handler registered for this command — invoke fallback (e.g. print help). */
    if (s_fallback_fn != NULL) {
        s_fallback_fn();
    } else {
        s_tx_fn("+CME ERROR: UNKNOWN_CMD\r\n");
    }
}

/** @brief Transmit the command at the head of the queue and start the timer. */
static void client_send_head(void)
{
    at_client_slot_t *slot = &s_queue[s_queue_head];

    s_client_resp[0] = '\0';
    slot->send_tick  = s_tick_fn();
    s_client_state   = CLIENT_WAITING;

    s_tx_fn(slot->cmd);
}

/** @brief Called when OK, ERROR, or timeout is received for the active command. */
static void client_handle_final(bool success)
{
    if (s_client_state != CLIENT_WAITING) {
        return;
    }

    at_client_slot_t *slot = &s_queue[s_queue_head];

    s_client_state = CLIENT_IDLE;

    if (slot->callback != NULL) {
        slot->callback(success, success ? s_client_resp : NULL);
    }

    slot->in_use = false;
    s_queue_head = queue_next(s_queue_head);
    s_queue_count--;
    s_client_resp[0] = '\0';
}

/** @brief Append a response data line to the client response buffer. */
static void client_append_resp(const char *line)
{
    /* If buffer already has content, append a newline separator. */
    uint16_t existing = (uint16_t)strlen(s_client_resp);

    if (existing > 0 && existing < AT_CLIENT_RESP_MAX_LEN - 1) {
        s_client_resp[existing++] = '\n';
        s_client_resp[existing]   = '\0';
    }

    uint16_t space    = (uint16_t)(AT_CLIENT_RESP_MAX_LEN - 1 - existing);
    uint16_t line_len = (uint16_t)strlen(line);
    uint16_t copy_len = (line_len < space) ? line_len : space;

    if (copy_len > 0) {
        memcpy(s_client_resp + existing, line, copy_len);
        s_client_resp[existing + copy_len] = '\0';
    }
}

/** @brief Advance a circular queue index. */
static uint8_t queue_next(uint8_t idx)
{
    return (uint8_t)((idx + 1) % AT_CLIENT_QUEUE_SIZE);
}
