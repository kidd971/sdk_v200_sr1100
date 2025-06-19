/** @file  swc_api_extension.c
 *  @brief SPARK Wireless Core Application Programming Interface extension.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "swc_api_extension.h"
#include "swc_api.h"

/* CONSTANTS ******************************************************************/

/*! Fallback threshold max count. */
#ifndef WPS_FALLBACK_THRESHOLD_COUNT_MAX
#define WPS_FALLBACK_THRESHOLD_COUNT_MAX 32
#endif

/*! Minimal pulse count. */
#define PULSE_COUNT_MIN 1
/*! Maximal pulse count. */
#define PULSE_COUNT_MAX 3
/*! Maximal pulse width. */
#define PULSE_WIDTH_MAX 7
/*! Maximal pulse gain. */
#define PULSE_GAIN_MAX 7
/*! Maximal clear channel assessment threshold (range 0 to 115). */
#define CCA_THRESH_MAX 115
/*! Maximum time for which the radio will sample before transmitting a frame (CCAONTIME+1)*8. */
#define CCA_ON_TIME_MAX 128
/*! Maximum CCA retry time. */
#define CCA_RETRY_TIME_MAX 2048
/*! The radio's maximum payload size is 256, one byte must be reserved for the header size. */
#define FRAME_SIZE_MAX 255
/*! Preamble length limit. */
#define PREAMBLE_LENGTH_REG_HIGHER_LIMIT 63
/*! Receiver gain max value. */
#define RX_GAIN_MAX 127
/*! Register first address value. */
#define REG_FIRST_ADDRESS 0x01
/*! Register last address value. */
#define REG_LAST_ADDRESS 0x3F
/*! Minimum value for DDCM sync loss in PLL. */
#define DDCM_SYNC_LOSS_MIN 1024  // 50us
/*! Maximum value for DDCM sync loss in PLL. */
#define DDCM_SYNC_LOSS_MAX 454656  // 22.2ms
/*! Maximum value for the max timeslot offset parameter. */
#define DDCM_MAX_TIMESLOT_OFFSET 79

/* MACROS *********************************************************************/
#define ARRAY_SIZE(a)                          (sizeof(a) / sizeof(*a))
#define EXTRACT_NETWORK_ID(addr, msbits_count) (addr >> (16 - msbits_count))
#define CHECK_ERROR(cond, err_ptr, err_code, ret) \
    do {                                          \
        if (cond) {                               \
            *(err_ptr) = (err_code);              \
            ret;                                  \
        }                                         \
    } while (0)

#define CHECK_WARNING(cond, warn_ptr, warn_code) \
    do {                                         \
        if (cond) {                              \
            *(warn_ptr) |= (warn_code);          \
        }                                        \
    } while (0)

/* EXTERNS ********************************************************************/
/* This public global variable comes from the swc_api.c compilation unit and is necessary
 * for this compilation unit to work as an extension of swc_api.c
 */
extern wps_t wps;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static bool is_rx_connection(uint8_t local_address, uint8_t source_address);
static bool has_main_timeslot(const int32_t *const timeslot_id, uint32_t timeslot_count);
static bool is_sfd_length_valid(sfd_length_t sfd_length);

/* PUBLIC FUNCTIONS ***********************************************************/
void swc_ext_set_sfd(const swc_node_t *const node, uint32_t sfd_idx, swc_ext_error_t *err, swc_ext_warning_t *warn)
{
    *err = SWC_EXT_ERR_NONE;

    (void)warn;

    CHECK_ERROR(node == NULL, err, SWC_EXT_ERR_NULL_PTR, return);
    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR(sfd_idx >= ARRAY_SIZE(sfd_table), err, SWC_EXT_ERR_SFD_INDEX_OUT_OF_RANGE, return);

    node->wps_node_handle->cfg.sfd_cfg.sfd = sfd_table[sfd_idx];
}

