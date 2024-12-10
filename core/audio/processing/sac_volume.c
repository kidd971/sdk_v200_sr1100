/** @file  sac_volume.c
 *  @brief SPARK Audio Core processing functions related to the software volume control.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include <string.h>
#include "sac_volume.h"

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void volume_increase(sac_volume_instance_t *volume_ctrl);
static void volume_decrease(sac_volume_instance_t *volume_ctrl);
static void volume_mute(sac_volume_instance_t *volume_ctrl);
static float volume_get_level(sac_volume_instance_t *volume_ctrl);
static void adjust_volume_factor(sac_volume_instance_t *volume);
static void apply_volume_factor_16bits(int16_t *audio_samples_in, uint16_t samples_count, int16_t *audio_samples_out,
                                       float volume_factor);
static void apply_volume_factor_32bits(int32_t *audio_samples_in, uint16_t samples_count, int32_t *audio_samples_out,
                                       float volume_factor);
static void validate_sac_bit_depth(sac_bit_depth_t bit_depth, sac_status_t *status);

/* PUBLIC FUNCTIONS ***********************************************************/
void sac_volume_init(void *instance, const char *name, sac_pipeline_t *pipeline, mem_pool_t *mem_pool,
                     sac_status_t *status)
{
    (void)pipeline;
    (void)mem_pool;
    (void)name;

    sac_volume_instance_t *vol_inst = instance;

    *status = SAC_OK;

    if (vol_inst == NULL) {
        *status = SAC_ERR_NULL_PTR;
        return;
    }

    validate_sac_bit_depth(vol_inst->sample_format.bit_depth, status);
    if (*status != SAC_OK) {
        return;
    }

    if (vol_inst->sample_format.bit_depth != SAC_16BITS) {
        if (vol_inst->sample_format.sample_encoding != SAC_SAMPLE_UNPACKED) {
            /* Only unpacked samples are supported. */
            *status = SAC_ERR_PROCESSING_STAGE_INIT;
            return;
        }
    }

    if (vol_inst->initial_volume_level > (SAC_VOLUME_MAX * 100.0)) {
        *status = SAC_ERR_PROCESSING_STAGE_INIT;
        return;
    }

    vol_inst->_internal.volume_factor = (vol_inst->initial_volume_level / 100.0);
    vol_inst->_internal.volume_threshold = (vol_inst->initial_volume_level / 100.0);
}

uint32_t sac_volume_ctrl(void *instance, sac_pipeline_t *pipeline, uint8_t cmd, uint32_t arg, sac_status_t *status)
{
    (void)pipeline;
    (void)arg;

    uint32_t ret = 0;
    sac_volume_instance_t *vol_inst = instance;

    *status = SAC_OK;

    switch ((sac_volume_cmd_t)cmd) {
    case SAC_VOLUME_INCREASE:
        volume_increase(vol_inst);
        break;
    case SAC_VOLUME_DECREASE:
        volume_decrease(vol_inst);
        break;
    case SAC_VOLUME_MUTE:
        volume_mute(vol_inst);
        break;
    case SAC_VOLUME_GET_FACTOR:
        ret = (uint32_t)(volume_get_level(vol_inst) * 10000.0);
        break;
    default:
        *status = SAC_ERR_INVALID_CMD;
    }
    return ret;
}

