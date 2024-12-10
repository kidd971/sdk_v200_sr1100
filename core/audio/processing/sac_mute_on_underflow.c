/** @file  sac_mute_on_underflow.c
 *  @brief Processing stage used to mute the audio output of a device for a short time
 *         when an underflow occurs.
 *
 *  @note This processing stage should be the last processing stage on an audio receiving pipeline.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sac_mute_on_underflow.h"
#include <string.h>
#include "sac_stats.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void sac_mute_on_underflow_init(void *instance, const char *name, sac_pipeline_t *pipeline, mem_pool_t *mem_pool,
                                sac_status_t *status)
{
    (void)mem_pool;
    (void)name;

    sac_mute_on_underflow_instance_t *sac_mute_on_underflow_instance = instance;

    *status = SAC_OK;

    if (sac_mute_on_underflow_instance == NULL) {
        *status = SAC_ERR_NULL_PTR;
        return;
    }

    sac_mute_on_underflow_instance->_internal.counter = 0;
    sac_mute_on_underflow_instance->_internal.underflow_count = sac_pipeline_get_consumer_buffer_underflow_count(pipeline);
}

uint32_t sac_mute_on_underflow_ctrl(void *instance, sac_pipeline_t *pipeline, uint8_t cmd, uint32_t arg,
                                    sac_status_t *status)
{
    (void)pipeline;

    uint32_t ret = 0;
    sac_mute_on_underflow_instance_t *sac_mute_on_underflow_instance = instance;

    *status = SAC_OK;

    switch ((sac_mute_on_underflow_cmd_t)cmd) {
    case SAC_MUTE_ON_UNDERFLOW_SET_RELOAD:
        sac_mute_on_underflow_instance->reload_value = arg;
        break;
    default:
        *status = SAC_ERR_INVALID_CMD;
    }
    return ret;
}

uint16_t sac_mute_on_underflow_process(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in,
                                       uint16_t size, uint8_t *data_out, sac_status_t *status)
{
    (void)header;
    (void)data_in;

    uint32_t current_underflow_count;
    uint16_t return_size = 0;
    sac_mute_on_underflow_instance_t *sac_mute_on_underflow_instance = instance;

    *status = SAC_OK;

    current_underflow_count = sac_pipeline_get_consumer_buffer_underflow_count(pipeline);

    if ((current_underflow_count != sac_mute_on_underflow_instance->_internal.underflow_count) &&
        (current_underflow_count != 0)) {
        /* Underflow count changed but was not reset. */
        sac_mute_on_underflow_instance->_internal.counter = sac_mute_on_underflow_instance->reload_value;
    }

    sac_mute_on_underflow_instance->_internal.underflow_count = current_underflow_count;

    if (sac_mute_on_underflow_instance->_internal.counter > 0) {
        /* Mute the packet. */
        return_size = size;
        memset(data_out, 0, return_size);
        sac_mute_on_underflow_instance->_internal.counter--;
    }

    return return_size;
}
