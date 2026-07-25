/** @file link_tdma_sync.c
 *  @brief TDMA sync module.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is confidential and proprietary.
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "link_tdma_sync.h"
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "link_utils.h"
#include "sr1100_def.h"
#include "wps_config.h"

/* CONSTANTS ******************************************************************/
#define RANDOM_OFFSET_COUNT         17
#define DEEP_TO_SHALLOW_XTAL_CYCLES 97
#define S_TO_US_DIVIDER             1000000
/* Offset applied to the SLPPERIOD register to calculate the sleep period. */
#define SLPPERIOD_OFFSET 1
/* Base PHY rate used for TDMA SYNC calculations. */
#define TDMA_BASE_PHY_RATE CHIP_RATE_40_96_MHZ
/* Base PHY rate used for CCA parameters. */
#define CCA_BASE_PHY_RATE CHIP_RATE_20_48_MHZ
/* Base PHY rate used for CCA parameters. */
#define SCHEDULER_BASE_PHY_RATE CHIP_RATE_20_48_MHZ
/* Value at which the xtal is reset relative to the xtal falling edge. */
#define XTAL_RESET_PLL_CYCLE ((uint32_t)(XTAL_TO_PLL_RATIO(TDMA_BASE_PHY_RATE) / 2))
/* Metastability window bounds in PLL cycles at TDMA_BASE_PHY_RATE */
#define METASTABILITY_WINDOW_WIDTH_PERCENTAGE 4
#define METASTABILITY_WINDOW_WIDTH_PLL_CYCLE \
    ((uint32_t)((XTAL_TO_PLL_RATIO(TDMA_BASE_PHY_RATE)) * (METASTABILITY_WINDOW_WIDTH_PERCENTAGE) / 100))
#define METASTABILITY_WINDOW_LOWER_BOUND (XTAL_RESET_PLL_CYCLE - (METASTABILITY_WINDOW_WIDTH_PLL_CYCLE / 2))
#define METASTABILITY_WINDOW_UPPER_BOUND (XTAL_RESET_PLL_CYCLE + (METASTABILITY_WINDOW_WIDTH_PLL_CYCLE / 2))

/* PRIVATE GLOBALS ************************************************************/
static const int8_t rand_offset_table[RANDOM_OFFSET_COUNT] = {
    -32, -28, -24, -20, -16, -12, -8, -4, 0, 4, 8, 12, 16, 20, 24, 28, 32,
};

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static inline void sync_update(tdma_sync_t *tdma_sync, uint32_t duration_pll_cycles, sleep_lvl_t sleep_mode);
static inline void slave_adjust_frame_rx(tdma_sync_t *tdma_sync, uint16_t rx_waited_pll_cycles, link_cca_t *cca,
                                         uint8_t rx_cca_retry_count);
static inline void slave_adjust_frame_lost(tdma_sync_t *tdma_sync);
static inline int32_t slave_calculate_offset(uint16_t target_rx_waited, uint16_t rx_waited);

/* PUBLIC FUNCTIONS ***********************************************************/
void link_tdma_sync_init(tdma_sync_t *tdma_sync, sleep_lvl_t sleep_mode, uint16_t setup_time_us,
                         uint32_t frame_lost_max_duration, uint8_t sfd_size_bits, uint16_t preamble_size_bits,
                         uint8_t pll_startup_xtal_cycles, isi_mitig_t isi_mitig, uint8_t isi_mitig_pauses,
                         uint16_t seed, bool fast_sync_enable, bool tx_jitter_enabled, chip_rate_cfg_t chip_rate)
{
    (void)pll_startup_xtal_cycles;

    uint16_t setup_time_pll_cycles = setup_time_us * PLL_FREQ_HZ(TDMA_BASE_PHY_RATE) / S_TO_US_DIVIDER;

    memset(tdma_sync, 0, sizeof(tdma_sync_t));
#if WPS_RADIO_COUNT == 1
    tdma_sync->sleep_mode = sleep_mode;
#else
    (void)sleep_mode;
    tdma_sync->sleep_mode = SLEEP_IDLE;
#endif
    tdma_sync->setup_time_pll_cycles = setup_time_pll_cycles;
    tdma_sync->frame_lost_max_duration = frame_lost_max_duration;
    tdma_sync->slave_sync_state = STATE_SYNCING;
    tdma_sync->sync_slave_offset = 0;
    tdma_sync->fast_sync_enable = fast_sync_enable;
    tdma_sync->isi_mitig = isi_mitig;
    tdma_sync->isi_mitig_pauses = isi_mitig_pauses;
    tdma_sync->tx_jitter_enabled = tx_jitter_enabled;
    tdma_sync->preamble_size_bits = preamble_size_bits;
    tdma_sync->sfd_size_bits = sfd_size_bits;
    tdma_sync->chip_rate = chip_rate;

    srand(seed + 2); /* Avoid seed value 1 which reset the seed by adding 2*/
}

