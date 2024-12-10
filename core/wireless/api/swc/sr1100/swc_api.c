/** @file  swc_api.c
 *  @brief SPARK Wireless Core Application Programming Interface.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "swc_api.h"
#include <stdio.h>
#include "mem_pool.h"
#include "swc_error.h"
#include "swc_utils.h"
#include "wps.h"
#if !WPS_DISABLE_FRAGMENTATION
#include "wps_frag.h"
#endif
#include "wps_stats.h"

/* CONSTANTS ******************************************************************/
/*! Default preamble length. */
#ifndef WPS_DEFAULT_PREAMBLE_LEN
#define WPS_DEFAULT_PREAMBLE_LEN OPTIMIZED_PREAMBLE_LEN
#endif

/*! Default CRC. */
#ifndef WPS_DEFAULT_CRC
#define WPS_DEFAULT_CRC 0x8FCC4AC9
#endif

/*! Default sync word length. */
#ifndef WPS_DEFAULT_SYNC_WORD_LEN
#define WPS_DEFAULT_SYNC_WORD_LEN SYNCWORD_LENGTH_32_1BIT_PPM
#endif

/*! Default callback queue size margin. */
#ifndef WPS_QUEUE_MARGIN
#define WPS_QUEUE_MARGIN 5
#endif

/*! Default frequency shift setting. */
#ifndef WPS_DEFAULT_FREQ_SHIFT
#define WPS_DEFAULT_FREQ_SHIFT false /* Not yet supported */
#endif

/*! Default pulse start position. */
#ifndef WPS_DEFAULT_PULSE_START_POS
#define WPS_DEFAULT_PULSE_START_POS 2
#endif

/*! Default pulse spacing. */
#ifndef WPS_DEFAULT_PULSE_SPACING
#define WPS_DEFAULT_PULSE_SPACING 1
#endif

/*! Default random data rate offset rollover value. */
#ifndef WPS_DEFAULT_RDO_ROLLOVER_VAL
#define WPS_DEFAULT_RDO_ROLLOVER_VAL 15
#endif

/*! Default random data rate offset increment interval. */
#ifndef WPS_DEFAULT_RDO_STEP_MS
#define WPS_DEFAULT_RDO_STEP_MS 10
#endif

/*! Default random data rate offset increment step. */
#ifndef WPS_DEFAULT_RDO_STEP_VALUE
#define WPS_DEFAULT_RDO_STEP_VALUE 1
#endif

/*! Default reception gain. */
#ifndef WPS_DEFAULT_RX_GAIN
#define WPS_DEFAULT_RX_GAIN 0
#endif

/*! Default transmission jitter. */
#ifndef WPS_DEFAULT_TX_JITTER
#define WPS_DEFAULT_TX_JITTER false
#endif

/*! Default multi radio average count for radio selection at runtime. */
#ifndef WPS_DEFAULT_MULTI_AVG_COUNT
#define WPS_DEFAULT_MULTI_AVG_COUNT 4
#endif

/*! Default multi radio mode state. */
#ifndef WPS_DEFAULT_MULTI_MODE
#define WPS_DEFAULT_MULTI_MODE MULTI_RADIO_MODE_0
#endif

/*! Default multi radio tx mode state. */
#ifndef WPS_DEFAULT_MULTI_TX_WAKEUP_MODE
#define WPS_DEFAULT_MULTI_TX_WAKEUP_MODE MULTI_TX_WAKEUP_MODE_MANUAL
#endif

/*! Default multi radio RSSI threshold. */
#ifndef WPS_DEFAULT_MULTI_RSSI_THRESH
#define WPS_DEFAULT_MULTI_RSSI_THRESH 25
#endif

/*! Default receiver integrator gain. */
#ifndef WPS_DEFAULT_INTEGGAIN
#define WPS_DEFAULT_INTEGGAIN 8
#endif

/*! Default pulse count value. */
#ifndef WPS_DEFAULT_PULSE_CFG_COUNT
#define WPS_DEFAULT_PULSE_CFG_COUNT 1
#endif

/*! Default consecutive successful received packet for connect state. */
#ifndef WPS_DEFAULT_CONNECT_STATUS_COUNT
#define WPS_DEFAULT_CONNECT_STATUS_COUNT 1
#endif

/*! Default consecutive missing received packet for disconnect state. */
#ifndef WPS_DEFAULT_DISCONNECT_STATUS_COUNT
#define WPS_DEFAULT_DISCONNECT_STATUS_COUNT 100
#endif

/*! Default maximal timeslot offset. */
#ifndef WPS_DEFAULT_MAX_TIMESLOT_OFFSET
#define WPS_DEFAULT_MAX_TIMESLOT_OFFSET 48
#endif

/*! Default synchronise frame lost maximal duration. */
#ifndef WPS_DEFAULT_SYNC_FRAME_LOST_MAX_DURATION
#define WPS_DEFAULT_SYNC_FRAME_LOST_MAX_DURATION ((uint32_t)409600) /* 409600 PLL cycles = 20 ms */
#endif

#ifndef WPS_DEFAULT_REQUEST_MEMORY_SIZE
#define WPS_DEFAULT_REQUEST_MEMORY_SIZE 5
#endif

/*! Default number of tries deadline for the Stop-and-Wait ARQ */
#ifndef WPS_DEFAULT_TRY_DEADLINE
#define WPS_DEFAULT_TRY_DEADLINE 0
#endif

/*! Default number of time ticks deadline for the Stop-and-Wait ARQ */
#ifndef WPS_DEFAULT_TIME_DEADLINE
#define WPS_DEFAULT_TIME_DEADLINE 0
#endif

/*! Default number of tries deadline for the Stop-and-Wait ARQ */
#ifndef WPS_DEFAULT_TRY_DEADLINE
#define WPS_DEFAULT_TRY_DEADLINE 0
#endif

/*! Default number of time ticks deadline for the Stop-and-Wait ARQ */
#ifndef WPS_DEFAULT_TIME_DEADLINE
#define WPS_DEFAULT_TIME_DEADLINE 0
#endif

/*! Default FEC ratio. */
#ifndef WPS_DEFAULT_FEC_RATIO
#define WPS_DEFAULT_FEC_RATIO FEC_LVL_3
#endif

/*! Default modulation method to inverted OOK */
#ifndef WPS_DEFAULT_MODULATION
#define WPS_DEFAULT_MODULATION MODULATION_IOOK
#endif

/*! Default sleep level */
#ifndef WPS_DEFAULT_SLEEP_LEVEL
#define WPS_DEFAULT_SLEEP_LEVEL SLEEP_IDLE
#endif

/*! Default priority */
#ifndef WPS_DEFAULT_PRIORITY
#define WPS_DEFAULT_PRIORITY 0
#endif

/*! Default timeslots priority */
#ifndef WPS_DEFAULT_SLOTS_PRIORITY
#define WPS_DEFAULT_SLOTS_PRIORITY NULL
#endif

/*! Connection ID protocol is disabled by default */
#ifndef WPS_DEFAULT_CONNECTION_ID
#define WPS_DEFAULT_CONNECTION_ID false
#endif

/*! Default CCA threshold for air traffic detection */
#ifndef WPS_DEFAULT_CCA_THRESHOLD
#define WPS_DEFAULT_CCA_THRESHOLD 65
#endif

/*! Default CCA ON time */
#ifndef WPS_DEFAULT_CCA_ON_TIME_PLL_CYCLES
#define WPS_DEFAULT_CCA_ON_TIME_PLL_CYCLES 32
#endif

#define DEFAULT_CCA_HP_RETRY_TIME 512 /* (512 * 48.8 ns -> 25 us) */
#define DEFAULT_CCA_HP_TRY_COUNT  2
#define DEFAULT_CCA_LP_RETRY_TIME CCAINTERV_MIN_VALUE
#define DEFAULT_CCA_LP_TRY_COUNT  1

/*! Default ranging setting */
#ifndef WPS_DEFAULT_RANGING
#define WPS_DEFAULT_RANGING WPS_RANGING_DISABLED
#endif

/*! Radio default IRQ polarity. */
#ifndef WPS_DEFAULT_RADIO_IRQ
#define WPS_DEFAULT_RADIO_IRQ IRQ_ACTIVE_HIGH
#endif

/*! Radio default SPI mode. */
#ifndef WPS_DEFAULT_RADIO_SPI_MODE
#define WPS_DEFAULT_RADIO_SPI_MODE SPI_FAST
#endif

/*! Radio default digital output driver impedance. */
#ifndef WPS_DEFAULT_RADIO_OUTIMPED
#define WPS_DEFAULT_RADIO_OUTIMPED OUTIMPED_2
#endif

/*! Radio default chip rate. */
#ifndef WPS_DEFAULT_RADIO_CHIP_RATE
#define WPS_DEFAULT_RADIO_CHIP_RATE CHIP_RATE_20_48_MHZ
#endif

#ifndef WPS_DEFAULT_SUMRXADC
#define WPS_DEFAULT_SUMRXADC ((radio->chip_rate) > CHIP_RATE_20_48_MHZ)
#endif

/*! Minimal pulse count. */
#define PULSE_COUNT_MIN 1
/*! Maximal pulse count. */
#define PULSE_COUNT_MAX 3
/*! Maximal pulse width. */
#define PULSE_WIDTH_MAX 7
/*! Maximal pulse gain. */
#define PULSE_GAIN_MAX 7
/*! Maximal clear channel assessment threshold. */
#define CCA_THRESH_MAX 115
/*! The radio's maximum payload size is 256, one byte must be reserved for the header size. */
#define FRAME_SIZE_MAX 255
/*! The minimum queue size required for WPS to enable parallel processing. */
#define WPS_MIN_QUEUE_SIZE 2
/*! Pulse start position when using 27MHz with 1 pulse */
#define DEFAULT_1PULSE_27M_START_POS 7
/*! Pulse start position when using 27MHz with 2 pulses and spacing of 1 */
#define DEFAULT_2PULSE_1SPACING_27M_START_POS 5
/*! Pulse start position when using 27MHz with 2 pulses and spacing of 0 */
#define DEFAULT_2PULSE_0SPACING_27M_START_POS 6
/* MACROS *********************************************************************/
#define HW_ADDR(net_id, node_id)        SWC_CONCAT_8B_TO_16B((net_id), (node_id))
#define NET_ID_FROM_PAN_ID(pan_id)      ((pan_id) & 0x0ff)
#define SYNCWORD_ID_FROM_PAN_ID(pan_id) (((pan_id) & 0xf00) >> 8)

#define CHECK_ERROR(cond, err_ptr, err_code, ret) \
    do {                                          \
        if (cond) {                               \
            *(err_ptr) = (err_code);              \
            ret;                                  \
        }                                         \
    } while (0)

/* PUBLIC GLOBALS *************************************************************/
wps_t wps = {0};

/* PRIVATE GLOBALS ************************************************************/
static bool is_started;
static mem_pool_t mem_pool;
static swc_concurrency_mode_t concurrency_mode;
static nvm_t saved_nvm[WPS_RADIO_COUNT];
static calib_vars_t saved_calib_vars[WPS_RADIO_COUNT];
/*! This variable is used to lock/unlock reserved address in the SWC. */
static bool reserved_address_lock = true;
static bool certification_mode_enabled;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static bool has_main_timeslot(const int32_t *timeslot_id, uint32_t timeslot_count);
static bool is_rx_connection(uint8_t local_address, uint8_t source_address);
static bool is_connection_address_valid(uint8_t local_address, uint8_t destination_address, uint8_t source_address);
static uint16_t get_rdo_increment_step(const uint32_t *timeslot_sequence, uint32_t timeslot_sequence_length,
                                       uint32_t rdo_step_ms);
static bool network_role_supported(swc_role_t role);
static wps_role_t network_role_swc_to_wps(swc_role_t role);
static bool sleep_level_supported(swc_sleep_level_t level, schedule_t *schedule);
static sleep_lvl_t sleep_level_swc_to_wps(swc_sleep_level_t level);
static isi_mitig_t isi_mitig_swc_to_wps(swc_isi_mitig_t isi_mitig);
static chip_rate_cfg_t chip_rate_swc_to_wps(swc_chip_rate_t chip_rate);
static bool irq_polarity_supported(swc_irq_polarity_t pol);
static irq_polarity_t irq_polarity_swc_to_wps(swc_irq_polarity_t pol);
static bool spi_mode_supported(swc_spi_mode_t mode);
static std_spi_t spi_mode_swc_to_wps(swc_spi_mode_t mode);
static bool outimped_supported(swc_outimped_t outimped);
static outimped_t outimped_swc_to_wps(swc_outimped_t outimped);
static bool modulation_supported(swc_modulation_t modulation);
static modulation_t modulation_swc_to_wps(swc_modulation_t modulation);
static chip_repetition_t chip_repetition_swc_to_wps(swc_chip_repetition_t chip_repet);
static bool fec_ratio_supported(swc_fec_ratio_t level);
static fec_level_t fec_ratio_swc_to_wps(swc_fec_ratio_t level);
static bool cca_fail_action_supported(swc_cca_fail_action_t action);
static cca_fail_action_t cca_fail_action_swc_to_wps(swc_cca_fail_action_t action);
static uint8_t get_integgain(chip_rate_cfg_t chip_rate, uint8_t pulse_count);
static void save_radio_configuration(uint8_t radio_id, nvm_t *nvm, calib_vars_t *calib_vars);
static void get_saved_radio_configuration(uint8_t radio_id, nvm_t *nvm, calib_vars_t *calib_vars);
static void check_main_connection_priority_errors(const swc_node_t *const node, timeslot_t timeslot, swc_error_t *err);
static void check_auto_connection_priority_errors(const swc_node_t *const node, timeslot_t timeslot, swc_error_t *err);
static void check_global_auto_connection_errors(const timeslot_t *const timeslot, uint32_t timeslot_count,
                                                swc_error_t *err);
static void validate_connection_priority_in_schedule(const swc_node_t *const node, swc_error_t *err);
static int format_radio_nvm(wps_radio_t *wps_radio, char *buffer, uint16_t size);
static bool validate_chip_rate(swc_chip_rate_t chip_rate);
static void initialize_radio_with_defaults(radio_t *radio, uint8_t radio_id);
static void allocate_payload_and_header_buffer_memory(const swc_node_t *const node, swc_error_t *err);
static uint32_t calculate_activated_callback_count(const swc_node_t *const node);
static void validate_channels(wps_connection_list_node_t *conn, void *arg);

/* PUBLIC FUNCTIONS ***********************************************************/

