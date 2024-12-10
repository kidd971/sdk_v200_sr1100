/** @file link_tdma_sync.h
 *  @brief TDMA sync module.
 *
 *  @copyright Copyright (C) 2020 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef LINK_TDMA_H_
#define LINK_TDMA_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "link_cca.h"
#include "link_utils.h"
#include "sr1100_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
#define UNSYNC_OFFSET_PLL_CYCLES 400

/* TYPES **********************************************************************/
#define STATE_SYNCING false
#define STATE_SYNCED  true

/** @brief Frame type enumeration.
 */
typedef enum frame_type {
    /*! Frame reception */
    FRAME_RX,
    /*! Frame transmission */
    FRAME_TX,
} frame_type_t;

typedef struct tdma_sync {
    /*! Sleep mode */
    sleep_lvl_t sleep_mode;
    /*! timeout duration in PLL cycles */
    uint32_t timeout_pll_cycles;
    /*! RX setup time in PLL cycles */
    uint16_t setup_time_pll_cycles;
    /*! Base target RX waited in PLL cycles */
    uint16_t base_target_rx_waited_pll_cycles;
    /*! Synchronization state */
    bool slave_sync_state;
    /*! Fast sync enable flag */
    bool fast_sync_enable;
    /*! Maximum duration of consecutive lost frames before the sync is considered lost */
    uint32_t frame_lost_max_duration;
    /*! Sleep time offset in PLL cycles */
    uint32_t sleep_offset_pll_cycles;
    /*! Slave sync off set in PLL cycles */
    int32_t sync_slave_offset;
    /*! Type of the previous frame */
    frame_type_t previous_frame_type;
    /*! Frame lost duration */
    uint32_t frame_lost_duration;
    /*! Sleep cycles value in PLL cycles */
    uint32_t sleep_cycles_value;
    /*! Timeslot duration in PLL cycles */
    uint32_t ts_duration_pll_cycles;
    /*! Timeout value in PLL cycles */
    uint32_t timeout_value;
    /*! Power up delay in PLL cycles */
    uint16_t pwr_up_value;
    /*! ISI mitigation level */
    isi_mitig_t isi_mitig;
    /*! ISI mitigation level corresponding pauses */
    uint8_t isi_mitig_pauses;
    /*! TX jitter enable flag */
    bool tx_jitter_enabled;
    /*! PLL ratio */
    uint16_t pll_ratio;
    /*! Sync word size in bits. */
    uint8_t sync_word_size_bits;
    /*! Preamble size in bits. */
    uint16_t preamble_size_bits;
} tdma_sync_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize TDMA sync object.
 *
 *                              |------preamble_size_bits------|-sync_word_size_bits-|
 *
 *  |---------------------------------------------timeout_pll_cycles---------------------------------------------|
 *
 *  |---setup_time_pll_cycles---|____________________________________________________|---setup_time_pll_cycles---|
 *
 *
 *  The preamble and sync word are expected to arrive in the center of the RX timeout window.
 *
 *  In order for the frame to start within the RX timeout window, the setup time must be chosen to
 *  take into account the maximum possible drift between 2 receptions.
 *
 *
 *  @param[in] tdma_sync               TDMA sync object.
 *  @param[in] sleep_mode              Sleep mode.
 *  @param[in] setup_time_pll_cycles   RX setup time in PLL cycles.
 *  @param[in] frame_lost_max_duration Frame lost maximum duration.
 *  @param[in] sync_word_size_bits     Sync word size in bits.
 *  @param[in] preamble_size_bits      Preamble size in bits.
 *  @param[in] pll_startup_xtal_cycles PLL startup time in XTAL cycles.
 *  @param[in] isi_mitig               ISI mitigation level.
 *  @param[in] isi_mitig_pauses        ISI mitigation level corresponding pauses.
 *  @param[in] seed                    Random TX jitter seed.
 *  @param[in] fast_sync_enable        Fast sync enable flag.
 *  @param[in] tx_jitter_enabled       TX jitter enable flag.
 *  @param[in] chip_rate               Chip rate.
 *  @return None.
 */
void link_tdma_sync_init(tdma_sync_t *tdma_sync, sleep_lvl_t sleep_mode, uint16_t setup_time_pll_cycles,
                         uint32_t frame_lost_max_duration, uint8_t sync_word_size_bits, uint16_t preamble_size_bits,
                         uint8_t pll_startup_xtal_cycles, isi_mitig_t isi_mitig, uint8_t isi_mitig_pauses,
                         uint16_t seed, bool fast_sync_enable, bool tx_jitter_enabled, chip_rate_cfg_t chip_rate);

