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
static bool                 s_disconnect_requested = false;
static bool                 s_shutdown_requested   = false;

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

    at_server_register("UWB_CONN_STATUS", handler_uwb_conn_status);


    at_server_register("UWB_PAIR",        handler_uwb_pair);
    at_server_register("FW_VERSION",      handler_fw_version);
    at_server_register("CONN_LM",         handler_conn_lm);
    at_server_register("I2S_MUX",         handler_i2s_mux);
    at_server_register("UWB_CONNECT",     handler_uwb_connect);
    at_server_register("UWB_DISCONNECT",  handler_uwb_disconnect);
    at_server_register("UWB_SHUTDOWN",    handler_uwb_shutdown);
    at_server_register("UWB_GET_ROLE",    handler_uwb_get_role);
    at_server_register("VOL",             handler_vol);
    at_server_register("PLAY",            handler_play);
    at_server_register("STOP",            handler_stop);
    at_server_register("NEXT_TRACK",      handler_next_track);
    at_server_register("PRE_TRACK",       handler_pre_track);
    at_server_register("BATTERY",         handler_battery);

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

void at_cmd_core_notify_uwb_ready(void)
{
    facade_expansion_uart_write("+EVENT: UWB_READY\r\n");
}

void at_cmd_core_register_pair_cb(void (*cb)(void))
{
    s_pair_cb = cb;
}

void at_cmd_core_set_uwb_conn_status(at_uwb_conn_status_t status)
{
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

    /* Reset MCU deferred — after at_module_process() has sent OK. */
    if (s_reset_requested) {
        s_reset_requested = false;
        facade_system_reset();
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
        facade_uwb_shutdown(); /* assert radio shutdown pin(s) */
    }

    if (s_link_status_cb == NULL || s_uwb_conn_status == AT_UWB_CONN_STATUS_PAIRING) {
        return;
    }

    /* Connecting window: wait for link-up or timeout → CONNECT_FAIL. */
    if (s_uwb_conn_status == AT_UWB_CONN_STATUS_CONNECTING) {
        if (s_link_status_cb()) {
            s_uwb_conn_status = AT_UWB_CONN_STATUS_CONNECTED;
            facade_expansion_uart_write("+EVENT: UWB_CONNECTED\r\n");
        } else if (facade_get_tick_ms() - s_connect_start_tick >= AT_UWB_CONNECT_TIMEOUT_MS) {
            s_uwb_conn_status = AT_UWB_CONN_STATUS_STANDBY;
            facade_expansion_uart_write("+EVENT: UWB_CONNECT_FAIL\r\n");
        }
        return;
    }

    at_uwb_conn_status_t new_status = s_link_status_cb()
                                      ? AT_UWB_CONN_STATUS_CONNECTED
                                      : AT_UWB_CONN_STATUS_STANDBY;

    if (new_status != s_uwb_conn_status) {
        s_uwb_conn_status = new_status;
        if (new_status == AT_UWB_CONN_STATUS_CONNECTED) {
            facade_expansion_uart_write("+EVENT: UWB_CONNECTED\r\n");
        } else {
            s_link_quality_weak = false; /* reset on disconnect so WEAK can fire again */
            facade_expansion_uart_write("+EVENT: UWB_DISCONNECTED\r\n");
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
                facade_expansion_uart_write("+EVENT: UWB_QUALITY:WEAK\r\n");
            } else if (s_link_quality_weak && margin >= AT_UWB_LINK_QUALITY_GOOD_THRESHOLD_DB) {
                s_link_quality_weak = false;
                facade_expansion_uart_write("+EVENT: UWB_QUALITY:GOOD\r\n");
            }
        }
    }
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief AT+VER — return SDK version string. */
static bool handler_ver(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    snprintf(resp, resp_size, "+VER: SPARK SDK SR1100 v2.3.0");
    return true;
}

