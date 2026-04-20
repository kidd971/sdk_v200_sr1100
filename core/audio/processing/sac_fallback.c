/** @file  sac_fallback.c
 *  @brief SPARK Audio Core Fallback processing stage is used to manage audio fallback. It allows other processes to be
 *         gated by the state of the fallback process. The pipeline's processes can thus be dynamic and can generate
 *         multiple types of output. (ex: uncompressed, compressed, resampled, etc.)
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sac_fallback.h"
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include "sac_api.h"
#include "sac_stats.h"
#include "sac_utils.h"

/* CONSTANTS ******************************************************************/
/*! Decimal factor used for tx queue length calculation. */
#define BUF_SIZE_DECIMAL_FACTOR 10
/*! Sampling frequency in Hertz. */
#ifndef SAMPLING_FREQ_HZ
#define SAMPLING_FREQ_HZ 10
#endif
/*! Decimal factor used for CCA fail count value. */
#define CCA_DECIMAL_FACTOR 100
/*! Number of averaging samples required for fast recovery. */
#ifndef FAST_RECOVERY_SAMPLE_COUNT
#define FAST_RECOVERY_SAMPLE_COUNT 3
#endif

/* TYPES **********************************************************************/
/** @brief The audio fallback mode process list.
 */
typedef struct sac_fallback_process {
    /*! Pointer to the audio processing stage handle. */
    sac_processing_t *process;
    /*! Pointer to the next process in the list. */
    struct sac_fallback_process *next_process;
} sac_fallback_process_t;

/** @brief The audio fallback mode instance.
 */
typedef struct sac_fallback_mode {
    /* Name of the mode. */
    const char *name;
    /* Whether the mode is active or inactive. (This affect only automatic fallback mode). */
    bool is_active;
    /*! Fallback mode configuration. */
    sac_fallback_mode_cfg_t cfg;
    /*! Next mode instance. */
    struct sac_fallback_mode *next_mode;
    /*! Previous mode instance. */
    struct sac_fallback_mode *prev_mode;
    /*! List of processes associated to this mode. */
    sac_fallback_process_t *process_list;
    /*! Index of the mode. */
    uint8_t index;
    struct {
        /*! Number of times this mode was triggered. */
        uint32_t trigger_count;
        /*! Number of times we recovered to this mode. */
        uint32_t recover_count;
        /*! Number of packets processed in this mode. */
        uint64_t packet_count;
    } stats;
    struct {
        /*! Number of continuous sampling periods where the link margin must be higher than the threshold to allow
         *  recovery to the previous mode.
         */
        uint16_t recovery_good_count_threshold;
    } lm;
    struct {
        /*! Averaged number CCA fail count allowed per transmission multiplied by CCA_DECIMAL_FACTOR. */
        uint32_t recovery_fail_count_threshold;
        /*! Number of continuous sampling periods where the CCA fail count must be lower than the threshold to allow
         *  recovery to the previous mode.
         */
        uint16_t recovery_good_count_threshold;
        /*! Averaged number CCA fail count allowed per transmission multiplied by CCA_DECIMAL_FACTOR. */
        uint32_t trigger_fail_count_threshold;
        /*! Number of continuous sampling periods where the CCA fail count must be higher than the threshold to trigger
         *  the next mode.
         */
        uint16_t trigger_bad_count_threshold;
    } cca;
} sac_fallback_mode_t;

/* PRIVATE FUNCTION PROTOTYPE *************************************************/
static void init_consumer_queue_metrics(sac_fallback_instance_t *instance);
static void reset_link_stats(sac_fallback_instance_t *instance);
static void update_consumer_queue_metrics(sac_fallback_instance_t *instance);
static void update_link_stats(sac_fallback_instance_t *instance);
static bool is_link_queue_size_high(sac_fallback_instance_t *instance);
static bool is_link_cca_bad(sac_fallback_instance_t *instance);
static bool is_link_bad(sac_fallback_instance_t *instance);
static bool is_link_good(sac_fallback_instance_t *instance);
static bool is_link_good_fast_recovery(sac_fallback_instance_t *instance);
static bool is_first_mode(sac_fallback_mode_t *mode);
static bool is_last_mode(sac_fallback_mode_t *mode);
static void update_state(sac_fallback_instance_t *instance);
static void calculate_link_margin_metrics(sac_fallback_instance_t *instance);
static void calculate_cca_metrics(sac_fallback_instance_t *instance);
static void reset_peak_stats(sac_fallback_instance_t *instance);
static void recover_to_previous_mode(sac_fallback_instance_t *instance);
static void trigger_next_mode(sac_fallback_instance_t *instance);
static sac_fallback_mode_t *get_mode_from_index(sac_fallback_instance_t *instance, uint8_t mode_index);

