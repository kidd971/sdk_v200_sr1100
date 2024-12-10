/** @file  link_phase.c
 *  @brief Link phases management module.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "link_phase.h"

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
void link_phase_init(link_phase_t *link_phase, phase_infos_t *phase_info_buffer, uint8_t max_sample_count)
{
    link_phase->current_sample_count = 0;
    link_phase->max_sample_count = max_sample_count;
    link_phase->phase_info_buffer = phase_info_buffer;
    link_phase->was_phase_read = true;
}

bool link_phase_add_data(link_phase_t *link_phase, phase_info_t local_info, phase_info_t remote_info)
{
    bool is_data_ready = false;

    if (!link_phase->was_phase_read) {
        is_data_ready = true;
        return is_data_ready;
    }

    link_phase->phase_info_buffer[link_phase->current_sample_count].local_info = local_info;
    link_phase->phase_info_buffer[link_phase->current_sample_count].remote_info = remote_info;
    link_phase->current_sample_count++;

    if (link_phase->current_sample_count >= link_phase->max_sample_count) {
        is_data_ready = true;
        link_phase->was_phase_read = false;
    }

    return is_data_ready;
}

uint8_t link_phase_get_metrics_array(link_phase_t *link_phase, phase_infos_t **phase_infos)
{
    uint8_t size = 0;

    if (link_phase->current_sample_count >= link_phase->max_sample_count) {
        *phase_infos = link_phase->phase_info_buffer;
        size = link_phase->max_sample_count;
    } else {
        *phase_infos = NULL;
    }

    return size;
}

bool link_phase_done(link_phase_t *link_phase)
{
    link_phase->was_phase_read = true;
    link_phase->current_sample_count  = 0;

    return link_phase->was_phase_read;
}
