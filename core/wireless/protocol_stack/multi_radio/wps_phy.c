/** @file  wps_phy.c
 *  @brief The wps_phy module controle the physical layer for the dual radio mode.
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

/* CONSTANTS ******************************************************************/
#define MULTI_RADIO_RSSI_HYSTERESIS       30
#define MULTI_RADIO_AVG_SAMPLE            4
#define MULTI_RADIO_RETRY_TIMER_PERIOD_US 2
#define US_TO_S(us)                       ((us) / 1000000)

/* PUBLIC GLOBALS *************************************************************/
wps_phy_multi_t wps_phy_multi;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static bool is_frame_done(phy_output_signal_t output_signal, uint8_t index);
static bool is_frame_processing(phy_output_signal_t output_signal, uint8_t index);
static void single_radio_processing_switch_radio(wps_phy_t *wps_phy);

/* PUBLIC FUNCTIONS ***********************************************************/
void wps_multi_radio_init(wps_multi_cfg_t multi_cfg, chip_rate_cfg_t chip_rate)
{
    memset(&wps_phy_multi, 0, sizeof(wps_phy_multi));
    wps_phy_multi.timer_frequency_hz = multi_cfg.timer_frequency_hz;
    wps_phy_multi.timer_frequency_ratio = (float)(multi_cfg.timer_frequency_hz / 1000) /
                                          (float)(PLL_FREQ_HZ(chip_rate) / 1000);
    wps_phy_multi.multi_radio.avg_sample_count = multi_cfg.avg_sample_count;
    wps_phy_multi.multi_radio.mode = multi_cfg.mode;
    wps_phy_multi.multi_radio.tx_wakeup_mode = multi_cfg.tx_wakeup_mode;
    wps_phy_multi.multi_radio.rssi_threshold = multi_cfg.rssi_threshold;
}

void wps_multi_radio_set_tx_wakeup_mode(multi_radio_tx_wakeup_mode_t tx_wakeup_mode)
{
    wps_phy_multi.multi_radio.tx_wakeup_mode = tx_wakeup_mode;
}

void wps_phy_init(wps_phy_t *wps_phy, wps_phy_cfg_t *cfg)
{
    phy_init(wps_phy, cfg);
    wps_phy->phy_handle                           = phy_handle;
    wps_phy_multi.multi_radio.radios_lqi          = wps_phy_multi.lqi;
    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        wps_phy_multi.multi_radio.radios_lqi[i].total_count = 0;
    }
    if (wps_phy_multi.multi_radio.mode == MULTI_RADIO_MODE_1) {
        for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
            wps_phy_multi.multi_radio.radios_lqi[i].mode = LQI_MODE_0;
        }
    }
    wps_phy_multi.multi_radio.radio_count         = WPS_RADIO_COUNT;
    wps_phy_multi.multi_radio.hysteresis_tenth_db = MULTI_RADIO_RSSI_HYSTERESIS;
}

void wps_phy_connect(wps_phy_t *wps_phy)
{
    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        phy_connect_multi(&wps_phy[i]);
    }
    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        phy_connect(&wps_phy[i]);
    }
    if (wps_phy_multi.multi_radio.mode == MULTI_RADIO_MODE_1) {
        single_radio_processing_switch_radio(wps_phy);
    }
    swc_hal_timer_multi_radio_timer_start();
    sr_access_radio_context_switch(0);
}

void wps_phy_disconnect(wps_phy_t *wps_phy)
{
    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        phy_abort_radio_events(&wps_phy[i]);
    }
    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        phy_disconnect(&wps_phy[i]);
    }
    swc_hal_timer_multi_radio_timer_stop();
}

void wps_phy_set_radio_select(multi_radio_select_t radio_select)
{
    wps_phy_multi.multi_radio.radio_select = radio_select;
}

