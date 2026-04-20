/** @file  sac_error.h
 *  @brief SPARK Audio Core error codes.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_ERROR_H_
#define SAC_ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/* Current __COUNTER__ value. */
enum { SAC_COUNTER_BASE = __COUNTER__ };

/* MACROS *********************************************************************/
#ifndef ASSERT_SAC_STATUS
/** @brief ASSERT the return status of a SAC function.
 */
#define ASSERT_SAC_STATUS(sac_status)        \
    do {                                     \
        if (sac_status == SAC_OK) {          \
            /* Exit early. */                \
            break;                           \
        }                                    \
                                             \
        if (sac_status > SAC_OK) {           \
            /* Handle warning. */            \
            sac_warning_handler(sac_status); \
            break;                           \
        }                                    \
                                             \
        /* Handle error. */                  \
        sac_error_handler(sac_status);       \
    } while (0)
#endif

/** @brief Create a local counter macro.
 */
#define SAC_COUNTER (__COUNTER__ - SAC_COUNTER_BASE)

/** @brief Automatically increase error count each time invoked. Starts at -1.
 */
#define SAC_GENERATE_ERR_CODE (-SAC_COUNTER)

/* TYPES **********************************************************************/
/** @brief SAC Errors and Warnings.
 */
typedef enum sac_status {
    /*! No error nor warning occurred. */
    SAC_OK = 0,

    /*! Warnings (positive values) */
    /*! Producer's queue is full when trying to produce. */
    SAC_WARN_PRODUCER_Q_FULL,
    /*! Consumer's queue is empty when trying to consume. */
    SAC_WARN_CONSUMER_Q_EMPTY,
    /*! Initial buffering is not completed when trying to consume. */
    SAC_WARN_BUFFERING_NOT_COMPLETE,
    /*! Producer's queue is empty when trying to process. */
    SAC_WARN_NO_SAMPLES_TO_PROCESS,
    /*! The processing queue is empty and no nodes are available for processing. */
    SAC_WARN_PROCESSING_Q_EMPTY,
    /*! A processing stage returned nothing to be produced.
     *  The processing stage might be accumulating data to generate bigger packets.
     */
    SAC_WARN_NO_PACKET_TO_PRODUCE,

    /*! Errors (negative values) */
    /*! Not enough memory is allocated by the application for a full audio core initialization. */
    SAC_ERR_NOT_ENOUGH_MEMORY = SAC_GENERATE_ERR_CODE,
    /*! Maximum number of processing stages for a given SAC pipeline is already reached when trying to add another one.
     */
    SAC_ERR_PROC_STAGE_LIMIT_REACHED = SAC_GENERATE_ERR_CODE,
    /*! An error occurred during the processing stage initialization. */
    SAC_ERR_PROCESSING_STAGE_INIT = SAC_GENERATE_ERR_CODE,
    /*! Pipeline configuration is invalid. */
    SAC_ERR_PIPELINE_CFG_INVALID = SAC_GENERATE_ERR_CODE,
    /*! A pointer is NULL while it should have been initialized. */
    SAC_ERR_NULL_PTR = SAC_GENERATE_ERR_CODE,
    /*! An error occurred during the mixer module initialization. */
    SAC_ERR_MIXER_INIT_FAILURE = SAC_GENERATE_ERR_CODE,
    /*! The maximum number of elements allowed have been reached. */
    SAC_ERR_MAXIMUM_REACHED = SAC_GENERATE_ERR_CODE,
    /*! A processing stage's control function has been called with an invalid command. */
    SAC_ERR_INVALID_CMD = SAC_GENERATE_ERR_CODE,
    /*! An error occurred during the fallback module initialization. */
    SAC_ERR_FALLBACK_INIT_FAILURE = SAC_GENERATE_ERR_CODE,
    /*! The configured bit depth is invalid. */
    SAC_ERR_BIT_DEPTH = SAC_GENERATE_ERR_CODE,
    /*! The configured channel count is invalid. */
    SAC_ERR_CHANNEL_COUNT = SAC_GENERATE_ERR_CODE,
    /*! The configured mixer option is invalid. */
    SAC_ERR_MIXER_OPTION = SAC_GENERATE_ERR_CODE,
    /*! A processing stage's control function has been called with an invalid argument. */
    SAC_ERR_INVALID_ARG = SAC_GENERATE_ERR_CODE,
    /*! A node is too small for the data that needs to be copied into it. */
    SAC_ERR_NODE_DATA_SIZE_TOO_SMALL = SAC_GENERATE_ERR_CODE,
    /*! The SAC has not been initialized yet. */
    SAC_ERR_NOT_INIT = SAC_GENERATE_ERR_CODE,
    /*! A fallback process wasn't found in the processing stage list. */
    SAC_ERR_FALLBACK_PROC_NOT_FOUND = SAC_GENERATE_ERR_CODE,
    /*! A process received an invalid packet size. */
    SAC_ERR_INVALID_PACKET_SIZE = SAC_GENERATE_ERR_CODE,
    /*! The number of fallback mode exceeds the maximum of SAC_FALLBACK_MAX_MODE_COUNT. */
    SAC_ERR_FALLBACK_MODE_COUNT = SAC_GENERATE_ERR_CODE,
    /*! The fallback mode is already part of the list of modes. */
    SAC_ERR_FALLBACK_MODE_DUPLICATE = SAC_GENERATE_ERR_CODE,
    /*! The pipeline payload size is not big enough to fit the audio sample accumulator size. */
    SAC_ERR_PIPELINE_PAYLOAD_NOT_BIG_ENOUGH = SAC_GENERATE_ERR_CODE,
} sac_status_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Handle a SAC error.
 *
 *  @note This function has a weak definition in sac_error.c. The function can be redifined in the application to change
 *        the error handling behaviour.
 *
 *  @param[in] sac_status  SAC status code.
 */
void sac_error_handler(sac_status_t sac_status);

/** @brief Handle a SAC warning.
 *
 *  @note This function has a weak definition in sac_error.c. The function can be redifined in the application to change
 *        the warning handling behaviour.
 *
 *  @param[in] sac_status  SAC status code.
 */
void sac_warning_handler(sac_status_t sac_status);

#ifdef __cplusplus
}
#endif

#endif /* SAC_ERROR_H_ */
