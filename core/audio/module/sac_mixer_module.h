/** @file  sac_mixer_module.h
 *  @brief SPARK Audio Core Mixer Module is used to mix multiple audio streams into a single one.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_MIXER_MODULE_H_
#define SAC_MIXER_MODULE_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "mem_pool.h"
#include "sac_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/*! The minimum number of input audio streams to be mixed. */
#define MIN_NB_OF_INPUTS            2
/*! The maximum supported number of input audio streams to be mixed. */
#define MAX_NB_OF_INPUTS            3
/*! The minimum number of bytes a payload can contain. */
#define MIN_NB_OF_BYTES_PER_PAYLOAD 2
/*! The maximum number of bytes a payload can contain. */
#define MAX_NB_OF_BYTES_PER_PAYLOAD 122
/*! Give a buffer to have at least 2 packets. */
#define MAX_NB_OF_BYTES_PER_BUFFER  (MAX_NB_OF_BYTES_PER_PAYLOAD * 2)

/* TYPES **********************************************************************/
/** @brief The SPARK Audio Core Mixer Module configurations.
 */
typedef struct sac_mixer_module_cfg {
    /*! The number of inputs to be mixed. */
    uint8_t nb_of_inputs;
    /*! The audio payload size in bytes which must match the output consuming endpoint. */
    uint8_t payload_size;
    /*! Bit depth of each sample in the payload. */
    uint8_t bit_depth;
} sac_mixer_module_cfg_t;

/** @brief The SPARK Audio Core Mixer queue.
 */
typedef struct sac_mixer_queue {
    /*! Can have up to 2x the maximum payload in bytes. */
    uint8_t samples[MAX_NB_OF_BYTES_PER_BUFFER];
    /*! The current size of the queue in bytes. */
    uint8_t current_size;
} sac_mixer_queue_t;

/** @brief The SPARK Audio Core Mixer Module instance.
 */
typedef struct sac_mixer_module {
    /*! SPARK Audio Core Mixer Module configurations. */
    sac_mixer_module_cfg_t cfg;
    /*! Pointer to the input samples to be mixed. */
    sac_mixer_queue_t input_samples_queue[MAX_NB_OF_INPUTS];
    /*! The mixed output packets array. */
    uint8_t output_packet_buffer[MAX_NB_OF_BYTES_PER_PAYLOAD];
} sac_mixer_module_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the SPARK Audio Core Mixer Module.
 *
 *  @param[in]  cfg          Used to configure the SPARK Audio Core Mixer Module.
 *  @param[in]  mem_pool     Memory pool for memory allocation.
 *  @param[out] sac_status   Status code.
 *  @return SPARK Audio Core Mixer Module instance.
 */
sac_mixer_module_t *sac_mixer_module_init(sac_mixer_module_cfg_t cfg, mem_pool_t *mem_pool, sac_status_t *sac_status);

/** @brief The SPARK Audio Core Mixer Module uses an algo to mix samples.
 *
 *  @param[in] sac_mixer_module  SPARK Audio Core Mixer Module instance.
 */
void sac_mixer_module_mix_packets(sac_mixer_module_t *sac_mixer_module);

/** @brief A payload is added to the input queue.
 *
 *  @param[in] input_samples_queue  The samples are stored in this queue.
 *  @param[in] samples              The stored audio samples.
 *  @param[in] size                 The stored audio samples size in bytes.
 */
void sac_mixer_module_append_samples(sac_mixer_queue_t *input_samples_queue, uint8_t *samples, uint8_t size);

/** @brief Silence samples are added to the input queue.
 *
 *  @param[in] input_samples_queue  The samples are stored in this queue.
 *  @param[in] size                 Size in bytes of silent audio samples to add.
 */
void sac_mixer_module_append_silence(sac_mixer_queue_t *input_samples_queue, uint8_t size);

/** @brief Put the remainder in front of the queue.
 *
 *  @param[in] sac_mixer_module  SPARK Audio Core Mixer Module instance.
 */
void sac_mixer_module_handle_remainder(sac_mixer_module_t *sac_mixer_module);

#ifdef __cplusplus
}
#endif

#endif /* SAC_MIXER_MODULE_H_ */
