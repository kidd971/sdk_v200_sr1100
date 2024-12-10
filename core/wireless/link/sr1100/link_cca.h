/** @file link_cca.h
 *  @brief Clear Channel Assessment module.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is confidential and proprietary.
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef LINK_CCA_H_
#define LINK_CCA_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief CCA fail action.
 */
typedef enum cca_fail_action {
    /*! Transmit anyway */
    CCA_FAIL_ACTION_TX = 0,
    /*! Abort transmission */
    CCA_FAIL_ACTION_ABORT_TX = 1
} cca_fail_action_t;

/** @brief Clear channel assessment information.
 */
typedef struct {
    /*! Clear channel threshold, valid values are between 0 and 47 */
    uint8_t threshold;
    /*! Maximum number of failed CCA tries before taking the configured fail action */
    uint8_t max_try_count;
    /*! Action to take when all tries failed */
    cca_fail_action_t fail_action;
    /*! CCA retry time in PLL cycles */
    uint16_t retry_time_pll_cycles;
    /*! CCA ON time in PLL cycles */
    uint16_t on_time_pll_cycles;
    /*! Fallback try count array */
    const uint8_t *fbk_try_count;
    /*! Enable feature */
    bool enable;
} link_cca_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize CCA object.
 *
 *  @param[in] cca                    CCA object.
 *  @param[in] threshold              CCA threshold.
 *  @param[in] retry_time_pll_cycles  CCA retry time.
 *  @param[in] on_time_pll_cycles     CCA On time.
 *  @param[in] max_try_count          CCA max try count.
 *  @param[in] fail_action            CCA fail action.
 *  @param[in] enable                 CCA enable flag.
 */
void link_cca_init(link_cca_t *cca, uint8_t threshold, uint16_t retry_time_pll_cycles, uint16_t on_time_pll_cycles,
                   uint8_t max_try_count, cca_fail_action_t fail_action, bool enable);

/** @brief Set CCA fallback try count array.
 *
 * @param[in] cca           CCA object.
 * @param[in] fbk_try_count Fallback try count array.
 * @param[in] fallback_size Fallback array size.
 */
void link_cca_set_fbk_try_count(link_cca_t *cca, uint8_t *fbk_try_count, size_t fallback_size);

/** @brief Get CCA on time.
 *
 *  @param[in] cca  CCA object.
 *  @return  CCA on time.
 */
uint16_t link_cca_get_on_time(link_cca_t *cca);

#ifdef __cplusplus
}
#endif
#endif /* LINK_CCA_H_ */
