/** @file link_scheduler.c
 *  @brief Scheduler module.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "link_scheduler.h"
#include <string.h>
#include "link_utils.h"

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static inline bool time_slot_is_empty(scheduler_t *scheduler, timeslot_t *time_slot);

/* PUBLIC FUNCTIONS ***********************************************************/
void link_scheduler_init(scheduler_t *scheduler, uint16_t local_addr)
{
    // Do not modify scheduler->schedule as it is already initialized prior to calling wps_init().
    scheduler->current_time_slot_num = 0;
    scheduler->sleep_cycles = 0;
    scheduler->local_addr = local_addr;
    scheduler->tx_disabled = false;
    scheduler->timeslot_mismatch = false;
}

void link_scheduler_reset(scheduler_t *scheduler)
{
    memset(scheduler->schedule.timeslot, 0, scheduler->schedule.size * sizeof(timeslot_t));
    scheduler->schedule.size = 0;
    scheduler->current_time_slot_num = 0;
    scheduler->sleep_cycles          = 0;
    scheduler->tx_disabled           = false;
}

uint8_t link_scheduler_increment_time_slot(scheduler_t *scheduler)
{
    uint8_t inc_count = 0;

    scheduler->timeslot_mismatch = false;

    if (scheduler->schedule.size != 0) {
        uint8_t i = scheduler->current_time_slot_num;

        scheduler->current_sleep_lvl = scheduler->schedule.timeslot[i].sleep_lvl;
        do {
            scheduler->sleep_cycles += scheduler->schedule.timeslot[i].duration_pll_cycles;
            i = (i + 1) % scheduler->schedule.size;
            inc_count++;
        } while (time_slot_is_empty(scheduler, &scheduler->schedule.timeslot[i]));

        scheduler->current_time_slot_num = i;
        scheduler->next_sleep_lvl = scheduler->schedule.timeslot[i].sleep_lvl;
    }

    return inc_count;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Get time slot empty flag.
 *
 *  @param[in]  scheduler  Scheduler object.
 *  @param[in]  time_slot  Time slot.
 */
static inline bool time_slot_is_empty(scheduler_t *scheduler, timeslot_t *time_slot)
{
    if (time_slot->connection_main[0] == NULL) {
        return true;
    }

    if ((scheduler->tx_disabled) && (time_slot->connection_main[0]->source_address == scheduler->local_addr)) {
        return true;
    }

    return false;
}
