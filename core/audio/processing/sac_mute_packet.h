/** @file  sac_mute_packet.h
 *  @brief Processing stage used to avoid sending packets full of samples with a numerical value of zero.
 *
 *  @note This processing stage should be the last processing stage on an audio transmitting pipeline
 *        and the first processing stage on an audio receiving pipeline.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_MUTE_PACKET_H_
#define SAC_MUTE_PACKET_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "sac_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Mute Packet Instance.
 */
typedef struct sac_mute_packet_instance {
    /*! Set to true if instantiated for an audio transmitting pipeline. */
    bool is_tx;
} sac_mute_packet_instance_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Process the mute packet processing stage.
 *
 *  @param[in]  instance  Process instance.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  header    Audio packet's header.
 *  @param[in]  data_in   Audio payload to process.
 *  @param[in]  size      Size in bytes of the audio payload.
 *  @param[out] data_out  Audio payload that has been processed.
 *  @param[out] status    Status code.
 *  @return Size in bytes of the processed samples, 0 if no processing happened.
 */
uint16_t sac_mute_packet_process(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in,
                                 uint16_t size, uint8_t *data_out, sac_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* SAC_MUTE_PACKET_H_ */
