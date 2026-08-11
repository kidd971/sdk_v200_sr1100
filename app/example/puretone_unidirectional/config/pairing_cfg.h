/** @file  pairing_cfg.h
 *  @brief Configuration constants for the SPARK Wireless Core Pairring Module.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef PAIRING_CFG_H_
#define PAIRING_CFG_H_

/* CONSTANTS ******************************************************************/
/* The device roles are used for the pairing discovery list. */
#define PAIRING_DEVICE_ROLE_COORDINATOR 0
#define PAIRING_DEVICE_ROLE_NODE        1
/* The discovery list includes the Coordinator and the Node. */
#define PAIRING_DISCOVERY_LIST_SIZE 2

/* The application code prevents unwanted devices from pairing with this application. */
#define PAIRING_APP_CODE 0x0000000000000222
/* The timeout in seconds after which the pairing procedure will abort. */
#define PAIRING_TIMEOUT_IN_SECONDS 10

#endif /* PAIRING_CFG_H_ */