void link_tdma_sync_update_tx(tdma_sync_t *tdma_sync, uint32_t duration_pll_cycles, link_cca_t *cca,
                              sleep_lvl_t sleep_mode, chip_rate_cfg_t chip_rate)
{
    uint8_t rand_num = 0;
    int8_t random_offset = 0;

    tdma_sync->chip_rate = chip_rate;

    if (tdma_sync->tx_jitter_enabled) {
        rand_num = rand() % RANDOM_OFFSET_COUNT;
        random_offset = rand_offset_table[rand_num];
    }

    duration_pll_cycles += random_offset;

    /* Convert scheduler duration to TDMA calculation PHY rate. */
    duration_pll_cycles = convert_pll_cycle_base(duration_pll_cycles, SCHEDULER_BASE_PHY_RATE, TDMA_BASE_PHY_RATE);

    if (tdma_sync->previous_frame_type == FRAME_RX) {
        duration_pll_cycles += tdma_sync->setup_time_pll_cycles;
        duration_pll_cycles -= convert_pll_cycle_base(cca->on_time_pll_cycles, CCA_BASE_PHY_RATE, TDMA_BASE_PHY_RATE);
    }
    tdma_sync->previous_frame_type = FRAME_TX;

    sync_update(tdma_sync, duration_pll_cycles, sleep_mode);

    /* In TX, force timeout value for auto-reply to be default one. */
    tdma_sync->timeout_value = link_tdma_get_rx_timeout_base_value(tdma_sync, chip_rate);
}

void link_tdma_sync_update_rx(tdma_sync_t *tdma_sync, uint32_t duration_pll_cycles, link_cca_t *cca,
                              sleep_lvl_t sleep_mode, chip_rate_cfg_t chip_rate)
{
    uint32_t cca_timeout_offset_value = 0;

    tdma_sync->chip_rate = chip_rate;

    /* Convert scheduler duration to TDMA calculation PHY rate. */
    duration_pll_cycles = convert_pll_cycle_base(duration_pll_cycles, SCHEDULER_BASE_PHY_RATE, TDMA_BASE_PHY_RATE);

    if (tdma_sync->previous_frame_type == FRAME_TX) {
        duration_pll_cycles -= tdma_sync->setup_time_pll_cycles;
        duration_pll_cycles += convert_pll_cycle_base(cca->on_time_pll_cycles, CCA_BASE_PHY_RATE, TDMA_BASE_PHY_RATE);
    }
    tdma_sync->previous_frame_type = FRAME_RX;

    /* Calculate rx timeout offset for CCA. */
    cca_timeout_offset_value = convert_pll_cycle_base(cca->rx_timeout_offset_pll_cycles, CCA_BASE_PHY_RATE, chip_rate);

    sync_update(tdma_sync, duration_pll_cycles, sleep_mode);

    /* In RX, timeout value needs to consider CCA window and pwr_up delay for auto-reply to be default one. */
    tdma_sync->timeout_value = link_tdma_get_rx_timeout_base_value(tdma_sync, chip_rate) + cca_timeout_offset_value +
                               tdma_sync->pwr_up_value;
}

