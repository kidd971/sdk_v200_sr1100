/** @file  sac_packing.c
 *  @brief SPARK Audio Core packing/unpacking for 18/20/24 bits audio processing stage.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include <string.h>
#include "sac_packing.h"

/* MACROS *********************************************************************/
#define SAMPLE_SIZE_18BITS (2.25f)
#define PACKED_SIZE_18BITS ((uint8_t)(SAMPLE_SIZE_18BITS * 4.0f))
#define SAMPLE_SIZE_20BITS (2.5f)
#define PACKED_SIZE_20BITS ((uint8_t)(SAMPLE_SIZE_20BITS * 2.0f))
#define SAMPLE_SIZE_16BITS 2
#define SAMPLE_SIZE_24BITS 3
#define SAMPLE_SIZE_32BITS 4

#define CODEC_WORD_SIZE_OFFSET_18BITS 2

/* PRIVATE FUNCTION PROTOTYPES *************************************************/
static void extend_msb_18bits_value(uint32_t *value);
static void extend_msb_20bits_value(uint32_t *value);
static void extend_msb_24bits_value(uint32_t *value);
static uint16_t pack_18bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t pack_20bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t pack_24bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t pack_32bits_24bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t pack_20bits_16bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t pack_24bits_16bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t scale_24bits_16bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t unpack_20bits_16bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t unpack_24bits_16bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t unpack_18bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t unpack_20bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t unpack_24bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t extend_18bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t extend_20bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);
static uint16_t extend_24bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out);

/* PUBLIC FUNCTIONS ***********************************************************/
void sac_packing_init(void *instance, const char *name, sac_pipeline_t *pipeline, mem_pool_t *mem_pool,
                      sac_status_t *status)
{
    (void)pipeline;
    (void)mem_pool;
    (void)name;

    sac_packing_instance_t *packing_inst = instance;

    *status = SAC_OK;

    if (packing_inst == NULL) {
        *status = SAC_ERR_NULL_PTR;
        return;
    }

    switch (packing_inst->packing_mode) {
    case SAC_PACK_18BITS:
    case SAC_PACK_20BITS:
    case SAC_PACK_24BITS:
    case SAC_PACK_32BITS_24BITS:
    case SAC_PACK_20BITS_16BITS:
    case SAC_PACK_24BITS_16BITS:
    case SAC_SCALE_24BITS_16BITS:
    case SAC_UNPACK_18BITS:
    case SAC_UNPACK_20BITS:
    case SAC_UNPACK_24BITS:
    case SAC_UNPACK_20BITS_16BITS:
    case SAC_UNPACK_24BITS_16BITS:
    case SAC_EXTEND_18BITS:
    case SAC_EXTEND_20BITS:
    case SAC_EXTEND_24BITS:
        break;
    default:
        *status = SAC_ERR_PROCESSING_STAGE_INIT;
        return;
    }
}

uint32_t sac_packing_ctrl(void *instance, sac_pipeline_t *pipeline, uint8_t cmd, uint32_t arg, sac_status_t *status)
{
    (void)pipeline;
    (void)arg;

    uint32_t ret = 0;
    sac_packing_instance_t *packing_inst = instance;

    *status = SAC_OK;

    switch ((sac_packing_cmd_t)cmd) {
    case SAC_PACKING_SET_MODE:
        packing_inst->packing_mode = arg;
        break;
    case SAC_PACKING_GET_MODE:
        ret = packing_inst->packing_mode;
        break;
    default:
        *status = SAC_ERR_INVALID_CMD;
    }

    return ret;
}

