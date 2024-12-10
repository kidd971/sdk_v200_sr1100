/** @file  wps_utils.h
 *  @brief wps_utils brief
 *
 *  wps_utils description
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef WPS_UTILS_H_
#define WPS_UTILS_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Find the Greatest Common Divider(GCD) between 2 numbers.
 *
 *  @note For example, with 60 / 100, the GCD is 20. The reduced
 *        fraction is then 3/5.
 *
 *  @param[in] number1  First number.
 *  @param[in] number2  Second number.
 *  @return Greatest common divider.
 */
int wps_utils_gcd(int number1, int number2);

/** @brief Get the maximum time to delay a wakeup event when no node is available in queue.
 *
 *  @param[in] preamble_bits     Number of bits for the Preamble (default: 1020 - 94, 1120 - 210).
 *  @param[in] syncword_bits     Number of bits for the Syncword (default: 1020 - 32, 1120 - 64).
 *  @param[in] iook              True: Using modulation IOOK - False: Not using modulation IOOK.
 *  @param[in] fec               FEC level, from 0 to 7.
 *  @param[in] mod_2bitppm       True: Using modulation 2BITPPM - False: Not using modulation 2BITPPM.
 *  @param[in] chip_repet        Number of chip repetition, from 1 to 4
 *  @param[in] isi_mitig         ISI mitigation level, from 0 to 3
 *  @param[in] address_bits      Number of bits for the address field (default: 16).
 *  @param[in] total_frame_size  Total main frame size, including all headers and payload.
 *  @param[in] crc_bits          Number of CRC bits.
 *  @param[in] cca_delay_pll     CCA delay between attempts, in radio clock cycle.
 *  @param[in] cca_retry         Number of CCA attempts.
 *  @param[in] ack               True: Ack is present - False: Ack is not present.
 *  @param[in] ack_payload_size  Total ack payload size.
 *  @return Estimated airtime, in radio clock cycle.
 */
uint32_t wps_utils_get_delayed_wakeup_event(uint32_t preamble_bits, uint32_t syncword_bits, bool iook, uint8_t fec,
                                            bool mod_2bitppm, uint8_t chip_repet, uint8_t isi_mitig,
                                            uint8_t address_bits, uint32_t total_frame_size, uint32_t crc_bits,
                                            uint32_t cca_delay_pll, uint32_t cca_retry, bool ack,
                                            uint8_t ack_payload_size);

#ifdef __cplusplus
}
#endif

#endif /* WPS_UTILS_H_ */
