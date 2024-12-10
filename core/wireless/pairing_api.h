/** @file  pairing_api.h
 *  @brief The pairing module is used to exchange data between two devices and make them connect with each other.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef PAIRING_API_H_
#define PAIRING_API_H_

/* INCLUDES *******************************************************************/
#include "pairing_def.h"
#include "swc_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Pairing parameters from the application.
 */
typedef struct pairing_cfg {
    /*! Application code to prevent pairing unwanted devices. */
    uint64_t app_code;
    /*! The timeout period in seconds after which the pairing process will stop. */
    uint16_t timeout_sec;
    /*! Optional application callback function pointer. */
    void (*application_callback)(void);
    /*!  A function pointer to the logic initiating context switches.*/
    void (*context_switch_callback)(void);
    /*! Ultra-wideband regulation used for the pairing process. */
    swc_uwb_regulation_t uwb_regulation;
    /*! Memory pool instance from which memory allocation is done. */
    uint8_t *memory_pool;
    /*! Memory pool size in bytes. */
    uint32_t memory_pool_size;
} pairing_cfg_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Start a pairing procedure with the coordinator.
 *
 *  @param[in]  pairing_cfg               Pairing configurations from the application.
 *  @param[out] pairing_assigned_address  Pairing addresses exchanged during the pairing procedure.
 *  @param[in]  discovery_list            List of discovered devices used by the coordinator.
 *  @param[in]  discovery_list_size       The size of the discovery list.
 *  @param[out] pairing_err               Pairing module error code.
 *  @return The pairing event.
 */
pairing_event_t pairing_coordinator_start(pairing_cfg_t *pairing_cfg,
                                          pairing_assigned_address_t *pairing_assigned_address,
                                          pairing_discovery_list_t *discovery_list, uint8_t discovery_list_size,
                                          pairing_error_t *pairing_err);

/** @brief Start a pairing procedure with the node.
 *
 *  @param[in]  pairing_cfg               Pairing configurations from the application.
 *  @param[out] pairing_assigned_address  Pairing addresses exchanged during the pairing procedure.
 *  @param[in]  device_role               Application level device role from the node.
 *  @param[out] pairing_err               Pairing module error code.
 *  @return The pairing event.
 */
pairing_event_t pairing_node_start(pairing_cfg_t *pairing_cfg, pairing_assigned_address_t *pairing_assigned_address,
                                   uint8_t device_role, pairing_error_t *pairing_err);

/** @brief Abort current pairing procedure.
 *
 *  The application must handle the abort event.
 */
void pairing_abort(void);

#ifdef __cplusplus
}
#endif

#endif /* PAIRING_API_H_ */
