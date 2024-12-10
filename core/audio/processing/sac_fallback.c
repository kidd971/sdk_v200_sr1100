/** @file  sac_fallback.c
 *  @brief SPARK Audio Core Fallback processing stage is used to manage audio fallback. It allows other processes to be
 *         gated by the state of the fallback process. The pipeline's processes can thus be dynamic and can generate
 *         multiple types of output. (ex: uncompressed, compressed, resampled, etc.)
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sac_fallback.h"
#include <stddef.h>
#include <stdio.h>
#include "sac_api.h"
#include "sac_stats.h"
#include "sac_utils.h"

/* CONSTANTS ******************************************************************/
/*! Decimal factor used for tx queue length calculation. */
#define BUF_SIZE_DECIMAL_FACTOR 10
/*! Sampling frequency in Hertz. */
#define SAMPLING_FREQ_HZ 10
/*! Decimal factor used for CCA fail count value. */
#define CCA_DECIMAL_FACTOR 100

/* PRIVATE FUNCTION PROTOTYPE *************************************************/
static void init_consumer_queue_metrics(sac_fallback_instance_t *instance);
static void init_link_stats(sac_fallback_instance_t *instance);
static void update_consumer_queue_metrics(sac_fallback_instance_t *instance);
static void update_link_stats(sac_fallback_instance_t *instance);
static bool is_link_good(sac_fallback_instance_t *instance);
static void update_state(sac_fallback_instance_t *instance, sac_status_t *status);
static void calculate_link_margin_metrics(sac_fallback_instance_t *instance);
static void calculate_cca_metrics(sac_fallback_instance_t *instance);
static void reset_peak_stats(sac_fallback_instance_t *instance);

/* PUBLIC FUNCTIONS ***********************************************************/
sac_fallback_instance_t sac_fallback_get_defaults(void)
{
    sac_fallback_instance_t sac_fallback_defaults = {0};

    /* Default to rx device. */
    sac_fallback_defaults.is_tx_device = false;
    /* Default link margin settings. */
    sac_fallback_defaults.link_margin_threshold = 50;
    sac_fallback_defaults.link_margin_threshold_hysteresis = 20;
    sac_fallback_defaults.link_margin_good_time_sec = 5;
    /* Default cca settings. */
    sac_fallback_defaults.cca_max_try_count = 0;
    sac_fallback_defaults.cca_try_count_threshold_perc = 5;
    sac_fallback_defaults.cca_good_time_sec = 30;
    sac_fallback_defaults.cca_bad_time_sec = 0.1;
    /* Default buffer load threshold set to an average of 1.3. */
    sac_fallback_defaults.consumer_buffer_load_threshold_tenths = 13;

    return sac_fallback_defaults;
}