/* PUBLIC FUNCTIONS ***********************************************************/
sac_fallback_mode_cfg_t sac_fallback_mode_get_defaults(void)
{
    sac_fallback_mode_cfg_t sac_fallback_mode_defaults = {0};

    /* Default link margin settings. */
    sac_fallback_mode_defaults.link_margin_threshold = 50;
    sac_fallback_mode_defaults.link_margin_threshold_hysteresis = 20;
    sac_fallback_mode_defaults.link_margin_good_time_sec = 5;
    /* Default cca settings. */
    sac_fallback_mode_defaults.cca_good_fail_count_threshold_perc = 5;
    sac_fallback_mode_defaults.cca_good_time_sec = 30;
    sac_fallback_mode_defaults.cca_bad_fail_count_threshold_perc = 5;
    sac_fallback_mode_defaults.cca_bad_time_sec = 0.1;
    /* Default buffer load threshold set to an average of 1.3. */
    sac_fallback_mode_defaults.consumer_buffer_load_threshold_tenths = 13;

    return sac_fallback_mode_defaults;
}

void sac_fallback_init(void *instance, const char *name, sac_pipeline_t *pipeline, mem_pool_t *mem_pool,
                       sac_status_t *status)
{
    sac_fallback_instance_t *inst = (sac_fallback_instance_t *)instance;

    *status = SAC_OK;

    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_FALLBACK_INIT_FAILURE, return);
    SAC_CHECK_STATUS(inst == NULL, status, SAC_ERR_FALLBACK_INIT_FAILURE, return);
    SAC_CHECK_STATUS(inst->connection == NULL, status, SAC_ERR_FALLBACK_INIT_FAILURE, return);

    /* Reset internal values. */
    memset(&inst->_internal, 0, sizeof(inst->_internal));
    inst->_internal.pipeline = pipeline;
    inst->_internal.sac_mem_pool = mem_pool;
    inst->_internal.name = name;

    if (inst->is_tx_device) {
        SAC_CHECK_STATUS(inst->get_tick == NULL, status, SAC_ERR_FALLBACK_INIT_FAILURE, return);
        SAC_CHECK_STATUS(inst->tick_frequency_hz == 0, status, SAC_ERR_FALLBACK_INIT_FAILURE, return);
        inst->_internal.consumer_buffer_size_tenths = pipeline->consumer->cfg.queue_size * BUF_SIZE_DECIMAL_FACTOR;

        init_consumer_queue_metrics(inst);
        reset_link_stats(inst);
    }
}

uint8_t sac_fallback_add_mode(sac_fallback_instance_t *instance, const char *name, sac_fallback_mode_cfg_t cfg,
                              sac_status_t *status)
{
    *status = SAC_OK;

    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return 0);
    SAC_CHECK_STATUS(name == NULL, status, SAC_ERR_NULL_PTR, return 0);

    /* Check if the fallback instance is initialized. */
    SAC_CHECK_STATUS(instance->_internal.pipeline == NULL, status, SAC_ERR_FALLBACK_INIT_FAILURE, return 0);
    SAC_CHECK_STATUS(instance->_internal.sac_mem_pool == NULL, status, SAC_ERR_FALLBACK_INIT_FAILURE, return 0);

    SAC_CHECK_STATUS(instance->_internal.mode_count >= SAC_FALLBACK_MAX_MODE_COUNT, status, SAC_ERR_FALLBACK_MODE_COUNT,
                     return 0);

    SAC_CHECK_STATUS(instance->is_tx_device && ((cfg.consumer_buffer_load_threshold_tenths == 0) ||
                                                (cfg.consumer_buffer_load_threshold_tenths >=
                                                 instance->_internal.consumer_buffer_size_tenths) ||
                                                (instance->_internal.consumer_buffer_size_tenths == 0)),
                     status, SAC_ERR_FALLBACK_INIT_FAILURE, return 0);

    /* Allocate mode object. */
    sac_fallback_mode_t *mode = (sac_fallback_mode_t *)mem_pool_malloc(instance->_internal.sac_mem_pool,
                                                                       sizeof(sac_fallback_mode_t));
    SAC_CHECK_STATUS(mode == NULL, status, SAC_ERR_NOT_ENOUGH_MEMORY, return 0);

    /* Initialize mode structure. */
    mode->name = name;
    mode->cfg = cfg;
    mode->is_active = true;

    if (instance->_internal.first_mode != NULL) {
        /* Check that the mode wasn't already added to the list. */
        sac_fallback_mode_t *mode_iter = instance->_internal.first_mode;

        for (uint8_t i = 0; i < instance->_internal.mode_count; i++) {
            SAC_CHECK_STATUS(mode_iter == mode, status, SAC_ERR_FALLBACK_MODE_DUPLICATE, return 0);
            mode_iter = mode_iter->next_mode;
        }
    }

    if (instance->is_tx_device) {
        mode->lm.recovery_good_count_threshold = cfg.link_margin_good_time_sec * SAMPLING_FREQ_HZ;
        mode->cca.recovery_good_count_threshold = cfg.cca_good_time_sec * SAMPLING_FREQ_HZ;
        mode->cca.trigger_bad_count_threshold = ((uint16_t)(cfg.cca_bad_time_sec * SAMPLING_FREQ_HZ));
        if (mode->cca.trigger_bad_count_threshold == 0) {
            /* Set threshold to lowest value. */
            mode->cca.trigger_bad_count_threshold = 1;
        }

        uint8_t max_try_count = instance->connection->wps_conn_handle->cca.max_try_count;

        mode->cca.recovery_fail_count_threshold = max_try_count * cfg.cca_good_fail_count_threshold_perc;
        mode->cca.trigger_fail_count_threshold = max_try_count * cfg.cca_bad_fail_count_threshold_perc;
    }

    /* Add the mode to the instance. */
    if (instance->_internal.first_mode == NULL) {
        /* This is the first mode. */
        instance->_internal.first_mode = mode;
        mode->next_mode = NULL;
        mode->prev_mode = NULL;
    } else {
        SAC_CHECK_STATUS(instance->_internal.last_mode == NULL, status, SAC_ERR_NULL_PTR, return 0);
        sac_fallback_mode_t *last_mode = instance->_internal.last_mode;

        /* Add the mode to the list. */
        last_mode->next_mode = mode;
        /* Set current last mode as the previous of this mode. */
        mode->prev_mode = instance->_internal.last_mode;
    }
    /* Set this mode as the last mode. */
    instance->_internal.last_mode = mode;
    /* Fallback starts on last mode. */
    instance->_internal.current_mode = mode;
    /* Set mode index. */
    mode->index = instance->_internal.mode_count;
    /* Increase the number of modes. */
    instance->_internal.mode_count++;

    /* Return the mode index. */
    return mode->index;
}

