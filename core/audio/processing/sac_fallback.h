/** @file  sac_fallback.h
 *  @brief SPARK Audio Core Fallback processing stage is used to manage audio fallback. It allows other processes to be
 *         gated by the state of the fallback process. The pipeline's processes can thus be dynamic and can generate
 *         multiple types of output. (ex: uncompressed, compressed, resampled, etc.)
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
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
#ifndef SAC_FALLBACK_QUEUE_ARRAY_LENGTH
#define SAC_FALLBACK_QUEUE_ARRAY_LENGTH 3
#endif
/*! Maximum number of fallback modes supported by the SAC header (3-bit field). */
#ifndef SAC_FALLBACK_MAX_MODE_COUNT
#define SAC_FALLBACK_MAX_MODE_COUNT 8
#endif

/* TYPES **********************************************************************/
typedef struct sac_fallback_instance sac_fallback_instance_t;

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
    /*! Accumulation of link margin values over a sampling period. */
    uint32_t accumulator;
    /*! Number of link margin values accumulated. */
    uint16_t accumulator_count;
    /*! Link margin average in a sampling period calculated from accumulator. */
    uint8_t accumulator_average;
    /*! Number of continuous sampling periods where the link margin values were above the threshold. */
    uint16_t recovery_good_count;
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
    /*! CCA fail count value at the start of the averaging period. */
    uint32_t fail_count_start;
    /*! Current CCA fail count value. */
    uint32_t fail_count_current;
    /*! Averaged number CCA fail count in the last sampling period multiplied by CCA_DECIMAL_FACTOR. */
    uint32_t fail_count_avg;
    /*! Number of continuous sampling periods where the CCA fail count value was below the threshold. */
    uint16_t recovery_good_count;
    /*! Number of continuous sampling periods where the CCA fail count value was over the threshold. */
    uint16_t trigger_bad_count;
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
    /*! Optional callback function called on a fallback state change. */
    void (*fallback_state_change_callback)(uint8_t mode_index);
    /* **** TX device settings. **** */
    /*! Frequency of the system tick in Hertz. */
    uint32_t tick_frequency_hz;
    /*! Function used to get the system tick value. */
    uint32_t (*get_tick)(void);
    struct {
        /*! Name of the instance. */
        const char *name;
        /*! Pipeline on which the fallback is instantiated. */
        sac_pipeline_t *pipeline;
        /*! SAC mem_pool instance. */
        mem_pool_t *sac_mem_pool;
        /*! Maximum size of the audio transmitting consumer buffer multiplied by 10. */
        uint32_t consumer_buffer_size_tenths;
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
        /*! First fallback mode. */
        void *first_mode;
        /*! Last fallback mode. */
        void *last_mode;
        /*! Current active mode. */
        void *current_mode;
        /*! Number of modes. */
        uint8_t mode_count;
        /*! Maximum number of CCA tries possible on this connection. */
        uint16_t cca_max_try_count;
        /*! Total number of packets processed. */
        uint64_t total_packet_count;
        /*! Indicates if the device is currently buffering the TX queue. */
        bool is_buffering;
        /*! Tick value when buffering was completed. */
        uint32_t buffering_complete_tick;
    } _internal;
} sac_fallback_instance_t;

/** @brief The SPARK Audio Core fallback mode configuration structure.
 */
typedef struct sac_fallback_mode_cfg {
    /*! Fallback algorithm sampling frequency in Hertz. */
    uint16_t sampling_frequency_hz;
    /*! ***  RECOVERY THRESHOLDS *** */
    /*! Default average link margin threshold to allow recovery. */
    uint8_t link_margin_threshold;
    /*! Link margin threshold hysteresis. */
    uint8_t link_margin_threshold_hysteresis;
    /*! Amount of time in seconds the link margin must be higher than the threshold to allow recovery. */
    uint32_t link_margin_good_time_sec;
    /*! Average CCA fail count threshold in percent of the maximum number of CCA tries possible on this connection to
     *  allow recovery to previous mode.
     */
    uint8_t cca_good_fail_count_threshold_perc;
    /*! Amount of time in seconds the CCA fail count must be lower than the threshold to allow recovery. */
    uint32_t cca_good_time_sec;

    /*! *** TRIGGER THRESHOLDS *** */
    /*! Average CCA fail count threshold in percent of the maximum number of CCA tries possible on this connection to
     *  trigger the next mode.
     */
    uint8_t cca_bad_fail_count_threshold_perc;
    /*! Amount of time in seconds the CCA fail count must be higher than the threshold to trigger the next mode. */
    float cca_bad_time_sec;
    /*! Audio transmitting pipeline consumer buffer load above which the next fallback mode is triggered.
     *   Value should be multiplied by 10. (ex: 1.3 is 13).
     */
    uint32_t consumer_buffer_load_threshold_tenths;
    /*! Number of samples to expect in a packet in this mode. This should be set to the expected number of samples of
     *  that specific fallback mode, if sample rate converter processing stage is added to that fallback mode.
     *
     *  It can also be used to track the expected sample count of each mode when using the accumulator processing stage.
     */
    uint32_t sample_count;
} sac_fallback_mode_cfg_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Get the fallback mode configuration default values.
 *
 *  @return Default fallback mode configuration structure.
 */