void sac_fallback_init(void *instance, const char *name, sac_pipeline_t *pipeline, mem_pool_t *mem_pool,
                       sac_status_t *status)
{
    (void)mem_pool;

    uint32_t consumer_buffer_size_tenths;
    sac_fallback_instance_t *inst = (sac_fallback_instance_t *)instance;
    sac_fallback_cca_metrics_t *cca_metrics;
    sac_fallback_link_margin_metrics_t *link_margin_metrics;

    *status = SAC_OK;

    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_FALLBACK_INIT_FAILURE, return);
    SAC_CHECK_STATUS(inst == NULL, status, SAC_ERR_FALLBACK_INIT_FAILURE, return);
    SAC_CHECK_STATUS(inst->connection == NULL, status, SAC_ERR_FALLBACK_INIT_FAILURE, return);

    inst->_internal.pipeline = pipeline;

    /* Assign name. */
    inst->_internal.name = name;

    /* Start system in fallback mode. */
    inst->_internal.fallback_flag = true;
    inst->_internal.fallback_state = SAC_FB_STATE_FALLBACK_DISCONNECT;
    inst->_internal.fallback_count = 0;

    cca_metrics = &inst->_internal.cca_metrics;
    link_margin_metrics = &inst->_internal.link_margin_metrics;

    if (inst->is_tx_device) {
        SAC_CHECK_STATUS(inst->get_tick == NULL, status, SAC_ERR_FALLBACK_INIT_FAILURE, return);
        SAC_CHECK_STATUS(inst->tick_frequency_hz == 0, status, SAC_ERR_FALLBACK_INIT_FAILURE, return);
        /* consumer_buffer_load_threshold_tenths is x10. */
        consumer_buffer_size_tenths = pipeline->consumer->cfg.queue_size * BUF_SIZE_DECIMAL_FACTOR;
        SAC_CHECK_STATUS(((inst->consumer_buffer_load_threshold_tenths == 0) ||
                          (inst->consumer_buffer_load_threshold_tenths >= consumer_buffer_size_tenths) ||
                          (consumer_buffer_size_tenths == 0)),
                         status, SAC_ERR_FALLBACK_INIT_FAILURE, return);

        link_margin_metrics->good_count_threshold = inst->link_margin_good_time_sec * SAMPLING_FREQ_HZ;
        cca_metrics->good_count_threshold = inst->cca_good_time_sec * SAMPLING_FREQ_HZ;
        cca_metrics->bad_count_threshold = ((uint16_t)(inst->cca_bad_time_sec * SAMPLING_FREQ_HZ));
        if (cca_metrics->bad_count_threshold == 0) {
            /* Set threshold to lowest value. */
            cca_metrics->bad_count_threshold = 1;
        }
        link_margin_metrics->threshold_default = inst->link_margin_threshold;
        link_margin_metrics->threshold = inst->link_margin_threshold;
        link_margin_metrics->threshold_hysteresis = inst->link_margin_threshold_hysteresis;
        inst->_internal.consumer_buffer_size_tenths = consumer_buffer_size_tenths;
        cca_metrics->fail_count_threshold = inst->cca_max_try_count * inst->cca_try_count_threshold_perc;
        cca_metrics->good_count = cca_metrics->good_count_threshold;

        init_consumer_queue_metrics(inst);
        init_link_stats(inst);
    }
}

uint16_t sac_fallback_process(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in,
                              uint16_t size, uint8_t *data_out, sac_status_t *status)
{
    (void)data_in;
    (void)data_out;
    (void)pipeline;
    (void)size;

    sac_fallback_instance_t *inst = (sac_fallback_instance_t *)instance;

    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return 0);

    if (inst->is_tx_device) {
        update_state(inst, status);

        header->fallback = inst->_internal.fallback_flag;
    } else {
        if (header->fallback == true) {
            sac_fallback_set_fallback_flag(inst, status);
        } else {
            sac_fallback_clear_fallback_flag(inst, status);
        }
    }

    return 0;
}

void sac_fallback_set_link_margin_good_time(sac_fallback_instance_t *instance, uint32_t time, sac_status_t *status)
{
    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return);

    instance->_internal.link_margin_metrics.good_count_threshold = time * SAMPLING_FREQ_HZ;
}

void sac_fallback_set_cca_good_time(sac_fallback_instance_t *instance, uint32_t time, sac_status_t *status)
{
    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return);

    instance->_internal.cca_metrics.good_count_threshold = time * SAMPLING_FREQ_HZ;
}

void sac_fallback_set_cca_bad_time(sac_fallback_instance_t *instance, float time, sac_status_t *status)
{
    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return);

    instance->_internal.cca_metrics.bad_count_threshold = ((uint16_t)(time * SAMPLING_FREQ_HZ));
    if (instance->_internal.cca_metrics.bad_count_threshold == 0) {
        /* Set threshold to lowest value. */
        instance->_internal.cca_metrics.bad_count_threshold = 1;
    }
}

