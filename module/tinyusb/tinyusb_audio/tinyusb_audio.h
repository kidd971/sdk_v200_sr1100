/** @file  tinyusb_audio.h
 *  @brief TinyUSB module with audio functionality.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef TINYUSB_AUDIO_H_
#define TINYUSB_AUDIO_H_

/* INCLUDES *******************************************************************/
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Start the speaker endpoint.
 */
void tinyusb_audio_start_spk(void);

/** @brief Stop the speaker endpoint.
 */
void tinyusb_audio_stop_spk(void);

/** @brief Set the EP IN target fifo size.
 *
 *  @param[in] target_fifo_size  Target fifo size in bytes.
 */
void tinyusb_audio_set_ep_in_target_fifo_size(uint16_t target_fifo_size);

/** @brief Set the speaker rx callback
 *
 *  @param[in] callback        Callback function to call on reception of speaker audio
 *  @param[in] fifo_threshold  Fifo threshold in bytes to trigger the callback
 */
void tinyusb_audio_set_speaker_rx_callback(void (*callback)(void), uint32_t fifo_threshold);

/** @brief Get the current size of the EP IN fifo.
 *
 *  @return Number of bytes in the EP IN fifo.
 */
uint32_t tinyusb_audio_get_epin_fifo_size(void);

/** @brief Write audio data to the USB audio input endpoint.
 *
 *  @param[in] data_in  Pointer to the data to write.
 *  @param[in] size     Length of the data to write.
 *  @return Number of bytes writen.
 */
uint16_t tinyusb_audio_write(void *data_in, uint16_t size);

/** @brief Read audio data from the USB audio output endpoint.
 *
 *  @param[out] data_out  Pointer to the data buffer to store data read from the USB endpoint.
 *  @param[in]  size      Length of the data to read.
 *  @return Number of bytes read.
 */
uint16_t tinyusb_audio_read(void *data_out, uint16_t size);

/** @brief Send media key report over USB HID.
 *
 *  @param[in] play_pause   Send play/pause key.
 *  @param[in] volume_up    Send volume up key.
 *  @param[in] volume_down  Send volume down key.
 */
void tinyusb_audio_send_media_key(bool play_pause, bool volume_up, bool volume_down);

/** @brief Get the output endpoint volume in db.
 *
 *  @return Output endpoint volume in db.
 */
int8_t tinyusb_audio_get_epout_volume_db(void);

/** @brief Get the input endpoint volume in db.
 *
 *  @return Input endpoint volume in db.
 */
int8_t tinyusb_audio_get_epin_volume_db(void);

#ifdef __cplusplus
}
#endif

#endif /* TINYUSB_AUDIO_H_ */