void sac_fallback_mode_assign_process(sac_fallback_instance_t *instance, uint8_t mode_index, sac_processing_t *process,
                                      sac_status_t *status)
{
    sac_fallback_mode_t *mode = NULL;
    sac_fallback_process_t *fallback_process = NULL;

    *status = SAC_OK;

    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return);
    SAC_CHECK_STATUS(instance->_internal.sac_mem_pool == NULL, status, SAC_ERR_NULL_PTR, return);
    SAC_CHECK_STATUS(process == NULL, status, SAC_ERR_NULL_PTR, return);
    /* Check the mode index is valid. */
    SAC_CHECK_STATUS(mode_index >= instance->_internal.mode_count, status, SAC_ERR_FALLBACK_MODE_COUNT, return);
    SAC_CHECK_STATUS(instance->_internal.current_mode == NULL, status, SAC_ERR_FALLBACK_INIT_FAILURE, return);

    /* Get the mode from the instance based on the index. */
    mode = get_mode_from_index(instance, mode_index);
    SAC_CHECK_STATUS(mode == NULL, status, SAC_ERR_FALLBACK_MODE_COUNT, return);

    /* Allocate fallback process object. */
    fallback_process = mem_pool_malloc(instance->_internal.sac_mem_pool, sizeof(sac_fallback_process_t));
    SAC_CHECK_STATUS(fallback_process == NULL, status, SAC_ERR_NOT_ENOUGH_MEMORY, return);
    fallback_process->process = process;

    /* Add to end of queue. */
    if (mode->process_list == NULL) {
        /* This is the first process. */
        mode->process_list = fallback_process;
    } else {
        sac_fallback_process_t *current_process = mode->process_list;

        /* Find last process. */
        while (current_process->next_process != NULL) {
            current_process = current_process->next_process;
        }
        current_process->next_process = fallback_process;
    }
}

bool sac_fallback_mode_get_active_state(sac_fallback_instance_t *instance, uint8_t mode_index, sac_status_t *status)
{
    sac_fallback_mode_t *mode = NULL;

    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return false);
    SAC_CHECK_STATUS(instance->_internal.current_mode == NULL, status, SAC_ERR_FALLBACK_INIT_FAILURE, return false);
    SAC_CHECK_STATUS(mode_index >= instance->_internal.mode_count, status, SAC_ERR_FALLBACK_MODE_COUNT, return false);

    mode = get_mode_from_index(instance, mode_index);
    SAC_CHECK_STATUS(mode == NULL, status, SAC_ERR_FALLBACK_MODE_COUNT, return false);

    return mode->is_active;
}

void sac_fallback_mode_set_active_state(sac_fallback_instance_t *instance, uint8_t mode_index, bool is_active,
                                        sac_status_t *status)
{
    sac_fallback_mode_t *mode = NULL;

    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return);
    SAC_CHECK_STATUS(instance->_internal.current_mode == NULL, status, SAC_ERR_FALLBACK_INIT_FAILURE, return);
    SAC_CHECK_STATUS(mode_index >= instance->_internal.mode_count, status, SAC_ERR_FALLBACK_MODE_COUNT, return);

    mode = get_mode_from_index(instance, mode_index);
    SAC_CHECK_STATUS(mode == NULL, status, SAC_ERR_FALLBACK_MODE_COUNT, return);

    mode->is_active = is_active;
}

