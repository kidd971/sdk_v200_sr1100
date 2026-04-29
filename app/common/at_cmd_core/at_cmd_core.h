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
 *    AT+UWB_CONN_STATUS? — current UWB connection status (0=Standby,1=Pairing,2=Connected)
 *    AT+UWB_PAIR        — trigger UWB pairing (invokes registered pair callback)
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

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Baud rate used for the expansion UART AT channel. */
#define AT_CMD_CORE_BAUD_RATE  115200

/** @brief UWB connection status codes reported by AT+UWB_CONN_STATUS?. */
typedef enum {
    AT_UWB_CONN_STATUS_STANDBY    = 0, /*!< Idle, not yet started. */
    AT_UWB_CONN_STATUS_PAIRING    = 1, /*!< Pairing procedure in progress. */
    AT_UWB_CONN_STATUS_CONNECTED  = 2, /*!< Link established and running. */
    AT_UWB_CONN_STATUS_CONNECTING = 3, /*!< Connection attempt in progress (after AT+UWB_CONNECT). */
} at_uwb_conn_status_t;

/** @brief Timeout for UWB connection attempts triggered by AT+UWB_CONNECT.
 *
 *  If the link is not established within this many milliseconds after
 *  AT+UWB_CONNECT, a +EVENT: UWB_CONNECT_FAIL notification is sent.
 */
#define AT_UWB_CONNECT_TIMEOUT_MS  5000

/** @brief Link margin threshold (dB) below which +EVENT: UWB_QUALITY:WEAK is sent. */
#define AT_UWB_LINK_QUALITY_WEAK_THRESHOLD_DB   5

/** @brief Link margin threshold (dB) above which the WEAK state clears (hysteresis). */
#define AT_UWB_LINK_QUALITY_GOOD_THRESHOLD_DB   10

/** @brief Interval (ms) between link quality polls when connected. */
#define AT_UWB_LINK_QUALITY_CHECK_INTERVAL_MS   1000

/** @brief Device role codes reported by AT+UWB_GET_ROLE?. */
typedef enum {
    AT_DEVICE_ROLE_NODE        = 0, /*!< Node (leaf) device. */
    AT_DEVICE_ROLE_COORDINATOR = 1, /*!< Coordinator device. */
} at_device_role_t;

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
 * @brief Update the UWB connection status reported by AT+UWB_CONN_STATUS?.
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
 * @brief Set the device role reported by AT+UWB_GET_ROLE?.
 *
 * Call once during application startup (before the main loop).
 * Node applications pass AT_DEVICE_ROLE_NODE; coordinator applications
 * pass AT_DEVICE_ROLE_COORDINATOR.
 *
 * @param[in] role  Device role.
 */
void at_cmd_core_set_device_role(at_device_role_t role);

/**
 * @brief Send +EVENT: UWB_READY to the external MCU.
 *
 * Call once after SWC initialization is complete and the device is ready
 * to connect or accept pairing.
 */
void at_cmd_core_notify_uwb_ready(void);

/**
 * @brief Register a connection status getter polled by at_cmd_core_process().
 *
 * The callback returns true when the UWB link is up, false otherwise.
 * at_cmd_core_process() polls this every main-loop iteration and sends
 * +EVENT: UWB_CONNECTED or +EVENT: UWB_DISCONNECTED only when the status
 * changes. Polling is skipped while status is AT_UWB_CONN_STATUS_PAIRING.
 *
 * Typical usage (call once after app_init()):
 *   at_cmd_core_register_link_status_cb(my_get_link_status);
 *
 * @param[in] cb  Function returning true if link is up. May be NULL to unregister.
 */
void at_cmd_core_register_link_status_cb(bool (*cb)(void));

