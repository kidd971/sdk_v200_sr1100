/** @file  wps_debug.h
 *  @brief WPS debugging feature.
 *
 *  Setup raw registers value for experimental radio features.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is confidential and proprietary.
 *  @author    SPARK FW Team.
 */
#ifndef WPS_DEBUG_H_
#define WPS_DEBUG_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "sr_calib.h"
#include "wps_phy.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Internal WPS debug feature.
 */
typedef struct phy_debug_cfg wps_debug_feature_t;

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Enable debug feature for the WPS PHY layer.
 *
 *  @note Refer to the internal structure locate in wps_phy_def.h
 *        phy_debug_cfg_t to set internal register.
 *
 *  @note Currently not use for SR1020
 *
 *  @param[in]  wps_debug  Debugging feature raw register value.
 *  @param[out] wps_phy    WPS phy instance.
 */
void wps_enable_debug_feature(wps_debug_feature_t *wps_debug, wps_phy_t *wps_phy);

#ifdef __cplusplus
}
#endif
#endif /* WPS_DEBUG_H_ */
