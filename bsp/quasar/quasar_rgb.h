/** @file  quasar_rgb.h
 *  @brief This module configures the RGB LED and provides functions to control each color.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_RGB_H_
#define QUASAR_RGB_H_

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Quasar BSP RGB LED colors selection.
 */
typedef enum quasar_rgb_colors {
    /*! User application RGB LED set to red. */
    QUASAR_RGB_COLOR_RED,
    /*! User application RGB LED set to green. */
    QUASAR_RGB_COLOR_GREEN,
    /*! User application RGB LED set to blue. */
    QUASAR_RGB_COLOR_BLUE,
    /*! User application RGB LED set to yellow. */
    QUASAR_RGB_COLOR_YELLOW,
    /*! User application RGB LED set to cyan. */
    QUASAR_RGB_COLOR_CYAN,
    /*! User application RGB LED set to magenta. */
    QUASAR_RGB_COLOR_MAGENTA,
    /*! User application RGB LED set to white. */
    QUASAR_RGB_COLOR_WHITE,
} quasar_rgb_color_t;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
/** @brief Initialize RGB LED peripherals.
 */
void quasar_rgb_init(void);

/** @brief Deinitialize RGB LED peripherals.
 */
void quasar_rgb_deinit(void);

/** @brief Configures the RGB LED to display a specified color.
 *
 *  @note  The color can be specified just once or every time a color change is
 *         desired. By default the specified color is white.
 *
 *  @param[in] rgb_color The color to configure the RGB LED to.
 */
void quasar_rgb_configure_color(quasar_rgb_color_t rgb_color);

/** @brief Set RGB LED peripheral to the specified color.
 */
void quasar_rgb_set(void);

/** @brief Clear RGB LED peripheral.
 */
void quasar_rgb_clear(void);

/** @brief Toggle RGB LED peripheral.
 */
void quasar_rgb_toggle(void);

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_RGB_H_ */
