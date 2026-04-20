/** @file  quasar_audio.h
 *  @brief Initialize audio related peripherals.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_AUDIO_H_
#define QUASAR_AUDIO_H_

/* INCLUDES *******************************************************************/
#include "quasar_def.h"
#include "quasar_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/** @brief The SAI mono/stereo modes.
 */
typedef enum quasar_sai_mono_stereo {
    QUASAR_SAI_MODE_MONO = 1,
    QUASAR_SAI_MODE_STEREO = 2
} quasar_sai_mono_stereo_t;

/** @brief The supported SAI bit depth.
 */
typedef enum quasar_sai_bit_depth {
    QUASAR_SAI_BIT_DEPTH_16BITS = SAI_PROTOCOL_DATASIZE_16BIT,
    QUASAR_SAI_BIT_DEPTH_16BITS_EXT = SAI_PROTOCOL_DATASIZE_16BITEXTENDED,
    QUASAR_SAI_BIT_DEPTH_24BITS = SAI_PROTOCOL_DATASIZE_24BIT,
    QUASAR_SAI_BIT_DEPTH_32BITS = SAI_PROTOCOL_DATASIZE_32BIT,
} quasar_sai_bit_depth_t;

/** @brief SAI modes.
 */
typedef enum quasar_sai_mode {
    /* SAI is master. */
    QUASAR_SAI_MASTER_MODE,
    /* SAI is slave. */
    QUASAR_SAI_SLAVE_MODE,
    /* SAI is slave and the SAI clock is output on the MCLK to drive an external peripheral. */
    QUASAR_SAI_SLAVE_MODE_MCLK,
} quasar_sai_mode_t;

/** @brief I2S MUX selections.
 */
typedef enum quasar_i2s_mux_select {
    QUASAR_SELECT_ON_BOARD_CODEC,
    QUASAR_SELECT_EXT_CODEC,
} quasar_i2s_mux_select_t;

/** @brief The supported SAI audio frequencies.
 */
typedef enum quasar_sai_frequency {
    QUASAR_SAI_AUDIO_FREQUENCY_96K = SAI_AUDIO_FREQUENCY_96K,
    QUASAR_SAI_AUDIO_FREQUENCY_48K = SAI_AUDIO_FREQUENCY_48K,
    QUASAR_SAI_AUDIO_FREQUENCY_32K = SAI_AUDIO_FREQUENCY_32K,
    QUASAR_SAI_AUDIO_FREQUENCY_16K = SAI_AUDIO_FREQUENCY_16K,
    QUASAR_SAI_AUDIO_FREQUENCY_8K = SAI_AUDIO_FREQUENCY_8K,
    QUASAR_SAI_AUDIO_FREQUENCY_MCKDIV = SAI_AUDIO_FREQUENCY_MCKDIV,
} quasar_sai_frequency_t;

/** @brief The supported SAI audio protocols.
 */
typedef enum quasar_sai_protocol {
    QUASAR_SAI_PROTOCOL_I2S_STANDARD = SAI_I2S_STANDARD,
    QUASAR_SAI_PROTOCOL_I2S_MSBJUSTIFIED = SAI_I2S_MSBJUSTIFIED,
    QUASAR_SAI_PROTOCOL_I2S_LSBJUSTIFIED = SAI_I2S_LSBJUSTIFIED,
} quasar_sai_protocol_t;

/** @brief SAI global configuration structure.
 */