sac_fallback_mode_cfg_t sac_fallback_mode_get_defaults(void);

/** @brief Initialize the SPARK Audio Core fallback.
 *
 *  @param[in]  instance  Fallback processing stage instance.
 *  @param[in]  name      Processing stage name.
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  mem_pool  Memory pool handle.
 *  @param[out] status    Status code.
 */
void sac_fallback_init(void *instance, const char *name, sac_pipeline_t *pipeline, mem_pool_t *mem_pool,
                       sac_status_t *status);

/** @brief Add a mode to a fallback instance.
 *
 *  @note Modes can only be added once the pipeline associated with the fallback process has been setup.
 *
 *  @param[in]  instance  Fallback processing stage instance.
 *  @param[in]  name      Fallback mode name.
 *  @param[in]  cfg       Fallback mode configuration.
 *  @param[out] status    Status code.
 *  @return Index of the added mode.
 */
uint8_t sac_fallback_add_mode(sac_fallback_instance_t *instance, const char *name, sac_fallback_mode_cfg_t cfg,
                              sac_status_t *status);

/** @brief Assign a process to a fallback mode.
 *
 *  @param[in]  instance    Fallback instance.
 *  @param[in]  mode_index  Index of the mode to assign the process to.
 *  @param[in]  process     Process to assign to this mode.
 *  @param[out] status      Status code.
 */
void sac_fallback_mode_assign_process(sac_fallback_instance_t *instance, uint8_t mode_index, sac_processing_t *process,
                                      sac_status_t *status);

/** @brief Get the fallback mode's active state using its index.
 *
 *  @param[in]  instance    Pointer to fallback instance.
 *  @param[in]  mode_index  Fallback mode index.
 *  @param[out] status      Status code.
 *  @return True if the fallback mode is active, false otherwise.
 */
bool sac_fallback_mode_get_active_state(sac_fallback_instance_t *instance, uint8_t mode_index, sac_status_t *status);

/** @brief Sets the fallback mode's active state using its index.
 *
 *  @param[in]  instance    Pointer to fallback instance.
 *  @param[in]  mode_index  Fallback mode index.
 *  @param[in]  is_active   Whether the fallback mode is active.
 *  @param[out] status      Status code.
 */
void sac_fallback_mode_set_active_state(sac_fallback_instance_t *instance, uint8_t mode_index, bool is_active,
                                        sac_status_t *status);

/** @brief Check if a process is assigned to the current fallback mode.
 *
 *  @param[in]  instance  Fallback processing stage instance.
 *  @param[in]  process   Process to verify.
 *  @param[out] status    Status code.
 *  @return True if the process is assigned to the current fallback mode.
 */
bool sac_fallback_is_process_in_current_mode(sac_fallback_instance_t *instance, sac_processing_t *process,
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

/** @brief Get averaged link margin value from fallback module statistics.
 *
 *  @param[in]  instance  Pointer to fallback instance.
 *  @param[out] status    Status code.
 *  @return Fallback's RX link margin value.
 */
uint8_t sac_fallback_get_rx_link_margin(sac_fallback_instance_t *instance, sac_status_t *status);

/** @brief Set the received rx link margin value from the node.
 *
 *  @param[in]  instance  Pointer to fallback instance.
 *  @param[in]  rx_lm     RX link margin value.
 *  @param[out] status    Status code.
 */
void sac_fallback_set_rx_link_margin(sac_fallback_instance_t *instance, uint8_t rx_lm, sac_status_t *status);

/** @brief Get the index of the current fallback mode.
 *
 *  @param[in]  instance  Fallback processing stage instance.
 *  @param[out] status    Status code.
 *  @return The index of the current fallback mode.
 */
uint8_t sac_fallback_get_current_mode(sac_fallback_instance_t *instance, sac_status_t *status);

/** @brief Set the current fallback mode using the mode's index.
 *
 *  @param[in]  instance    Pointer to fallback instance.
 *  @param[in]  mode_index  Index of the mode to set as current.
 *  @param[out] status      Status code.
 */
void sac_fallback_set_current_mode(sac_fallback_instance_t *instance, uint8_t mode_index, sac_status_t *status);

/** @brief Get whether the fallback is in manual mode.
 *
 *  @param[in]  instance  Pointer to fallback instance.
 *  @param[out] status    Status code.
 *  @return True if manual mode is enabled, false otherwise.
 */
bool sac_fallback_get_manual_mode(sac_fallback_instance_t *instance, sac_status_t *status);

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