void swc_ext_set_sfd_length(const swc_node_t *const node, sfd_length_t sfd_length, swc_ext_error_t *err,
                            swc_ext_warning_t *warn)
{
    *err = SWC_EXT_ERR_NONE;

    (void)warn;

    CHECK_ERROR(node == NULL, err, SWC_EXT_ERR_NULL_PTR, return);
    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR(is_sfd_length_valid(sfd_length) == false, err, SWC_EXT_ERR_SFD_LENGTH_INVALID, return);

    node->wps_node_handle->cfg.sfd_cfg.sfd_length = sfd_length;

    uint8_t isi_mitigation_pauses = link_tdma_sync_get_isi_mitigation_pauses(wps.node->cfg.isi_mitig);

    link_tdma_set_sfd_length(&wps.mac.tdma_sync, isi_mitigation_pauses, sfd_length);
}

void swc_ext_set_preamble_length(const swc_node_t *const node, uint32_t preamble_length_reg, swc_ext_error_t *err,
                                 swc_ext_warning_t *warn)
{
    *err = SWC_EXT_ERR_NONE;

    (void)warn;

    CHECK_ERROR(node == NULL, err, SWC_EXT_ERR_NULL_PTR, return);
    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR((preamble_length_reg > PREAMBLE_LENGTH_REG_HIGHER_LIMIT), err, SWC_EXT_ERR_PREAMBLE_LENGTH_OUT_OF_RANGE,
                return);

    uint8_t isi_mitigation_pauses = link_tdma_sync_get_isi_mitigation_pauses(wps.node->cfg.isi_mitig);
    uint8_t sfd_reg_value = node->wps_node_handle->cfg.sfd_cfg.sfd_length;
    uint32_t preamble_length_bits = link_tdma_get_preamble_length(isi_mitigation_pauses, preamble_length_reg,
                                                                  sfd_reg_value);
    node->wps_node_handle->cfg.preamble_len = preamble_length_bits;
    link_tdma_set_preamble_length(&wps.mac.tdma_sync, isi_mitigation_pauses, preamble_length_reg, sfd_reg_value);
}

void swc_ext_set_crc(const swc_node_t *const node, uint32_t crc, swc_ext_error_t *err, swc_ext_warning_t *warn)
{
    *err = SWC_EXT_ERR_NONE;

    (void)warn;

    CHECK_ERROR(node == NULL, err, SWC_EXT_ERR_NULL_PTR, return);
    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);

    node->wps_node_handle->cfg.crc_polynomial = crc;
}

void swc_ext_enable_rdo(swc_ext_error_t *err, swc_ext_warning_t *warn)
{
    wps_error_t wps_err = WPS_NO_ERROR;
    *err = SWC_EXT_ERR_NONE;

    (void)warn;

    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);

    wps_enable_rdo(&wps, &wps_err);
    CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_EXT_ERR_INTERNAL, return);
}

void swc_ext_disable_rdo(swc_ext_error_t *err, swc_ext_warning_t *warn)
{
    wps_error_t wps_err = WPS_NO_ERROR;
    *err = SWC_EXT_ERR_NONE;

    (void)warn;

    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);

    wps_disable_rdo(&wps, &wps_err);
    CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_EXT_ERR_INTERNAL, return);
}

void swc_ext_enable_tx_jitter(const swc_node_t *const node, swc_ext_error_t *err, swc_ext_warning_t *warn)
{
    *err = SWC_EXT_ERR_NONE;

    (void)warn;

    CHECK_ERROR(node == NULL, err, SWC_EXT_ERR_NULL_PTR, return);
    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);

    wps.mac.tdma_sync.tx_jitter_enabled = true;
    node->wps_node_handle->cfg.tx_jitter_enabled = true;
}

void swc_ext_disable_tx_jitter(const swc_node_t *const node, swc_ext_error_t *err, swc_ext_warning_t *warn)
{
    *err = SWC_EXT_ERR_NONE;

    (void)warn;

    CHECK_ERROR(node == NULL, err, SWC_EXT_ERR_NULL_PTR, return);
    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);

    wps.mac.tdma_sync.tx_jitter_enabled = false;
    node->wps_node_handle->cfg.tx_jitter_enabled = false;
}

void swc_ext_connection_disable_gain_loop(const swc_connection_t *const conn, uint8_t rx_gain, swc_ext_error_t *err,
                                          swc_ext_warning_t *warn)
{
    wps_error_t wps_err = WPS_NO_ERROR;
    *err = SWC_EXT_ERR_NONE;

    (void)warn;

    CHECK_ERROR(conn == NULL, err, SWC_EXT_ERR_NULL_PTR, return);
    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR(rx_gain > RX_GAIN_MAX, err, SWC_EXT_ERR_RX_GAIN_OUT_OF_RANGE, return);

    wps_connection_disable_gain_loop(conn->wps_conn_handle, rx_gain, &wps_err);
    CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_EXT_ERR_INTERNAL, return);
}