void swc_init(swc_cfg_t cfg, void (*callback)(void), swc_error_t *err)
{
    wps_error_t wps_err;

    *err = SWC_ERR_NONE;

    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR(cfg.timeslot_sequence == NULL, err, SWC_ERR_NULL_PTR, return);
    CHECK_ERROR(cfg.channel_sequence == NULL, err, SWC_ERR_NULL_PTR, return);
    CHECK_ERROR(cfg.timeslot_sequence_length == 0, err, SWC_ERR_ZERO_TIMESLOT_SEQ_LEN, return);
    CHECK_ERROR(cfg.channel_sequence_length == 0, err, SWC_ERR_ZERO_CHAN_SEQ_LEN, return);
    CHECK_ERROR(!validate_chip_rate(cfg.chip_rate), err, SWC_ERR_CHIP_RATE, return);

    uint32_t timeslot_sequence_pll_cycle[cfg.timeslot_sequence_length];
    timeslot_t *timeslots;
    xlayer_request_info_t *request;
    wps_request_config_info_t request_config;

    memset(&wps, 0, sizeof(wps_t));
    mem_pool_init(&mem_pool, cfg.memory_pool, (size_t)cfg.memory_pool_size);

    /* Allocate memory */
    request_config.schedule_ratio_buffer = mem_pool_malloc(&mem_pool, sizeof(wps_schedule_ratio_cfg_t) *
                                                                          WPS_DEFAULT_REQUEST_MEMORY_SIZE);
    CHECK_ERROR(request_config.schedule_ratio_buffer == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return);
    request_config.schedule_ratio_size = WPS_DEFAULT_REQUEST_MEMORY_SIZE;

    request_config.write_request_buffer = mem_pool_malloc(&mem_pool, sizeof(xlayer_write_request_info_t) *
                                                                         WPS_DEFAULT_REQUEST_MEMORY_SIZE);
    CHECK_ERROR(request_config.write_request_buffer == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return);
    request_config.write_request_size = WPS_DEFAULT_REQUEST_MEMORY_SIZE;

    request_config.read_request_buffer = mem_pool_malloc(&mem_pool, sizeof(xlayer_read_request_info_t) *
                                                                        WPS_DEFAULT_REQUEST_MEMORY_SIZE);
    CHECK_ERROR(request_config.read_request_buffer == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return);
    request_config.read_request_size = WPS_DEFAULT_REQUEST_MEMORY_SIZE;

    timeslots = mem_pool_malloc(&mem_pool, sizeof(timeslot_t) * cfg.timeslot_sequence_length);
    CHECK_ERROR(timeslots == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return);
    request = mem_pool_malloc(&mem_pool, sizeof(xlayer_request_info_t) * WPS_DEFAULT_REQUEST_MEMORY_SIZE);
    CHECK_ERROR(request == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return);
    CHECK_ERROR(callback == NULL, err, SWC_ERR_CONTEXT_SWITCH_TRIGGER_IS_NULL, return);

    /* Initialize the request queue which will be used to accumulate request from the application to the WPS */
    wps_init_request_queue(&wps, request, WPS_DEFAULT_REQUEST_MEMORY_SIZE, &request_config);

    wps.chip_rate = chip_rate_swc_to_wps(cfg.chip_rate);
    swc_hal_free_running_timer_init();

#if WPS_RADIO_COUNT == 1
    swc_hal_set_radio_1_irq_callback(swc_radio_irq_handler);
    swc_hal_set_radio_1_dma_rx_callback(swc_radio_spi_receive_complete_handler);

#elif WPS_RADIO_COUNT == 2
    swc_hal_set_radio_1_irq_callback(swc_radio1_irq_handler);
    swc_hal_set_radio_1_dma_rx_callback(swc_radio1_spi_receive_complete_handler);
    swc_hal_set_radio_2_irq_callback(swc_radio2_irq_handler);
    swc_hal_set_radio_2_dma_rx_callback(swc_radio2_spi_receive_complete_handler);
    swc_hal_set_multi_radio_timer_callback(swc_radio_synchronization_timer_callback);
    swc_hal_multi_radio_timer_init();

    /* Initialize MCU timer functions used for timing when in dual radio configuration */
    wps_multi_cfg_t multi_cfg = {
        .timer_frequency_hz = swc_hal_get_timer_multi_frequency_hz(),
        .avg_sample_count = WPS_DEFAULT_MULTI_AVG_COUNT,
        .mode = WPS_DEFAULT_MULTI_MODE,
        .rssi_threshold = WPS_DEFAULT_MULTI_RSSI_THRESH,
        .tx_wakeup_mode = WPS_DEFAULT_MULTI_TX_WAKEUP_MODE,
    };
    wps_multi_init(multi_cfg, chip_rate_swc_to_wps(cfg.chip_rate), &wps_err);
#endif

    for (uint32_t i = 0; i < cfg.timeslot_sequence_length; i++) {
        CHECK_ERROR(cfg.timeslot_sequence[i] == 0, err, SWC_ERR_NULL_TIMESLOT_DURATION, return);
        timeslot_sequence_pll_cycle[i] = wps_us_to_pll_cycle(cfg.timeslot_sequence[i], wps.chip_rate);
    }

    wps_config_network_schedule(&wps, timeslot_sequence_pll_cycle, timeslots, cfg.timeslot_sequence_length, &wps_err);

    uint8_t *channel_buffer_sequence = mem_pool_malloc(&mem_pool, sizeof(uint8_t) * cfg.channel_sequence_length);

    CHECK_ERROR(channel_buffer_sequence == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return);
    wps_config_network_channel_sequence(&wps, cfg.channel_sequence, channel_buffer_sequence,
                                        cfg.channel_sequence_length, &wps_err);

    /* Enable/disable global miscellaneous WPS features */
#if (WPS_RADIO_COUNT == 1)
    wps_disable_fast_sync(&wps, &wps_err);
#endif

    uint16_t increment_step = get_rdo_increment_step(cfg.timeslot_sequence, cfg.timeslot_sequence_length,
                                                     WPS_DEFAULT_RDO_STEP_MS);
    wps_init_rdo(&wps, WPS_DEFAULT_RDO_ROLLOVER_VAL, increment_step, &wps_err);

    wps_enable_random_channel_sequence(&wps, &wps_err);

    concurrency_mode = cfg.concurrency_mode;

    switch (concurrency_mode) {
    case SWC_CONCURRENCY_MODE_HIGH_PERFORMANCE:
    case SWC_CONCURRENCY_MODE_LOW_PERFORMANCE:
        wps_disable_rdo(&wps, &wps_err);
        CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return);
        wps_enable_ddcm(&wps, WPS_DEFAULT_MAX_TIMESLOT_OFFSET, WPS_DEFAULT_SYNC_FRAME_LOST_MAX_DURATION, &wps_err);
        CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return);
        break;
    default:
        *err = SWC_ERR_INVALID_PARAMETER;
        return;
    }

    /* Disable certification mode. */
    certification_mode_enabled = false;

    /* Register the context switch callback trigger to the mac structure. */
    wps.mac.callback_context_switch = callback;
}

void swc_set_fast_sync(bool enabled, swc_error_t *err)
{
#if (WPS_RADIO_COUNT == 1)
    wps_error_t wps_err;

    *err = SWC_ERR_NONE;

    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);

    if (enabled) {
        wps_enable_fast_sync(&wps, &wps_err);
    } else {
        wps_disable_fast_sync(&wps, &wps_err);
    }
#elif WPS_RADIO_COUNT == 2
    if (enabled) {
        *err = SWC_ERR_FAST_SYNC_WITH_DUAL_RADIO;
    }
#endif
}

void swc_set_certification_mode(bool enabled, swc_error_t *err)
{
    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);

    wps_error_t wps_err = WPS_NO_ERROR;

    *err = SWC_ERR_NONE;
    if (enabled) {
        /* Disable features that could affect TDMA. */
        wps_disable_rdo(&wps, &wps_err);
        CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return);
        wps_disable_ddcm(&wps, &wps_err);
        CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return);
        wps_disable_random_channel_sequence(&wps, &wps_err);
        CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return);

#if WPS_RADIO_COUNT == 2
        wps_multi_radio_set_tx_wakeup_mode(MULTI_TX_WAKEUP_MODE_MANUAL);
#endif
    }

    certification_mode_enabled = enabled;
}

swc_node_t *swc_node_init(swc_node_cfg_t cfg, swc_error_t *err)
{
    wps_error_t wps_err;
    wps_node_cfg_t wps_node_cfg;
    swc_node_t *node;

    *err = SWC_ERR_NONE;

    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return NULL);
    CHECK_ERROR(cfg.local_address == SWC_BROADCAST_ADDRESS, err, SWC_ERR_LOCAL_ADDRESS, return NULL);
    CHECK_ERROR(cfg.pan_id & 0xf000, err, SWC_ERR_PAN_ID, return NULL);
    CHECK_ERROR(((cfg.pan_id == 0x0000) && reserved_address_lock), err, SWC_ERR_PAN_ID, return NULL);
    CHECK_ERROR((((cfg.pan_id & 0xff) == 0xff) && reserved_address_lock), err, SWC_ERR_PAN_ID, return NULL);
    CHECK_ERROR(!network_role_supported(cfg.role), err, SWC_ERR_NETWORK_ROLE, return NULL);

    /* Allocate memory */
    node = mem_pool_malloc(&mem_pool, sizeof(swc_node_t));
    CHECK_ERROR(node == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return NULL);
    node->wps_node_handle = mem_pool_malloc(&mem_pool, sizeof(wps_node_t));
    CHECK_ERROR(node->wps_node_handle == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return NULL);
    node->wps_radio_handle = mem_pool_malloc(&mem_pool, sizeof(wps_radio_t) * WPS_RADIO_COUNT);
    CHECK_ERROR(node->wps_radio_handle == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return NULL);

    node->cfg = cfg;
    wps_node_cfg.role = network_role_swc_to_wps(cfg.role);
    wps_node_cfg.sleep_lvl = WPS_DEFAULT_SLEEP_LEVEL;
    wps_node_cfg.crc_polynomial = WPS_DEFAULT_CRC;
    wps_node_cfg.local_address = HW_ADDR(NET_ID_FROM_PAN_ID(cfg.pan_id), cfg.local_address);
    wps_node_cfg.syncword_cfg.syncword = sync_word_table[SYNCWORD_ID_FROM_PAN_ID(cfg.pan_id)];
    wps_node_cfg.syncword_cfg.syncword_length = WPS_DEFAULT_SYNC_WORD_LEN;
    wps_node_cfg.isi_mitig = isi_mitig_swc_to_wps(cfg.isi_mitig);
    wps_node_cfg.rx_gain = WPS_DEFAULT_RX_GAIN;
    wps_node_cfg.tx_jitter_enabled = WPS_DEFAULT_TX_JITTER;
    wps_node_cfg.frame_lost_max_duration = WPS_DEFAULT_SYNC_FRAME_LOST_MAX_DURATION;

    uint8_t isi_mitigation_pauses = link_tdma_sync_get_isi_mitigation_pauses(wps_node_cfg.isi_mitig);

    wps_node_cfg.preamble_len = link_tdma_get_preamble_length(isi_mitigation_pauses, WPS_DEFAULT_PREAMBLE_LEN,
                                                              wps_node_cfg.syncword_cfg.syncword_length);

    wps_set_network_id(&wps, NET_ID_FROM_PAN_ID(cfg.pan_id), &wps_err);
    wps_set_syncing_address(&wps, HW_ADDR(NET_ID_FROM_PAN_ID(cfg.pan_id), cfg.coordinator_address), &wps_err);
    wps_config_node(node->wps_node_handle, node->wps_radio_handle, &wps_node_cfg, &wps_err);

    /* All time slots are of the same sleep level by default as configured in the node */
    wps.mac.scheduler.schedule.lightest_sleep_lvl = wps_node_cfg.sleep_lvl;
    for (uint32_t i = 0; i < wps.mac.scheduler.schedule.size; i++) {
        wps.mac.scheduler.schedule.timeslot[i].sleep_lvl = wps_node_cfg.sleep_lvl;
    }

    CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return NULL);

    return node;
}

void swc_set_concurrency_cfg(swc_concurrency_cfg_t cfg, swc_error_t *err)
{
    wps_error_t wps_err;

    if (certification_mode_enabled) {
        /* Disable features that could affect TDMA. */
        wps_disable_rdo(&wps, &wps_err);
        wps_disable_ddcm(&wps, &wps_err);
        wps_disable_random_channel_sequence(&wps, &wps_err);
        return;
    }

    if (cfg.random_channel_sequence_enabled) {
        wps_enable_random_channel_sequence(&wps, &wps_err);
        CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return);
    } else {
        wps_disable_random_channel_sequence(&wps, &wps_err);
        CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return);
    }

    if (cfg.rdo_enabled) {
        wps_enable_rdo(&wps, &wps_err);
        CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return);
    } else {
        wps_disable_rdo(&wps, &wps_err);
        CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return);
    }

    if (cfg.ddcm_enabled) {
        wps_enable_ddcm(&wps, WPS_DEFAULT_MAX_TIMESLOT_OFFSET, WPS_DEFAULT_SYNC_FRAME_LOST_MAX_DURATION, &wps_err);
        CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return);
    } else {
        wps_disable_ddcm(&wps, &wps_err);
        CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return);
    }
}

void swc_radio_module_init(swc_node_t *const node, swc_radio_id_t radio_id, bool calibrate, swc_error_t *err)
{
    *err = SWC_ERR_NONE;
    uint8_t radio_index = (uint8_t)radio_id;
    sr_phy_error_t phy_err = PHY_MODEL_ERROR_NONE;

    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR(node == NULL, err, SWC_ERR_NULL_PTR, return);
    CHECK_ERROR(radio_id >= SWC_RADIO_ID_MAX, err, SWC_ERR_RADIO_ID_INVALID, return);
    CHECK_ERROR((calibrate == false && node->wps_radio_handle[radio_index].nvm == NULL), err,
                SWC_ERR_CALIBRATION_MISSING, return);
    CHECK_ERROR(*err != SWC_ERR_NONE, err, *err, return);

    initialize_radio_with_defaults(&node->wps_radio_handle[radio_index].radio, radio_index);

    /* Allocate memory */
    node->wps_radio_handle[radio_id].nvm = mem_pool_malloc(&mem_pool, sizeof(nvm_t));
    CHECK_ERROR(node->wps_radio_handle[radio_id].nvm == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return);
    node->wps_radio_handle[radio_id].spectral_calib_vars = mem_pool_malloc(&mem_pool, sizeof(calib_vars_t));
    CHECK_ERROR(node->wps_radio_handle[radio_id].spectral_calib_vars == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return);

    /* Disable MCU external interrupt servicing the radio IRQ before initializing the WPS.
     * It will be later re-activated with a call to the swc_connect() function.
     */
    sr_access_disable_radio_irq(radio_index);

    if (calibrate) {
        /* Allocate memory */
        node->wps_radio_handle[radio_index].nvm = mem_pool_malloc(&mem_pool, sizeof(nvm_t));
        CHECK_ERROR(node->wps_radio_handle[radio_index].nvm == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return);
        node->wps_radio_handle[radio_index].spectral_calib_vars = mem_pool_malloc(&mem_pool, sizeof(calib_vars_t));
        CHECK_ERROR(node->wps_radio_handle[radio_index].spectral_calib_vars == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY,
                    return);

        wps_radio_init(&node->wps_radio_handle[radio_index], false, &phy_err);
        CHECK_ERROR(phy_err == PHY_MODEL_NOT_FOUND, err, SWC_ERR_RADIO_NOT_FOUND, return);
        wps_radio_calibration(&node->wps_radio_handle[radio_index]);
        save_radio_configuration(radio_index, node->wps_radio_handle[radio_index].nvm,
                                 node->wps_radio_handle[radio_index].spectral_calib_vars);
    } else {
        get_saved_radio_configuration(radio_id, node->wps_radio_handle[radio_id].nvm,
                                      node->wps_radio_handle[radio_id].spectral_calib_vars);
    }
}

void swc_node_set_radio_irq_polarity(swc_node_t *node, swc_radio_id_t radio_id, swc_irq_polarity_t irq_polarity,
                                     swc_error_t *err)
{
    *err = SWC_ERR_NONE;

    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR(node == NULL, err, SWC_ERR_NULL_PTR, return);
    CHECK_ERROR(radio_id >= SWC_RADIO_ID_MAX, err, SWC_ERR_RADIO_ID_INVALID, return);
    CHECK_ERROR(!irq_polarity_supported(irq_polarity), err, SWC_ERR_IRQ_POLARITY, return);

    uint8_t radio_index = (uint8_t)radio_id;

    node->wps_radio_handle[radio_index].radio.irq_polarity = irq_polarity_swc_to_wps(irq_polarity);
}

void swc_node_set_radio_spi_mode(swc_node_t *node, swc_radio_id_t radio_id, swc_spi_mode_t spi_mode, swc_error_t *err)
{
    *err = SWC_ERR_NONE;

    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR(node == NULL, err, SWC_ERR_NULL_PTR, return);
    CHECK_ERROR(radio_id >= SWC_RADIO_ID_MAX, err, SWC_ERR_RADIO_ID_INVALID, return);
    CHECK_ERROR(!spi_mode_supported(spi_mode), err, SWC_ERR_SPI_MODE, return);

    uint8_t radio_index = (uint8_t)radio_id;

    node->wps_radio_handle[radio_index].radio.std_spi = spi_mode_swc_to_wps(spi_mode);
}