void sac_fallback_set_rx_link_margin(sac_fallback_instance_t *instance, uint8_t rx_lm, sac_status_t *status)
{
    sac_fallback_link_margin_metrics_t *link_margin_metrics = &instance->_internal.link_margin_metrics;

    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return);

    link_margin_metrics->accumulator += rx_lm;
    link_margin_metrics->accumulator_count++;
    if (rx_lm < link_margin_metrics->consumer_link_margin_min_peak) {
        link_margin_metrics->consumer_link_margin_min_peak = rx_lm;
    }
}

bool sac_fallback_is_active(sac_fallback_instance_t *instance, sac_status_t *status)
{
    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return false);

    return instance->_internal.fallback_flag;
}

void sac_fallback_set_fallback_flag(sac_fallback_instance_t *instance, sac_status_t *status)
{
    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return);

    if (!instance->_internal.fallback_flag) {
        instance->_internal.fallback_flag = true;
        instance->_internal.fallback_count++;
        if (instance->fallback_state_change_callback != NULL) {
            instance->fallback_state_change_callback(instance->_internal.fallback_flag);
        }
    }
}

void sac_fallback_clear_fallback_flag(sac_fallback_instance_t *instance, sac_status_t *status)
{
    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return);

    if (instance->_internal.fallback_flag) {
        instance->_internal.fallback_flag = false;
        if (instance->fallback_state_change_callback != NULL) {
            instance->fallback_state_change_callback(instance->_internal.fallback_flag);
        }
    }
}

void sac_fallback_set_manual_mode(sac_fallback_instance_t *instance, bool manual_mode_enabled, sac_status_t *status)
{
    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return);

    instance->_internal.manual_mode = manual_mode_enabled;
}

uint32_t sac_fallback_get_fallback_count(sac_fallback_instance_t *instance, sac_status_t *status)
{
    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return 0);

    return instance->_internal.fallback_count;
}

sac_fallback_cca_metrics_t *sac_fallback_get_cca_metrics(sac_fallback_instance_t *instance, sac_status_t *status)
{
    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return NULL);

    return &instance->_internal.cca_metrics;
}

sac_fallback_link_margin_metrics_t *sac_fallback_get_link_margin_metrics(sac_fallback_instance_t *instance,
                                                                         sac_status_t *status)
{
    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return NULL);

    return &instance->_internal.link_margin_metrics;
}

uint8_t sac_fallback_get_rx_link_margin(sac_fallback_instance_t *instance, sac_status_t *status)
{
    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return 0);

    return instance->_internal.link_margin_metrics.accumulator_average;
}