void link_tdma_sync_slave_adjust(tdma_sync_t *tdma_sync, frame_outcome_t frame_outcome, uint16_t rx_waited_pll_cycles,
                                 link_cca_t *cca, uint8_t rx_cca_retry_count)
{
    if (frame_outcome == FRAME_RECEIVED) {
        slave_adjust_frame_rx(tdma_sync, rx_waited_pll_cycles, cca, rx_cca_retry_count);
    } else {
        slave_adjust_frame_lost(tdma_sync);
    }
}

void link_tdma_sync_slave_find(tdma_sync_t *tdma_sync, frame_outcome_t frame_outcome, uint16_t rx_waited_pll_cycles,
                               link_cca_t *cca, uint8_t rx_cca_retry_count)
{
    if (frame_outcome == FRAME_RECEIVED) {
        slave_adjust_frame_rx(tdma_sync, rx_waited_pll_cycles, cca, rx_cca_retry_count);
    } else {
        tdma_sync->sync_slave_offset = UNSYNC_OFFSET_PLL_CYCLES;
        /* Save the chip rate at which the sync_slave_offset was calculated. */
        tdma_sync->sync_slave_offset_chip_rate = TDMA_BASE_PHY_RATE;
    }
}

uint8_t link_tdma_sync_get_isi_mitigation_pauses(isi_mitig_t isi_mitig_reg_val)
{
    switch (isi_mitig_reg_val) {
    case ISI_MITIG_0:
        return 0;
    case ISI_MITIG_1:
        return 1;
    case ISI_MITIG_2:
        return 2;
    case ISI_MITIG_3:
        return 2;
    default:
        return 0;
    }
}

uint16_t link_tdma_get_preamble_reg_value(uint8_t isi_mitig_pauses, uint32_t preamble_len, sfd_length_t sfd_len_reg_val)
{
    uint16_t chip_multiplier = 0;
    uint16_t chips_per_symbol = 0;
    uint16_t symbols_count = 0;

    chips_per_symbol = isi_mitig_pauses + 2;

    switch (sfd_len_reg_val) {
    case SFD_LENGTH_64_1BIT_PPM:
        chip_multiplier = 2;
        break;
    case SFD_LENGTH_32_OOK:
    case SFD_LENGTH_16_1BIT_PPM:
    case SFD_LENGTH_32_1BIT_PPM:
        chip_multiplier = 1;
        break;
    default:
        chip_multiplier = 1;
        break;
    }

    symbols_count = preamble_len / chips_per_symbol;

    return ((symbols_count - 1) - (48 / chips_per_symbol)) / MUL_4(chip_multiplier);
}

uint32_t link_tdma_get_preamble_length(uint8_t isi_mitig_pauses, uint32_t preamble_len_reg_val,
                                       sfd_length_t sfd_len_reg_val)
{
    uint16_t chip_multiplier = 0;
    uint16_t chips_per_symbol = 0;
    uint16_t symbols_count = 0;

    chips_per_symbol = isi_mitig_pauses + 2;

    switch (sfd_len_reg_val) {
    case SFD_LENGTH_64_1BIT_PPM:
        chip_multiplier = 2;
        break;
    case SFD_LENGTH_32_OOK:
    case SFD_LENGTH_16_1BIT_PPM:
    case SFD_LENGTH_32_1BIT_PPM:
        chip_multiplier = 1;
        break;
    default:
        chip_multiplier = 1;
        break;
    }

    symbols_count = (MUL_4(preamble_len_reg_val) * chip_multiplier) + (48 / chips_per_symbol) + 1;

    return symbols_count * chips_per_symbol;
}

uint32_t link_tdma_get_sfd_length(uint8_t isi_mitig_pauses, sfd_length_t sfd_len_reg_val)