void swc_node_set_radio_outimped(swc_node_t *node, swc_radio_id_t radio_id, swc_outimped_t outimped, swc_error_t *err)
{
    *err = SWC_ERR_NONE;

    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR(node == NULL, err, SWC_ERR_NULL_PTR, return);
    CHECK_ERROR(radio_id >= SWC_RADIO_ID_MAX, err, SWC_ERR_RADIO_ID_INVALID, return);
    CHECK_ERROR(!outimped_supported(outimped), err, SWC_ERR_OUTIMPED, return);

    uint8_t radio_index = (uint8_t)radio_id;

    node->wps_radio_handle[radio_index].radio.outimped = outimped_swc_to_wps(outimped);
}

void swc_node_set_low_power_callback(const swc_node_t *const node, void (*cb)(void *node), swc_error_t *err)
{
    CHECK_ERROR(node == NULL, err, SWC_ERR_NOT_INITIALIZED, return);
    CHECK_ERROR(node->wps_node_handle == NULL, err, SWC_ERR_NOT_INITIALIZED, return);

    node->wps_node_handle->low_power_callback = cb;
    *err = SWC_ERR_NONE;
}

void swc_node_set_sleep_level(swc_node_t *node, swc_sleep_level_t sleep_level, swc_error_t *err)
{
    *err = SWC_ERR_NONE;

    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR(node == NULL, err, SWC_ERR_NULL_PTR, return);
    CHECK_ERROR(!sleep_level_supported(sleep_level, &wps.mac.scheduler.schedule), err, SWC_ERR_SLEEP_LEVEL, return);

    node->wps_node_handle->cfg.sleep_lvl = sleep_level_swc_to_wps(sleep_level);

    /* All time slots are of the same sleep level by default as configured in the node */
    wps.mac.scheduler.schedule.lightest_sleep_lvl = node->wps_node_handle->cfg.sleep_lvl;
    for (uint32_t i = 0; i < wps.mac.scheduler.schedule.size; i++) {
        wps.mac.scheduler.schedule.timeslot[i].sleep_lvl = node->wps_node_handle->cfg.sleep_lvl;
    }
}

bool swc_node_is_low_power_allowed(const swc_node_t *const node, swc_error_t *err)
{
    CHECK_ERROR(node == NULL, err, SWC_ERR_NOT_INITIALIZED, return false);
    CHECK_ERROR(node->wps_node_handle == NULL, err, SWC_ERR_NOT_INITIALIZED, return false);

    *err = SWC_ERR_NONE;

    return node->wps_node_handle->low_power_allowed;
}

uint64_t swc_node_get_radio_serial_number(swc_error_t *err)
{
    *err = SWC_ERR_NONE;

    CHECK_ERROR(wps.node->radio == NULL, err, SWC_ERR_NOT_INITIALIZED, return 0);
    CHECK_ERROR(wps.node->radio->nvm == NULL, err, SWC_ERR_NOT_INITIALIZED, return 0);

    return wps_radio_get_serial_number(&wps.node->radio[0]);
}

uint8_t swc_node_get_radio_product_model(swc_error_t *err)
{
    *err = SWC_ERR_NONE;

    CHECK_ERROR(wps.node->radio == NULL, err, SWC_ERR_NOT_INITIALIZED, return 0);
    CHECK_ERROR(wps.node->radio[0].nvm == NULL, err, SWC_ERR_NOT_INITIALIZED, return 0);

    return wps_radio_get_product_id_model(&wps.node->radio[0]);
}

uint8_t swc_node_get_radio_product_version(swc_error_t *err)
{
    *err = SWC_ERR_NONE;

    CHECK_ERROR(wps.node->radio == NULL, err, SWC_ERR_NOT_INITIALIZED, return 0);
    CHECK_ERROR(wps.node->radio[0].nvm == NULL, err, SWC_ERR_NOT_INITIALIZED, return 0);

    return wps_radio_get_product_id_version(&wps.node->radio[0]);
}

int swc_format_radio_nvm(char *const buffer, uint16_t size, swc_error_t *err)
{
    *err = SWC_ERR_NONE;

    CHECK_ERROR(wps.node->radio == NULL, err, SWC_ERR_NOT_INITIALIZED, return 0);
    CHECK_ERROR(wps.node->radio->nvm == NULL, err, SWC_ERR_NOT_INITIALIZED, return 0);

    return format_radio_nvm(&wps.node->radio[0], buffer, size);
}

#if (WPS_RADIO_COUNT == 2)
uint64_t swc_node_get_radio2_serial_number(swc_error_t *err)
{
    *err = SWC_ERR_NONE;

    CHECK_ERROR(wps.node->radio == NULL, err, SWC_ERR_NOT_INITIALIZED, return 0);
    CHECK_ERROR(wps.node->radio[1].nvm == NULL, err, SWC_ERR_SECOND_RADIO_NOT_INIT, return 0);

    return wps_radio_get_serial_number(&wps.node->radio[1]);
}

uint8_t swc_node_get_radio2_product_model(swc_error_t *err)
{
    *err = SWC_ERR_NONE;

    CHECK_ERROR(wps.node->radio == NULL, err, SWC_ERR_NOT_INITIALIZED, return 0);
    CHECK_ERROR(wps.node->radio[1].nvm == NULL, err, SWC_ERR_NOT_INITIALIZED, return 0);

    return wps_radio_get_product_id_model(&wps.node->radio[1]);
}

uint8_t swc_node_get_radio2_product_version(swc_error_t *err)
{
    *err = SWC_ERR_NONE;

    CHECK_ERROR(wps.node->radio == NULL, err, SWC_ERR_NOT_INITIALIZED, return 0);
    CHECK_ERROR(wps.node->radio[1].nvm == NULL, err, SWC_ERR_NOT_INITIALIZED, return 0);

    return wps_radio_get_product_id_version(&wps.node->radio[1]);
}

int swc_format_radio2_nvm(char *const buffer, uint16_t size, swc_error_t *err)
{
    *err = SWC_ERR_NONE;

    CHECK_ERROR(wps.node->radio == NULL, err, SWC_ERR_NOT_INITIALIZED, return 0);
    CHECK_ERROR(wps.node->radio[1].nvm == NULL, err, SWC_ERR_SECOND_RADIO_NOT_INIT, return 0);

    return format_radio_nvm(&wps.node->radio[1], buffer, size);
}
#endif

swc_connection_t *swc_connection_init(swc_node_t *const node, swc_connection_cfg_t cfg, swc_error_t *err)
{
    wps_error_t wps_err;
    wps_connection_cfg_t wps_conn_cfg;
    wps_header_cfg_t wps_header_cfg = {0};
    uint8_t header_size;
    uint8_t conn_frame_length;
    bool is_rx_conn;
    bool is_conn_address_valid;
    bool has_main_ts;
    swc_connection_t *conn;

    *err = SWC_ERR_NONE;

    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return NULL);
    CHECK_ERROR((node == NULL), err, SWC_ERR_NULL_PTR, return NULL);
    CHECK_ERROR((cfg.timeslot_id == NULL), err, SWC_ERR_NULL_PTR, return NULL);
    CHECK_ERROR((cfg.timeslot_count == 0), err, SWC_ERR_ZERO_TIMESLOT_COUNT, return NULL);
    CHECK_ERROR((cfg.queue_size < WPS_MIN_QUEUE_SIZE), err, SWC_ERR_MIN_QUEUE_SIZE, return NULL);

    is_rx_conn = is_rx_connection(node->cfg.local_address, cfg.source_address);
    is_conn_address_valid = is_connection_address_valid(node->cfg.local_address, cfg.destination_address,
                                                        cfg.source_address);
    has_main_ts = has_main_timeslot(cfg.timeslot_id, cfg.timeslot_count);

    CHECK_ERROR(cfg.source_address == SWC_BROADCAST_ADDRESS, err, SWC_ERR_SOURCE_ADDRESS, return NULL);
    CHECK_ERROR(is_rx_conn && (cfg.destination_address == SWC_BROADCAST_ADDRESS), err, SWC_ERR_DESTINATION_ADDRESS,
                return NULL);
    CHECK_ERROR(!is_conn_address_valid, err, SWC_ERR_CONNECTION_ADDRESS, return NULL);

    wps_header_cfg.main_connection = has_main_ts;
    wps_header_cfg.rdo_enabled = (has_main_ts && wps.mac.link_rdo.enabled) ? true : false;
    wps_header_cfg.ranging_mode = WPS_DEFAULT_RANGING;
    wps_header_cfg.credit_fc_enabled = false;
    wps_header_cfg.connection_id = WPS_DEFAULT_CONNECTION_ID;

    header_size = wps_get_connection_header_size(&wps, wps_header_cfg);
    CHECK_ERROR(((cfg.max_payload_size + header_size + WPS_PAYLOAD_SIZE_BYTE_SIZE) > FRAME_SIZE_MAX), err,
                SWC_ERR_PAYLOAD_TOO_BIG, return NULL);
    conn_frame_length = cfg.max_payload_size + header_size + WPS_PAYLOAD_SIZE_BYTE_SIZE;

    /* Allocate memory */
    conn = mem_pool_malloc(&mem_pool, sizeof(swc_connection_t));
    CHECK_ERROR(conn == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return NULL);
    conn->wps_conn_handle = mem_pool_malloc(&mem_pool, sizeof(wps_connection_t));
    CHECK_ERROR(conn->wps_conn_handle == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return NULL);

    conn->channel_count = 0;
    conn->wps_conn_handle->max_channel_count = wps_get_channel_count(&wps, &wps_err);
    conn->cfg = cfg;

    wps_conn_cfg.source_address = HW_ADDR(NET_ID_FROM_PAN_ID(node->cfg.pan_id), cfg.source_address);
    wps_conn_cfg.destination_address = HW_ADDR(NET_ID_FROM_PAN_ID(node->cfg.pan_id), cfg.destination_address);
    wps_conn_cfg.header_length = header_size;
    wps_conn_cfg.ack_header_length = wps_get_connection_ack_header_size(&wps, wps_header_cfg);
    wps_conn_cfg.frame_length = conn_frame_length;
    wps_conn_cfg.get_tick = swc_hal_get_tick_free_running_timer;
    wps_conn_cfg.tick_frequency_hz = swc_hal_get_free_running_timer_frequency_hz();
    wps_conn_cfg.fifo_buffer_size = cfg.queue_size;
    wps_conn_cfg.priority = WPS_DEFAULT_PRIORITY;
    wps_conn_cfg.ranging_mode = WPS_DEFAULT_RANGING;
    wps_conn_cfg.credit_fc_enabled = false;

    wps_create_connection(conn->wps_conn_handle, node->wps_node_handle, &wps_conn_cfg, &wps_err);

    wps_connection_config_frame(conn->wps_conn_handle, WPS_DEFAULT_MODULATION,
                                chip_repetition_swc_to_wps(cfg.chip_repet), WPS_DEFAULT_FEC_RATIO, &wps_err);

    wps_connection_set_timeslot(conn->wps_conn_handle, &wps, cfg.timeslot_id, cfg.timeslot_count, &wps_err);
    CHECK_ERROR(wps_err == WPS_TIMESLOT_CONN_LIMIT_REACHED_ERROR, err, SWC_ERR_TIMESLOT_CONN_LIMIT_REACHED,
                return NULL);

    connect_status_cfg_t status_cfg = {
        .connect_count = WPS_DEFAULT_CONNECT_STATUS_COUNT,
        .disconnect_count = WPS_DEFAULT_DISCONNECT_STATUS_COUNT,
    };

    wps_connection_config_status(conn->wps_conn_handle, &status_cfg, &wps_err);

    /* Enable ACK and ARQ only if connection is main */
    if (has_main_ts == true) {
        wps_connection_enable_ack(conn->wps_conn_handle, &wps_err);
        wps_connection_enable_stop_and_wait_arq(conn->wps_conn_handle, node->wps_node_handle->cfg.local_address,
                                                WPS_DEFAULT_TRY_DEADLINE, WPS_DEFAULT_TIME_DEADLINE, &wps_err);
    } else {
        wps_connection_disable_ack(conn->wps_conn_handle, &wps_err);
        wps_connection_disable_stop_and_wait_arq(conn->wps_conn_handle, &wps_err);
    }

    wps_connection_disable_auto_sync(conn->wps_conn_handle, &wps_err);

    wps_connection_disable_fallback(conn->wps_conn_handle, &wps_err);

    if (certification_mode_enabled == false) {
        switch (concurrency_mode) {
        case SWC_CONCURRENCY_MODE_HIGH_PERFORMANCE:
            wps_connection_enable_cca(conn->wps_conn_handle, WPS_DEFAULT_CCA_THRESHOLD, DEFAULT_CCA_HP_RETRY_TIME,
                                      DEFAULT_CCA_HP_TRY_COUNT, CCA_FAIL_ACTION_ABORT_TX,
                                      WPS_DEFAULT_CCA_ON_TIME_PLL_CYCLES, &wps_err);
            CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_CCA_INVALID_PARAMETERS, return NULL);
            break;
        case SWC_CONCURRENCY_MODE_LOW_PERFORMANCE:
            wps_connection_enable_cca(conn->wps_conn_handle, WPS_DEFAULT_CCA_THRESHOLD, DEFAULT_CCA_LP_RETRY_TIME,
                                      DEFAULT_CCA_LP_TRY_COUNT, CCA_FAIL_ACTION_ABORT_TX,
                                      WPS_DEFAULT_CCA_ON_TIME_PLL_CYCLES, &wps_err);
            CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_CCA_INVALID_PARAMETERS, return NULL);
            break;
        default:
            *err = SWC_ERR_INVALID_PARAMETER;
            return NULL;
        }
    } else {
        /* Disable CCA to avoid changing timing of transmission. */
        wps_connection_disable_cca(conn->wps_conn_handle, &wps_err);
    }

    wps_connection_disable_credit_flow_ctrl(conn->wps_conn_handle, &wps_err);
    CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return NULL);

    wps_configure_header_connection(&wps, conn->wps_conn_handle, wps_header_cfg, &wps_err);
    CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return NULL);
    wps_configure_header_acknowledge(&wps, conn->wps_conn_handle, wps_header_cfg, &wps_err);
    CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return NULL);

    /* Init connection reset tick value. */
    conn->stats.tick_on_reset = conn->wps_conn_handle->get_tick();

    size_t channel_count = conn->wps_conn_handle->max_channel_count;

    /* Gain loop per channel allocation */
    conn->wps_conn_handle->gain_loop = mem_pool_malloc(&mem_pool, channel_count * sizeof(gain_loop_t[WPS_RADIO_COUNT]));
    CHECK_ERROR(conn->wps_conn_handle->gain_loop == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return NULL);
    /* Channel allocation */
    conn->wps_conn_handle->channel = mem_pool_malloc(&mem_pool, channel_count * sizeof(rf_channel_t[WPS_RADIO_COUNT]));
    CHECK_ERROR(conn->wps_conn_handle->channel == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return NULL);

#if WPS_ENABLE_PHY_STATS_PER_BANDS
    /* Channel LQI 1D array allocation */
    conn->wps_conn_handle->channel_lqi = mem_pool_malloc(&mem_pool,
                                                         channel_count * sizeof(*conn->wps_conn_handle->channel_lqi));
    CHECK_ERROR(conn->wps_conn_handle->channel_lqi == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return NULL);

    /* Allocate stats per bands */
    conn->wps_conn_handle->wps_chan_stats = mem_pool_malloc(&mem_pool, channel_count * sizeof(wps_stats_t));
    CHECK_ERROR(conn->wps_conn_handle->wps_chan_stats == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return NULL);
    conn->stats_per_bands = mem_pool_malloc(&mem_pool, channel_count * sizeof(swc_statistics_t));
    CHECK_ERROR(conn->stats_per_bands == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return NULL);
    /* Set channel count for stats per bands since auto connection won't increment channel
     * count through swc_connection_add_channel
     */
    if (has_main_ts == false) {
        conn->channel_count = conn->wps_conn_handle->max_channel_count;
    }
#endif

    return conn;
}

swc_connection_cfg_t swc_get_beacon_connection_config(const swc_node_t *const node, uint8_t source_address,
                                                      const int32_t *const timeslot_id, uint8_t timeslot_count)
{
    uint8_t destination_address;

    if (is_rx_connection(node->cfg.local_address, source_address)) {
        destination_address = (uint8_t)node->wps_node_handle->cfg.local_address;
    } else {
        destination_address = SWC_BROADCAST_ADDRESS;
    }

    swc_connection_cfg_t beacon_conn_cfg = {
        .name = "Beacon Connection",
        .source_address = source_address,
        .destination_address = destination_address,
        .max_payload_size = 0,
        .queue_size = WPS_MIN_QUEUE_SIZE,
        .timeslot_id = timeslot_id,
        .timeslot_count = timeslot_count,
    };
    return beacon_conn_cfg;
}

