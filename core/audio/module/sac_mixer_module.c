/** @file  sac_mixer_module.c
 *  @brief SPARK Audio Core Mixer Module is used to mix multiple audio streams into one.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sac_mixer_module.h"
#include <stddef.h>

/* PRIVATE FUNCTION PROTOTYPE *************************************************/
static void algo_mix_int16_samples(sac_mixer_module_t *sac_mixer_module);
static uint8_t get_audio_payload_samples_count(sac_mixer_module_t *sac_mixer_module);

/* PUBLIC FUNCTIONS ***********************************************************/
sac_mixer_module_t *sac_mixer_module_init(sac_mixer_module_cfg_t cfg, mem_pool_t *mem_pool, sac_status_t *sac_status)
{
    *sac_status = SAC_OK;

    sac_mixer_module_t *sac_mixer_module = (sac_mixer_module_t *)mem_pool_malloc(mem_pool, sizeof(sac_mixer_module_t));

    if (sac_mixer_module == NULL) {
        *sac_status = SAC_ERR_NOT_ENOUGH_MEMORY;
        return NULL;
    }

    /* Verify configurations */
    if (cfg.nb_of_inputs < MIN_NB_OF_INPUTS ||
        cfg.nb_of_inputs > MAX_NB_OF_INPUTS) {
        *sac_status = SAC_ERR_MIXER_INIT_FAILURE;
        return NULL;
    }

    if (cfg.bit_depth != 16) {
        *sac_status = SAC_ERR_MIXER_INIT_FAILURE;
        return NULL;
    }

    if ((cfg.payload_size < MIN_NB_OF_BYTES_PER_PAYLOAD) ||
        (cfg.payload_size > MAX_NB_OF_BYTES_PER_PAYLOAD)) {
        *sac_status = SAC_ERR_MIXER_INIT_FAILURE;
        return NULL;
    }

    /* Apply the configurations */
    sac_mixer_module->cfg = cfg;

    return sac_mixer_module;
}

void sac_mixer_module_mix_packets(sac_mixer_module_t *sac_mixer_module)
{
    if (sac_mixer_module->cfg.bit_depth == 16) {
        algo_mix_int16_samples(sac_mixer_module);
    }

    sac_mixer_module_handle_remainder(sac_mixer_module);
}

void sac_mixer_module_append_samples(sac_mixer_queue_t *input_samples_queue, uint8_t *samples, uint8_t size)
{
    uint8_t mem_offset = 0;

    /* Use the current size for the memory offset */
    mem_offset = input_samples_queue->current_size;

    /* Add the payload to the Input Samples Queue */
    memcpy(input_samples_queue->samples + mem_offset, samples, size);

    /* Update Input Samples Queue size */
    input_samples_queue->current_size += size;
}

void sac_mixer_module_append_silence(sac_mixer_queue_t *input_samples_queue, uint8_t size)
{
    uint8_t mem_offset = 0;

    /* Use the current size for the memory offset */
    mem_offset = input_samples_queue->current_size;

    /* Add the payload to the Input Samples Queue */
    memset(input_samples_queue->samples + mem_offset, 0, size);

    /* Update Input Samples Queue size */
    input_samples_queue->current_size += size;
}

void sac_mixer_module_handle_remainder(sac_mixer_module_t *sac_mixer_module)
{
    uint8_t sample_remainder = 0;
    uint8_t current_size = 0;
    uint8_t sample_size = 0;

    sample_size = sac_mixer_module->cfg.payload_size;

    /* Move the remaining input samples to the front of the queue */
    for (uint8_t input = 0; input < sac_mixer_module->cfg.nb_of_inputs; input++) {
        current_size = sac_mixer_module->input_samples_queue[input].current_size;
        sample_remainder = current_size - sample_size;

        if (sample_remainder > 0) {
            memcpy(sac_mixer_module->input_samples_queue[input].samples,
                   sac_mixer_module->input_samples_queue[input].samples + sample_size,
                   sample_remainder);
        }
        sac_mixer_module->input_samples_queue[input].current_size = sample_remainder;
    }
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Mixing algorithm using int16 samples.
 *
 *  @param[in] sac_mixer_module  SPARK Audio Core Mixer Module structure.
 */
static void algo_mix_int16_samples(sac_mixer_module_t *sac_mixer_module)
{
    uint8_t nb_of_inputs = 0;
    uint8_t packet_size = 0;
    int32_t sample_summation = 0;

    nb_of_inputs = sac_mixer_module->cfg.nb_of_inputs;
    packet_size = get_audio_payload_samples_count(sac_mixer_module);

    for (uint8_t sample = 0; sample < packet_size; sample++) {
        for (uint8_t input = 0; input < nb_of_inputs; input++) {
            sample_summation += ((int16_t *)sac_mixer_module->input_samples_queue[input].samples)[sample];
        }
        ((int16_t *)sac_mixer_module->output_packet_buffer)[sample] = (sample_summation / nb_of_inputs);
        sample_summation = 0;
    }
}

/** @brief Get the number of samples forming an audio payload.
 *
 *  @param[in] sac_mixer_module  SPARK Audio Core Mixer Module structure.
 *  @return Number of samples.
 */
static uint8_t get_audio_payload_samples_count(sac_mixer_module_t *sac_mixer_module)
{
    uint8_t size_type;

    if (sac_mixer_module->cfg.bit_depth == 16) {
        size_type = sizeof(int16_t);
    }

    return sac_mixer_module->cfg.payload_size / size_type;
}