void swc_ext_connection_enable_gain_loop(const swc_connection_t *const conn, swc_ext_error_t *err,
                                         swc_ext_warning_t *warn)
{
    wps_error_t wps_err = WPS_NO_ERROR;
    *err = SWC_EXT_ERR_NONE;

    (void)warn;

    CHECK_ERROR(conn == NULL, err, SWC_EXT_ERR_NULL_PTR, return);
    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);

    wps_connection_enable_gain_loop(conn->wps_conn_handle, &wps_err);
    CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_EXT_ERR_INTERNAL, return);
}

void swc_ext_pll_disable(const swc_node_t *const node, swc_ext_error_t *err, swc_ext_warning_t *warn)
{
    *err = SWC_EXT_ERR_NONE;

    (void)warn;

    CHECK_ERROR(node == NULL, err, SWC_EXT_ERR_NULL_PTR, return);
    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);

    for (uint8_t i = 0; i < SWC_RADIO_COUNT; i++) {
        node->wps_radio_handle[i].radio.clock_source.pll_clk_source = CHIP_CLK_INTERNAL_OUTPUT_HIGH_IMPED;
    }
}

void swc_ext_pll_enable(const swc_node_t *const node, swc_ext_error_t *err, swc_ext_warning_t *warn)
{
    *err = SWC_EXT_ERR_NONE;

    (void)warn;

    CHECK_ERROR(node == NULL, err, SWC_EXT_ERR_NULL_PTR, return);
    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);

    for (uint8_t i = 0; i < SWC_RADIO_COUNT; i++) {
        node->wps_radio_handle[i].radio.clock_source.pll_clk_source = CHIP_CLK_EXTERNAL_INTERNAL_ENABLE;
    }
}

void swc_ext_xtal_enable(swc_node_t *node, swc_ext_error_t *err, swc_ext_warning_t *warn)
{
    *err = SWC_EXT_ERR_NONE;

    (void)warn;

    CHECK_ERROR(node == NULL, err, SWC_EXT_ERR_NULL_PTR, return);
    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);

    for (uint8_t i = 0; i < SWC_RADIO_COUNT; i++) {
        node->wps_radio_handle[i].radio.clock_source.xtal_clk_source = XTAL_CLK_BOTH_ENABLE;
    }
}

void swc_ext_xtal_disable(const swc_node_t *const node, swc_ext_error_t *err, swc_ext_warning_t *warn)
{
    *err = SWC_EXT_ERR_NONE;

    (void)warn;

    CHECK_ERROR(node == NULL, err, SWC_EXT_ERR_NULL_PTR, return);
    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);

    for (uint8_t i = 0; i < SWC_RADIO_COUNT; i++) {
        node->wps_radio_handle[i].radio.clock_source.xtal_clk_source = XTAL_CLK_INTERNAL_OUTPUT_HIGH_IMPED;
    }
}