int sac_fallback_format_stats(sac_fallback_instance_t *instance, char *buffer, uint16_t size, sac_status_t *status)
{
    sac_fallback_cca_metrics_t *cca_metrics;
    sac_fallback_queue_metrics_t *consumer_queue_metrics;
    sac_fallback_link_margin_metrics_t *link_margin_metrics;
    sac_pipeline_t *pipeline;
    int string_length = 0;

    const char *is_active_str = "Fallback State";
    const char *activation_count_str = "Fallback Activation Count";
    const char *queue_avg_str = "Queue Length Average";
    const char *queue_peak_str = "Queue Length Max Peak";
    const char *queue_thr_str = "Queue Length Threshold";
    const char *link_margin_str = "Link Margin Value";
    const char *link_margin_peak_str = "Link Margin Min Peak";
    const char *link_margin_thr_str = "Link Margin Threshold";
    const char *cca_fail_count_str = "CCA Fail Count Value";
    const char *cca_fail_peak_str = "CCA Fail Max Peak";
    const char *cca_fail_count_thr_str = "CCA Fail Count Threshold";

    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return 0);

    cca_metrics = &instance->_internal.cca_metrics;
    consumer_queue_metrics = &instance->_internal.consumer_queue_metrics;
    link_margin_metrics = &instance->_internal.link_margin_metrics;
    pipeline = instance->_internal.pipeline;

    if (instance->is_tx_device) {
        string_length = snprintf(buffer, size,
                                 "<<< %s >>>\r\n"
                                 "  %s:\t\t%10s\r\n"
                                 "  %s:\t%10lu\r\n"
                                 "Consumer queue\r\n"
                                 "  %s:\t\t%10i\r\n"
                                 "  %s:\t%10lu\r\n"
                                 "  %s:\t%10lu\r\n"
                                 "Link Margin\r\n"
                                 "  %s:\t\t%10i\r\n"
                                 "  %s:\t\t%10i\r\n"
                                 "  %s:\t%10i\r\n"
                                 "Clear Channel Assessment\r\n"
                                 "  %s:\t\t%10lu\r\n"
                                 "  %s:\t\t%10lu\r\n"
                                 "  %s:\t%10lu\r\n",
                                 instance->_internal.name, is_active_str,
                                 instance->_internal.fallback_flag ? "Active" : "Inactive", activation_count_str,
                                 instance->_internal.fallback_count, queue_avg_str,
                                 consumer_queue_metrics->queue_length_avg_tenths, queue_peak_str,
                                 sac_pipeline_get_consumer_queue_peak_buffer_load(pipeline) * BUF_SIZE_DECIMAL_FACTOR,
                                 queue_thr_str, instance->consumer_buffer_load_threshold_tenths, link_margin_str,
                                 link_margin_metrics->accumulator_average, link_margin_peak_str,
                                 link_margin_metrics->consumer_link_margin_min_peak, link_margin_thr_str,
                                 link_margin_metrics->threshold, cca_fail_count_str, cca_metrics->fail_count_avg,
                                 cca_fail_peak_str, cca_metrics->consumer_cca_fail_count_peak, cca_fail_count_thr_str,
                                 cca_metrics->fail_count_threshold);
    } else {
        string_length = snprintf(buffer, size,
                                 "<<< %s >>>\r\n"
                                 "  %s:\t\t%10s\r\n",
                                 instance->_internal.name, is_active_str,
                                 instance->_internal.fallback_flag ? "Active" : "Inactive");
    }

    return string_length;
}

void sac_fallback_reset_stats(sac_fallback_instance_t *instance, sac_status_t *status)
{
    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return);

    instance->_internal.fallback_count = 0;

    reset_peak_stats(instance);
}

bool sac_fallback_is_tx_device(sac_fallback_instance_t *instance, sac_status_t *status)
{
    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return false);

    return instance->is_tx_device;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Function for coordinator to update the fallback state machine.
 *
 *  @note  This function should be in sync with the producer packet generation.
 *
 *  @param[in]  instance  Pointer to fallback instance.
 *  @param[out] status    Status code.
 */