bool sac_fallback_is_process_in_current_mode(sac_fallback_instance_t *instance, sac_processing_t *process,
                                             sac_status_t *status)
{
    /* Loop through current mode's processes. If process exist, return true. */
    sac_fallback_mode_t *current_mode = NULL;
    sac_fallback_process_t *current_process = NULL;

    *status = SAC_OK;

    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return false);
    SAC_CHECK_STATUS(process == NULL, status, SAC_ERR_NULL_PTR, return false);
    SAC_CHECK_STATUS(instance->_internal.current_mode == NULL, status, SAC_ERR_FALLBACK_INIT_FAILURE, return false);

    current_mode = instance->_internal.current_mode;

    /* If there aren't any processes in the mode, return. */
    if (current_mode->process_list == NULL) {
        return false;
    }

    current_process = current_mode->process_list;
    do {
        if (current_process->process == process) {
            return true;
        }
        current_process = current_process->next_process;
    } while (current_process != NULL);

    return false;
}

uint8_t sac_fallback_get_current_mode(sac_fallback_instance_t *instance, sac_status_t *status)
{
    sac_fallback_mode_t *current_mode = NULL;

    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return 0);
    SAC_CHECK_STATUS(instance->_internal.current_mode == NULL, status, SAC_ERR_FALLBACK_INIT_FAILURE, return 0);

    current_mode = instance->_internal.current_mode;

    return current_mode->index;
}

void sac_fallback_set_current_mode(sac_fallback_instance_t *instance, uint8_t mode_index, sac_status_t *status)
{
    uint8_t prev_mode_index = 0;
    sac_fallback_mode_t *current_mode = NULL;

    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return);
    SAC_CHECK_STATUS(instance->_internal.current_mode == NULL, status, SAC_ERR_FALLBACK_INIT_FAILURE, return);
    SAC_CHECK_STATUS(mode_index >= instance->_internal.mode_count, status, SAC_ERR_FALLBACK_MODE_COUNT, return);

    current_mode = instance->_internal.current_mode;

    if (current_mode->index != mode_index) {
        /* Save previous mode index. */
        prev_mode_index = current_mode->index;
        /* Get the new current mode's instance. */
        current_mode = instance->_internal.current_mode = get_mode_from_index(instance, mode_index);
        SAC_CHECK_STATUS(current_mode == NULL, status, SAC_ERR_FALLBACK_MODE_COUNT, return);
        /* Update Stats. */
        if (prev_mode_index < mode_index) {
            current_mode->stats.trigger_count++;
        } else {
            current_mode->stats.recover_count++;
        }
        /* Notify the application. */
        if (instance->fallback_state_change_callback != NULL) {
            instance->fallback_state_change_callback(mode_index);
        }
    }
}

uint16_t sac_fallback_process(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in,
                              uint16_t size, uint8_t *data_out, sac_status_t *status)
{
    (void)data_in;
    (void)data_out;
    (void)size;

    sac_fallback_instance_t *inst = (sac_fallback_instance_t *)instance;

    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return 0);
    SAC_CHECK_STATUS(inst->_internal.current_mode == NULL, status, SAC_ERR_FALLBACK_INIT_FAILURE, return 0);

    if (inst->is_tx_device) {
        update_state(inst);

        header->fallback = sac_fallback_get_current_mode(inst, status);
    } else {
        sac_fallback_set_current_mode(inst, header->fallback, status);
        if (*status != SAC_OK) {
            return 0;
        }
        sac_fallback_mode_t *mode = (sac_fallback_mode_t *)inst->_internal.current_mode;

        pipeline->_internal.current_sample_count = mode->cfg.sample_count;
    }

    /* Monitor total number of packet processed. */
    inst->_internal.total_packet_count++;

    /* Monitor number of packet processed in each state. */
    ((sac_fallback_mode_t *)inst->_internal.current_mode)->stats.packet_count++;

    return 0;
}

uint8_t sac_fallback_get_rx_link_margin(sac_fallback_instance_t *instance, sac_status_t *status)
{
    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return 0);

    return instance->_internal.link_margin_metrics.accumulator_average;
}

void sac_fallback_set_rx_link_margin(sac_fallback_instance_t *instance, uint8_t rx_lm, sac_status_t *status)
{
    sac_fallback_link_margin_metrics_t *link_margin_metrics = NULL;

    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return);

    link_margin_metrics = &instance->_internal.link_margin_metrics;
    link_margin_metrics->accumulator += rx_lm;
    link_margin_metrics->accumulator_count++;
    if (rx_lm < link_margin_metrics->consumer_link_margin_min_peak) {
        link_margin_metrics->consumer_link_margin_min_peak = rx_lm;
    }
}

bool sac_fallback_get_manual_mode(sac_fallback_instance_t *instance, sac_status_t *status)
{
    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return false);

    return instance->_internal.manual_mode;
}

void sac_fallback_set_manual_mode(sac_fallback_instance_t *instance, bool manual_mode_enabled, sac_status_t *status)
{
    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return);

    reset_link_stats(instance);

    instance->_internal.manual_mode = manual_mode_enabled;
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