uint16_t sac_packing_process(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in,
                             uint16_t size, uint8_t *data_out, sac_status_t *status)
{
    (void)pipeline;
    (void)header;

    sac_packing_instance_t *packing_inst = instance;
    uint16_t output_size = 0;

    *status = SAC_OK;

    switch (packing_inst->packing_mode) {
    case SAC_PACK_18BITS:
        output_size = pack_18bits(data_in, size, data_out);
        break;
    case SAC_PACK_20BITS:
        output_size = pack_20bits(data_in, size, data_out);
        break;
    case SAC_PACK_24BITS:
        output_size = pack_24bits(data_in, size, data_out);
        break;
    case SAC_UNPACK_18BITS:
        output_size = unpack_18bits(data_in, size, data_out);
        break;
    case SAC_UNPACK_20BITS:
        output_size = unpack_20bits(data_in, size, data_out);
        break;
    case SAC_UNPACK_24BITS:
        output_size = unpack_24bits(data_in, size, data_out);
        break;
    case SAC_EXTEND_18BITS:
        output_size = extend_18bits(data_in, size, data_out);
        break;
    case SAC_EXTEND_20BITS:
        output_size = extend_20bits(data_in, size, data_out);
        break;
    case SAC_EXTEND_24BITS:
        output_size = extend_24bits(data_in, size, data_out);
        break;
    case SAC_PACK_32BITS_24BITS:
        output_size = pack_32bits_24bits(data_in, size, data_out);
        break;
    case SAC_PACK_20BITS_16BITS:
        output_size = pack_20bits_16bits(data_in, size, data_out);
        break;
    case SAC_PACK_24BITS_16BITS:
        output_size = pack_24bits_16bits(data_in, size, data_out);
        break;
    case SAC_SCALE_24BITS_16BITS:
        output_size = scale_24bits_16bits(data_in, size, data_out);
        break;
    case SAC_UNPACK_20BITS_16BITS:
        output_size = unpack_20bits_16bits(data_in, size, data_out);
        break;
    case SAC_UNPACK_24BITS_16BITS:
        output_size = unpack_24bits_16bits(data_in, size, data_out);
        break;
    }

    return output_size;
}

/* PRIVATE FUNCTIONS ***********************************************************/
/** @brief Pack 32-bit audio samples into 18-bit audio samples.
 *
 *  @param[in]  buffer_in       Array of the input 32-bit samples containing 20-bit audio.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the packed 18-bit stream is written to.
 *  @return Written size, in byte, to the output buffer.
 */
static uint16_t pack_18bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint32_t *data32_in = (uint32_t *)buffer_in;
    uint8_t *data_out = buffer_out;
    uint16_t sample_count = buffer_in_size / SAMPLE_SIZE_32BITS;
    uint16_t i;
    uint16_t ret = 0;

    for (i = 0; i < sample_count; i += 4) {
        *(uint32_t *)(&(data_out[0])) = 0;
        /* Check if sample #4 exists. */
        if ((i + 3) < sample_count) {
            /* Get 18 MSB's from (18 + CODEC_WORD_SIZE_OFFSET_18BITS) bit sample, shift sample by 6 bits and copy sample #4. */
            *(uint32_t *)(&(data_out[6])) &= (~0x3FFFFU << 6);
            *(uint32_t *)(&(data_out[6])) = (((data32_in[3] >> CODEC_WORD_SIZE_OFFSET_18BITS) & 0x3FFFF) << 6);
            /* Increment return size by (floor(2.25)). */
            ret += 2;
        }

        /* Check if sample #3 exists. */
        if ((i + 2) < sample_count) {
            /* Get 18 MSB's from (18 + CODEC_WORD_SIZE_OFFSET_18BITS) bit sample, shift sample by 4 bits and copy sample #3. */
            *(uint32_t *)(&(data_out[4])) &= (~0x3FFFFU << 4);
            *(uint32_t *)(&(data_out[4])) |= (((data32_in[2] >> CODEC_WORD_SIZE_OFFSET_18BITS) & 0x3FFFF) << 4);
            /* Increment return size by (floor(2.25)). */
            ret += 2;
        }

        /* Check if sample #2 exists. */
        if ((i + 1) < sample_count) {
            /* Get 18 MSB's from (18 + CODEC_WORD_SIZE_OFFSET_18BITS) bit sample, shift sample by 2 bits and copy sample #2. */
            *(uint32_t *)(&(data_out[2])) &= (~0x3FFFFU << 2);
            *(uint32_t *)(&(data_out[2])) |= (((data32_in[1] >> CODEC_WORD_SIZE_OFFSET_18BITS) & 0x3FFFF) << 2);
            /* Increment return size by (floor(2.25)). */
            ret += 2;
        }

        /* Get 18 MSB's from (18 + CODEC_WORD_SIZE_OFFSET_18BITS) bit sample, copy sample #1 without erasing sample #2. */
        *(uint32_t *)(&(data_out[0])) &= ~0x3FFFF;
        *(uint32_t *)(&(data_out[0])) |= ((data32_in[0] >> CODEC_WORD_SIZE_OFFSET_18BITS) & 0x3FFFF);
        /* Increment return size by (ceil(2.25)). */
        ret += 3;

        /* Increment pointers. */
        data_out += PACKED_SIZE_18BITS;
        data32_in += 4;
    }

    return ret;
}