/** @brief Update TDMA sync module for RX frame.
 *
 *  @param[in] tdma_sync            TDMA sync object.
 *  @param[in] duration_pll_cycles  Duration in PLL clock cycles.
 *  @param[in] cca                  CCA object.
 *  @param[in] sleep_mode           Sleep mode.
 *  @return None.
 */
void link_tdma_sync_update_tx(tdma_sync_t *tdma_sync, uint32_t duration_pll_cycles, link_cca_t *cca,
                              sleep_lvl_t sleep_mode);

/** @brief Update TDMA sync module for TX frame.
 *
 *  @param[in] tdma_sync            TDMA sync object.
 *  @param[in] duration_pll_cycles  Duration in PLL clock cycles.
 *  @param[in] cca                  CCA object.
 *  @param[in] sleep_mode           Sleep mode.
 *  @return None.
 */
void link_tdma_sync_update_rx(tdma_sync_t *tdma_sync, uint32_t duration_pll_cycles, link_cca_t *cca,
                              sleep_lvl_t sleep_mode);

/** @brief Update Adjust slave sync.
 *
 *  @param[in] tdma_sync             TDMA sync object.
 *  @param[in] frame_outcome         Frame outcome.
 *  @param[in] rx_waited_pll_cycles  RX waited value in PLL clock cycles.
 *  @param[in] cca                   CCA object.
 *  @param[in] rx_cca_retry_count    RX CCA retry count
 *  @return None.
 */
void link_tdma_sync_slave_adjust(tdma_sync_t *tdma_sync, frame_outcome_t frame_outcome, uint16_t rx_waited_pll_cycles, link_cca_t *cca,
                                 uint8_t rx_cca_retry_count);

/** @brief Try to get synced on the master
 *
 *  @param[in] tdma_sync             TDMA sync object.
 *  @param[in] frame_outcome         Frame outcome.
 *  @param[in] rx_waited_pll_cycles  RX waited value in PLL clock cycles.
 *  @param[in] cca                   CCA object.
 *  @param[in] rx_cca_retry_count    RX CCA retry count
 */
void link_tdma_sync_slave_find(tdma_sync_t *tdma_sync, frame_outcome_t frame_outcome, uint16_t rx_waited_pll_cycles, link_cca_t *cca,
                               uint8_t rx_cca_retry_count);

/** @brief Get sleep cycles.
 *
 *  @param[in] tdma_sync  TDMA sync object.
 *  @return Sleep cycles.
 */
static inline uint32_t link_tdma_sync_get_sleep_cycles(tdma_sync_t *tdma_sync)
{
    return tdma_sync->sleep_cycles_value;
}

/** @brief Get timeout.
 *
 *  @param[in] tdma_sync  TDMA sync object.
 *  @return Timeout.
 */
static inline uint32_t link_tdma_sync_get_timeout(tdma_sync_t *tdma_sync)
{
    return tdma_sync->timeout_value;
}

/** @brief Get power up delay.
 *
 *  @param[in] tdma_sync  TDMA sync object.
 *  @return Power up delay.
 */
static inline uint16_t link_tdma_sync_get_pwr_up(tdma_sync_t *tdma_sync)
{
    return tdma_sync->pwr_up_value;
}

/** @brief Get slave sync flag.
 *
 *  @param[in] tdma_sync  TDMA sync object.
 *  @return Slave sync state.
 */
static inline bool link_tdma_sync_is_slave_synced(tdma_sync_t *tdma_sync)
{
    return tdma_sync->slave_sync_state;
}

/** @brief Get the ISI mitigation pauses.
 *
 *  @param[in] input_isi_mitig ISI_MITIG register value
 *  @return Number of ISI mitigation pauses
 */
uint8_t link_tdma_sync_get_isi_mitigation_pauses(isi_mitig_t isi_mitig_reg_val);

/** @brief Return the preamble length in bits for the MAC layer.
 *
 *  @param[in] isi_mitig_pauses      Number of ISI mitigation pauses
 *  @param[in] preamble_len_reg_val  Preamble length register value.
 *  @param[in] syncword_len_reg_val  Sync word length register value.
 *  @return Preamble length in chips
 */
uint32_t link_tdma_get_preamble_length(uint8_t isi_mitig_pauses, uint32_t preamble_len_reg_val,
                                       syncword_length_t syncword_len_reg_val);

/** @brief Get syncword length.
 *
 *  @param[in] isi_mitig_pauses      Number of ISI mitigation pauses
 *  @param[in] syncword_len_reg_val  Syncword length register value.
 *  @return Syncword length in chips
 */
uint32_t link_tdma_get_syncword_length(uint8_t isi_mitig_pauses, syncword_length_t syncword_len_reg_val);

#ifdef __cplusplus
}
#endif
#endif /* LINK_MULTI_RADIO_H_ */
