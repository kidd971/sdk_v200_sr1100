/** @file  pairing_security.h
 *  @brief This file handles the security features for the pairing procedure.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef PAIRING_SECURITY_H_
#define PAIRING_SECURITY_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/*! The default application code if none was provided. */
#define PAIRING_APP_CODE_DEFAULT 0

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the pairing's security features.
 */
void pairing_security_init(void);

/** @brief Set the application code for the current device.
 *
 *  @param[in] app_code  Application code to be set locally.
 */
void pairing_security_set_app_code(uint64_t app_code);

/** @brief Get the application code for the current device.
 *
 *  @return The local application code.
 */
uint64_t pairing_security_get_app_code(void);

/** @brief Compare the local pairing app code with the received app code.
 *
 *  @return True if valid.
 *  @return False if invalid.
 */
bool pairing_security_compare_app_code(uint64_t app_code);

#ifdef __cplusplus
}
#endif

#endif /* PAIRING_SECURITY_H_ */
