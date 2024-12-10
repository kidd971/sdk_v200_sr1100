/** @file  evk_audio.h
 *  @brief Initialize audio related peripherals.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef EVK_AUDIO_H_
#define EVK_AUDIO_H_

/* INCLUDES *******************************************************************/
#include "evk_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
#define EVK_SAI_NB_OF_SLOTS 2

/* TYPES **********************************************************************/
/** @brief SAI blocks mono/stereo mode enum.
 */
typedef enum evk_sai_mono_stereo {
    EVK_SAI_MODE_MONO   = 1,
    EVK_SAI_MODE_STEREO = 2
} evk_sai_mono_stereo_t;

/** @brief SAI bit depth enum.
 */
typedef enum evk_sai_bit_depth {
    EVK_SAI_BIT_DEPTH_16BITS     = SAI_PROTOCOL_DATASIZE_16BIT,
    EVK_SAI_BIT_DEPTH_16BITS_EXT = SAI_PROTOCOL_DATASIZE_16BITEXTENDED,
    EVK_SAI_BIT_DEPTH_24BITS     = SAI_PROTOCOL_DATASIZE_24BIT,
    EVK_SAI_BIT_DEPTH_32BITS     = SAI_PROTOCOL_DATASIZE_32BIT,
} evk_sai_bit_depth_t;

/** @brief SAI global configuration structure.
 */
typedef struct evk_sai_config {
    evk_sai_mono_stereo_t rx_sai_mono_stereo;
    evk_sai_mono_stereo_t tx_sai_mono_stereo;
    evk_sai_bit_depth_t sai_bit_depth;
} evk_sai_config_t;

/** @brief IRQ function pointer callback type.
 */
typedef void (*irq_callback)(void);

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Audio's I2C write single byte blocking function.
 *
 *  @param[in] dev_addr  Device address.
 *  @param[in] mem_addr  Target memory address.
 *  @param[in] data      1 Byte size data to be send.
 */
void evk_audio_i2c_write_byte_blocking(uint8_t dev_addr, uint8_t mem_addr, uint8_t data);

/** @brief Audio's I2C read single byte blocking function.
 *
 *  @param[in] dev_addr  Device address.
 *  @param[in] mem_addr  Target memory address.
 *  @param[in] data      1 Byte size data to be read.
 */
void evk_audio_i2c_read_byte_blocking(uint8_t dev_addr, uint8_t mem_addr, uint8_t *data);

/** @brief Configure the Audio's SAI peripheral and initialize it.
 *
 *  This function configures and initializes multiple peripherals sequentially.
 *
 *      1. Enable SAI clocks.
 *      2. Initialize SAI GPIOs.
 *      3. Configure SAI mono stereo mode.
 *      4. Initialize SAI block configuration.
 *      5. Initialize SAI DMA NVIC.
 *      6. Initialize SAI DMA peripherals.
 *      7. Enable I2C clock.
 *      8. Initialize I2C GPIOS.
 *      9. Initialize I2C peripheral.
 *
 *  @param[in] sai_config  SAI global configuration structure.
 */
void evk_audio_sai_configuration(evk_sai_config_t *sai_config);

/** @brief Deinitialize the SAI peripheral.
 */
void evk_audio_sai_deinit(void);

/** @brief Write data on the SAI in non-blocking mode with DMA.
 *
 *  @param[in] data  Data buffer to write.
 *  @param[in] size  Size of the data to write.
 */
void evk_audio_sai_write_non_blocking(uint8_t *data, uint16_t size);

/** @brief Read data on the SAI in non-blocking mode with DMA.
 *
 *  @param[in] data  Data buffer to write.
 *  @param[in] size  Size of the data to write.
 */
void evk_audio_sai_read_non_blocking(uint8_t *data, uint16_t size);

/** @brief This function sets the function callback for the audio SAI TX complete.
 *
 *  @param[in] callback  External interrupt callback function pointer.
 */
void evk_audio_set_sai_tx_dma_cplt_callback(irq_callback callback);

/** @brief This function sets the function callback for the audio SAI RX complete.
 *
 *  @param[in] callback  External interrupt callback function pointer.
 */
void evk_audio_set_sai_rx_dma_cplt_callback(irq_callback callback);

/** @brief Start SAI transmit with DMA peripheral.
 */
void evk_audio_sai_start_write_non_blocking(void);

/** @brief Start SAI receive with DMA peripheral.
 */
void evk_audio_sai_start_read_non_blocking(void);

/** @brief Stop SAI writing with DMA peripheral.
 */
void evk_audio_sai_stop_write_non_blocking(void);

/** @brief Stop SAI reading with DMA peripheral.
 */
void evk_audio_sai_stop_read_non_blocking(void);

/** @brief Initialize the I2C peripheral.
 */
void evk_audio_i2c_init(void);

#ifdef __cplusplus
}
#endif

#endif /* EVK_AUDIO_H_ */