/** @brief Pack 32-bit audio samples into 20-bit audio samples.
 *
 *  @param[in]  buffer_in       Array of the input 32-bit samples containing 20-bit audio.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the packed 20-bit stream is written to.
 *  @return Written size, in byte, to the output buffer.
 */
static uint16_t pack_20bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint32_t *data32_in = (uint32_t *)buffer_in;
    uint8_t *data_out = buffer_out;
    uint16_t sample_count = buffer_in_size / SAMPLE_SIZE_32BITS;
    uint16_t i;
    uint16_t ret = 0;

    for (i = 0; i < sample_count; i += 2) {
        *(uint32_t *)(&(data_out[0])) = 0;
        /* Check if sample #2 exists. */
        if ((i + 1) < sample_count) {
            /* Copy Sample #2 */
            *(uint32_t *)(&(data_out[2])) = ((data32_in[1]) & 0xFFFFF);
            /* Shift Sample #2 by 4 bits. */
            *(uint32_t *)(&(data_out[2])) <<= 4;
            /* Increment return size by (floor(2.5)). */
            ret += 2;
        }
        /* Copy Sample #1 without erasing Sample #2. */
        *(uint32_t *)(&(data_out[0])) |=  ((data32_in[0]) & 0xFFFFF);
        /* Increment return size by (ceil(2.5)). */
        ret += 3;

        /* Increment pointers */
        data_out += PACKED_SIZE_20BITS;
        data32_in += 2;
    }

    return ret;
}

/** @brief Pack 32-bit audio samples into 24-bit audio samples.
 *
 *  @param[in]  buffer_in       Array of the input 32-bit samples containing 24-bit audio.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the packed 24-bit stream is written to.
 *  @return Written size, in byte, to the output buffer.
 */
static uint16_t pack_24bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint32_t *data32_in = (uint32_t *)buffer_in;
    uint8_t *data_out = buffer_out;
    uint16_t sample_count = buffer_in_size / SAMPLE_SIZE_32BITS;
    uint16_t i;
    uint16_t ret = 0;

    for (i = 0; i < sample_count; i++) {
        /* Copy Sample. */
        *(uint32_t *)(&(data_out[0])) =  ((*data32_in) & 0xFFFFFF);
        /* Increment return size. */
        ret += SAMPLE_SIZE_24BITS;

        /* Increment pointers. */
        data_out += SAMPLE_SIZE_24BITS;
        data32_in++;
    }

    return ret;
}

/** @brief Pack 32-bit words containing 32-bit audio samples into 24-bit audio samples.
 *
 *  @param[in]  buffer_in       Array of the input 32-bit samples containing 32-bit audio.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the packed 24-bit stream is written to.
 *  @return Written size, in byte, to the output buffer.
 */
static uint16_t pack_32bits_24bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint32_t *data32_in = (uint32_t *)buffer_in;
    uint8_t *data_out = buffer_out;
    uint16_t sample_count = buffer_in_size / SAMPLE_SIZE_32BITS;
    uint16_t i;
    uint16_t ret = 0;

    for (i = 0; i < sample_count; i++) {
        /* Copy Sample. */
        *(uint32_t *)(&(data_out[0])) =  (((*data32_in) >> 8) & 0xFFFFFF);
        /* Increment return size. */
        ret += SAMPLE_SIZE_24BITS;

        /* Increment pointers. */
        data_out += SAMPLE_SIZE_24BITS;
        data32_in++;
    }

    return ret;
}

/** @brief Pack 32-bit words containing 20-bit audio samples into 16-bit audio samples.
 *
 *  @param[in]  buffer_in       Array of the input 32-bit samples containing 20-bit audio.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the packed 16-bit stream is written to.
 *  @return Written size, in byte, to the output buffer.
 */
static uint16_t pack_20bits_16bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint32_t *data32_in = (uint32_t *)buffer_in;
    uint16_t *data_out = (uint16_t *)buffer_out;
    uint16_t sample_count = buffer_in_size / SAMPLE_SIZE_32BITS;
    uint16_t i;
    uint16_t ret = 0;

    for (i = 0; i < sample_count; i++) {
        /* Copy 16 bits MSB of input sample. */
        data_out[i] = ((data32_in[i] >> 4) & 0xFFFF);
        /* Increment return size. */
        ret += SAMPLE_SIZE_16BITS;
    }

    return ret;
}