void swc_ext_set_connection_channel(const swc_node_t *const node, const swc_connection_t *const conn,
                                    channel_cfg_t new_channel, uint8_t chan_idx, uint8_t fallback_idx,
                                    swc_ext_error_t *err, swc_ext_warning_t *warn)
{
    wps_error_t wps_err = WPS_NO_ERROR;
    bool is_rx_conn;
    bool is_tx_conn;
    bool has_main_ts;

    *err = SWC_EXT_ERR_NONE;

    CHECK_ERROR(node == NULL, err, SWC_EXT_ERR_NULL_PTR, return);
    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR((conn == NULL) || (node == NULL), err, SWC_EXT_ERR_NULL_PTR, return);

    is_rx_conn = is_rx_connection(node->cfg.local_address, conn->cfg.source_address);
    is_tx_conn = !is_rx_conn;
    has_main_ts = has_main_timeslot(conn->cfg.timeslot_id, conn->cfg.timeslot_count);

    CHECK_ERROR(has_main_ts == false, err, SWC_EXT_ERR_ADD_CHANNEL_ON_INVALID_CONNECTION, return);

    if (is_tx_conn || (is_rx_conn && conn->wps_conn_handle->ack_enable)) {
        CHECK_WARNING((new_channel.pulse_count < PULSE_COUNT_MIN), warn, SWC_EXT_WARNING_ZERO_PULSE_COUNT);
        CHECK_WARNING((new_channel.pulse_count > PULSE_COUNT_MAX), warn, SWC_EXT_WARNING_HIGH_PULSE_COUNT);
        CHECK_ERROR(*new_channel.pulse_width_table > PULSE_WIDTH_MAX, err, SWC_EXT_ERR_TX_PULSE_WIDTH, return);
        CHECK_ERROR(new_channel.tx_gain > PULSE_GAIN_MAX, err, SWC_EXT_ERR_TX_PULSE_GAIN, return);
    }

    uint8_t local_integgain = swc_ext_get_integgain(wps.chip_rate, new_channel.pulse_count, err, warn);

    CHECK_WARNING((new_channel.integrators_gain != local_integgain), warn,
                  SWC_EXT_WARNING_INTEGGAIN_VALUE_OUT_OF_LOOKUP_TABLE);

    if (fallback_idx == 0) {
        wps_connection_config_channel_20_48(conn->wps_conn_handle, node->wps_node_handle, chan_idx, &new_channel,
                                            &wps_err);
        CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_EXT_ERR_INTERNAL, return);
    } else {
        wps_connection_config_fallback_channel_20_48(conn->wps_conn_handle, node->wps_node_handle, chan_idx,
                                                     (fallback_idx - 1), &new_channel, &wps_err);
        CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_EXT_ERR_INTERNAL, return);
    }
}

void swc_ext_set_network_msbit_count(uint8_t msbits_count, swc_ext_error_t *err, swc_ext_warning_t *warn)
{
    wps_error_t wps_err = WPS_NO_ERROR;
    *err = SWC_EXT_ERR_NONE;

    (void)warn;

    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);

    wps_set_network_id(&wps, EXTRACT_NETWORK_ID(wps.mac.syncing_address, msbits_count), &wps_err);
    CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_EXT_ERR_INTERNAL, return);
}

void swc_ext_set_chip_rate(swc_node_t *node, chip_rate_cfg_t chip_rate, swc_ext_error_t *err, swc_ext_warning_t *warn)
{
    *err = SWC_EXT_ERR_NONE;

    (void)warn;

    CHECK_ERROR(node == NULL, err, SWC_EXT_ERR_NULL_PTR, return);
    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);

    wps.chip_rate = chip_rate;
}

uint8_t swc_ext_get_integgain(chip_rate_cfg_t chip_rate, uint8_t pulse_count, swc_ext_error_t *err,
                              swc_ext_warning_t *warn)
{
    *err = SWC_EXT_ERR_NONE;

    (void)warn;

    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING,
                return DEFAULT_INTEGGAIN);

    if (chip_rate == CHIP_RATE_20_48_MHZ) {
        if (pulse_count == 1) {
            return INTEGGAIN_20_48_PC1;
        } else if (pulse_count == 2) {
            return INTEGGAIN_20_48_PC2;
        } else {
            return INTEGGAIN_20_48_PCX;
        }
    } else if (chip_rate == CHIP_RATE_27_30_MHZ) {
        if (pulse_count == 1) {
            return INTEGGAIN_27_30_PC1;
        } else if (pulse_count == 2) {
            return INTEGGAIN_27_30_PC2;
        } else {
            return INTEGGAIN_27_30_PCX;
        }
    } else {
        if (pulse_count == 1) {
            return INTEGGAIN_40_96_PC1;
        } else if (pulse_count == 2) {
            return INTEGGAIN_40_96_PC2;
        } else {
            return INTEGGAIN_40_96_PCX;
        }
    }
}