/**
 * @brief Register a callback invoked when AT+UWB_PAIR is received.
 *
 * The callback should initiate the application's pairing procedure.
 * If no callback is registered the command still returns OK but does nothing.
 *
 * @param[in] cb  Function to call on AT+UWB_PAIR. May be NULL to unregister.
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
 * @brief Register a callback invoked when AT+I2S_FMT is received.
 *
 * The callback receives the selected format mode (1=RJF, 2=LJF, 3=I2S standard)
 * and should reinitialize the SAI peripheral with the new protocol.
 * If no callback is registered the command still updates the tracked state
 * but does not touch hardware.
 *
 * @param[in] cb  Function accepting format mode 1-3. May be NULL to unregister.
 */
void at_cmd_core_register_i2s_fmt_cb(void (*cb)(uint8_t fmt));

/**
 * @brief Register a link margin getter polled by AT+CONN_LM?.
 *
 * The callback should call swc_connection_update_stats() on the application's
 * primary RX connection and return the link margin in whole dB
 * (i.e. stats->link_margin_avg / 10, cast to int32_t).
 * Set to NULL to unregister (e.g. on unpair); the command then returns N/A.
 *
 * @param[in] cb  Function returning link margin in dB. May be NULL to unregister.
 */
void at_cmd_core_register_link_margin_cb(int32_t (*cb)(void));

/**
 * @brief Register a callback invoked when an AT command requires sending a
 *        control command to the remote device over the UWB data channel.
 *
 * The callback receives the command type and value (see app_cmd.h) and is
 * responsible for packing an app_cmd_t and calling wireless_send_data().
 * Used on the DG side only; HS does not register this callback.
 *
 * @param[in] cb  Function accepting (cmd_type, value). May be NULL to unregister.
 */
void at_cmd_core_register_cmd_tx_cb(void (*cb)(uint8_t cmd_type, uint8_t value));

/**
 * @brief Register a callback invoked when volume should be applied to hardware.
 *
 * Used on the HS side. Called when AT+VOL=N is received from the local SOC
 * OR when a CMD_VOL packet arrives from the DG over the UWB data channel.
 * The callback receives the new volume level (0-100) and should apply it to
 * the audio hardware (e.g. via sac_volume_ctrl).
 * Not used on the DG side — DG forwards the command over UWB instead.
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
 * @brief Register a callback invoked when AT+UWB_SHUTDOWN is received.
 *
 * The callback should perform software cleanup (stop timers, call swc_disconnect,
 * stop audio pipelines) before the hardware shutdown pin is asserted by the core.
 * The core always calls facade_uwb_shutdown() after this callback returns,
 * regardless of whether a callback is registered.
 * If the device is already disconnected the callback should be a no-op.
 *
 * @param[in] cb  Function to call on AT+UWB_SHUTDOWN. May be NULL to unregister.
 */
void at_cmd_core_register_shutdown_cb(void (*cb)(void));

/**
 * @brief Register a callback invoked when AT+UWB_DISCONNECT is received.
 *
 * The callback should terminate the active UWB connection and stop all
 * associated timers/pipelines (i.e. call unpair_device()). The pairing
 * address is preserved so that AT+UWB_CONNECT can reconnect afterwards.
 * If the device is already disconnected the callback should be a no-op.
 *
 * @param[in] cb  Function to call on AT+UWB_DISCONNECT. May be NULL to unregister.
 */
void at_cmd_core_register_disconnect_cb(void (*cb)(void));

/**
 * @brief Register a callback invoked when AT+UWB_CONNECT is received.
 *
 * The callback should attempt to re-establish the UWB connection using the
 * previously assigned pairing addresses, without entering the pairing procedure.
 * If the device is already connected the callback should be a no-op.
 * If no pairing address is available the callback should do nothing.
 *
 * @param[in] cb  Function to call on AT+UWB_CONNECT. May be NULL to unregister.
 */
void at_cmd_core_register_connect_cb(void (*cb)(void));

#ifdef __cplusplus
}
#endif

#endif /* AT_CMD_CORE_H_ */
