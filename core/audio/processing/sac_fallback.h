/** @file  sac_fallback.h
 *  @brief SPARK Audio Core Fallback processing stage is used to manage audio fallback. It allows other processes to be
 *         gated by the state of the fallback process. The pipeline's processes can thus be dynamic and can generate
 *         multiple types of output. (ex: uncompressed, compressed, resampled, etc.)
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_FALLBACK_H_
#define SAC_FALLBACK_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "mem_pool.h"
#include "sac_api.h"
#include "sac_error.h"
#include "swc_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/*! Array size holding buffer load values to calculate a rolling average */
#define SAC_FALLBACK_QUEUE_ARRAY_LENGTH 3

/** @brief The SPARK Audio Core fallback States.
 */
typedef enum sac_fallback_state {
    /*! Normal state, monitor the TX audio buffer to switch to wait threshold. */
    SAC_FB_STATE_NORMAL,
    /*! Link degrading, measure link margin for a sampling period to determine return from fallback threshold. */
    SAC_FB_STATE_WAIT_THRESHOLD,
    /*! Fallback mode due to degraded link, monitor link margin to return to normal mode. */
    SAC_FB_STATE_FALLBACK,
    /*! Fallback mode due to disconnected link, threshold set to fixed value,
     *   monitor link margin to return to normal mode.
     */
    SAC_FB_STATE_FALLBACK_DISCONNECT
} sac_fallback_state_t;

/* TYPES **********************************************************************/
/** @brief The SPARK Audio Core fallback Queue Metrics.
 */
typedef struct sac_fallback_queue_metrics {
    /*! Queue length averaging array. */
    uint8_t queue_length_arr[SAC_FALLBACK_QUEUE_ARRAY_LENGTH];
    /*! Queue length averaging array index. */
    uint8_t queue_length_arr_idx;
    /*! Queue length rolling average sum. */
    uint16_t queue_length_sum;
    /*! Queue length rolling average. */
    uint8_t queue_length_avg_tenths;
} sac_fallback_queue_metrics_t;

/** @brief The SPARK Audio Core fallback Link Margin Metrics.
 */
typedef struct sac_fallback_link_margin_metrics {
    /*! Current link margin threshold to return to normal. */
    uint8_t threshold;
    /*! Default link margin threshold. */
    uint8_t threshold_default;
    /*! Link margin threshold hysteresis. */
    uint8_t threshold_hysteresis;
    /*! Accumulation of link margin values over a sampling period. */
    uint32_t accumulator;
    /*! Number of link margin values accumulated. */
    uint16_t accumulator_count;
    /*! Link margin average in a sampling period calculated from accumulator. */
    uint8_t accumulator_average;
    /*! Number of continuous sampling periods where the link margin values were above the threshold. */
    uint8_t good_count;
    /*! Number of continuous sampling periods where the link margin must be higher than
     *   the threshold to allow disabling fallback.
     */
    uint16_t good_count_threshold;
    /*! Consumer link margin min. */
    uint8_t consumer_link_margin_min_peak;
} sac_fallback_link_margin_metrics_t;

/** @brief The SPARK Audio Core fallback CCA Metrics.
 */
typedef struct sac_fallback_cca_metrics {
    /*! CCA event count value at the start of the averaging period. */
    uint32_t cca_event_count_start;
    /*! Current CCA event count value. */
    uint32_t cca_event_count_current;
    /*! Number of CCA event in the last sampling period. */
    uint32_t cca_event_count;
    /*! Averaged number CCA fail count allowed per transmission multiplied by CCA_DECIMAL_FACTOR. */
    uint32_t fail_count_threshold;
    /*! CCA fail count value at the start of the averaging period. */
    uint32_t fail_count_start;
    /*! Current CCA fail count value. */
    uint32_t fail_count_current;
    /*! Number CCA fail count in the last sampling period. */
    uint32_t fail_count;
    /*! Averaged number CCA fail count in the last sampling period multiplied by CCA_DECIMAL_FACTOR. */
    uint32_t fail_count_avg;
    /*! Number of continuous sampling periods where the CCA fail count value was below the threshold. */
    uint16_t good_count;
    /*! Number of continuous sampling periods where the CCA fail count value was over the threshold. */
    uint16_t bad_count;
    /*! Number of continuous sampling periods where the CCA try count must be lower than
     *   the threshold to allow disabling fallback.
     */
    uint16_t good_count_threshold;
    /*! Number of continuous sampling periods where the CCA try count must be higher than
     *   the threshold to trigger the fallback mode.
     */
    uint16_t bad_count_threshold;
    /*! Consumer CCA count peak. */
    uint32_t consumer_cca_fail_count_peak;
} sac_fallback_cca_metrics_t;