void swc_ext_enable_ddcm(uint16_t max_timeslot_offset, uint32_t sync_loss_max_duration_pll, swc_ext_error_t *err,
                         swc_ext_warning_t *warn)
{
    *err = SWC_EXT_ERR_NONE;

    (void)warn;

    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR(max_timeslot_offset > DDCM_MAX_TIMESLOT_OFFSET, err, SWC_EXT_ERR_DDCM_MAX_TIMESLOT_OFFSET_OUT_OF_RANGE,
                return);
    CHECK_ERROR((sync_loss_max_duration_pll < DDCM_SYNC_LOSS_MIN) && (sync_loss_max_duration_pll > DDCM_SYNC_LOSS_MAX),
                err, SWC_EXT_ERR_DDCM_SYNC_LOSS_VALUE_OUT_OF_RANGE, return);

    link_ddcm_init(&wps.mac.link_ddcm, max_timeslot_offset, sync_loss_max_duration_pll);
}

void swc_ext_disable_ddcm(swc_ext_error_t *err, swc_ext_warning_t *warn)
{
    *err = SWC_EXT_ERR_NONE;

    (void)warn;

    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);

    link_ddcm_init(&wps.mac.link_ddcm, DDCM_DISABLE, DDCM_DISABLE);
}

void swc_ext_enable_fallback(const swc_connection_t *const conn, const uint8_t *const threshold,
                             uint8_t threshold_count, swc_ext_error_t *err, swc_ext_warning_t *warn)
{
    *err = SWC_EXT_ERR_NONE;

    (void)warn;

    CHECK_ERROR(conn == NULL, err, SWC_EXT_ERR_NULL_PTR, return);
    CHECK_ERROR(threshold == NULL, err, SWC_EXT_ERR_NULL_PTR, return);
    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR(threshold_count > WPS_FALLBACK_THRESHOLD_COUNT_MAX, err, SWC_EXT_ERR_NULL_PTR, return);

    link_fallback_init(&conn->wps_conn_handle->link_fallback, threshold, threshold_count);
}

void swc_ext_disable_fallback(const swc_connection_t *const conn, swc_ext_error_t *err, swc_ext_warning_t *warn)
{
    *err = SWC_EXT_ERR_NONE;

    (void)warn;

    CHECK_ERROR(conn == NULL, err, SWC_EXT_ERR_NULL_PTR, return);
    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);

    link_fallback_init(&conn->wps_conn_handle->link_fallback, NULL, 0);
}

void swc_ext_set_cca_threshold(const swc_connection_t *const conn, const uint8_t cca_threshold, swc_ext_error_t *err,
                               swc_ext_warning_t *warn)
{
    *err = SWC_EXT_ERR_NONE;

    (void)warn;

    CHECK_ERROR(conn == NULL, err, SWC_EXT_ERR_NULL_PTR, return);
    CHECK_ERROR(cca_threshold > CCA_THRESH_MAX, err, SWC_EXT_ERR_CCA_THRESHOLD_OUT_OF_RANGE, return);
    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);

    conn->wps_conn_handle->cca.threshold = cca_threshold;
}

void swc_ext_set_cca_retry_time(const swc_connection_t *const conn, const uint16_t cca_retry_time, swc_ext_error_t *err,
                                swc_ext_warning_t *warn)
{
    *err = SWC_EXT_ERR_NONE;

    (void)warn;

    CHECK_ERROR(conn == NULL, err, SWC_EXT_ERR_NULL_PTR, return);
    CHECK_ERROR(cca_retry_time > CCA_RETRY_TIME_MAX, err, SWC_EXT_ERR_CCA_RETRY_TIME_OUT_OF_RANGE, return);
    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);

    conn->wps_conn_handle->cca.retry_time_pll_cycles = CCA_RETRY_TIME_PLL_TO_REG(cca_retry_time);
#if WPS_ENABLE_PHY_STATS_PER_BANDS
    size_t channel_count = conn->wps_conn_handle->max_channel_count;

    /* Initialize fixed QOS member */
    for (uint8_t i_chan = 0; i_chan < channel_count; i_chan++) {
        conn->qos_indicators_per_bands[i_chan].cca_retry_time = conn->wps_conn_handle->cca.retry_time_pll_cycles;
    }
#endif
}

void swc_ext_set_cca_on_time(const swc_connection_t *const conn, const uint8_t cca_on_time, swc_ext_error_t *err,
                             swc_ext_warning_t *warn)
{
    *err = SWC_EXT_ERR_NONE;

    (void)warn;

    CHECK_ERROR(conn == NULL, err, SWC_EXT_ERR_NULL_PTR, return);
    CHECK_ERROR(cca_on_time > CCA_ON_TIME_MAX, err, SWC_EXT_ERR_CCA_ON_TIME_OUT_OF_RANGE, return);
    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);

    conn->wps_conn_handle->cca.on_time_pll_cycles = CCA_ON_TIME_REG_TO_PLL(cca_on_time);