phy_output_signal_t wps_phy_get_main_signal(wps_phy_t *wps_phy)
{
    phy_output_signal_t leading_signal   = PHY_SIGNAL_YIELD;
    phy_output_signal_t following_signal = PHY_SIGNAL_YIELD;
    uint8_t radio_idx                    = wps_phy_multi.current_radio_idx;

    wps_phy_multi.leading_radio_idx = link_multi_radio_get_replying_radio(&wps_phy_multi.multi_radio);
    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        if (i == wps_phy_multi.leading_radio_idx) {
            leading_signal = phy_get_main_signal(&wps_phy[i]);
        } else {
            wps_phy_multi.following_radio_idx = i;
            following_signal = phy_get_main_signal(&wps_phy[i]);
        }
    }

    if (is_frame_done(phy_get_main_signal(&wps_phy[radio_idx]), radio_idx)) {
        sr_access_disable_dma_irq(radio_idx);
    }

    if ((leading_signal == PHY_SIGNAL_CONFIG_COMPLETE) &&
        ((following_signal == PHY_SIGNAL_CONFIG_COMPLETE) || (wps_phy_multi.multi_radio.mode == MULTI_RADIO_MODE_1))) {
        return PHY_SIGNAL_CONFIG_COMPLETE;
    } else if (radio_idx == wps_phy_multi.leading_radio_idx) {
        /* send the signal to process callback when the leading radio have finished to prepare the next frame */
        if ((leading_signal == PHY_SIGNAL_PREPARE_DONE) || (leading_signal == PHY_SIGNAL_CONNECT)) {
            return leading_signal;
            /* send the end of frame signal when both radio frame processing is done.*/
        } else if (!is_frame_processing(leading_signal, wps_phy_multi.leading_radio_idx) &&
                   !is_frame_processing(following_signal, wps_phy_multi.following_radio_idx)) {
            return leading_signal;
            /* Yield if one of both radio is still processing the frame. */
        } else {
            return PHY_SIGNAL_YIELD;
        }
    } else {
        /* Yield if one of both radio is still processing the frame. */
        if (is_frame_processing(leading_signal, wps_phy_multi.leading_radio_idx) || is_frame_processing(following_signal, wps_phy_multi.following_radio_idx)) {
            return PHY_SIGNAL_YIELD;
        } else {
            return leading_signal;
        }
    }
    return leading_signal;
}

phy_output_signal_t wps_phy_get_auto_signal(wps_phy_t *wps_phy)
{
    uint8_t leading_radio_idx = link_multi_radio_get_replying_radio(&wps_phy_multi.multi_radio);

    return phy_get_auto_signal(&wps_phy[leading_radio_idx]);
}

void wps_phy_set_main_xlayer(wps_phy_t *wps_phy, xlayer_t *xlayer, xlayer_cfg_internal_t *xlayer_cfg)
{
    uint8_t previous_leading_radio_idx;
    uint8_t current_leading_radio_idx;

    /* Store previous leading radio index. */
    previous_leading_radio_idx = link_multi_radio_get_replying_radio(&wps_phy_multi.multi_radio);
    /* Update leading radio. */
    link_multi_radio_update(&wps_phy_multi.multi_radio);
    current_leading_radio_idx = link_multi_radio_get_replying_radio(&wps_phy_multi.multi_radio);
    /* Update single radio processing if leading radio switched. */
    if ((wps_phy_multi.multi_radio.mode == MULTI_RADIO_MODE_1) &&
        (previous_leading_radio_idx != current_leading_radio_idx)) {
        single_radio_processing_switch_radio(wps_phy);
    }

    wps_phy_multi.following_main_xlayer = *xlayer;
    wps_phy_multi.following_xlayer_cfg = *xlayer_cfg;

    if (xlayer->frame.source_address == wps_phy->local_address) {
        /* Following radio does not transmit packets. */
        wps_phy_multi.following_main_xlayer.frame.header_memory       = NULL;
        wps_phy_multi.following_main_xlayer.frame.header_begin_it     = NULL;
        wps_phy_multi.following_main_xlayer.frame.header_end_it       = NULL;
        wps_phy_multi.following_main_xlayer.frame.header_memory_size  = 0;
        wps_phy_multi.following_main_xlayer.frame.payload_memory      = NULL;
        wps_phy_multi.following_main_xlayer.frame.payload_begin_it    = NULL;
        wps_phy_multi.following_main_xlayer.frame.payload_end_it      = NULL;
        wps_phy_multi.following_main_xlayer.frame.payload_memory_size = 0;
    } else {
        /* Following radio does not transmit ack. */
        wps_phy_multi.following_xlayer_cfg.expect_ack = false;
    }

    rf_channel_t *channel = xlayer_cfg->channel;

    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        if (i == current_leading_radio_idx) {
            xlayer_cfg->channel = &channel[i];
            xlayer_cfg->rx_constgain = link_gain_loop_get_gain_value(&xlayer_cfg->gain_loop[i]);
            phy_set_main_xlayer(&wps_phy[i], xlayer, xlayer_cfg);
        } else {
            wps_phy_multi.following_xlayer_cfg.channel      = &channel[i];
            wps_phy_multi.following_xlayer_cfg.rx_constgain = link_gain_loop_get_gain_value(
                &xlayer_cfg->gain_loop[i]);
            wps_phy_multi.following_xlayer_cfg.certification_header_en = false;
            phy_set_main_xlayer(&wps_phy[i], &wps_phy_multi.following_main_xlayer, &wps_phy_multi.following_xlayer_cfg);
        }
    }
}

