/** @file  at_command_test.c
 *  @brief AT command module integration test.
 *
 *  Wires at_module to the expansion UART (USART2, PA2=TX / PA3=RX, 115200).
 *  Uses profiler_backend for all hardware access.
 *
 *  What this test does:
 *    1. Registers two example AT server commands:
 *         AT+VER   -> returns a version string
 *         AT+LED=n -> turns LED n on/off (stub)
 *    2. Every 5 seconds sends "AT+VER\r\n" as an AT client command and
 *       prints the result to the STLink debug log.
 *    3. Incoming AT commands from the remote are handled continuously.
 *
 *  Connect PA2/PA3 to a serial terminal or another device at 115200-8N1.
 *  STLink COM port (115200) shows the debug log output.
 */

/* INCLUDES *******************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "at_module.h"        /* library/at_module */
#include "profiler_facade.h"

/* CONSTANTS ******************************************************************/
#define AT_BAUD_RATE              115200
#define CLIENT_SEND_PERIOD_INIT   200     /* ms, first interval              */
#define CLIENT_SEND_PERIOD_MAX    10000   /* ms, cap to avoid infinite growth */
#define CLIENT_SEND_MAX_COUNT     4       /* stop after this many sends       */

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static bool handler_ver(const char *args, char *resp, uint16_t resp_size);
static bool handler_led(const char *args, char *resp, uint16_t resp_size);
static bool handler_status(const char *args, char *resp, uint16_t resp_size);
static bool handler_help(const char *args, char *resp, uint16_t resp_size);
static void on_ver_response(bool success, const char *response);

/* PUBLIC FUNCTIONS ***********************************************************/
int main(void)
{
    /* Board and clock init. */
    facade_board_init();

    /* Debug log on STLink UART4 (PC10/PC11). */
    facade_log_init();
    facade_log_write("\r\n=== AT Command Test ===\r\n");

    /* AT module on Expansion USART2 (PA2/PA3). */
    facade_expansion_uart_init(AT_BAUD_RATE);

    at_module_init(facade_expansion_uart_write,
                   facade_expansion_uart_read_byte,
                   facade_get_tick_ms);

    /* Register server-side commands. */
    at_server_register("VER",    handler_ver);
    at_server_register("LED",    handler_led);
    at_server_register("STATUS", handler_status);
    at_server_register("HELP",   handler_help);

    facade_log_write("AT server ready. Registered: AT+VER, AT+LED, AT+STATUS, AT+HELP\r\n\r\n");

    uint32_t last_send      = 0;
    uint32_t send_interval  = CLIENT_SEND_PERIOD_INIT;
    uint32_t ver_index      = 0;

    while (1) {
        /* Drive the AT state machine. */
        at_module_process();

        /* Send AT+VER<n> up to CLIENT_SEND_MAX_COUNT times with exponential backoff. */
        uint32_t now = facade_get_tick_ms();
        if (ver_index < CLIENT_SEND_MAX_COUNT && (now - last_send) >= send_interval) {
            last_send = now;
            if (at_client_send("AT+VER\r\n", 2000, on_ver_response)) {
                ver_index++;
                char log[64];
                if (ver_index < CLIENT_SEND_MAX_COUNT) {
                    snprintf(log, sizeof(log), "[client] Sent AT+VER #%lu (next in %lums)\r\n",
                             (unsigned long)ver_index,
                             (unsigned long)(send_interval * 2 < CLIENT_SEND_PERIOD_MAX
                                             ? send_interval * 2
                                             : CLIENT_SEND_PERIOD_MAX));
                } else {
                    snprintf(log, sizeof(log), "[client] Sent AT+VER #%lu (done)\r\n",
                             (unsigned long)ver_index);
                }
                facade_log_write(log);

                /* Double the interval, cap at maximum. */
                if (send_interval < CLIENT_SEND_PERIOD_MAX / 2) {
                    send_interval *= 2;
                } else {
                    send_interval = CLIENT_SEND_PERIOD_MAX;
                }
            } else {
                facade_log_write("[client] Queue full, skipped\r\n");
            }
        }
    }

    return 0;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Server handler: AT+VER — return firmware version string. */
static bool handler_ver(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    snprintf(resp, resp_size, "+VER: AT-Module-Test v1.0");
    return true;
}

/** @brief Server handler: AT+LED=<n> — stub LED control, echoes value back. */
static bool handler_led(const char *args, char *resp, uint16_t resp_size)
{
    /* Expect args like "=1" or "=0". */
    if (args[0] != '=') {
        snprintf(resp, resp_size, "USAGE: AT+LED=<0|1>");
        return false;
    }

    snprintf(resp, resp_size, "+LED: %s", args + 1);

    char log[64];
    snprintf(log, sizeof(log), "[server] LED command, val=%s\r\n", args + 1);
    facade_log_write(log);

    return true;
}

/** @brief Server handler: AT+STATUS — return uptime in milliseconds. */
static bool handler_status(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    snprintf(resp, resp_size, "+STATUS: uptime=%lums", (unsigned long)facade_get_tick_ms());
    return true;
}

/** @brief Server handler: AT+HELP — list all registered commands. */
static bool handler_help(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    char names[AT_LINE_MAX_LEN];
    at_server_list_commands(names, sizeof(names));
    snprintf(resp, resp_size, "+HELP: AT+%s",
             names[0] != '\0' ? names : "(none)");
    return true;
}

/** @brief Client callback: called when AT+VER response arrives or times out. */
static void on_ver_response(bool success, const char *response)
{
    if (success) {
        char log[AT_LINE_MAX_LEN + 32];
        snprintf(log, sizeof(log), "[client] AT+VER OK: %s\r\n",
                 response ? response : "(empty)");
        facade_log_write(log);
    } else {
        facade_log_write("[client] AT+VER FAILED / timeout\r\n");
    }
}
