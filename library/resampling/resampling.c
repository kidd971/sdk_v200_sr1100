/** @file  resampling.c
 *  @brief Add or remove a sample over a predefined number of samples by doing a linear interpolation.
 *
 *  @copyright Copyright (C) 2019 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "resampling.h"
#include <stdbool.h>
#include <string.h>

/* CONSTANTS ******************************************************************/
#define ADD_REM_DIFF       2

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static uint16_t resample_add_sample(resampling_instance_t *instance, uint32_t *ptr_input, uint32_t *ptr_output, uint16_t sample_count);
static uint16_t resample_remove_sample(resampling_instance_t *instance, uint32_t *ptr_input, uint32_t *ptr_output, uint16_t sample_count);
static uint16_t resample_bypass(resampling_instance_t *instance, uint32_t *ptr_input, uint32_t *ptr_output, uint16_t sample_count);
static void resampling_stop(resampling_instance_t *instance);
static uint32_t interp_linear(resampling_instance_t *instance, int32_t *y, int32_t *y1, int32_t *out, uint16_t size);
static void update_last_sample(resampling_instance_t *instance, int32_t *ptr_input, uint16_t sample_count);
static int32_t cast_type_read(resampling_instance_t *instance, int32_t *in, uint16_t index);
static void cast_type_write(resampling_instance_t *instance, int32_t *out, uint16_t index, int32_t value);
static int32_t *get_ptr_addr(resampling_instance_t *instance, int32_t *sample_array, uint16_t idx);
static uint8_t sizeof_buffer_type(resampling_instance_t *instance);

/* PUBLIC FUNCTIONS ***********************************************************/
resampling_errors_t resampling_init(resampling_instance_t *instance, resampling_config_t *resampling_config)
{
    /* WARNING this initialisation works only in audio 16 bits sample bit depth */
    uint32_t resampling_size;
    uint16_t nb_sample_ch;

    /* Config verification */
    if (resampling_config->nb_channel > RESAMPLING_CFG_MAX_NB_CHANNEL) {
        return RESAMPLING_INVALID_NB_CHANNEL;
    }
    switch (resampling_config->buffer_type) {
    case BUFFER_8BITS:
    case BUFFER_16BITS:
    case BUFFER_20BITS:
    case BUFFER_24BITS:
    case BUFFER_32BITS:
        break;
    default:
        return RESAMPLING_INVALID_TYPE;
    }

    /* Struct initialization */
    instance->status      = RESAMPLING_WAIT_QUEUE_FULL;
    instance->correction  = RESAMPLING_NO_CORRECTION;
    instance->buffer_type = resampling_config->buffer_type;
    instance->nb_channel  = resampling_config->nb_channel;
    nb_sample_ch = resampling_config->nb_sample / resampling_config->nb_channel;

    instance->buffer_type_max = (1 << instance->buffer_type);

    resampling_size  = (uint32_t)(resampling_config->resampling_length / nb_sample_ch) * nb_sample_ch;

    instance->step_add      = (uint32_t)(instance->buffer_type_max / resampling_size);
    instance->step_rem      = (uint32_t)(instance->buffer_type_max / (resampling_size - ADD_REM_DIFF));

    instance->max_x_axis    = (uint32_t)((resampling_size - 1) * (1.0 / resampling_size) * instance->buffer_type_max);

    instance->bias_step_add = (uint32_t)((((double)1.0 / (double)resampling_size) * (double)instance->buffer_type_max -
                                          (double)instance->step_add) * (double)instance->buffer_type_max);
    instance->bias_step_rem = (uint32_t)((((double)1.0 / (double)(resampling_size - ADD_REM_DIFF)) * (double)instance->buffer_type_max -
                                          (double)instance->step_rem) * (double)instance->buffer_type_max);

    return RESAMPLING_NO_ERROR;
}

void resampling_start(resampling_instance_t *instance, resampling_correction_t correction)
{
    instance->status = RESAMPLING_START;
    instance->correction = correction;
}

uint16_t resample(resampling_instance_t *instance, void *ptr_input, void *ptr_output, uint16_t sample_count)
{
    if (instance->status != RESAMPLING_IDLE) {
        switch (instance->correction) {
        case RESAMPLING_ADD_SAMPLE:
                return resample_add_sample(instance, (uint32_t *)ptr_input, (uint32_t *)ptr_output, sample_count);
                break;
        case RESAMPLING_REMOVE_SAMPLE:
                return resample_remove_sample(instance, ptr_input, ptr_output, sample_count);
                break;
        case RESAMPLING_NO_CORRECTION:
                return resample_bypass(instance, ptr_input, ptr_output, sample_count);
                break;
        default:
            return resample_bypass(instance, ptr_input, ptr_output, sample_count);
            break;
        }
    } else {
        return resample_bypass(instance, ptr_input, ptr_output, sample_count);
    }
}