void swc_connection_add_channel(swc_connection_t *const conn, const swc_node_t *const node, swc_channel_cfg_t cfg,
                                swc_error_t *err)
{
    wps_error_t wps_err;
    channel_cfg_t wps_chann_cfg;
    bool is_rx_conn;
    bool is_tx_conn;
    bool has_main_ts;

    *err = SWC_ERR_NONE;

    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR((conn == NULL) || (node == NULL), err, SWC_ERR_NULL_PTR, return);

    is_rx_conn = is_rx_connection(node->cfg.local_address, conn->cfg.source_address);
    is_tx_conn = !is_rx_conn;
    has_main_ts = has_main_timeslot(conn->cfg.timeslot_id, conn->cfg.timeslot_count);

    CHECK_ERROR(has_main_ts == false, err, SWC_ERR_ADD_CHANNEL_ON_INVALID_CONNECTION, return);

    if (is_tx_conn || (is_rx_conn && conn->wps_conn_handle->ack_enable)) {
        CHECK_ERROR((cfg.tx_pulse_count < PULSE_COUNT_MIN) || (cfg.tx_pulse_count > PULSE_COUNT_MAX), err,
                    SWC_ERR_TX_PULSE_COUNT, return);
        CHECK_ERROR(cfg.tx_pulse_width > PULSE_WIDTH_MAX, err, SWC_ERR_TX_PULSE_WIDTH, return);
        CHECK_ERROR(cfg.tx_pulse_gain > PULSE_GAIN_MAX, err, SWC_ERR_TX_PULSE_GAIN, return);
    }

    if ((is_tx_conn && conn->wps_conn_handle->ack_enable) || is_rx_conn) {
        CHECK_ERROR((cfg.rx_pulse_count < PULSE_COUNT_MIN) || (cfg.rx_pulse_count > PULSE_COUNT_MAX), err,
                    SWC_ERR_RX_PULSE_COUNT, return);
    }

    /* Configure RF channels the connection will use */
    wps_chann_cfg.pulse_count = cfg.tx_pulse_count;
    wps_chann_cfg.tx_gain = cfg.tx_pulse_gain;
    wps_chann_cfg.pulse_spacing = WPS_DEFAULT_PULSE_SPACING;
    wps_chann_cfg.start_pos = WPS_DEFAULT_PULSE_START_POS;
    wps_chann_cfg.center_freq = (cfg.frequency * 4096) / 100; /* center_freq is in MHz */
    for (uint8_t i = 0; i < wps_chann_cfg.pulse_count; i++) {
        wps_chann_cfg.pulse_cfg_selector[i] = SR_SPECTRAL_TX_CFG1;
    }
    wps_chann_cfg.pulse_width_table = &cfg.tx_pulse_width;
    wps_chann_cfg.pulse_cfg_num = WPS_DEFAULT_PULSE_CFG_COUNT;
    wps_chann_cfg.integrators_gain = get_integgain(wps.chip_rate, cfg.rx_pulse_count);
    wps_chann_cfg.freq_shift = WPS_DEFAULT_FREQ_SHIFT;

    if (node->wps_radio_handle->radio.chip_rate == CHIP_RATE_27_30_MHZ) {
        if (cfg.tx_pulse_count == 1) {
            wps_chann_cfg.start_pos = DEFAULT_1PULSE_27M_START_POS;
        } else if (cfg.tx_pulse_count == 2) {
            if (wps_chann_cfg.pulse_spacing == 1) {
                wps_chann_cfg.start_pos = DEFAULT_2PULSE_1SPACING_27M_START_POS;
            } else if (wps_chann_cfg.pulse_spacing == 0) {
                wps_chann_cfg.start_pos = DEFAULT_2PULSE_0SPACING_27M_START_POS;
            } else {
                *err = SWC_ERR_INVALID_PULSE_CONFIG_27M;
                return;
            }
        }
        /* If using two pulses with one spacing, only position 5 and 7 are working */
        CHECK_ERROR((cfg.tx_pulse_count == 2) && (wps_chann_cfg.start_pos != DEFAULT_2PULSE_1SPACING_27M_START_POS) &&
                        (wps_chann_cfg.pulse_spacing == 1),
                    err, SWC_ERR_INVALID_PULSE_CONFIG_27M, return);
        /* If using two pulses with 0 spacing, only position 6 and 7 are working */
        CHECK_ERROR((cfg.tx_pulse_count == 2) && (wps_chann_cfg.start_pos != DEFAULT_2PULSE_0SPACING_27M_START_POS) &&
                        (wps_chann_cfg.pulse_spacing == 0),
                    err, SWC_ERR_INVALID_PULSE_CONFIG_27M, return);
        /* Every other config are not valid*/
        CHECK_ERROR((cfg.tx_pulse_count > 2) || (wps_chann_cfg.start_pos < DEFAULT_2PULSE_1SPACING_27M_START_POS) ||
                        (wps_chann_cfg.pulse_spacing > 1),
                    err, SWC_ERR_INVALID_PULSE_CONFIG_27M, return);
    }
    wps_connection_config_channel(conn->wps_conn_handle, node->wps_node_handle, conn->channel_count, &wps_chann_cfg,
                                  &wps_err);

    CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return);

    conn->channel_count++;
}

void swc_connection_add_fallback_channel(swc_connection_t *const conn, const swc_node_t *const node,
                                         swc_channel_cfg_t main_cfg, swc_fallback_channel_cfg_t cfg,
                                         uint8_t channel_index, uint8_t fallback_index, swc_error_t *err)
{
    channel_cfg_t wps_chann_cfg;
    wps_error_t wps_err;

    *err = SWC_ERR_NONE;

    if ((!is_rx_connection(node->cfg.local_address, conn->cfg.source_address)) &&
        (conn->wps_conn_handle->fallback_channel != NULL)) {
        wps_chann_cfg.pulse_spacing = WPS_DEFAULT_PULSE_SPACING;
        wps_chann_cfg.start_pos = WPS_DEFAULT_PULSE_START_POS;
        wps_chann_cfg.center_freq = (main_cfg.frequency * 4096) / 100; /* center_freq is in MHz */
        for (uint8_t i = 0; i < WPS_DEFAULT_PULSE_CFG_COUNT; i++) {
            wps_chann_cfg.pulse_cfg_selector[i] = SR_SPECTRAL_TX_CFG1;
        }
        wps_chann_cfg.pulse_cfg_num = WPS_DEFAULT_PULSE_CFG_COUNT;
        wps_chann_cfg.integrators_gain = get_integgain(node->wps_node_handle->radio->radio.chip_rate,
                                                       main_cfg.rx_pulse_count);
        wps_chann_cfg.freq_shift = WPS_DEFAULT_FREQ_SHIFT;

        wps_chann_cfg.pulse_count = cfg.tx_pulse_count;
        wps_chann_cfg.pulse_width_table = &cfg.tx_pulse_width;
        wps_chann_cfg.tx_gain = cfg.tx_pulse_gain;

        CHECK_ERROR((wps_chann_cfg.pulse_count < PULSE_COUNT_MIN) || (wps_chann_cfg.pulse_count > PULSE_COUNT_MAX), err,
                    SWC_ERR_TX_PULSE_COUNT_OFFSET, return);
        CHECK_ERROR(wps_chann_cfg.pulse_width_table[0] > PULSE_WIDTH_MAX, err, SWC_ERR_TX_PULSE_WIDTH_OFFSET, return);
        CHECK_ERROR(wps_chann_cfg.tx_gain > PULSE_GAIN_MAX, err, SWC_ERR_TX_GAIN_OFFSET, return);

        wps_connection_config_fallback_channel(conn->wps_conn_handle, node->wps_node_handle, channel_index,
                                               fallback_index, &wps_chann_cfg, &wps_err);
    }
}

void swc_connection_set_tx_success_callback(swc_connection_t *const conn, void (*cb)(void *conn), swc_error_t *err)
{
    *err = SWC_ERR_NONE;

    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR(conn == NULL, err, SWC_ERR_NULL_PTR, return);
    CHECK_ERROR(wps.node != NULL, err, SWC_ERR_INVALID_OPERATION_AFTER_SETUP, return);

#if !WPS_DISABLE_FRAGMENTATION
    if (conn->wps_conn_handle->frag.enabled) {
        wps_frag_set_tx_success_callback(conn->wps_conn_handle, cb, conn);
    } else {
        wps_set_tx_success_callback(conn->wps_conn_handle, cb, conn);
    }
#else
    wps_set_tx_success_callback(conn->wps_conn_handle, cb, conn);
#endif
}

void swc_connection_set_tx_fail_callback(swc_connection_t *const conn, void (*cb)(void *conn), swc_error_t *err)
{
    *err = SWC_ERR_NONE;

    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR(conn == NULL, err, SWC_ERR_NULL_PTR, return);
    CHECK_ERROR(wps.node != NULL, err, SWC_ERR_INVALID_OPERATION_AFTER_SETUP, return);

    wps_set_tx_fail_callback(conn->wps_conn_handle, cb, conn);
}

void swc_connection_set_tx_dropped_callback(swc_connection_t *const conn, void (*cb)(void *conn), swc_error_t *err)
{
    *err = SWC_ERR_NONE;

    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR(conn == NULL, err, SWC_ERR_NULL_PTR, return);
    CHECK_ERROR(wps.node != NULL, err, SWC_ERR_INVALID_OPERATION_AFTER_SETUP, return);

    wps_set_tx_drop_callback(conn->wps_conn_handle, cb, conn);
}

void swc_connection_set_rx_success_callback(swc_connection_t *const conn, void (*cb)(void *conn), swc_error_t *err)
{
    *err = SWC_ERR_NONE;

    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR(conn == NULL, err, SWC_ERR_NULL_PTR, return);
    CHECK_ERROR(wps.node != NULL, err, SWC_ERR_INVALID_OPERATION_AFTER_SETUP, return);

#if !WPS_DISABLE_FRAGMENTATION
    if (conn->wps_conn_handle->frag.enabled) {
        wps_frag_set_rx_success_callback(conn->wps_conn_handle, cb, conn);
    } else {
        wps_set_rx_success_callback(conn->wps_conn_handle, cb, conn);
    }
#else
    wps_set_rx_success_callback(conn->wps_conn_handle, cb, conn);
#endif
}

void swc_connection_set_event_callback(swc_connection_t *const conn, void (*cb)(void *conn), swc_error_t *err)
{
    *err = SWC_ERR_NONE;

    CHECK_ERROR(conn == NULL, err, SWC_ERR_NULL_PTR, return);
    CHECK_ERROR(wps.node != NULL, err, SWC_ERR_INVALID_OPERATION_AFTER_SETUP, return);

#if !WPS_DISABLE_FRAGMENTATION
    if (conn->wps_conn_handle->frag.enabled) {
        wps_frag_set_event_callback(conn->wps_conn_handle, cb, conn);
    } else {
        wps_set_event_callback(conn->wps_conn_handle, cb, conn);
    }
#else
    wps_set_event_callback(conn->wps_conn_handle, cb, conn);
#endif
}

void swc_connection_optimized_latency(swc_connection_t *conn, swc_node_t *node, uint8_t auto_reply_payload_size,
                                      swc_error_t *err)
{
    wps_error_t wps_err;

    *err = SWC_ERR_NONE;
    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR((conn == NULL) || (node == NULL), err, SWC_ERR_NULL_PTR, return);
    CHECK_ERROR((node->cfg.isi_mitig > SWC_ISI_MITIG_2) || (certification_mode_enabled == true), err,
                SWC_ERR_INVALID_PARAMETER, return);

    wps_connection_optimize_latency(conn->wps_conn_handle, auto_reply_payload_size, node->wps_node_handle, true, true,
                                    &wps_err);

    /* Ensure that the delay doesn't bust the minimum possible timeslot */
    int32_t min_timeslot_duration = INT32_MAX;

    for (uint8_t i = 0; i < wps.mac.scheduler.schedule.size; i++) {
        if ((int32_t)wps.mac.scheduler.schedule.timeslot[i].duration_pll_cycles < min_timeslot_duration) {
            min_timeslot_duration = wps.mac.scheduler.schedule.timeslot[i].duration_pll_cycles;
        }
    }

    CHECK_ERROR(conn->wps_conn_handle->empty_queue_max_delay >= min_timeslot_duration, err,
                SWC_ERR_OPTIMIZATION_DELAY_TO_HIGH, return);
}

#if !WPS_DISABLE_FRAGMENTATION
void swc_connection_set_fragmentation(swc_connection_t *conn, swc_error_t *err)
{
    *err = SWC_ERR_NONE;
    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR((conn == NULL), err, SWC_ERR_NULL_PTR, return);
    CHECK_ERROR((conn->cfg.queue_size < WPS_MIN_QUEUE_SIZE), err, SWC_ERR_MIN_QUEUE_SIZE, return);

    uint16_t *frag_tx_meta_buffer = mem_pool_malloc(&mem_pool, sizeof(uint16_t) * conn->cfg.queue_size);

    CHECK_ERROR(frag_tx_meta_buffer == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return);
    wps_frag_init(conn->wps_conn_handle, (void *)frag_tx_meta_buffer, conn->cfg.queue_size);
}
#endif

void swc_connection_set_acknowledgement(swc_connection_t *conn, bool enabled, swc_error_t *err)
{
    bool has_main_ts = has_main_timeslot(conn->cfg.timeslot_id, conn->cfg.timeslot_count);

    *err = SWC_ERR_NONE;
    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR((conn == NULL), err, SWC_ERR_NULL_PTR, return);
    CHECK_ERROR((has_main_ts == false) && enabled, err, SWC_ERR_ACK_NOT_SUPPORTED_IN_AUTO_REPLY_CONNECTION, return);
    CHECK_ERROR((conn->wps_conn_handle->ack_frame_enable == true) && (enabled == false) && (has_main_ts == true), err,
                SWC_ERR_CREDIT_FLOW_CTRL_WITH_ACK_DISABLED, return);
    wps_error_t wps_err;

    if (enabled) {
        wps_connection_enable_ack(conn->wps_conn_handle, &wps_err);
    } else {
        wps_connection_disable_ack(conn->wps_conn_handle, &wps_err);
    }
}

