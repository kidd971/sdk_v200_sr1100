/** @file  resampling.h
 *  @brief This file contains all the functions prototypes for the resampling module.
 *
 *  How to use the module :
 *      Set all the flags according to your use
 *      Initialize all your instances once by launching the resampling_init() function.
 *      Use the resampling_start() function to begin to resample frames.
 *      Use the resample() function to copy a certain amount of samples from an input buffer into an output buffer.
 *      Use the resample_get_state() to get the current state of the resampling module.
 *
 *  @copyright Copyright (C) 2019 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef RESAMPLING_H_
#define RESAMPLING_H_

/* INCLUDES *******************************************************************/
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/* TODO this number can be more of 2 */
#define RESAMPLING_CFG_MAX_NB_CHANNEL 2
#define LAST_SAMPLE_AMT               2
#define LAST_SAMPLE_ARRAY_SIZE        (LAST_SAMPLE_AMT * RESAMPLING_CFG_MAX_NB_CHANNEL) /* [Samp-2][Samp-1] */

/* TYPES **********************************************************************/
/** @brief Resampling Errors Codes.
 *
 *  This enum contains all the errors returned by this library.
 */
typedef enum resampling_errors {
    RESAMPLING_NO_ERROR = 0,
    RESAMPLING_INVALID_TYPE = -1,
    RESAMPLING_INVALID_NB_CHANNEL = -2,
} resampling_errors_t;

/** @brief Resampling Buffer Types.
 *
 *  This enum contains all the sample bit depth supported by this library.
 */
typedef enum resampling_buffer_type {
    BUFFER_8BITS  = 7,
    BUFFER_16BITS = 15,
    BUFFER_20BITS = 19,
    BUFFER_24BITS = 23,
    BUFFER_32BITS = 31
} resampling_buffer_type_t;

/** @brief Resampling Correction Modes.
 *
 *  This enum contains all the correction modes for this library.
 */
typedef enum resampling_correction {
    RESAMPLING_NO_CORRECTION,
    RESAMPLING_ADD_SAMPLE,
    RESAMPLING_REMOVE_SAMPLE,
} resampling_correction_t;

/** @brief Resampling Instance Status.
 *
 *  This enum contains all states for this library.
 */
typedef enum resampling_status {
    RESAMPLING_WAIT_QUEUE_FULL,
    RESAMPLING_IDLE,
    RESAMPLING_START,
    RESAMPLING_RUNNING
} resampling_status_t;

/** @brief Resampling library configuration structure.
 *
 *  Variables within this structure can be set by the user to configure the resampling instance.
 */
typedef struct resampling_config {
    uint16_t nb_sample;
    resampling_buffer_type_t buffer_type;
    uint16_t resampling_length;
    uint8_t nb_channel;
} resampling_config_t;

/** @brief Resampling library instance structure.
 *
 *  Variables within this structure will be set by the library in the init function.
 */
typedef struct resampling_instance {
    resampling_status_t status;
    resampling_correction_t correction;
    resampling_buffer_type_t buffer_type;
    uint32_t buffer_type_max;
    int32_t last_sample[LAST_SAMPLE_ARRAY_SIZE];
    uint32_t step_add;
    uint32_t step_rem;
    uint32_t bias;
    uint32_t bias_step_add;
    uint32_t bias_step_rem;
    int64_t x_axis;
    uint8_t nb_channel;
    uint32_t max_x_axis;
} resampling_instance_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the resampling struct that correspond to the instance.
 *
 *  @param[in] instance           Structure instance pointer.
 *  @param[in] resampling_config  Pointer to the instance's configuration structure.
 *  @return resampling_errors_t  Resampling error code.
 */
resampling_errors_t resampling_init(resampling_instance_t *instance, resampling_config_t *resampling_config);

/** @brief Start to resample the signal(s).
 *
 *  @param[in] instance    Structure instance pointer.
 *  @param[in] correction  ADD_SAMPLE or REMOVE_SAMPLE.
 */
void resampling_start(resampling_instance_t *instance, resampling_correction_t correction);

/** @brief resample the signal if STARTED.
 *
 *  @param[in] instance       Structure instance pointer.
 *  @param[in] ptr_input      Pointer to input data.
 *  @param[out] ptr_output    Pointer to output data.
 *  @param[in] sample_count   Amount of samples to be treated.
 *  @return Samples count.
 */
uint16_t resample(resampling_instance_t *instance, void *ptr_input, void *ptr_output, uint16_t sample_count);

/** @brief Return the resampling status.
 *
 *  @param[in] instance  IStructure instance pointer.
 *  @return resampling_status_t  Resampling status.
 */
resampling_status_t resample_get_state(resampling_instance_t *instance);

/** @brief Return the resampling channel count.
 *
 *  @param[in] instance  Structure instance pointer.
 *  @return Number of channels the instance is configured for.
 */
uint8_t resample_get_channel_count(resampling_instance_t *instance);

#ifdef __cplusplus
}
#endif

#endif /* RESAMPLING_H_ */