uint16_t sac_volume_process(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in,
                            uint16_t size, uint8_t *data_out, sac_status_t *status)
{
    (void)pipeline;
    (void)header;

    sac_volume_instance_t *vol_inst = instance;

    *status = SAC_OK;

    if ((vol_inst->_internal.volume_threshold != SAC_VOLUME_MAX) || (vol_inst->_internal.volume_factor != SAC_VOLUME_MAX)) {
        adjust_volume_factor(vol_inst);

        switch (vol_inst->sample_format.bit_depth) {
        case SAC_16BITS:
            if (vol_inst->sample_format.sample_encoding == SAC_SAMPLE_PACKED) {
                apply_volume_factor_16bits((int16_t *)data_in, (size / (SAC_WORD_SIZE_BYTE / 2)), (int16_t *)data_out, vol_inst->_internal.volume_factor);
            } else {
                apply_volume_factor_32bits((int32_t *)data_in, (size / SAC_WORD_SIZE_BYTE), (int32_t *)data_out, vol_inst->_internal.volume_factor);
            }
            break;
        case SAC_20BITS:
            apply_volume_factor_32bits((int32_t *)data_in, (size / SAC_WORD_SIZE_BYTE), (int32_t *)data_out, vol_inst->_internal.volume_factor);
            break;
        case SAC_24BITS:
            apply_volume_factor_32bits((int32_t *)data_in, (size / SAC_WORD_SIZE_BYTE), (int32_t *)data_out, vol_inst->_internal.volume_factor);
            break;
        case SAC_32BITS:
            apply_volume_factor_32bits((int32_t *)data_in, (size / SAC_WORD_SIZE_BYTE), (int32_t *)data_out, vol_inst->_internal.volume_factor);
            break;
        default:
            return 0;
        }

        return size;
    } else {
        return 0;
    }
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Increase the audio volume.
 *
 *  @param[in] instance  Volume instance.
 */
static void volume_increase(sac_volume_instance_t *instance)
{
    instance->_internal.volume_threshold += SAC_VOLUME_TICK;
    if (instance->_internal.volume_threshold >= SAC_VOLUME_MAX) {
        instance->_internal.volume_threshold = SAC_VOLUME_MAX;
    }
}

/** @brief Decrease the audio volume.
 *
 *  @param[in] instance  Volume instance.
 */
static void volume_decrease(sac_volume_instance_t *instance)
{
    instance->_internal.volume_threshold -= SAC_VOLUME_TICK;
    if (instance->_internal.volume_threshold <= SAC_VOLUME_MIN) {
        instance->_internal.volume_threshold = SAC_VOLUME_MIN;
    }
}

/** @brief Mute the audio.
 *
 *  @param[in] instance  Volume instance.
 */
static void volume_mute(sac_volume_instance_t *instance)
{
    instance->_internal.volume_factor = 0;
    instance->_internal.volume_threshold = 0;
}

/** @brief Get the audio volume level.
 *
 *  @param[in] instance  Volume instance.
 *  @return Volume level.
 */
static float volume_get_level(sac_volume_instance_t *instance)
{
    return instance->_internal.volume_factor;
}

/** @brief Adjust volume factor to tend toward volume threshold.
 *
 *  @param[in]  instance  Volume instance.
 */
static void adjust_volume_factor(sac_volume_instance_t *instance)
{
    /*
     * Test if factor increases or decreases and reaches the desired value.
     * The value is correct if overflow.
     */
    if (instance->_internal.volume_factor < instance->_internal.volume_threshold) {
        instance->_internal.volume_factor += SAC_VOLUME_GRAD;
        if (instance->_internal.volume_factor >= instance->_internal.volume_threshold) {
            instance->_internal.volume_factor = instance->_internal.volume_threshold;
        }
    } else if (instance->_internal.volume_factor > instance->_internal.volume_threshold) {
        instance->_internal.volume_factor -= SAC_VOLUME_GRAD;
        if (instance->_internal.volume_factor <= instance->_internal.volume_threshold) {
            instance->_internal.volume_factor = instance->_internal.volume_threshold;
        }
    }
}

/** @brief Process a volume factor on each sample.
 *
 *  @param[in]  audio_samples_in   16bits samples pointer of data in.
 *  @param[in]  samples_count      Number of samples to process.
 *  @param[out] audio_samples_out  16bits samples pointer of data out.
 *  @param[in]  volume_factor      Volume factor to apply.
 */
static void apply_volume_factor_16bits(int16_t *audio_samples_in, uint16_t samples_count,
                                       int16_t *audio_samples_out, float volume_factor)
{
    uint16_t count;

    for (count = 0; count < samples_count; count++) {
        audio_samples_out[count] = audio_samples_in[count] * volume_factor;
    }
}

/** @brief Process a volume factor on each sample.
 *
 *  @param[in]  audio_samples_in   32bits samples pointer of data in.
 *  @param[in]  samples_count      Number of samples to process.
 *  @param[out] audio_samples_out  32bits samples pointer of data out.
 *  @param[in]  volume_factor      Volume factor to apply.
 */
static void apply_volume_factor_32bits(int32_t *audio_samples_in, uint16_t samples_count,
                                       int32_t *audio_samples_out, float volume_factor)
{
    uint16_t count;

    for (count = 0; count < samples_count; count++) {
        audio_samples_out[count] = audio_samples_in[count] * volume_factor;
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
