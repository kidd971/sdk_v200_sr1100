/** @file  star_network_facade.h
 *  @brief Facades for low-level platform-specific features required by the application example.
 *
 *  @note This header defines the interfaces for various hardware features used by the star network example.
 *
 *  These facades abstract the underlying platform-specific implementations of features like SPI communication, IRQ
 *  handling, timer functions, and context switching mechanisms. The actual implementations are selected at compile time
 *  based on the target platform, allowing for flexibility and portability across different hardware. The facade is
 *  designed to be a compile-time dependency only, with no support for runtime polymorphism. This ensures tight
 *  integration with the build system and minimal overhead.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef STAR_NETWORK_FACADE_H_
#define STAR_NETWORK_FACADE_H_

/* INCLUDES *******************************************************************/
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include "common_facade.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Certification modes.
 */
typedef enum facade_certification_mode {
    FACADE_CERTIF_NONE,
    FACADE_CERTIF_STAR_NETWORK,
} facade_certification_mode_t;

/** @brief Function callbacks for button presses.
 */
typedef struct facade_button_callbacks {
    /*! Function callback to pair/unpair the device. */
    void (*pairing_callback)(void);
    /*! Function callback to unpair the device. */
    void (*unpair_device_callback)(void);
} facade_button_callbacks_t;

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Read button state to define if certification mode is required.
 *
 *  @return The certification mode to be applied.
 */
facade_certification_mode_t facade_get_certification_mode(void);

/** @brief Set button function callbacks.
 *
 *  @param[in] button_callbacks  Button function callback structure.
 */
void facade_set_button_callbacks(facade_button_callbacks_t button_callbacks);

/** @brief Poll for button presses and execute function callback.
 */
void facade_button_handling(void);

/** @brief Read the state of the node's user button that will set the coordinator's LED state.
 *
 *  @return Returns true if the button is pressed, false otherwise.
 */
bool facade_node_read_user_button_state(void);

/** @brief Read the state of the coordinator's user button 1 that will set the node1's LED state.
 *
 *  @return Returns true if the button is pressed, false otherwise.
 */
bool facade_coord_read_user_button_1_state(void);

/** @brief Read the state of the coordinator's user button 2 that will set the node2's LED state.
 *
 *  @return Returns true if the button is pressed, false otherwise.
 */
bool facade_coord_read_user_button_2_state(void);

/** @brief Notify user that the coord received a payload from node 1.
 */
void facade_node1_payload_coord_received_status(void);

/** @brief Notify user that the coord received an empty payload from node 1.
 */
void facade_node1_empty_payload_coord_received_status(void);

/** @brief Notify user that the coord received a payload from node 2.
 */
void facade_node2_payload_coord_received_status(void);

/** @brief Notify user that the coord received an empty payload from node 2.
 */
void facade_node2_empty_payload_coord_received_status(void);

/** @brief Notify user that the node received a payload from the coord.
 */
void facade_coord_payload_node_received_status(void);

/** @brief Notify user that the node received an empty payload from the coord.
 */
void facade_coord_empty_payload_node_received_status(void);

#ifdef __cplusplus
}
#endif

#endif /* STAR_NETWORK_FACADE_H_ */