void swc_connection_set_credit_flow_ctrl(swc_connection_t *conn, swc_node_t *node, bool enabled, swc_error_t *err)
{
    wps_error_t wps_err;
    wps_header_cfg_t wps_header_cfg = {0};
    uint8_t header_size;

    *err = SWC_ERR_NONE;

    bool has_main_ts = has_main_timeslot(conn->cfg.timeslot_id, conn->cfg.timeslot_count);

    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR((conn == NULL) || (node == NULL), err, SWC_ERR_NULL_PTR, return);
    CHECK_ERROR((conn->wps_conn_handle->ack_enable == false) && (enabled == true) && (has_main_ts == true), err,
                SWC_ERR_CREDIT_FLOW_CTRL_WITH_ACK_DISABLED, return);

    if (enabled) {
        /* Enable Ack frame and allocate memory */
        conn->wps_conn_handle->ack_frame_enable = true;
        link_protocol_t *auto_link_protocol = mem_pool_malloc(&mem_pool, sizeof(link_protocol_t));

        CHECK_ERROR(auto_link_protocol == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return);
        conn->wps_conn_handle->auto_link_protocol = auto_link_protocol;
        wps_connection_enable_credit_flow_ctrl(conn->wps_conn_handle, has_main_ts, &wps_err);
    } else {
        wps_connection_disable_credit_flow_ctrl(conn->wps_conn_handle, &wps_err);
    }

    CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return);
    wps_header_cfg.main_connection = has_main_ts;
    wps_header_cfg.rdo_enabled = (has_main_ts && wps.mac.link_rdo.enabled) ? true : false;
    wps_header_cfg.connection_id = true;
    wps_header_cfg.ranging_mode = WPS_DEFAULT_RANGING;
    wps_header_cfg.credit_fc_enabled = enabled;

    header_size = wps_get_connection_header_size(&wps, wps_header_cfg);
    CHECK_ERROR(((conn->cfg.max_payload_size + header_size + WPS_PAYLOAD_SIZE_BYTE_SIZE) > FRAME_SIZE_MAX), err,
                SWC_ERR_PAYLOAD_TOO_BIG, return);

    conn->wps_conn_handle->header_size = header_size;
    if (header_size > node->wps_node_handle->max_header_size) {
        node->wps_node_handle->max_header_size = header_size;
    }
    conn->wps_conn_handle->ack_header_size = wps_get_connection_ack_header_size(&wps, wps_header_cfg);

    wps_configure_header_connection(&wps, conn->wps_conn_handle, wps_header_cfg, &wps_err);
    CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return);
    wps_configure_header_acknowledge(&wps, conn->wps_conn_handle, wps_header_cfg, &wps_err);
    CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return);

    /* Validate credit flow control auto reply connection requirement */
    for (uint32_t i = 0; i < conn->cfg.timeslot_count; ++i) {
        uint32_t id = conn->cfg.timeslot_id[i];

        if ((id & BIT_AUTO_REPLY_TIMESLOT)) {
            id = id & TIMESLOT_VALUE_MASK;
            wps_connection_t *first_main_connection = wps.mac.scheduler.schedule.timeslot[id].connection_main[0];
            wps_connection_t *first_connection = wps.mac.scheduler.schedule.timeslot[id].connection_auto_reply[0];

            if (first_main_connection != NULL) {
                /* If credit control flow is enabled for the main connection, it must also be enabled for the auto-reply
                 * connection
                 */
                CHECK_ERROR((first_main_connection->credit_flow_ctrl.enabled !=
                             first_connection->credit_flow_ctrl.enabled),
                            err, SWC_ERR_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD, return);
            }
        }
    }
}

void swc_connection_set_retransmission(swc_connection_t *conn, swc_node_t *node, bool enabled, uint32_t try_deadline,
                                       uint32_t time_deadline, swc_error_t *err)
{
    *err = SWC_ERR_NONE;
    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR((conn == NULL), err, SWC_ERR_NULL_PTR, return);
    CHECK_ERROR(enabled && !conn->wps_conn_handle->ack_enable, err, SWC_ERR_ARQ_WITH_ACK_DISABLED, return);
    CHECK_ERROR(enabled && !has_main_timeslot(conn->cfg.timeslot_id, conn->cfg.timeslot_count), err,
                SWC_ERR_ARQ_WITH_ACK_DISABLED, return);

    wps_error_t wps_err;

    if (enabled) {
        wps_connection_enable_stop_and_wait_arq(conn->wps_conn_handle, node->wps_node_handle->cfg.local_address,
                                                try_deadline, time_deadline, &wps_err);
    } else {
        wps_connection_disable_stop_and_wait_arq(conn->wps_conn_handle, &wps_err);
    }
}

void swc_connection_set_throttling_active_ratio(const swc_connection_t *const conn, uint8_t active_ratio,
                                                swc_error_t *err)
{
    wps_error_t wps_err;

    *err = SWC_ERR_NONE;

    CHECK_ERROR(conn == NULL, err, SWC_ERR_NULL_PTR, return);
    CHECK_ERROR(conn->wps_conn_handle->pattern == NULL, err, SWC_ERR_THROTTLING_NOT_SUPPORTED, return);

    wps_set_active_ratio(&wps, conn->wps_conn_handle, active_ratio, &wps_err);

    CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return);
}

void swc_connection_set_fec_ratio(swc_connection_t *conn, swc_fec_ratio_t ratio, swc_error_t *err)
{
    *err = SWC_ERR_NONE;

    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR(conn == NULL, err, SWC_ERR_NULL_PTR, return);
    CHECK_ERROR(!fec_ratio_supported(ratio), err, SWC_ERR_FEC_RATIO, return);

    conn->wps_conn_handle->frame_cfg.fec = fec_ratio_swc_to_wps(ratio);
}

void swc_connection_set_modulation(swc_connection_t *conn, swc_modulation_t modulation, swc_error_t *err)
{
    *err = SWC_ERR_NONE;

    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR(conn == NULL, err, SWC_ERR_NULL_PTR, return);
    CHECK_ERROR(!modulation_supported(modulation), err, SWC_ERR_MODULATION, return);

    if (modulation == SWC_MOD_OOK) {
        /* For OOK, CHIPCODE is the same as IOOK, but CHIPREPET bit #0 should be 1. */
        modulation = SWC_MOD_IOOK;
        conn->wps_conn_handle->frame_cfg.chip_repet |= CHIP_REPET_2;
    }

    conn->wps_conn_handle->frame_cfg.modulation = modulation_swc_to_wps(modulation);
}

void swc_connection_set_auto_sync(swc_connection_t *conn, bool enabled, swc_error_t *err)
{
    wps_error_t wps_err;

    *err = SWC_ERR_NONE;

    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR(conn == NULL, err, SWC_ERR_NULL_PTR, return);

    if (enabled) {
        wps_connection_enable_auto_sync(conn->wps_conn_handle, &wps_err);
    } else {
        wps_connection_disable_auto_sync(conn->wps_conn_handle, &wps_err);
    }
}

void swc_connection_set_throttling(swc_connection_t *conn, swc_error_t *err)
{
    wps_error_t wps_err;

    *err = SWC_ERR_NONE;

    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR(conn == NULL, err, SWC_ERR_NULL_PTR, return);

    conn->wps_conn_handle->pattern = mem_pool_malloc(&mem_pool, sizeof(bool) * WPS_PATTERN_THROTTLE_GRANULARITY);
    CHECK_ERROR(conn->wps_conn_handle->pattern == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return);

    wps_init_connection_throttle(conn->wps_conn_handle, &wps_err);
}

void swc_connection_set_connection_priority(swc_node_t *node, swc_connection_t *conn, uint8_t priority,
                                            swc_error_t *err)
{
    wps_error_t wps_err;
    wps_header_cfg_t wps_header_cfg = {0};
    uint8_t header_size;

    *err = SWC_ERR_NONE;

    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR((conn == NULL) || (node == NULL), err, SWC_ERR_NULL_PTR, return);
    CHECK_ERROR((priority > WPS_MAX_CONN_PRIORITY), err, SWC_ERR_MAX_CONN_PRIORITY, return);
    bool has_main_ts = has_main_timeslot(conn->cfg.timeslot_id, conn->cfg.timeslot_count);

    conn->wps_conn_handle->priority = priority;

    wps_header_cfg.main_connection = has_main_ts;
    wps_header_cfg.rdo_enabled = (has_main_ts && wps.mac.link_rdo.enabled) ? true : false;
    wps_header_cfg.connection_id = true;
    wps_header_cfg.ranging_mode = WPS_DEFAULT_RANGING;
    wps_header_cfg.credit_fc_enabled = conn->wps_conn_handle->credit_flow_ctrl.enabled;

    header_size = wps_get_connection_header_size(&wps, wps_header_cfg);
    CHECK_ERROR(((conn->cfg.max_payload_size + header_size + WPS_PAYLOAD_SIZE_BYTE_SIZE) > FRAME_SIZE_MAX), err,
                SWC_ERR_PAYLOAD_TOO_BIG, return);

    conn->wps_conn_handle->header_size = header_size;
    if (header_size > node->wps_node_handle->max_header_size) {
        node->wps_node_handle->max_header_size = header_size;
    }
    conn->wps_conn_handle->ack_header_size = wps_get_connection_ack_header_size(&wps, wps_header_cfg);

    wps_configure_header_connection(&wps, conn->wps_conn_handle, wps_header_cfg, &wps_err);
    CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return);
    wps_configure_header_acknowledge(&wps, conn->wps_conn_handle, wps_header_cfg, &wps_err);
    CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return);

    wps_connection_set_timeslot_priority(conn->wps_conn_handle, &wps, conn->cfg.timeslot_id, conn->cfg.timeslot_count,
                                         WPS_DEFAULT_SLOTS_PRIORITY);
}

void swc_connection_set_connection_slots_priority(swc_node_t *node, swc_connection_t *conn, uint8_t *slots_priority,
                                                  swc_error_t *err)
{
    wps_error_t wps_err;
    wps_header_cfg_t wps_header_cfg = {0};
    uint8_t header_size;

    *err = SWC_ERR_NONE;

    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);
    CHECK_ERROR((conn == NULL) || (node == NULL) || (slots_priority == NULL), err, SWC_ERR_NULL_PTR, return);
    CHECK_ERROR((conn->wps_conn_handle->priority > 0), err, SWC_ERR_NOT_ALLOWED_CONN_PRIORITY_CONFIGURATION, return);
    for (uint8_t timeslot_id = 0; timeslot_id < conn->cfg.timeslot_count; timeslot_id++) {
        CHECK_ERROR((slots_priority[timeslot_id] > WPS_MAX_CONN_PRIORITY), err, SWC_ERR_MAX_CONN_PRIORITY, return);
    }
    bool has_main_ts = has_main_timeslot(conn->cfg.timeslot_id, conn->cfg.timeslot_count);

    wps_header_cfg.main_connection = has_main_ts;
    wps_header_cfg.rdo_enabled = (has_main_ts && wps.mac.link_rdo.enabled) ? true : false;
    wps_header_cfg.connection_id = true;
    wps_header_cfg.ranging_mode = WPS_DEFAULT_RANGING;
    wps_header_cfg.credit_fc_enabled = conn->wps_conn_handle->credit_flow_ctrl.enabled;

    header_size = wps_get_connection_header_size(&wps, wps_header_cfg);
    CHECK_ERROR(((conn->cfg.max_payload_size + header_size + WPS_PAYLOAD_SIZE_BYTE_SIZE) > FRAME_SIZE_MAX), err,
                SWC_ERR_PAYLOAD_TOO_BIG, return);

    conn->wps_conn_handle->header_size = header_size;
    if (header_size > node->wps_node_handle->max_header_size) {
        node->wps_node_handle->max_header_size = header_size;
    }
    conn->wps_conn_handle->ack_header_size = wps_get_connection_ack_header_size(&wps, wps_header_cfg);

    wps_configure_header_connection(&wps, conn->wps_conn_handle, wps_header_cfg, &wps_err);
    CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return);
    wps_configure_header_acknowledge(&wps, conn->wps_conn_handle, wps_header_cfg, &wps_err);
    CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return);

    wps_connection_set_timeslot_priority(conn->wps_conn_handle, &wps, conn->cfg.timeslot_id, conn->cfg.timeslot_count,
                                         slots_priority);
}

void swc_connection_set_concurrency_cfg(const swc_connection_t *const conn, swc_connection_concurrency_cfg_t *cfg,
                                        swc_error_t *err)
{
    wps_error_t wps_err;

    CHECK_ERROR(cfg->enabled && !cca_fail_action_supported(cfg->fail_action), err, SWC_ERR_CCA_FAIL_ACTION, return);

    if (certification_mode_enabled) {
        /* Disable CCA to avoid changing timing of transmission. */
        cfg->enabled = false;
    }

    if (cfg->enabled) {
        wps_connection_enable_cca(conn->wps_conn_handle, WPS_DEFAULT_CCA_THRESHOLD, cfg->retry_time, cfg->try_count,
                                  cca_fail_action_swc_to_wps(cfg->fail_action), WPS_DEFAULT_CCA_ON_TIME_PLL_CYCLES,
                                  &wps_err);
        CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_CCA_INVALID_PARAMETERS, return);
    } else {
        wps_connection_disable_cca(conn->wps_conn_handle, &wps_err);
        CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return);
    }
}

void swc_connection_set_fallback_cfg(swc_connection_t *conn, swc_connection_fallback_cfg_t *cfg, swc_error_t *err)
{
    wps_error_t wps_err;
    *err = SWC_ERR_NONE;

    if (cfg->enabled && cfg->fallback_mode_count > 0) {

        /* loop through the fallback thresholds and check if they are in descending order */
        for (uint8_t i = 0; i < cfg->fallback_mode_count - 1; i++) {
            if (cfg->thresholds[i] <= cfg->thresholds[i + 1]) {
                *err = SWC_ERR_INVALID_PARAMETER;
                return;
            }
        }

        uint8_t *fallback_cca_try_count = mem_pool_malloc(&mem_pool, sizeof(uint8_t) * (cfg->fallback_mode_count));

        CHECK_ERROR(fallback_cca_try_count == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return);
        memcpy(fallback_cca_try_count, cfg->cca_try_count, cfg->fallback_mode_count * sizeof(uint8_t));

        link_cca_set_fbk_try_count(&conn->wps_conn_handle->cca, fallback_cca_try_count, cfg->fallback_mode_count);

        uint8_t *fallback_threshold = mem_pool_malloc(&mem_pool, sizeof(uint8_t) * cfg->fallback_mode_count);

        CHECK_ERROR(fallback_threshold == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return);
        memcpy(fallback_threshold, cfg->thresholds, cfg->fallback_mode_count);

        rf_channel_array_t fallback_channel_buffer;
        uint8_t channel_count = wps_get_channel_count(&wps, &wps_err);

        /* Allocate memory for fallback configuration */
        fallback_channel_buffer = mem_pool_malloc(&mem_pool,
                                                  cfg->fallback_mode_count * sizeof(rf_channel_t(*)[WPS_RADIO_COUNT]));
        for (size_t fallback_count_index = 0; fallback_count_index < cfg->fallback_mode_count; fallback_count_index++) {
            fallback_channel_buffer[fallback_count_index] =
                mem_pool_malloc(&mem_pool, channel_count * sizeof(rf_channel_t[WPS_RADIO_COUNT]));
        }

        wps_connection_enable_fallback(conn->wps_conn_handle, fallback_threshold, cfg->fallback_mode_count,
                                       fallback_channel_buffer, &wps_err);
    } else {
        link_cca_set_fbk_try_count(&conn->wps_conn_handle->cca, NULL, 0);
    }
}

void swc_set_time_slots_sleep_level(swc_sleep_level_t *sleep_level, swc_error_t *err)
{
    *err = SWC_ERR_NONE;

    wps.mac.scheduler.schedule.lightest_sleep_lvl = SLEEP_DEEP;

    for (uint32_t i = 0; i < wps.mac.scheduler.schedule.size; i++) {
        CHECK_ERROR(((sleep_level[i] == SWC_SLEEP_IDLE) || (sleep_level[i] == SWC_SLEEP_SHALLOW) ||
                     (sleep_level[i] == SWC_SLEEP_DEEP)) == false,
                    err, SWC_ERR_INCORRECT_TS_SLEEP_LEVEL, return);
        wps.mac.scheduler.schedule.timeslot[i].sleep_lvl = sleep_level_swc_to_wps(sleep_level[i]);
        if (sleep_level_swc_to_wps(sleep_level[i]) < wps.mac.scheduler.schedule.lightest_sleep_lvl) {
            wps.mac.scheduler.schedule.lightest_sleep_lvl = sleep_level_swc_to_wps(sleep_level[i]);
        }
    }
}

void swc_connection_get_payload_buffer(const swc_connection_t *const conn, uint8_t **const payload_buffer,
                                       swc_error_t *err)
{
    wps_error_t wps_err;

    *err = SWC_ERR_NONE;

    CHECK_ERROR((conn == NULL) || (payload_buffer == NULL), err, SWC_ERR_NULL_PTR, return);
    CHECK_ERROR((conn->wps_conn_handle->frag.enabled), err, SWC_ERR_FRAGMENTATION_NOT_SUPPORTED, return);

    wps_get_free_slot(conn->wps_conn_handle, payload_buffer, conn->cfg.max_payload_size, &wps_err);
    if (wps_err != WPS_NO_ERROR) {
        *err = SWC_ERR_NO_BUFFER_AVAILABLE;
        *payload_buffer = NULL;
        return;
    }
}

