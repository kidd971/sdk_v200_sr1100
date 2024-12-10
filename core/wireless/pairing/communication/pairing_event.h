/** @file  pairing_event.h
 *  @brief This file handles the pairing events returned to the application.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef PAIRING_EVENT_H_
#define PAIRING_EVENT_H_

/* INCLUDES *******************************************************************/
#include "pairing_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the pairing events.
 */
void pairing_event_init(void);

/** @brief Set the pairing event.
 *
 *  @param[in] pairing_event  The pairing event to be set.
 */
void pairing_event_set_event(pairing_event_t pairing_event);

/** @brief Get the last pairing event.
 *
 *  @return The last pairing event.
 */
pairing_event_t pairing_event_get_event(void);

#ifdef __cplusplus
}
#endif

#endif /* PAIRING_EVENT_H_ */