typedef struct quasar_sai_config {
    quasar_sai_mono_stereo_t rx_sai_mono_stereo;
    quasar_sai_mono_stereo_t tx_sai_mono_stereo;
    quasar_sai_bit_depth_t sai_bit_depth;
    quasar_sai_mode_t sai_mode;
    quasar_sai_frequency_t sai_audio_frequency;
    quasar_sai_protocol_t sai_protocol;
} quasar_sai_config_t;

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Configure the Audio's SAI peripheral and initialize it.
 *
 *  This function configures and initialized multiple peripherals sequentially.
 *
 *      1. Enable SAI clocks.
 *      2. Initialize SAI GPIOs.
 *      3. Configure SAI mono stereo mode.
 *      4. Initialize SAI block configuration.
 *      5. Initialize SAI DMA NVIC.
 *      6. Initialize SAI DMA peripherals.
 *
 *  @param[in]  sai_config  SAI global configuration structure.
 *  @param[out] err         Pointer to store error status.
 */
void quasar_audio_init_sai(quasar_sai_config_t sai_config, quasar_bsp_status_t *err);

/** @brief Deinitialize the SAI peripheral used for audio.
 *
 *  @param[out] err  Pointer to store error status.
 */
void quasar_audio_deinit_sai(quasar_bsp_status_t *err);

/** @brief Configure the CODEC I2C peripheral and initialize it.
 *
 *  The following steps are performed.
 *
 *      1. Enable I2C clock.
 *      2. Initialize I2C GPIOs.
 *      3. Initialize I2C peripheral.
 */
void quasar_audio_init_i2c(void);

/** @brief Deinitialize the CODEC I2C peripheral.
 */
void quasar_audio_deinit_i2c(void);

/** @brief Set audio i2s mux selection.
 *
 *  @param[in] mux_select  Mux selection.
 */
void quasar_audio_set_i2s_mux_selection(quasar_i2s_mux_select_t mux_select);

/** @brief Audio's I2C write single byte blocking function.
 *
 *  @param[in]  dev_addr  Device address.
 *  @param[in]  mem_addr  Target memory address.
 *  @param[in]  data      1 Byte size data to be send.
 *  @param[out] err       Pointer to store error status.
 */
void quasar_audio_i2c_write_byte_blocking(uint8_t dev_addr, uint8_t mem_addr, uint8_t data, quasar_bsp_status_t *err);

/** @brief Audio's I2C read single byte blocking function.
 *
 *  @param[in]  dev_addr  Device address.
 *  @param[in]  mem_addr  Target memory address.
 *  @param[out] data      1 Byte size data to be read.
 *  @param[out] err       Pointer to store error status.
 */
void quasar_audio_i2c_read_byte_blocking(uint8_t dev_addr, uint8_t mem_addr, uint8_t *data, quasar_bsp_status_t *err);

/** @brief Write data on the SAI in non-blocking mode with DMA.
 *
 *  @param[in] data  Data buffer to write.
 *  @param[in] size  Size of the data to write.
 */
void quasar_audio_sai_write_non_blocking(uint8_t *data, uint16_t size);

/** @brief Read data on the SAI in non-blocking mode with DMA.
 *
 *  @param[in] data  Data buffer to write.
 *  @param[in] size  Size of the data to write.
 */
void quasar_audio_sai_read_non_blocking(uint8_t *data, uint16_t size);

/** @brief This function sets the function callback for the audio SAI TX complete.
 *
 *  @param[in] callback  External interrupt callback function pointer.
 */
void quasar_audio_set_sai_tx_dma_cplt_callback(void (*callback)(void));

/** @brief This function sets the function callback for the audio SAI RX complete.
 *
 *  @param[in] callback  External interrupt callback function pointer.
 */
void quasar_audio_set_sai_rx_dma_cplt_callback(void (*callback)(void));

/** @brief Start SAI transmit with DMA peripheral.
 */
void quasar_audio_sai_start_write_non_blocking(void);

/** @brief Start SAI receive with DMA peripheral.
 */
void quasar_audio_sai_start_read_non_blocking(void);

/** @brief Stop SAI writing with DMA peripheral.
 */
void quasar_audio_sai_stop_write_non_blocking(void);

/** @brief Stop SAI reading with DMA peripheral.
 */
void quasar_audio_sai_stop_read_non_blocking(void);

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_AUDIO_H_ */