int sac_fallback_format_stats(sac_fallback_instance_t *instance, char *buffer, uint16_t size, sac_status_t *status)
{
    sac_fallback_mode_t *current_mode = NULL;
    sac_fallback_cca_metrics_t *cca_metrics = NULL;
    sac_fallback_queue_metrics_t *consumer_queue_metrics = NULL;
    sac_fallback_link_margin_metrics_t *link_margin_metrics = NULL;
    sac_fallback_mode_t *mode = NULL;
    sac_pipeline_t *pipeline = NULL;
    int string_length = 0;
    uint8_t i = 0;

    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return 0);
    SAC_CHECK_STATUS(instance->_internal.current_mode == NULL, status, SAC_ERR_FALLBACK_INIT_FAILURE, return 0);

    current_mode = instance->_internal.current_mode;
    cca_metrics = &instance->_internal.cca_metrics;
    consumer_queue_metrics = &instance->_internal.consumer_queue_metrics;
    link_margin_metrics = &instance->_internal.link_margin_metrics;
    pipeline = instance->_internal.pipeline;

    string_length = snprintf(buffer, size,
                             "<<< %s >>>\r\n"
                             "Mode: [%u] %s\r\n",
                             instance->_internal.name, current_mode->index, current_mode->name);

    /* Print modes trigger count. */
    string_length += snprintf(buffer + string_length, size - string_length, "%14s", "Trigger Count:");
    mode = instance->_internal.first_mode;
    for (i = 0; i < instance->_internal.mode_count; i++) {
        if (mode == NULL) {
            break;
        }
        string_length += snprintf(buffer + string_length, size - string_length, " [%u] %7" PRIu32, i,
                                  mode->stats.trigger_count);
        mode = mode->next_mode;
    }
    string_length += snprintf(buffer + string_length, size - string_length, "\r\n");

    /* Print modes recover count. */
    string_length += snprintf(buffer + string_length, size - string_length, "%14s", "Recover Count:");
    mode = instance->_internal.first_mode;
    for (i = 0; i < instance->_internal.mode_count; i++) {
        if (mode == NULL) {
            break;
        }
        string_length += snprintf(buffer + string_length, size - string_length, " [%u] %7" PRIu32, i,
                                  mode->stats.recover_count);
        mode = mode->next_mode;
    }
    string_length += snprintf(buffer + string_length, size - string_length, "\r\n");

    /* Print modes usage. */
    string_length += snprintf(buffer + string_length, size - string_length, "%14s", "Mode usage:");
    mode = instance->_internal.first_mode;
    for (i = 0; i < instance->_internal.mode_count; i++) {
        if (mode == NULL) {
            break;
        }
        string_length += snprintf(buffer + string_length, size - string_length, " [%u] %6.2f%%", i,
                                  (double)mode->stats.packet_count / (double)instance->_internal.total_packet_count *
                                      100);
        mode = mode->next_mode;
    }
    string_length += snprintf(buffer + string_length, size - string_length, "\r\n");

    if (instance->is_tx_device) {
        /* Print algorithm statistics. */
        string_length +=
            snprintf(buffer + string_length, size - string_length,
                     "Metrics:\r\n"
                     "  Queue Length THRSH: %4" PRIu32 ", Average: %4u, Max Peak: %4" PRIu32 "\r\n"
                     "  Link Margin  THRSH: %4u,   Value: %4u, Min Peak: %4u\r\n"
                     "    Good Count THRSH: %4" PRIu16 ",   Value: %4" PRIu16 "\r\n"
                     "  CCA    THRSH: [%3" PRIu32 ", %3" PRIu32 "],   Value: %4" PRIu32 ", Max Peak: %4" PRIu32 "\r\n"
                     "    Good Count THRSH: %4" PRIu16 ",   Value: %4" PRIu16 "\r\n"
                     "    Bad Count  THRSH: %4" PRIu16 ",   Value: %4" PRIu16 "\r\n",
                     current_mode->cfg.consumer_buffer_load_threshold_tenths,
                     consumer_queue_metrics->queue_length_avg_tenths,
                     sac_pipeline_get_consumer_queue_peak_buffer_load(pipeline, status) * BUF_SIZE_DECIMAL_FACTOR,
                     current_mode->cfg.link_margin_threshold + current_mode->cfg.link_margin_threshold_hysteresis,
                     link_margin_metrics->accumulator_average, link_margin_metrics->consumer_link_margin_min_peak,
                     current_mode->lm.recovery_good_count_threshold, link_margin_metrics->recovery_good_count,
                     current_mode->cca.recovery_fail_count_threshold, current_mode->cca.trigger_fail_count_threshold,
                     cca_metrics->fail_count_avg, cca_metrics->consumer_cca_fail_count_peak,
                     current_mode->cca.recovery_good_count_threshold, cca_metrics->recovery_good_count,
                     current_mode->cca.trigger_bad_count_threshold, cca_metrics->trigger_bad_count);
    }

    return string_length;
}

void sac_fallback_reset_stats(sac_fallback_instance_t *instance, sac_status_t *status)
{
    sac_fallback_mode_t *mode = NULL;

    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return);
    SAC_CHECK_STATUS(instance->_internal.first_mode == NULL, status, SAC_ERR_FALLBACK_INIT_FAILURE, return);

    mode = instance->_internal.first_mode;
    do {
        memset(&mode->stats, 0, sizeof(mode->stats));
        instance->_internal.total_packet_count = 0;
        mode = mode->next_mode;
    } while (mode != NULL);

    reset_peak_stats(instance);
}

