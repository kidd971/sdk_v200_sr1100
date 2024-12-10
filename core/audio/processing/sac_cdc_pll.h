/** @file  sac_cdc_pll.h
 *  @brief Clock drift compensation processing stage using audio buffer load averaging for
 *         detecting the drift and audio pll adjustment for correcting it.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_CDC_PLL_H_
#define SAC_CDC_PLL_H_

/* INCLUDES *******************************************************************/
#include "sac_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief CDC Commands.
 */
typedef enum sac_cdc_pll_cmd {
    SAC_CDC_PLL_CMD_NONE,
    SAC_CDC_PLL_CMD_INCREASE,
    SAC_CDC_PLL_CMD_DECREASE,
    SAC_CDC_PLL_CMD_SET_TARGET_QUEUE_SIZE,
} sac_cdc_pll_cmd_t;

/** @brief CDC Statistics.
 */
typedef struct sac_cdc_pll_stats {
    uint32_t target_queue_size;
    uint32_t avg_queue_size;
    int32_t queue_size_error;
    int32_t queue_size_avg_delta;
    uint32_t current_pll_value;
    int32_t pll_fracn_offset;
} sac_cdc_pll_stats_t;

/** @brief CDC PLL Hardware Abstraction Layer (HAL).
 */
typedef struct sac_cdc_pll_hal {
    /*! Function used to set the FRACN. */
    void (*set_fracn)(uint32_t fracn);
    /*! Function used to get the current FRACN. */
    uint32_t (*get_fracn)(void);
    /*! FRACN minimum value. */
    uint32_t fracn_min_value;
    /*! FRACN maximum value. */
    uint32_t fracn_max_value;
    /*! FRACN default value. */
    uint32_t fracn_default_value;
} sac_cdc_pll_hal_t;

/** @brief CDC Instance.
 */
typedef struct sac_cdc_pll_instance {
    /*! Format of the audio samples. */
    sac_sample_format_t sample_format;
    /*! CDC PLL HAL. */
    sac_cdc_pll_hal_t cdc_pll_hal;
    struct {
        /*! Internal: Number of bytes per audio sample. */
        uint8_t size_of_buffer_type;
        /*! Internal: A circular array of queue size used for averaging. */
        uint8_t *avg_arr;
        /*! Internal: Rolling average of the avg_arr. */
        uint32_t avg_sum;
        /*! Internal: Average queue size in number of samples. */
        uint32_t avg_val;
        /*! Internal: Previous queue size average in number of samples. */
        uint32_t prev_avg_val;
        /*! Internal: Delta between previous queue size average in number of samples. */
        int32_t avg_val_delta;
        /*! Internal: Error between avg_val and target_queue_size in number of samples. */
        int32_t error;
        /*! Internal: Index into the avg_arr. */
        uint32_t avg_idx;
        /*! Internal: Target queue size in number of samples. */
        uint32_t target_queue_size;
        /* Internal: Number of samples in each audio payload to process. */
        uint32_t sample_amount;
        /*! Internal: Current PLL fracn offset from locked value. */
        int16_t pll_fracn_offset;
        /*! Internal: Current queue level is high. */
        bool queue_level_high;
        /*! Internal: Current queue level is low. */
        bool queue_level_low;
        /*! Internal: CDC PLL statistics structure. */
        sac_cdc_pll_stats_t sac_cdc_pll_stats;
    } _internal;
} sac_cdc_pll_instance_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the CDC processing stage.
 *
 *  @param[in]  instance  CDC instance.
 *  @param[in]  name      Processing stage name.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  mem_pool  Memory pool handle.
 *  @param[out] status    Status code.
 */
void sac_cdc_pll_init(void *instance, const char *name, sac_pipeline_t *pipeline, mem_pool_t *mem_pool,
                      sac_status_t *status);

/** @brief Control the CDC processing stage.
 *
 *  @param[in]  instance  CDC instance.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  cmd       Command.
 *  @param[in]  arg       Argument.
 *  @param[out] status    Status code.
 *
 *  @return Command specific value.
 */
uint32_t sac_cdc_pll_ctrl(void *instance, sac_pipeline_t *pipeline, uint8_t cmd, uint32_t arg, sac_status_t *status);

/** @brief Process the CDC processing stage.
 *
 *  @param[in]  instance  CDC instance.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  header    Audio packet's header.
 *  @param[in]  data_in   Audio payload to process.
 *  @param[in]  size      Size in bytes of the audio payload.
 *  @param[out] data_out  Audio payload that has been processed.
 *  @param[out] status    Status code.
 *
 *  @return Size in bytes of the processed samples, 0 if no processing happened.
 */
uint16_t sac_cdc_pll_process(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in,
                             uint16_t size, uint8_t *data_out, sac_status_t *status);

/** @brief Get the Clock Drift Compensation statistics.
 *
 *  @param[in] cdc  CDC instance.
 *  @return The Clock Drift Compensation statistics.
 */
sac_cdc_pll_stats_t sac_cdc_pll_get_stats(sac_cdc_pll_instance_t *cdc);

/** @brief Format the Clock Drift Compensation statistics as a string of characters.
 *
 *  @param[in]  cdc     CDC instance.
 *  @param[out] buffer  Buffer where to put the formatted string.
 *  @param[in]  size    Size of the buffer.
 *  @return The formatted string length, excluding the NULL terminator.
 */
int sac_cdc_pll_format_stats(sac_cdc_pll_instance_t *cdc, char *buffer, uint16_t size);

#ifdef __cplusplus
}
#endif

#endif /* SAC_CDC_PLL_H_ */