static void update_state(sac_fallback_instance_t *instance, sac_status_t *status)
{
    sac_fallback_cca_metrics_t *cca_metrics;
    sac_fallback_queue_metrics_t *consumer_queue_metrics;
    sac_fallback_link_margin_metrics_t *link_margin_metrics;

    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return);

    cca_metrics = &instance->_internal.cca_metrics;
    consumer_queue_metrics = &instance->_internal.consumer_queue_metrics;
    link_margin_metrics = &instance->_internal.link_margin_metrics;

    if (instance->_internal.manual_mode) {
        /* Clear the stats. */
        init_link_stats(instance);
        /* Update the fallback state. */
        if (instance->_internal.fallback_flag) {
            instance->_internal.fallback_state = SAC_FB_STATE_FALLBACK;
        } else {
            instance->_internal.fallback_state = SAC_FB_STATE_NORMAL;
        }
        return;
    }

    if (instance->is_tx_device) {
        /* Fallback state machine only runs on the tx device. */
        update_consumer_queue_metrics(instance);
        update_link_stats(instance);

        /* Fallback state machine only runs on the tx device. */
        switch (instance->_internal.fallback_state) {
        case SAC_FB_STATE_NORMAL:
            if (consumer_queue_metrics->queue_length_avg_tenths == instance->_internal.consumer_buffer_size_tenths) {
                /* TX Queue is full => Link is disconnected. */
                link_margin_metrics->threshold = link_margin_metrics->threshold_default;
                init_link_stats(instance); /* Clear the lm stats since they are from non-fallback mode. */
                sac_fallback_set_fallback_flag(instance, status);
                instance->_internal.fallback_state = SAC_FB_STATE_FALLBACK_DISCONNECT;
            } else if ((consumer_queue_metrics->queue_length_avg_tenths >
                        instance->consumer_buffer_load_threshold_tenths) &&
                       !instance->_internal.fallback_flag) {
                /* Buffer load threshold has been reached. Measure the current link margin to use it as a threshold. */
                init_link_stats(instance); /* Clear the lm stats since they are from non-fallback mode. */
                sac_fallback_set_fallback_flag(instance, status);
                instance->_internal.fallback_state = SAC_FB_STATE_WAIT_THRESHOLD;
            } else if (cca_metrics->bad_count >= cca_metrics->bad_count_threshold) {
                /* CCA try count is too high. */
                link_margin_metrics->threshold = link_margin_metrics->threshold_default;
                init_link_stats(instance); /* Clear the lm stats since they are from non-fallback mode. */
                sac_fallback_set_fallback_flag(instance, status);
                instance->_internal.fallback_state = SAC_FB_STATE_WAIT_THRESHOLD;
            }
            break;
        case SAC_FB_STATE_WAIT_THRESHOLD:
            /* State entered due to a degrading link, waiting to measure return to normal threshold. */
            if (consumer_queue_metrics->queue_length_avg_tenths == instance->_internal.consumer_buffer_size_tenths) {
                /* TX Queue is full => Link is disconnected. */
                link_margin_metrics->threshold = link_margin_metrics->threshold_default;
                instance->_internal.fallback_state = SAC_FB_STATE_FALLBACK_DISCONNECT;
            } else if (link_margin_metrics->accumulator_average > 0) {
                /* Averaging is complete. Use this value as a threshold to return to normal. */
                link_margin_metrics->threshold = link_margin_metrics->accumulator_average;
                /* Make sure threshold is reasonable. */
                if ((link_margin_metrics->threshold >
                     (link_margin_metrics->threshold_default + link_margin_metrics->threshold_hysteresis)) ||
                    (link_margin_metrics->threshold <
                     (link_margin_metrics->threshold_default - link_margin_metrics->threshold_hysteresis))) {
                    link_margin_metrics->threshold = link_margin_metrics->threshold_default;
                }
                instance->_internal.fallback_state = SAC_FB_STATE_FALLBACK;
                reset_peak_stats(instance);
            }
            break;
        case SAC_FB_STATE_FALLBACK:
            /* State entered due to a degraded link. */
            if (consumer_queue_metrics->queue_length_avg_tenths == instance->_internal.consumer_buffer_size_tenths) {
                /* TX Queue is full => Link is disconnected. */
                link_margin_metrics->threshold = link_margin_metrics->threshold_default;
                instance->_internal.fallback_state = SAC_FB_STATE_FALLBACK_DISCONNECT;
            } else if (is_link_good(instance)) {
                /* LM is continuously above threshold for 3 seconds, switch to normal. */
                sac_fallback_clear_fallback_flag(instance, status);
                instance->_internal.fallback_state = SAC_FB_STATE_NORMAL;
                reset_peak_stats(instance);
            }
            break;
        case SAC_FB_STATE_FALLBACK_DISCONNECT:
            /* State entered due to a disconnected link. */
            if (is_link_good(instance)) {
                /* LM is continuously above threshold for 3 seconds, switch to normal. */
                sac_fallback_clear_fallback_flag(instance, status);
                instance->_internal.fallback_state = SAC_FB_STATE_NORMAL;
                reset_peak_stats(instance);
            }
            break;
        }
    }
}

/** @brief Clear the consumer queue metrics.
 *
 *  @param[in] instance  Pointer to fallback instance.
 */
static void init_consumer_queue_metrics(sac_fallback_instance_t *instance)
{
    memset(&instance->_internal.consumer_queue_metrics, 0, sizeof(instance->_internal.consumer_queue_metrics));
}

