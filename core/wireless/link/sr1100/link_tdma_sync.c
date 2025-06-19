/** @file link_tdma_sync.c
 *  @brief TDMA sync module.
 *
 *  @copyright Copyright (C) 2020 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is confidential and proprietary.
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "link_tdma_sync.h"
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "link_utils.h"
#include "wps_config.h"

/* CONSTANTS ******************************************************************/
#define RANDOM_OFFSET_COUNT              17
#define DEEP_TO_SHALLOW_XTAL_CYCLES      97
#define METASTABILITY_WINDOW_LOWER_BOUND 300
#define METASTABILITY_WINDOW_UPPER_BOUND 324
#define METASTABILITY_WINDOW_WIDTH       24
#define S_TO_US_DIVIDER                  1000000

/* PRIVATE GLOBALS ************************************************************/
static const int8_t rand_offset_table[RANDOM_OFFSET_COUNT] = {
    -32, -28, -24, -20, -16, -12, -8, -4, 0, 4, 8, 12, 16, 20, 24, 28, 32,
};

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static inline void sync_update(tdma_sync_t *tdma_sync, uint32_t duration_pll_cycles, link_cca_t *cca);
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

    float chip_rate_ratio = PLL_RATIO(chip_rate) / (float)PLL_RATIO(CHIP_RATE_20_48_MHZ);
    uint16_t setup_time_pll_cycles = setup_time_us * PLL_FREQ_HZ(chip_rate) * chip_rate_ratio / S_TO_US_DIVIDER;

    memset(tdma_sync, 0, sizeof(tdma_sync_t));
#if WPS_RADIO_COUNT == 1
    tdma_sync->sleep_mode = sleep_mode;
#else
    (void)sleep_mode;
    tdma_sync->sleep_mode = SLEEP_IDLE;
#endif
    tdma_sync->timeout_pll_cycles = 2 * setup_time_pll_cycles + preamble_size_bits + sfd_size_bits;
    tdma_sync->setup_time_pll_cycles = setup_time_pll_cycles;
    tdma_sync->base_target_rx_waited_pll_cycles = setup_time_pll_cycles + preamble_size_bits + sfd_size_bits;
    tdma_sync->frame_lost_max_duration = frame_lost_max_duration;
    tdma_sync->slave_sync_state = STATE_SYNCING;
    tdma_sync->sync_slave_offset = 0;
    tdma_sync->fast_sync_enable = fast_sync_enable;
    tdma_sync->isi_mitig = isi_mitig;
    tdma_sync->isi_mitig_pauses = isi_mitig_pauses;
    tdma_sync->tx_jitter_enabled = tx_jitter_enabled;
    tdma_sync->pll_ratio = PLL_RATIO(chip_rate);
    tdma_sync->preamble_size_bits = preamble_size_bits;
    tdma_sync->sfd_size_bits = sfd_size_bits;
    tdma_sync->chip_rate_ratio = 1;
    tdma_sync->chip_rate = CHIP_RATE_20_48_MHZ;

    srand(seed + 2); /* Avoid seed value 1 which reset the seed by adding 2*/
}

