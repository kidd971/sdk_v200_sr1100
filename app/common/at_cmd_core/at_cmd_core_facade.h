/** @file  at_cmd_core_facade.h
 *  @brief Hardware interface required by the AT command core module.
 *
 *  Each application backend must implement these four functions.
 *  The expansion UART (USART2, PA2=TX / PA3=RX) is used for AT communication
 *  with an external MCU; it is independent from the STLink debug log UART.
 */
#ifndef AT_CMD_CORE_FACADE_H_
#define AT_CMD_CORE_FACADE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Initialize the expansion UART (USART2, PA2=TX / PA3=RX).
 *
 *  @param[in] baud_rate  Baud rate (e.g. 115200).
 */
void facade_expansion_uart_init(uint32_t baud_rate);

/** @brief Queue a null-terminated string on the expansion UART (NON-BLOCKING).
 *
 *  Returns as soon as the bytes are in the driver's TX FIFO; the UART interrupt puts them on
 *  the wire. This must not block, because +EVENT lines are emitted from the wireless RX
 *  callback, which runs in PendSV -- above the audio process timers. A blocking transmit
 *  there stalled the audio pipeline for the length of the line (~2.3 ms at 115200), which was
 *  audible at one vendor command per second.
 *
 *  A whole string is queued atomically with respect to interrupts, so two writers cannot
 *  interleave mid-line.
 *
 *  @param[in] string  Null-terminated string to transmit.
 */
void facade_expansion_uart_write(char *string);

/** @brief Wait until everything queued by facade_expansion_uart_write() is on the wire.
 *
 *  The ONLY legitimate use is immediately before the module stops running -- system reset,
 *  radio shutdown, entering standby -- where the queued bytes would otherwise be discarded
 *  with the rest of the state and the host would see the UART simply fall silent. Bounded by
 *  an internal timeout so a wedged peripheral cannot hang the shutdown path.
 *
 *  Anywhere else this reintroduces exactly the stall the non-blocking write exists to avoid.
 */
void facade_expansion_uart_flush(void);

/** @brief Read one byte from the expansion UART RX FIFO (non-blocking).
 *
 *  @return The received byte (1-255), or 0 if the FIFO is empty.
 */
uint8_t facade_expansion_uart_read_byte(void);

/** @brief Get the current system tick in milliseconds.
 *
 *  @return Current tick value in milliseconds.
 */
uint32_t facade_get_tick_ms(void);

/** @brief Perform a system reset (MCU reboot).
 *
 *  Called after the AT response has been flushed. Does not return.
 */
void facade_system_reset(void);

/** @brief Assert the hardware shutdown pin(s) on the UWB radio(s).
 *
 *  Puts the UWB transceiver(s) into their hardware shutdown state.
 *  Called after the AT response has been flushed and any app-level
 *  cleanup (swc_disconnect, audio stop) has completed.
 */
void facade_uwb_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* AT_CMD_CORE_FACADE_H_ */
