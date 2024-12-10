/** @file  wps_debug.c
 *  @brief WPS debugging feature for SR1120
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is confidential and proprietary.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "wps_debug.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void wps_enable_debug_feature(wps_debug_feature_t *wps_debug, wps_phy_t *wps_phy)
{
    wps_phy_enable_debug_feature(wps_phy, wps_debug);
}