void link_tdma_sync_update_tx(tdma_sync_t *tdma_sync, uint32_t duration_pll_cycles, link_cca_t *cca,
                              sleep_lvl_t sleep_mode, chip_rate_cfg_t chip_rate)
{
    uint8_t rand_num = 0;
    int8_t random_offset = 0;
    chip_rate_cfg_t previous_chip_rate = tdma_sync->chip_rate;

    tdma_sync->pll_ratio = PLL_RATIO(chip_rate);
    tdma_sync->chip_rate = chip_rate;

    float chip_rate_transition_ratio = PLL_RATIO(chip_rate) / (float)PLL_RATIO(previous_chip_rate);

    tdma_sync->chip_rate_ratio = PLL_RATIO(chip_rate) / (float)PLL_RATIO(CHIP_RATE_20_48_MHZ);

    duration_pll_cycles = duration_pll_cycles * tdma_sync->chip_rate_ratio;

    tdma_sync->xtal_cycle_offset = tdma_sync->xtal_cycle_offset * chip_rate_transition_ratio;

    if (sleep_mode == SLEEP_IDLE) {
        duration_pll_cycles += tdma_sync->pwr_up_value;
    }

    /* When changing from non-idle to idle, increase the duration by pwr by previous pwr up delay */
    if (tdma_sync->sleep_mode != SLEEP_IDLE && sleep_mode == SLEEP_IDLE) {
        tdma_sync->xtal_cycle_offset = 0;
    }

    /* When changing from non-deep to deep, reduce the duration by the time the radio needs to wakeup
     * from shallow to deep
     */
    if (tdma_sync->sleep_mode != SLEEP_DEEP && sleep_mode == SLEEP_DEEP) {
        duration_pll_cycles -= DEEP_TO_SHALLOW_XTAL_CYCLES * tdma_sync->pll_ratio;
    }

    /* When changing from deep to shallow, increase the duration by the time the radio needs to wakeup
     * from shallow to deep
     */
    if (tdma_sync->sleep_mode == SLEEP_DEEP && sleep_mode == SLEEP_SHALLOW) {
        duration_pll_cycles += DEEP_TO_SHALLOW_XTAL_CYCLES * tdma_sync->pll_ratio;
    }

    /* When changing from idle to non-idle, adjust duration based on XTAL clock offset */
    if (tdma_sync->sleep_mode == SLEEP_IDLE && sleep_mode != SLEEP_IDLE) {
        /* Apply offset */
        duration_pll_cycles -= (tdma_sync->pll_ratio - tdma_sync->xtal_cycle_offset);
        /* Adjust depending on what side of XTAL clock rising edge the offset is */
        if (tdma_sync->xtal_cycle_offset >= tdma_sync->pll_ratio / 2) {
            duration_pll_cycles -= tdma_sync->pll_ratio;
        }
    }

#if WPS_RADIO_COUNT == 1
    tdma_sync->sleep_mode = sleep_mode;
#else
    (void)sleep_mode;
    tdma_sync->sleep_mode = SLEEP_IDLE;
#endif

    switch (tdma_sync->sleep_mode) {
    case SLEEP_IDLE:
        tdma_sync->sleep_offset_pll_cycles = 1;
        break;
    case SLEEP_SHALLOW:
    case SLEEP_DEEP:
        tdma_sync->sleep_offset_pll_cycles = tdma_sync->pll_ratio;
        break;
    default:
        break;
    }

    if (tdma_sync->tx_jitter_enabled) {
        rand_num = rand() % RANDOM_OFFSET_COUNT;
        random_offset = rand_offset_table[rand_num];
    }

    duration_pll_cycles += tdma_sync->sync_slave_offset * chip_rate_transition_ratio;
    duration_pll_cycles += random_offset;

    if (tdma_sync->previous_frame_type == FRAME_RX) {
        duration_pll_cycles += tdma_sync->setup_time_pll_cycles * tdma_sync->chip_rate_ratio;
        duration_pll_cycles -= cca->on_time_pll_cycles * tdma_sync->chip_rate_ratio;
    }
    tdma_sync->previous_frame_type = FRAME_TX;

    sync_update(tdma_sync, duration_pll_cycles, cca);
    tdma_sync->sync_slave_offset = 0;

    /* In TX, force timeout value for auto-reply to be default one */
    tdma_sync->timeout_value = 2 * tdma_sync->setup_time_pll_cycles + tdma_sync->preamble_size_bits +
                               tdma_sync->sfd_size_bits;
}

