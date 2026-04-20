/** @file  audio_core_tinyusb_backend.c
 *  @brief Implement sac_hal_facade facade prototype functions for the Tinyusb USB Audio interface.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sac_hal_facade.h"
#include "tinyusb_audio.h"

/* PUBLIC FUNCTION PROTOTYPES ************************************************/
static uint16_t ep_tusb_action_produce(void *instance, uint8_t *samples, uint16_t size);
static uint16_t ep_tusb_action_consume(void *instance, uint8_t *samples, uint16_t size);
static void ep_tusb_start(void *instance);
static void ep_tusb_stop(void *instance);

/* PUBLIC FUNCTIONS ***********************************************************/
void sac_facade_audio_endpoint_init(sac_endpoint_interface_t *usb_producer_iface,
                                    sac_endpoint_interface_t *usb_consumer_iface)
{
    if (usb_producer_iface != NULL) {
        usb_producer_iface->action = ep_tusb_action_produce;
        usb_producer_iface->start = ep_tusb_start;
        usb_producer_iface->stop = ep_tusb_stop;
    }

    if (usb_consumer_iface != NULL) {
        usb_consumer_iface->action = ep_tusb_action_consume;
        usb_consumer_iface->start = ep_tusb_start;
        usb_consumer_iface->stop = ep_tusb_stop;
    }
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Produce Endpoint of UAC2.
 *
 *  @param[in]  instance  Endpoint instance.
 *  @param[out] samples   Produced samples.
 *  @param[in]  size      Size of samples to produce in bytes.
 *  @return Number of bytes produced.
 */
static uint16_t ep_tusb_action_produce(void *instance, uint8_t *samples, uint16_t size)
{
    (void)instance;

    return tinyusb_audio_read(samples, size);
}

/** @brief Consume Endpoint of UAC2.
 *
 *  @param[in] instance  Endpoint instance.
 *  @param[in] samples   Samples to consume.
 *  @param[in] size      Size of samples to consume in bytes.
 *  @return Number of bytes consumed.
 */
static uint16_t ep_tusb_action_consume(void *instance, uint8_t *samples, uint16_t size)
{
    (void)instance;

    return tinyusb_audio_write(samples, size);
}

/** @brief Start the endpoint.
 *
 *  @param[in] instance  Endpoint instance.
 */
static void ep_tusb_start(void *instance)
{
    (void)instance;

    tinyusb_audio_start_spk();
}

/** @brief Stop the endpoint.
 *
 *  @param[in] instance  Endpoint instance.
 */
static void ep_tusb_stop(void *instance)
{
    (void)instance;

    tinyusb_audio_stop_spk();
}
