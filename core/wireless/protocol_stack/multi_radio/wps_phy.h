/** @file  wps_phy.h
 *  @brief The wps_phy module controle the physical layer for the dual radio mode.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef WPS_PHY_MULTI_H_
#define WPS_PHY_MULTI_H_

/* INCLUDES *******************************************************************/
#include "link_multi_radio.h"
#include "wps_config.h"
#include "wps_phy_common.h"
#include "wps_phy_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief WPS Dual radio PHY layer structure.
 */
typedef struct wps_phy_multi {
    /*! Current radio index */
    uint8_t current_radio_idx;
    /*! Leading radio index */
    uint8_t leading_radio_idx;
    /*! Following radio index */
    uint8_t following_radio_idx;
    /*! Multi radio instance */
    multi_radio_t multi_radio;
    /*! Lqi instance for multi radio processing */
    lqi_t lqi[WPS_RADIO_COUNT];
    /*! Main xlayer of the following radio */
    xlayer_t following_main_xlayer;
    /*! Auto xlayer of the following radio */
    xlayer_t following_auto_xlayer;
    /*! xlayer configuration structure of the following radio */
    xlayer_cfg_internal_t following_xlayer_cfg;
    /*! Radio timer frequency in Hz */
    uint32_t timer_frequency_hz;
    /*! Ratio between configured muti radio timer frequency and radio timer frequency*/
    float timer_frequency_ratio;
} wps_phy_multi_t;

/* EXTERNS ********************************************************************/
extern wps_phy_multi_t wps_phy_multi;

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Initialize The phy layer of the wireless protocol stack.
 *
 *  @param[in] wps_phy  WPS PHY instance.
 *  @param[in] cfg      PHY layer configuration instance.
 */
void wps_phy_init(wps_phy_t *wps_phy, wps_phy_cfg_t *cfg);

/** @brief Connect the phy Layer.
 *
 *  @param[in] wps_phy  WPS PHY instance.
 */
void wps_phy_connect(wps_phy_t *wps_phy);

/** @brief Disconnect the phy Layer.
 *
 *  @param[in] wps_phy  WPS PHY instance.
 */
void wps_phy_disconnect(wps_phy_t *wps_phy);

/** @brief Select radio to use.
 *
 *  @param[in] radio_select  Radio selection.
 */
void wps_phy_set_radio_select(multi_radio_select_t radio_select);

/** @brief Set syncing RX period duration.
 *
 *   @param[in] wps_phy                    WPS PHY instance.
 *   @param[in] syncing_period_pll_cycles  Time in pll cycle the radio is sleeping between rx period.
 */
void wps_phy_set_syncing_duration(wps_phy_t *wps_phy, uint16_t syncing_period_pll_cycles);

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
 *  @param[in] wps_phy  WPS PHY instance.
 *  @param[in] xlayer   Auto xlayer.
 */
void wps_phy_set_auto_xlayer(wps_phy_t *wps_phy, xlayer_t *xlayer);

/** @brief Process phy end of frame.
 *
 *  @note Update gain loop and multi radio LQI.
 *
 *  @param[in] wps_phy       WPS PHY instance.
 */
void wps_phy_end_process(wps_phy_t *wps_phy);

/** @brief Process the MCU timer callback for radio synchronization
 *
 *  @param[in] wps_phy  WPS PHY instance.
 */
void wps_phy_multi_process_radio_timer(wps_phy_t *wps_phy);

/** @brief Initialize the multi-radio module.
 *
 *  @param[in] multi_cfg  multi-radio init configuration.
 *  @param[in] chip_rate  Radio chip rate.
 */
void wps_multi_radio_init(wps_multi_cfg_t multi_cfg, chip_rate_cfg_t chip_rate);

/** @brief Set the multi radio TX wakeup mode.
 *
 *  @param[in] tx_wakeup_mode  multi radio TX wakeup mode.
 */
void wps_multi_radio_set_tx_wakeup_mode(multi_radio_tx_wakeup_mode_t tx_wakeup_mode);

/** @brief Write to a register in the radio.
 *
 *  @param[in] wps_phy       Both radio PHY instance.
 *  @param[in] starting_reg  Target register.
 *  @param[in] data          Byte to send.
 *  @param[in] cfg           Write config.
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
 *  @param[in]  wps_phy          Both radio PHY instance.
 *  @param[in]  target_register  Target register.
 *  @param[out] rx_buffer        Buffer containing register data.
 *  @param[out] xfer_cmplt       Flag to notify transfer complete.
 */
void wps_phy_read_register(wps_phy_t *wps_phy, uint8_t target_register, uint16_t *rx_buffer,
                           bool *xfer_cmplt);

/** @brief Process the phy Layer state machine of the wireless protocol stack.
 *
 *  This function should be called by the WPS inside the dma or the radio interrupt.
 *
 *  @param[in] wps_phy  WPS PHY instance.
 */
static inline void wps_phy_prepare_frame(wps_phy_t *wps_phy)
{
    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        sr_access_enable_dma_irq(i);
        phy_process(&wps_phy[i]);
    }
    swc_hal_timer_multi_radio_timer_set_period(wps_phy_multi.timer_frequency_ratio * wps_phy->config->sleep_time);
}

/** @brief Process the phy Layer state machine of the wireless protocol stack.
 *
 *  This function should be called by the WPS inside the dma or the radio interrupt.
 *
 *  @param[in] wps_phy  WPS PHY instance.
 */
static inline void wps_phy_process(wps_phy_t *wps_phy)
{
    /* Specific PHY process */
    if (wps_phy->signal_main != PHY_SIGNAL_CONNECT) {
        phy_process(&wps_phy[wps_phy_multi.current_radio_idx]);
    }

    /* Generic PHY process */
    wps_phy->phy_handle(wps_phy);
}

/** @brief Set the phy input signal.
 *
 *  @param[in] wps_phy  WPS PHY instance.
 *  @param[in] signal   Input signal.
 */
static inline void wps_phy_set_input_signal(wps_phy_t *wps_phy, phy_input_signal_t signal)
{
    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        phy_set_input_signal(&wps_phy[i], signal);
    }
}

/** @brief Set the current radio index for phy processing.
 *
 *  @param[in] radio_idx  Radio index.
 */
static inline void wps_phy_multi_set_current_radio_idx(uint8_t radio_idx)
{
    wps_phy_multi.current_radio_idx = radio_idx;
}

/** @brief Enable the debug feature of the radio.
 *
 *  @note Use for experimental feature located in
 *        the radio, like interleav, preamble / syncword
 *        detection etc.
 *
 *  @param[in] phy_debug  PHY debugging features configuration.
 */
void wps_phy_enable_debug_feature(wps_phy_t *wps_phy, phy_debug_cfg_t *phy_debug);

/** @brief Enable the debug feature of the radio. */
uint8_t wps_phy_multi_get_replying_radio(void);

/** @brief Get the multi radio TX wakeup mode.
 *
 *  @return The multi radio TX wakeup mode.
 */
static inline multi_radio_tx_wakeup_mode_t wps_phy_multi_get_tx_wakeup_mode(void)
{
    return wps_phy_multi.multi_radio.tx_wakeup_mode;
}

#ifdef __cplusplus
}
#endif

#endif /* WPS_PHY_MULTI_H_ */
