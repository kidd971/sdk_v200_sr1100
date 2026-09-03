/** @file  puretone_unidirectional_facade.h
 *  @brief Facades for low-level platform-specific features required by the puretone unidirectional example.
 *
 *  @note This header defines the interfaces for various hardware features used by the puretone unidirectional example.
 *
 *  These facades abstract the underlying platform-specific implementations of features like SPI communication, IRQ
 *  handling, timer functions, and context switching mechanisms. The actual implementations are selected at compile time
 *  based on the target platform, allowing for flexibility and portability across different hardware. The facade is
 *  designed to be a compile-time dependency only, with no support for runtime polymorphism. This ensures tight
 *  integration with the build system and minimal overhead.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef PURETONE_UNIDIRECTIONAL_FACADE_H_
#define PURETONE_UNIDIRECTIONAL_FACADE_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "common_facade.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Certification modes.
 */
typedef enum facade_certification_mode {
    /*! No certification mode. */
    FACADE_CERTIF_NONE,
    /*! Audio certification mode at 96kHz 24-bit. */
    FACADE_CERTIF_AUDIO_96k_24_BIT,
    /*! Audio certification mode at 48kHz 24-bit. */
    FACADE_CERTIF_AUDIO_48k_24_BIT,
    /*! Audio certification mode at 48kHz 16-bit. */
    FACADE_CERTIF_AUDIO_48k_16_BIT,
    /*! Audio certification mode at 48kHz ADPCM. */
    FACADE_CERTIF_AUDIO_48k_ADPCM,
    /*! Data certification mode. */
    FACADE_CERTIF_DATA,
} facade_certification_mode_t;

/** @brief Function callbacks for button presses.
 */
typedef struct facade_button_callbacks {
    /*! Function callback to pair/unpair the device. */
    void (*pairing_callback)(void);
    /*! Function callback to cycle through forced fallback states. */
    void (*fallback_callback)(void);
    /*! Function callback to increase the playback volume. */
    void (*volume_up_callback)(void);
    /*! Function callback to decrease the playback volume. */
    void (*volume_down_callback)(void);
} facade_button_callbacks_t;

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Initialize the Coordinator's audio peripherals.
 *
 *  @note Configure the serial audio interface to Mono or Stereo.
 */
void facade_audio_coord_init(void);

/** @brief Initialize the Node's audio peripherals.
 *
 *  @note Configure the serial audio interface to Mono or Stereo.
 */
void facade_audio_node_init(void);

/** @brief Deinitialize the audio peripherals.
 */
void facade_audio_deinit(void);

/** @brief Set the serial audio interface transfer complete callbacks.
 *
 *  @note Set NULL in place of unused callback.
 *
 *  @param[in] tx_callback  Audio i2s tx complete callback.
 *  @param[in] rx_callback  Audio i2s rx complete callback.
 */
void facade_set_audio_complete_callback(void (*tx_callback)(void), void (*rx_callback)(void));

/** @brief Set button function callbacks.
 *
 *  @param[in] button_callbacks  Button function callback structure.
 */
void facade_set_button_callbacks(facade_button_callbacks_t button_callbacks);

/** @brief Poll for button presses and execute function callback.
 */
void facade_button_handling(void);

/** @brief Read button state to define if certification mode for the Coordinator is required.
 *
 *  @return The certification mode to be applied.
 */
facade_certification_mode_t facade_get_coord_certification_mode(void);

/** @brief Read button state to define if certification mode for the Node is required.
 *
 *  @return The certification mode to be applied.
 */
facade_certification_mode_t facade_get_node_certification_mode(void);

/** @brief Notify user of the wireless Audio TX connection status.
 *
 *  @note This function is intended only for the Coordinator, which is responsible for sending audio packets.
 */
void facade_tx_audio_conn_status(void);

/** @brief Notify user of the wireless Data TX connection status.
 */
void facade_tx_data_conn_status(void);

/** @brief Notify user of the wireless Audio RX connection status.
 *
 *  @note This function is intended only for the Node, which is responsible for receiving audio packets.
 */
void facade_rx_audio_conn_status(void);

/** @brief Notify user of the wireless Data RX connection status.
 */
void facade_rx_data_conn_status(void);

/** @brief Notify user of the fallback status.
 *
 *  @param[in] on  Fallback status indicator.
 */
void facade_fallback_status(bool on);

/** @brief Initialize the audio process timer.
 *
 *  @param[in] callback  Callback function to execute on timer event.
 */
void facade_audio_process_timer_init(void (*callback)(void));

/** @brief Start the audio process timer.
 */
void facade_audio_process_timer_start(void);

/** @brief Generate an event for the audio process timer.
 */
void facade_audio_process_timer_trigger(void);

/** @brief Stop the audio process timer.
 */
void facade_audio_process_timer_stop(void);

/** @brief Initialize and set the data timer period which include statistics and data transmitted to the other device.
 *
 *  @param[in] period_ms  Timer period in ms.
 */
void facade_data_timer_init(uint32_t period_ms);

/** @brief Set the data timer callback.
 *
 *  @param[in] callback  Callback when timer expires.
 */
void facade_data_timer_set_callback(void (*callback)(void));

/** @brief Start the data timer.
 */
void facade_data_timer_start(void);

/** @brief Stop the data timer.
 */
void facade_data_timer_stop(void);

/** @brief Notify user of payload present in frame.
 */
void facade_payload_received_status(void);