/** @brief Clear the link stats.
 */
static void init_link_stats(sac_fallback_instance_t *instance)
{
    sac_fallback_link_margin_metrics_t *lm_metrics = &instance->_internal.link_margin_metrics;
    sac_fallback_cca_metrics_t *cca_metrics = &instance->_internal.cca_metrics;
    swc_fallback_info_t fallback_info;
    swc_error_t swc_err;

    fallback_info = swc_connection_get_fallback_info(instance->connection, &swc_err);

    /* Reset Link margin metrics. */
    lm_metrics->accumulator = 0;
    lm_metrics->accumulator_count = 0;
    lm_metrics->accumulator_average = 0;
    lm_metrics->good_count = 0;
    /* Reset CCA metrics. */
    cca_metrics->fail_count_current = fallback_info.cca_fail_count;
    cca_metrics->cca_event_count_current = fallback_info.cca_event_count;
    cca_metrics->fail_count_start = cca_metrics->fail_count_current;
    cca_metrics->cca_event_count_start = cca_metrics->cca_event_count_current;
    /* Reset sampling time tracking. */
    instance->_internal.sampling_tick_start = instance->get_tick();
}

/** @brief Update the consumer queue metrics.
 *
 *  @param[in] instance  Pointer to fallback instance.
 */
static void update_consumer_queue_metrics(sac_fallback_instance_t *instance)
{
    sac_fallback_queue_metrics_t *metrics = &instance->_internal.consumer_queue_metrics;
    sac_pipeline_t *pipeline = instance->_internal.pipeline;

    if (pipeline->consumer->_internal.buffering_complete) {
        metrics->queue_length_sum -= metrics->queue_length_arr[metrics->queue_length_arr_idx];
        metrics->queue_length_arr[metrics->queue_length_arr_idx] = queue_get_length(
            pipeline->consumer->_internal.queue);
        metrics->queue_length_sum += metrics->queue_length_arr[metrics->queue_length_arr_idx++];
        metrics->queue_length_avg_tenths = (metrics->queue_length_sum * BUF_SIZE_DECIMAL_FACTOR) /
                                           SAC_FALLBACK_QUEUE_ARRAY_LENGTH;
        metrics->queue_length_arr_idx %= SAC_FALLBACK_QUEUE_ARRAY_LENGTH;
    }
}
/** @brief Update the link stats.
 *
 *  @param[in] instance  Pointer to fallback instance.
 */
static void update_link_stats(sac_fallback_instance_t *instance)
{
    if ((instance->get_tick() - instance->_internal.sampling_tick_start) >=
        (instance->tick_frequency_hz / SAMPLING_FREQ_HZ)) {
        /* Averaging period reached: Calculate metrics. */
        instance->_internal.sampling_tick_start = instance->get_tick();
        calculate_link_margin_metrics(instance);
        calculate_cca_metrics(instance);
    }
}

/** @brief Calculate link margin metrics.
 *
 *  @param[in] instance  Pointer to fallback instance.
 */
static void calculate_link_margin_metrics(sac_fallback_instance_t *instance)
{
    sac_fallback_link_margin_metrics_t *link_margin_metrics = &instance->_internal.link_margin_metrics;

    if (link_margin_metrics->accumulator_count == 0) {
        return;
    }

    link_margin_metrics->accumulator_average = link_margin_metrics->accumulator /
                                               link_margin_metrics->accumulator_count;
    link_margin_metrics->accumulator = 0;
    link_margin_metrics->accumulator_count = 0;
    if (link_margin_metrics->accumulator_average >=
            (link_margin_metrics->threshold + link_margin_metrics->threshold_hysteresis) &&
        instance->_internal.fallback_flag) {
        /* Average above threshold, increment good count. */
        link_margin_metrics->good_count = (link_margin_metrics->good_count + 1) <
                                                  link_margin_metrics->good_count_threshold ?
                                              (link_margin_metrics->good_count + 1) :
                                              link_margin_metrics->good_count_threshold;
    } else {
        /* Below average, reset good count. */
        link_margin_metrics->good_count = 0;
    }
}

