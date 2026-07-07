/** @file  sac_sinus_endpoint_96k.h
 *  @brief SPARK Audio Core endpoint that produces a 1 kHz sine wave at 96 kHz / 24-bit stereo.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_SINUS_ENDPOINT_96K_H_
#define SAC_SINUS_ENDPOINT_96K_H_

/* INCLUDES *******************************************************************/
#include "sac_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Fill a buffer with right-justified (bits[23:0], no <<8) 1 kHz sine, stereo.
 *
 *  Used by SINE_INJECT_DG / SINE_INJECT_HS: overwrite a DMA-read/TX I2S buffer with sine while
 *  the real external I2S clock remains the timing source. Same format as real audio.
 *
 *  @param[out] samples  Buffer to fill.
 *  @param[in]  size     Size in bytes.
 */
void ep_sinus_96k_fill_rj(uint8_t *samples, uint16_t size);

#ifdef __cplusplus
}
#endif

#endif /* SAC_SINUS_ENDPOINT_96K_H_ */
