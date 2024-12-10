/** @file  sac_error.h
 *  @brief SPARK Audio Core error codes.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_ERROR_H_
#define SAC_ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief SAC Errors and Warnings.
 */
typedef enum sac_status {
    /*! No error nor warning occurred. */
    SAC_OK = 0,

    /*! Errors (negative values) */
    /*! Not enough memory is allocated by the application for a full audio core initialization. */
    SAC_ERR_NOT_ENOUGH_MEMORY = -1,
    /*! Maximum number of processing stages for a given SAC pipeline is already reached when trying to add another one.
     */
    SAC_ERR_PROC_STAGE_LIMIT_REACHED = -2,
    /*! An error occurred during the processing stage initialization. */
    SAC_ERR_PROCESSING_STAGE_INIT = -3,
    /*! Pipeline configuration is invalid. */
    SAC_ERR_PIPELINE_CFG_INVALID = -4,
    /*! A pointer is NULL while it should have been initialized. */
    SAC_ERR_NULL_PTR = -5,
    /*! An error occurred during the mixer module initialization. */
    SAC_ERR_MIXER_INIT_FAILURE = -6,
    /*! The maximum number of elements allowed have been reached. */
    SAC_ERR_MAXIMUM_REACHED = -7,
    /*! A processing stage's control function has been called with an invalid command. */
    SAC_ERR_INVALID_CMD = -8,
    /*! An error occurred during the fallback module initialization. */
    SAC_ERR_FALLBACK_INIT_FAILURE = -9,
    /*! The configured bit depth is invalid. */
    SAC_ERR_BIT_DEPTH = -10,
    /*! The configured channel count is invalid. */
    SAC_ERR_CHANNEL_COUNT = -11,
    /*! The configured mixer option is invalid. */
    SAC_ERR_MIXER_OPTION = -12,
    /*! A processing stage's control function has been called with an invalid argument. */
    SAC_ERR_INVALID_ARG = -13,
    /*! A node is too small for the data that needs to be copied into it. */
    SAC_ERR_NODE_DATA_SIZE_TOO_SMALL = -14,
    /*! The SAC has not been initialized yet. */
    SAC_ERR_NOT_INIT = -15,
    /*! A fallback process wasn't found in the processing stage list. */
    SAC_ERR_FALLBACK_PROC_NOT_FOUND = -16,
    /*! A process received an invalid packet size. */
    SAC_ERR_INVALID_PACKET_SIZE = -17,

    /*! Warnings (positive values) */
    /*! Producer's queue is full when trying to produce. */
    SAC_WARN_PRODUCER_Q_FULL = 1,
    /*! Consumer's queue is empty when trying to consume. */
    SAC_WARN_CONSUMER_Q_EMPTY = 2,
    /*! Initial buffering is not completed when trying to consume. */
    SAC_WARN_BUFFERING_NOT_COMPLETE = 3,
    /*! Producer's queue is empty when trying to process. */
    SAC_WARN_NO_SAMPLES_TO_PROCESS = 4,
    /*! The processing queue is empty and no nodes are available for processing. */
    SAC_WARN_PROCESSING_Q_EMPTY = 5,
} sac_status_t;

#ifdef __cplusplus
}
#endif

#endif /* SAC_ERROR_H_ */
