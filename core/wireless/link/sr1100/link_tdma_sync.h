/* clang-format off */
/** @file link_tdma_sync.h
 *  @brief TDMA sync module.
 *
 *  The TDMA module is responsible for keeping the sync between two radio. It is to ensure that the receiving radio
 *  catches the preamble and SFD that the transmitter is sending and locks on the signal.
 *
 *  ========== Notes about the wake-up timers of the radio ==========
 *
 *  The SR11X0 radio has wake-up timers. The first timer always counts on the xtal clock. The second timer always count
 *  on the currently active system clock. When the radio is in SHALLOW or DEEP sleep states, the clock is 32.768 kHz
 *  (xtal clock). For IDLE or ACTIVE power states, the clock is the currently configured PHY rate (chip rate). These may
 *  be 20.48 MHz, 27.30 MHz or 40.96 MHz. These clock speed are determined by the PLL.
 *
 *  ref: SR1120 V3 datasheet, section 5.2.3. Wake-Up Timers: SLPPERIOD description
 *
 *  ========== Important registers ==========
 *
 *  REG16_SLPPERIOD_15_0(0x0C): First 2 bytes of the sleep period.
 *
 *      The sleep period is the amount of clock cycles that the wake-up timers must be equal to for the radio to wake
 *      up. The timers countinuously count and will simply rollover when they reach the "sleep period". The exact period
 *      between two consecutive wake-up events (provided the device goes to sleep before the end of each power cycle) is
 *      SLPPERIOD + 1 clock cycles of the clock source used by the selected wake-up timer.
 *
 *      ref: SR1120 V3 datasheet, section 5.2.3 - Wake-Up Timers: SLPPERIOD description
 *
 *  REG16_SLPPERIOD_PWRUPDLAY(0x0D):
 *
 *      - SLPPERIOD_23_16: Last byte of the configurable sleep period.
 *      - PWRUPDLAY      : Extra time to wait after the chip wakes up before the transceiver listens or transmits over
 *                         the air. The time waited is equal to the register value times 8 chip clock cycles (PHY
 *                         rate/PLL clock). This is quite usefull if the radio is in a deep/shallow sleep and only has
 *                         a crystal clock precision to count. It can add a transceiver "power-up delay" that is
 *                         counted with the precision of the PLL since the radio is now awake.
 *
 *                         ref: SR1120 V3 datasheet,
 *                         section 5.2.4 - Transceiver Wake-Up Time and Timeout: PWRUPDLAY description
 *
 *  REG16_TIMELIMIT_BIASDELAY(0x0E):
 *
 *      - TIMEOUT: Cycle count for the radio's RX to timeout and indicate that an RX has failed and timed out, usually
 *                 through an IRQ. When the radio starts listening over the air, a timer begins and counts until it
 *                 has reaches this value.
 *
 *  REG16_TIMERCFG_SLEEPCFG(0x0F): Configure the sleep behavior.
 *
 *      - SLPRXEND: 1 - Put radio to sleep when RX ends
 *      - SLPTXEND: 1 - Put radio to sleep when TX ends
 *      - SYNWAKUP: X - Reset the wake-up timer when radio wakes up (This is not set explicitaly in the code, but the
 *                      default behavior of the radio is that it does reset)
 *                      ref: SR1120 V3 datasheet, section 5.2.5 - Wake-Up Timers: SYNWAKUP description
 *
 *  RXSFDTIME(0x13): Saves the count held in the wake-up timer right after the SFD is received. This is the value that
 *                   will allow to adjust the sync.
 *                   ref: SR1120 V3 datasheet, section 5.2.5 - Time Records: RXSFDTIME description
 *
 *  ========== Strategy for syncing ==========
 *
 *      In the TDMA sync module, a "target" RXSFDTIME is defined. On each time slot, the RXSFDTIME is read from the
 *      radios registers. If there is a difference (error), the sleep time is adjusted so that on the next timeslot,
 *      the difference between the target and the actual goes to 0.
 *
 *      Ex:
 *          - expected = 1000
 *          - actual = 1024 (this mean we woke up 24 cycles too early)
 *          - set SLPPERIOD = 1024 (we compensate on the next timeslot to wake up 24 cycles later)
 *
 *  ========== Timeslot diagram ==========
 *
 *      Now that we have a strategy, let's define what the "target" RXSFDTIME should theoretically be. To do that, the
 *      events happening during a timeslot must be analyzed. Let's use a simple timeslot of 1 ms.
 *
 *      Let's first consider what the TX looks like for the radio, since ultimately, it's the RX that must sync
 *      onto the TX.
 *
 *      radio air time  : ====
 *      radio sleep time: ----
 *
 *      0 ms                                                                                   1ms
 *  TX: |<cca><preamb><sfd><payload>-<ack>-----------------------------------------------------|
 *  TX: |=================================-----------------------------------------------------|
 *
 *                                  Figure 1: A typical TX frame
 *
 *      To sync on the signal, the RX must catch the preamble and the SFD. It will then extend its listening period
 *      until the full payload is received. With this information, we can define an ideal RX frame. The RX frame should
 *      try to "surround" the preamble and SFD with a small error margin.
 *
 *      0 ms                                                                                   1ms
 *  TX: |<cca><preamb><sfd><payload>-<ack>-----------------------------------------------------|
 *  RX: |<---- RX window ---->|
 *      |=================================-----------------------------------------------------|
 *
 *                             Figure 2: A typical RX frame next to a TX
 *
 *      The TDMA sync module represents this error margin as the "setup" time. In short, that whole sequence is the RX
 *      timeout.
 *
 *                      |<-setup-><----preamble----><----SFD----><-setup->|
 *                      |<------------------ RX timeout ----------------->|
 *
 *                            Figure 3: Definition of an RX window
 *
 *      In reality, the TDMA sync module tries to wake up TX and RX a bit delayed. This is because the TX will
 *      immediately start doing a CCA, preamble, SFD, payload and ack when it wakes up. The ideal scenario is that the
 *      TX wakes up a bit later (tx_delay) so that its preamble starts just after the setup time of the RX. Here are
 *      both TX and RX frames together.
 *
 *      The code will simply add or remove this delay when switching from RX to TX. Its counterpart will do the same,
 *      and all this will then cancel out.
 *
 *                   0 ms                                                                                     1ms
 *  TX:  < tx_delay >|<cca><preamble><sfd><----------- payload ---//------>-<ack>-----------------------------|
 *  TX:  < tx_delay >|============================================//=============-----------------------------|
 *  RX:  |<---- setup ---->               <---- setup ---->|                                                  |
 *  RX:  |<------ target RXSFDTIME ----->| (expected time between wake up and end of SFD)                     |
 *  RX:  |<------------------- RX timeout ---------------->|                                                  |
 *  RX:  |                                                                       <---- sleep ---->|< tx_delay >
 *  RX:  |<----------------------------------- SLPPERIOD ---------//----------------------------->|< tx_delay >
 *       0 ms                                                                                     1ms
 *
 *                      Figure 4: Synced RX and TX frames with the aligned RX window definition
 *
 *      If the 2 radios have any drift at all in their PLLs, the two frames might become de-synced, as represented on
 *      figure 5, the RX will add this drift to the sleep period so that on the next timeslot, the
 *      expected RXSFDTIME is equal to the actual RXSFDTIME. This is a visual representation of the example given in
 *      the strategy explanation.
 *
 *                   0 ms                                                                                     1ms
 *  TX:  < tx_delay >|<- drift -><cca><preamble><sfd><----------- payload -----//---->-<ack>-----------------------------|
 *  TX:  < tx_delay >|<- drift ->==============================================//===========-----------------------------|
 *  RX:  |<---- setup ---->               <---- setup ---->|                                                             |
 *  RX:  |<------ target RXSFDTIME ----->| (expected time between wake up and end of SFD)                                |
 *  RX:  |<------------------- RX timeout ---------------->|                                                             |
 *  RX:  |                                                                                   <---- sleep ---->< tx_delay >
 *  RX:  |<----------------------------------------- SLPPERIOD ----------------//---------------->|<- drift ->< tx_delay >
 *  RX:  |<--------------------------------------- new SLPPERIOD --------------//--------------------------->|< tx_delay >
 *       0 ms                                                                                     1ms
 *
 *                   Figure 5: De-synced RX and TX frames compensated by adding drift to next sleep period
 *
 * ========== SHALLOW and DEEP sleep levels ===========
 *
 *      When the radio is requested to go into SHALLOW or DEEP sleep, the wake-up timer in the radio counts at 32.768
 *      kHz (xtal clock). This is considerably lower than a typical PHY rate of 20.48 MHz. The TDMA sync module will
 *      then calculate the nearest xtal clock cycle and add a power up delay so that the TX/RX begins at the proper
 *      time. The difference is represented in figure 6.
 *
 *      Note:
 *
 *      1. The radio wakes up on xtal clock falling edge.
 *      2. The XTAL counter resets on xtal clock rising edge.
 *      3. There is a race condition between the reset of the xtal counter and the signal to reset the timer when using
 *         the PLL clock. So if we wake up the radio (sending a timer reset signal) at the same time that the xtal timer
 *         should reset, we cannot be sure whether the reset will happen this cycle or the next one.
 *
 *    start sleep                                                      wake-up     Target wake-up
 *        |                                                               |<--------->|
 *        |                                                               |PWRUP DELAY|
 *        |-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-
 *  PLL   | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
 *        | +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+
 *        |   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27
 *        |                                                               |           |
 *        |       +-------+       +-------+       +-------+       +-------+       +-------+       +-------+       +-----
 *  xtal  |       |       |       |       |       |       |       |       |       |   |   |       |       |       |
 *        +-------+       +-------+       +-------+       +-------+       +-------+   |   +-------+       +-------+
 *                        1               2               3               4       ^       5               6
 *                                                                                |
 *                                                                        Timer Reset Signal
 *
 *                   Figure 6: Using PWRUPDLAY to compensate for xtal's timing precision compared to PLL clock
 *
 *      We see that when in IDLE the number of PLL cycles to put in SLPPERIOD is 18 (19 cycles). If the radio was to be
 *      in SHALLOW sleep mode, the number of cycles to put in SLPPERIOD would be 3 (4 cycles) and PWRUPDLAY would be 3
 *      PLL cycle.
 *
 *      We also notice that at any moment, the xtal offset must be kept into account to make sure that this half
 *      cycle's time is accounted for in our calculations. The xtal offset is 0 when the radio wakes up from SHALLOW
 *      or DEEP sleep by definition.
 *
 *      Here are the procedures to address the race condition problem:
 *
 *      1. Keep track of the offset between the xtal clock falling edge and the PLL clock wakeup event.
 *      2. If we are waking up within the metastability window of the xtal rising edge, (i.e. when xtal_cycle_offset =
 *         1/2 xtal cycle), we sould wake up the equivalent of two times the width of the metastability window and use
 *         PWRUPDLAY to fill the remaining "sleep" time. This will allow the reset signal to be received in time for the
 *         xtal timer reset to be applied properly. This is shown in figure 7 and figure 8.
 *
 *      Let's see a problematic example when xtal offset is close to 1/2 xtal cycle.
 *
 *    start sleep                                              wake-up
 *        |                                                       |
 *        |                                                       |
 *        |-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-
 *  PLL   | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
 *        | +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+
 *        |   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27
 *        |                                                       |
 *        |                                                      <-> (metastability window)
 *        |       +-------+       +-------+       +-------+       +-------+       +-------+       +-------+       +-----
 *  xtal  |       |       |       |       |       |       |       |       |       |       |       |       |       |
 *        +-------+       +-------+       +-------+       +-------+       +-------+       +-------+       +-------+
 *                        1               2               3       ^       4               5               6
 *                         (xtal offset = ~1/2 xtal cycle)|<----->|
 *                                                                |
 *                                                        Timer Reset Signal
 *
 *                                              Figure 7: Unsafe xtal counter reset timing
 *
 *                                                          PWRUPDLAY
 *    start sleep                                             <--->
 *        |                                             wake-up   desired wake-up
 *        |                                                   |   |
 *        |-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-
 *  PLL   | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
 *        | +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+
 *        |   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27
 *        |                                                   |   |
 *        |                                                   <---> (2x metastability window)
 *        |       +-------+       +-------+       +-------+   |   +-------+       +-------+       +-------+       +-----
 *  xtal  |       |       |       |       |       |       |   |   |       |       |       |       |       |       |
 *        +-------+       +-------+       +-------+       +-------+       +-------+       +-------+       +-------+
 *                        1               2               3   |   ^       4               5               6
 *                                           (xtal offset)|<->|   |
 *                                                            | Timer Reset
 *                                                      Timer Reset
 *                                                         Signal
 *
 *                                              Figure 8: Safe xtal counter reset timing
 *
 *      We can see in figure 8 that the reset does not fall in the race condition window as opposed to figure 7.
 *      However, the two start the transceiver at the exact same moment.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
/* clang-format on */

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
/* Offset to apply when unsynchronized (800 / 40.96 MHz = 19.53 us) */
#define UNSYNC_OFFSET_PLL_CYCLES 800

