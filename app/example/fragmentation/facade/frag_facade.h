/** @file  frag_facade.h
 *  @brief Facades for low-level platform-specific features required by the application example.
 *
 *  @note This header defines the interfaces for various hardware features used by the fragmentation example.
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
#ifndef FRAG_FACADE_H_
#define FRAG_FACADE_H_

/* INCLUDES *******************************************************************/
#include <stdarg.h>
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
    FACADE_CERTIF_FRAGMENTATION,
} facade_certification_mode_t;

/** @brief Function callbacks for button presses.
 */
typedef struct facade_button_callbacks {
    /*! Function callback to pair/unpair the device. */
    void (*pairing_callback)(void);
    /*! Function callback to reset the statistics. */
    void (*reset_stats_callback)(void);
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

/** @brief This function sets the function callback for the packet generation timer IRQ.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void facade_packet_generation_set_timer_callback(void (*irq_callback)(void));

/** @brief Initialize packet generation timer peripheral.
 *
 *  @param[in] timeslot  Application timeslot duration.
 */
void facade_packet_generation_timer_init(uint32_t timeslot);

/** @brief Start packet generation timer peripheral.
 */
void facade_packet_generation_timer_start(void);

/** @brief Stop packet generation timer peripheral.
 */
void facade_packet_generation_timer_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* FRAG_FACADE_H_ */