/** @brief Calculate CCA metrics.
 *
 *  @param[in] instance  Pointer to fallback instance.
 */
static void calculate_cca_metrics(sac_fallback_instance_t *instance)
{
    sac_fallback_cca_metrics_t *cca_metrics = &instance->_internal.cca_metrics;
    swc_fallback_info_t fallback_info;
    swc_error_t swc_err;

    fallback_info = swc_connection_get_fallback_info(instance->connection, &swc_err);

    /* Calculate CCA event count value. */
    cca_metrics->cca_event_count_current = fallback_info.cca_event_count;
    if (cca_metrics->cca_event_count_current < cca_metrics->cca_event_count_start) {
        /* Adjust for roll over */
        cca_metrics->cca_event_count = (UINT32_MAX - cca_metrics->cca_event_count_start) +
                                       cca_metrics->cca_event_count_current;
    } else {
        cca_metrics->cca_event_count = cca_metrics->cca_event_count_current - cca_metrics->cca_event_count_start;
    }
    cca_metrics->cca_event_count_start = cca_metrics->cca_event_count_current;

    /* Calculate CCA fail count value. */
    cca_metrics->fail_count_current = fallback_info.cca_fail_count;
    if (cca_metrics->fail_count_current < cca_metrics->fail_count_start) {
        /* Adjust for roll over */
        cca_metrics->fail_count = (UINT32_MAX - cca_metrics->fail_count_start) + cca_metrics->fail_count_current;
    } else {
        cca_metrics->fail_count = cca_metrics->fail_count_current - cca_metrics->fail_count_start;
    }
    cca_metrics->fail_count_start = cca_metrics->fail_count_current;
    cca_metrics->fail_count_avg = ((uint64_t)cca_metrics->fail_count * CCA_DECIMAL_FACTOR) /
                                  cca_metrics->cca_event_count;

    if (cca_metrics->fail_count_avg > cca_metrics->consumer_cca_fail_count_peak) {
        cca_metrics->consumer_cca_fail_count_peak = cca_metrics->fail_count_avg;
    }

    if (cca_metrics->fail_count_avg <= cca_metrics->fail_count_threshold) {
        /* Average under threshold, increment good count and reset bad count. */
        cca_metrics->good_count = (cca_metrics->good_count + 1) < cca_metrics->good_count_threshold ?
                                      (cca_metrics->good_count + 1) :
                                      cca_metrics->good_count_threshold;
        cca_metrics->bad_count = 0;
    } else {
        /* Average over threshold, increment bad count and reset good count. */
        cca_metrics->bad_count = (cca_metrics->bad_count + 1) < cca_metrics->bad_count_threshold ?
                                     (cca_metrics->bad_count + 1) :
                                     cca_metrics->bad_count_threshold;
        cca_metrics->good_count = 0;
    }
}

/** @brief Return if link is good enough to switch to normal mode.
 *
 *  @param[in] instance  Pointer to fallback instance.
 *  @return true if the link is good, false otherwise.
 */
static bool is_link_good(sac_fallback_instance_t *instance)
{
    return ((instance->_internal.link_margin_metrics.good_count >=
             instance->_internal.link_margin_metrics.good_count_threshold) &&
            (instance->_internal.cca_metrics.good_count >= instance->_internal.cca_metrics.good_count_threshold));
}

/** @brief Reset peak value statistics.
 *
 *  @param[in] instance  Pointer to fallback instance.
 */
static void reset_peak_stats(sac_fallback_instance_t *instance)
{
    instance->_internal.link_margin_metrics.consumer_link_margin_min_peak = UINT8_MAX;
    instance->_internal.cca_metrics.consumer_cca_fail_count_peak = 0;
    instance->_internal.pipeline->_statistics.consumer_queue_peak_buffer_load = 0;
}