/** @brief Pack 32-bit words containing 24-bit audio samples into 16-bit audio samples.
 *
 *  @param[in]  buffer_in       Array of the input 32-bit samples containing 24-bit audio.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the packed 16-bit stream is written to.
 *  @return Written size, in byte, to the output buffer.
 */
static uint16_t pack_24bits_16bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint32_t *data32_in = (uint32_t *)buffer_in;
    uint16_t *data_out = (uint16_t *)buffer_out;
    uint16_t sample_count = buffer_in_size / SAMPLE_SIZE_32BITS;
    uint16_t i;
    uint16_t ret = 0;

    for (i = 0; i < sample_count; i++) {
        /* Copy 16 bits MSB of input sample. */
        data_out[i] =  ((data32_in[i] >> 8) & 0xFFFF);
        /* Increment return size. */
        ret += SAMPLE_SIZE_16BITS;
    }

    return ret;
}

/** @brief Scale packed 24-bit audio samples into packed 16-bit audio samples.
 *
 *  @param[in]  buffer_in       Array of the input packed 24-bit samples.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the packed 16-bit stream is written to.
 *  @return Written size, in byte, to the output buffer.
 */
static uint16_t scale_24bits_16bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint8_t *data24_in = buffer_in;
    uint16_t *data_out = (uint16_t *)buffer_out;
    uint16_t sample_count = buffer_in_size / SAMPLE_SIZE_24BITS;
    uint16_t i;
    uint16_t ret = 0;

    for (i = 0; i < sample_count; i++) {
        /* Copy 16 bits MSB of input sample. */
        (*data_out) = (*(uint16_t *)(&(data24_in[1])) & 0xFFFF);
        /* Increment return size. */
        ret += SAMPLE_SIZE_16BITS;
        /* Increment pointers. */
        data24_in += SAMPLE_SIZE_24BITS;
        data_out++;
    }

    return ret;
}

/** @brief Unpack 18-bit audio samples into 32-bit audio samples.
 *
 *  @param[in]  buffer_in       Array of the input 18-bit samples.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the unpacked 32-bit stream is written to.
 *  @return Written size, in byte, to the output buffer.
 */
static uint16_t unpack_18bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint32_t *data32_out = (uint32_t *)buffer_out;
    uint8_t *data_in = buffer_in;
    uint16_t sample_count = (uint16_t)((buffer_in_size + SAMPLE_SIZE_18BITS - 1) / SAMPLE_SIZE_18BITS);
    uint16_t i;
    uint16_t ret = 0;

    for (i = 0; i < sample_count; i += 4) {
        /* Copy sample #1, shift left by offset required for codec's data word size. */
        (data32_out[0]) = ((*(uint32_t *)(&(data_in[0])) & 0x3FFFF) << CODEC_WORD_SIZE_OFFSET_18BITS);
        extend_msb_18bits_value(&data32_out[0]);
        /* Increment return size. */
        ret += SAMPLE_SIZE_32BITS;

        /* Check if sample #4 exists. */
        if ((i + 3) < sample_count) {
            /* Copy sample #4 and right shift by 6 bits, shift left by offset required for codec's data word size. */
            (data32_out[3]) = (((*(uint32_t *)(&(data_in[6])) & 0xFFFFC0) >> 6) << CODEC_WORD_SIZE_OFFSET_18BITS);
            extend_msb_18bits_value(&data32_out[3]);
            /* Increment return size. */
            ret += SAMPLE_SIZE_32BITS;
        }

        /* Check if sample #3 exists. */
        if ((i + 2) < sample_count) {
            /* Copy sample #3 and right shift by 4 bits, shift left by offset required for codec's data word size. */
            (data32_out[2]) = (((*(uint32_t *)(&(data_in[4])) & 0x3FFFF0) >> 4) << CODEC_WORD_SIZE_OFFSET_18BITS);
            extend_msb_18bits_value(&data32_out[2]);
            /* Increment return size. */
            ret += SAMPLE_SIZE_32BITS;
        }

        /* Check if sample #2 exists. */
        if ((i + 1) < sample_count) {
            /* Copy sample #2 and right shift by 2 bits, shift left by offset required for codec's data word size. */
            (data32_out[1]) = (((*(uint32_t *)(&(data_in[2])) & 0xFFFFC) >> 2) << CODEC_WORD_SIZE_OFFSET_18BITS);
            extend_msb_18bits_value(&data32_out[1]);
            /* Increment return size. */
            ret += SAMPLE_SIZE_32BITS;
        }

        /* Increment pointers. */
        data_in += PACKED_SIZE_18BITS;
        data32_out += 4;
    }

    return ret;
}

