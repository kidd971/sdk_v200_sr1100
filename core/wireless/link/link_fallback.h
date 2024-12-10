/** @file  link_fallback.h
 *  @brief Link module to handle dynamic settings based on the payload size.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef LINK_FALLBACK_H_
#define LINK_FALLBACK_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Link fallback module structure.
 */
typedef struct link_fallback {
    /*! Array of fallback threshold, in payload size */
    const uint8_t *threshold;
    /*! Number of fallback threshold */
    uint8_t threshold_count;
} link_fallback_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the link fallback module.
 *
 *  @param[in] link_fallback    Link fallback instance.
 *  @param[in] threshold        Array of thresholds.
 *  @param[in] threshold_count  Number of thresholds.
 */
void link_fallback_init(link_fallback_t *const link_fallback, const uint8_t *const threshold, uint8_t threshold_count);

/** @brief Get the current fallback channel index based on the payload size.
 *
 *  @param[in]  link_fallback  Link fallback instance.
 *  @param[in]  payload_size   Current payload size.
 *  @param[out] index          Fallback channel index, NULL if fallback is not active.
 *
 *  @return True if fallback is active, False otherwise.
 */
bool link_fallback_get_index(link_fallback_t *link_fallback, uint8_t payload_size, uint8_t *index);

#ifdef __cplusplus
}
#endif

#endif /* LINK_FALLBACK_H_ */