/** @brief AT+FW_VERSION? — return firmware version string. */
static bool handler_fw_version(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    snprintf(resp, resp_size, "+FW_VERSION: v2.3.0");
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

/** @brief AT+UWB_CONN_STATUS? — current UWB connection status (0=Standby, 1=Pairing, 2=Connected). */
static bool handler_uwb_conn_status(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    snprintf(resp, resp_size, "+UWB_CONN_STATUS: %d", (int)s_uwb_conn_status);
    return true;
}

/** @brief AT+UWB_PAIR — trigger UWB pairing via registered callback. */
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

/** @brief AT+UWB_SHUTDOWN — disconnect and assert hardware shutdown pin on UWB radio(s). */
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
        s_cmd_tx_cb(0x01 /* CMD_VOL */, (uint8_t)vol); /* DG: forward over UWB */
    }
    if (s_vol_hw_cb != NULL) {
        s_vol_hw_cb((uint8_t)vol); /* HS: apply to hardware */
    }
    snprintf(resp, resp_size, "OK");
    return true;
}

/** @brief AT+STOP — DG: forward Stop over UWB to HS; HS: apply locally. */
static bool handler_stop(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    if (s_cmd_tx_cb != NULL) {
        s_cmd_tx_cb(0x03 /* CMD_STOP */, 0); /* DG: forward to HS over UWB */
    }
    if (s_stop_hw_cb != NULL) {
        s_stop_hw_cb(); /* HS: apply locally */
    }
    snprintf(resp, resp_size, "OK");
    return true;
}

/** @brief AT+NEXT_TRACK — DG: forward over UWB to HS; HS: apply locally. */
static bool handler_next_track(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    if (s_cmd_tx_cb != NULL) {
        s_cmd_tx_cb(0x04 /* CMD_NEXT_TRACK */, 0);
    }
    if (s_next_track_hw_cb != NULL) {
        s_next_track_hw_cb();
    }
    snprintf(resp, resp_size, "OK");
    return true;
}

/** @brief AT+PRE_TRACK — DG: forward over UWB to HS; HS: apply locally. */
static bool handler_pre_track(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    if (s_cmd_tx_cb != NULL) {
        s_cmd_tx_cb(0x05 /* CMD_PRE_TRACK */, 0);
    }
    if (s_pre_track_hw_cb != NULL) {
        s_pre_track_hw_cb();
    }
    snprintf(resp, resp_size, "OK");
    return true;
}

/** @brief AT+PLAY — DG: forward Play/Pause over UWB to HS; HS: apply locally. */
static bool handler_play(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    if (s_cmd_tx_cb != NULL) {
        s_cmd_tx_cb(0x02 /* CMD_PLAY */, 0); /* DG: forward to HS over UWB */
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

/** @brief AT+UWB_GET_ROLE? — report device role (0=Node, 1=Coordinator). */
static bool handler_uwb_get_role(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    snprintf(resp, resp_size, "+GET_ROLE: %d (%s)", (int)s_device_role,
             s_device_role == AT_DEVICE_ROLE_NODE ? "Node/HS" : "Coordinator/DG");
    return true;
}

/** @brief AT+UWB_DISCONNECT — terminate the active UWB connection. */
static bool handler_uwb_disconnect(const char *args, char *resp, uint16_t resp_size)
{
    (void)args;
    s_disconnect_requested = true;
    snprintf(resp, resp_size, "OK");
    return true;
}

/** @brief AT+UWB_CONNECT — re-establish UWB connection using stored pairing address. */
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
        "  AT+UWB_CONN_STATUS?\r\n",
        "  AT+UWB_PAIR\r\n",
        "  AT+MODULE_RESET\r\n",
        //"  AT+CONN_LM? (internal)\r\n",
        "  AT+I2S_MUX\r\n",
        "  AT+UWB_CONNECT\r\n",
        "  AT+UWB_DISCONNECT\r\n",
        "  AT+UWB_SHUTDOWN\r\n",
        "  AT+UWB_GET_ROLE? (0=Node/HS, 1=Coordinator/DG)\r\n",
        "  AT+VOL=[0-100]\r\n",
        "  AT+VOL?\r\n",
        "  AT+PLAY\r\n",
        "  AT+STOP\r\n",
        "  AT+NEXT_TRACK\r\n",
        "  AT+PRE_TRACK\r\n",
        "  AT+BATTERY?\r\n",
    };

    facade_expansion_uart_write("+HELP:\r\n");
    for (uint8_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        facade_expansion_uart_write((char *)cmds[i]);
    }

    resp[0] = '\0'; /* at_module will append OK\r\n */
    return true;
}
