/** @file  max98091.h
 *  @brief Driver for the max98091 codec.
 *
 *  @copyright Copyright (C) 2018-2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef MAX98091_H_
#define MAX98091_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include "max98091_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
#define MAX98091_SET_MICROPHONE 0x20

/* Volume control definition */
#define MAX98091_VOLUME_UNMUTE 0
#define MAX98091_VOLUME_MUTE   1

/* Filter definition */
#define FLAT_B0   0x100000
#define FLAT_B1   0x000000
#define FLAT_B2   0x000000
#define FLAT_A1   0x000000
#define FLAT_A2   0x000000

#define LS_172_B0 0x1014AE /* 1.005049 */
#define LS_172_B1 0xE03369 /*-1.987449 */
#define LS_172_B2 0x0FB8EB /* 0.982646 */
#define LS_172_A1 0xE03311 /*-1.987533 */
#define LS_172_A2 0x0FCD41 /* 0.987611 */

#define LP_12k_B0 0x04CF21 /* 0.300569 */
#define LP_12k_B1 0x099E42 /* 0.601137 */
#define LP_12k_B2 0x04CF21 /* 0.300569 */
#define LP_12k_A1 0xFFF2FB /*-0.003179 */
#define LP_12k_A2 0x03498A /* 0.205454 */

#define LP_6k_B0 0x0196E4 /* 0.099339 */
#define LP_6k_B1 0x032DC9 /* 0.198678 */
#define LP_6k_B2 0x0196E4 /* 0.099339 */
#define LP_6k_A1 0xF0832B /*-0.967977 */
#define LP_6k_A2 0x05D867 /* 0.365333 */

#define LP_5k_B0 0x014F48 /* 0.081856 */
#define LP_5k_B1 0x029E90 /* 0.163712 */
#define LP_5k_B2 0x014F48 /* 0.081856 */
#define LP_5k_A1 0xECA6D9 /*-1.209265 */
#define LP_5k_A2 0x089647 /* 0.536689 */

/* f0=1302.2 Q=0.7 -42.9dB@12kHz */
#define LP_1_3k_B0 0x001A79 /* 0.006464 */
#define LP_1_3k_B1 0x0034F5 /* 0.012929 */
#define LP_1_3k_B2 0x001A79 /* 0.006464 */
#define LP_1_3k_A1 0xE3DD94 /*-1.758403 */
#define LP_1_3k_A2 0x0C8C55 /* 0.784261 */

#define LP_1k_B0 0x001080 /* 0.004029 */
#define LP_1k_B1 0x002100 /* 0.008057 */
#define LP_1k_B2 0x001080 /* 0.004029 */
#define LP_1k_A1 0xE238AF /*-1.861161 */
#define LP_1k_A2 0x0E0952 /* 0.877276 */

/* TYPES **********************************************************************/
/** @brief max98091 I2C Addresses.
 *
 *  This enum contains all supported I2C addresses for this codec.
 */
typedef enum max98091_i2c_address {
    MAX98091A_I2C_ADDR = 0x20,
    MAX98091B_I2C_ADDR = 0x22
} max98091_i2c_address_t;

/** @brief max98091 driver API Hardware Abstraction Layer.
 *
 *  This structure contains all function pointers used to interact with the
 *  codec's peripherals.
 */
typedef struct max98091_i2c_hal {
    max98091_i2c_address_t i2c_addr;
    void (*write)(uint8_t dev_address, uint8_t mem_addr, uint8_t data); /* Blocking I2C write function */
    void (*read)(uint8_t dev_address, uint8_t mem_addr, uint8_t *data);  /* Blocking I2C read function */
} max98091_i2c_hal_t;

/** @brief max98091 driver supported sampling rates.
 *
 *  This enum contains all the sampling rates supported by this driver.
 */
typedef enum max98091_sampling_rate {
    MAX98091_AUDIO_96KHZ,
    MAX98091_AUDIO_48KHZ,
    MAX98091_AUDIO_44_1KHZ,
    MAX98091_AUDIO_32KHZ,
    MAX98091_AUDIO_24KHZ,
    MAX98091_AUDIO_16KHZ,
    MAX98091_AUDIO_12KHZ,
    MAX98091_AUDIO_8KHZ
} max98091_sampling_rate_t;

/** @brief max98091 word size enum.
 *
 *  This enum contains all the word size supported by this driver.
 */
typedef enum max98091_word_size {
    MAX98091_AUDIO_16BITS,
    MAX98091_AUDIO_20BITS,
    MAX98091_AUDIO_24BITS,
    MAX98091_AUDIO_32BITS
} max98091_word_size_t;

/** @brief max98091 driver configuration structure.
 *
 *  Variables within this structure can be set by the user to configure the CODEC.
 */
typedef struct max98091_codec_cfg {
    max98091_sampling_rate_t sampling_rate;
    max98091_word_size_t word_size;
    bool record_enabled;
    bool playback_enabled;
    bool record_filter_enabled;
    bool playback_filter_enabled;
} max98091_codec_cfg_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the max98091 audio codec driver.
 *
 *  @param[in]  i2c_hal       Hardware Abstraction Layer structure i2c_hal.
 *  @param[in]  codec_config  Max98091 hardware codec configuration structure.
 */
void max98091_init(max98091_i2c_hal_t *i2c_hal, max98091_codec_cfg_t *codec_cfg);

/** @brief Perfom a software reset.
 *
 *  @param[in]  i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
void max98091_reset_codec(max98091_i2c_hal_t *i2c_hal);

/** @brief Increase headphone volume.
 *
 *  @param[in]  i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
void max98091_hp_increase_volume(max98091_i2c_hal_t *i2c_hal);

/** @brief Decrease headphone volume.
 *
 *  @param[in]  i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
void max98091_hp_decrease_volume(max98091_i2c_hal_t *i2c_hal);

/** @brief Unmute global volume.
 *
 *  @param[in]  i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
void max98091_unmute_volume(max98091_i2c_hal_t *i2c_hal);

/** @brief Mute global volume.
 *
 *  @param[in]  i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
void max98091_mute_volume(max98091_i2c_hal_t *i2c_hal);

/** @brief Set Codec headphone volume level.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 *  @param[in] volume   Headphone volume headset.
 */
void max98091_hp_set_volume(max98091_i2c_hal_t *i2c_hal, max98091_hp_vol_t volume);

/** @brief Returns if a microphone is plugged in.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 *  @return True if mic is plugged in.
 */
bool max98091_is_microphone_present(max98091_i2c_hal_t *i2c_hal);

/** @brief Returns if a audio headset or headphone is plugged in.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 *  @return True if an headset or headphone is plugged in.
 */
bool max98091_is_jack_present(max98091_i2c_hal_t *i2c_hal);

/** @brief Enable microphone mixer.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
void max98091_enable_mic_trrs(max98091_i2c_hal_t *i2c_hal);

/** @brief Disable microphone mixer.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
void max98091_disable_mic_trrs(max98091_i2c_hal_t *i2c_hal);

/** @brief Enable codec output.
 *
 *  Speaker outputs are not enable.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
void max98091_enable_output(max98091_i2c_hal_t *i2c_hal);

/** @brief Disable codec output.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
void max98091_disable_output(max98091_i2c_hal_t *i2c_hal);

/** @brief Reset the codec IRQ by reading the codec status.
 *
 *  @param[in] i2c_hal  Hardware Abstraction Layer structure i2c_hal.
 */
void max98091_reset_codec_irq(max98091_i2c_hal_t *i2c_hal);

#ifdef __cplusplus
}
#endif

#endif /* MAX98091_H_ */