/* TYPES **********************************************************************/
#define STATE_SYNCING false
#define STATE_SYNCED  true

/** @brief Frame type enumeration.
 */
typedef enum frame_type {
    /*! Frame reception. */
    FRAME_RX,
    /*! Frame transmission. */
    FRAME_TX,
} frame_type_t;

typedef struct tdma_sync {
    /*! Sleep mode. */
    sleep_lvl_t sleep_mode;
    /*! RX setup time in PLL cycles. */
    uint16_t setup_time_pll_cycles;
    /*! Synchronization state. */
    bool slave_sync_state;
    /*! Fast sync enable flag. */
    bool fast_sync_enable;
    /*! Maximum duration of consecutive lost frames before the sync is considered lost. */
    uint32_t frame_lost_max_duration;
    /*! Slave sync off set in PLL cycles. */
    int32_t sync_slave_offset;
    /*! Chip rate. */
    chip_rate_cfg_t sync_slave_offset_chip_rate;
    /*! Type of the previous frame. */
    frame_type_t previous_frame_type;
    /*! Frame lost duration. */
    uint32_t frame_lost_duration;
    /*! Sleep cycles register value. */
    uint32_t sleep_cycles_value;
    /*! Timeout value in PLL cycles. */
    uint32_t timeout_value;
    /*! Power up delay in PLL cycles. */
    uint16_t pwr_up_delay_pll_cycles;
    /*! Power up delay register value. */
    uint16_t pwr_up_value;
    /*! ISI mitigation level. */
    isi_mitig_t isi_mitig;
    /*! ISI mitigation level corresponding pauses. */
    uint8_t isi_mitig_pauses;
    /*! TX jitter enable flag. */
    bool tx_jitter_enabled;
    /*! SFD size in bits. */
    uint8_t sfd_size_bits;
    /*! Preamble size in bits. */
    uint16_t preamble_size_bits;
    /*! Crystal cycle offset. */
    uint32_t xtal_cycle_offset;
    /*! Chip rate. */
    chip_rate_cfg_t chip_rate;

    /* **** Time tracking @ TDMA BASE PHY RATE **** */

    /*! Timeslot duration in PLL cycles. */
    uint32_t ts_duration_pll_cycles;
    /*! Free running timer. */
    uint64_t free_running_timer;
    /*! Last timer increment value. */
    uint32_t last_timer_increment;
} tdma_sync_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize TDMA sync object.
 *
 *                              |------preamble_size_bits------|-sfd_size_bits-|
 *
 *  |---------------------------------------------timeout_pll_cycles---------------------------------------------|
 *
 *  |---setup_time_pll_cycles---|____________________________________________________|---setup_time_pll_cycles---|
 *
 *
 *  The preamble and SFD are expected to arrive in the center of the RX timeout window.
 *
 *  In order for the frame to start within the RX timeout window, the setup time must be chosen to
 *  take into account the maximum possible drift between 2 receptions.
 *
 *
 *  @param[in] tdma_sync                TDMA sync object.
 *  @param[in] sleep_mode               Sleep mode.
 *  @param[in] setup_time_us            RX setup time in micro seconds.
 *  @param[in] frame_lost_max_duration  Frame lost maximum duration.
 *  @param[in] sfd_size_bits            SFD size in bits.
 *  @param[in] preamble_size_bits       Preamble size in bits.
 *  @param[in] pll_startup_xtal_cycles  PLL startup time in XTAL cycles.
 *  @param[in] isi_mitig                ISI mitigation level.
 *  @param[in] isi_mitig_pauses         ISI mitigation level corresponding pauses.
 *  @param[in] seed                     Random TX jitter seed.
 *  @param[in] fast_sync_enable         Fast sync enable flag.
 *  @param[in] tx_jitter_enabled        TX jitter enable flag.
 *  @param[in] chip_rate                Chip rate.
 *  @return None.
 */
