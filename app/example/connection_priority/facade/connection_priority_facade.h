/** @file  connection_priority_facade.h
 *  @brief Facades for low-level platform-specific features required by the application example.
 *
 *  @note This header defines the interfaces for various hardware features used by the connection priority example.
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
#ifndef CONNECTION_PRIORITY_H_
#define CONNECTION_PRIORITY_H_

/* INCLUDES *******************************************************************/
#include <stdint.h>
#include "common_facade.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES***********************************************************************/
/** @brief Certification modes.
 */
typedef enum facade_certification_mode {
    FACADE_CERTIF_NONE,
    FACADE_CERTIF_CONNECTION_ID_0,
    FACADE_CERTIF_CONNECTION_ID_1,
    FACADE_CERTIF_CONNECTION_ID_2,
    FACADE_CERTIF_CONNECTION_ID_3,
    FACADE_CERTIF_CONNECTION_ID_4,
} facade_certification_mode_t;

/** @brief Function callbacks for button presses.
 */
typedef struct facade_button_callbacks {
    /*! Function callback to pair/unpair the device. */
    void (*pairing_callback)(void);
    /*! Function callback to reset the statistics. */
    void (*reset_stats_callback)(void);
    /*! Function callback to toggle whether payloads are generated on Connection ID 0 by the application or not. */
    void (*stop_cid0_toggle_callback)(void);
} facade_button_callbacks_t;

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Read button state to define if certification mode for Coord is required.
 *
 *  @return The certification mode to be applied.
 */
facade_certification_mode_t facade_get_coord_certification_mode(void);

/** @brief Read button state to define if certification mode for Node is required.
 *
 *  @return The certification mode to be applied.
 */
facade_certification_mode_t facade_get_node_certification_mode(void);

/** @brief Set button function callbacks.
 *
 *  @param[in] button_callbacks  Button function callback structure.
 */
void facade_set_button_callbacks(facade_button_callbacks_t button_callbacks);

/** @brief Poll for button presses and execute function callback.
 */
void facade_button_handling(void);

/** @brief Initialize and set the timer 1 period.
 *
 *  @param[in] period_us  Timer period in microseconds.
 */
void facade_packet_rate_timer1_init(uint32_t period_us);

/** @brief Set the timer 1 callback.
 *
 *  @param[in] callback  Callback when timer expires.
 */
void facade_packet_rate_set_timer1_callback(void (*callback)(void));

/** @brief Start the timer 1.
 */
void facade_packet_rate_timer1_start(void);

/** @brief Stop the timer 1.
 */
void facade_packet_rate_timer1_stop(void);

/** @brief Initialize and set the timer 2 period.
 *
 *  @param[in] period_us  Timer period in microseconds.
 */
void facade_packet_rate_timer2_init(uint32_t period_us);

/** @brief Set the timer 2 callback.
 *
 *  @param[in] callback  Callback when timer expires.
 */
void facade_packet_rate_set_timer2_callback(void (*callback)(void));

/** @brief Start the timer 2.
 */
void facade_packet_rate_timer2_start(void);

/** @brief Stop the timer 2.
 */
void facade_packet_rate_timer2_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* CONNECTION_PRIORITY_H_ */