#if WPS_ENABLE_PHY_STATS_PER_BANDS
    size_t channel_count = conn->wps_conn_handle->max_channel_count;

    /* Initialize fixed QOS member */
    for (uint8_t i_chan = 0; i_chan < channel_count; i_chan++) {
        conn->qos_indicators_per_bands[i_chan].cca_on_time_pll_cycles = conn->wps_conn_handle->cca.on_time_pll_cycles;
    }
#endif
}

void swc_ext_set_fallback_cca_try_count(const swc_connection_t *const conn, const uint8_t *const fbk_try_count,
                                        swc_ext_error_t *err, swc_ext_warning_t *warn)
{
    *err = SWC_EXT_ERR_NONE;

    (void)warn;

    CHECK_ERROR(conn == NULL, err, SWC_EXT_ERR_NULL_PTR, return);
    CHECK_ERROR(fbk_try_count == NULL, err, SWC_EXT_ERR_NULL_PTR, return);
    CHECK_ERROR(swc_get_status() == SWC_STATUS_RUNNING, err, SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);

    conn->wps_conn_handle->cca.fbk_try_count = fbk_try_count;
}

void swc_ext_request_register_read(uint8_t target_register, uint16_t *const rx_buffer, volatile bool *xfer_cmplt,
                                   swc_ext_error_t *err)
{
    *err = SWC_EXT_ERR_NONE;
    wps_error_t wps_err = WPS_NO_ERROR;

    CHECK_ERROR((target_register < REG_FIRST_ADDRESS) || (target_register > REG_LAST_ADDRESS), err,
                SWC_EXT_ERR_REQUESTED_ADDRESS_REGISTER_OUT_OF_RANGE, return);
    wps_request_read_register(&wps, target_register, rx_buffer, xfer_cmplt, &wps_err);
    CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_EXT_ERR_READ_REQUEST_QUEUE_FULL, return);
}

void swc_ext_request_register_write(uint8_t target_register, uint16_t data, reg_write_cfg_t cfg, swc_ext_error_t *err)
{
    *err = SWC_EXT_ERR_NONE;
    wps_error_t wps_err = WPS_NO_ERROR;

    CHECK_ERROR((target_register < REG_FIRST_ADDRESS) || (target_register > REG_LAST_ADDRESS), err,
                SWC_EXT_ERR_REQUESTED_ADDRESS_REGISTER_OUT_OF_RANGE, return);
    wps_request_write_register(&wps, target_register, data, cfg, &wps_err);
    CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_EXT_ERR_WRITE_REQUEST_QUEUE_FULL, return);
}

void swc_ext_clear_register_write(swc_ext_error_t *err)
{
    *err = SWC_EXT_ERR_NONE;

    wps_clear_write_register(&wps);
}

calib_vars_t *swc_ext_get_calib_vars_20_48(uint8_t radio_num, swc_ext_error_t *err)
{
    *err = SWC_EXT_ERR_NONE;
    CHECK_ERROR(radio_num >= WPS_RADIO_COUNT, err, SWC_EXT_ERR_REQUESTED_RADIO_UNAVAILABLE, return NULL);
    CHECK_ERROR(wps.node->radio[radio_num].spectral_calib_vars_20_48 == NULL, err, SWC_EXT_ERR_CALIBRATION_NOT_DONE,
                return NULL);
    return wps.node->radio[radio_num].spectral_calib_vars_20_48;
}

calib_vars_t *swc_ext_get_calib_vars_40_96(uint8_t radio_num, swc_ext_error_t *err)
{
    *err = SWC_EXT_ERR_NONE;
    CHECK_ERROR(radio_num >= WPS_RADIO_COUNT, err, SWC_EXT_ERR_REQUESTED_RADIO_UNAVAILABLE, return NULL);
    CHECK_ERROR(wps.node->radio[radio_num].spectral_calib_vars_40_96 == NULL, err, SWC_EXT_ERR_CALIBRATION_NOT_DONE,
                return NULL);
    return wps.node->radio[radio_num].spectral_calib_vars_40_96;
}

