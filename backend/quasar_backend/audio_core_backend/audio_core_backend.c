/** @file  quasar_backend.c
 *  @brief Implement sac_hal_facade facade prototype functions.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
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

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static uint16_t ep_max98091_action_produce(void *instance, uint8_t *samples, uint16_t size);
static void ep_max98091_start_produce(void *instance);
static void ep_max98091_stop_produce(void *instance);
static uint16_t ep_max98091_action_consume(void *instance, uint8_t *samples, uint16_t size);
static void ep_max98091_start_consume(void *instance);
static void ep_max98091_stop_consume(void *instance);

/* PUBLIC FUNCTIONS ***********************************************************/
void sac_facade_hal_init(sac_hal_t *hal)
{
    hal->enter_critical = quasar_it_enter_critical;
    hal->exit_critical = quasar_it_exit_critical;
}

void sac_facade_codec_endpoint_init(sac_endpoint_interface_t *codec_producer_iface,
                                    sac_endpoint_interface_t *codec_consumer_iface)
{
    if (codec_producer_iface != NULL) {
        codec_producer_iface->action = ep_max98091_action_produce;
        codec_producer_iface->start = ep_max98091_start_produce;
        codec_producer_iface->stop = ep_max98091_stop_produce;
    }

    if (codec_consumer_iface != NULL) {
        codec_consumer_iface->action = ep_max98091_action_consume;
        codec_consumer_iface->start = ep_max98091_start_consume;
        codec_consumer_iface->stop = ep_max98091_stop_consume;
    }
}

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

int sac_facade_cdc_format_stats(char *buffer, uint16_t size)
{
    return sac_cdc_pll_format_stats(&cdc_instance, buffer, size);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Produce Endpoint of the audio codec
 *
 *  @param[in]  instance  Endpoint instance (not used).
 *  @param[out] samples   Location to put produced samples.
 *  @param[in]  size      Size of samples to produce in bytes.
 *  @return Number of bytes produced (always 0 since production is delayed).
 */
static uint16_t ep_max98091_action_produce(void *instance, uint8_t *samples, uint16_t size)
{
    (void)instance;

    quasar_audio_sai_read_non_blocking(samples, size);

    return 0;
}

/** @brief Start the endpoint when used as a producer.
 *
 *  @param[in] instance  Endpoint instance (not used).
 */
static void ep_max98091_start_produce(void *instance)
{
    (void)instance;

    quasar_audio_sai_start_read_non_blocking();
}

/** @brief Stop the endpoint when used as a producer.
 *
 *  @param[in] instance  Endpoint instance (not used).
 */
static void ep_max98091_stop_produce(void *instance)
{
    (void)instance;

    quasar_audio_sai_stop_read_non_blocking();
}

/** @brief Consume Endpoint of the audio codec
 *
 *  @param[in] instance  Endpoint instance (not used).
 *  @param[in] samples   Samples to consume.
 *  @param[in] size      Size of samples to consume in bytes.
 *  @return Number of bytes consumed (always 0 since consumption is delayed).
 */
static uint16_t ep_max98091_action_consume(void *instance, uint8_t *samples, uint16_t size)
{
    (void)instance;

    quasar_audio_sai_write_non_blocking(samples, size);

    return 0;
}

/** @brief Start the endpoint when used as a consumer.
 *
 *  @param[in] instance  Endpoint instance (not used).
 */
static void ep_max98091_start_consume(void *instance)
{
    (void)instance;

    quasar_audio_sai_start_write_non_blocking();
}

/** @brief Stop the endpoint when used as a consumer.
 *
 *  @param[in] instance  Endpoint instance (not used).
 */
static void ep_max98091_stop_consume(void *instance)
{
    (void)instance;

    quasar_audio_sai_stop_write_non_blocking();
}