/** @brief Unpack 20-bit audio samples into 32-bit audio samples.
 *
 *  @param[in]  buffer_in       Array of the input 20-bit samples.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the unpacked 32-bit stream is written to.
 *  @return Written size, in byte, to the output buffer.
 */
static uint16_t unpack_20bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint32_t *data32_out = (uint32_t *)buffer_out;
    uint8_t *data_in = buffer_in;
    uint16_t sample_count = (uint16_t)((float)buffer_in_size / SAMPLE_SIZE_20BITS);
    uint16_t i;
    uint16_t ret = 0;

    for (i = 0; i < sample_count; i += 2) {
        /* Copy Sample #1. */
        (data32_out[0]) = (*(uint32_t *)(&(data_in[0])));
        extend_msb_20bits_value(&data32_out[0]);
        /* Increment return size. */
        ret += SAMPLE_SIZE_32BITS;

        /* Check if sample #2 exists. */
        if ((i + 1) < sample_count) {
            /* Copy Sample #2. */
            (data32_out[1]) = (*(uint32_t *)(&(data_in[2])) & 0xFFFFF0);
            /* Shift Sample #2 by 4 bits. */
            (data32_out[1]) >>= 4;
            extend_msb_20bits_value(&data32_out[1]);
            /* Increment return size. */
            ret += SAMPLE_SIZE_32BITS;
        }

        /* Increment pointers. */
        data_in += PACKED_SIZE_20BITS;
        data32_out += 2;
    }

    return ret;
}

/** @brief Unpack 24-bit audio samples into 32-bit audio samples.
 *
 *  @param[in]  buffer_in       Array of the input 24-bit samples.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the unpacked 32-bit stream is written to.
 *  @return Written size, in byte, to the output buffer.
 */
static uint16_t unpack_24bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint32_t *data32_out = (uint32_t *)buffer_out;
    uint8_t *data_in = buffer_in;
    uint16_t sample_count = (uint16_t)((float)buffer_in_size / SAMPLE_SIZE_24BITS);
    uint16_t i;
    uint16_t ret = 0;

    for (i = 0; i < sample_count; i++) {
        /* Copy Sample. */
        (*data32_out) = (*(uint32_t *)(&(data_in[0])) & 0xFFFFFF);
        extend_msb_24bits_value(data32_out);
        /* Increment return size. */
        ret += SAMPLE_SIZE_32BITS;

        /* Increment pointers. */
        data_in += SAMPLE_SIZE_24BITS;
        data32_out++;
    }

    return ret;
}

/** @brief Unpack 16-bit audio samples into 32-bit words containing 20-bit audio.
 *
 *  @param[in]  buffer_in       Array of the input 16-bit audio samples.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the unpacked 20-bit stream is written to.
 *  @return Written size, in byte, to the output buffer.
 */
static uint16_t unpack_20bits_16bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint16_t *data32_in = (uint16_t *)buffer_in;
    uint32_t *data_out = (uint32_t *)buffer_out;
    uint16_t sample_count = buffer_in_size / SAMPLE_SIZE_16BITS;
    uint16_t i;
    uint16_t ret = 0;

    for (i = 0; i < sample_count; i++) {
        /* Copy input sample. */
        data_out[i] = ((data32_in[i] << 4) & 0x000FFFF0);
        extend_msb_20bits_value(&data_out[i]);
        /* Increment return size. */
        ret += SAMPLE_SIZE_32BITS;
    }

    return ret;
}

/** @brief Unpack 16-bit audio samples into 32-bit words containing 24-bit audio.
 *
 *  @param[in]  buffer_in       Array of the input 16-bit audio samples.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Array where the unpacked 24-bit stream is written to.
 *  @return Written size, in byte, to the output buffer.
 */
