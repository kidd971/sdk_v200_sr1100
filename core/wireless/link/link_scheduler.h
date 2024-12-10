/** @file link_scheduler.h
 *  @brief Scheduler module.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef LINK_SCHEDULER_H_
#define LINK_SCHEDULER_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "wps_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Timeslot instance.
 */
typedef struct timeslot {
    /*! Main connection instance. */
    wps_connection_t *connection_main[WPS_MAX_CONN_PER_TIMESLOT];
    /*! Auto-reply connection instance. */
    wps_connection_t *connection_auto_reply[WPS_MAX_CONN_PER_TIMESLOT];
    /*! Timeslot duration, in PLL cycles. */
    uint32_t duration_pll_cycles;
    /*! Timeslot priority for main connection instances. */
    uint8_t connection_main_priority[WPS_MAX_CONN_PER_TIMESLOT];
    /*! Timeslot priority for auto-reply connection instances. */
    uint8_t connection_auto_priority[WPS_MAX_CONN_PER_TIMESLOT];
    /*! Number of main connections on this time slot. */
    uint8_t main_connection_count;
    /*! Number of auto reply connections on this time slot. */
    uint8_t auto_connection_count;
    /*! Last used connection on this time slot. */
    uint8_t last_used_main_connection;
    /*! Sleep level for this time slot. */
    sleep_lvl_t sleep_lvl;
} timeslot_t;

/** @brief Schedule instance.
 */
typedef struct schedule {
    /*! Array containing every schedule timeslot. */
    timeslot_t *timeslot;
    /*! Number of timeslot for the schedule. */
    uint32_t size;
    /*! Current sleep level */
    sleep_lvl_t lightest_sleep_lvl;
} schedule_t;

typedef struct scheduler {
    /*! The schedule */
    schedule_t schedule;
    /*! Current time slot number */
    uint8_t current_time_slot_num;
    /*! Current sleep level */
    sleep_lvl_t current_sleep_lvl;
    /*! Next sleep level */
    sleep_lvl_t next_sleep_lvl;
    /*! Sleep time in PLL cycles */
    uint32_t sleep_cycles;
    /*! Local Address. */
    uint16_t local_addr;
    /*! TX disabled flag. */
    bool tx_disabled;
    /*! Timeslot mismatch index flag. */
    bool timeslot_mismatch;
} scheduler_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize scheduler object.
 *
 *  @param[in] scheduler  Scheduler object.
 *  @param[in] local_addr Local address.
 *  @return None.
 */
void link_scheduler_init(scheduler_t *scheduler, uint16_t local_addr);

/** @brief Reset scheduler object.
 *
 *  @param[in] scheduler  Scheduler object.
 */
void link_scheduler_reset(scheduler_t *scheduler);

/** @brief Add a time slot to schedule.
 *
 *  @note Sleep cycle is computed based on number of
 *        timeslot increment. This function do not reset
 *        the sleep cycle. When computing a new time, user
 *        should always call link_scheduler_reset_sleep_time
 *        in order to reset the sleep_cycle computation.
 *
 *  @param[in] scheduler  Scheduler object.
 *  @return The number of timeslot incremented in the schedule.
 */
uint8_t link_scheduler_increment_time_slot(scheduler_t *scheduler);

/** @brief Set current time slot index.
 *
 *  @param[in] scheduler    Scheduler object.
 *  @param[in] time_slot_i  Time slot index.
 */
static inline void link_scheduler_set_time_slot_i(scheduler_t *scheduler, uint8_t time_slot_i)
{
    scheduler->current_time_slot_num = time_slot_i;
}

/** @brief Enable transmissions.
 *
 *  @return The current time slot.
 */
static inline void link_scheduler_enable_tx(scheduler_t *scheduler)
{
    scheduler->tx_disabled = false;
}

/** @brief Disable transmissions.
 *
 *  @param[in]  scheduler  Scheduler object.
 */
static inline void link_scheduler_disable_tx(scheduler_t *scheduler)
{
    scheduler->tx_disabled = true;
}