{
    uint16_t chip_multiplier = 0;
    uint16_t symbol_count = 0;

    switch (sfd_len_reg_val) {
    case SFD_LENGTH_16_1BIT_PPM:
        symbol_count = 16;
        chip_multiplier = 2;
        break;
    case SFD_LENGTH_32_OOK:
        symbol_count = 32;
        chip_multiplier = 1;
        break;
    case SFD_LENGTH_32_1BIT_PPM:
        symbol_count = 32;
        chip_multiplier = 2;
        break;
    case SFD_LENGTH_64_1BIT_PPM:
        symbol_count = 64;
        chip_multiplier = 2;
        break;
    default:
        symbol_count = 32;
        chip_multiplier = 2;
        break;
    }
    return (symbol_count * chip_multiplier) + (symbol_count * isi_mitig_pauses);
}

void link_tdma_set_sfd_length(tdma_sync_t *tdma_sync, uint8_t isi_mitig_pauses, sfd_length_t sfd_len_reg_val)
{
    uint16_t chip_multiplier = 0;
    uint16_t symbol_count = 0;

    switch (sfd_len_reg_val) {
    case SFD_LENGTH_16_1BIT_PPM:
        symbol_count = 16;
        chip_multiplier = 2;
        break;
    case SFD_LENGTH_32_OOK:
        symbol_count = 32;
        chip_multiplier = 1;
        break;
    case SFD_LENGTH_32_1BIT_PPM:
        symbol_count = 32;
        chip_multiplier = 2;
        break;
    case SFD_LENGTH_64_1BIT_PPM:
        symbol_count = 64;
        chip_multiplier = 2;
        break;
    default:
        symbol_count = 32;
        chip_multiplier = 2;
        break;
    }

    tdma_sync->sfd_size_bits = (symbol_count * chip_multiplier) + (symbol_count * isi_mitig_pauses);
}

void link_tdma_set_preamble_length(tdma_sync_t *tdma_sync, uint8_t isi_mitig_pauses, uint32_t preamble_len_reg_val,
                                   sfd_length_t sfd_len_reg_val)
{
    uint16_t chip_multiplier = 0;
    uint16_t chips_per_symbol = 0;
    uint16_t symbols_count = 0;

    chips_per_symbol = isi_mitig_pauses + 2;

    switch (sfd_len_reg_val) {
    case SFD_LENGTH_64_1BIT_PPM:
        chip_multiplier = 2;
        break;
    case SFD_LENGTH_32_OOK:
    case SFD_LENGTH_16_1BIT_PPM:
    case SFD_LENGTH_32_1BIT_PPM:
        chip_multiplier = 1;
        break;
    default:
        chip_multiplier = 1;
        break;
    }
    symbols_count = (MUL_4(preamble_len_reg_val) * chip_multiplier) + (48 / chips_per_symbol) + 1;

    tdma_sync->preamble_size_bits = symbols_count * chips_per_symbol;
}

uint64_t link_tdma_get_time_stamp_pll_cycles(tdma_sync_t *tdma_sync)
{
    return tdma_sync->free_running_timer;
}

uint16_t link_tdma_get_elapsed_time_us(tdma_sync_t *tdma_sync, uint64_t previous_time_stamp)
{
    return ((uint64_t)(tdma_sync->free_running_timer - previous_time_stamp) * 1000) / PLL_FREQ_KHZ(TDMA_BASE_PHY_RATE);
}

uint16_t link_tdma_get_last_timer_increment_us(tdma_sync_t *tdma_sync)
{
    return ((uint64_t)tdma_sync->last_timer_increment * 1000) / PLL_FREQ_KHZ(TDMA_BASE_PHY_RATE);
}

