/** @file  pairing_error.h
 *  @brief This file provides helper functions to manage the error state of the pairing module.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef PAIRING_ERROR_H_
#define PAIRING_ERROR_H_

/* INCLUDES *******************************************************************/
#include "pairing_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the pairing error module.
 *
 *  @param[in] pairing_error  The pairing error handle.
 */
void pairing_error_init(pairing_error_t *pairing_error);

/** @brief Set the pairing error.
 *
 *  @param[in] pairing_error  The pairing error.
 */
void pairing_error_set_error(pairing_error_t pairing_error);

/** @brief Get the pairing error.
 *
 *  @return The pairing error.
 */
pairing_error_t pairing_error_get_error(void);

#ifdef __cplusplus
}
#endif

#endif /* PAIRING_ERROR_H_ */
