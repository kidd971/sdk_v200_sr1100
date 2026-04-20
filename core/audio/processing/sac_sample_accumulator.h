/** @file  sac_sample_accumulator.h
 *  @brief SPARK Audio Core sample accumulator processing stage.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_SAMPLE_ACCUMULATOR_H_
#define SAC_SAMPLE_ACCUMULATOR_H_

/* INCLUDES *******************************************************************/
#include "sac_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief SPARK Audio Core commands.
 */
typedef enum sac_sample_accumulator_cmd {
    /*! Get the sample accumulator used size. */
    SAC_SAMPLE_ACCUMULATOR_GET_USED_SIZE,
} sac_sample_accumulator_cmd_t;

/** @brief SPARK Audio Core Compression Instance.
 */
typedef struct sac_sample_accumulator_instance {
    /*! Maximum number of bytes to accumulate. */
    uint32_t max_accumulator_size;
    /*! Pointer to function that gets accumulator size*/
    uint32_t (*get_accumulator_size)(sac_pipeline_t *pipeline);
    struct {
        /*! Internal: Accumulator memory. */
        uint8_t *accumulator;
        /*! Internal: Current number of bytes to accumulate */
        uint32_t accumulator_size;
        /*! Internal: Accumulator used memory size in bytes. */
        uint32_t accumulator_used_size;
        /*! Internal: Underflow count tracker to flush accumulator on packet loss. */
        uint32_t underflow_count;
        /*! Internal: Number of packets enqueued on the producer. */
        uint8_t prod_enqueue_count;
    } _internal;
} sac_sample_accumulator_instance_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize sample accumulator process.
 *
 *  @param[in]  instance  Sample accumulator instance.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  mem_pool  Memory pool for memory allocation.
 *  @param[out] status    Status code.
 */
void sac_sample_accumulator_init(void *instance, const char *name, sac_pipeline_t *pipeline, mem_pool_t *mem_pool,
                                 sac_status_t *status);

/** @brief Control the sample accumulator processing stage.
 *
 *  @param[in]  instance  Sample accumulator instance.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  cmd       Command.
 *  @param[in]  arg       Argument.
 *  @param[out] status    Status code.
 *  @return Command specific value.
 */
uint32_t sac_sample_accumulator_ctrl(void *instance, sac_pipeline_t *pipeline, uint8_t cmd, uint32_t arg,
                                     sac_status_t *status);

/** @brief Process accumulated audio samples.
 *
 *  @param[in]  instance  Sample accumulator instance.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  header    SPARK Audio Core header.
 *  @param[in]  data_in   Data in to be processed.
 *  @param[in]  size      Number of bytes to process.
 *  @param[out] data_out  Processed data out.
 *  @param[out] status    Status code.
 *  @return Number of bytes processed.
 */
uint16_t sac_sample_accumulator_process(void *instance, sac_pipeline_t *pipeline, sac_header_t *header,
                                        uint8_t *data_in, uint16_t size, uint8_t *data_out, sac_status_t *status);

/** @brief Flush sample accumulator.
 *
 *  @note This function can be used to flush the accumulator when the link is lost on the receiver. This avoids old
 *        samples being played when the link resumes.
 *
 *  @param[in]  instance  Sample accumulator instance.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  header    SPARK Audio Core header.
 *  @param[in]  data_in   Data in to be processed.
 *  @param[in]  size      Number of bytes to process.
 *  @param[out] data_out  Processed data out.
 *  @param[out] status    Status code.
 *  @return Number of bytes processed.
 */
uint16_t sac_sample_accumulator_flush(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in,
                                      uint16_t size, uint8_t *data_out, sac_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* SAC_SAMPLE_ACCUMULATOR_H_ */