void link_tdma_sync_init(tdma_sync_t *tdma_sync, sleep_lvl_t sleep_mode, uint16_t setup_time_us,
                         uint32_t frame_lost_max_duration, uint8_t sfd_size_bits, uint16_t preamble_size_bits,
                         uint8_t pll_startup_xtal_cycles, isi_mitig_t isi_mitig, uint8_t isi_mitig_pauses,
                         uint16_t seed, bool fast_sync_enable, bool tx_jitter_enabled, chip_rate_cfg_t chip_rate);

/** @brief Update TDMA sync module for RX frame.
 *
 *  @param[in] tdma_sync            TDMA sync object.
 *  @param[in] duration_pll_cycles  Duration in PLL clock cycles.
 *  @param[in] cca                  CCA object.
 *  @param[in] sleep_mode           Sleep mode.
 *  @param[in] chip_rate            Chip rate.
 *  @return None.
 */
void link_tdma_sync_update_tx(tdma_sync_t *tdma_sync, uint32_t duration_pll_cycles, link_cca_t *cca,
                              sleep_lvl_t sleep_mode, chip_rate_cfg_t chip_rate);

/** @brief Update TDMA sync module for TX frame.
 *
 *  @param[in] tdma_sync            TDMA sync object.
 *  @param[in] duration_pll_cycles  Duration in PLL clock cycles.
 *  @param[in] cca                  CCA object.
 *  @param[in] sleep_mode           Sleep mode.
 *  @param[in] chip_rate            Chip rate.
 *  @return None.
 */