bool sac_fallback_is_tx_device(sac_fallback_instance_t *instance, sac_status_t *status)
{
    SAC_CHECK_STATUS(instance == NULL, status, SAC_ERR_NULL_PTR, return false);

    return instance->is_tx_device;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Get a mode instance from its index.
 *
 *  @param[in]  instance    Fallback processing stage instance.
 *  @param[in]  mode_index  Fallback mode index.
 *  @return Pointer to the mode instance.
 */
static sac_fallback_mode_t *get_mode_from_index(sac_fallback_instance_t *instance, uint8_t mode_index)
{
    sac_fallback_mode_t *current_mode = NULL;

    current_mode = instance->_internal.first_mode;
    do {
        if (current_mode->index == mode_index) {
            return current_mode;
        }
        current_mode = current_mode->next_mode;
    } while (current_mode != NULL);

    return NULL;
}

/** @brief Change fallback state to previous mode.
 *
 *  @param[in] instance  Pointer to fallback instance.
 */
static void recover_to_previous_mode(sac_fallback_instance_t *instance)
{
    sac_fallback_mode_t *current_mode = instance->_internal.current_mode;
    sac_fallback_mode_t *target_mode = NULL;

    /* Search for a valid previous mode (not NULL and is_active). */
    target_mode = current_mode->prev_mode;
    while (target_mode != NULL) {
        if (target_mode->is_active) {
            break;
        }
        target_mode = target_mode->prev_mode;
    }

    /* Return without changing mode if no valid previous mode found. */
    if (target_mode == NULL) {
        return;
    }

    /* Reset link stats when changing mode. */
    reset_link_stats(instance);

    /* Change the current mode. */
    instance->_internal.current_mode = target_mode;
    current_mode = instance->_internal.current_mode;
    current_mode->stats.recover_count++;

    /* Notify the application. */
    if (instance->fallback_state_change_callback != NULL) {
        instance->fallback_state_change_callback(current_mode->index);
    }
}

/** @brief Change fallback state to next mode.
 *
 *  @param[in] instance  Pointer to fallback instance.
 */
static void trigger_next_mode(sac_fallback_instance_t *instance)
{
    sac_fallback_mode_t *current_mode = instance->_internal.current_mode;
    sac_fallback_mode_t *target_mode = NULL;

    /* Search for a valid next mode (not NULL and is_active). */
    target_mode = current_mode->next_mode;
    while (target_mode != NULL) {
        if (target_mode->is_active) {
            break;
        }
        target_mode = target_mode->next_mode;
    }

    /* Return without changing mode if no valid next mode found. */
    if (target_mode == NULL) {
        return;
    }

    /* Reset link stats when changing mode. */
    reset_link_stats(instance);

    /* Change the current mode. */
    instance->_internal.current_mode = target_mode;
    current_mode = instance->_internal.current_mode;
    current_mode->stats.trigger_count++;

    /* Notify the application. */
    if (instance->fallback_state_change_callback != NULL) {
        instance->fallback_state_change_callback(current_mode->index);
    }
}

/** @brief Function for coordinator to update the fallback state machine.
 *
 *  @note  This function should be in sync with the producer packet generation.
 *
 *  @param[in]  instance  Pointer to fallback instance.
 *  @param[out] status    Status code.
 */
static void update_state(sac_fallback_instance_t *instance)
{
    sac_fallback_mode_t *current_mode = NULL;
    sac_endpoint_t *consumer = NULL;

    /* Fallback state machine only runs on the tx device. */
    if (instance->is_tx_device) {
        /* Update fallback statistics. */
        update_consumer_queue_metrics(instance);
        update_link_stats(instance);

        if (instance->_internal.manual_mode) {
            /* Disable automatic mode change. */
            return;
        }

        current_mode = instance->_internal.current_mode;
        consumer = instance->_internal.pipeline->consumer;

        if (!consumer->_internal.buffering_complete) {
            /* TX queue buffering is not complete. */
            instance->_internal.is_buffering = true;
            instance->_internal.buffering_complete_tick = 0;
        }

        /* Check next mode trigger metrics. */
        if (!is_last_mode(current_mode) && is_link_bad(instance)) {
            trigger_next_mode(instance);
            return;
        }

        /* Verify if TX Queue buffering just completed. */
        if (instance->_internal.is_buffering && consumer->_internal.buffering_complete &&
            queue_get_length(consumer->_internal.queue) <= 1) {
            /* Buffering is complete and TX queue is almost empty. */
            if (instance->_internal.buffering_complete_tick == 0) {
                /* Start tracking time since end of buffering. */
                instance->_internal.buffering_complete_tick = instance->get_tick();
                return;
            }
            /* Wait for link monitoring to be valid. */
            if ((instance->get_tick() - instance->_internal.buffering_complete_tick) >=
                (FAST_RECOVERY_SAMPLE_COUNT * instance->tick_frequency_hz / SAMPLING_FREQ_HZ)) {
                /* Buffering complete and enough time has passed to update link stats. */
                instance->_internal.is_buffering = false;

                /* Once buffering is done, allow instant recovery if link is good. */
                if (!is_first_mode(current_mode) && is_link_good_fast_recovery(instance)) {
                    recover_to_previous_mode(instance);
                    if (!is_first_mode(current_mode)) {
                        /* Check again next time for previous mode. */
                        instance->_internal.is_buffering = true;
                        /* Reset tick to force refresh of link stats. */
                        instance->_internal.buffering_complete_tick = instance->get_tick();
                    }
                }
            }
            return;
        }

        /* Check previous mode recovery metrics. */
        if (!is_first_mode(current_mode) && is_link_good(instance)) {
            recover_to_previous_mode(instance);
            return;
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

/** @brief Reset the link stats.
 */
static void reset_link_stats(sac_fallback_instance_t *instance)
{
    sac_fallback_link_margin_metrics_t *lm_metrics = &instance->_internal.link_margin_metrics;
    sac_fallback_cca_metrics_t *cca_metrics = &instance->_internal.cca_metrics;
    swc_fallback_info_t fallback_info = {0};
    swc_error_t swc_err = SWC_ERR_NONE;
    bool is_cca_bad = is_link_cca_bad(instance);

    fallback_info = swc_connection_get_fallback_info(instance->connection, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    /* Reset Link margin metrics. */
    lm_metrics->accumulator = 0;
    lm_metrics->accumulator_count = 0;
    lm_metrics->accumulator_average = 0;
    lm_metrics->recovery_good_count = 0;
    /* Reset CCA metrics. */
    cca_metrics->fail_count_avg = 0;
    cca_metrics->fail_count_current = fallback_info.cca_fail_count;
    cca_metrics->cca_event_count_current = fallback_info.cca_event_count;
    cca_metrics->fail_count_start = cca_metrics->fail_count_current;
    cca_metrics->cca_event_count_start = cca_metrics->cca_event_count_current;
    cca_metrics->trigger_bad_count = 0;
    /* If fallback is triggered by CCA, the recovery_good_count should be 0 to force a longer recovery time. */
    cca_metrics->recovery_good_count = is_cca_bad ? 0 : UINT16_MAX;
    /* Reset sampling time tracking. */
    instance->_internal.sampling_tick_start = instance->get_tick();

    reset_peak_stats(instance);
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
    sac_fallback_mode_t *current_mode = instance->_internal.current_mode;

    if (link_margin_metrics->accumulator_count == 0) {
        return;
    }

    link_margin_metrics->accumulator_average = link_margin_metrics->accumulator /
                                               link_margin_metrics->accumulator_count;
    link_margin_metrics->accumulator = 0;
    link_margin_metrics->accumulator_count = 0;
    if (link_margin_metrics->accumulator_average >=
        (current_mode->cfg.link_margin_threshold + current_mode->cfg.link_margin_threshold_hysteresis)) {
        /* Average above threshold, increment good count. */
        link_margin_metrics->recovery_good_count = (link_margin_metrics->recovery_good_count < UINT16_MAX) ?
                                                       (link_margin_metrics->recovery_good_count + 1) :
                                                       link_margin_metrics->recovery_good_count;
    } else {
        /* Below average, reset good count. */
        link_margin_metrics->recovery_good_count = 0;
    }
}

/** @brief Calculate CCA metrics.
 *
 *  @param[in] instance  Pointer to fallback instance.
 */
static void calculate_cca_metrics(sac_fallback_instance_t *instance)
{
    sac_fallback_cca_metrics_t *cca_metrics = &instance->_internal.cca_metrics;
    sac_fallback_mode_t *current_mode = instance->_internal.current_mode;
    swc_fallback_info_t fallback_info = {0};
    swc_error_t swc_err = SWC_ERR_NONE;
    uint32_t fail_count = 0;
    uint32_t event_count = 0;

    fallback_info = swc_connection_get_fallback_info(instance->connection, &swc_err);
    ASSERT_SWC_STATUS(swc_err);

    /* Calculate CCA event count value. */
    cca_metrics->cca_event_count_current = fallback_info.cca_event_count;
    if (cca_metrics->cca_event_count_current < cca_metrics->cca_event_count_start) {
        /* Adjust for roll over. */
        event_count = (UINT32_MAX - cca_metrics->cca_event_count_start) + cca_metrics->cca_event_count_current;
    } else {
        event_count = cca_metrics->cca_event_count_current - cca_metrics->cca_event_count_start;
    }
    cca_metrics->cca_event_count_start = cca_metrics->cca_event_count_current;

    /* Calculate CCA fail count value. */
    cca_metrics->fail_count_current = fallback_info.cca_fail_count;
    if (cca_metrics->fail_count_current < cca_metrics->fail_count_start) {
        /* Adjust for roll over. */
        fail_count = (UINT32_MAX - cca_metrics->fail_count_start) + cca_metrics->fail_count_current;
    } else {
        fail_count = cca_metrics->fail_count_current - cca_metrics->fail_count_start;
    }
    cca_metrics->fail_count_start = cca_metrics->fail_count_current;
    cca_metrics->fail_count_avg = ((uint64_t)fail_count * CCA_DECIMAL_FACTOR) / event_count;

    if (cca_metrics->fail_count_avg > cca_metrics->consumer_cca_fail_count_peak) {
        cca_metrics->consumer_cca_fail_count_peak = cca_metrics->fail_count_avg;
    }

    /* CCA fallback recovery. */
    if (cca_metrics->fail_count_avg < current_mode->cca.recovery_fail_count_threshold) {
        /* Average under threshold, increment good count and reset bad count. */
        cca_metrics->recovery_good_count = (cca_metrics->recovery_good_count < UINT16_MAX) ?
                                               (cca_metrics->recovery_good_count + 1) :
                                               cca_metrics->recovery_good_count;
    } else {
        cca_metrics->recovery_good_count = 0;
    }

    /* CCA fallback trigger. */
    if (cca_metrics->fail_count_avg > current_mode->cca.trigger_fail_count_threshold) {
        /* Average over threshold, increment bad count and reset good count. */
        cca_metrics->trigger_bad_count = (cca_metrics->trigger_bad_count < UINT16_MAX) ?
                                             (cca_metrics->trigger_bad_count + 1) :
                                             cca_metrics->trigger_bad_count;
    } else {
        cca_metrics->trigger_bad_count = 0;
    }
}

/** @brief Return if link queue size is high enough to trigger the next mode.
 *
 *  @param[in] instance  Pointer to fallback instance.
 *  @return True if the link queue length exceeds the threshold, false otherwise.
 */
static bool is_link_queue_size_high(sac_fallback_instance_t *instance)
{
    sac_fallback_queue_metrics_t *consumer_queue_metrics = &instance->_internal.consumer_queue_metrics;
    sac_fallback_mode_t *current_mode = instance->_internal.current_mode;

    if (current_mode == NULL) {
        return false;
    }

    return (consumer_queue_metrics->queue_length_avg_tenths > current_mode->cfg.consumer_buffer_load_threshold_tenths);
}

/** @brief Return if link CCA usage is high enough to trigger the next mode.
 *
 *  @param[in] instance  Pointer to fallback instance.
 *  @return True if the link's CCA usage has exceeded the threshold, false otherwise.
 */
static bool is_link_cca_bad(sac_fallback_instance_t *instance)
{
    sac_fallback_cca_metrics_t *cca_metrics = &instance->_internal.cca_metrics;
    sac_fallback_mode_t *current_mode = instance->_internal.current_mode;

    if (current_mode == NULL) {
        return false;
    }

    return (cca_metrics->trigger_bad_count >= current_mode->cca.trigger_bad_count_threshold);
}

/** @brief Return if link is bad enough to trigger the next mode.
 *
 *  @param[in] instance  Pointer to fallback instance.
 *  @return True if the link is bad, false otherwise.
 */
static bool is_link_bad(sac_fallback_instance_t *instance)
{
    return (is_link_queue_size_high(instance) || is_link_cca_bad(instance));
}

/** @brief Return if link is good enough to recover to the previous mode.
 *
 *  @param[in] instance  Pointer to fallback instance.
 *  @return True if the link is good, false otherwise.
 */
static bool is_link_good(sac_fallback_instance_t *instance)
{
    sac_fallback_mode_t *current_mode = instance->_internal.current_mode;

    return ((instance->_internal.link_margin_metrics.recovery_good_count >=
             current_mode->lm.recovery_good_count_threshold) &&
            (instance->_internal.cca_metrics.recovery_good_count >= current_mode->cca.recovery_good_count_threshold));
}

/** @brief Return if link is good enough for fast recovery at boot.
 *
 *  @note This function uses fixed thresholds for fast recovery right after buffering is complete.
 *
 *  @param[in] instance  Pointer to fallback instance.
 *  @return True if the link is good, false otherwise.
 */
static bool is_link_good_fast_recovery(sac_fallback_instance_t *instance)
{
    return ((instance->_internal.link_margin_metrics.recovery_good_count >= FAST_RECOVERY_SAMPLE_COUNT) &&
            (instance->_internal.cca_metrics.recovery_good_count >= FAST_RECOVERY_SAMPLE_COUNT));
}

/** @brief Check if the given fallback mode is the first mode.
 *
 *  @param[in] mode  Pointer to the fallback mode.
 *  @return True if the mode is the first mode, false otherwise.
 */
static bool is_first_mode(sac_fallback_mode_t *mode)
{
    return (mode->prev_mode == NULL);
}

/** @brief Check if the given fallback mode is the last mode.
 *
 *  @param[in] mode  Pointer to the fallback mode.
 *  @return True if the mode is the last mode, false otherwise.
 */
static bool is_last_mode(sac_fallback_mode_t *mode)
{
    return (mode->next_mode == NULL);
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