/** @brief Notify user of no payload present in frame.
 */
void facade_empty_payload_received_status(void);

/** @brief Read the state of the button that will set the other device's LED state.
 *
 *  @return Returns true if the button is pressed, false otherwise.
 */
bool facade_read_button_state(void);

#if USB_AUDIO_ENABLED
/** @brief Configure the coordinator's USB audio.
 */
void facade_configure_coord_usb_audio(void);

/** @brief Configure the node's USB audio.
 */
void facade_configure_node_usb_audio(void);

/** @brief Get the current number of samples in the Node's USB audio TX fifo.
 *
 *  @return Number of samples in the USB audio TX fifo.
 */
uint32_t facade_get_node_usb_audio_tx_fifo_sample_count(void);

/** @brief Get the remaining number of bytes in the IN endpoint fifo.
 *
 *  @return The remaining number of bytes in the IN endpoint fifo.
 */
uint32_t facade_app_audio_usb_get_epin_fifo_remaining(void);

/** @brief Set the target fifo size for the input endpoint.
 *
 *  @param[in] target_fifo_size  Target fifo size in bytes.
 */
void facade_app_audio_usb_set_epin_target_fifo_size(uint16_t target_fifo_size);
#endif

/* **** Diagnostics ****
 *
 * All four report state the BSP keeps regardless of whether anyone reads it, and all four return
 * false on boards that do not keep it, leaving the outputs untouched -- so a caller can ignore the
 * return and print zeros, or use it to omit the line entirely.
 */

/** @brief Read the per-radio IRQ and DMA counters.
 *
 *  One radio's counters freezing while the other keeps moving is the dual-radio wedge signature,
 *  and it cannot be seen in the packet statistics: the connection simply goes quiet either way.
 *
 *  @param[out] r1_irq  Radio 1 interrupt count.
 *  @param[out] r2_irq  Radio 2 interrupt count.
 *  @param[out] r1_dma  Radio 1 DMA transfer count.
 *  @param[out] r2_dma  Radio 2 DMA transfer count.
 *  @retval true   Counters available on this board.
 *  @retval false  Board keeps no per-radio counters; outputs untouched.
 */
bool facade_get_radio_hw_counters(uint32_t *r1_irq, uint32_t *r2_irq, uint32_t *r1_dma, uint32_t *r2_dma);

/** @brief Read the scheduler liveness signals.
 *
 *  Compare across two reads: mrt frozen means the wireless core's scheduler stopped; mrt moving
 *  while the radio counters are frozen means the scheduler is alive but the radios are not being
 *  serviced. The two cases have different causes and different fixes.
 *
 *  @param[out] mrt   Multi-radio (TIM4) scheduler tick count.
 *  @param[out] frt   Free-running millisecond tick. Same counter facade_get_tick_ms() returns.
 *  @param[out] irq1  Radio 1 IRQ pin level.
 *  @param[out] irq2  Radio 2 IRQ pin level.
 *  @retval true   Signals available on this board.
 *  @retval false  Board exposes none; outputs untouched.
 */
bool facade_get_sched_liveness(uint32_t *mrt, uint32_t *frt, bool *irq1, bool *irq2);

/** @brief Read the raw multi-radio scheduler timer registers, to pin down WHY mrt froze.
 *
 *  cen=0 means the timer was stopped; cen=1 with arr=0 means the period was programmed to zero
 *  and the timer stalled; cen=1 with a sane arr and a counter that advances means the timer is
 *  not the problem. A single-radio board never starts this timer, so all-zero is normal there.
 *
 *  @param[out] cr1   Control register 1.
 *  @param[out] arr   Auto-reload register.
 *  @param[out] cnt   Counter register.
 *  @param[out] dier  DMA/interrupt enable register.
 *  @retval true   Timer readable on this board.
 *  @retval false  No such timer here; outputs untouched.
 */
bool facade_get_multi_radio_timer_regs(uint32_t *cr1, uint32_t *arr, uint32_t *cnt, uint32_t *dier);

/** @brief Read the last captured HardFault register snapshot.
 *
 *  All-zero means no HardFault has been taken, which is the normal case. A non-zero cfsr says the
 *  board faulted rather than hanging in a while(1) somewhere -- the two look identical from the
 *  outside and are worth separating before looking for a cause. Cross-reference pc and lr against
 *  the .elf.
 *
 *  @param[out] cfsr  Configurable Fault Status Register at the time of the fault.
 *  @param[out] hfsr  HardFault Status Register.
 *  @param[out] pc    Program counter of the faulting instruction.
 *  @param[out] lr    Link register at the fault.
 *  @retval true   Snapshot available on this board.
 *  @retval false  Board captures none; outputs untouched.
 */
bool facade_get_hardfault_snapshot(uint32_t *cfsr, uint32_t *hfsr, uint32_t *pc, uint32_t *lr);

/** @brief Set the I2S MUX selection.
 *
 *  Drives the board's MUX_SEL line, which decides whether the SAI reaches the on-board codec or
 *  the external codec header. Only u5a5 brings that line out; on u535 the pin is not connected
 *  and the BSP call underneath is a documented no-op, so this is safe to wire on both.
 *
 *  @param[in] use_ext  true = external codec port, false = on-board codec.
 */
void facade_set_i2s_mux(bool use_ext);

#ifdef __cplusplus
}
#endif

#endif /* PURETONE_UNIDIRECTIONAL_FACADE_H_ */