void link_tdma_sync_update_rx(tdma_sync_t *tdma_sync, uint32_t duration_pll_cycles, link_cca_t *cca,
                              sleep_lvl_t sleep_mode, chip_rate_cfg_t chip_rate);

/** @brief Update Adjust slave sync.
 *
 *  @param[in] tdma_sync             TDMA sync object.
 *  @param[in] frame_outcome         Frame outcome.
 *  @param[in] rx_waited_pll_cycles  RX waited value in PLL clock cycles.
 *  @param[in] cca                   CCA object.
 *  @param[in] rx_cca_retry_count    RX CCA retry count.
 *  @return None.
 */
void link_tdma_sync_slave_adjust(tdma_sync_t *tdma_sync, frame_outcome_t frame_outcome, uint16_t rx_waited_pll_cycles,
                                 link_cca_t *cca, uint8_t rx_cca_retry_count);

/** @brief Try to get synced on the master.
 *
 *  @param[in] tdma_sync             TDMA sync object.
 *  @param[in] frame_outcome         Frame outcome.
 *  @param[in] rx_waited_pll_cycles  RX waited value in PLL clock cycles.
 *  @param[in] cca                   CCA object.
 *  @param[in] rx_cca_retry_count    RX CCA retry count.
 */
void link_tdma_sync_slave_find(tdma_sync_t *tdma_sync, frame_outcome_t frame_outcome, uint16_t rx_waited_pll_cycles,
                               link_cca_t *cca, uint8_t rx_cca_retry_count);

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
 *  @param[in] input_isi_mitig  ISI_MITIG register value.
 *  @return Number of ISI mitigation pauses.
 */
