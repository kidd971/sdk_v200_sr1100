/** @file  quasar_audio.h
 *  @brief Initialize audio related peripherals.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_AUDIO_H_
#define QUASAR_AUDIO_H_

/* INCLUDES *******************************************************************/
#include "quasar_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
#define QUASAR_SAI_NB_OF_SLOTS 2

/** @brief SAI blocks mono/stereo mode enum.
 */
typedef enum quasar_sai_mono_stereo {
    QUASAR_SAI_MODE_MONO = 1,
    QUASAR_SAI_MODE_STEREO = 2
} quasar_sai_mono_stereo_t;

/** @brief SAI bit depth enum.
 */
typedef enum quasar_sai_bit_depth {
    QUASAR_SAI_BIT_DEPTH_16BITS     = SAI_PROTOCOL_DATASIZE_16BIT,
    QUASAR_SAI_BIT_DEPTH_16BITS_EXT = SAI_PROTOCOL_DATASIZE_16BITEXTENDED,
    QUASAR_SAI_BIT_DEPTH_24BITS     = SAI_PROTOCOL_DATASIZE_24BIT,
    QUASAR_SAI_BIT_DEPTH_32BITS     = SAI_PROTOCOL_DATASIZE_32BIT,
} quasar_sai_bit_depth_t;

/** @brief SAI global configuration structure.
 */
typedef struct quasar_sai_config {
    quasar_sai_mono_stereo_t rx_sai_mono_stereo;
    quasar_sai_mono_stereo_t tx_sai_mono_stereo;
    quasar_sai_bit_depth_t sai_bit_depth;
} quasar_sai_config_t;

/** @brief IRQ function pointer callback type.
 */
typedef void (*irq_callback)(void);

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
 */
void quasar_audio_init(quasar_sai_config_t sai_config);

/** @brief Deinitialize the Audio's peripheral.
 */
void quasar_audio_deinit(void);

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

/** @brief Audio's I2C write single byte blocking function.
 *
 *  @param[in] dev_addr  Device address.
 *  @param[in] mem_addr  Target memory address.
 *  @param[in] data      1 Byte size data to be send.
 */
void quasar_audio_i2c_write_byte_blocking(uint8_t dev_addr, uint8_t mem_addr, uint8_t data);

/** @brief Audio's I2C read single byte blocking function.
 *
 *  @param[in] dev_addr  Device address.
 *  @param[in] mem_addr  Target memory address.
 *  @param[in] data      1 Byte size data to be read.
 */
void quasar_audio_i2c_read_byte_blocking(uint8_t dev_addr, uint8_t mem_addr, uint8_t *data);

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
void quasar_audio_set_sai_tx_dma_cplt_callback(irq_callback callback);

/** @brief This function sets the function callback for the audio SAI RX complete.
 *
 *  @param[in] callback  External interrupt callback function pointer.
 */
void quasar_audio_set_sai_rx_dma_cplt_callback(irq_callback callback);

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
