/** @file  sac_fallback_gate.c
 *  @brief SPARK Audio Core Fallback gate is used to gate a processing stage based on the fallback state.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sac_fallback_gate.h"
#include "sac_fallback.h"
#include "sac_utils.h"

/* PRIVATE FUNCTION PROTOTYPE *************************************************/
static sac_fallback_instance_t *get_fallback_instance(sac_pipeline_t *pipeline, sac_status_t *status);

/* PUBLIC FUNCTIONS ***********************************************************/
bool sac_fallback_gate_is_fallback_on(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in,
                                      uint16_t size, sac_status_t *status)
{
    (void)instance;
    (void)data_in;
    (void)header;
    (void)size;

    sac_fallback_instance_t *inst;
    bool return_error = false; /* If this gate fails, consider fallback OFF. */

    *status = SAC_OK;

    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_NULL_PTR, return return_error);

    inst = get_fallback_instance(pipeline, status);
    if (*status != SAC_OK) {
        return return_error;
    }

    return sac_fallback_is_active(inst, status);
}

bool sac_fallback_gate_is_fallback_off(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in,
                                       uint16_t size, sac_status_t *status)
{
    (void)instance;
    (void)data_in;
    (void)header;
    (void)size;

    sac_fallback_instance_t *inst;
    bool return_error = true; /* If this gate fails, consider fallback OFF. */

    *status = SAC_OK;

    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_NULL_PTR, return return_error);

    inst = get_fallback_instance(pipeline, status);
    if (*status != SAC_OK) {
        return return_error;
    }

    return !sac_fallback_is_active(inst, status);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Get the fallback instance from the processing stage list.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[out] status    Status code.
 *  @return If found returns the fallback processing stage instance.
 */
static sac_fallback_instance_t *get_fallback_instance(sac_pipeline_t *pipeline, sac_status_t *status)
{
    sac_processing_t *current_process;

    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_NULL_PTR, return NULL);
    SAC_CHECK_STATUS(pipeline->process == NULL, status, SAC_ERR_NULL_PTR, return NULL);

    /* Find the fallback processing stage in the chain. */
    current_process = pipeline->process;
    do {
        if ((uint32_t)current_process->iface.process == (uint32_t)sac_fallback_process) {
            return current_process->instance;
        }
        current_process = current_process->next_process;
    } while (current_process != NULL);

    /* Could not find the fallback process in the processing stage list. */
    *status = SAC_ERR_FALLBACK_PROC_NOT_FOUND;

    return NULL;
}
