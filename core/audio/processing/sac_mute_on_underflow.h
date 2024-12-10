/** @file  sac_mute_on_underflow.h
 *  @brief Processing stage used to mute the audio output of a device for a short time
 *         when an underflow occurs.
 *
 *  @note This processing stage should be the last processing stage on an audio receiving pipeline.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_MUTE_ON_UNDERFLOW_H_
#define SAC_MUTE_ON_UNDERFLOW_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "sac_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief SPARK Audio Core Mute on Underflow Commands.
 */
typedef enum sac_mute_on_underflow_cmd {
    /* Command to set the reload value of the counter. */
    SAC_MUTE_ON_UNDERFLOW_SET_RELOAD
} sac_mute_on_underflow_cmd_t;

/** @brief SPARK Audio Core Mute on Underflow Instance.
 */
typedef struct sac_mute_on_underflow_instance {
    /*! Number of packets that will be muted after the last underflow occured. */
    uint32_t reload_value;
    struct {
        /*! Internal: Counter used to keep track of how many muted packets are left. */
        uint32_t counter;
        /*! Internal: Counter used to keep track of the underflow statistic. */
        uint32_t underflow_count;
    } _internal;
} sac_mute_on_underflow_instance_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize mute on underflow processing stage.
 *
 *  @param[in]  instance  Process instance.
 *  @param[in]  name      Processing stage name.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  mem_pool  Memory pool for memory allocation.
 *  @param[out] status    Status code.
 */
void sac_mute_on_underflow_init(void *instance, const char *name, sac_pipeline_t *pipeline, mem_pool_t *mem_pool,
                                sac_status_t *status);

/** @brief Control the mute on underflow processing stage.
 *
 *  @param[in]  instance  Process instance.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  cmd       Command.
 *  @param[in]  args      Argument.
 *  @param[out] status    Status code.
 *
 *  @return Command specific value.
 */
uint32_t sac_mute_on_underflow_ctrl(void *instance, sac_pipeline_t *pipeline, uint8_t cmd, uint32_t arg,
                                    sac_status_t *status);

/** @brief Process the mute on underflow processing stage.
 *
 *  @param[in]  instance  Process instance.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  header    Audio packet's header.
 *  @param[in]  data_in   Audio payload to process.
 *  @param[in]  size      Size in bytes of the audio payload.
 *  @param[out] data_out  Audio payload that has been processed.
 *  @param[out] status    Status code.
 *
 *  @return Size in bytes of the processed samples, 0 if no processing happened.
 */
uint16_t sac_mute_on_underflow_process(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in,
                                       uint16_t size, uint8_t *data_out, sac_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* SAC_MUTE_ON_UNDERFLOW_H_ */