uint8_t link_tdma_sync_get_isi_mitigation_pauses(isi_mitig_t isi_mitig_reg_val);

/** @brief Return the preamble length register value for the PHY layer.
 *
 *  @param[in] isi_mitig_pauses  Number of ISI mitigation pauses.
 *  @param[in] preamble_len      Preamble length in bits.
 *  @param[in] sfd_len_reg_val   SFD length register value.
 *  @return Preamble length register value.
 */
uint16_t link_tdma_get_preamble_reg_value(uint8_t isi_mitig_pauses, uint32_t preamble_len,
                                          sfd_length_t sfd_len_reg_val);

/** @brief Return the preamble length in bits for the MAC layer.
 *
 *  @param[in] isi_mitig_pauses      Number of ISI mitigation pauses.
 *  @param[in] preamble_len_reg_val  Preamble length register value.
 *  @param[in] sfd_len_reg_val       SFD length register value.
 *  @return Preamble length in chips.
 */
uint32_t link_tdma_get_preamble_length(uint8_t isi_mitig_pauses, uint32_t preamble_len_reg_val,
                                       sfd_length_t sfd_len_reg_val);

/** @brief Get SFD length.
 *
 *  @param[in] isi_mitig_pauses  Number of ISI mitigation pauses.
 *  @param[in] sfd_len_reg_val   SFD length register value.
 *  @return SFD length in chips.
 */
