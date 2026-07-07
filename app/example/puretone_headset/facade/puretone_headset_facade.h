/** @file  puretone_headset_facade.h
 *  @brief Facades for low-level platform-specific features required by the application example.
 *
 *  @note This header defines the interfaces for various hardware features used by the puretone headset example.
 *
 *  These facades abstract the underlying platform-specific implementations of features like SPI communication, IRQ
 *  handling, timer functions, and context switching mechanisms. The actual implementations are selected at compile time
 *  based on the target platform, allowing for flexibility and portability across different hardware. The facade is
 *  designed to be a compile-time dependency only, with no support for runtime polymorphism. This ensures tight
 *  integration with the build system and minimal overhead.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef PURETONE_HEADSET_FACADE_H_
#define PURETONE_HEADSET_FACADE_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "common_facade.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Certification modes.
 */
typedef enum facade_certification_mode {
    FACADE_CERTIF_NONE,
    FACADE_CERTIF_AUDIO_96k_24_BIT,
    FACADE_CERTIF_AUDIO_48k_24_BIT,
    FACADE_CERTIF_AUDIO_48k_16_BIT,
    FACADE_CERTIF_AUDIO_48k_ADPCM,
    FACADE_CERTIF_DATA,
} facade_certification_mode_t;

/** @brief Function callbacks for button presses.
 */
typedef struct facade_button_callbacks {
    /*! Function callback to pair/unpair the device. */
    void (*pairing_callback)(void);
    /*! Function callback to increase the playback volume. */
    void (*volume_up_callback)(void);
    /*! Function callback to decrease the playback volume. */
    void (*volume_down_callback)(void);
    /*! Funciotn callback to change fallback mode. */
    void (*fallback_callback)(void);
} facade_button_callbacks_t;

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Initialize the Coordinator's audio peripherals.
 *
 *  @param[in] i2s_master_mode  True to initialize as I2S master; false for slave with MCLK output.
 *  @note Configure the serial audio interface to Mono or Stereo.
 */
void facade_audio_coord_init(bool i2s_master_mode);

/** @brief Initialize the Node's SAI peripheral.
 *
 *  @param[in] i2s_master_mode  True to initialize as I2S master; false for slave with MCLK output.
 *  @note Configure the serial audio interface to Mono or Stereo.
 */
void facade_audio_node_init(bool i2s_master_mode);

/** @brief Deinitialize the audio peripherals.
 */
void facade_audio_deinit(void);

/** @brief Set the serial audio interface transfer complete callbacks.
 *
 *  @note Set NULL in place of unused callback.
 *
 *  @param[in] tx_callback  Audio I2S TX complete callback.
 *  @param[in] rx_callback  Audio I2S RX complete callback.
 */
void facade_set_audio_complete_callback(void (*tx_callback)(void), void (*rx_callback)(void));

/** @brief Read Coordinator's button state to define if certification mode is required.
 *
 *  @return The certification mode to be applied.
 */
facade_certification_mode_t facade_coord_get_certification_mode(void);

/** @brief Read Node's button state to define if certification mode is required.
 *
 *  @return The certification mode to be applied.
 */
facade_certification_mode_t facade_node_get_certification_mode(void);

/** @brief Set button function callbacks.
 *
 *  @param[in] button_callbacks  Button function callback structure.
 */
void facade_set_button_callbacks(facade_button_callbacks_t button_callbacks);

/** @brief Poll for button presses and execute function callback.
 */
void facade_button_handling(void);

/** @brief Notify user of the wireless audio TX connection status.
 */
void facade_tx_audio_conn_status(void);

/** @brief Notify user of the wireless data TX connection status.
 */
void facade_tx_data_conn_status(void);

/** @brief Notify user of the wireless audio RX connection status.
 */
void facade_rx_audio_conn_status(void);

/** @brief Notify user of the wireless data RX connection status.
 */
void facade_rx_data_conn_status(void);

/** @brief Initialize the timer of the main channel audio process.
 *
 *  @param[in] callback  Callback function to execute on timer event.
 */
void facade_audio_process_main_channel_timer_init(void (*callback)(void));