void swc_connection_allocate_payload_buffer(const swc_connection_t *const conn, uint8_t **const payload_buffer,
                                            uint16_t payload_size, swc_error_t *err)
{
    wps_error_t wps_err;

    *err = SWC_ERR_NONE;

    CHECK_ERROR((conn == NULL) || (payload_buffer == NULL), err, SWC_ERR_NULL_PTR, return);
    CHECK_ERROR((conn->wps_conn_handle->frag.enabled), err, SWC_ERR_FRAGMENTATION_NOT_SUPPORTED, return);
    CHECK_ERROR((payload_size == 0 || payload_size > conn->cfg.max_payload_size), err, SWC_ERR_INVALID_PARAMETER,
                return);

    *payload_buffer = NULL;

    wps_get_free_slot(conn->wps_conn_handle, payload_buffer, payload_size, &wps_err);
    if (wps_err == WPS_NOT_ENOUGH_MEMORY_ERROR) {
        *err = SWC_ERR_NOT_ENOUGH_MEMORY;
    } else if (wps_err != WPS_NO_ERROR) {
        *err = SWC_ERR_NO_BUFFER_AVAILABLE;
    }
}

void swc_connection_send(const swc_connection_t *const conn, const uint8_t *const payload_buffer, uint16_t size,
                         swc_error_t *err)
{
    wps_error_t wps_err;

    *err = SWC_ERR_NONE;

    CHECK_ERROR((conn == NULL) || (payload_buffer == NULL), err, SWC_ERR_NULL_PTR, return);
    if (is_rx_connection(wps.node->cfg.local_address, conn->cfg.source_address)) {
        *err = SWC_ERR_SEND_ON_RX_CONN;
        return;
    }

#if !WPS_DISABLE_FRAGMENTATION
    if (!conn->wps_conn_handle->frag.enabled) {
        wps_send(conn->wps_conn_handle, payload_buffer, size, &wps_err);
    } else {
        wps_frag_send(conn->wps_conn_handle, payload_buffer, size, &wps_err);
    }
#else
    wps_send(conn->wps_conn_handle, payload_buffer, size, &wps_err);
#endif

    if (wps_err == WPS_WRONG_TX_SIZE_ERROR) {
        *err = SWC_ERR_SIZE_TOO_BIG;
        return;
    } else if (wps_err == WPS_QUEUE_FULL_ERROR) {
        *err = SWC_ERR_SEND_QUEUE_FULL;
        return;
    } else if (wps_err == WPS_NOT_ENOUGH_MEMORY_ERROR) {
        *err = SWC_ERR_NO_BUFFER_AVAILABLE;
        return;
    } else if (wps_err != WPS_NO_ERROR) {
        *err = SWC_ERR_INTERNAL;
        return;
    }
}

uint16_t swc_connection_receive(const swc_connection_t *const conn, uint8_t **const payload, swc_error_t *err)
{
    wps_error_t wps_err;
    wps_rx_frame frame;

    *err = SWC_ERR_NONE;

    CHECK_ERROR((conn == NULL) || (payload == NULL), err, SWC_ERR_NULL_PTR, return 0);
    CHECK_ERROR((conn->wps_conn_handle->frag.enabled), err, SWC_ERR_FRAGMENTATION_NOT_SUPPORTED, return 0);

    frame = wps_read(conn->wps_conn_handle, &wps_err);
    if (wps_err != WPS_NO_ERROR) {
        *err = SWC_ERR_RECEIVE_QUEUE_EMPTY;
        *payload = NULL;
        return 0;
    }
    *payload = frame.payload;

    return frame.size;
}

uint16_t swc_connection_receive_get_payload_size(const swc_connection_t *const conn, swc_error_t *err)
{
    wps_error_t wps_err;
    uint16_t payload_size;

    *err = SWC_ERR_NONE;

    CHECK_ERROR((conn == NULL), err, SWC_ERR_NULL_PTR, return 0);

#if !WPS_DISABLE_FRAGMENTATION
    if (conn->wps_conn_handle->frag.enabled) {
        payload_size = wps_frag_get_read_payload_size(conn->wps_conn_handle, &wps_err);
    } else {
        payload_size = wps_get_read_payload_size(conn->wps_conn_handle, &wps_err);
    }
#else
    payload_size = wps_get_read_payload_size(conn->wps_conn_handle, &wps_err);
#endif

    if (wps_err != WPS_NO_ERROR) {
        *err = SWC_ERR_RECEIVE_QUEUE_EMPTY;
        return 0;
    }

    return payload_size;
}

void swc_connection_receive_complete(const swc_connection_t *const conn, swc_error_t *err)
{
    wps_error_t wps_err;

    *err = SWC_ERR_NONE;

    CHECK_ERROR(conn == NULL, err, SWC_ERR_NULL_PTR, return);

    wps_read_done(conn->wps_conn_handle, &wps_err);

    CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_RECEIVE_QUEUE_EMPTY, return);
}

void swc_reserved_address_unlock(void)
{
    reserved_address_lock = false;
}

void swc_reserved_address_lock(void)
{
    reserved_address_lock = true;
}

uint16_t swc_connection_receive_to_buffer(const swc_connection_t *const conn, uint8_t *const payload, uint16_t size,
                                          swc_error_t *err)
{
    wps_error_t wps_err;
    wps_rx_frame frame;

    *err = SWC_ERR_NONE;

    CHECK_ERROR((conn == NULL) || (payload == NULL), err, SWC_ERR_NULL_PTR, return 0);

#if !WPS_DISABLE_FRAGMENTATION
    if (conn->wps_conn_handle->frag.enabled) {
        frame = wps_frag_read(conn->wps_conn_handle, payload, size, &wps_err);
    } else {
        frame = wps_read_to_buffer(conn->wps_conn_handle, payload, size, &wps_err);
    }
#else
    frame = wps_read_to_buffer(conn->wps_conn_handle, payload, size, &wps_err);
#endif

    if (wps_err == WPS_WRONG_RX_SIZE_ERROR) {
        *err = SWC_ERR_BUFFER_SIZE_TOO_SMALL;
        return 0;
    } else if (wps_err != WPS_NO_ERROR) {
        *err = SWC_ERR_RECEIVE_QUEUE_EMPTY;
        return 0;
    }
    return frame.size;
}

uint16_t swc_connection_get_enqueued_count(const swc_connection_t *const conn, swc_error_t *err)
{
    *err = SWC_ERR_NONE;

#if !WPS_DISABLE_FRAGMENTATION
    if (conn->wps_conn_handle->frag.enabled) {
        return wps_frag_get_fifo_size(conn->wps_conn_handle);
    } else {
        return wps_get_fifo_size(conn->wps_conn_handle);
    }
#else
    return wps_get_fifo_size(conn->wps_conn_handle);
#endif
}

bool swc_connection_get_connect_status(const swc_connection_t *const conn, swc_error_t *err)
{
    *err = SWC_ERR_NONE;

    return wps_get_connect_status(conn->wps_conn_handle);
}

void swc_setup(const swc_node_t *const node, swc_error_t *err)
{
    wps_error_t wps_err;
    uint8_t *xlayer_tx_pool;
    uint8_t *xlayer_rx_pool;
    uint32_t required_callback_queue_size = 0;

    CHECK_ERROR(is_started == true, err, SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING, return);

    *err = SWC_ERR_NONE;

    CHECK_ERROR(node == NULL, err, SWC_ERR_NULL_PTR, return);

#if (WPS_RADIO_COUNT == 2)
    CHECK_ERROR(node->wps_radio_handle[1].radio.radio_id != 1, err, SWC_ERR_SECOND_RADIO_NOT_INIT, return);
#endif

    check_global_auto_connection_errors(wps.mac.scheduler.schedule.timeslot, wps.mac.scheduler.schedule.size, err);
    CHECK_ERROR(*err != SWC_ERR_NONE, err, *err, return);

    allocate_payload_and_header_buffer_memory(node, err);
    CHECK_ERROR(*err != SWC_ERR_NONE, err, *err, return);

    xlayer_tx_pool =
        (uint8_t *)mem_pool_malloc(&mem_pool, wps_get_xlayer_tx_queue_nb_bytes_needed(node->wps_node_handle, &wps_err));
    CHECK_ERROR(xlayer_tx_pool == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return);

    xlayer_rx_pool =
        (uint8_t *)mem_pool_malloc(&mem_pool, wps_get_xlayer_rx_queue_nb_bytes_needed(node->wps_node_handle, &wps_err));
    CHECK_ERROR(xlayer_rx_pool == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return);

    wps_init_xlayer(node->wps_node_handle, xlayer_tx_pool, xlayer_rx_pool, &wps_err);

    /* Determine callbacks count and add a margin for other event callbacks */
    required_callback_queue_size = calculate_activated_callback_count(node) + WPS_QUEUE_MARGIN;

    /* Allocate the callback queue based on the required size */
    wps_callback_inst_t *callback_queue = mem_pool_malloc(&mem_pool,
                                                          sizeof(wps_callback_inst_t) * required_callback_queue_size);

    CHECK_ERROR(callback_queue == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return);

    /* Initialize the callback queue */
    wps_init_callback_queue(&wps, callback_queue, required_callback_queue_size);

    wps_init(&wps, node->wps_node_handle, &wps_err);

    validate_connection_priority_in_schedule(node, err);
    CHECK_ERROR(*err != SWC_ERR_NONE, err, *err, return);
    CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return);

    wps_connection_list_iterate_connections(&node->wps_node_handle->conn_list, validate_channels, err);
    if (*err != SWC_ERR_NONE) {
        return;
    }

    if (certification_mode_enabled) {
        wps_mac_certification_init(&wps.mac);
    }
}

swc_status_t swc_get_status(void)
{
    if (is_started) {
        return SWC_STATUS_RUNNING;
    } else {
        return SWC_STATUS_STOPPED;
    }
}

void swc_connect(swc_error_t *err)
{
    wps_error_t wps_err;

    *err = SWC_ERR_NONE;

#if WPS_RADIO_COUNT == 2
    swc_hal_multi_radio_timer_init();
#endif

    wps_connect(&wps, &wps_err);

    CHECK_ERROR(wps_err == WPS_NOT_INIT_ERROR, err, SWC_ERR_NOT_INITIALIZED, return);
    CHECK_ERROR(wps_err == WPS_ALREADY_CONNECTED_ERROR, err, SWC_ERR_ALREADY_CONNECTED, return);
    CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return);
    is_started = true;
}

void swc_disconnect(swc_error_t *err)
{
    wps_error_t wps_err;

    *err = SWC_ERR_NONE;

    wps_disconnect(&wps, &wps_err);
    is_started = false;
    CHECK_ERROR(wps_err == WPS_ALREADY_DISCONNECTED_ERROR, err, SWC_ERR_NOT_CONNECTED, return);
    CHECK_ERROR(wps_err == WPS_DISCONNECT_TIMEOUT_ERROR, err, SWC_ERR_DISCONNECT_TIMEOUT, return);
    CHECK_ERROR(wps_err == WPS_NOT_INIT_ERROR, err, SWC_ERR_NOT_INITIALIZED, return);
    CHECK_ERROR(wps_err != WPS_NO_ERROR, err, SWC_ERR_INTERNAL, return);
}

swc_event_t swc_get_event(const swc_connection_t *const conn)
{
    switch (wps_get_event(conn->wps_conn_handle)) {
    case WPS_EVENT_CONNECT:
        return SWC_EVENT_CONNECT;
    case WPS_EVENT_DISCONNECT:
        return SWC_EVENT_DISCONNECT;
    case WPS_EVENT_ERROR:
        return SWC_EVENT_ERROR;
    default:
        return SWC_EVENT_NONE;
    }
}

swc_error_t swc_get_event_error(const swc_connection_t *const conn)
{
    switch (wps_get_error(conn->wps_conn_handle)) {
    case WPS_RX_OVERRUN_ERROR:
        return SWC_ERR_RX_OVERRUN;
    default:
        return SWC_ERR_INTERNAL;
    }
}

swc_fallback_info_t swc_connection_get_fallback_info(const swc_connection_t *const conn, swc_error_t *err)
{
    swc_fallback_info_t info = {0};
    int32_t link_margin;

    *err = SWC_ERR_NONE;

    if (conn == NULL) {
        *err = SWC_ERR_NULL_PTR;
        return info;
    }

#if WPS_ENABLE_PHY_STATS
    link_margin = wps_stats_get_inst_phy_margin(conn->wps_conn_handle);
#else
    link_margin = 0;
#endif
    if (link_margin > UINT8_MAX) {
        info.link_margin = UINT8_MAX;
    } else if (link_margin < 0) {
        info.link_margin = 0;
    } else {
        info.link_margin = (uint8_t)link_margin;
    }

    info.cca_fail_count = wps_get_phy_total_cca_fail_count(conn->wps_conn_handle);
    info.cca_tx_fail_count = wps_get_phy_total_cca_tx_fail_count(conn->wps_conn_handle);
    info.tx_pkt_dropped = wps_get_phy_total_pkt_dropped(conn->wps_conn_handle);
    info.cca_event_count = wps_get_phy_total_cca_events(conn->wps_conn_handle);

    return info;
}

uint32_t swc_get_allocated_bytes(void)
{
    return mem_pool_get_allocated_bytes(&mem_pool);
}

void swc_free_memory(void)
{
    is_started = false;
    memset(&wps, 0, sizeof(wps_t));
    mem_pool_free(&mem_pool);
}

void swc_connection_callbacks_processing_handler(void)
{
    wps_process_callback(&wps);
}

void swc_send_tx_flush_request(const swc_connection_t *const conn)
{
    conn->wps_conn_handle->tx_flush = true;
}

#if (WPS_RADIO_COUNT == 1)
void swc_radio_irq_handler(void)
{
    wps_radio_irq(&wps);
}

void swc_radio_spi_receive_complete_handler(void)
{
    wps_transfer_complete(&wps);
}
#elif (WPS_RADIO_COUNT == 2)
void swc_radio1_irq_handler(void)
{
    wps_set_irq_index(0);
    wps_radio_irq(&wps);
}

void swc_radio1_spi_receive_complete_handler(void)
{
    wps_set_irq_index(0);
    wps_transfer_complete(&wps);
}

void swc_radio2_irq_handler(void)
{
    wps_set_irq_index(1);
    wps_radio_irq(&wps);
}

void swc_radio2_spi_receive_complete_handler(void)
{
    wps_set_irq_index(1);
    wps_transfer_complete(&wps);
}

void swc_radio_synchronization_timer_callback(void)
{
    wps_multi_radio_timer_process(&wps);
}

void swc_radio_select(multi_radio_select_t radio_select)
{
    wps_phy_set_radio_select(radio_select);
}
#else
#error "Number of radios must be either 1 or 2"
#endif

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Check if a connection is using at least on main timeslot.
 *
 *  @param[in] timeslot_id     ID of timeslots used by the connection.
 *  @param[in] timeslot_count  Number of timeslots used by the connection.
 *  @retval true  Connection is using at least one main timeslot.
 *  @retval false Connection is using only auto-reply timeslots.
 */
static bool has_main_timeslot(const int32_t *timeslot_id, uint32_t timeslot_count)
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

/** @brief Calculate the increment step of the RDO from an increment time in miliseconds
 *
 *  @param[in] timeslot_sequence        Network schedule as an array of timeslot durations in microseconds.
 *  @param[in] timeslot_sequence_length Number of timeslots in the timeslot sequence.
 *  @param[in] rdo_step_ms              Time between RDO increment in ms.
 *  @return    RDO increment step.
 */
static uint16_t get_rdo_increment_step(const uint32_t *timeslot_sequence, uint32_t timeslot_sequence_length,
                                       uint32_t rdo_step_ms)
{
    uint32_t average = 0;

    for (size_t i = 0; i < timeslot_sequence_length; i++) {
        average += timeslot_sequence[i];
    }
    average = average / timeslot_sequence_length;

    return rdo_step_ms * 1000 / average;
}

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

/** @brief Check if the specified addresses implicate the local device (local address), otherwise the connection is
 * invalid.
 *
 *  @param[in] local_address        Node's local address.
 *  @param[in] destination_address  Connection's destination address.
 *  @param[in] source_address       Connection's source address.
 *  @retval true   The address is valid for this device.
 *  @retval false  The address isn't valid for this device.
 */
static bool is_connection_address_valid(uint8_t local_address, uint8_t destination_address, uint8_t source_address)
{
    return (local_address == destination_address) || (local_address == source_address);
}