uint32_t link_tdma_get_sfd_length(uint8_t isi_mitig_pauses, sfd_length_t sfd_len_reg_val);

/** @brief Set the SFD length.
 *
 *  @param[in] tdma_sync         TDMA sync object.
 *  @param[in] isi_mitig_pauses  Number of ISI mitigation pauses.
 *  @param[in] sfd_len_reg_val   SFD length register value.
 */
void link_tdma_set_sfd_length(tdma_sync_t *tdma_sync, uint8_t isi_mitig_pauses, sfd_length_t sfd_len_reg_val);

/** @brief Set the preamble length.
 *
 *  @param[in] tdma_sync             TDMA sync object.
 *  @param[in] isi_mitig_pauses      Number of ISI mitigation pauses.
 *  @param[in] preamble_len_reg_val  Preamble length register value.
 *  @param[in] sfd_len_reg_val       SFD length register value.
 */
void link_tdma_set_preamble_length(tdma_sync_t *tdma_sync, uint8_t isi_mitig_pauses, uint32_t preamble_len_reg_val,
                                   sfd_length_t sfd_len_reg_val);

/** @brief Perform modulo on float input.
 *
 *  @param[in] a  Numerator of the modulo operation.
 *  @param[in] b  Denominator of the modulo operation.
 *  @return Modulo result.
 */
static inline float float_mod(float a, float b)
{
    return a - (int)(a / b) * b;
}

/** @brief Get a time stamp in PLL cycles.
 *
 *  @param[in] tdma_sync  TDMA sync object.
 *  @return Time stamp in PLLL cycles.
 */
uint64_t link_tdma_get_time_stamp_pll_cycles(tdma_sync_t *tdma_sync);

/** @brief Get elapsed time in microseconds since provided time stamp.
 *
 *  @param[in] tdma_sync            TDMA sync object.
 *  @param[in] previous_time_stamp  Previous time stamp in PLL cycles.
 *  @return Elapsed time in microseconds.
 */
uint16_t link_tdma_get_elapsed_time_us(tdma_sync_t *tdma_sync, uint64_t previous_time_stamp);

/** @brief Get last timer increment in microseconds.
 *
 *  @param[in] tdma_sync  TDMA sync object.
 *  @return Last timer increment in microseconds.
 */
uint16_t link_tdma_get_last_timer_increment_us(tdma_sync_t *tdma_sync);

/** @brief Update ISI mitigation pauses based on ISI mitigation level.
 *
 *  @param[in] tdma_sync  TDMA sync object.
 *  @param[in] isi_mitig  ISI mitigation level.
 */
void link_tdma_update_isi_mitig_pauses(tdma_sync_t *tdma_sync, isi_mitig_t isi_mitig);

/** @brief Calculate the product of a value with the PLL ratio without loss of precision.
 *
 *  @param[in] value      The value to multiply.
 *  @param[in] chip_rate  The current chip rate used.
 *  @return The multiplied value.
 */
static inline uint32_t mul_xtal_to_pll_ratio(uint32_t value, chip_rate_cfg_t chip_rate)
{
    switch (chip_rate) {
    case CHIP_RATE_20_48_MHZ:
        return value * XTAL_TO_PLL_RATIO(CHIP_RATE_20_48_MHZ);
        break;
    case CHIP_RATE_27_30_MHZ:
        return (value * 2500) / 3; /* Multiply by 833.3333. */
        break;
    case CHIP_RATE_40_96_MHZ:
        return value * XTAL_TO_PLL_RATIO(CHIP_RATE_40_96_MHZ);
        break;
    default:
        while (1);
    }
}

