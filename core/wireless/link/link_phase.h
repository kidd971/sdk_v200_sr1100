/** @file  link_phase.h
 *  @brief Link phases management module.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef LINK_PHASE_H_
#define LINK_PHASE_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Phases value, last received preamble phase correlation data.
 */
typedef struct phase_info {
    /*! Receiver time waited (MSB) */
    uint8_t rx_waited1;
    /*! Receiver time waited (LSB) */
    uint8_t rx_waited0;
    /*! Phase information #1 */
    int8_t phase1;
    /*! Phase information #2 */
    int8_t phase2;
    /*! Phase information #3 */
    int8_t phase3;
    /*! Phase information #4 */
    int8_t phase4;
} phase_info_t;

/** @brief Phases value, last received preamble phase correlation data.
 */
typedef struct phase_infos {
   /*! Initiator's ranging data */
   phase_info_t local_info;
   /*! Responder's ranging data */
   phase_info_t remote_info;
} phase_infos_t;

/** @brief Link phase structure.
 */
typedef struct link_phase {
    /*! The current ranging data element in queue */
    phase_infos_t *phase_info_buffer;
    /*! The required number of samples */
    uint8_t max_sample_count;
    /*! The accumulated number of samples */
    uint8_t current_sample_count;
    /*! This indicates whether phase data was read or not */
    bool was_phase_read;
} link_phase_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the link phase module.
 *
 *  @param[in] link_phase         Link phase instance.
 *  @param[in] phase_info_buffer  Array of thresholds.
 *  @param[in] max_sample_count   Sample count to accumulate before next queue element.
 */
void link_phase_init(link_phase_t *link_phase, phase_infos_t *phase_info_buffer, uint8_t max_sample_count);

/** @brief Add link phase data.
 *
 *  @param[in] link_phase   Link phase instance.
 *  @param[in] local_info   Local phase information.
 *  @param[in] remote_info  Remote phase information.
 *
 *  @return True if the required sample count has been accumulated.
 */
bool link_phase_add_data(link_phase_t *link_phase, phase_info_t local_info, phase_info_t remote_info);

/** @brief Get metrics array of link phase module.
 *
 *  @param[in] link_phase    Link phase instance.
 *  @param[out] phase_infos  Pointer to phase metrics in queue.
 *
 *  @return Number of sample in the array.
 */
uint8_t link_phase_get_metrics_array(link_phase_t *link_phase, phase_infos_t **phase_infos);

/** @brief Free latest element in queue.
 *
 *  @param[in] link_phase  Link phase instance.
 *
 *  @return False if queue was empty
 */
bool link_phase_done(link_phase_t *link_phase);

#ifdef __cplusplus
}
#endif

#endif /* LINK_PHASE_H_ */
