/** @file  quasar.h
 *  @brief Board Support Package for the Quasar board.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_H_
#define QUASAR_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include "quasar_audio.h"
#include "quasar_button.h"
#include "quasar_clock.h"
#include "quasar_dma.h"
#include "quasar_it.h"
#include "quasar_led.h"
#include "quasar_memory.h"
#include "quasar_power.h"
#include "quasar_radio.h"
#include "quasar_rgb.h"
#include "quasar_timer.h"
#include "quasar_timer_ext.h"
#include "quasar_uart.h"
#include "quasar_usb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Configuration set by the application to configure the Quasar BSP.
 *
 * @note If debug mode is enabled, both debug GPIOs and a debug UART will be initialized.
 *       The GPIOs are located on the expansion port, and communication via UART is available through the port
 *       of the ST-Link programmer alongside SWD.
 *       The UART protocol is set to 115200 baud, 8 data bits, no parity, and 1 stop bit (115200 8N1).
 */
typedef struct quasar_config {
    /*! Enable debug mode to control debug pins and UART on the ST-Link. */
    bool debug_enabled;
    /*! Enable radio 1 peripherals. */
    bool radio1_enabled;
    /*! Enable radio 2 peripherals. */
    bool radio2_enabled;
    /*! Select if adc features are activated. This allow to deinitialize the ADC if battery monitoring is not used. */
    bool adc_enabled;
    /*! Select the board clock frequency. */
    quasar_clk_freq_t clk_freq;
    /*! Select the board VDD level. */
    quasar_vdd_selection_t quasar_vdd_selection;
} quasar_config_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the Quasar board's peripherals.
 *
 *  @param[in] quasar_config  The application level configuration.
 */
void quasar_init(quasar_config_t quasar_config);

/** @brief Initiate a system reset request to reset the MCU.
 */
void quasar_system_reset(void);

/** @brief Get the board revision.
 *
 *  @return Board revision.
 */
quasar_revision_t quasar_get_board_revision(void);

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_H_ */