/** @brief Check if the network role is supported.
 *
 *  @param[in] role  Node's network role.
 *  @retval true  Supported.
 *  @retval false Not supported.
 */
static bool network_role_supported(swc_role_t role)
{
    switch (role) {
    case SWC_ROLE_COORDINATOR:
    case SWC_ROLE_NODE:
        return true;
    default:
        return false;
    }
}

/** @brief Convert SWC network role to WPS's.
 *
 *  @param[in] role  Node's network role (SWC type).
 *  @return Node's network role (WPS type).
 */
static wps_role_t network_role_swc_to_wps(swc_role_t role)
{
    switch (role) {
    case SWC_ROLE_COORDINATOR:
        return NETWORK_COORDINATOR;
    case SWC_ROLE_NODE:
        return NETWORK_NODE;
    default: /* This should never happen */
        while (1) {};
    }
}

/** @brief Check if the sleep level is supported.
 *
 *  @param[in] level     Node's sleep level.
 *  @param[in] schedule  Pointer to WPS schedule.
 *  @retval true  Supported.
 *  @retval false Not supported.
 */
static bool sleep_level_supported(swc_sleep_level_t level, schedule_t *schedule)
{
    (void)schedule;

    switch (level) {
    case SWC_SLEEP_IDLE:
    case SWC_SLEEP_SHALLOW:
    case SWC_SLEEP_DEEP:
        return true;
    default:
        return false;
    }
}

/** @brief Convert SWC sleep level to WPS's.
 *
 *  @param[in] level  Node's sleep level (SWC type).
 *  @return Node's sleep level (WPS type).
 */
static sleep_lvl_t sleep_level_swc_to_wps(swc_sleep_level_t level)
{
    switch (level) {
    case SWC_SLEEP_IDLE:
        return SLEEP_IDLE;
    case SWC_SLEEP_SHALLOW:
        return SLEEP_SHALLOW;
    case SWC_SLEEP_DEEP:
        return SLEEP_DEEP;
    default: /* This should never happen */
        while (1) {};
    }
}

/** @brief Convert SWC ISI mitigation level to WPS's.
 *
 *  @param[in] isi_mitig  Node's ISI mitigation level (SWC type).
 *  @return Node's ISI mitigation level (WPS type).
 */
static isi_mitig_t isi_mitig_swc_to_wps(swc_isi_mitig_t isi_mitig)
{
    switch (isi_mitig) {
    case SWC_ISI_MITIG_0:
        return ISI_MITIG_0;
    case SWC_ISI_MITIG_1:
        return ISI_MITIG_1;
    case SWC_ISI_MITIG_2:
        return ISI_MITIG_2;
    case SWC_ISI_MITIG_3:
        return ISI_MITIG_3;
    default: /* This should never happen */
        while (1) {};
    }
}

/** @brief Convert SWC chip rate to WPS's.
 *
 *  @param[in] level  Node's chip rate (SWC type).
 *  @return Node's chip rate (WPS type).
 */
static chip_rate_cfg_t chip_rate_swc_to_wps(swc_chip_rate_t chip_rate)
{
    switch (chip_rate) {
    case SWC_CHIP_RATE_20_48_MHZ:
        return CHIP_RATE_20_48_MHZ;
    case SWC_CHIP_RATE_27_30_MHZ:
        return CHIP_RATE_27_30_MHZ;
    case SWC_CHIP_RATE_40_96_MHZ:
        return CHIP_RATE_40_96_MHZ;
    default: /* This should never happen */
        while (1) {};
    }
}

/** @brief Check if the IRQ polarity is supported.
 *
 *  @param[in] pol  Radio's IRQ polarity.
 *  @retval true  Supported.
 *  @retval false Not supported.
 */
static bool irq_polarity_supported(swc_irq_polarity_t pol)
{
    switch (pol) {
    case SWC_IRQ_ACTIVE_LOW:
    case SWC_IRQ_ACTIVE_HIGH:
        return true;
    default:
        return false;
    }
}

/** @brief Convert SWC IRQ polarity to WPS's.
 *
 *  @param[in] pol  Radio's IRQ polarity (SWC type).
 *  @return Radio's IRQ polarity (WPS type).
 */
static irq_polarity_t irq_polarity_swc_to_wps(swc_irq_polarity_t pol)
{
    switch (pol) {
    case SWC_IRQ_ACTIVE_LOW:
        return IRQ_ACTIVE_LOW;
    case SWC_IRQ_ACTIVE_HIGH:
        return IRQ_ACTIVE_HIGH;
    default: /* This should never happen */
        while (1) {};
    }
}

/** @brief Check if the SPI mode is supported.
 *
 *  @param[in] mode  Radio's SPI mode.
 *  @retval true  Supported.
 *  @retval false Not supported.
 */
static bool spi_mode_supported(swc_spi_mode_t mode)
{
    switch (mode) {
    case SWC_SPI_STANDARD:
    case SWC_SPI_FAST:
        return true;
    default:
        return false;
    }
}

/** @brief Convert SWC SPI mode to WPS's.
 *
 *  @param[in] mode  Radio's SPI mode (SWC type).
 *  @return Radio's SPI mode (WPS type).
 */
static std_spi_t spi_mode_swc_to_wps(swc_spi_mode_t mode)
{
    switch (mode) {
    case SWC_SPI_STANDARD:
        return SPI_STANDARD;
    case SWC_SPI_FAST:
        return SPI_FAST;
    default: /* This should never happen */
        while (1) {};
    }
}

/** @brief Check if the digital output driver impedance is supported.
 *
 *  @param[in] outimped  Radio's digital output driver impedance.
 *  @retval true  Supported.
 *  @retval false Not supported.
 */
static bool outimped_supported(swc_outimped_t outimped)
{
    switch (outimped) {
    case SWC_OUTIMPED_0:
    case SWC_OUTIMPED_1:
    case SWC_OUTIMPED_2:
    case SWC_OUTIMPED_3:
        return true;
    default:
        return false;
    }
}

/** @brief Convert SWC digital output driver impedance to WPS's.
 *
 *  @param[in] outimped  Radio's digital output driver impedance (SWC type).
 *  @return Radio's digital output driver impedance (WPS type).
 */
static outimped_t outimped_swc_to_wps(swc_outimped_t outimped)
{
    switch (outimped) {
    case SWC_OUTIMPED_0:
        return OUTIMPED_0;
    case SWC_OUTIMPED_1:
        return OUTIMPED_1;
    case SWC_OUTIMPED_2:
        return OUTIMPED_2;
    case SWC_OUTIMPED_3:
        return OUTIMPED_3;
    default: /* This should never happen */
        while (1) {};
    }
}

/** @brief Check if the modulation is supported.
 *
 *  @param[in] modulation  Connection's modulation.
 *  @retval true  Supported.
 *  @retval false Not supported.
 */
static bool modulation_supported(swc_modulation_t modulation)
{
    switch (modulation) {
    case SWC_MOD_OOK:
    case SWC_MOD_IOOK:
    case SWC_MOD_PPM:
    case SWC_MOD_2BITPPM:
        return true;
    default:
        return false;
    }
}

/** @brief Convert SWC modulation to WPS's.
 *
 *  @param[in] modulation  Connection's modulation (SWC type).
 *  @return Connection's modulation (WPS type).
 */
static modulation_t modulation_swc_to_wps(swc_modulation_t modulation)
{
    switch (modulation) {
    case SWC_MOD_OOK:
        return MODULATION_OOK;
    case SWC_MOD_IOOK:
        return MODULATION_IOOK;
    case SWC_MOD_PPM:
        return MODULATION_PPM;
    case SWC_MOD_2BITPPM:
        return MODULATION_2BITPPM;
    default: /* This should never happen */
        while (1) {};
    }
}

/** @brief Convert SWC chip repetition to WPS's.
 *
 *  @param[in] chip_repet  Connection's chip repetition (SWC type).
 *  @return Connection's chip repetition (WPS type).
 */
static chip_repetition_t chip_repetition_swc_to_wps(swc_chip_repetition_t chip_repet)
{
    switch (chip_repet) {
    case SWC_CHIP_REPET_1:
        return CHIP_REPET_1;
    case SWC_CHIP_REPET_2:
        return CHIP_REPET_2;
    case SWC_CHIP_REPET_3:
        return CHIP_REPET_3;
    case SWC_CHIP_REPET_4:
        return CHIP_REPET_4;
    default: /* This should never happen */
        while (1) {};
    }
}

/** @brief Check if the FEC ratio is supported.
 *
 *  @param[in] ratio  Connection's FEC ratio.
 *  @retval true  Supported.
 *  @retval false Not supported.
 */
static bool fec_ratio_supported(swc_fec_ratio_t ratio)
{
    switch (ratio) {
    case SWC_FEC_1_0_0_0:
    case SWC_FEC_1_2_5_0:
    case SWC_FEC_1_3_7_5:
    case SWC_FEC_1_5_0_0:
    case SWC_FEC_1_6_2_5:
    case SWC_FEC_1_7_5_0:
    case SWC_FEC_1_8_7_5:
    case SWC_FEC_2_0_0_0:
        return true;
    default:
        return false;
    }
}

/** @brief Convert SWC FEC ratio to WPS's.
 *
 *  @param[in] ratio  Connection's FEC ratio (SWC type).
 *  @return Connection's FEC ratio (WPS type).
 */
static fec_level_t fec_ratio_swc_to_wps(swc_fec_ratio_t ratio)
{
    switch (ratio) {
    case SWC_FEC_1_0_0_0:
        return FEC_LVL_0;
    case SWC_FEC_1_2_5_0:
        return FEC_LVL_1;
    case SWC_FEC_1_3_7_5:
        return FEC_LVL_2;
    case SWC_FEC_1_5_0_0:
        return FEC_LVL_3;
    case SWC_FEC_1_6_2_5:
        return FEC_LVL_4;
    case SWC_FEC_1_7_5_0:
        return FEC_LVL_5;
    case SWC_FEC_1_8_7_5:
        return FEC_LVL_6;
    case SWC_FEC_2_0_0_0:
        return FEC_LVL_7;
    default: /* This should never happen */
        while (1) {};
    }
}

/** @brief Check if the CCA fail action is supported.
 *
 *  @param[in] action  Connection's CCA fail action.
 *  @retval true  Supported.
 *  @retval false Not supported.
 */
static bool cca_fail_action_supported(swc_cca_fail_action_t action)
{
    switch (action) {
    case SWC_CCA_FORCE_TX:
    case SWC_CCA_ABORT_TX:
        return true;
    default:
        return false;
    }
}

/** @brief Convert SWC CCA fail action to WPS's.
 *
 *  @param[in] action  Connection's CCA fail action (SWC type).
 *  @return Connection's CCA fail action (WPS type).
 */
static cca_fail_action_t cca_fail_action_swc_to_wps(swc_cca_fail_action_t action)
{
    switch (action) {
    case SWC_CCA_FORCE_TX:
        return CCA_FAIL_ACTION_TX;
    case SWC_CCA_ABORT_TX:
        return CCA_FAIL_ACTION_ABORT_TX;
    default: /* This should never happen */
        while (1) {};
    }
}

/** @brief Return integgain value based on radio chip rate, modulation and pulse_count
 *
 *  @param[in] chip_rate    Radio PHY Rate.
 *  @param[in] modulation   Connection modulation.
 *  @param[in] pulse_count  Channel pulse count.
 *  @return Channel integgain.
 */
