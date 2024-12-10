/** @file  sac_packing.h
 *  @brief SPARK Audio Core packing/unpacking for 18/20/24 bits audio processing stage.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_PACKING_H_
#define SAC_PACKING_H_

/* INCLUDES *******************************************************************/
#include "sac_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
typedef enum sac_packing_cmd {
    SAC_PACKING_SET_MODE,
    SAC_PACKING_GET_MODE
} sac_packing_cmd_t;

typedef enum sac_packing_mode {
    /*! Packing 32-bit words containing 18-bit audio samples into 18-bit audio samples. */
    SAC_PACK_18BITS,
    /*! Packing 32-bit words containing 20-bit audio samples into 20-bit audio samples. */
    SAC_PACK_20BITS,
    /*! Packing 32-bit words containing 24-bit audio samples into 24-bit audio samples. */
    SAC_PACK_24BITS,
    /*! Packing 32-bit words containing 32-bit audio samples into 24-bit audio samples. */
    SAC_PACK_32BITS_24BITS,
    /*! Packing 32-bit words containing 20-bit audio samples into 16-bit audio samples. */
    SAC_PACK_20BITS_16BITS,
    /*! Packing 32-bit words containing 24-bit audio samples into 16-bit audio samples. */
    SAC_PACK_24BITS_16BITS,
    /*! Scale packed 24-bit audio samples into packed 16-bit audio samples. */
    SAC_SCALE_24BITS_16BITS,
    /*! Unpacking 18-bit audio samples into 32-bit words containing 18-bit audio. */
    SAC_UNPACK_18BITS,
    /*! Unpacking 20-bit audio samples into 32-bit words containing 20-bit audio. */
    SAC_UNPACK_20BITS,
    /*! Unpacking 24-bit audio samples into 32-bit words containing 24-bit audio. */
    SAC_UNPACK_24BITS,
    /*! Unpacking 16-bit audio samples into 32-bit words containing 20-bit audio. */
    SAC_UNPACK_20BITS_16BITS,
    /*! Unpacking 16-bit audio samples into 32-bit words containing 24-bit audio. */
    SAC_UNPACK_24BITS_16BITS,
    /*! Extend 18-bit value's sign bit into 32-bit word. */
    SAC_EXTEND_18BITS,
    /*! Extend 20-bit value's sign bit into 32-bit word. */
    SAC_EXTEND_20BITS,
    /*! Extend 24-bit value's sign bit into 32-bit word. */
    SAC_EXTEND_24BITS,
} sac_packing_mode_t;

typedef struct sac_packing_instance {
    sac_packing_mode_t packing_mode;
} sac_packing_instance_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize packing process.
 *
 *  @param[in]  instance  Packing instance.
 *  @param[in]  name      Processing stage name.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  mem_pool  Memory pool handle.
 *  @param[out] status    Status code.
 */
void sac_packing_init(void *instance, const char *name, sac_pipeline_t *pipeline, mem_pool_t *mem_pool,
                      sac_status_t *status);

/** @brief SPARK Audio Core packing control function.
 *
 *  @param[in]  instance  Packing instance.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  cmd       Command.
 *  @param[in]  args      Argument.
 *  @param[out] status    Status code.
 *
 *  @return Command specific value.
 */
uint32_t sac_packing_ctrl(void *instance, sac_pipeline_t *pipeline, uint8_t cmd, uint32_t arg, sac_status_t *status);

/** @brief Process audio samples packing.
 *
 *  @param[in]  instance  Packing instance.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  header    Audio packet's header.
 *  @param[in]  data_in   Audio payload to process.
 *  @param[in]  size      Size in bytes of the audio payload.
 *  @param[out] data_out  Audio payload that has been processed.
 *  @param[out] status    Status code.
 *
 *  @return Size in bytes of the processed samples, 0 if no processing happened.
 */
uint16_t sac_packing_process(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in,
                             uint16_t size, uint8_t *data_out, sac_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* SAC_PACKING_H_ */
