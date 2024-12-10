/** @file  sac_volume.h
 *  @brief SPARK Audio Core processing functions related to the software volume control.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_VOLUME_H_
#define SAC_VOLUME_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "sac_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/*! Maximum possible value for the audio volume. */
#define SAC_VOLUME_MAX  1
/*! Minimum possible value for the audio volume. */
#define SAC_VOLUME_MIN  0
/*! Step value to use when gradually increasing the volume towards the desired value. */
#define SAC_VOLUME_GRAD 0.0003
/*! Step value to use when increasing or decreasing the volume. */
#define SAC_VOLUME_TICK 0.1

/* TYPES **********************************************************************/
/** @brief Volume Commands.
 */
typedef enum sac_volume_cmd {
    /*! Increase the volume by one tick. */
    SAC_VOLUME_INCREASE,
    /*! Decrease the volume by one tick. */
    SAC_VOLUME_DECREASE,
    /*! Set the volume to 0. */
    SAC_VOLUME_MUTE,
    /*! Get the current volume value (between 0 and 10000). */
    SAC_VOLUME_GET_FACTOR,
} sac_volume_cmd_t;

/** @brief Volume Instance.
 */
typedef struct sac_volume_instance {
    /*! Audio sample format. */
    sac_sample_format_t sample_format;
    /*! Initial volume level from 0 to 100. */
    uint8_t initial_volume_level;
    struct {
        /*! Internal: Factor used for calculation. */
        float volume_factor;
        /*! Internal: Threshold set by user that _volume_factor will tend towards. */
        float volume_threshold;
    } _internal;
} sac_volume_instance_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the digital volume control processing stage.
 *
 *  @param[in]  instance  Volume control instance.
 *  @param[in]  name      Name of the processing stage.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  mem_pool  Memory pool for memory allocation.
 *  @param[out] status    Status code.
 */
void sac_volume_init(void *instance, const char *name, sac_pipeline_t *pipeline, mem_pool_t *mem_pool,
                     sac_status_t *status);

/** @brief Process volume on each audio sample.
 *
 *  @param[in]  volume       Volume instance.
 *  @param[in]  header       Audio header.
 *  @param[in]  data_in      Data in to be processed.
 *  @param[in]  bytes_count  Number of bytes to process.
 *  @param[out] data_out     Processed samples out.
 *  @param[out] status       Status code.
 *  @return Number of samples processed. Return 0 if no samples processed.
 */
uint16_t sac_volume_process(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in,
                            uint16_t size, uint8_t *data_out, sac_status_t *status);

/** @brief Volume Control function.
 *
 *  @param[in]  volume  Volume instance.
 *  @param[in]  cmd     Control command.
 *  @param[in]  arg     Control argument.
 *  @param[out] status  Status code.
 *  @return Value returned dependent on command.
 */
uint32_t sac_volume_ctrl(void *instance, sac_pipeline_t *pipeline, uint8_t cmd, uint32_t arg, sac_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* SAC_VOLUME_H_ */
