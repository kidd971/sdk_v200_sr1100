/** @file  sac_compression.h
 *  @brief SPARK Audio Core ADPCM compression / decompression processing stage.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_COMPRESSION_H_
#define SAC_COMPRESSION_H_

/* INCLUDES *******************************************************************/
#include "adpcm.h"
#include "sac_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* MACROS *********************************************************************/
/* Calculate the size of the ADPCM compression header needed for audio samples. */
#define SAC_COMPRESSION_HEADER_SIZE(nb_ch) ((nb_ch) * sizeof(adpcm_state_t))

/* TYPES **********************************************************************/
/** @brief SPARK Audio Core Compression Commands.
 */
typedef enum sac_compression_cmd {
    /*! Get the SPARK Audio Core compression state. */
    SAC_COMPRESSION_GET_STATE
} sac_compression_cmd_t;

/** @brief SPARK Audio Core Compression Mode.
 */
typedef enum sac_compression_mode {
    /*! Pack stereo uncompressed 16-bit stream to stereo 16-bit compressed stream mode. */
    SAC_COMPRESSION_PACK_STEREO,
    /*! Unpack stereo compressed 16-bit stream to stereo 16-bit uncompressed stream mode. */
    SAC_COMPRESSION_UNPACK_STEREO,
    /*! Pack mono uncompressed 16-bit stream to mono 16-bit compressed stream mode. */
    SAC_COMPRESSION_PACK_MONO,
    /*! Unpack mono compressed 16-bit stream to mono 16-bit uncompressed stream mode. */
    SAC_COMPRESSION_UNPACK_MONO,
} sac_compression_mode_t;

/** @brief SPARK Audio Core Compression Instance.
 */
typedef struct sac_compression_instance {
    /*! SPARK Audio Core Compression mode. */
    sac_compression_mode_t compression_mode;
    /*! Format of the uncompressed audio samples. */
    sac_sample_format_t sample_format;
    struct {
        /*! Internal: Left ADPCM encoder state. */
        adpcm_state_t adpcm_left_state;
        /*! Internal: Right ADPCM encoder state. */
        adpcm_state_t adpcm_right_state;
        /*! Internal: Sample size of an uncompressed sample in bits. */
        uint8_t sample_size_bit;
        /*! Internal: Sample size of an uncompressed sample in bytes. */
        uint8_t sample_size_byte;
        /*! Internal: Number of sample samples to be added to the history. */
        uint8_t discard_size;
        /*! Internal: Bit shift to downsize samples to 16-bits bit depth. */
        uint8_t bit_shift_16bits;
        /*! Internal: Position of the sample MSB. */
        uint8_t msb_position;
        /*! Internal: Number of bits needed to be extended. */
        uint32_t extend_size;
    } _internal;
} sac_compression_instance_t;

/** @brief SPARK Audio Core Compression Stereo Header.
 */
typedef struct sac_compression_adpcm_stereo_header {
    /*! SPARK Audio Core compression left channel state. */
    adpcm_state_t adpcm_header_left_state;
    /*! SPARK Audio Core compression right channel state. */
    adpcm_state_t adpcm_header_right_state;
} sac_compression_adpcm_stereo_header_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize compression process.
 *
 *  @param[in]  instance  Process instance.
 *  @param[in]  name      Processing stage name.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  mem_pool  Memory pool for memory allocation.
 *  @param[out] status    Status code.
 */
void sac_compression_init(void *instance, const char *name, sac_pipeline_t *pipeline, mem_pool_t *mem_pool,
                          sac_status_t *status);

/** @brief SPARK Audio Core compression control function.
 *
 *  @param[in]  instance  Compression instance.
 *  @param[in]  cmd       Control command.
 *  @param[in]  arg       Control argument.
 *  @param[out] status    Status code.
 *  @return SPARK Audio Core Compression Status.
 */
uint32_t sac_compression_ctrl(void *instance, sac_pipeline_t *pipeline, uint8_t cmd, uint32_t arg,
                              sac_status_t *status);

/** @brief Process audio samples compression.
 *
 *  @param[in]  instance     Compression instance.
 *  @param[in]  header       SPARK Audio Core header.
 *  @param[in]  data_in      Data in to be processed.
 *  @param[in]  bytes_count  Number of bytes to process.
 *  @param[out] data_out     Processed data out.
 *  @param[out] status       Status code.
 *  @return Number of bytes processed.
 */
uint16_t sac_compression_process(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in,
                                 uint16_t size, uint8_t *data_out, sac_status_t *status);

/** @brief Process the last audio samples to maintain the sample history. Thus, any subsequent switch to
 *         sac_compression_process will provide a clean audio output. Output is discarded and
 *         the function returns 0. Only valid for packing.
 *
 *  @param[in]  instance     Compression instance.
 *  @param[in]  header       SPARK Audio Core header.
 *  @param[in]  data_in      Data in to be processed.
 *  @param[in]  bytes_count  Number of bytes to process.
 *  @param[out] data_out     Processed data out.
 *  @param[out] status       Status code.
 *  @return 0.
 */
uint16_t sac_compression_process_discard(void *instance, sac_pipeline_t *pipeline, sac_header_t *header,
                                         uint8_t *data_in, uint16_t size, uint8_t *data_out, sac_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* SAC_COMPRESSION_H_ */