resampling_status_t resample_get_state(resampling_instance_t *instance)
{
    return instance->status;
}

uint8_t resample_get_channel_count(resampling_instance_t *instance)
{
    return instance->nb_channel;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Add a sample over n samples using array.
 *
 *  @param[in]  instance      Structure instance pointer.
 *  @param[in]  ptr_input     Pointer to input data.
 *  @param[in]  sample_count  Amount of samples to be treated.
 *  @param[out] ptr_output    Pointer to output data.
 *  @return Samples count.
 */
static uint16_t resample_add_sample(resampling_instance_t *instance, uint32_t *ptr_input, uint32_t *ptr_output, uint16_t sample_count)
{
    uint16_t size;
    uint8_t nb_ch;

    /* Initialise variable */
    nb_ch = (instance->nb_channel == 0) ? 1 : instance->nb_channel;
    size = 0;

    /*
     * Resampling START
     */
    if (instance->status == RESAMPLING_START) {
        instance->status = RESAMPLING_RUNNING;
        instance->bias = instance->bias_step_add;
        instance->x_axis = instance->max_x_axis;
        /*
         * The first sample of the resampled signal and input are the same.
         * for loop treat every channel.
         */
        for (uint8_t mux_index = 0; mux_index < nb_ch; mux_index++) {
            cast_type_write(instance, (int32_t *)ptr_output, size,
                            (int32_t)cast_type_read(instance, (int32_t *)instance->last_sample, nb_ch + mux_index));
            size++;
        }
    /*
     * resampling is already running
     */
    } else if (instance->status == RESAMPLING_RUNNING) {
        /*
         * Calculate the first value(s) of the output buffer using the last samples of last interpolation.
         */
        size += interp_linear(instance, get_ptr_addr(instance, instance->last_sample, nb_ch),
                              (int32_t *)instance->last_sample, (int32_t *)ptr_output, nb_ch);
    }

    /*
     * Calculate the second value(s) of the output buffer using the last sample of last interpolation.
     */
    size += interp_linear(instance, (int32_t *)ptr_input, get_ptr_addr(instance, instance->last_sample, nb_ch),
                          (int32_t *)get_ptr_addr(instance, (int32_t *)ptr_output, size), nb_ch);

    /*
     * Interpolation
     */
    size += interp_linear(instance, (int32_t *)get_ptr_addr(instance, (int32_t *)ptr_input, nb_ch), (int32_t *)ptr_input,
                          (int32_t *)get_ptr_addr(instance, (int32_t *)ptr_output, size), (sample_count - size));

    update_last_sample(instance, (int32_t *)ptr_input, sample_count);

    /* If the resampling is complete, last value of each signal is equal to the last value on the input signal. */
    if (instance->x_axis <= instance->step_add) {
        for (uint8_t mux_index = 0; mux_index < nb_ch; mux_index++) {
            cast_type_write(instance, (int32_t *)ptr_output, size,
                            cast_type_read(instance, (int32_t *)ptr_input, size - (nb_ch * LAST_SAMPLE_AMT)));
            size++;
        }
        resampling_stop(instance);
    }

    return size;
}

/** @brief Remove a sample over n samples.
 *
 *  @param[in]  instance      Structure instance pointer.
 *  @param[in]  ptr_input     Pointer to input data.
 *  @param[in]  sample_count  Amount of samples to be treated.
 *  @param[out] ptr_output    Pointer to output data.
 *  @return Samples count.
 */
static uint16_t resample_remove_sample(resampling_instance_t *instance, uint32_t *ptr_input, uint32_t *ptr_output, uint16_t sample_count)
{
    volatile uint16_t size;
    uint8_t nb_ch;

    nb_ch = instance->nb_channel == 0 ? 1 : instance->nb_channel;
    size = 0;
    /*
     * Resampling START
     */
    if (instance->status == RESAMPLING_START) {
        instance->status = RESAMPLING_RUNNING;
        instance->bias = instance->bias_step_rem;
        instance->x_axis = instance->step_rem;
        /*
         * The first sample of the resampled signal and input are the same.
         * for loop treat every channel.
         */
        for (uint8_t mux_index = 0; mux_index < nb_ch; mux_index++) {
            cast_type_write(instance, (int32_t *)ptr_output, size,
                            (int32_t)cast_type_read(instance, (int32_t *)instance->last_sample, nb_ch + mux_index));
            size++;
        }
    /*
     * resampling is already running
     */
    } else if (instance->status == RESAMPLING_RUNNING) {
        /*
         * Calculate the first value(s) of the output buffer using the last sample of last interpolation.
         */
        size += interp_linear(instance, (int32_t *)ptr_input, get_ptr_addr(instance, (int32_t *)instance->last_sample, nb_ch),
                              (int32_t *)ptr_output, nb_ch);
    }

    /*
     * Interpolation
     */
    size += interp_linear(instance, get_ptr_addr(instance, (int32_t *)ptr_input, size), (int32_t *)ptr_input,
                          get_ptr_addr(instance, (int32_t *)ptr_output, size), sample_count - size);

    update_last_sample(instance, (int32_t *)ptr_input, sample_count);

    /* When the resampling is finish, remove extra sample. */
    if (instance->x_axis >= instance->max_x_axis) {
        for (uint8_t mux_index = 0; mux_index < nb_ch; mux_index++) {
            cast_type_write(instance, (int32_t *)ptr_output, size, cast_type_read(instance, (int32_t *)ptr_input, size));
            size++;
        }
        resampling_stop(instance);
    }

    return size;
}

/** @brief Move data to output buffer without correction.
 *
 *  @param[in]  instance      Structure instance pointer.
 *  @param[in]  ptr_input     Pointer to input data.
 *  @param[in]  sample_count  Amount of samples to be treated.
 *  @param[out] ptr_output    Pointer to output data.
 *  @return Samples count.
 */
static uint16_t resample_bypass(resampling_instance_t *instance, uint32_t *ptr_input, uint32_t *ptr_output, uint16_t sample_count)
{
    uint16_t size;
    uint8_t nb_ch;

    /* Initialise variable */
    nb_ch = (instance->nb_channel == 0) ? 1 : instance->nb_channel;
    size = 0;

    /* First sample is last sample of last pkt */
    for (uint8_t mux_index = 0; mux_index < nb_ch; mux_index++) {
        cast_type_write(instance, (int32_t *)ptr_output, size,
                        (int32_t)cast_type_read(instance, (int32_t *)instance->last_sample, nb_ch + mux_index));
        size++;
    }

    memcpy((void *)get_ptr_addr(instance, (int32_t *)ptr_output, size),
           (void *)ptr_input,
           sizeof_buffer_type(instance) * (sample_count - size));

    update_last_sample(instance, (int32_t *)ptr_input, sample_count);

    return sample_count;
}

/** @brief Change resampling status to RESAMPLING_IDLE.
 *
 *  @param[in] instance  Structure instance pointer.
 */
static void resampling_stop(resampling_instance_t *instance)
{
    instance->status = RESAMPLING_IDLE;
    instance->correction = RESAMPLING_NO_CORRECTION;
}

/** @brief Linear interpolation
 *
 *  @param[in]  instance  Structure instance pointer.
 *  @param[in]  y         First data.
 *  @param[in]  y1        Second data.
 *  @param[in]  size     Number of iteration to compute.
 *  @param[out] out      Linear interpolation result.
 *  @return Number of computation.
 */
static uint32_t interp_linear(resampling_instance_t *instance, int32_t *y, int32_t *y1, int32_t *out, uint16_t size)
{
    uint16_t idx;
    uint8_t nb_ch;
    int64_t y1_value;
    int64_t y_value;
    int32_t interp;
    int8_t bias_comp;

    idx = 0;
    nb_ch = instance->nb_channel;

    while (idx < size) {
        y1_value = cast_type_read(instance, y1, idx);
        y_value  = cast_type_read(instance, y, idx);

        interp = (int32_t)(y1_value + ((instance->x_axis * (y_value - y1_value)) >> instance->buffer_type));

        cast_type_write(instance, out, idx, interp);

        idx++;
        /* If all channel have been calculated of the current lookup table index, increment the index */
        if ((idx % nb_ch) == 0) {
            if (instance->correction == RESAMPLING_ADD_SAMPLE) {
                instance->bias += instance->bias_step_add;
                if (instance->bias >= instance->buffer_type_max) {
                    bias_comp = 1;
                    instance->bias -= instance->buffer_type_max;
                } else {
                    bias_comp = 0;
                }
                if (instance->x_axis > (instance->step_add + bias_comp)) {
                    instance->x_axis -= (instance->step_add + bias_comp);
                } else {
                    /* Resampling done */
                    break;
                }
            } else {
                instance->bias += instance->bias_step_rem;
                if (instance->bias >= instance->buffer_type_max) {
                    bias_comp = 1;
                    instance->bias -= instance->buffer_type_max;
                } else {
                    bias_comp = 0;
                }
                instance->x_axis += instance->step_rem + bias_comp;
                if (instance->x_axis > instance->max_x_axis) {
                    /* Resampling done */
                    break;
                }
            }
        }
    }
    return idx;
}

/** @brief Move last samples of input to last_sample array of instance.
 *
 *  @param[in]  instance      Structure instance pointer.
 *  @param[in]  ptr_input     Pointer to input data.
 *  @param[in]  sample_count  Amount of samples to be treated.
 *  @return Samples count.
 */
static void update_last_sample(resampling_instance_t *instance, int32_t *ptr_input, uint16_t sample_count)
{
    uint16_t nb_sample = LAST_SAMPLE_AMT * instance->nb_channel; /* nb sample in last_sample array */

    for (uint16_t mux_index = 0; mux_index < nb_sample; mux_index++) {
        cast_type_write(instance, instance->last_sample, mux_index,
                        (int32_t)cast_type_read(instance, (int32_t *)ptr_input, (sample_count - nb_sample) + mux_index));
    }
}

/** @brief Read casting type
 *
 *  @param[in] instance  Structure instance pointer.
 *  @param[in] in        Pointer to the array.
 *  @param[in] index     Array index.
 *  @return data in the memory according to the cast.
 */
static int32_t cast_type_read(resampling_instance_t *instance, int32_t *in, uint16_t index)
{
    int32_t value;

    switch (instance->buffer_type) {
    case BUFFER_8BITS:
        value = ((int8_t *)in)[index];
        break;
    case BUFFER_16BITS:
        value = ((int16_t *)in)[index];
        break;
    case BUFFER_20BITS:
    case BUFFER_24BITS:
    case BUFFER_32BITS:
        value = ((int32_t *)in)[index];
        break;
    default:
        value = ((int16_t *)in)[index];
        break;
    }

    return value;
}

/** @brief Write casting type.
 *
 *  @param[in]  instance  Structure instance pointer.
 *  @param[in]  index     Array index.
 *  @param[in]  value     data to write in the memory.
 *  @param[out] out       Pointer to the array.
 */
static void cast_type_write(resampling_instance_t *instance, int32_t *out, uint16_t index, int32_t value)
{
    switch (instance->buffer_type) {
    case BUFFER_8BITS:
        ((int8_t *)out)[index] = (int8_t)value;
        break;
    case BUFFER_16BITS:
        ((int16_t *)out)[index] = (int16_t)value;
        break;
    case BUFFER_20BITS:
    case BUFFER_24BITS:
    case BUFFER_32BITS:
        ((int32_t *)out)[index] = (int32_t)value;
        break;
    default:
        ((int16_t *)out)[index] = (int16_t)value;
        break;
    }
}

/** @brief Get the address pointer of an array.
 *
 *  @param[in] instance      Structure instance pointer.
 *  @param[in] sample_array  Pointer to the array.
 *  @param[in] idx           Array index.
 *  @return memory address according to the index.
 */
static int32_t *get_ptr_addr(resampling_instance_t *instance, int32_t *sample_array, uint16_t idx)
{
    switch (instance->buffer_type) {
    case BUFFER_8BITS:
        return (int32_t *)&((int8_t *)sample_array)[idx];
    case BUFFER_16BITS:
        return (int32_t *)&((int16_t *)sample_array)[idx];
    case BUFFER_20BITS:
    case BUFFER_24BITS:
    case BUFFER_32BITS:
        return (int32_t *)&((int32_t *)sample_array)[idx];
    default:
        return (int32_t *)&((int16_t *)sample_array)[idx];
    }
}

/** @brief Get the size in bytes of the buffer type.
 *
 *  @param[in] instance      Structure instance pointer.
 *  @return buffer size.
 */
static uint8_t sizeof_buffer_type(resampling_instance_t *instance)
{
    switch (instance->buffer_type) {
    case BUFFER_8BITS:
        return sizeof(int8_t);
    case BUFFER_16BITS:
        return sizeof(int16_t);
    case BUFFER_20BITS:
    case BUFFER_24BITS:
    case BUFFER_32BITS:
        return sizeof(int32_t);
    default:
        return sizeof(int16_t);
    }
}
