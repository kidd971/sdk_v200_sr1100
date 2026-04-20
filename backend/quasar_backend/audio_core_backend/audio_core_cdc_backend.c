/** @file  audio_core_cdc_backend.c
 *  @brief Implement sac_hal_facade facade CDC prototype functions.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar.h"
#include "sac_cdc_pll.h"
#include "sac_hal_facade.h"

/* PRIVATE GLOBALS ************************************************************/
static sac_cdc_pll_instance_t cdc_instance;

/* PUBLIC FUNCTIONS ***********************************************************/
sac_processing_t *sac_facade_cdc_processing_init(sac_sample_format_t format, sac_status_t *status)
{
    sac_processing_t *cdc_processing;
    sac_processing_interface_t cdc_iface = {
        .init = sac_cdc_pll_init,
        .ctrl = sac_cdc_pll_ctrl,
        .process = sac_cdc_pll_process,
        .gate = NULL,
    };

    cdc_instance.sample_format = format;
    cdc_instance.cdc_pll_hal.get_fracn = quasar_clock_get_pll2_fracn;
    cdc_instance.cdc_pll_hal.set_fracn = quasar_clock_set_pll2_fracn;
    cdc_instance.cdc_pll_hal.fracn_min_value = QUASAR_PLL2_FRACN_MIN_VALUE;
    cdc_instance.cdc_pll_hal.fracn_max_value = QUASAR_PLL2_FRACN_MAX_VALUE;
    cdc_instance.cdc_pll_hal.fracn_default_value = QUASAR_PLL2_FRACN_DEFAULT_VALUE;

    cdc_processing = sac_processing_stage_init((void *)&cdc_instance, "CDC", cdc_iface, status);

    return cdc_processing;
}

int sac_facade_cdc_format_stats(char *buffer, uint16_t size, sac_status_t *status)
{
    return sac_cdc_pll_format_stats(&cdc_instance, buffer, size, status);
}

void facade_app_audio_cdc_set_target_queue_size(sac_pipeline_t *pipeline, uint8_t queue_size, sac_status_t *status)
{
    sac_cdc_pll_ctrl(&cdc_instance, pipeline, SAC_CDC_PLL_CMD_SET_TARGET_QUEUE_SIZE, queue_size, status);
}
