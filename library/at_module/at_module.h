/** @file  at_module.h
 *  @brief AT command module — bidirectional Hayes AT over any UART.
 *
 *  The module is hardware-agnostic. All UART access is provided via function
 *  pointers passed to at_module_init(), so it can be used with any backend.
 *
 *  Roles:
 *    Server — receives and parses incoming AT commands, dispatches to
 *             registered handlers, replies OK / ERROR.
 *    Client — sends AT commands asynchronously, queues up to
 *             AT_CLIENT_QUEUE_SIZE requests, invokes a callback when the
 *             remote responds with OK / ERROR or when a timeout occurs.
 *
 *  Usage:
 *    1. Call at_module_init() once at startup.
 *    2. Register server commands with at_server_register().
 *    3. Call at_module_process() every iteration of the main loop.
 *    4. Send commands with at_client_send(); result arrives via callback.
 */
#ifndef AT_MODULE_H_
#define AT_MODULE_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CONFIGURATION **************************************************************/
/** @brief Maximum number of AT commands the server can handle. */
#define AT_SERVER_MAX_COMMANDS  128

/** @brief Depth of the client outgoing command queue. */
#define AT_CLIENT_QUEUE_SIZE    4

/** @brief Maximum length of one AT line (command or response), incl. null. */
#define AT_LINE_MAX_LEN         256

/** @brief Maximum length of the accumulated multi-line client response buffer.
 *
 *  Must be >= AT_LINE_MAX_LEN to hold at least one full response line.
 *  Increase if the remote can return multiple long lines before OK/ERROR.
 */
#define AT_CLIENT_RESP_MAX_LEN  1024

/** @brief Maximum length of an AT command name (e.g. "COPS", "CGDCONT"). */
#define AT_CMD_NAME_MAX         32

/* TYPES **********************************************************************/
/**
 * @brief Server-side handler for a single registered AT command.
 *
 * Called when the remote sends "AT+<name>" / "AT+<name>?" / "AT+<name>=...".
 *
 * @param[in]  args      Everything after the command name, e.g. "=1", "?",
 *                       "=?" or "" for a bare execution.
 * @param[out] resp      Buffer to write a response line into (optional).
 *                       Written content is sent before the final OK/ERROR.
 *                       Leave empty (resp[0]='\0') if no extra line needed.
 * @param[in]  resp_size Size of the resp buffer.
 * @return true  → module replies OK
 *         false → module replies ERROR
 */
typedef bool (*at_server_handler_t)(const char *args,
                                    char       *resp,
                                    uint16_t    resp_size);

/**
 * @brief Client-side callback invoked when a sent command completes.
 *
 * @param[in] success   true if the remote replied OK, false for ERROR/timeout.
 * @param[in] response  Any response data line(s) received before OK/ERROR,
 *                      or NULL on timeout.
 */
typedef void (*at_client_callback_t)(bool success, const char *response);

/* PUBLIC FUNCTIONS ***********************************************************/
/**
 * @brief Initialise the AT module.
 *
 * @param[in] tx_fn    Function to transmit a null-terminated string.
 * @param[in] rx_fn    Function to read one byte; returns 0 when FIFO empty.
 * @param[in] tick_fn  Function returning current time in milliseconds.
 */
void at_module_init(void     (*tx_fn)(char *str),
                    uint8_t  (*rx_fn)(void),
                    uint32_t (*tick_fn)(void));

/**
 * @brief Drive the AT module state machine.
 *
 * Must be called repeatedly from the main loop. Drains the RX FIFO, assembles
 * lines, dispatches to server/client, and checks client timeouts.
 */
void at_module_process(void);

/**
 * @brief Register a server-side AT command handler.
 *
 * @param[in] cmd_name  Command name without "AT+" prefix (e.g. "CGMI").
 *                      Case-insensitive matching is used at dispatch time.
 * @param[in] handler   Handler function (see at_server_handler_t).
 * @return true on success, false if the table is full.
 */
bool at_server_register(const char *cmd_name, at_server_handler_t handler);

/**
 * @brief Queue an outgoing AT command (non-blocking).
 *
 * The command string should include the full "AT..." prefix and trailing
 * "\r\n", e.g. "AT+CGMI\r\n".
 *
 * @param[in] cmd         Full AT command string to send.
 * @param[in] timeout_ms  Time in ms to wait for a response before giving up.
 * @param[in] callback    Called when the command completes or times out.
 * @return true if the command was queued, false if the queue is full.
 */
bool at_client_send(const char          *cmd,
                    uint32_t             timeout_ms,
                    at_client_callback_t callback);

/**
 * @brief Fill buf with a comma-separated list of all registered server command
 *        names (e.g. "VER,LED,STATUS,HELP").
 *
 * @param[out] buf       Destination buffer.
 * @param[in]  buf_size  Size of the destination buffer.
 */
void at_server_list_commands(char *buf, uint16_t buf_size);

/**
 * @brief Register a fallback handler called for any unrecognised or malformed input.
 *
 * When set, replaces the default "+CME ERROR: UNKNOWN_CMD / INVALID_CMD" response.
 * Typical use: register a function that prints AT+HELP output so the user always
 * sees the supported command list on bad input.
 *
 * @param[in] cb  Void callback to invoke on bad input. Pass NULL to restore default.
 */
void at_module_set_fallback_handler(void (*cb)(void));

#ifdef __cplusplus
}
#endif

#endif /* AT_MODULE_H_ */
