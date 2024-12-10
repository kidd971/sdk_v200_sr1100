/** @file  sac_endpoint_swc.h
 *
 *  @brief Wireless Core audio endpoint initialization.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_ENDPOINT_SWC_H_
#define SAC_ENDPOINT_SWC_H_

/* INCLUDES *******************************************************************/
#include "sac_api.h"
#include "swc_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief SPARK Wireless Core endpoint instance.
 */
typedef struct ep_swc_instance {
    /*! Wireless connection to use when producing or consuming */
    swc_connection_t *connection;
} ep_swc_instance_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize Wireless Core audio endpoint.
 *
 *  @param[out] swc_producer_iface  Wireless Core producer audio endpoint interface.
 *  @param[out] swc_consumer_iface  Wireless Core consumer audio endpoint interface.
 */
void sac_endpoint_swc_init(sac_endpoint_interface_t *swc_producer_iface,
                           sac_endpoint_interface_t *swc_consumer_iface);

#ifdef __cplusplus
}
#endif

#endif /* SAC_ENDPOINT_SWC_H_ */
