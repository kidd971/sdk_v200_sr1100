/** @file  audio_bidirectional_facade.h
 *  @brief Facades for low-level platform-specific features required by the application example.
 *
 *  @note This header defines the interfaces for various hardware features used by the audio unidirectional example.
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
#ifndef AUDIO_BIDIRECTIONAL_FACADE_H_
#define AUDIO_BIDIRECTIONAL_FACADE_H_

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
    FACADE_CERTIF_AUDIO_24_BIT,
    FACADE_CERTIF_AUDIO_ADPCM,
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
} facade_button_callbacks_t;

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Initialize the Coordinator's audio peripherals.
 *
 *  @note Configure the serial audio interface to Mono or Stereo.
 */
void facade_audio_coord_init(void);

/** @brief Initialize the Node's SAI peripheral.
 *
 *  @note Configure the serial audio interface to Mono or Stereo.
 */
void facade_audio_node_init(void);

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

/** @brief Read button state to define if certification mode is required.
 *
 *  @return The certification mode to be applied.
 */
facade_certification_mode_t facade_get_certification_mode(void);

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

/** @brief Notify user of the fallback status.
 */
void facade_fallback_status(bool on);

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
#endif

/** @brief Set the I2S MUX selection.
 *
 *  @param[in] use_ext  true = external codec port, false = onboard codec.
 */
void facade_set_i2s_mux(bool use_ext);

/** @brief Reconfigure the SAI audio format.
 *
 *  Deinitializes the SAI peripheral and reinitializes it with the new protocol.
 *  Must only be called after facade_audio_coord_init() or facade_audio_node_init().
 *
 *  @param[in] fmt  Format mode: 1=RJF (LSB justified), 2=LJF (MSB justified), 3=I2S standard.
 */
void facade_set_i2s_fmt(uint8_t fmt);

/** @brief Get the current SAI format index (1=RJF, 2=LJF, 3=I2S standard).
 */
uint8_t facade_get_i2s_fmt(void);

/** @brief Initialize the ADC peripheral for battery level monitoring.
 */
void facade_battery_init(void);

/** @brief Read the current battery level.
 *
 *  @return Battery level as a percentage (0-100).
 */
uint8_t facade_read_battery_level_pct(void);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_BIDIRECTIONAL_FACADE_H_ */