static uint8_t get_integgain(chip_rate_cfg_t chip_rate, uint8_t pulse_count)
{
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

/** @brief Save the current NVM and calibration.
 *
 *  @param[in] radio_id    Radio number.
 *  @param[in] nvm         NVM structure.
 *  @param[in] calib_vars  Calibration structure.
 */
static void save_radio_configuration(uint8_t radio_id, nvm_t *nvm, calib_vars_t *calib_vars)
{
    memcpy(&saved_nvm[radio_id], nvm, sizeof(nvm_t));
    memcpy(&saved_calib_vars[radio_id], calib_vars, sizeof(calib_vars_t));
}

/** @brief Get the previously saved calibration and NVM using save_radio_configuration.
 *
 * @param[in]  radio_id    Radio number.
 * @param[out] nvm         NVM structure.
 * @param[out] calib_vars  Calibration structure.
 */
static void get_saved_radio_configuration(uint8_t radio_id, nvm_t *nvm, calib_vars_t *calib_vars)
{
    memcpy(nvm, &saved_nvm[radio_id], sizeof(nvm_t));
    memcpy(calib_vars, &saved_calib_vars[radio_id], sizeof(calib_vars_t));
}

/** @brief Do main connection priority error checks.
 *
 * Some main connection fields must be identical in all connections assigned to the same timeslot,
 * we need to return an error to the SWC user if this is not the case for any of those fields.
 * This check needs to be done every time a connection is assigned to a time slot.
 *
 * @param[in]  timeslots  Timeslot.
 * @param[out] err  Wireless Core error code.
 */
static void check_main_connection_priority_errors(const swc_node_t *const node, timeslot_t timeslot, swc_error_t *err)
{
    wps_connection_t *connection;
    wps_connection_t *first_connection = timeslot.connection_main[0];
    bool is_timeslot_rx = is_rx_connection(node->cfg.local_address, first_connection->source_address);

    for (uint8_t i = 1; i < timeslot.main_connection_count; i++) {
        connection = timeslot.connection_main[i];

        if (is_timeslot_rx) {
            /* Both local address should match. */
            CHECK_ERROR((first_connection->destination_address != connection->destination_address), err,
                        SWC_ERR_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD, return);
            /* If first connection does not have ack, radio won't be configured with acks. */
            CHECK_ERROR((!first_connection->ack_enable && connection->ack_enable), err,
                        SWC_ERR_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD, return);
        } else {
            /* Both local address should match. */
            CHECK_ERROR((first_connection->source_address != connection->source_address), err,
                        SWC_ERR_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD, return);
        }
        CHECK_ERROR(memcmp(&first_connection->link_phase, &connection->link_phase, sizeof(link_phase_t)), err,
                    SWC_ERR_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD, return);
        CHECK_ERROR((first_connection->header_size != connection->header_size), err,
                    SWC_ERR_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD, return);
        CHECK_ERROR(memcmp(&first_connection->link_protocol, &connection->link_protocol, sizeof(link_protocol_t)), err,
                    SWC_ERR_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD, return);
#if !WPS_DISABLE_FRAGMENTATION
        CHECK_ERROR(memcmp(&first_connection->frag, &connection->frag, sizeof(frag_t)), err,
                    SWC_ERR_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD, return);
#endif
        CHECK_ERROR(memcmp(&first_connection->frame_cfg, &connection->frame_cfg, sizeof(frame_cfg_t)), err,
                    SWC_ERR_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD, return);
    }
}

/** @brief Do auto connection priority error checks.
 *
 * Some auto connection fields must be identical in all connections assigned to the same timeslot,
 * we need to return an error to the SWC user if this is not the case for any of those fields.
 * This check needs to be done every time a connection is assigned to a time slot.
 *
 * @param[in]  timeslots  Timeslot.
 * @param[out] err  Wireless Core error code.
 */
static void check_auto_connection_priority_errors(const swc_node_t *const node, timeslot_t timeslot, swc_error_t *err)
{
    wps_connection_t *connection;
    wps_connection_t *first_connection = timeslot.connection_auto_reply[0];
    bool is_timeslot_rx = is_rx_connection(node->cfg.local_address, first_connection->source_address);

    for (uint8_t i = 1; i < timeslot.main_connection_count; i++) {
        connection = timeslot.connection_main[i];
        /* Main connection source address should match auto reply destination address. */
        CHECK_ERROR((first_connection->destination_address != connection->source_address), err,
                    SWC_ERR_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD, return);
        /* Main connection destination address should match auto reply source address. */
        CHECK_ERROR((first_connection->source_address != connection->destination_address), err,
                    SWC_ERR_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD, return);
    }

    for (uint8_t i = 1; i < timeslot.auto_connection_count; i++) {
        connection = timeslot.connection_auto_reply[i];

        /* Both destination and source address should match. */
        CHECK_ERROR((first_connection->destination_address != connection->destination_address), err,
                    SWC_ERR_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD, return);
        CHECK_ERROR((first_connection->source_address != connection->source_address), err,
                    SWC_ERR_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD, return);
        if (is_timeslot_rx) {
            /* If first connection does not have ack, radio won't be configured with acks. */
            CHECK_ERROR((!first_connection->ack_enable && connection->ack_enable), err,
                        SWC_ERR_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD, return);
        }
        CHECK_ERROR(memcmp(&first_connection->link_phase, &connection->link_phase, sizeof(link_phase_t)), err,
                    SWC_ERR_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD, return);
        CHECK_ERROR((first_connection->header_size != connection->header_size), err,
                    SWC_ERR_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD, return);
        CHECK_ERROR(memcmp(&first_connection->link_protocol, &connection->link_protocol, sizeof(link_protocol_t)), err,
                    SWC_ERR_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD, return);
#if !WPS_DISABLE_FRAGMENTATION
        CHECK_ERROR(memcmp(&first_connection->frag, &connection->frag, sizeof(frag_t)), err,
                    SWC_ERR_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD, return);
#endif
        CHECK_ERROR(memcmp(&first_connection->frame_cfg, &connection->frame_cfg, sizeof(frame_cfg_t)), err,
                    SWC_ERR_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD, return);
    }

    wps_connection_t *first_main_connection = timeslot.connection_main[0];

    if (first_main_connection != NULL) {
        /* If credit control flow is enabled for the main connection, it must also be enabled for the auto-reply
         * connection.
         */
        CHECK_ERROR((first_main_connection->credit_flow_ctrl.enabled != first_connection->credit_flow_ctrl.enabled),
                    err, SWC_ERR_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD, return);
    }
}

/** @brief Do global auto connection settings verification
 *
 *  @param[in]  timeslot        Timeslots table
 *  @param[in]  timeslot_count  Number of the timeslots.
 *  @param[out] err             Wireless Core error code.
 */
static void check_global_auto_connection_errors(const timeslot_t *const timeslot, uint32_t timeslot_count,
                                                swc_error_t *err)
{
    *err = SWC_ERR_NONE;

    for (uint8_t i = 0; i < timeslot_count; i++) {
        wps_connection_t *first_main_connection = timeslot[i].connection_main[0];

        if (first_main_connection != NULL) {
            bool ack_frame_enabled = first_main_connection->ack_frame_enable;

            if (ack_frame_enabled == true) {
                /* If there is an auto-reply connection, the number of main connections should match the number of
                 * auto-reply connections.
                 */
                CHECK_ERROR((timeslot[i].main_connection_count != timeslot[i].auto_connection_count &&
                             timeslot[i].auto_connection_count > 0),
                            err, SWC_ERR_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD, return);
            }
        }
    }
}

/** @brief Validate parameters of connections sharing the same timeslot in schedule.
 *
 *  @note This will make sure that each connection sharing the same timeslot has
 *          - connection priority enable
 *          - a bunch of matching parameters.
 *
 *  @param[in]  node  A pointer to the node handle structure.
 *  @param[out] err   Wireless Core error code.
 */
static void validate_connection_priority_in_schedule(const swc_node_t *const node, swc_error_t *err)
{
    /* Loop over the scheduler to find connection priority issue */
    for (uint8_t i = 0; i < wps.mac.scheduler.schedule.size; i++) {
        /* Validate parameters for main connection */
        if (wps.mac.scheduler.schedule.timeslot[i].main_connection_count > 1) {
            /* Check every connection if they have connection priority enable */
            for (uint8_t j = 0; j < wps.mac.scheduler.schedule.timeslot[i].main_connection_count; j++) {
                wps_connection_t *current_conn = wps.mac.scheduler.schedule.timeslot[i].connection_main[j];
                uint8_t current_prio = current_conn->priority;
                bool priority_enable = false;
                bool connection_is_tx =
                    (is_rx_connection(node->cfg.local_address, current_conn->source_address) == true) ? false : true;

                /* Check if priority is enabled by checking header link protocol */
                for (uint8_t k = 0; k < current_conn->link_protocol.current_number_of_protocol; k++) {
                    if (current_conn->link_protocol.protocol_info[k].id == MAC_PROTO_ID_CONNECTION_ID) {
                        priority_enable = true;
                    }
                }
                /* Check if priority is enable on all connection that share same timeslot */
                CHECK_ERROR(priority_enable == false, err, SWC_ERR_PRIO_NOT_ENABLE_ON_ALL_CONN, return);
                /* Check if invalid priority */
                CHECK_ERROR((current_prio >= 3) && connection_is_tx, err,
                            SWC_ERR_NOT_ALLOWED_CONN_PRIORITY_CONFIGURATION, return);
            }

            check_main_connection_priority_errors(node, wps.mac.scheduler.schedule.timeslot[i], err);
            CHECK_ERROR(*err != SWC_ERR_NONE, err, *err, return);
        }
        /* Validate parameters for auto connection */
        if (wps.mac.scheduler.schedule.timeslot[i].auto_connection_count > 1) {
            /* Check every connection if they have connection priority enable */
            for (uint8_t j = 0; j < wps.mac.scheduler.schedule.timeslot[i].auto_connection_count; j++) {
                wps_connection_t *current_conn = wps.mac.scheduler.schedule.timeslot[i].connection_auto_reply[j];
                uint8_t current_prio = current_conn->priority;
                bool priority_enable = false;
                bool connection_is_tx =
                    (is_rx_connection(node->cfg.local_address, current_conn->source_address) == true) ? false : true;

                /* Check if priority is enabled by checking header link protocol */
                for (uint8_t k = 0; k < current_conn->link_protocol.current_number_of_protocol; k++) {
                    if (current_conn->link_protocol.protocol_info[k].id == MAC_PROTO_ID_CONNECTION_ID) {
                        priority_enable = true;
                    }
                }
                /* Check if priority is enable on all connection that share same timeslot */
                CHECK_ERROR(priority_enable == false, err, SWC_ERR_PRIO_NOT_ENABLE_ON_ALL_CONN, return);
                /* Check if invalid priority */
                CHECK_ERROR((current_prio >= 3) && connection_is_tx, err,
                            SWC_ERR_NOT_ALLOWED_CONN_PRIORITY_CONFIGURATION, return);
            }

            check_auto_connection_priority_errors(node, wps.mac.scheduler.schedule.timeslot[i], err);
            CHECK_ERROR(*err != SWC_ERR_NONE, err, *err, return);
        }
    }
}

/** @brief Get a formatted string of the radio's nvm content.
 *
 *  @param[in] wps_radio  Radio handle.
 *  @param[in] buffer     Buffer where to put the formatted string.
 *  @param[in] size       Size of the buffer.
 *  @return The formated string length, excluding the NULL terminator.
 */
static int format_radio_nvm(wps_radio_t *wps_radio, char *buffer, uint16_t size)
{
    static const char *const phy_version_strings[] = {"v3.0"};
    static const char *const phy_model_strings[] = {"SR1120"};
    int string_length;
    uint8_t id_model = wps_radio_get_product_id_model(wps_radio);
    uint8_t id_version = wps_radio_get_product_id_version(wps_radio);
    uint64_t radio_serial = wps_radio_get_serial_number(wps_radio);
    uint8_t *serial_ptr = (uint8_t *)&radio_serial;
    char tmp[32];

    if (radio_serial == 0) {
        /* Format the output string. */
        string_length = snprintf(buffer, size,
                                 "<<  RADIO NVM  >>\r\n"
                                 " Radio Serial: N/A\r\n"
                                 " Radio Model: N/A\r\n"
                                 " Radio Version: N/A\r\n");
    } else {
        /* Format the serial string. */
        snprintf(tmp, sizeof(tmp), "%c%c%02x%02x%02x%02x%02x%02x", serial_ptr[7], serial_ptr[6], serial_ptr[5],
                 serial_ptr[4], serial_ptr[3], serial_ptr[2], serial_ptr[1], serial_ptr[0]);
        /* Format the output string. */
        string_length = snprintf(buffer, size,
                                 "<<  RADIO NVM  >>\r\n"
                                 " Radio Serial: %s\r\n"
                                 " Radio Model: %s\r\n"
                                 " Radio Version: %s\r\n",
                                 tmp, phy_model_strings[id_model], phy_version_strings[id_version]);
    }

    return string_length;
}

/** @brief Validate the chip rate configuration.
 *
 *  @param[in] chip_rate  SWC chip rate configuration.
 *  @return True if chip_rate is valid.
 */
static bool validate_chip_rate(swc_chip_rate_t chip_rate)
{
    switch (chip_rate) {
    case SWC_CHIP_RATE_20_48_MHZ:
    case SWC_CHIP_RATE_27_30_MHZ:
    case SWC_CHIP_RATE_40_96_MHZ:
        return true;
    default:
        return false;
    }
}

/** @brief Initializes a radio structure with default configuration settings.
 *
 * This function sets the configuration of a given radio structure to predefined
 * default values. It configures various operational parameters including IRQ polarity,
 * SPI mode, output impedance, clock sources, and chip rate.
 *
 * @param[in] radio     A pointer to the radio structure to be initialized.
 * @param[in] radio_id  Radio structure index
 *
 */
static void initialize_radio_with_defaults(radio_t *radio, uint8_t radio_id)
{
    radio->irq_polarity = WPS_DEFAULT_RADIO_IRQ;
    radio->std_spi = WPS_DEFAULT_RADIO_SPI_MODE;
    radio->outimped = WPS_DEFAULT_RADIO_OUTIMPED;
    radio->clock_source.pll_clk_source = CHIP_CLK_INTERNAL_OUTPUT_HIGH_IMPED;
    radio->clock_source.xtal_clk_source = XTAL_CLK_INTERNAL_OUTPUT_HIGH_IMPED;
    radio->chip_rate = wps.chip_rate;
    radio->sumrxadc = WPS_DEFAULT_SUMRXADC;
    radio->radio_id = radio_id;
}

/** @brief Allocate payload and header buffer memory based off connection configuration.
 *
 *  @note Since the header size can be changed after the connection has been initialized through
 *        the swc_connection_init method, the memory need to be allocated during swc_setup.
 *
 *  @param[in]  node  A pointer to the node handle structure.
 *  @param[out] err   Wireless Core error code.
 */
static void allocate_payload_and_header_buffer_memory(const swc_node_t *const node, swc_error_t *err)
{
    bool is_rx_conn = false;
    /* Loop over the scheduler to find connection priority issue */
    for (uint8_t i = 0; i < wps.mac.scheduler.schedule.size; i++) {
        for (uint8_t j = 0; j < wps.mac.scheduler.schedule.timeslot[i].main_connection_count; j++) {
            wps_connection_t *current_conn = wps.mac.scheduler.schedule.timeslot[i].connection_main[j];

            is_rx_conn = is_rx_connection(node->cfg.local_address, current_conn->source_address);
            /* Allocate memory for TX main or auto-reply connection */
            if (is_rx_conn == false && current_conn->tx_data == NULL) {
                current_conn->tx_data = mem_pool_malloc(&mem_pool, sizeof(xlayer_circular_data_t));
                CHECK_ERROR(current_conn->tx_data == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return);

                uint16_t conn_buffer_size =
                    xlayer_circular_data_get_tx_required_bytes(current_conn->xlayer_queue.max_size,
                                                               current_conn->header_size, current_conn->payload_size);

                current_conn->tx_data->buffer = mem_pool_malloc(&mem_pool, conn_buffer_size);
                CHECK_ERROR(current_conn->tx_data->buffer == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return);

                xlayer_circular_data_init(current_conn->tx_data, current_conn->tx_data->buffer, conn_buffer_size);
            } else if (is_rx_conn == true && current_conn->rx_data == NULL) {
                current_conn->rx_data = mem_pool_malloc(&mem_pool, sizeof(xlayer_circular_data_t));
                CHECK_ERROR(current_conn->rx_data == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return);

                uint16_t conn_buffer_size =
                    xlayer_circular_data_get_rx_required_bytes(current_conn->xlayer_queue.max_size,
                                                               current_conn->payload_size);

                current_conn->rx_data->buffer = mem_pool_malloc(&mem_pool, conn_buffer_size);
                CHECK_ERROR(current_conn->rx_data->buffer == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return);

                xlayer_circular_data_init(current_conn->rx_data, current_conn->rx_data->buffer, conn_buffer_size);
            }
        }
        for (uint8_t j = 0; j < wps.mac.scheduler.schedule.timeslot[i].auto_connection_count; j++) {
            wps_connection_t *current_conn = wps.mac.scheduler.schedule.timeslot[i].connection_auto_reply[j];

            is_rx_conn = is_rx_connection(node->cfg.local_address, current_conn->source_address);
            /* Allocate memory for TX main or auto-reply connection */
            if (is_rx_conn == false && current_conn->tx_data == NULL) {
                current_conn->tx_data = mem_pool_malloc(&mem_pool, sizeof(xlayer_circular_data_t));
                CHECK_ERROR(current_conn->tx_data == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return);

                uint16_t conn_buffer_size =
                    xlayer_circular_data_get_tx_required_bytes(current_conn->xlayer_queue.max_size,
                                                               current_conn->header_size, current_conn->payload_size);

                current_conn->tx_data->buffer = mem_pool_malloc(&mem_pool, conn_buffer_size);
                CHECK_ERROR(current_conn->tx_data->buffer == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return);

                xlayer_circular_data_init(current_conn->tx_data, current_conn->tx_data->buffer, conn_buffer_size);
            } else if (is_rx_conn == true && current_conn->rx_data == NULL) {
                current_conn->rx_data = mem_pool_malloc(&mem_pool, sizeof(xlayer_circular_data_t));
                CHECK_ERROR(current_conn->rx_data == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return);

                uint16_t conn_buffer_size =
                    xlayer_circular_data_get_rx_required_bytes(current_conn->xlayer_queue.max_size,
                                                               current_conn->payload_size);

                current_conn->rx_data->buffer = mem_pool_malloc(&mem_pool, conn_buffer_size);
                CHECK_ERROR(current_conn->rx_data->buffer == NULL, err, SWC_ERR_NOT_ENOUGH_MEMORY, return);

                xlayer_circular_data_init(current_conn->rx_data, current_conn->rx_data->buffer, conn_buffer_size);
            }
        }
    }
}

/** @brief Calculate the number of activated callbacks on all connections.
 *
 *  @param[in]  node  A pointer to the node handle structure.
 */
static uint32_t calculate_activated_callback_count(const swc_node_t *const node)
{
    uint32_t activated_callback_count = 0;
    wps_connection_list_node_t *connection_list = wps_connection_list_get_head(&node->wps_node_handle->conn_list);

    while (connection_list != NULL) {
        wps_connection_t *connection = (wps_connection_t *)connection_list->connection;

        if (connection->tx_success_callback || connection->tx_fail_callback) {
            activated_callback_count += xlayer_queue_get_max_size(&connection->xlayer_queue);
        }

        if (connection->rx_success_callback) {
            activated_callback_count += xlayer_queue_get_max_size(&connection->xlayer_queue);
        }

        if (connection->tx_drop_callback) {
            activated_callback_count += xlayer_queue_get_max_size(&connection->xlayer_queue);
        }

        if (connection->ranging_data_ready_callback) {
            activated_callback_count += xlayer_queue_get_max_size(&connection->xlayer_queue);
        }

        if (connection->evt_callback) {
            activated_callback_count++;
        }

        connection_list = wps_connection_list_get_next(connection_list);
    }

    return activated_callback_count;
}

/** @brief Validate connection channel presence
 *
 *  @param[in]  conn  A pointer to the connection.
 *  @param[out] arg   A pointer to the swc error.
 */
static void validate_channels(wps_connection_list_node_t *conn, void *arg)
{
    rf_channel_t empty_channel = {0};
    swc_error_t *err = (swc_error_t *)arg;

    wps_connection_t *connection = (wps_connection_t *)conn->connection;

    if (connection->is_main) {
        CHECK_ERROR(memcmp(&connection->channel[0][0], &empty_channel, sizeof(rf_channel_t)) == 0, err,
                    SWC_ERR_NO_CHANNEL_INIT, return);
    }
}