void wps_phy_set_auto_xlayer(wps_phy_t *wps_phy, xlayer_t *xlayer)
{
    if (xlayer == NULL) {
        for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
            phy_set_auto_xlayer(&wps_phy[i], NULL);
        }
        return;
    }

    uint8_t leading_radio_idx = link_multi_radio_get_replying_radio(&wps_phy_multi.multi_radio);

    if (xlayer->frame.source_address == wps_phy->local_address) {
        for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
            if (i == leading_radio_idx) {
                phy_set_auto_xlayer(&wps_phy[i], xlayer);
            } else {
                phy_set_auto_xlayer(&wps_phy[i], NULL);
            }
        }
    } else {
        wps_phy_multi.following_auto_xlayer                   = *xlayer;

        for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
            if (i == leading_radio_idx) {
                phy_set_auto_xlayer(&wps_phy[i], xlayer);
            } else {
                phy_set_auto_xlayer(&wps_phy[i], &wps_phy_multi.following_auto_xlayer);
            }
        }
    }
}

void wps_phy_end_process(wps_phy_t *wps_phy)
{
    for (int radio_idx = 0; radio_idx < WPS_RADIO_COUNT; radio_idx++) {
        uint8_t gain_index = link_gain_loop_get_gain_index(&wps_phy->config->gain_loop[radio_idx]);

        link_lqi_update(&wps_phy_multi.multi_radio.radios_lqi[radio_idx], gain_index,
                        wps_phy[radio_idx].xlayer_main->frame.frame_outcome,
                        wps_phy[radio_idx].config->rssi_raw, wps_phy[radio_idx].config->rnsi_raw,
                        wps_phy[radio_idx].config->phase_offset);

        /* Update gain loop */
        link_gain_loop_update(&wps_phy->config->gain_loop[radio_idx],
                              wps_phy[radio_idx].xlayer_main->frame.frame_outcome,
                              wps_phy[radio_idx].config->rssi_raw);
    }
}

