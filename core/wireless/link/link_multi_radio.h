/** @file link_multi_radio.h
 *  @brief Multi radio module.
 *
 *  @copyright Copyright (C) 2020 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef LINK_MULTI_RADIO_H_
#define LINK_MULTI_RADIO_H_

/* INCLUDES *******************************************************************/
#include <stdint.h>
#include "link_lqi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Multi radio modes.
 */
typedef enum multi_radio_mode {
    /*! Dual radio processing. */
    MULTI_RADIO_MODE_0,
    /*! Single radio processing. */
    MULTI_RADIO_MODE_1,
} multi_radio_mode_t;

/** @brief Multi radio transmission modes.
 */
typedef enum multi_radio_tx_mode {
    /*! Radios are manually awakened on transmission start. */
    MULTI_TX_WAKEUP_MODE_MANUAL,
    /*! Radios are automatically awakened on transmission start.
     *
     *  NOTE: Auto mode will not work if the device is mainly transmitting. Both radios risk to become out of sync.
     */
    MULTI_TX_WAKEUP_MODE_AUTO,
} multi_radio_tx_wakeup_mode_t;

/** @brief Radio selections.
 */
typedef enum multi_radio_select {
    /*! Let the multi radio algorithm select the radio. */
    MULTI_RADIO_SELECT_ALGO,
    /*! Select radio 1. */
    MULTI_RADIO_SELECT_RADIO1,
    /*! Select radio 2. */
    MULTI_RADIO_SELECT_RADIO2,
    /*! Radio selections count. */
    _MULTI_RADIO_SELECT_COUNT,
} multi_radio_select_t;

/** @brief Multi radio instance.
 */
typedef struct multi_radio {
    /*! Radios LQI. */
    lqi_t *radios_lqi;
    /*! Radio count. */
    uint8_t radio_count;
    /*! Number of samples to average on. */
    uint16_t avg_sample_count;
    /*! Hysteresis between radios (only for mode 0). */
    uint16_t hysteresis_tenth_db;
    /*! Replying radio. */
    uint8_t replying_radio;
    /*! Radio selection for debug, 0 for algorithm, specific radio otherwise. */
    multi_radio_select_t radio_select;
    /*! Chosen multi radio mode. */
    multi_radio_mode_t mode;
    /*! Multi radio TX wakeup mode. */
    multi_radio_tx_wakeup_mode_t tx_wakeup_mode;
    /*! RSSI threshold (only for mode 1). */
    uint8_t rssi_threshold;
} multi_radio_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Update multi radio module.
 *
 *  @param[in] multi_radio  Multi radio object.
 *  @return None.
 */
void link_multi_radio_update(multi_radio_t *multi_radio);

/** @brief Get replying radio.
 *
 *  @param[in] multi_radio  Multi radio object.
 *  @return Replying radio.
 */
uint8_t link_multi_radio_get_replying_radio(multi_radio_t *multi_radio);

#ifdef __cplusplus
}
#endif
#endif /* LINK_MULTI_RADIO_H_ */
