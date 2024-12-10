/** @file  wps_phy.c
 *  @brief The wps_phy module control the physical layer for the single radio mode.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "wps_phy.h"

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void phy_handle(wps_phy_t *wps_phy);

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
void wps_phy_init(wps_phy_t *wps_phy, wps_phy_cfg_t *cfg)
{
    phy_init(wps_phy, cfg);
    wps_phy->phy_handle = phy_handle;
}

void wps_phy_connect(wps_phy_t *wps_phy)
{
    phy_connect_single(wps_phy);
}

void wps_phy_disconnect(wps_phy_t *wps_phy)
{
    phy_abort_radio_events(wps_phy);
    phy_disconnect(wps_phy);
}

void wps_phy_start_tx_now(wps_phy_t *wps_phy)
{
    phy_start_tx_now(wps_phy);
}

phy_output_signal_t wps_phy_get_main_signal(wps_phy_t *wps_phy)
{
    return phy_get_main_signal(wps_phy);
}

phy_output_signal_t wps_phy_get_auto_signal(wps_phy_t *wps_phy)
{
    return phy_get_auto_signal(wps_phy);
}

void wps_phy_set_main_xlayer(wps_phy_t *wps_phy, xlayer_t *xlayer, xlayer_cfg_internal_t *xlayer_cfg)
{
    xlayer_cfg->rx_constgain = link_gain_loop_get_gain_value(xlayer_cfg->gain_loop);
    phy_set_main_xlayer(wps_phy, xlayer, xlayer_cfg);
}

void wps_phy_set_auto_xlayer(wps_phy_t *wps_phy, xlayer_t *xlayer)
{
    phy_set_auto_xlayer(wps_phy, xlayer);
}

void wps_phy_end_process(wps_phy_t *wps_phy)
{
    link_gain_loop_update(wps_phy->config->gain_loop, wps_phy->xlayer_main->frame.frame_outcome,
                          wps_phy->config->rssi_raw);
}

void wps_phy_write_register(wps_phy_t *wps_phy, uint8_t starting_reg, uint16_t data,
                            reg_write_cfg_t cfg)
{
    phy_write_register(wps_phy, starting_reg, data, cfg);
}

void wps_phy_clear_write_register(wps_phy_t *wps_phy)
{
    phy_clear_write_register(wps_phy);
}

void wps_phy_read_register(wps_phy_t *wps_phy, uint8_t target_register, uint16_t *rx_buffer,
                           bool *xfer_cmplt)
{
    phy_read_register(wps_phy, target_register, rx_buffer, xfer_cmplt);
}

void wps_phy_enable_debug_feature(wps_phy_t *wps_phy, phy_debug_cfg_t *phy_debug)
{
    phy_enable_debug_feature(wps_phy, phy_debug);
}

/* PRIVATE FUNCTION DEFINITIONS ***********************************************/
/** @brief State : PHY handle - Handle PHY signals during SPI transfers with the radio and frame
 * outcome reception.
 *
 *   @param[in] wps  WPS instance struct.
 */
static void phy_handle(wps_phy_t *wps_phy)
{
    phy_output_signal_t phy_out_signal = wps_phy_get_main_signal(wps_phy);

    switch (phy_out_signal) {
    case PHY_SIGNAL_CONFIG_COMPLETE:
    case PHY_SIGNAL_CONNECT:
        wps_phy->wps_phy_callback(wps_phy->mac, wps_phy);
        break;
    case PHY_SIGNAL_FRAME_SENT_ACK:
    case PHY_SIGNAL_FRAME_SENT_NACK:
    case PHY_SIGNAL_FRAME_RECEIVED:
    case PHY_SIGNAL_FRAME_MISSED:
    case PHY_SIGNAL_FRAME_NOT_SENT:
        wps_phy_end_process(wps_phy);
        wps_phy->wps_phy_callback(wps_phy->mac, wps_phy);
        break;
    case PHY_SIGNAL_ERROR:
        while (true) {};
        break;
    default:
        break;
    }
}
