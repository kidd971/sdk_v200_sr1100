/** @file  wps_callback.h
 *  @brief The WPS callback module handles the callback queue of the Wireless Protocol Stack.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef WPS_CALLBACK_H_
#define WPS_CALLBACK_H_

#ifdef __cplusplus
extern "C" {
#endif

/* INCLUDES *******************************************************************/
#include "wps_def.h"
#include "xlayer.h"

/* TYPES **********************************************************************/
/** @brief WPS callback instance structure.
 */
typedef struct wps_callback_inst {
    /*! WPS callback function */
    wps_callback_t func;
    /*! WPS callback void pointer argument */
    void *parg;
} wps_callback_inst_t;

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Enqueue a new callback to process at the end of the wps process.
 *
 *  @param[in] queue            Callback queue instance.
 *  @param[in] xlayer_callback  Callback source.
 */
void wps_callback_enqueue(circular_queue_t *queue, xlayer_callback_t *xlayer_callback);

#ifdef __cplusplus
}
#endif

#endif /* WPS_CALLBACK_H_ */
