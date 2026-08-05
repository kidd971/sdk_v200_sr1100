/** @file  pairing_cfg.h
 *  @brief Configuration constants for the SPARK Wireless Core Pairing Module.
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
#define PAIRING_APP_CODE 0x0000000000000999

/** @brief The timeout in seconds after which the pairing procedure will abort.
 *
 *  Both ends have to be inside their windows at the same time -- UWB is the only channel
 *  between HS and DG, so with the link down neither can tell the other to re-pair -- and
 *  the user triggers them one after the other. That serialisation is what the value has to
 *  cover, not the handshake, which is sub-second once both sides are listening:
 *
 *      t=0    start the hold on the first end
 *      t=3    it enters pairing        -> window [3, 3+T]
 *      t=4-5  hands move, hold begins on the second end
 *      t=7-8  it enters pairing        -> window [7-8, 7-8+T]
 *      overlap = T - 5 or so
 *
 *  So T must clear ~5 s just to overlap at all, and wants a few more to be usable. 12 gives
 *  ~7 s of overlap, which is comfortable on the bench without making a mistimed attempt
 *  expensive to wait out -- there is no AT abort, only the pairing button.
 *
 *  SHIPPING WANTS 30. An end user is wearing the headset, reading instructions and fumbling
 *  for a second button; 12 s is a bench number, not a consumer one. Bumping it here is the
 *  whole change, or override it per-preset (-DPAIRING_TIMEOUT_IN_SECONDS=30) without
 *  touching this file.
 */
#ifndef PAIRING_TIMEOUT_IN_SECONDS
#define PAIRING_TIMEOUT_IN_SECONDS 12
#endif

#endif /* PAIRING_CFG_H_ */
