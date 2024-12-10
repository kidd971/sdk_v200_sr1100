/** @file  sac_endpoint_swc.c
 *  @brief Implement Wireless Core audio endpoint initialization functions.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sac_endpoint_swc.h"

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static uint16_t ep_swc_action_produce(void *instance, uint8_t *samples, uint16_t size);
static uint16_t ep_swc_action_consume(void *instance, uint8_t *samples, uint16_t size);
static void ep_swc_start(void *instance);
static void ep_swc_stop(void *instance);

/* PUBLIC FUNCTIONS ***********************************************************/
void sac_endpoint_swc_init(sac_endpoint_interface_t *swc_producer_iface,
                           sac_endpoint_interface_t *swc_consumer_iface)
{
    if (swc_producer_iface != NULL) {
        swc_producer_iface->action = ep_swc_action_produce;
        swc_producer_iface->start = ep_swc_start;
        swc_producer_iface->stop = ep_swc_stop;
    }

    if (swc_consumer_iface != NULL) {
        swc_consumer_iface->action = ep_swc_action_consume;
        swc_consumer_iface->start = ep_swc_start;
        swc_consumer_iface->stop = ep_swc_stop;
    }
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Produce Endpoint of the SPARK Wireless Core.
 *
 *  @param[in]  instance  Endpoint instance.
 *  @param[out] samples   Produced samples.
 *  @param[in]  size      Size of samples to produce in bytes.
 *  @return Number of bytes produced.
 */
static uint16_t ep_swc_action_produce(void *instance, uint8_t *samples, uint16_t size)
{
    uint8_t *payload = NULL;
    uint8_t payload_size;
    swc_error_t err;
    ep_swc_instance_t *inst = (ep_swc_instance_t *)instance;
    (void)size;

    payload_size = swc_connection_receive(inst->connection, &payload, &err);

    memcpy(samples, payload, payload_size);
    swc_connection_receive_complete(inst->connection, &err);

    return payload_size;
}

/** @brief Consume Endpoint of the SPARK Wireless Core.
 *
 *  @param[in] instance  Endpoint instance.
 *  @param[in] samples   Samples to consume.
 *  @param[in] size      Size of samples to consume in bytes.
 *  @return Number of bytes consumed.
 */
static uint16_t ep_swc_action_consume(void *instance, uint8_t *samples, uint16_t size)
{
    uint8_t *buf;
    swc_error_t err;
    ep_swc_instance_t *inst = (ep_swc_instance_t *)instance;

    /** When the fallback is activated, and the payload size is smaller, the variable allocation of memory causes
     *  fragmentation in the queue of the wireless.
     */
    swc_connection_get_payload_buffer(inst->connection, &buf, &err);
    if (buf != NULL) {
        memcpy(buf, samples, size);
        swc_connection_send(inst->connection, buf, size, &err);
        return size;
    } else {
        return 0;
    }
}

/** @brief Start the endpoint.
 *
 *  @param[in] instance  Endpoint instance.
 */
static void ep_swc_start(void *instance)
{
    (void)instance;
}

/** @brief Stop the endpoint.
 *
 *  @param[in] instance  Endpoint instance.
 */
static void ep_swc_stop(void *instance)
{
    (void)instance;
}
