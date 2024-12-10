/** @file  sac_dummy_endpoint.c
 *  @brief SPARK Audio Core dummy endpoints.
 *
 *  This file contains functions designed to control a dummy endpoint for audio data handling. The main actions provided
 *  by these functions do not perform any operations themselves. Instead, they serve as placeholders, providing function
 *  pointers to 'dummy' functions. These dummy functions are used primarily to facilitate the action of clearing the
 *  queue of audio samples without actually processing them. This setup helps in maintaining the structure necessary for
 *  potentially more complex operations while using simple, non-functional placeholders.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sac_dummy_endpoint.h"

/* PUBLIC FUNCTIONS ***********************************************************/
uint16_t ep_dummy_produce(void *instance, uint8_t *samples, uint16_t size)
{
    (void)instance;
    (void)samples;
    (void)size;

    return 0;
}

uint16_t ep_dummy_consume(void *instance, uint8_t *samples, uint16_t size)
{
    (void)instance;
    (void)samples;
    (void)size;

    return 0;
}

void ep_dummy_start(void *instance)
{
    (void)instance;
}

void ep_dummy_stop(void *instance)
{
    (void)instance;
}
