/** @file  sac_compression.c
 *  @brief SPARK Audio Core ADPCM compression / decompression processing stage.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include <string.h>
#include "sac_compression.h"

/* MACROS *********************************************************************/
#define BYTE_TO_BITS(byte) ((byte) * SAC_BYTE_SIZE_BITS)
#define BITS_TO_BYTE(bits) ((bits) / SAC_BYTE_SIZE_BITS)

/* PRIVATE FUNCTION PROTOTYPES *************************************************/
static void extend_msb_to_32bits(void *instance, uint32_t *value);
static uint16_t pack_stereo(void *instance, uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t unpack_stereo(void *instance, uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t pack_mono(void *instance, uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t unpack_mono(void *instance, uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static void validate_sac_bit_depth(sac_bit_depth_t bit_depth, sac_status_t *status);
static void instance_status_check(void *instance, sac_status_t *status);

/* PUBLIC FUNCTIONS ***********************************************************/
void sac_compression_init(void *instance, const char *name, sac_pipeline_t *pipeline, mem_pool_t *mem_pool,
                          sac_status_t *status)
{
    (void)pipeline;
    (void)mem_pool;
    (void)name;

    sac_compression_instance_t *compress_inst = instance;

    *status = SAC_OK;

    instance_status_check(instance, status);
    if (*status != SAC_OK) {
        return;
    }

    adpcm_init_state(&(compress_inst->_internal.adpcm_left_state));
    adpcm_init_state(&(compress_inst->_internal.adpcm_right_state));
    compress_inst->_internal.bit_shift_16bits = compress_inst->sample_format.bit_depth - SAC_16BITS;
    if (compress_inst->sample_format.sample_encoding == SAC_SAMPLE_UNPACKED) {
        compress_inst->_internal.sample_size_bit = SAC_WORD_SIZE_BITS;
    } else {
        compress_inst->_internal.sample_size_bit = compress_inst->sample_format.bit_depth;
    }
    compress_inst->_internal.sample_size_byte = compress_inst->_internal.sample_size_bit / SAC_BYTE_SIZE_BITS;

    switch (compress_inst->compression_mode) {
    case SAC_COMPRESSION_PACK_STEREO:
        compress_inst->_internal.discard_size = BITS_TO_BYTE(compress_inst->_internal.sample_size_bit * 2);
        break;
    case SAC_COMPRESSION_PACK_MONO:
        compress_inst->_internal.discard_size = BITS_TO_BYTE(compress_inst->_internal.sample_size_bit);
        break;
    case SAC_COMPRESSION_UNPACK_STEREO:
    case SAC_COMPRESSION_UNPACK_MONO:
        compress_inst->_internal.msb_position = compress_inst->sample_format.bit_depth - 1;
        compress_inst->_internal.extend_size  = compress_inst->_internal.sample_size_bit - compress_inst->sample_format.bit_depth;
        break;
    default:
        *status = SAC_ERR_PROCESSING_STAGE_INIT;
        return;
    }
}

uint32_t sac_compression_ctrl(void *instance, sac_pipeline_t *pipeline, uint8_t cmd, uint32_t arg, sac_status_t *status)
{
    (void)instance;
    (void)pipeline;
    (void)cmd;
    (void)arg;

    *status = SAC_OK;

    return 0;
}

uint16_t sac_compression_process(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in,
                                 uint16_t size, uint8_t *data_out, sac_status_t *status)
{
    (void)pipeline;
    (void)header;

    sac_compression_instance_t *compress_inst = instance;
    uint16_t output_size = 0;

    *status = SAC_OK;

    switch (compress_inst->compression_mode) {
    case SAC_COMPRESSION_PACK_STEREO:
        output_size = pack_stereo(instance, data_in, size, data_out);
        break;
    case SAC_COMPRESSION_UNPACK_STEREO:
        output_size = unpack_stereo(instance, data_in, size, data_out);
        break;
    case SAC_COMPRESSION_PACK_MONO:
        output_size = pack_mono(instance, data_in, size, data_out);
        break;
    case SAC_COMPRESSION_UNPACK_MONO:
        output_size = unpack_mono(instance, data_in, size, data_out);
        break;
    }
    return output_size;
}

uint16_t sac_compression_process_discard(void *instance, sac_pipeline_t *pipeline, sac_header_t *header,
                                         uint8_t *data_in, uint16_t size, uint8_t *data_out, sac_status_t *status)
{
    (void)pipeline;
    (void)header;
    (void)data_out;

    sac_compression_instance_t *compress_inst = instance;

    *status = SAC_OK;

    switch (compress_inst->compression_mode) {
    case SAC_COMPRESSION_PACK_STEREO:
        pack_stereo(instance, data_in, size, data_out);
        break;
    case SAC_COMPRESSION_PACK_MONO:
        pack_mono(instance, data_in, size, data_out);
        break;
    case SAC_COMPRESSION_UNPACK_STEREO:
    case SAC_COMPRESSION_UNPACK_MONO:
        break;
    }
    return 0;
}

/* PRIVATE FUNCTIONS ***********************************************************/
/** @brief Pack stereo uncompressed stream to stereo compressed stream.
 *
 *  @param[in]  instance        Compression instance.
 *  @param[in]  buffer_in       Array of the uncompressed stereo data.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the compressed stereo stream is written to.
 *  @return written size, in byte, to the output buffer.
 */
static uint16_t pack_stereo(void *instance, uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint8_t *input_buffer = (uint8_t *)buffer_in;
    uint16_t pcm_sample_count;
    uint8_t left_code;
    uint8_t right_code;
    int16_t sample;
    sac_compression_instance_t *compress_inst = instance;

    pcm_sample_count = (BYTE_TO_BITS(buffer_in_size) / compress_inst->_internal.sample_size_bit) / 2;

    /* Set left ADPCM encoder status. */
    memcpy(buffer_out, &(compress_inst->_internal.adpcm_left_state), sizeof(adpcm_state_t));

    /* Set right ADPCM encoder status. */
    memcpy(&buffer_out[sizeof(adpcm_state_t)], &(compress_inst->_internal.adpcm_right_state), sizeof(adpcm_state_t));

    buffer_out += sizeof(sac_compression_adpcm_stereo_header_t);

    /*
     * Since two samples get compressed into a single byte,
     * the loop will work with two samples at the same time (left and right samples).
     */
    for (uint8_t i = 0; i < pcm_sample_count; i++) {
        /* Left Channel. */
        sample = (*((int32_t *)input_buffer) >> compress_inst->_internal.bit_shift_16bits) & 0xFFFF;
        left_code  = adpcm_encode(sample, &(compress_inst->_internal.adpcm_left_state));
        input_buffer += compress_inst->_internal.sample_size_byte;
        /* Right Channel. */
        sample = (*((int32_t *)input_buffer) >> compress_inst->_internal.bit_shift_16bits) & 0xFFFF;
        right_code = adpcm_encode(sample, &(compress_inst->_internal.adpcm_right_state));
        input_buffer += compress_inst->_internal.sample_size_byte;
        /* Concatenate two ADPCM samples code per byte in the output buffer (4-bit MSB, 4-bit LSB). */
        *buffer_out++ = (left_code & 0x0F) | ((right_code << 4) & 0xF0);
    }

    /* Each stereo sample takes 1 byte. */
    return pcm_sample_count + sizeof(sac_compression_adpcm_stereo_header_t);
}

/** @brief Unpack stereo compressed stream to stereo uncompressed stream.
 *
 *  @param[in]  instance        Compression instance.
 *  @param[in]  buffer_in       Array of the input stereo compressed data.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the uncompressed stereo stream is written to.
 *  @return written size, in byte, to the output buffer.
 */
static uint16_t unpack_stereo(void *instance, uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint8_t *output_buffer = (uint8_t *)buffer_out;
    uint16_t pcm_sample_count;
    sac_compression_instance_t *compress_inst = instance;

    /* Get left ADPCM status. */
    memcpy(&(compress_inst->_internal.adpcm_left_state), buffer_in, sizeof(adpcm_state_t));

    /* Get right ADPCM encoder status. */
    memcpy(&(compress_inst->_internal.adpcm_right_state), &buffer_in[sizeof(adpcm_state_t)], sizeof(adpcm_state_t));

    buffer_in += sizeof(sac_compression_adpcm_stereo_header_t);
    pcm_sample_count = (buffer_in_size - sizeof(sac_compression_adpcm_stereo_header_t)) * 2;

    /*
     * Since two samples are compressed into a single byte, the loop will work with two compressed
     * samples at the same time (left and right samples).
     */
    for (uint8_t i = 0; i < pcm_sample_count / 2; i++) {
        *(int32_t *)output_buffer = (adpcm_decode(*buffer_in & 0x0F, &(compress_inst->_internal.adpcm_left_state)) <<
                                    compress_inst->_internal.bit_shift_16bits);
        if (compress_inst->_internal.extend_size > 0) {
            extend_msb_to_32bits(instance, (uint32_t *)output_buffer);
        }
        output_buffer += compress_inst->_internal.sample_size_byte;
        *(int32_t *)output_buffer = (adpcm_decode((*buffer_in++ >> 4) & 0x0F, &(compress_inst->_internal.adpcm_right_state)) <<
                                    compress_inst->_internal.bit_shift_16bits);
        if (compress_inst->_internal.extend_size > 0) {
            extend_msb_to_32bits(instance, (uint32_t *)output_buffer);
        }
        output_buffer += compress_inst->_internal.sample_size_byte;
    }
    return compress_inst->_internal.sample_size_byte * pcm_sample_count;
}

/** @brief Pack mono uncompressed stream to mono compressed stream.
 *
 *  @param[in]  instance        Compression instance.
 *  @param[in]  buffer_in       Array of the uncompressed mono data.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the compressed stereo stream is written to.
 *  @return written size, in byte, to the output buffer.
 */
static uint16_t pack_mono(void *instance, uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint8_t *input_buffer = (uint8_t *)buffer_in;
    uint16_t pcm_sample_count;
    uint16_t compressed_byte_count;
    int16_t sample;
    sac_compression_instance_t *compress_inst = instance;

    pcm_sample_count = (BYTE_TO_BITS(buffer_in_size) / compress_inst->_internal.sample_size_bit);
    compressed_byte_count = (pcm_sample_count / 2);

    /* Set left ADPCM encoder status. */
    memcpy(buffer_out, &(compress_inst->_internal.adpcm_left_state), sizeof(adpcm_state_t));
    buffer_out += sizeof(adpcm_state_t);

    /* Since two samples get compressed into a single byte, the loop will work with two samples at the same time. */
    for (uint8_t i = 0; i < compressed_byte_count; i++) {
        /* Concatenate two ADPCM samples code per byte in the output buffer (4-bit MSB, 4-bit LSB). */
        sample = (*((int32_t *)input_buffer) >> compress_inst->_internal.bit_shift_16bits) & 0xFFFF;
        *buffer_out = adpcm_encode(sample, &(compress_inst->_internal.adpcm_left_state)) & 0x0F;
        input_buffer += compress_inst->_internal.sample_size_byte;
        sample = (*((int32_t *)input_buffer) >> compress_inst->_internal.bit_shift_16bits) & 0xFFFF;
        *buffer_out++ |= (adpcm_encode(sample, &(compress_inst->_internal.adpcm_left_state)) << 4) & 0xF0;
        input_buffer += compress_inst->_internal.sample_size_byte;
    }
    /* Manage odd number of samples */
    if (pcm_sample_count & 0x01) {
        sample = (*((int32_t *)input_buffer) >> compress_inst->_internal.bit_shift_16bits) & 0xFFFF;
        *buffer_out  = adpcm_encode(sample, &(compress_inst->_internal.adpcm_left_state)) & 0x0F;
    }

    return (compressed_byte_count + (pcm_sample_count & 0x01)) * sizeof(uint8_t) + sizeof(state_variable_t);
}

/** @brief Unpack mono compressed stream to mono uncompressed stream.
 *
 *  @param[in]  instance        Compression instance.
 *  @param[in]  buffer_in       Array of the input mono compressed data.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the uncompressed stereo stream is written to.
 *  @return written size, in byte, to the output buffer.
 */
static uint16_t unpack_mono(void *instance, uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint8_t *output_buffer = (uint8_t *)buffer_out;
    uint16_t pcm_sample_count;
    sac_compression_instance_t *compress_inst = instance;

    /* Get left ADPCM status. */
    memcpy(&(compress_inst->_internal.adpcm_left_state), buffer_in, sizeof(adpcm_state_t));
    buffer_in += sizeof(adpcm_state_t);

    /* Get number of mono samples. */
    pcm_sample_count = (buffer_in_size - sizeof(adpcm_state_t)) * 2;

    /*
     * Since two samples are compressed into a single byte, the loop will work with two compressed
     * samples at the same time.
     */
    for (uint8_t i = 0; i < pcm_sample_count / 2; i++) {
        *(int32_t *)output_buffer = (adpcm_decode((*buffer_in & 0x0F), &(compress_inst->_internal.adpcm_left_state)) <<
                                    compress_inst->_internal.bit_shift_16bits);
        if (compress_inst->_internal.extend_size > 0) {
            extend_msb_to_32bits(instance, (uint32_t *)output_buffer);
        }
        output_buffer += compress_inst->_internal.sample_size_byte;
        *(int32_t *)output_buffer = (adpcm_decode((*buffer_in++ >> 4) & 0x0F, &(compress_inst->_internal.adpcm_left_state)) <<
                                    compress_inst->_internal.bit_shift_16bits);
        if (compress_inst->_internal.extend_size > 0) {
            extend_msb_to_32bits(instance, (uint32_t *)output_buffer);
        }
        output_buffer += compress_inst->_internal.sample_size_byte;
    }
    return pcm_sample_count * compress_inst->_internal.sample_size_byte;
}

/** @brief Extend value's sign bit into 32-bit word.
 *
 *  @param[in] value  The input value to be extended.
 */
static void extend_msb_to_32bits(void *instance, uint32_t *value)
{
    sac_compression_instance_t *compress_inst = instance;
    uint32_t msb_value = ((*value) & (1 << compress_inst->_internal.msb_position));
    uint8_t i;

    for (i = 0; i < compress_inst->_internal.extend_size; i++) {
        if (msb_value) {
            *value |= (1 << (compress_inst->sample_format.bit_depth + i));
        } else {
            *value &= ~(1 << (compress_inst->sample_format.bit_depth + i));
        }
    }
}

/** @brief Validate if bit depth value is supported by the SAC.
 *
 *  @param[in]  bit_depth  Bit depth to validate.
 *  @param[out] status     Status code.
 */
static void validate_sac_bit_depth(sac_bit_depth_t bit_depth, sac_status_t *status)
{
    if ((bit_depth != SAC_16BITS) &&
        (bit_depth != SAC_18BITS) &&
        (bit_depth != SAC_20BITS) &&
        (bit_depth != SAC_24BITS) &&
        (bit_depth != SAC_32BITS)) {
        *status = SAC_ERR_BIT_DEPTH;
    }
}

/** @brief Check audio core status.
 *
 *  @param[in]  instance  Compression instance.
 *  @param[out] status    Status code.
 */
static void instance_status_check(void *instance, sac_status_t *status)
{
    sac_compression_instance_t *compress_inst = instance;

    if (compress_inst == NULL) {
        *status = SAC_ERR_NULL_PTR;
        return;
    }

    validate_sac_bit_depth(compress_inst->sample_format.bit_depth, status);
    if (*status != SAC_OK) {
        return;
    }

    if ((compress_inst->sample_format.sample_encoding != SAC_SAMPLE_UNPACKED) &&
        (compress_inst->sample_format.sample_encoding != SAC_SAMPLE_PACKED)) {
        *status = SAC_ERR_PROCESSING_STAGE_INIT;
        return;
    }

    if ((compress_inst->sample_format.sample_encoding == SAC_SAMPLE_PACKED) &&
        ((compress_inst->sample_format.bit_depth % SAC_BYTE_SIZE_BITS) != 0)) {
        /* SAC compression does not support packed samples not aligned to bytes. */
        *status = SAC_ERR_PROCESSING_STAGE_INIT;
        return;
    }
}