/** @brief The SPARK Audio Core fallback instance.
 */
typedef struct sac_fallback_instance {
    /*! Wireless connection on which the fallback is instantiated. */
    swc_connection_t *connection;
    /*! Set to true if instantiated for an audio transmitting pipeline. */
    bool is_tx_device;
    /*! Default average link margin threshold to allow disabling fallback. */
    uint8_t link_margin_threshold;
    /*! Link margin threshold hysteresis. */
    uint8_t link_margin_threshold_hysteresis;
    /*! Amount of time in seconds the link margin must be higher than the threshold
     *   to allow disabling fallback.
     */
    uint32_t link_margin_good_time_sec;
    /*! Maximum number of CCA tries possible on this connection. */
    uint16_t cca_max_try_count;
    /*! Average CCA try count threshold in percent of the maximum number of
     *   CCA tries possible on this connection.
     */
    uint8_t cca_try_count_threshold_perc;
    /*! Amount of time in seconds the CCA try count must be lower than the threshold
     *   to allow disabling fallback.
     */
    uint32_t cca_good_time_sec;
    /*! Amount of time in seconds the CCA try count must be higher than the threshold
     *   to enable fallback.
     */
    float cca_bad_time_sec;
    /*! Audio transmitting pipeline consumer buffer load above which fallback is triggered.
     *   Value should be multiplied by 10. (ex: 1.3 is 13).
     */
    uint32_t consumer_buffer_load_threshold_tenths;
    /*! Frequency of the system tick in Hertz. */
    uint32_t tick_frequency_hz;
    /*! Function used to get the system tick value. */
    uint32_t (*get_tick)(void);
    /*! Optional callback function called on a fallback state change. */
    void (*fallback_state_change_callback)(bool enabled);
    struct {
        /*! Name of the instance. */
        const char *name;
        /*! Pipeline on which the fallback is instantiated. */
        sac_pipeline_t *pipeline;
        /*! Fallback state. */
        sac_fallback_state_t fallback_state;
        /*! Number of times fallback was triggered. */
        uint32_t fallback_count;
        /*! Maximum size of the audio transmitting consumer buffer multiplied by 10. */
        uint32_t consumer_buffer_size_tenths;
        /*! Fallback mode flag. */
        bool fallback_flag;
        /*! Manual fallback mode. */
        bool manual_mode;
        /*! Tick value of the last sampling event. */
        uint32_t sampling_tick_start;
        /*! Audio transmitting pipeline consumer queue metrics. */
        sac_fallback_queue_metrics_t consumer_queue_metrics;
        /*! Link margin metrics. */
        sac_fallback_link_margin_metrics_t link_margin_metrics;
        /*! CCA metrics. */
        sac_fallback_cca_metrics_t cca_metrics;
    } _internal;
} sac_fallback_instance_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Get the default values of the SPARK Audio Core fallback.
 *
 *  @note These parameters still need to be set by user:
 *         - wireless connection
 *         - is_tx_device
 *         Tx device only:
 *         - link_margin_conn_packets_per_second
 *         - main_conn_packets_per_second
 *        Other parameters should be reviewed by user but will work with their default value.
 *
 *  @return Default fallback instance structure.
 */
sac_fallback_instance_t sac_fallback_get_defaults(void);

/** @brief Initialize the SPARK Audio Core fallback.
 *
 *  @param[in]  instance  Processing stage instance.
 *  @param[in]  name      Processing stage name.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  mem_pool  Memory pool handle.
 *  @param[out] status    Status code.
 */
void sac_fallback_init(void *instance, const char *name, sac_pipeline_t *pipeline, mem_pool_t *mem_pool,
                       sac_status_t *status);

/** @brief Process the fallback.
 *
 *  @note This processing stage should be ran before processing audio packets to be sure the fallback is in the correct
 *        state.
 *
 *  @param[in]  instance  Processing stage instance.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  header    Audio packet's header.
 *  @param[in]  data_in   Audio payload to process.
 *  @param[in]  size      Size in bytes of the audio payload.
 *  @param[out] data_out  Audio payload that has been processed.
 *  @param[out] status    Status code.
 *
 *  @return Size in bytes of the processed samples, 0 if no processing happened.
 */
uint16_t sac_fallback_process(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in,
                              uint16_t size, uint8_t *data_out, sac_status_t *status);

/** @brief Set the time the link margin has to be good to get out of fallback mode.
 *
 *  @param[in]  instance  Pointer to fallback instance.
 *  @param[in]  time      Time in seconds.
 *  @param[out] status    Status code.
 */
void sac_fallback_set_link_margin_good_time(sac_fallback_instance_t *instance, uint32_t time, sac_status_t *status);