/** @brief Calculate the quotient of a value with the PLL ratio without loss of precision.
 *
 *  @param[in] value      The value to divide.
 *  @param[in] chip_rate  The current chip rate used.
 *  @return The divided value.
 */
static inline uint32_t div_pll_ratio(uint32_t value, chip_rate_cfg_t chip_rate)
{
    switch (chip_rate) {
    case CHIP_RATE_20_48_MHZ:
        return value / (uint32_t)XTAL_TO_PLL_RATIO(CHIP_RATE_20_48_MHZ);
        break;
    case CHIP_RATE_27_30_MHZ:
        value = (value << 1) + value;
        /* We have done x3. */
        return value / 2500; /* Divide by 833.3333. */
        break;
    case CHIP_RATE_40_96_MHZ:
        return value / (uint32_t)XTAL_TO_PLL_RATIO(CHIP_RATE_40_96_MHZ);
        break;
    default:
        while (1);
    }
}

/** @brief Calculate the modulo of a value with the PLL ratio without loss of precision.
 *
 *  @param[in] value      The value to mod.
 *  @param[in] chip_rate  The current chip rate used.
 *  @return The moduloed value.
 */
static inline uint32_t mod_pll_ratio(uint32_t value, chip_rate_cfg_t chip_rate)
{
    switch (chip_rate) {
    case CHIP_RATE_20_48_MHZ:
        return value % (uint32_t)XTAL_TO_PLL_RATIO(CHIP_RATE_20_48_MHZ);
        break;
    case CHIP_RATE_27_30_MHZ:
        value = (value << 1) + value;
        /* We have done x3. */
        return (value % (2500)) / 3; /* This is like doing float_mod with 833.3333. */
        break;
    case CHIP_RATE_40_96_MHZ:
        return value % (uint32_t)XTAL_TO_PLL_RATIO(CHIP_RATE_40_96_MHZ);
        break;
    default:
        while (1);
    }
}

/** @brief Convert Chip Rate Transition Ratio.
 *
 *  @param[in] pll_cycles     The number of PLL cycles to be converted.
 *  @param[in] old_chip_rate  The current chip rate of the provided PLL cycles.
 *  @param[in] new_chip_rate  The target chip rate to convert to.
 *  @return The number of PLL cycles converted from the old chip rate to the new one.
 */
static inline int64_t convert_pll_cycle_base(int64_t pll_cycles, chip_rate_cfg_t old_chip_rate,
                                             chip_rate_cfg_t new_chip_rate)
{
    switch (old_chip_rate) {
    case CHIP_RATE_20_48_MHZ: {
        switch (new_chip_rate) {
        case CHIP_RATE_20_48_MHZ:
            /* Factor = 1. */
            return pll_cycles;
            break;
        case CHIP_RATE_27_30_MHZ:
            /* Factor = 1.33 */
            return (pll_cycles * 4) / 3;
            break;
        case CHIP_RATE_40_96_MHZ:
            /* Factor = 2. */
            return pll_cycles * 2;
            break;
        default:
            while (1);
        }
        break;
    }
    case CHIP_RATE_27_30_MHZ: {
        switch (new_chip_rate) {
        case CHIP_RATE_20_48_MHZ:
            /* Factor = 0.75. */
            return (pll_cycles * 3) / 4;
            break;
        case CHIP_RATE_27_30_MHZ:
            /* Factor = 1. */
            return pll_cycles;
            break;
        case CHIP_RATE_40_96_MHZ:
            /* Factor = 1.5. */
            return (pll_cycles * 3) / 2;
            break;
        default:
            while (1);
        }
        break;
    }
    case CHIP_RATE_40_96_MHZ: {
        switch (new_chip_rate) {
        case CHIP_RATE_20_48_MHZ:
            /* Factor = 0.5. */
            return pll_cycles / 2;
            break;
        case CHIP_RATE_27_30_MHZ:
            /* Factor = 0.6666 */
            return (pll_cycles * 2) / 3;
            break;
        case CHIP_RATE_40_96_MHZ:
            /* Factor = 1. */
            return pll_cycles;
            break;
        default:
            while (1);
        }
        break;
    }
    default:
        while (1);
    }
    return 0;
}

#ifdef __cplusplus
}
#endif
#endif /* LINK_MULTI_RADIO_H_ */
