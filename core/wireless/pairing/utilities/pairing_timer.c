/** @file  pairing_timer.c
 *  @brief This file handles the time management for pairing module such as the timeout.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "pairing_timer.h"
#include "swc_hal_facade.h"

/* CONSTANTS ******************************************************************/
#define MS_IN_SEC 1000

/* PRIVATE GLOBALS ************************************************************/
static uint16_t local_timeout_tick_count;
static uint32_t local_initial_timer_tick_count;
static uint32_t local_tick_frequency_hz;

/* PRIVATE FUNCTIONS PROTOTYPE ************************************************/
static uint32_t get_tick_frequency_hz(void);

/* PUBLIC FUNCTIONS ***********************************************************/
void pairing_timer_init(uint16_t timeout_sec)
{
    /* Get the tick frequency. */
    local_tick_frequency_hz = swc_hal_get_free_running_timer_frequency_hz();

    /* Retrieve the application timeout duration in seconds and convert it to tick. */
    local_timeout_tick_count = timeout_sec * local_tick_frequency_hz;

    /* Get a snapshot of the free running timer tick count. */
    local_initial_timer_tick_count = pairing_timer_get_current_timer_tick_count();
}

uint32_t pairing_timer_get_current_timer_tick_count(void)
{
    return swc_hal_get_tick_free_running_timer();
}

bool pairing_timer_is_timeout(void)
{
    return ((pairing_timer_get_current_timer_tick_count() - local_initial_timer_tick_count) > local_timeout_tick_count) ? true : false;
}

void pairing_timer_blocking_delay_ms(uint16_t delay_ms)
{
    uint32_t initial_tick_count = 0;
    uint32_t final_tick_count = 0;

    /* Get the initial tick count */
    initial_tick_count = pairing_timer_get_current_timer_tick_count();

    /* Convert delay ms into tick count. */
    final_tick_count = (delay_ms * get_tick_frequency_hz()) / MS_IN_SEC;

    while ((pairing_timer_get_current_timer_tick_count() - initial_tick_count) < final_tick_count);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Get the initial timer count when the time module was initialized.
 *
 *  @return The initial timer count.
 */
static uint32_t get_tick_frequency_hz(void)
{
    return local_tick_frequency_hz;
}
