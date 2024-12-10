/** @file  wps_phy_common.h
 *  @brief Wireless protocol stack phy control.
 *
 *  @note Supports little endian only.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef WPS_PHY_H
#define WPS_PHY_H

/* INCLUDES *******************************************************************/
#include "wps_phy_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize The phy layer of the wireless protocol stack.
 *
 *  @param[in] wps_phy  WPS PHY instance.
 *  @param[in] cfg      PHY layer configuration instance.
 */
void phy_init(wps_phy_t *wps_phy, wps_phy_cfg_t *cfg);

/** @brief Connect the phy Layer.
 *
 *  @param[in] wps_phy  WPS PHY instance.
 */
void phy_connect(wps_phy_t *wps_phy);

/** @brief Connect the phy Layer for single radio.
 *
 *  @param[in] wps_phy WPS PHY instance.
 */
void phy_connect_single(wps_phy_t *wps_phy);

/** @brief Connect the phy Layer for multi radio.
 *
 *  @param[in] wps_phy WPS PHY instance.
 */
void phy_connect_multi(wps_phy_t *wps_phy);

/** @brief Wake up radio for multi radio.
 *
 *  @param[in] wps_phy WPS PHY instance.
 */
void phy_wakeup_multi(wps_phy_t *wps_phy);

/** @brief Abort configured radio events.
 *
 *  @param[in] wps_phy  WPS PHY instance.
 */
void phy_abort_radio_events(wps_phy_t *wps_phy);

/** @brief Disconnect the phy Layer.
 *
 *  @note phy_abort_radio_events should be called for all radios before phy_disconnect.
 *
 *  @param[in] wps_phy  WPS PHY instance.
 */
void phy_disconnect(wps_phy_t *wps_phy);

/** @brief Trigger the transmission of a frame.
 *
 *  @param[in] wps_phy  WPS PHY instance.
 */
void phy_start_tx_now(wps_phy_t *wps_phy);

/** @brief Enqueue PHY prepare states.
 *
 *  @param[in] wps_phy  WPS PHY instance.
 */
void phy_enqueue_prepare(wps_phy_t *wps_phy);

/** @brief Enqueue PHY none states.
 *
 *  @param[in] wps_phy  WPS PHY instance.
 */
void phy_enqueue_none(wps_phy_t *wps_phy);

/** @brief Get the phy main output signal.
 *
 *  @param[in] wps_phy  WPS PHY instance.
 *  @return Main output signal
 */
phy_output_signal_t phy_get_main_signal(wps_phy_t *wps_phy);

/** @brief Get the autoreply phy output signal.
 *
 *  @param[in] wps_phy  WPS PHY instance.
 *  @return Auto output signal.
 */
phy_output_signal_t phy_get_auto_signal(wps_phy_t *wps_phy);

/** @brief Set the phy main xlayer.
 *
 *  @param[in] wps_phy     WPS PHY instance.
 *  @param[in] xlayer      Main xlayer.
 *  @param[in] xlayer_cfg  xlayer configs.
 */
void phy_set_main_xlayer(wps_phy_t *wps_phy, xlayer_t *xlayer, xlayer_cfg_internal_t *xlayer_cfg);

/** @brief Set the phy auto-reply xlayer.
 *
 *  @param[in] wps_phy  WPS PHY instance.
 *  @param[in] xlayer   Auto xlayer.
 */
void phy_set_auto_xlayer(wps_phy_t *wps_phy, xlayer_t *xlayer);

/** @brief Write to a register in the radio.
 *
 *  @param[in] wps_phy       WPS PHY instance.
 *  @param[in] starting_reg  Target register.
 *  @param[in] data          Byte to send.
 *  @param[in] cfg           Write config.
 */
void phy_write_register(wps_phy_t *wps_phy, uint8_t starting_reg, uint16_t data,
                        reg_write_cfg_t cfg);

/** @brief Clear periodic write registers.
 *
 *  @param[in] wps_phy       WPS PHY instance.
 */
void phy_clear_write_register(wps_phy_t *wps_phy);

/** @brief Read to a register in the radio.
 *
 *  @param[in]  wps_phy          WPS PHY instance.
 *  @param[in]  target_register  Target register.
 *  @param[out] rx_buffer        Buffer containing register data.
 *  @param[out] xfer_cmplt       Flag to notify transfer complete.
 */
void phy_read_register(wps_phy_t *wps_phy, uint8_t target_register, uint16_t *rx_buffer,
                       bool *xfer_cmplt);

/** @brief Enable the debug feature of the radio.
 *
 *  @note Use for experimental feature located in
 *        the radio, like interleav, preamble / syncword
 *        detection etc.
 *  @param[in] wps_phy    WPS PHY instance.
 *  @param[in] phy_debug  PHY debugging features configuration.
 */
void phy_enable_debug_feature(wps_phy_t *wps_phy, phy_debug_cfg_t *phy_debug);

/** @brief Set the phy input signal.
 *
 *  @param[in] wps_phy  WPS PHY instance.
 *  @param[in] signal   Input signal.
 */
static inline void phy_set_input_signal(wps_phy_t *wps_phy, phy_input_signal_t signal)
{
    wps_phy->input_signal = signal;
}

/** @brief Process the phy Layer state machine of the wireless protocol stack.
 *
 *  This function should be called by the WPS inside the dma or the radio interrupt.
 *
 *  @param[in]  wps_phy  WPS PHY instance.
 */
static inline void phy_process(wps_phy_t *wps_phy)
{
    wps_phy->signal_main = PHY_SIGNAL_PROCESSING;

    do {
        wps_phy->current_state[wps_phy->state_step++](wps_phy);
    } while (wps_phy->signal_main == PHY_SIGNAL_PROCESSING);

    if (wps_phy->current_state[wps_phy->state_step] == wps_phy->end_state) {
        wps_phy->current_state[wps_phy->state_step](wps_phy);
    }
}

#ifdef __cplusplus
}
#endif

#endif /* WPS_PHY_H */