void wps_phy_multi_process_radio_timer(wps_phy_t *wps_phy)
{
    /* If xlayer_main is NULL (connect event) or the current timeslot is RX or TX wakeup mode is manual,
     * process the radio timer.
     */
    if (wps_phy->xlayer_main == NULL || (wps_phy_multi_get_tx_wakeup_mode() == MULTI_TX_WAKEUP_MODE_MANUAL) ||
        (wps_phy->xlayer_main->frame.destination_address == wps_phy->local_address)) {
        /* Check if config is complete. */
        for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
            if (wps_phy[i].signal_main != PHY_SIGNAL_CONFIG_COMPLETE) {
                swc_hal_timer_multi_radio_timer_set_period(
                    US_TO_S(MULTI_RADIO_RETRY_TIMER_PERIOD_US * wps_phy_multi.timer_frequency_hz));
                return;
            }
        }
        /* Sync timer on frame start. */
        swc_hal_timer_multi_radio_timer_set_max_period();

        for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
            if ((i == wps_phy_multi.leading_radio_idx) || (wps_phy_multi.multi_radio.mode == MULTI_RADIO_MODE_0)) {
                if (!is_frame_done(wps_phy[i].signal_main, i)) {
                    phy_wakeup_multi(&wps_phy[i]);
                }
            }
        }
        for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
            if ((i == wps_phy_multi.leading_radio_idx) || (wps_phy_multi.multi_radio.mode == MULTI_RADIO_MODE_0)) {
                if (!is_frame_done(wps_phy[i].signal_main, i)) {
                    /* Set CS to indicate to radio that this is a new transfer. */
                    sr_access_close(i);
                }
            }
        }
    } else {
        /* Sync timer on frame start. */
        swc_hal_timer_multi_radio_timer_set_max_period();
    }
}

void wps_phy_write_register(wps_phy_t *wps_phy, uint8_t starting_reg, uint16_t data,
                            reg_write_cfg_t cfg)
{
    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        phy_write_register(&wps_phy[i], starting_reg, data, cfg);
    }
}

void wps_phy_clear_write_register(wps_phy_t *wps_phy)
{
    phy_clear_write_register(wps_phy);
}

void wps_phy_read_register(wps_phy_t *wps_phy, uint8_t target_register, uint16_t *rx_buffer,
                           bool *xfer_cmplt)
{
    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        phy_read_register(&wps_phy[i], target_register, rx_buffer, xfer_cmplt);
    }
}

void wps_phy_enable_debug_feature(wps_phy_t *wps_phy, phy_debug_cfg_t *phy_debug)
{
    phy_enable_debug_feature(wps_phy, phy_debug);
}

uint8_t wps_phy_multi_get_replying_radio(void)
{
    return link_multi_radio_get_replying_radio(&wps_phy_multi.multi_radio);
}

/* PRIVATE FUNCTIONS **********************************************************/
static bool is_frame_done(phy_output_signal_t output_signal, uint8_t index)
{
    if ((index == wps_phy_multi.following_radio_idx) && (wps_phy_multi.multi_radio.mode == MULTI_RADIO_MODE_1)) {
        return true;
    }
    return (output_signal > PHY_SIGNAL_PREPARE_DONE);
}

static bool is_frame_processing(phy_output_signal_t output_signal, uint8_t index)
{
    if ((index == wps_phy_multi.following_radio_idx) && (wps_phy_multi.multi_radio.mode == MULTI_RADIO_MODE_1)) {
        return false;
    }
    return (output_signal < PHY_SIGNAL_PREPARE_DONE);
}

static void single_radio_processing_switch_radio(wps_phy_t *wps_phy)
{
    for (size_t i = 0; i < WPS_RADIO_COUNT; i++) {
        if (i == link_multi_radio_get_replying_radio(&wps_phy_multi.multi_radio)) {
            wps_phy_multi.leading_radio_idx = i;
        } else {
            wps_phy_multi.following_radio_idx = i;
        }
    }
    wps_phy[wps_phy_multi.leading_radio_idx].signal_main = wps_phy[wps_phy_multi.following_radio_idx].signal_main;
    wps_phy[wps_phy_multi.leading_radio_idx].signal_auto = wps_phy[wps_phy_multi.following_radio_idx].signal_auto;
    wps_phy[wps_phy_multi.following_radio_idx].signal_main = PHY_SIGNAL_YIELD;
    wps_phy[wps_phy_multi.following_radio_idx].signal_auto = PHY_SIGNAL_YIELD;
    phy_enqueue_prepare(&wps_phy[wps_phy_multi.leading_radio_idx]);
    phy_enqueue_none(&wps_phy[wps_phy_multi.following_radio_idx]);
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
