/** @file  audio_core_i2s_backend.c
 *  @brief Implement sac_hal_facade facade prototype functions for the I2S interface.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar.h"
#include "sac_hal_facade.h"

/* PRIVATE GLOBALS ************************************************************/
#ifdef SINE_DEBUG_CAPTURE
/* Captures what is actually written to the SAI DMA (codec input).
 * 192 entries = 96 stereo frames = one full 1 kHz cycle at 96 kHz. */
volatile int32_t s_node_debug_buf[192];
static uint32_t s_node_debug_idx = 0;
#endif

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static uint16_t ep_i2s_action_produce(void *instance, uint8_t *samples, uint16_t size);
static void ep_i2s_start_produce(void *instance);
static void ep_i2s_stop_produce(void *instance);
static uint16_t ep_i2s_action_consume(void *instance, uint8_t *samples, uint16_t size);
static void ep_i2s_start_consume(void *instance);
static void ep_i2s_stop_consume(void *instance);

/* PUBLIC FUNCTIONS ***********************************************************/
void sac_facade_audio_endpoint_init(sac_endpoint_interface_t *codec_producer_iface,
                                    sac_endpoint_interface_t *codec_consumer_iface)
{
    if (codec_producer_iface != NULL) {
        codec_producer_iface->action = ep_i2s_action_produce;
        codec_producer_iface->start = ep_i2s_start_produce;
        codec_producer_iface->stop = ep_i2s_stop_produce;
    }

    if (codec_consumer_iface != NULL) {
        codec_consumer_iface->action = ep_i2s_action_consume;
        codec_consumer_iface->start = ep_i2s_start_consume;
        codec_consumer_iface->stop = ep_i2s_stop_consume;
    }
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Produce Endpoint of the audio codec
 *
 *  @param[in]  instance  Endpoint instance (not used).
 *  @param[out] samples   Location to put produced samples.
 *  @param[in]  size      Size of samples to produce in bytes.
 *  @return Number of bytes produced (always 0 since production is delayed).
 */
static uint16_t ep_i2s_action_produce(void *instance, uint8_t *samples, uint16_t size)
{
    (void)instance;

    quasar_audio_sai_read_non_blocking(samples, size);

    return 0;
}

/** @brief Start the endpoint when used as a producer.
 *
 *  @param[in] instance  Endpoint instance (not used).
 */
static void ep_i2s_start_produce(void *instance)
{
    (void)instance;

    quasar_audio_sai_start_read_non_blocking();
}

/** @brief Stop the endpoint when used as a producer.
 *
 *  @param[in] instance  Endpoint instance (not used).
 */
static void ep_i2s_stop_produce(void *instance)
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
static uint16_t ep_i2s_action_consume(void *instance, uint8_t *samples, uint16_t size)
{
    (void)instance;

#ifdef SINE_DEBUG_CAPTURE
    {
        int32_t *in = (int32_t *)samples;
        uint32_t count = size / sizeof(int32_t);

        for (uint32_t i = 0; i < count; i++) {
            s_node_debug_buf[s_node_debug_idx] = in[i];
            s_node_debug_idx = (s_node_debug_idx + 1 >= sizeof(s_node_debug_buf) / sizeof(s_node_debug_buf[0])) ? 0 : s_node_debug_idx + 1;
        }
    }
#endif

    quasar_audio_sai_write_non_blocking(samples, size);

    return 0;
}

/** @brief Start the endpoint when used as a consumer.
 *
 *  @param[in] instance  Endpoint instance (not used).
 */
static void ep_i2s_start_consume(void *instance)
{
    (void)instance;

    quasar_audio_sai_start_write_non_blocking();
}

/** @brief Stop the endpoint when used as a consumer.
 *
 *  @param[in] instance  Endpoint instance (not used).
 */
static void ep_i2s_stop_consume(void *instance)
{
    (void)instance;

    quasar_audio_sai_stop_write_non_blocking();
}