/** @brief Set the time the CCA try count has to be lower than the threshold to get out of fallback mode.
 *
 *  @param[in]  instance  Pointer to fallback instance.
 *  @param[in]  time      Time in seconds.
 *  @param[out] status    Status code.
 */
void sac_fallback_set_cca_good_time(sac_fallback_instance_t *instance, uint32_t time, sac_status_t *status);

/** @brief Set the time the CCA try count has to be higher than the threshold to activate fallback mode.
 *
 *  @param[in]  instance  Pointer to fallback instance.
 *  @param[in]  time      Time in seconds.
 *  @param[out] status    Status code.
 */
void sac_fallback_set_cca_bad_time(sac_fallback_instance_t *instance, float time, sac_status_t *status);

/** @brief Set the received rx link margin value from the node.
 *
 *  @param[in]  instance  Pointer to fallback instance.
 *  @param[in]  rx_lm     RX link margin value.
 *  @param[out] status    Status code.
 */
void sac_fallback_set_rx_link_margin(sac_fallback_instance_t *instance, uint8_t rx_lm, sac_status_t *status);

/** @brief Return status of fallback flag.
 *
 *  @param[in]  instance  Pointer to fallback instance.
 *  @param[out] status    Status code.
 *  @return True if active.
 */
bool sac_fallback_is_active(sac_fallback_instance_t *instance, sac_status_t *status);

/** @brief Set fallback flag.
 *
 *  @param[in]  instance  Pointer to fallback instance.
 *  @param[out] status    Status code.
 */
void sac_fallback_set_fallback_flag(sac_fallback_instance_t *instance, sac_status_t *status);

/** @brief Clear fallback flag.
 *
 *  @param[in]  instance  Pointer to fallback instance.
 *  @param[out] status    Status code.
 */
void sac_fallback_clear_fallback_flag(sac_fallback_instance_t *instance, sac_status_t *status);

/** @brief Disable the automatic fallback mode switching.
 *
 *  @note The fallback mode can be changed using the sac_fallback_set_fallback_flag or the
 *        sac_fallback_clear_fallback_flag function.
 *
 *  @param[in]  instance             Pointer to fallback instance.
 *  @param[in]  manual_mode_enabled  True to enable manual mode.
 *  @param[out] status               Status code.
 */
void sac_fallback_set_manual_mode(sac_fallback_instance_t *instance, bool manual_mode_enabled, sac_status_t *status);

/** @brief Get fallback count.
 *
 *  @param[in]  instance  Pointer to fallback instance.
 *  @param[out] status    Status code.
 *  @return Fallback count.
 */
uint32_t sac_fallback_get_fallback_count(sac_fallback_instance_t *instance, sac_status_t *status);

/** @brief Get CCA metrics.
 *
 *  @param[in]  instance  Pointer to fallback instance.
 *  @param[out] status    Status code.
 *  @return Pointer to CCA metrics.
 */
sac_fallback_cca_metrics_t *sac_fallback_get_cca_metrics(sac_fallback_instance_t *instance, sac_status_t *status);

/** @brief Get link margin metrics.
 *
 *  @param[in]  instance  Pointer to fallback instance.
 *  @param[out] status    Status code.
 *  @return Pointer to link margin metrics.
 */
sac_fallback_link_margin_metrics_t *sac_fallback_get_link_margin_metrics(sac_fallback_instance_t *instance,
                                                                         sac_status_t *status);

/** @brief Get RX link margin.
 *
 *  @param[in]  instance  Pointer to fallback instance.
 *  @param[out] status    Status code.
 *  @return RX link margin.
 */
uint8_t sac_fallback_get_rx_link_margin(sac_fallback_instance_t *instance, sac_status_t *status);

/** @brief Format the fallback statistics as a string of characters.
 *
 *  @param[in]  instance  Pointer to fallback instance.
 *  @param[out] buffer    Buffer where to put the formatted string.
 *  @param[in]  size      Size of the buffer.
 *  @param[out] status    Status code.
 *  @return The formatted string length, excluding the NULL terminator.
 */
int sac_fallback_format_stats(sac_fallback_instance_t *instance, char *buffer, uint16_t size, sac_status_t *status);

/** @brief Reset fallback statistics.
 *
 *  @param[in]  instance  Pointer to fallback instance.
 *  @param[out] status    Status code.
 */
void sac_fallback_reset_stats(sac_fallback_instance_t *instance, sac_status_t *status);

/** @brief Return the device's role as either a transmitter or receiver.
 *
 *  @param[in]  instance  Pointer to fallback instance.
 *  @param[out] status    Status code.
 *  @return True if transmitter, false if receiver.
 */
bool sac_fallback_is_tx_device(sac_fallback_instance_t *instance, sac_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* SAC_FALLBACK_H_ */
