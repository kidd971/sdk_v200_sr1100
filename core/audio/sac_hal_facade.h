/** @file  sac_hal_facade.h
 *
 *  @brief Facades for low-level platform-specific features required by SPARK Audio Core.
 *
 *  @note This header defines the interfaces for various hardware features used by the
 *  SPARK Audio Core library. The actual implementations are selected at compile time
 *  based on the target platform, allowing for flexibility and portability across
 *  different hardware.
 *
 *  The facade is designed to be a compile-time dependency only, with no
 *  support for runtime polymorphism. This ensures tight integration with the
 *  build system and minimal overhead.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_HAL_FACADE_H_
#define SAC_HAL_FACADE_H_

/* INCLUDES *******************************************************************/
#include "sac_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the Audio Core HAL.
 *
 *  @param[out] hal  Audio Core HAL.
 */
void sac_facade_hal_init(sac_hal_t *hal);

/** @brief Initialize the codec audio endpoint interfaces.
 *
 *  @param[out] codec_producer_iface  Codec producer audio endpoint interface.
 *  @param[out] codec_consumer_iface  Codec consumer audio endpoint interface.
 */
void sac_facade_codec_endpoint_init(sac_endpoint_interface_t *codec_producer_iface,
                                    sac_endpoint_interface_t *codec_consumer_iface);

/** @brief Initialize the Clock Drift Compensation processing stage.
 *
 *  @param[in]  format  Audio sample format used in the CDC processing.
 *  @param[out] status  Status code.
 *  @return Reference to the initialized CDC processing stage.
 */
sac_processing_t *sac_facade_cdc_processing_init(sac_sample_format_t format, sac_status_t *status);

/** @brief Format the Clock Drift Compensation statistics as a string of characters.
 *
 *  @param[out] buffer  Buffer where to put the formatted string.
 *  @param[in]  size    Size of the buffer.
 *  @return The formatted string length, excluding the NULL terminator.
 */
int sac_facade_cdc_format_stats(char *buffer, uint16_t size);

#ifdef __cplusplus
}
#endif

#endif /* SAC_HAL_FACADE_H_ */
