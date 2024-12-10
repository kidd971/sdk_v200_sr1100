/** @file link_ddcm.h
 *  @brief Distributed De-syncronization Concurrency Mechanism.
 *
 *  This algorithm is used for link concurrency to drift the schedule in a slot
 *  where there is less CCA fails, thus optimizing the air time usage.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef LINK_DDCM_H_
#define LINK_DDCM_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
#define DDCM_DISABLE                0
#define UNSYNC_TX_OFFSET_PLL_CYCLES 1024

/* TYPES **********************************************************************/
/** @brief TX offset module instance.
 */
typedef struct link_ddcm {
    /*! Current target offset value in PLL cycle. */
    uint16_t target_offset;
    /*! Maximum target offset to be applied on a timeslot in PLL cycle. */
    uint16_t max_timeslot_offset;
    /*! PLL cycles elapsed since the last post TX update. */
    uint32_t pll_cycles_since_tx;
    /*! Number of pll cycles since sync was lost. */
    uint32_t sync_loss_duration_pll;
    /*! Maximum sync lost pll cycles before applying unsync tx offset. */
    uint32_t sync_loss_max_duration_pll;
    /*! TX offset enable flag. */
    bool enabled;
    /*! True if the last transmission was successful. */
    bool last_tx_successful;
} link_ddcm_t;

/* PUBLIC GLOBALS *************************************************************/
/** @brief Initialize the distributed desync module.
 *
 *  @note When the distributed desync module is disabled, every call to the link_ddcm_get_offset
 *        will return 0.
 *
 *  @param[in] instance                    Distributed desync module instance.
 *  @param[in] max_timeslot_offset         Maximum offset to apply every timeslot in pll cycles.
 *                                         Set to DDCM_DISABLE to disable the module.
 *  @param[in] sync_loss_max_duration_pll  Maximum sync lost pll cycles before applying unsync tx
 *                                         offset.
 */
static inline void link_ddcm_init(link_ddcm_t *instance, uint16_t max_timeslot_offset,
                                  uint32_t sync_loss_max_duration_pll)
{
    if (instance == NULL) {
        return;
    }

    instance->target_offset              = 0;
    instance->max_timeslot_offset        = max_timeslot_offset;
    instance->enabled                    = (max_timeslot_offset != DDCM_DISABLE);
    instance->sync_loss_max_duration_pll = sync_loss_max_duration_pll;
    instance->pll_cycles_since_tx        = 0;
    instance->sync_loss_duration_pll     = 0;
    instance->last_tx_successful         = false;
}

/** @brief Update the PLL cycles elapsed since the last post TX update.
 *
 *  @param[in] instance    Distributed desync module instance.
 *  @param[in] pll_cycles  Number of pll cycles of the current timeslot.
 */
static inline void link_ddcm_pll_cycles_update(link_ddcm_t *instance, uint32_t pll_cycles)
{
    if ((instance == NULL) || !instance->enabled) {
        return;
    }

    /* Keep track of timeslots duration. */
    instance->pll_cycles_since_tx += pll_cycles;
}

/** @brief Update the distributed desync instance after a transmission.
 *
 *  @param[in] instance             Distributed desync module instance.
 *  @param[in] cca_try_count        Number of CCA failures in the last transmission.
 *  @param[in] cca_retry_time       Delay in pll cycles for each CCA failure.
 *  @param[in] is_tx_event_success  Wether the last transmission was successful.
 */
static inline void link_ddcm_post_tx_update(link_ddcm_t *instance, uint8_t cca_try_count,
                                            uint16_t cca_retry_time, bool is_tx_event_success)
{
    if ((instance == NULL) || !instance->enabled) {
        return;
    }

    if (!is_tx_event_success) {
        instance->sync_loss_duration_pll += instance->pll_cycles_since_tx;
    } else {
        if (instance->sync_loss_duration_pll > instance->pll_cycles_since_tx) {
            instance->sync_loss_duration_pll -= instance->pll_cycles_since_tx;
        } else {
            instance->sync_loss_duration_pll = 0;
        }
        if (instance->target_offset == 0) {
            /* Update the target offset once the previous target was achieved. */
            if (cca_try_count > 0) {
                instance->target_offset = (cca_try_count - 1) * cca_retry_time +
                                          instance->max_timeslot_offset;
            } else {
                instance->target_offset = 0;
            }
        }
    }
    instance->pll_cycles_since_tx = 0;
    instance->last_tx_successful  = is_tx_event_success;
}

/** @brief Get the distributed desync offset to apply to the current timeslot.
 *
 *  @param[in] instance  Distributed desync module instance.
 *  @return Offset in pll cycles to be applied.
 */
static inline uint16_t link_ddcm_get_offset(link_ddcm_t *instance)
{
    uint16_t timeslot_offset;

    if ((instance == NULL) || !instance->enabled) {
        return 0;
    }

    /* Previous transmission was unsuccessful. */
    if (instance->sync_loss_duration_pll >= instance->sync_loss_max_duration_pll) {
        /* Apply a bigger offset when unable to transmit to try to find a free air time slot. */
        instance->sync_loss_duration_pll = 0;
        instance->target_offset          = 0;
        return UNSYNC_TX_OFFSET_PLL_CYCLES;
    }

    if (!instance->last_tx_successful) {
        /* Do not drift if link is lost. */
        return 0;
    }

    if (instance->target_offset > instance->max_timeslot_offset) {
        /* Use the maximum offset. */
        timeslot_offset = instance->max_timeslot_offset;
    } else {
        /* Use the remaining offset. */
        timeslot_offset = instance->target_offset;
    }
    /* Update the target offset. */
    instance->target_offset -= timeslot_offset;

    return timeslot_offset;
}

#ifdef __cplusplus
}
#endif
#endif /* LINK_DDCM_H_ */
