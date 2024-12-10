/** @file  pairing_state_machine.h
 *  @brief This file contains the functions to use a state machine.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef PAIRING_STATE_MACHINE_H_
#define PAIRING_STATE_MACHINE_H_

/* INCLUDES *******************************************************************/
#include "pairing_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief State machine callback function.
 */
typedef void (*const pairing_state_machine_callback_t)(void);

/** @brief State machine callback function link.
 *
 *  Link a state with a callback function for the state machine.
 */
typedef struct pairing_state_machine {
    /*! State. */
    uint8_t state;
    /*! State callback function. */
    pairing_state_machine_callback_t state_machine_callback;
} pairing_state_machine_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the state machine.
 *
 *  @note The size must be the number of paired state/function elements.
 *
 *  @param[in] state_machine      The state machine to be processed.
 *  @param[in] state_machine_size The number of elements in the state machine.
 */
void pairing_state_machine_init(pairing_state_machine_t *state_machine, uint8_t state_machine_size);

/** @brief Get the machine state instance.
 *
 *  @return The configured machine state instance.
 */
pairing_state_machine_t *pairing_state_machine_get_instance(void);

/** @brief Get the machine state size.
 *
 *  @return The configured machine state size.
 */
uint8_t pairing_state_machine_get_size(void);

/** @brief Execute the current state machine function.
 *
 *  @param[in] state  The state to be processed.
 */
void pairing_state_machine_execute_state(uint8_t state);

#ifdef __cplusplus
}
#endif

#endif /* PAIRING_STATE_MACHINE_H_ */
