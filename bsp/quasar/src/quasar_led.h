/** @file  quasar_led.h
 *  @brief This module configures LEDs and provides functions to control each of them.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_LED_H_
#define QUASAR_LED_H_

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Quasar BSP LED peripherals selection.
 */
typedef enum quasar_led_peripherals {
    /*! The LED_USB indicates whether the USB port is the audio source. */
    QUASAR_LED_USB,
    /*! The LED_LINEIN indicates that the line-in jack is enabled. */
    QUASAR_LED_LINEIN,
    /*! The LED_HPJACK indicates that the headphone jack is enabled. */
    QUASAR_LED_HPJACK,
    /*! User application LED. */
    QUASAR_LED_USER_1,
    /*! User application LED. */
    QUASAR_LED_USER_2,
    /*! User application LED. */
    QUASAR_LED_USER_3,
    /*! User application LED. */
    QUASAR_LED_USER_4,
} quasar_led_peripherals_t;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
/** @brief Initialize LED peripherals.
 */
void quasar_led_init(void);

/** @brief Deinitialize LED peripherals.
 */
void quasar_led_deinit(void);

/** @brief Set the GPIO output related to the selected LED peripheral.
 *
 *  @param[in] quasar_led_peripheral  Selected LED peripheral.
 */
void quasar_led_set(quasar_led_peripherals_t quasar_led_peripheral);

/** @brief Clear the GPIO output related to the selected LED peripheral.
 *
 *  @param[in] quasar_led_peripheral  Selected LED peripheral.
 */
void quasar_led_clear(quasar_led_peripherals_t quasar_led_peripheral);

/** @brief Toggle the GPIO output related to the selected LED peripheral.
 *
 *  @param[in] quasar_led_peripheral  Selected LED peripheral.
 */
void quasar_led_toggle(quasar_led_peripherals_t quasar_led_peripheral);

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_LED_H_ */
