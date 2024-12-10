/** @file  sac_src_cmsis.h
 *  @brief Sampling rate converter processing stage using the CMSIS DSP software library.
 *
 *  @note This processing stage requires an Arm Cortex-M processor based device.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_SRC_CMSIS_H_
#define SAC_SRC_CMSIS_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "filtering_functions.h"
#include "sac_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief SRC CMSIS Ratio.
 */
typedef enum src_cmsis_ratio {
    /*! Ratio of 1 between original and resulting sampling rate */
    SAC_SRC_ONE = 1,
    /*! Ratio of 2 between original and resulting sampling rate */
    SAC_SRC_TWO = 2,
    /*! Ratio of 3 between original and resulting sampling rate. */
    SAC_SRC_THREE = 3,
    /*! Ratio of 4 between original and resulting sampling rate. */
    SAC_SRC_FOUR = 4,
    /*! Ratio of 6 between original and resulting sampling rate. */
    SAC_SRC_SIX = 6,
} src_cmsis_ratio_t;

/** @brief SRC CMSIS Configuration.
 */
typedef struct src_cmsis_cfg {
    /*! Multiply ratio to use for the SRC interpolation. */
    src_cmsis_ratio_t multiply_ratio;
    /*! Divide ratio to use for the SRC decimation. */
    src_cmsis_ratio_t divide_ratio;
    /*! Size of the payload in bytes expected at input. */
    uint16_t payload_size;
    /*! Audio input sample format. */
    sac_sample_format_t input_sample_format;
    /*! Audio output sample format. */
    sac_sample_format_t output_sample_format;
    /*! Number of channels in audio packet. */
    uint8_t channel_count;
} src_cmsis_cfg_t;

/** @brief SRC CMSIS Instance.
 */
typedef struct src_cmsis_instance {
    /*! SRC CMSIS user configuration. */
    src_cmsis_cfg_t cfg;
    struct {
        /*! Internal: Instances for the arm_fir_interpolation. 1 instance per channel. */
        fir_interpolate_instance_t *interpolate_instances;
        /*! Internal: Instances for the arm_fir_decimation. 1 instance per channel. */
        fir_decimate_instance_t *decimate_instances;
        /*! Internal: Audio buffer to be used between multiply and divide process. */
        uint8_t *multiply_out_buffer;
        /*! Internal: Buffer to accumulate last FIR_NUM_TAPS samples of input payload. */
        uint8_t *discard_accumulator;
        /*! Internal: Size of the discard accumulator buffer. */
        int16_t discard_accumulator_size;
        /*! Internal: True if the discard process is active. */
        bool discard_active;
    } _internal;
} src_cmsis_instance_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the SRC CMSIS processing stage.
 *
 *  @param[in]  instance  SRC CMSIS instance.
 *  @param[in]  name      Processing stage name.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  mem_pool  Memory pool handle.
 *  @param[out] status    Status code.
 */
void sac_src_cmsis_init(void *instance, const char *name, sac_pipeline_t *pipeline, mem_pool_t *mem_pool,
                        sac_status_t *status);

/** @brief Initialize the SRC CMSIS discard processing stage.
 *
 *  @param[in]  instance  SRC CMSIS instance.
 *  @param[in]  name      Processing stage name.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  mem_pool  Memory pool handle.
 *  @param[out] status    Status code.
 */
void sac_src_cmsis_discard_init(void *instance, const char *name, sac_pipeline_t *pipeline, mem_pool_t *mem_pool,
                                sac_status_t *status);

/** @brief Process SRC on an audio packet.
 *
 *  @param[in]  instance  SRC CMSIS instance.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  header    Audio packet's header.
 *  @param[in]  data_in   Audio payload to process.
 *  @param[in]  size      Size in bytes of the audio payload.
 *  @param[out] data_out  Audio payload that has been processed.
 *  @param[out] status    Status code.
 *
 *  @return Size in bytes of the processed samples, 0 if no processing happened
 */
uint16_t sac_src_cmsis_process(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in,
                               uint16_t size, uint8_t *data_out, sac_status_t *status);

/** @brief Discard process for SRC on an audio packet.
 *
 *  @param[in]  instance  SRC CMSIS instance.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  header    Audio packet's header.
 *  @param[in]  data_in   Audio payload to process.
 *  @param[in]  size      Size in bytes of the audio payload.
 *  @param[out] data_out  Audio payload that has been processed.
 *  @param[out] status    Status code.
 *
 *  @return Size in bytes of the processed samples, 0 if no processing happened
 *
 *   @note The current sampling rate conversion solution lacks the capability to use the discard processing function
 *         when configured for non-integer conversion rate. This means that users are unable to adjust the conversion
 *         rate to values that are not whole numbers.
 *
 *         If the user require a conversion rate that is not an integer (e.g., 1.5x), it is not possible to use the
 *         discard function while doing so.
 */
uint16_t sac_src_cmsis_process_discard(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in, uint16_t size,
                                       uint8_t *data_out, sac_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* SAC_SRC_CMSIS_H_ */