void link_tdma_sync_update_rx(tdma_sync_t *tdma_sync, uint32_t duration_pll_cycles, link_cca_t *cca,
                              sleep_lvl_t sleep_mode, chip_rate_cfg_t chip_rate)
{
    chip_rate_cfg_t previous_chip_rate = tdma_sync->chip_rate;

    tdma_sync->pll_ratio = PLL_RATIO(chip_rate);
    tdma_sync->chip_rate = chip_rate;

    float chip_rate_transition_ratio = PLL_RATIO(chip_rate) / (float)PLL_RATIO(previous_chip_rate);

    tdma_sync->chip_rate_ratio = PLL_RATIO(chip_rate) / (float)PLL_RATIO(CHIP_RATE_20_48_MHZ);

    duration_pll_cycles = duration_pll_cycles * tdma_sync->chip_rate_ratio;

    tdma_sync->xtal_cycle_offset = tdma_sync->xtal_cycle_offset * chip_rate_transition_ratio;

    tdma_sync->timeout_pll_cycles = (2 * tdma_sync->setup_time_pll_cycles + tdma_sync->preamble_size_bits +
                                     tdma_sync->sfd_size_bits) *
                                    tdma_sync->chip_rate_ratio;

    if (sleep_mode == SLEEP_IDLE) {
        duration_pll_cycles += tdma_sync->pwr_up_value;
    }

    /* When changing from non-idle to idle, increase the duration by pwr by previous pwr up delay */
    if (tdma_sync->sleep_mode != SLEEP_IDLE && sleep_mode == SLEEP_IDLE) {
        tdma_sync->xtal_cycle_offset = 0;
    }

    /* When changing from non-deep to deep, reduce the duration by the time the radio needs to wakeup
     * from shallow to deep
     */
    if (tdma_sync->sleep_mode != SLEEP_DEEP && sleep_mode == SLEEP_DEEP) {
        duration_pll_cycles -= DEEP_TO_SHALLOW_XTAL_CYCLES * tdma_sync->pll_ratio;
    }

    /* When changing from deep to shallow, increase the duration by the time the radio needs to wakeup
     * from shallow to deep
     */
    if (tdma_sync->sleep_mode == SLEEP_DEEP && sleep_mode == SLEEP_SHALLOW) {
        duration_pll_cycles += DEEP_TO_SHALLOW_XTAL_CYCLES * tdma_sync->pll_ratio;
    }

    /* When changing from idle to non-idle, adjust duration based on XTAL clock offset */
    if (tdma_sync->sleep_mode == SLEEP_IDLE && sleep_mode != SLEEP_IDLE) {
        /* Apply offset */
        duration_pll_cycles -= (tdma_sync->pll_ratio - tdma_sync->xtal_cycle_offset);
        /* Adjust depending on what side of XTAL clock rising edge the offset is */
        if (tdma_sync->xtal_cycle_offset >= tdma_sync->pll_ratio / 2) {
            duration_pll_cycles -= tdma_sync->pll_ratio;
        }
    }

#if WPS_RADIO_COUNT == 1
    tdma_sync->sleep_mode = sleep_mode;
#else
    (void)sleep_mode;
    tdma_sync->sleep_mode = SLEEP_IDLE;
#endif

    switch (tdma_sync->sleep_mode) {
    case SLEEP_IDLE:
        tdma_sync->sleep_offset_pll_cycles = 1;
        break;
    case SLEEP_SHALLOW:
    case SLEEP_DEEP:
        tdma_sync->sleep_offset_pll_cycles = tdma_sync->pll_ratio;
        break;
    default:
        break;
    }

    duration_pll_cycles += tdma_sync->sync_slave_offset * chip_rate_transition_ratio;

    if (tdma_sync->previous_frame_type == FRAME_TX) {
        duration_pll_cycles -= tdma_sync->setup_time_pll_cycles * tdma_sync->chip_rate_ratio;
        duration_pll_cycles += cca->on_time_pll_cycles * tdma_sync->chip_rate_ratio;
    }
    tdma_sync->previous_frame_type = FRAME_RX;

    sync_update(tdma_sync, duration_pll_cycles, cca);
    tdma_sync->sync_slave_offset = 0;
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

    return (symbols_count - 1 - (48 / chips_per_symbol)) / (4 * chip_multiplier);
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

    symbols_count = (preamble_len_reg_val * 4 * chip_multiplier) + (48 / chips_per_symbol) + 1;

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
    tdma_sync->timeout_pll_cycles = 2 * tdma_sync->setup_time_pll_cycles + tdma_sync->preamble_size_bits +
                                    tdma_sync->sfd_size_bits;
    tdma_sync->base_target_rx_waited_pll_cycles = tdma_sync->setup_time_pll_cycles + tdma_sync->preamble_size_bits +
                                                  tdma_sync->sfd_size_bits;
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
    symbols_count = (preamble_len_reg_val * 4 * chip_multiplier) + (48 / chips_per_symbol) + 1;

    tdma_sync->preamble_size_bits = symbols_count * chips_per_symbol;
    tdma_sync->timeout_pll_cycles = 2 * tdma_sync->setup_time_pll_cycles + tdma_sync->preamble_size_bits +
                                    tdma_sync->sfd_size_bits;
    tdma_sync->base_target_rx_waited_pll_cycles = tdma_sync->setup_time_pll_cycles + tdma_sync->preamble_size_bits +
                                                  tdma_sync->sfd_size_bits;
}

