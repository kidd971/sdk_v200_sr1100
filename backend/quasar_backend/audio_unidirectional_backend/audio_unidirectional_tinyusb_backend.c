/** @file  audio_unidirectional_tinyusb_backend.c
 *  @brief Implement audio unidrectional facade prototype functions for tinyusb USB Audio.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "audio_unidirectional_facade.h"
#include "sac_cfg.h"
#include "tinyusb_audio.h"
#include "tusb.h"

/* PRIVATE GLOBALS ************************************************************/
static uint16_t app_expected_rx_payload_size;

/* PUBLIC FUNCTIONS ***********************************************************/
void facade_audio_coord_init(void)
{
    /* Nothing to initialize. */
}

void facade_audio_node_init(void)
{
    /* Nothing to initialize. */
}

void facade_configure_coord_usb_audio(void)
{
    /* Configure the payload size that the application expects to receive. */
    app_expected_rx_payload_size = MAIN_CHANNEL_SWC_PAYLOAD_SIZE;

    /* Configure main channel interface. */
    tud_audio_config_t audio_cfg = {
        .sampling_rate = MAIN_CHANNEL_SAMPLE_RATE_HZ,
        .bytes_per_sample = MAIN_CHANNEL_BIT_DEPTH / 8,
        .resolution = MAIN_CHANNEL_BIT_DEPTH,
        .nb_ch = MAIN_CHANNEL_CHANNEL_COUNT,
    };

    /* Set unique PID to avoid conflicts between configurations. */
    tusb_desc_device_t *desc_device = (tusb_desc_device_t *)tud_descriptor_device_cb();

    desc_device->idProduct = 0xFF03;
    tud_audio_set_epout_cfg(audio_cfg);
}

void facade_configure_node_usb_audio(void)
{
    /* Configure main channel interface. */
    tud_audio_config_t audio_cfg = {
        .sampling_rate = MAIN_CHANNEL_SAMPLE_RATE_HZ,
        .bytes_per_sample = MAIN_CHANNEL_BIT_DEPTH / 8,
        .resolution = MAIN_CHANNEL_BIT_DEPTH,
        .nb_ch = MAIN_CHANNEL_CHANNEL_COUNT,
    };

    /* Set unique PID to avoid conflicts between configurations. */
    tusb_desc_device_t *desc_device = (tusb_desc_device_t *)tud_descriptor_device_cb();

    desc_device->idProduct = 0xFF04;
    tud_audio_set_epin_cfg(audio_cfg);

    tinyusb_audio_set_ep_in_target_fifo_size(MAIN_CHANNEL_SWC_PAYLOAD_SIZE * MAIN_CHANNEL_LATENCY_QUEUE_SIZE);
}

void facade_audio_deinit(void)
{
    /* Nothing to deinitialize. */
}

void facade_set_audio_complete_callback(void (*tx_callback)(void), void (*rx_callback)(void))
{
    /* TX Callback is not required. */
    (void)tx_callback;

    if (rx_callback == NULL) {
        /* No callback to setup. */
        return;
    }

    tinyusb_audio_set_speaker_rx_callback(rx_callback, app_expected_rx_payload_size);
}

uint32_t facade_get_node_usb_audio_tx_fifo_sample_count(void)
{
    /* Get the number of samples contained in the EPIN fifo. */
    return SAC_CALCULATE_SAMPLE_COUNT(tinyusb_audio_get_epin_fifo_size(), MAIN_CHANNEL_CHANNEL_COUNT,
                                      MAIN_CHANNEL_BIT_DEPTH / 8);
}
