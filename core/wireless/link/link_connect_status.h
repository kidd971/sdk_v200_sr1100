/** @file link_connect_status.h
 *  @brief Link connection status module.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef LINK_CONNECT_STATUS_H_
#define LINK_CONNECT_STATUS_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "link_lqi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
typedef enum connect_status {
    /*! Connected status */
    CONNECT_STATUS_CONNECTED,
    /*! Disconnected status */
    CONNECT_STATUS_DISCONNECTED
} connect_status_t;

typedef struct connect_status_cfg {
    /*! Number of consecutive recieved frame before the status is changed to connected */
    uint8_t connect_count;
    /*! Number of consecutive lost frame before the status is changed to disconnected */
    uint8_t disconnect_count;
} connect_status_cfg_t;

typedef struct link_connect_status {
    /*! Number of consecutive received frame before the status is changed to connected */
    uint8_t connect_count;
    /*! Number of consecutive lost frame before the status is changed to disconnected */
    uint8_t disconnect_count;
    /*! Current consecutive Received frame */
    uint8_t received_count;
    /*! Current consecutive lost frame */
    uint8_t lost_count;
    /*! Current connection status */
    connect_status_t status;
} link_connect_status_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the link connection status module.
 *
 *  @param[in] link_connect_status  link connection status module instance.
 *  @param[in] cfg                  link connection status module configuration.
 */
void link_connect_status_init(link_connect_status_t *link_connect_status, connect_status_cfg_t *cfg);

/** @brief Update the link connection status module.
 *
 *  @param[in] link_connect_status link connection status module instance.
 *  @param[in] frame_outcome       Frame outcome.
 *  @param[in] ack_enabled         Acknowledge enabled.
 *  @return True if the connection status changed.
 */
bool link_update_connect_status(link_connect_status_t *link_connect_status, frame_outcome_t frame_outcome,
                                bool sync_status, bool ack_enabled);

#ifdef __cplusplus
}
#endif
#endif /* LINK_CONNECT_STATUS_H_ */