void link_tdma_update_isi_mitig_pauses(tdma_sync_t *tdma_sync, isi_mitig_t isi_mitig)
{
    uint8_t prev_isi_mitig_pauses = tdma_sync->isi_mitig_pauses;
    uint8_t prev_chips_per_symbol = prev_isi_mitig_pauses + 2;
    uint8_t new_chips_per_symbol;

    tdma_sync->isi_mitig = isi_mitig;
    tdma_sync->isi_mitig_pauses = link_tdma_sync_get_isi_mitigation_pauses(isi_mitig);
    new_chips_per_symbol = tdma_sync->isi_mitig_pauses + 2;
    tdma_sync->preamble_size_bits =
        (((tdma_sync->preamble_size_bits / prev_chips_per_symbol) - (48 / prev_chips_per_symbol)) +
         (48 / new_chips_per_symbol)) *
        new_chips_per_symbol;
    tdma_sync->sfd_size_bits = tdma_sync->sfd_size_bits * (tdma_sync->isi_mitig_pauses + 1) /
                               (prev_isi_mitig_pauses + 1);
}

uint32_t link_tdma_get_rx_timeout_base_value(const tdma_sync_t *tdma_sync, chip_rate_cfg_t chip_rate)
{
    return convert_pll_cycle_base(MUL_2(tdma_sync->setup_time_pll_cycles), TDMA_BASE_PHY_RATE, chip_rate) +
           tdma_sync->preamble_size_bits + tdma_sync->sfd_size_bits;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Update TDMA sync module.
 *
 *  @param[in] tdma_sync            TDMA sync object.
 *  @param[in] duration_pll_cycles  Duration in PLL clock cycles.
 *  @param[in] sleep_mode           Sleep mode.
 *  @return None.
 */
static inline void sync_update(tdma_sync_t *tdma_sync, uint32_t duration_pll_cycles, sleep_lvl_t sleep_mode)
{
    if (tdma_sync->sync_slave_offset != 0) {
        /* Apply slave offset measured in previous RX frame to the power up delay. */
        duration_pll_cycles += convert_pll_cycle_base(tdma_sync->sync_slave_offset,
                                                      tdma_sync->sync_slave_offset_chip_rate, TDMA_BASE_PHY_RATE);
        /* Reset slave offset after applying it to power up delay. */
        tdma_sync->sync_slave_offset = 0;
    }

    if (sleep_mode == SLEEP_IDLE) {
        /* Increase the sleep duration by previous pwr up delay since timer resets before pwr_up_delay */
        duration_pll_cycles += tdma_sync->pwr_up_delay_pll_cycles;
    }

    /* When changing from non-idle to idle, reset XTAL offset. */
    if (tdma_sync->sleep_mode != SLEEP_IDLE && sleep_mode == SLEEP_IDLE) {
        tdma_sync->xtal_cycle_offset = 0;
    }

    /* When changing from non-deep to deep, reduce the duration by the time the radio needs to wakeup
     * from shallow to deep
     */
    if (tdma_sync->sleep_mode != SLEEP_DEEP && sleep_mode == SLEEP_DEEP) {
        duration_pll_cycles -= mul_xtal_to_pll_ratio(DEEP_TO_SHALLOW_XTAL_CYCLES, TDMA_BASE_PHY_RATE);
    }

    /* When changing from deep to shallow, increase the duration by the time the radio needs to wakeup
     * from shallow to deep
     */
    if (tdma_sync->sleep_mode == SLEEP_DEEP && sleep_mode == SLEEP_SHALLOW) {
        duration_pll_cycles += mul_xtal_to_pll_ratio(DEEP_TO_SHALLOW_XTAL_CYCLES, TDMA_BASE_PHY_RATE);
    }

    /* When changing from idle to non-idle, adjust duration based on XTAL clock offset */
    if (tdma_sync->sleep_mode == SLEEP_IDLE && sleep_mode != SLEEP_IDLE) {
        /* Apply offset */
        duration_pll_cycles -= (XTAL_TO_PLL_RATIO(TDMA_BASE_PHY_RATE) - tdma_sync->xtal_cycle_offset);
        /* Adjust depending on what side of XTAL clock rising edge the offset is */
        if (tdma_sync->xtal_cycle_offset >= XTAL_RESET_PLL_CYCLE) {
            duration_pll_cycles -= XTAL_TO_PLL_RATIO(TDMA_BASE_PHY_RATE);
        }
    }

#if WPS_RADIO_COUNT == 1
    tdma_sync->sleep_mode = sleep_mode;
#else
    (void)sleep_mode;
    tdma_sync->sleep_mode = SLEEP_IDLE;
#endif

    switch (tdma_sync->sleep_mode) {
    case SLEEP_SHALLOW:
    case SLEEP_DEEP:
        /* When in SHALLOW or DEEP sleep, the currently active system clock is the 32.768 kHz xtal.
         *
         * We can see that there is no need to keep track of the xtal_cycle_offset because we are waking up with no
         * offset.
         *
         * However, we must configure the power up delay since the xtal clock does not have as much precision compared
         * to the PLL.
         */
        tdma_sync->sleep_cycles_value = div_pll_ratio(duration_pll_cycles + tdma_sync->pwr_up_delay_pll_cycles,
                                                      TDMA_BASE_PHY_RATE) -
                                        SLPPERIOD_OFFSET;
        tdma_sync->pwr_up_delay_pll_cycles = mod_pll_ratio(duration_pll_cycles + tdma_sync->pwr_up_delay_pll_cycles,
                                                           TDMA_BASE_PHY_RATE);
        break;
    case SLEEP_IDLE:
    default:
        /*
         * When in IDLE sleep, the currently active system clock is the PLL of the current PHY rate (20.48 MHz, 27.30
         * MHz or 40.96 MHz).
         *
         * In this mode, it is important to keep track of the xtal_cycle_offset to prevent the metastability for the
         * XTAL_COUNT and that we may have a proper reset behavior.
         */
        tdma_sync->sleep_cycles_value =
            convert_pll_cycle_base(duration_pll_cycles, TDMA_BASE_PHY_RATE, tdma_sync->chip_rate) - SLPPERIOD_OFFSET;
        tdma_sync->pwr_up_delay_pll_cycles = 0;
        /* Compute XTAL offset */
        tdma_sync->xtal_cycle_offset += duration_pll_cycles;
        tdma_sync->xtal_cycle_offset = mod_pll_ratio(tdma_sync->xtal_cycle_offset, TDMA_BASE_PHY_RATE);
        /* Avoid having an offset exactly on XTAL clock rising edge where timer reset moment can vary slightly  */
        if (tdma_sync->xtal_cycle_offset >= METASTABILITY_WINDOW_LOWER_BOUND &&
            tdma_sync->xtal_cycle_offset <= METASTABILITY_WINDOW_UPPER_BOUND) {
            /* The metastability might occur when a configured wake up would occur near the increment of the
             * xtal wakeup timer. If those two are too close, undefined behavior might occur. In that case, we
             * wake up the radio earlier and start the transceiver a bit later with the power up delay.
             */
            uint32_t two_window_width = MUL_2(METASTABILITY_WINDOW_WIDTH_PLL_CYCLE);

            tdma_sync->pwr_up_delay_pll_cycles = two_window_width;
            tdma_sync->sleep_cycles_value -= convert_pll_cycle_base(two_window_width, TDMA_BASE_PHY_RATE,
                                                                    tdma_sync->chip_rate);
            tdma_sync->xtal_cycle_offset -= two_window_width;
        }
        break;
    }
    /* Update time tracking. */
    tdma_sync->ts_duration_pll_cycles += duration_pll_cycles;
    tdma_sync->free_running_timer += duration_pll_cycles;
    tdma_sync->last_timer_increment = duration_pll_cycles;
    /* Convert power up delay to current PHY rate. */
    tdma_sync->pwr_up_value = convert_pll_cycle_base(tdma_sync->pwr_up_delay_pll_cycles, TDMA_BASE_PHY_RATE,
                                                     tdma_sync->chip_rate);
}

/** @brief Update Adjust slave sync when frame is received.
 *
 *  @param[in] tdma_sync            TDMA sync object.
 *  @param[in] rx_waited_pll_cycles RX waited value in PLL clock cycles.
 *  @param[in] cca                  CCA object.
 *  @param[in] rx_cca_retry_count   RX CCA retry count.
 *  @return None.
 */
static inline void slave_adjust_frame_rx(tdma_sync_t *tdma_sync, uint16_t rx_waited_pll_cycles, link_cca_t *cca,
                                         uint8_t rx_cca_retry_count)
{
    uint16_t target_rx_waited_pll_cycles = convert_pll_cycle_base(tdma_sync->setup_time_pll_cycles, TDMA_BASE_PHY_RATE,
                                                                  tdma_sync->chip_rate) +
                                           tdma_sync->preamble_size_bits + tdma_sync->sfd_size_bits;
    uint16_t cca_rx_waited_pll_cycles = convert_pll_cycle_base((cca->retry_time_pll_cycles + cca->on_time_pll_cycles) *
                                                                   rx_cca_retry_count,
                                                               CCA_BASE_PHY_RATE, tdma_sync->chip_rate);

    if (tdma_sync->fast_sync_enable && (tdma_sync->slave_sync_state == STATE_SYNCING)) {
        tdma_sync->pwr_up_value = 0;
    }

    rx_waited_pll_cycles -= tdma_sync->pwr_up_value;

    tdma_sync->frame_lost_duration = 0;
    tdma_sync->ts_duration_pll_cycles = 0;
    /* Calculate the expected waited time based on the setup_time + preamble + SFD and number of CCA retry used. */
    target_rx_waited_pll_cycles = (target_rx_waited_pll_cycles + cca_rx_waited_pll_cycles);

    if (rx_cca_retry_count == link_cca_get_highest_try_count(cca) && cca->fail_action == CCA_FAIL_ACTION_TX) {
        /**
         * In TX anyway, the radio will apply a retry_time on the last check starting from the beginning of the previous
         * CCA ON period. We thus need to remove the cca_on_time from the target_rx_waited_pll_cycles.
         */
        target_rx_waited_pll_cycles -= convert_pll_cycle_base(cca->on_time_pll_cycles, CCA_BASE_PHY_RATE,
                                                              tdma_sync->chip_rate);
    }

    if (tdma_sync->fast_sync_enable && (tdma_sync->slave_sync_state == STATE_SYNCING)) {
        tdma_sync->sync_slave_offset = -target_rx_waited_pll_cycles;
    } else {
        tdma_sync->sync_slave_offset = slave_calculate_offset(target_rx_waited_pll_cycles, rx_waited_pll_cycles);
    }
    /* Save the chip rate at which the sync_slave_offset was calculated. */
    tdma_sync->sync_slave_offset_chip_rate = tdma_sync->chip_rate;

    tdma_sync->slave_sync_state = STATE_SYNCED;
}

/** @brief Update Adjust slave sync when frame is lost.
 *
 *  @param[in] tdma_sync  TDMA sync object.
 *  @return None.
 */
static inline void slave_adjust_frame_lost(tdma_sync_t *tdma_sync)
{
    tdma_sync->frame_lost_duration += tdma_sync->ts_duration_pll_cycles;
    tdma_sync->ts_duration_pll_cycles = 0;
    tdma_sync->sync_slave_offset = 0;
    if (tdma_sync->frame_lost_duration >= tdma_sync->frame_lost_max_duration) {
        tdma_sync->slave_sync_state = STATE_SYNCING;
        tdma_sync->frame_lost_duration = tdma_sync->frame_lost_max_duration;
    }
}

/** @brief Calculate slave offset in normal condition.
 *
 *  @param[in] target_rx_waited  Target RX waited value.
 *  @param[in] rx_waited         RX waited value.
 *  @return Offset.
 */
static inline int32_t slave_calculate_offset(uint16_t target_rx_waited, uint16_t rx_waited)
{
    int32_t offset = 0;
    /* Slave woke up too early */
    if (rx_waited > target_rx_waited) {
        offset = (rx_waited - target_rx_waited);
        /* Slave woke up too late */
    } else if (rx_waited < target_rx_waited) {
        offset = -(target_rx_waited - rx_waited);
        /* Slave woke up in time */
    } else {
        offset = 0;
    }

    return offset;
}