rf_channel_t *swc_ext_connection_get_channel_info_20_48(const swc_connection_t *const connection, uint8_t channel_num,
                                                        uint8_t fallback_idx, uint8_t radio_num, swc_ext_error_t *err)
{
    *err = SWC_EXT_ERR_NONE;
    CHECK_ERROR(connection == NULL, err, SWC_EXT_ERR_UNINITIALIZED_CONNECTION, return NULL);
    CHECK_ERROR(radio_num >= WPS_RADIO_COUNT, err, SWC_EXT_ERR_REQUESTED_RADIO_UNAVAILABLE, return NULL);
    CHECK_ERROR(connection->wps_conn_handle->channel_20_48 == NULL, err, SWC_EXT_ERR_UNINITIALIZED_CHANNEL,
                return NULL);

    if (fallback_idx == 0) {
        return &connection->wps_conn_handle->channel_20_48[channel_num][radio_num];
    } else {
        return &connection->wps_conn_handle->fallback_channel_20_48[channel_num][fallback_idx - 1][radio_num];
    }
}

rf_channel_t *swc_ext_connection_get_channel_info_40_96(const swc_connection_t *const connection, uint8_t channel_num,
                                                        uint8_t fallback_idx, uint8_t radio_num, swc_ext_error_t *err)
{
    *err = SWC_EXT_ERR_NONE;
    CHECK_ERROR(connection == NULL, err, SWC_EXT_ERR_UNINITIALIZED_CONNECTION, return NULL);
    CHECK_ERROR(radio_num >= WPS_RADIO_COUNT, err, SWC_EXT_ERR_REQUESTED_RADIO_UNAVAILABLE, return NULL);
    CHECK_ERROR(connection->wps_conn_handle->channel_40_96 == NULL, err, SWC_EXT_ERR_UNINITIALIZED_CHANNEL,
                return NULL);

    if (fallback_idx == 0) {
        return &connection->wps_conn_handle->channel_40_96[channel_num][radio_num];
    } else {
        return &connection->wps_conn_handle->fallback_channel_40_96[channel_num][fallback_idx - 1][radio_num];
    }
}

uint8_t swc_ext_get_number_of_configured_channel(swc_ext_error_t *err)
{
    *err = SWC_EXT_ERR_NONE;
    CHECK_ERROR(wps.channel_sequence.channel == NULL, err, SWC_EXT_ERR_UNINITIALIZED_CHANNEL, return 0);
    return wps.channel_sequence.channel_number;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Check if the connection is an RX one.
 *
 *  @param[in] local_address   Node's local address.
 *  @param[in] source_address  Connection's source address.
 *  @retval true  This is an RX connection.
 *  @retval false This is a TX connection.
 */
static bool is_rx_connection(uint8_t local_address, uint8_t source_address)
{
    return (local_address != source_address);
}

/** @brief Check if a connection is using at least on main timeslot.
 *
 *  @param[in] timeslot_id     ID of timeslots used by the connection.
 *  @param[in] timeslot_count  Number of timeslots used by the connection.
 *  @retval true  Connection is using at least one main timeslot.
 *  @retval false Connection is using only auto-reply timeslots.
 */
static bool has_main_timeslot(const int32_t *const timeslot_id, uint32_t timeslot_count)
{
    bool main_timeslot = false;

    for (uint32_t i = 0; i < timeslot_count; i++) {
        main_timeslot = !(timeslot_id[i] & BIT_AUTO_REPLY_TIMESLOT);
        if (main_timeslot) {
            break;
        }
    }

    return main_timeslot;
}

/** @brief Validate that the SFD length is valid.
 *
 *  @param[in] sfd_length  SFD length.
 *  @retval true  SFD is valid.
 *  @retval false SFD is invalid.
 */
static bool is_sfd_length_valid(sfd_length_t sfd_length)
{
    bool is_valid = false;

    if (sfd_length == SFD_LENGTH_32_OOK || sfd_length == SFD_LENGTH_16_1BIT_PPM ||
        sfd_length == SFD_LENGTH_32_1BIT_PPM || sfd_length == SFD_LENGTH_64_1BIT_PPM) {
        is_valid = true;
    }
    return is_valid;
}
