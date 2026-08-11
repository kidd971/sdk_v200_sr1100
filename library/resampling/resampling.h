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
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
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
/* TODO this number can be more of 2. */
#define RESAMPLING_CFG_MAX_NB_CHANNEL 2
#define LAST_SAMPLE_AMT               2
#define LAST_SAMPLE_ARRAY_SIZE        (LAST_SAMPLE_AMT * RESAMPLING_CFG_MAX_NB_CHANNEL) /* [Samp-2][Samp-1] */

/* TYPES **********************************************************************/
/** @brief Resampling Errors Codes.
 *
 *  @note This enum contains all the errors returned by this library.
 */
typedef enum resampling_errors {
    /*! No error. */
    RESAMPLING_NO_ERROR = 0,
    /*! Invalid buffer type. */
    RESAMPLING_INVALID_TYPE = -1,
    /*! Invalid number of channels. */
    RESAMPLING_INVALID_NB_CHANNEL = -2,
} resampling_errors_t;

/** @brief Resampling Buffer Types.
 *
 *  @note This enum contains all the sample bit depth supported by this library.
 */
typedef enum resampling_buffer_type {
    /*! 8-bit samples. */
    BUFFER_8BITS = 7,
    /*! 16-bit samples. */
    BUFFER_16BITS = 15,
    /*! 20-bit samples. */
    BUFFER_20BITS = 19,
    /*! 24-bit samples. */
    BUFFER_24BITS = 23,
    /*! 32-bit samples. */
    BUFFER_32BITS = 31
} resampling_buffer_type_t;

/** @brief Resampling Correction Modes.
 *
 *  @note This enum contains all the correction modes for this library.
 */
typedef enum resampling_correction {
    /*! No correction applied. */
    RESAMPLING_NO_CORRECTION,
    /*! Add a sample for correction. */
    RESAMPLING_ADD_SAMPLE,
    /*! Remove a sample for correction. */
    RESAMPLING_REMOVE_SAMPLE,
} resampling_correction_t;

/** @brief Resampling Instance Status.
 *
 *  @note This enum contains all states for this library.
 */
typedef enum resampling_status {
    /*! Waiting for the queue to be full. */
    RESAMPLING_WAIT_QUEUE_FULL,
    /*! Idle state. */
    RESAMPLING_IDLE,
    /*! Start state. */
    RESAMPLING_START,
    /*! Running state. */
    RESAMPLING_RUNNING
} resampling_status_t;

/** @brief Resampling library configuration structure.
 *
 *  @note Variables within this structure can be set by the user to configure the resampling instance.
 */
typedef struct resampling_config {
    /*! Number of samples per frame. */
    uint16_t nb_sample;
    /*! Sample bit depth type. */
    resampling_buffer_type_t buffer_type;
    /*! Resampling length. */
    uint16_t resampling_length;
    /*! Number of audio channels. */
    uint8_t nb_channel;
} resampling_config_t;

/** @brief Resampling library instance structure.
 *
 *  @note Variables within this structure will be set by the library in the init function.
 */
typedef struct resampling_instance {
    /*! Current resampling status. */
    resampling_status_t status;
    /*! Active correction mode. */
    resampling_correction_t correction;
    /*! Sample bit depth type. */
    resampling_buffer_type_t buffer_type;
    /*! Maximum value for the buffer type. */
    uint32_t buffer_type_max;
    /*! Last samples buffer for interpolation. */
    int32_t last_sample[LAST_SAMPLE_ARRAY_SIZE];
    /*! Interpolation step for sample addition. */
    uint32_t step_add;
    /*! Interpolation step for sample removal. */
    uint32_t step_rem;
    /*! Interpolation bias value. */
    uint32_t bias;
    /*! Bias step for sample addition. */
    uint32_t bias_step_add;
    /*! Bias step for sample removal. */
    uint32_t bias_step_rem;
    /*! Current x-axis interpolation position. */
    int64_t x_axis;
    /*! Number of audio channels. */
    uint8_t nb_channel;
    /*! Maximum x-axis value. */
    uint32_t max_x_axis;
} resampling_instance_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the resampling struct that correspond to the instance.
 *
 *  @param[in] instance           Structure instance pointer.
 *  @param[in] resampling_config  Pointer to the instance's configuration structure.
 *  @return resampling_errors_t  Resampling error code.
 */
resampling_errors_t resampling_init(resampling_instance_t *instance, const resampling_config_t *resampling_config);

/** @brief Start to resample the signal(s).
 *
 *  @param[in] instance    Structure instance pointer.
 *  @param[in] correction  ADD_SAMPLE or REMOVE_SAMPLE.
 */
void resampling_start(resampling_instance_t *instance, resampling_correction_t correction);

/** @brief resample the signal if STARTED.
 *
 *  @param[in]  instance      Structure instance pointer.
 *  @param[in]  ptr_input     Pointer to input data.
 *  @param[out] ptr_output    Pointer to output data.
 *  @param[in]  sample_count  Amount of samples to be treated.
 *  @return Samples count.
 */
uint16_t resample(resampling_instance_t *instance, void *ptr_input, void *ptr_output, uint16_t sample_count);

/** @brief Return the resampling status.
 *
 *  @param[in] instance  IStructure instance pointer.
 *  @return resampling_status_t  Resampling status.
 */
resampling_status_t resample_get_state(const resampling_instance_t *instance);

/** @brief Return the resampling channel count.
 *
 *  @param[in] instance  Structure instance pointer.
 *  @return Number of channels the instance is configured for.
 */
uint8_t resample_get_channel_count(const resampling_instance_t *instance);

#ifdef __cplusplus
}
#endif

#endif /* RESAMPLING_H_ */