uint64_t link_tdma_get_time_stamp_pll_cycles(tdma_sync_t *tdma_sync)
{
    return tdma_sync->free_running_timer;
}

uint16_t link_tdma_get_elapsed_time_us(tdma_sync_t *tdma_sync, uint64_t previous_time_stamp)
{
    return ((uint64_t)(tdma_sync->free_running_timer - previous_time_stamp) * 1000) /
           PLL_FREQ_KHZ(tdma_sync->chip_rate);
}

uint16_t link_tdma_get_last_timer_increment_us(tdma_sync_t *tdma_sync)
{
    return ((uint64_t)tdma_sync->last_timer_increment * 1000) / PLL_FREQ_KHZ(tdma_sync->chip_rate);
}
/* PRIVATE FUNCTIONS **********************************************************/

/** @brief Update TDMA sync module.
 *
 *  @param[in] tdma_sync            TDMA sync object.
 *  @param[in] duration_pll_cycles  Duration in PLL clock cycles.
 *  @param[in] cca                  CCA object.
 *  @return None.
 */
static inline void sync_update(tdma_sync_t *tdma_sync, uint32_t duration_pll_cycles, link_cca_t *cca)
{
    uint32_t timeout_pll_cycles = 0;

    uint16_t cca_retry_time = cca->retry_time_pll_cycles * tdma_sync->chip_rate_ratio;
    uint16_t cca_on_time = cca->on_time_pll_cycles * tdma_sync->chip_rate_ratio;

    if (cca->enable) {
        if (cca->fail_action == CCA_FAIL_ACTION_ABORT_TX) {
            timeout_pll_cycles = tdma_sync->timeout_pll_cycles +
                                 (cca->max_try_count - 1) * (cca_retry_time + cca_on_time);
        } else {
            timeout_pll_cycles = tdma_sync->timeout_pll_cycles + cca->max_try_count * (cca_retry_time + cca_on_time);
        }
    } else {
        timeout_pll_cycles = tdma_sync->timeout_pll_cycles;
    }

    switch (tdma_sync->sleep_mode) {
    case SLEEP_SHALLOW:
    case SLEEP_DEEP:
        duration_pll_cycles -= tdma_sync->sleep_offset_pll_cycles;
        tdma_sync->sleep_cycles_value = duration_pll_cycles / tdma_sync->pll_ratio;
        tdma_sync->pwr_up_value += float_mod((float)duration_pll_cycles, tdma_sync->pll_ratio);
        if (tdma_sync->pwr_up_value > tdma_sync->pll_ratio) {
            tdma_sync->sleep_cycles_value++;
            tdma_sync->pwr_up_value = float_mod((float)tdma_sync->pwr_up_value, tdma_sync->pll_ratio);
        }
        tdma_sync->timeout_value = timeout_pll_cycles + tdma_sync->pwr_up_value;
        break;
    case SLEEP_IDLE:
    default:
        tdma_sync->sleep_cycles_value = duration_pll_cycles - tdma_sync->sleep_offset_pll_cycles;
        tdma_sync->pwr_up_value = 0;
        tdma_sync->timeout_value = timeout_pll_cycles;
        tdma_sync->xtal_cycle_offset += float_mod((float)tdma_sync->sleep_cycles_value, tdma_sync->pll_ratio);
        /* Compute XTAL offset */
        if (tdma_sync->xtal_cycle_offset > tdma_sync->pll_ratio) {
            tdma_sync->xtal_cycle_offset = float_mod((float)tdma_sync->xtal_cycle_offset, tdma_sync->pll_ratio);
        }
        /* Avoid having an offset exactly on XTAL clock rising edge where timer reset moment can vary slightly  */
        if ((tdma_sync->xtal_cycle_offset >= METASTABILITY_WINDOW_LOWER_BOUND * tdma_sync->chip_rate_ratio) &&
            (tdma_sync->xtal_cycle_offset <= METASTABILITY_WINDOW_UPPER_BOUND * tdma_sync->chip_rate_ratio)) {
            tdma_sync->pwr_up_value = 2 * METASTABILITY_WINDOW_WIDTH * tdma_sync->chip_rate_ratio;
            tdma_sync->sleep_cycles_value -= 2 * METASTABILITY_WINDOW_WIDTH * tdma_sync->chip_rate_ratio;
            tdma_sync->xtal_cycle_offset -= 2 * METASTABILITY_WINDOW_WIDTH * tdma_sync->chip_rate_ratio;
        }
        break;
    }
    tdma_sync->ts_duration_pll_cycles += duration_pll_cycles;
    tdma_sync->free_running_timer += duration_pll_cycles;
    tdma_sync->last_timer_increment = duration_pll_cycles;
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
    uint16_t target_rx_waited_pll_cycles = 0;
    uint16_t cca_retry_time = cca->retry_time_pll_cycles * tdma_sync->chip_rate_ratio;
    uint16_t cca_on_time = cca->on_time_pll_cycles * tdma_sync->chip_rate_ratio;

    if (tdma_sync->fast_sync_enable && (tdma_sync->slave_sync_state == STATE_SYNCING)) {
        tdma_sync->pwr_up_value = 0;
    }

    rx_waited_pll_cycles -= tdma_sync->pwr_up_value;

    tdma_sync->frame_lost_duration = 0;
    tdma_sync->ts_duration_pll_cycles = 0;
    target_rx_waited_pll_cycles = (tdma_sync->base_target_rx_waited_pll_cycles +
                                   (cca_retry_time + cca_on_time) * rx_cca_retry_count);

    if (rx_cca_retry_count == cca->max_try_count && cca->fail_action == CCA_FAIL_ACTION_TX) {
        /**
         * In TX anyway, the radio will apply a retry_time on the last check starting from the begining of the previous
         * CCA ON period. We thus need to remove the cca_on_time from the target_rx_waited_pll_cycles.
         */
        target_rx_waited_pll_cycles -= cca_on_time;
    }

    if (tdma_sync->fast_sync_enable && (tdma_sync->slave_sync_state == STATE_SYNCING)) {
        tdma_sync->sync_slave_offset = -target_rx_waited_pll_cycles;
    } else {
        tdma_sync->sync_slave_offset = slave_calculate_offset(target_rx_waited_pll_cycles, rx_waited_pll_cycles);
    }

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