/** @brief Get the current time slot.
 *
 *  @param[in]  scheduler  Scheduler object.
 *  @return The current time slot.
 */
static inline timeslot_t *link_scheduler_get_current_timeslot(scheduler_t *scheduler)
{
    return &scheduler->schedule.timeslot[scheduler->current_time_slot_num];
}

/** @brief Get the handle of the previous time slot index.
 *
 *  @param[in]  scheduler  Scheduler object.
 *  @return The handle of the previous time slot index.
 */
static inline timeslot_t *link_scheduler_get_previous_timeslot_index(scheduler_t *scheduler)
{
    uint8_t previous_timeslot_num;

    if (scheduler->current_time_slot_num == 0) {
        previous_timeslot_num = scheduler->schedule.size - 1;
    } else {
        previous_timeslot_num = scheduler->current_time_slot_num - 1;
    }

    return &scheduler->schedule.timeslot[previous_timeslot_num];
}

/** @brief Get the current time slot's main connection.
 *
 *  @param[in]  scheduler  Scheduler object.
 *  @param[in]  id         The ID of the desired connection.
 *  @return The current main connection.
 */
static inline wps_connection_t *link_scheduler_get_current_main_connection(scheduler_t *scheduler, uint8_t id)
{
    return scheduler->schedule.timeslot[scheduler->current_time_slot_num].connection_main[id];
}

/** @brief Get the current time slot's auto reply connection.
 *
 *  @param[in]  scheduler  Scheduler object.
 *  @param[in]  id         The ID of the desired connection.
 *  @return The current auto reply connection.
 */
static inline wps_connection_t *link_scheduler_get_current_auto_connection(scheduler_t *scheduler, uint8_t id)
{
    return scheduler->schedule.timeslot[scheduler->current_time_slot_num].connection_auto_reply[id];
}

/** @brief Get the total number of time slots.
 *
 *  @param[in]  scheduler  Scheduler object.
 *  @return The total number of time slots.
 */
static inline uint8_t link_scheduler_get_total_timeslot_count(scheduler_t *scheduler)
{
    return scheduler->schedule.size;
}

/** @brief Get the current time slot index.
 *
 *  @param[in]  scheduler  Scheduler object.
 *  @return The current time slot index.
 */
static inline uint8_t link_scheduler_get_next_timeslot_index(scheduler_t *scheduler)
{
    return scheduler->current_time_slot_num;
}

/** @brief Get sleep the amount of time to sleep in PLL cycles.
 *
 *  @param[in]   scheduler  Scheduler object.
 *  @return time to sleep in PLL cycles.
 */
static inline uint32_t link_scheduler_get_sleep_time(scheduler_t *scheduler)
{
    return scheduler->sleep_cycles;
}

/** @brief Start at the end of the schedule so the first effective time slot is the first one.
 *
 *  @param[in]   scheduler  Scheduler object.
 *  @return time to sleep in PLL cycles.
 */
static inline void link_scheduler_set_first_time_slot(scheduler_t *scheduler)
{
    if (scheduler->schedule.size > 1) {
        link_scheduler_set_time_slot_i(scheduler, scheduler->schedule.size - 1);
    }
}

/** @brief Reset the scheduler compute sleep time.
 *
 *  @note This should be called before incrementing the
 *        timeslot in order to reset the sleep_cycle computation.
 *
 *  @param[in] scheduler  Scheduler object.
 */
static inline void link_scheduler_reset_sleep_time(scheduler_t *scheduler)
{
    scheduler->sleep_cycles = 0;
}

/** @brief  Set mismatch schedule index to true;
 *
 *  @param[in] scheduler  Scheduler object.
 */
static inline void link_scheduler_set_mismatch(scheduler_t *scheduler)
{
    scheduler->timeslot_mismatch = true;
}

/** @brief  Get the mismatch schedule index flag;
 *
 *  @param[in] scheduler  Scheduler object.
 */
static inline bool link_scheduler_get_mismatch(scheduler_t *scheduler)
{
    return scheduler->timeslot_mismatch;
}

#ifdef __cplusplus
}
#endif
#endif /* LINK_SCHEDULER_H_ */
