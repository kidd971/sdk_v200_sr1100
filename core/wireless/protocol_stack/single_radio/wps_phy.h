/** @file  wps_phy.h
 *  @brief The wps_phy module control the physical layer for the single radio mode.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef WPS_PHY_H_
#define WPS_PHY_H_

#ifdef __cplusplus
extern "C" {
#endif

/* INCLUDES *******************************************************************/
#include "wps_phy_common.h"
#include "wps_phy_def.h"

/* PRIVATE FUNCTION PROTOTYPES ************************************************/

/** @brief Initialize The phy layer of the wireless protocol stack.
 *
 *  @param[in] wps_phy  WPS PHY instance.
 *  @param[in] cfg      PHY layer configuration instance.
 */
void wps_phy_init(wps_phy_t *wps_phy, wps_phy_cfg_t *cfg);

/** @brief Connect the phy Layer.
 *
 *  @param wps_phy WPS PHY instance.
 */
void wps_phy_connect(wps_phy_t *wps_phy);

/** @brief Disconnect the phy Layer.
 *
 *  @param wps_phy WPS PHY instance.
 */
void wps_phy_disconnect(wps_phy_t *wps_phy);

/** @brief Trigger the transmission of a frame.
 *
 *  @param wps_phy WPS PHY instance.
 */
void wps_phy_start_tx_now(wps_phy_t *wps_phy);

/** @brief Get the phy main output signal.
 *
 *  @param[in] wps_phy  WPS PHY instance.
 *  @return Main output signal
 */
phy_output_signal_t wps_phy_get_main_signal(wps_phy_t *wps_phy);

/** @brief Get the autoreply phy output signal.
 *
 *  @param[in] wps_phy  WPS PHY instance.
 *  @return Auto output signal.
 */
phy_output_signal_t wps_phy_get_auto_signal(wps_phy_t *wps_phy);

/** @brief Set the phy main xlayer.
 *
 *  @param[in] wps_phy     WPS PHY instance.
 *  @param[in] xlayer      Main xlayer.
 *  @param[in] xlayer_cfg  xlayer configs.
 */
void wps_phy_set_main_xlayer(wps_phy_t *wps_phy, xlayer_t *xlayer, xlayer_cfg_internal_t *xlayer_cfg);

/** @brief Set the phy auto-reply xlayer.
 *
 *  @param[in] wps_phy       WPS PHY instance.
 *  @param[in] xlayer        Auto xlayer.
 */
void wps_phy_set_auto_xlayer(wps_phy_t *wps_phy, xlayer_t *xlayer);

/** @brief Write to a register in the radio.
 *
 *  @param[in] wps_phy       WPS PHY instance.
 *  @param[in] starting_reg  Target register.
 *  @param[in] data          Byte to send.
 *  @param[in] cfg           write config.
 */
void wps_phy_write_register(wps_phy_t *wps_phy, uint8_t starting_reg, uint16_t data,
                            reg_write_cfg_t cfg);

/** @brief Clear periodic register write.
 *
 *  @param[in] wps_phy       WPS PHY instance.
 */
void wps_phy_clear_write_register(wps_phy_t *wps_phy);

/** @brief Read to a register in the radio.
 *
 *  @param[in]  wps_phy          WPS PHY instance.
 *  @param[in]  target_register  Target register.
 *  @param[out] rx_buffer        Buffer containing register data.
 *  @param[out] xfer_cmplt       Flag to notify transfer complete.
 */
void wps_phy_read_register(wps_phy_t *wps_phy, uint8_t target_register, uint16_t *rx_buffer,
                           bool *xfer_cmplt);

/** @brief Enable the debug feature of the radio.
 *
 *  @note Use for experimental feature located in
 *        the radio, like interleav, preamble / syncword
 *        detection etc.
 *
 *  @param[in] phy_debug  PHY debugging features configuration.
 */
void wps_phy_enable_debug_feature(wps_phy_t *wps_phy, phy_debug_cfg_t *phy_debug);

/** @brief Process the phy Layer state machine of the wireless protocol stack.
 *
 *  This function should be called by the WPS inside the dma or the radio interrupt.
 *
 *  @param[in]  wps_phy  WPS PHY instance.
 */
static inline void wps_phy_prepare_frame(wps_phy_t *wps_phy)
{
    phy_process(wps_phy);
}

/** @brief Process the phy Layer state machine of the wireless protocol stack.
 *
 *  This function should be called by the WPS inside the dma or the radio interrupt.
 *
 *  @param[in]  wps_phy  WPS PHY instance.
 */
static inline void wps_phy_process(wps_phy_t *wps_phy)
{
    /* Specific PHY process */
    if (wps_phy->signal_main != PHY_SIGNAL_CONNECT) {
        phy_process(wps_phy);
    }

    /* Generic PHY process */
    wps_phy->phy_handle(wps_phy);
}

/** @brief Process phy end of frame.
 *
 *  @param[in] wps_phy       WPS PHY instance.
 */
void wps_phy_end_process(wps_phy_t *wps_phy);

/** @brief Set the phy input signal.
 *
 *  @param[in] wps_phy  WPS PHY instance.
 *  @param[in] signal   Input signal.
 */
static inline void wps_phy_set_input_signal(wps_phy_t *wps_phy, phy_input_signal_t signal)
{
    phy_set_input_signal(wps_phy, signal);
}

#ifdef __cplusplus
}
#endif

#endif /* WPS_PHY_H_ */
