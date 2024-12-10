/** @file  wps_utils.c
 *  @brief WPS utility function.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "wps_utils.h"

/* TYPES **********************************************************************/
#define IOOK_FLIP_SYMBOL      4
#define IOOK_FLIP_SYMBOL      4
#define TWO_BIT_PPM_MODIFIER  2
#define OTHER_MOD_MODIFIER    1
#define SIZE_SYMBOL           8

#if SR1100
#define FEC_TRAIL_SYMBOL      4
#define ACK_TURNAROUND_SYMBOL 50
#define FEC_DIVIDER           8
#define RETRY_COUNT_SYMBOL    8
const uint8_t fec_multiplier[] = {8, 10, 11, 12, 13, 14, 15, 16};
#else
#define FEC_TRAIL_SYMBOL      3
#define ACK_TURNAROUND_SYMBOL 16
#define FEC_DIVIDER           3
const uint8_t fec_multiplier[] = {3, 4, 5, 6};
#endif

/* PUBLIC FUNCTIONS ***********************************************************/
int wps_utils_gcd(int number1, int number2)
{
    int latest_remainder = number2; /* Remainder k - 1 */
    int predecesor       = number1; /* Remainder k - 2 */
    int temp_remainder;

    while (latest_remainder) {
        temp_remainder   = latest_remainder;
        latest_remainder = predecesor % temp_remainder;
        predecesor       = temp_remainder;
    }

    return predecesor;
}

uint32_t wps_utils_get_delayed_wakeup_event(uint32_t preamble_bits, uint32_t syncword_bits, bool iook, uint8_t fec,
                                            bool mod_2bitppm, uint8_t chip_repet, uint8_t isi_mitig,
                                            uint8_t address_bits, uint32_t total_frame_size, uint32_t crc_bits,
                                            uint32_t cca_delay_pll, uint32_t cca_retry, bool ack,
                                            uint8_t ack_payload_size)
{
#if SR1100
    (void)iook;
    uint32_t main_frame_symbol = (address_bits + RETRY_COUNT_SYMBOL + SIZE_SYMBOL + (total_frame_size * 8) + crc_bits +
                                  FEC_TRAIL_SYMBOL);
    uint32_t ack_frame_symbol = (address_bits + SIZE_SYMBOL + (ack_payload_size * 8) + crc_bits + FEC_TRAIL_SYMBOL);
    uint32_t modulation_modifier = ((mod_2bitppm) ? TWO_BIT_PPM_MODIFIER : OTHER_MOD_MODIFIER);
    uint8_t isi_mitig_multiplier = (isi_mitig < 2) ? 0 : isi_mitig - 1;
    uint32_t main_frame_clock_cycle = preamble_bits + syncword_bits + ((iook) ? IOOK_FLIP_SYMBOL : 0) +
                                      (main_frame_symbol * modulation_modifier * fec_multiplier[fec] * chip_repet) /
                                          FEC_DIVIDER +
                                      ((main_frame_symbol)*isi_mitig_multiplier);
    uint32_t ack_frame_clock_cycle = preamble_bits + syncword_bits + ((iook) ? IOOK_FLIP_SYMBOL : 0) +
                                     (ack_frame_symbol * modulation_modifier * fec_multiplier[fec] * chip_repet) /
                                         FEC_DIVIDER +
                                     ((ack_frame_symbol)*isi_mitig_multiplier);
    uint32_t cca_clock_cycle = (cca_delay_pll * (cca_retry - 1));

    if (ack) {
        return main_frame_clock_cycle + ACK_TURNAROUND_SYMBOL + ack_frame_clock_cycle + cca_clock_cycle;
    } else {
        return main_frame_clock_cycle + cca_clock_cycle;
    }
#else
    (void)chip_repet;
    (void)isi_mitig;

    uint32_t main_frame_symbol = (address_bits + SIZE_SYMBOL + (total_frame_size * 8) + crc_bits + FEC_TRAIL_SYMBOL) *
                                 fec_multiplier[fec] * ((mod_2bitppm) ? TWO_BIT_PPM_MODIFIER : OTHER_MOD_MODIFIER) /
                                 FEC_DIVIDER;
    uint32_t ack_frame_symbol = (address_bits + SIZE_SYMBOL + (ack_payload_size * 8) + crc_bits + FEC_TRAIL_SYMBOL) *
                                fec_multiplier[fec] * ((mod_2bitppm) ? TWO_BIT_PPM_MODIFIER : OTHER_MOD_MODIFIER) /
                                FEC_DIVIDER;
    if (ack) {
        return (preamble_bits + syncword_bits + ((iook) ? IOOK_FLIP_SYMBOL : 0) + main_frame_symbol) +
               (cca_delay_pll * (cca_retry - 1)) + ACK_TURNAROUND_SYMBOL +
               (preamble_bits + syncword_bits + ((iook) ? IOOK_FLIP_SYMBOL : 0) + ack_frame_symbol);
    } else {
        return (preamble_bits + syncword_bits + ((iook) ? IOOK_FLIP_SYMBOL : 0) + main_frame_symbol) +
               (cca_delay_pll * (cca_retry - 1));
    }
#endif
}