static uint16_t unpack_24bits_16bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint16_t *data32_in = (uint16_t *)buffer_in;
    uint32_t *data_out = (uint32_t *)buffer_out;
    uint16_t sample_count = buffer_in_size / SAMPLE_SIZE_16BITS;
    uint16_t i;
    uint16_t ret = 0;

    for (i = 0; i < sample_count; i++) {
        /* Copy input sample. */
        data_out[i] = ((data32_in[i] << 8) & 0x00FFFF00);
        extend_msb_24bits_value(&data_out[i]);
        /* Increment return size. */
        ret += SAMPLE_SIZE_32BITS;
    }

    return ret;
}

/** @brief Extend 18-bit audio samples sign bit into 32-bit word.
 *
 *  @param[in]  buffer_in       Array of the input 32-bit samples containing 18-bit audio.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Output of the input 32-bit samples.
 *  @return Written size, in byte, to the output buffer.
 */
static uint16_t extend_18bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint32_t *data32_out = (uint32_t *)buffer_out;
    uint16_t sample_count = buffer_in_size / SAMPLE_SIZE_32BITS;
    uint16_t i;

    /* Copy data into output buffer. */
    memcpy(buffer_out, buffer_in, buffer_in_size);

    /* Extend sign bit of output buffer samples. */
    for (i = 0; i < sample_count; i++) {
        extend_msb_18bits_value(&data32_out[i]);
    }

    return buffer_in_size;
}

/** @brief Extend 20-bit audio samples sign bit into 32-bit word.
 *
 *  @param[in]  buffer_in       Array of the input 32-bit samples containing 20-bit audio.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Output of the input 32-bit samples.
 *  @return Written size, in byte, to the output buffer.
 */
static uint16_t extend_20bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint32_t *data32_out = (uint32_t *)buffer_out;
    uint16_t sample_count = buffer_in_size / SAMPLE_SIZE_32BITS;
    uint16_t i;

    /* Copy data into output buffer. */
    memcpy(buffer_out, buffer_in, buffer_in_size);

    /* Extend sign bit of output buffer samples. */
    for (i = 0; i < sample_count; i++) {
        extend_msb_20bits_value(&data32_out[i]);
    }

    return buffer_in_size;
}

/** @brief Extend 24-bit audio samples sign bit into 32-bit word.
 *
 *  @param[in]  buffer_in       Array of the input 32-bit samples containing 24-bit audio.
 *  @param[in]  buffer_in_size  Size in byte of the input array.
 *  @param[out] buffer_out      Output of the input 32-bit samples.
 *  @return Written size, in byte, to the output buffer.
 */
static uint16_t extend_24bits(uint8_t *buffer_in, uint16_t buffer_in_size, uint8_t *buffer_out)
{
    uint32_t *data32_out = (uint32_t *)buffer_out;
    uint16_t sample_count = buffer_in_size / SAMPLE_SIZE_32BITS;
    uint16_t i;

    /* Copy data into output buffer. */
    memcpy(buffer_out, buffer_in, buffer_in_size);

    /* Extend sign bit of output buffer samples. */
    for (i = 0; i < sample_count; i++) {
        extend_msb_24bits_value(&data32_out[i]);
    }

    return buffer_in_size;
}

/** @brief Extend 18-bit value's sign bit into 32-bit word.
 *
 *  @param[in] value  The 18-bit input value to be extended.
 */
static void extend_msb_18bits_value(uint32_t *value)
{
    *value >>= CODEC_WORD_SIZE_OFFSET_18BITS;

    if ((*value) & (1 << 17)) {
        /* Negative value. */
        *value |= 0xFFFC0000;
    } else {
        /* Positive value. */
        *value &= 0x0003FFFF;
    }

    *value <<= CODEC_WORD_SIZE_OFFSET_18BITS;
}

/** @brief Extend 20-bit value's sign bit into 32-bit word.
 *
 *  @param[in] value  The 20-bit input value to be extended.
 */
static void extend_msb_20bits_value(uint32_t *value)
{
    if ((*value) & (1 << 19)) {
        /* Negative value. */
        *value |= 0xFFF00000;
    } else {
        /* Positive value. */
        *value &= 0x000FFFFF;
    }
}

/** @brief Extend 24-bit value's sign bit into 32-bit word.
 *
 *  @param[in] value  The 24-bit input value to be extended.
 */
static void extend_msb_24bits_value(uint32_t *value)
{
    if ((*value) & (1 << 23)) {
        /* Negative value. */
        *value |= 0xFF000000;
    } else {
        /* Positive value. */
        *value &= 0x00FFFFFF;
    }
}