/** @brief Initialize the timer of the back channel audio process.
 *
 *  @param[in] callback  Callback function to execute on timer event.
 */
void facade_audio_process_back_channel_timer_init(void (*callback)(void));

/** @brief Start the audio process of the main channel timer.
 */
void facade_audio_process_main_channel_timer_start(void);

/** @brief Start the audio process of the back channel timer.
 */
void facade_audio_process_back_channel_timer_start(void);

/** @brief Generate an event for the audio process of the main channel timer.
 */
void facade_audio_process_main_channel_timer_trigger(void);

/** @brief Generate an event for the audio process of the back channel timer.
 */
void facade_audio_process_back_channel_timer_trigger(void);

/** @brief Stop the audio process of the main channel timer.
 */
void facade_audio_process_main_channel_timer_stop(void);

/** @brief Stop the audio process of the back channel timer.
 */
void facade_audio_process_back_channel_timer_stop(void);

/** @brief Initialize and set the data timer period which include statistics and data transmitted to the other device.
 *
 *  @param[in] period_ms  Timer period in ms.
 */
void facade_data_timer_init(uint32_t period_ms);

/** @brief Set the data timer callback.
 *
 *  @param[in] callback  Callback when timer expires.
 */
void facade_data_timer_set_callback(void (*callback)(void));

/** @brief Start the data timer.
 */
void facade_data_timer_start(void);

/** @brief Stop the data timer.
 */
void facade_data_timer_stop(void);

/** @brief Notify user of payload present in frame.
 */
void facade_payload_received_status(void);

/** @brief Notify user of no payload present in frame.
 */
void facade_empty_payload_received_status(void);

/** @brief Read the state of the button that will set the other device's LED state.
 *
 *  @return Returns true if the button is pressed, false otherwise.
 */
bool facade_read_button_state(void);

/** @brief Successful pairing notification LED pattern.
 */
void facade_notify_pairing_successful(void);

/** @brief Set the I2S MUX selection.
 *
 *  @param[in] use_ext  true = external codec port, false = onboard codec.
 */
void facade_set_i2s_mux(bool use_ext);

/** @brief Initialize the ADC peripheral for battery level monitoring.
 */
void facade_battery_init(void);

/** @brief Read the current battery level.
 *
 *  @return Battery level as a percentage (0-100).
 */
uint8_t facade_read_battery_level_pct(void);

/** @brief Write the periodic statistics dump to the board's console.
 *
 *  Board-dependent routing (decided in the backend, where the chip macro is visible):
 *  u535 (LDO board) has no convenient USB CDC, so stats go to the AT-command/expansion
 *  UART (LPUART1, same pins as the AT console); every other board (u5a5 EVK, etc.)
 *  prints to the USB CDC port.
 *
 *  @param[in] string  Null-terminated statistics string to emit.
 */
void facade_stats_write(char *string);

#if USB_AUDIO_ENABLED
/** @brief Configure the coordinator's USB audio.
 */
void facade_configure_coord_usb_audio(void);

/** @brief Configure the node's USB audio.
 */
void facade_configure_node_usb_audio(void);

/** @brief Get the current number of samples in the Coordinator's USB audio TX fifo.
 *
 *  @return Number of samples in the USB audio TX fifo.
 */
uint32_t facade_get_coord_usb_audio_tx_fifo_sample_count(void);

/** @brief Get the current number of samples in the Node's USB audio TX fifo.
 *
 *  @return Number of samples in the USB audio TX fifo.
 */
uint32_t facade_get_node_usb_audio_tx_fifo_sample_count(void);

/** @brief Get the remaining number of bytes in the IN endpoint fifo.
 *
 *  @return The remaining number of bytes in the IN endpoint fifo.
 */
uint32_t facade_app_audio_usb_get_epin_fifo_remaining(void);

/** @brief Set the target fifo size for the input endpoint.
 *
 *  @param[in] target_fifo_size  Target fifo size in bytes.
 */
void facade_app_audio_usb_set_epin_target_fifo_size(uint16_t target_fifo_size);
#endif

#ifdef __cplusplus
}
#endif

#endif /* PURETONE_HEADSET_FACADE_H_ */
