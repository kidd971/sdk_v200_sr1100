/** @file  sac_api.c
 *  @brief SPARK Audio Core Application Programming Interface.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sac_api.h"
#include <string.h>
#include "sac_utils.h"

/* CONSTANTS ******************************************************************/
#define CDC_QUEUE_DATA_SIZE_INFLATION (SAC_MAX_CHANNEL_COUNT * SAC_WORD_SIZE_BYTE)
#define CDC_QUEUE_SIZE_INFLATION      3
#define TX_QUEUE_HIGH_LEVEL           2
/* Number of free nodes required to do audio processing. */
#define PROCESSING_NODE_COUNT 2
/* Number of free nodes required for endpoint action. */
#define EP_ACTION_NODE_COUNT 1
/* Number of free nodes required for audio process input. */
#define PROCESS_INPUT_NODE_COUNT 1

/* PRIVATE GLOBALS ************************************************************/
static mem_pool_t mem_pool;
static sac_mixer_module_t *sac_mixer_module;
static void (*enter_critical)(void);
static void (*exit_critical)(void);
static bool sac_initialized;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void init_audio_queues(sac_pipeline_t *pipeline, sac_status_t *status);
static queue_t *init_audio_free_queue(const char *queue_name, uint16_t queue_data_size, uint8_t queue_size,
                                      sac_status_t *status);
static void move_audio_packet_to_consumer_queue(sac_pipeline_t *pipeline, queue_node_t *processing_node,
                                                sac_status_t *status);
static bool is_process_exec_required(sac_processing_t *process, sac_pipeline_t *pipeline, queue_node_t *input_node,
                                     sac_status_t *status);
static queue_node_t *process_samples(sac_pipeline_t *pipeline, queue_node_t *input_node, sac_status_t *status);
static void enqueue_producer_node(sac_pipeline_t *pipeline, sac_status_t *status);
static uint16_t produce(sac_pipeline_t *pipeline, sac_status_t *status);
static uint16_t consume(sac_pipeline_t *pipeline, sac_endpoint_t *consumer, sac_status_t *status);
static void consume_no_delay(sac_pipeline_t *pipeline, sac_endpoint_t *consumer, sac_status_t *status);
static void consume_delay(sac_pipeline_t *pipeline, sac_endpoint_t *consumer, sac_status_t *status);
static queue_node_t *start_mixing_process(sac_pipeline_t *pipeline, sac_status_t *status);
static bool is_consumer_overflowing(sac_endpoint_t *consumer);
static sac_endpoint_t *find_last_endpoint(sac_endpoint_t *ep);

/* PUBLIC FUNCTIONS ***********************************************************/
void sac_init(sac_cfg_t cfg, sac_hal_t *hal, sac_status_t *status)
{
    queue_critical_cfg_t queue_critical;

    *status = SAC_OK;

    SAC_CHECK_STATUS(hal == NULL, status, SAC_ERR_NULL_PTR, return);
    SAC_CHECK_STATUS(hal->enter_critical == NULL, status, SAC_ERR_NULL_PTR, return);
    SAC_CHECK_STATUS(hal->exit_critical == NULL, status, SAC_ERR_NULL_PTR, return);

    enter_critical = hal->enter_critical;
    exit_critical = hal->exit_critical;

    queue_critical.enter_critical = hal->enter_critical;
    queue_critical.exit_critical = hal->exit_critical;

    queue_init(queue_critical);

    mem_pool_init(&mem_pool, cfg.memory_pool, cfg.memory_pool_size);

    sac_initialized = true;
}

void sac_mixer_init(sac_mixer_module_cfg_t cfg, sac_status_t *status)
{
    *status = SAC_OK;

    SAC_CHECK_STATUS(!sac_initialized, status, SAC_ERR_NOT_INIT, return);

    sac_mixer_module = sac_mixer_module_init(cfg, &mem_pool, status);
}

sac_pipeline_t *sac_pipeline_init(const char *name, sac_endpoint_t *producer, sac_pipeline_cfg_t cfg,
                                  sac_endpoint_t *consumer, sac_status_t *status)
{
    sac_pipeline_t *pipeline;

    *status = SAC_OK;

    SAC_CHECK_STATUS(!sac_initialized, status, SAC_ERR_NOT_INIT, return NULL);
    SAC_CHECK_STATUS(name == NULL, status, SAC_ERR_NULL_PTR, return NULL);
    SAC_CHECK_STATUS(producer == NULL, status, SAC_ERR_NULL_PTR, return NULL);
    SAC_CHECK_STATUS(consumer == NULL, status, SAC_ERR_NULL_PTR, return NULL);
    SAC_CHECK_STATUS((cfg.mixer_option.input_mixer_pipeline) && (cfg.mixer_option.output_mixer_pipeline), status,
                     SAC_ERR_MIXER_OPTION, return NULL);

    pipeline = mem_pool_malloc(&mem_pool, sizeof(sac_pipeline_t));
    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_NOT_ENOUGH_MEMORY, return NULL);

    pipeline->name = name;
    pipeline->producer = producer;
    pipeline->consumer = consumer;
    pipeline->cfg = cfg;

    return pipeline;
}

sac_endpoint_t *sac_endpoint_init(void *instance, const char *name, sac_endpoint_interface_t iface,
                                  sac_endpoint_cfg_t cfg, sac_status_t *status)
{
    sac_endpoint_t *endpoint;

    *status = SAC_OK;

    SAC_CHECK_STATUS(!sac_initialized, status, SAC_ERR_NOT_INIT, return NULL);
    SAC_CHECK_STATUS(name == NULL, status, SAC_ERR_NULL_PTR, return NULL);
    SAC_CHECK_STATUS(iface.action == NULL, status, SAC_ERR_NULL_PTR, return NULL);
    SAC_CHECK_STATUS(iface.start == NULL, status, SAC_ERR_NULL_PTR, return NULL);
    SAC_CHECK_STATUS(iface.stop == NULL, status, SAC_ERR_NULL_PTR, return NULL);
    SAC_CHECK_STATUS((cfg.channel_count != 1) && (cfg.channel_count != 2), status, SAC_ERR_CHANNEL_COUNT, return NULL);

    endpoint = mem_pool_malloc(&mem_pool, sizeof(sac_endpoint_t));
    SAC_CHECK_STATUS(endpoint == NULL, status, SAC_ERR_NOT_ENOUGH_MEMORY, return NULL);

    endpoint->instance = instance;
    endpoint->name = name;
    endpoint->iface = iface;
    endpoint->cfg = cfg;
    endpoint->_internal.extra_queue_size = 0;

    return endpoint;
}

sac_processing_t *sac_processing_stage_init(void *instance, const char *name, sac_processing_interface_t iface,
                                            sac_status_t *status)
{
    sac_processing_t *process;

    *status = SAC_OK;

    SAC_CHECK_STATUS(!sac_initialized, status, SAC_ERR_NOT_INIT, return NULL);
    SAC_CHECK_STATUS(name == NULL, status, SAC_ERR_NULL_PTR, return NULL);
    SAC_CHECK_STATUS(iface.process == NULL, status, SAC_ERR_NULL_PTR, return NULL);

    process = mem_pool_malloc(&mem_pool, sizeof(sac_processing_t));
    SAC_CHECK_STATUS(process == NULL, status, SAC_ERR_NOT_ENOUGH_MEMORY, return NULL);

    process->instance = instance;
    process->name = name;
    process->iface = iface;

    return process;
}

void sac_pipeline_add_processing(sac_pipeline_t *pipeline, sac_processing_t *process, sac_status_t *status)
{
    sac_processing_t *current_process;

    *status = SAC_OK;

    SAC_CHECK_STATUS(!sac_initialized, status, SAC_ERR_NOT_INIT, return);
    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_NULL_PTR, return);
    SAC_CHECK_STATUS(process == NULL, status, SAC_ERR_NULL_PTR, return);

    current_process = pipeline->process;

    if (current_process == NULL) {
        pipeline->process = process;
        return;
    }

    /* Find the last processing stage in the chain. */
    while (current_process->next_process != NULL) {
        current_process = current_process->next_process;
    }

    current_process->next_process = process;
}

void sac_pipeline_add_extra_consumer(sac_pipeline_t *pipeline, sac_endpoint_t *next_consumer, sac_status_t *status)
{
    *status = SAC_OK;

    SAC_CHECK_STATUS(!sac_initialized, status, SAC_ERR_NOT_INIT, return);
    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_NULL_PTR, return);
    SAC_CHECK_STATUS(next_consumer == NULL, status, SAC_ERR_NULL_PTR, return);

    find_last_endpoint(pipeline->consumer)->next_endpoint = next_consumer;
}

void sac_pipeline_add_extra_producer(sac_pipeline_t *pipeline, sac_endpoint_t *next_producer, sac_status_t *status)
{
    *status = SAC_OK;

    SAC_CHECK_STATUS(!sac_initialized, status, SAC_ERR_NOT_INIT, return);
    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_NULL_PTR, return);
    SAC_CHECK_STATUS(next_producer == NULL, status, SAC_ERR_NULL_PTR, return);

    find_last_endpoint(pipeline->producer)->next_endpoint = next_producer;
}

void sac_add_producer(sac_endpoint_t *main_producer, sac_endpoint_t *next_producer, sac_status_t *status)
{
    *status = SAC_OK;

    SAC_CHECK_STATUS(main_producer == NULL, status, SAC_ERR_NULL_PTR, return);
    SAC_CHECK_STATUS(next_producer == NULL, status, SAC_ERR_NULL_PTR, return);

    find_last_endpoint(main_producer)->next_endpoint = next_producer;
}

void sac_endpoint_link(sac_endpoint_t *consumer, sac_endpoint_t *producer, sac_status_t *status)
{
    *status = SAC_OK;

    if (consumer == NULL || producer == NULL) {
        *status = SAC_ERR_NULL_PTR;
    } else {
        producer->_internal.queue = consumer->_internal.queue;
        producer->_internal.free_queue = consumer->_internal.free_queue;
    }
}

void sac_pipeline_add_input_pipeline(sac_pipeline_t *pipeline, sac_pipeline_t *input_pipeline, sac_status_t *status)
{
    sac_endpoint_t *producer;

    *status = SAC_OK;

    SAC_CHECK_STATUS(!sac_initialized, status, SAC_ERR_NOT_INIT, return);
    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_NULL_PTR, return);
    SAC_CHECK_STATUS(input_pipeline == NULL, status, SAC_ERR_NULL_PTR, return);
    SAC_CHECK_STATUS(pipeline->input_pipeline[MAX_NB_OF_INPUTS - 1] != NULL, status, SAC_ERR_MAXIMUM_REACHED, return);

    producer = pipeline->producer;

    for (uint8_t i = 0; i < MAX_NB_OF_INPUTS; i++) {
        if (pipeline->input_pipeline[i] == NULL) {
            pipeline->input_pipeline[i] = input_pipeline;
            sac_endpoint_link(input_pipeline->consumer, producer, status);
            return;
        }
        producer = producer->next_endpoint; /* TODO: Add status check. */
    }
}

void sac_pipeline_setup(sac_pipeline_t *pipeline, sac_status_t *status)
{
    sac_processing_t *process;
    sac_endpoint_t *consumer;
    sac_endpoint_t *producer;

    *status = SAC_OK;

    SAC_CHECK_STATUS(!sac_initialized, status, SAC_ERR_NOT_INIT, return);
    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_NULL_PTR, return);

    process = pipeline->process;
    consumer = pipeline->consumer;
    producer = pipeline->producer;

    /* Initialize processing stages. */
    while (process != NULL) {
        if (process->iface.init != NULL) {
            process->iface.init(process->instance, process->name, pipeline, &mem_pool, status);
            if (*status != SAC_OK) {
                return;
            }
        }
        process = process->next_process;
    }

    /* Initialize audio queues. */
    init_audio_queues(pipeline, status);
    if (*status != SAC_OK) {
        return;
    }

    /* Initialize stats. */
    pipeline->_statistics.producer_buffer_size = queue_get_limit(producer->_internal.queue);
    pipeline->_statistics.consumer_buffer_size = queue_get_limit(consumer->_internal.queue);
}

void sac_pipeline_produce(sac_pipeline_t *pipeline, sac_status_t *status)
{
    sac_endpoint_t *producer;
    uint16_t size;

    *status = SAC_OK;

    SAC_CHECK_STATUS(!sac_initialized, status, SAC_ERR_NOT_INIT, return);
    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_NULL_PTR, return);

    producer = pipeline->producer;

    if (pipeline->producer->cfg.delayed_action) {
        if (producer->_internal.current_node != NULL) {
            /* Enqueue previous node.
             * -> Ignore queue full status to allow delayed endpoint to be reloaded.
             */
            enqueue_producer_node(pipeline, status);
        }
        /* Start production of next node. */
        produce(pipeline, status);
        if (*status != SAC_OK) {
            return;
        }
    } else {
        /* Start production of next node. */
        size = produce(pipeline, status);
        if (*status != SAC_OK) {
            return;
        }
        if (size > 0) {
            /* Endpoint produced the node, so enqueue it. */
            enqueue_producer_node(pipeline, status);
        } else {
            /* Error: producer returned no data, so free the current node. */
            queue_free_node(producer->_internal.current_node);
            producer->_internal.current_node = NULL;
            pipeline->_statistics.producer_packets_corrupted_count++;
        }
    }
}

void sac_pipeline_consume(sac_pipeline_t *pipeline, sac_status_t *status)
{
    sac_endpoint_t *consumer;

    *status = SAC_OK;

    SAC_CHECK_STATUS(!sac_initialized, status, SAC_ERR_NOT_INIT, return);
    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_NULL_PTR, return);

    consumer = pipeline->consumer;

    if (consumer->cfg.delayed_action) {
        consume_delay(pipeline, consumer, status);
    } else {
        do {
            if (queue_get_length(consumer->_internal.queue) > 0) {
                consume_no_delay(pipeline, consumer, status);
            }
            consumer = consumer->next_endpoint;
        } while (consumer != NULL);
    }
}

void sac_pipeline_start(sac_pipeline_t *pipeline, sac_status_t *status)
{
    *status = SAC_OK;

    SAC_CHECK_STATUS(!sac_initialized, status, SAC_ERR_NOT_INIT, return);
    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_NULL_PTR, return);

    /*
     * If buffering is enabled, the consumer will only be started once the
     * consumer queue is about to be full. Otherwise, the consumer is started
     * as soon as a packet is in the queue.
     */
    if (pipeline->cfg.do_initial_buffering) {
        pipeline->_internal.buffering_threshold = pipeline->consumer->cfg.queue_size - 1;
    } else {
        pipeline->_internal.buffering_threshold = 1;
    }

    /* Start producing samples. */
    pipeline->producer->iface.start(pipeline->producer->instance);
}

void sac_pipeline_stop(sac_pipeline_t *pipeline, sac_status_t *status)
{
    sac_endpoint_t *consumer;

    *status = SAC_OK;

    SAC_CHECK_STATUS(!sac_initialized, status, SAC_ERR_NOT_INIT, return);
    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_NULL_PTR, return);

    consumer = pipeline->consumer;

    /* Stop endpoints. */
    do {
        pipeline->consumer->iface.stop(consumer->instance);
        consumer = consumer->next_endpoint;
    } while (consumer != NULL);

    pipeline->producer->iface.stop(pipeline->producer->instance);

    /* Free current node. */
    queue_free_node(pipeline->producer->_internal.current_node);
    pipeline->producer->_internal.current_node = NULL;
}

uint32_t sac_processing_ctrl(sac_processing_t *sac_processing, sac_pipeline_t *pipeline, uint8_t cmd, uint32_t arg,
                             sac_status_t *status)
{
    *status = SAC_OK;

    SAC_CHECK_STATUS(!sac_initialized, status, SAC_ERR_NOT_INIT, return 0);

    return sac_processing->iface.ctrl(sac_processing->instance, pipeline, cmd, arg, status);
}

void sac_pipeline_process(sac_pipeline_t *pipeline, sac_status_t *status)
{
    queue_node_t *producer_node;
    queue_node_t *input_node;
    queue_node_t *output_node;
    sac_endpoint_t *consumer;
    sac_endpoint_t *producer;
    uint8_t crc;

    *status = SAC_OK;

    SAC_CHECK_STATUS(!sac_initialized, status, SAC_ERR_NOT_INIT, return);
    SAC_CHECK_STATUS(pipeline == NULL, status, SAC_ERR_NULL_PTR, return);

    consumer = pipeline->consumer;
    producer = pipeline->producer;

    /* Prevent the mixer to make the buffering before the mixing. */
    if ((!pipeline->cfg.mixer_option.input_mixer_pipeline) && (!pipeline->cfg.mixer_option.output_mixer_pipeline)) {
        do {
            if (!consumer->_internal.buffering_complete) {
                if (queue_get_length(consumer->_internal.queue) >= (pipeline->_internal.buffering_threshold)) {
                    /* Buffering threshold reached. */
                    consumer->_internal.buffering_complete = true;
                    consumer->iface.start(consumer->instance);
                }
            }
            consumer = consumer->next_endpoint;
        } while (consumer != NULL);
    }

    /*
     * If it's a Mixing Pipeline get the mixed packet of all Output Producer Endpoints.
     * Otherwise, get the packet from the pipeline's producer endpoint.
     */
    if (pipeline->cfg.mixer_option.output_mixer_pipeline) {
        input_node = start_mixing_process(pipeline, status);
        if (status != SAC_OK) {
            return;
        }
    } else {
        /* Get a node with audio samples that need processing from the producer queue. */
        producer_node = queue_dequeue_node(producer->_internal.queue);
        if (producer_node == NULL) {
            *status = SAC_WARN_NO_SAMPLES_TO_PROCESS;
            return;
        }
        input_node = queue_get_free_node(pipeline->_internal.processing_queue);
        sac_node_memcpy(input_node, producer_node->data, producer_node->data_size, status);
        /* Free producer node to avoid conflict with producer. */
        queue_free_node(producer_node);
        if (*status != SAC_OK) {
            /* Error while copying node content. */
            queue_free_node(input_node);
            return;
        }
    }

    /*
     * Check if payload size in audio header is what is expected. If not, packet may have
     * been corrupted. In which case, set it to expected value to avoid queue node overflow
     * when using this packet as data source for memcpy().
     */
    if (producer->cfg.use_encapsulation) {
        crc = sac_node_get_header(input_node)->crc4;
        sac_node_get_header(input_node)->crc4 = 0;
        sac_node_get_header(input_node)->reserved = 0;
        if (crc4itu(0, (uint8_t *)sac_node_get_header(input_node), sizeof(sac_header_t)) != crc) {
            /* Audio packet is corrupted, set it to a known value. */
            sac_node_set_payload_size(input_node, producer->cfg.audio_payload_size);
            sac_node_get_header(input_node)->fallback = 0;
            sac_node_get_header(input_node)->tx_queue_level_high = 0;
            pipeline->_statistics.producer_packets_corrupted_count++;
        }
    }

    if (pipeline->process != NULL) {
        /* Apply all processing stages on audio packet. */
        output_node = process_samples(pipeline, input_node, status);
        if (*status != SAC_OK) {
            return;
        }
    } else {
        /* No processing to be done. */
        output_node = input_node;
    }

    move_audio_packet_to_consumer_queue(pipeline, output_node, status);

    /*
     * Start the Mixer Output Pipeline as soon as the first mixed audio packet is ready.
     * The Mixer Output Pipeline consumer will never be stopped after this point
     * since the mixer will always produce audio packets to be consumed.
     */
    if (pipeline->cfg.mixer_option.output_mixer_pipeline) {
        if (consumer->_internal.buffering_complete == false) {
            consumer->_internal.buffering_complete = true;
            consumer->iface.start(consumer->instance);
        }
    }
    queue_free_node(output_node);
}

uint32_t sac_get_allocated_bytes(sac_status_t *status)
{
    *status = SAC_OK;

    SAC_CHECK_STATUS(!sac_initialized, status, SAC_ERR_NOT_INIT, return 0);

    return mem_pool_get_allocated_bytes(&mem_pool);
}

uint16_t sac_node_memcpy(queue_node_t *dest_node, uint8_t *data, uint16_t size, sac_status_t *status)
{
    SAC_CHECK_STATUS(data == NULL, status, SAC_ERR_NULL_PTR, return 0);
    SAC_CHECK_STATUS(dest_node == NULL, status, SAC_ERR_NULL_PTR, return 0);
    SAC_CHECK_STATUS(dest_node->data == NULL, status, SAC_ERR_NULL_PTR, return 0);
    SAC_CHECK_STATUS(size == 0, status, SAC_ERR_INVALID_ARG, return 0);
    SAC_CHECK_STATUS(dest_node->data_size < size, status, SAC_ERR_NODE_DATA_SIZE_TOO_SMALL, return 0);

    memcpy(dest_node->data, data, size);

    return size;
}

void sac_set_extra_queue_size(sac_endpoint_t *endpoint, uint8_t extra_queue_size, sac_status_t *status)
{
    if (endpoint->_internal.extra_queue_size > (UINT8_MAX - extra_queue_size)) {
        *status = SAC_ERR_MAXIMUM_REACHED;
        return;
    }
    endpoint->_internal.extra_queue_size += extra_queue_size;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize audio queues.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[out] status    Status code.
 */
static void init_audio_queues(sac_pipeline_t *pipeline, sac_status_t *status)
{
    sac_endpoint_t *consumer = pipeline->consumer;
    sac_endpoint_t *producer = pipeline->producer;
    uint16_t queue_data_inflation_size;
    uint16_t queue_data_size;
    uint8_t free_queue_size;
    uint8_t queue_size;

    *status = SAC_OK;

    /* Calculate required queue data inflation size. */
    queue_data_inflation_size = SAC_NODE_PAYLOAD_SIZE_VAR_SIZE;
    queue_data_inflation_size += sizeof(sac_header_t);
    queue_data_inflation_size += CDC_QUEUE_DATA_SIZE_INFLATION;

    /*
     * The processing queue needs to handle any data size. We need to use the maximum value between the producer data
     * size and the consumer data size.
     */
    if (consumer->cfg.audio_payload_size > producer->cfg.audio_payload_size) {
        queue_data_size = consumer->cfg.audio_payload_size;
    } else {
        queue_data_size = producer->cfg.audio_payload_size;
    }
    queue_data_size += queue_data_inflation_size;
    queue_data_size += sac_align_data_size(queue_data_size, uint32_t); /* Align nodes on 32bits. */

    /* Initialize processing queue. */
    pipeline->_internal.processing_queue = init_audio_free_queue("Processing Free Queue", queue_data_size,
                                                                 PROCESSING_NODE_COUNT, status);
    if (*status != SAC_OK) {
        return;
    }

    /*
     * Initialize producer queue.
     * If the producer queue is already initialized, it is linked to another endpoint.
     */
    if (producer->_internal.queue == NULL) {
        /* Calculate producer initial queue data size.  */
        queue_data_size = producer->cfg.audio_payload_size;
        queue_data_size += queue_data_inflation_size;
        queue_data_size += sac_align_data_size(queue_data_size, uint32_t); /* Align nodes on 32bits. */

        if (producer->cfg.queue_size < SAC_MIN_PRODUCER_QUEUE_SIZE) {
            producer->cfg.queue_size = SAC_MIN_PRODUCER_QUEUE_SIZE;
        }
        queue_size = producer->cfg.queue_size;
        /* Free queue is bigger to ensure produce action and audio process input can always succeed. */
        free_queue_size = queue_size + EP_ACTION_NODE_COUNT + PROCESS_INPUT_NODE_COUNT;
        /* Initialize free queue with one extra node for producer action.
         * If multiple producers are chained, they will all share this free queue.
         */
        producer->_internal.free_queue = init_audio_free_queue("Producer Free Queue", queue_data_size, free_queue_size,
                                                               status);
        if (*status != SAC_OK) {
            return;
        }

        do {
            producer->_internal.queue = mem_pool_malloc(&mem_pool, sizeof(queue_t));
            SAC_CHECK_STATUS(producer->_internal.queue == NULL, status, SAC_ERR_NOT_ENOUGH_MEMORY, return);
            /* All producers use the same memory space. */
            producer->_internal.free_queue = pipeline->producer->_internal.free_queue;
            /* Initialize queue. */
            queue_init_queue(producer->_internal.queue, queue_size, "Processing Queue");
            /* Get the next producer. */
            producer = producer->next_endpoint;
        } while (producer != NULL);
    }

    /*
     * Initialize consumer queue.
     * If the consumer queue is already initialized, it is linked to another endpoint.
     */
    if (consumer->_internal.queue == NULL) {
        /* Calculate consumer initial queue data size.  */
        queue_data_size = consumer->cfg.audio_payload_size;
        queue_data_size += queue_data_inflation_size;
        queue_data_size += sac_align_data_size(queue_data_size, uint32_t); /* Align nodes on 32bits. */

        /* Initialize consumer free queue. */
        queue_size = consumer->cfg.queue_size;
        /* Handle queue extra. */
        if (queue_size > (UINT8_MAX - consumer->_internal.extra_queue_size)) {
            *status = SAC_ERR_MAXIMUM_REACHED;
            return;
        }
        queue_size += consumer->_internal.extra_queue_size;

        /* Initialize free queue with one extra node for consumer action.
         * If multiple producers are chained, they will all share this free queue.
         */
        free_queue_size = queue_size;
        if (consumer->cfg.delayed_action) {
            free_queue_size += EP_ACTION_NODE_COUNT;
        }
        consumer->_internal.free_queue = init_audio_free_queue("Audio Buffer Free Queue", queue_data_size,
                                                               free_queue_size, status);
        if (*status != SAC_OK) {
            return;
        }

        /* Initialize consumer queues. */
        do {
            consumer->_internal.queue = mem_pool_malloc(&mem_pool, sizeof(queue_t));
            SAC_CHECK_STATUS(consumer->_internal.queue == NULL, status, SAC_ERR_NOT_ENOUGH_MEMORY, return);
            /* All consumers use the same memory space. */
            consumer->_internal.free_queue = pipeline->consumer->_internal.free_queue;
            /* Initialize queue. */
            queue_init_queue(consumer->_internal.queue, queue_size, "Audio Buffer");
            /* Get the next consumer. */
            consumer = consumer->next_endpoint;
        } while (consumer != NULL);
    }
}

/** @brief Initialize an audio free queue.
 *
 *  @param[in]  queue_name       Name of the queue.
 *  @param[in]  queue_data_size  Size in bytes of the data in a node.
 *  @param[in]  queue_size       Number of nodes in the queue.
 *  @param[out] status           Status code.
 *  @return Pointer to the initialized free queue.
 */
static queue_t *init_audio_free_queue(const char *queue_name, uint16_t queue_data_size, uint8_t queue_size,
                                      sac_status_t *status)
{
    uint8_t *pool_ptr = NULL;
    queue_t *free_queue = NULL;

    *status = SAC_OK;

    pool_ptr = mem_pool_malloc(&mem_pool, QUEUE_NB_BYTES_NEEDED(queue_size, queue_data_size));
    if (pool_ptr == NULL) {
        *status = SAC_ERR_NOT_ENOUGH_MEMORY;
        return NULL;
    }
    free_queue = mem_pool_malloc(&mem_pool, sizeof(queue_t));
    if (free_queue == NULL) {
        *status = SAC_ERR_NOT_ENOUGH_MEMORY;
        return NULL;
    }

    queue_init_pool(pool_ptr, free_queue, queue_size, queue_data_size, queue_name);

    return free_queue;
}

/** @brief Check if a consumer is overflowing.
 *
 *  @param[in] consumer  Pointer to the consumer endpoint.
 *  @return True if the consumer is full and does not have memory available to allocate.
 */
static bool is_consumer_overflowing(sac_endpoint_t *consumer)
{
    if (queue_get_length(consumer->_internal.queue) == queue_get_limit(consumer->_internal.queue)) {
        /* Consumer queue is full. */
        if (consumer->cfg.delayed_action) {
            /* Make sure at least one node is available for delayed consumption. */
            if (consumer->_internal.current_node == NULL) {
                /* Nothing is being consumed. */
                if (queue_get_length(consumer->_internal.free_queue) <= 1) {
                    return true;
                }
            } else {
                /* A node is used for consumption. */
                if (queue_get_length(consumer->_internal.free_queue) == 0) {
                    return true;
                }
            }
        } else {
            if (queue_get_length(consumer->_internal.free_queue) == 0) {
                return true;
            }
        }
    }

    return false;
}

/** @brief Copy data from a node of the producer queue to a node of the consumer queue.
 *
 *  @param[in]  pipeline         Pipeline instance.
 *  @param[in]  processing_node  Node from processing queue.
 *  @param[out] status           Status code.
 */
static void move_audio_packet_to_consumer_queue(sac_pipeline_t *pipeline, queue_node_t *processing_node,
                                                sac_status_t *status)
{
    uint16_t length;
    queue_node_t *consumer_node;
    sac_endpoint_t *consumer = pipeline->consumer;

    *status = SAC_OK;

    /* Detect overflow */
    do {
        if (is_consumer_overflowing(consumer)) {
            pipeline->_statistics.consumer_buffer_overflow_count++;
            consumer_node = queue_dequeue_node(consumer->_internal.queue);
            enter_critical();
            if (pipeline->cfg.mixer_option.output_mixer_pipeline) {
                for (uint8_t i = 0; i < MAX_NB_OF_INPUTS; i++) {
                    if (pipeline->input_pipeline[i] != NULL) {
                        pipeline->input_pipeline[i]->_internal.samples_buffered_size -= sac_node_get_payload_size(
                            consumer->_internal.current_node);
                    }
                }
            } else {
                /* FIXME: This only works for a single consumer. */
                pipeline->_internal.samples_buffered_size -= sac_node_get_payload_size(consumer_node);
            }
            exit_critical();
            queue_free_node(consumer_node);
        }
        consumer = consumer->next_endpoint;
    } while (consumer != NULL);

    /* Move audio packet into a consumer node. */
    consumer_node = queue_get_free_node(pipeline->consumer->_internal.free_queue);
    if (consumer_node == NULL) {
        *status = SAC_ERR_NULL_PTR;
        return;
    }
    sac_node_memcpy(consumer_node, processing_node->data,
                    SAC_PACKET_HEADER_OFFSET + sizeof(sac_header_t) + sac_node_get_payload_size(processing_node),
                    status);
    if (*status != SAC_OK) {
        queue_free_node(consumer_node);
        return;
    }

    /* Enqueue node for all consumers. */
    consumer = pipeline->consumer;
    do {
        queue_enqueue_node(consumer->_internal.queue, consumer_node);
        enter_critical();
        /* FIXME: This only works for a single consumer. */
        pipeline->_internal.samples_buffered_size += sac_node_get_payload_size(consumer_node);
        exit_critical();
        consumer = consumer->next_endpoint;
    } while (consumer != NULL);

    length = queue_get_length(pipeline->consumer->_internal.queue);
    if (length > pipeline->_statistics.consumer_queue_peak_buffer_load) {
        pipeline->_statistics.consumer_queue_peak_buffer_load = length;
    }
}

/** @brief Check if a process execution is required.
 *
 *  @param[in]  process   Process to check.
 *  @param[in]  pipeline  Pipeline of the process.
 *  @param[in]  node      Node to process.
 *  @param[out] status    Status code.
 *  @return True if the process execution is required.
 */
static bool is_process_exec_required(sac_processing_t *process, sac_pipeline_t *pipeline, queue_node_t *node,
                                     sac_status_t *status)
{
    /* Only run process if gate returns true or gate is NULL. */
    if (process->iface.gate == NULL) {
        return true;
    } else {
        return process->iface.gate(process->instance, pipeline, sac_node_get_header(node), sac_node_get_data(node),
                                   sac_node_get_payload_size(node), status);
    }
}

/** @brief Apply all processing stages to a producer queue node.
 *
 *  @param[in]  pipeline          Pipeline instance.
 *  @param[in]  input_node        Input node from the producer queue. This node could be shared with another pipeline,
 *                                so it should be read then freed.
 *  @param[out] status            Status code.
 *  @return Pointer to a producer queue node containing the processed data.
 */
static queue_node_t *process_samples(sac_pipeline_t *pipeline, queue_node_t *input_node, sac_status_t *status)
{
    uint16_t rv;
    queue_node_t *output_node;
    sac_processing_t *process = pipeline->process;

    *status = SAC_OK;

    do {
        if (is_process_exec_required(process, pipeline, input_node, status)) {
            if (*status != SAC_OK) {
                queue_free_node(input_node);
                return NULL;
            }

            /* Get a process destination node. */
            output_node = queue_get_free_node(pipeline->_internal.processing_queue);
            if (output_node == NULL) {
                *status = SAC_WARN_PROCESSING_Q_EMPTY;
                queue_free_node(input_node);
                return NULL;
            }

            rv = process->iface.process(process->instance, pipeline, sac_node_get_header(input_node),
                                        sac_node_get_data(input_node), sac_node_get_payload_size(input_node),
                                        sac_node_get_data(output_node), status);
            if (*status != SAC_OK) {
                queue_free_node(input_node);
                queue_free_node(output_node);
                return NULL;
            }
            if (rv != 0) { /* != 0 means processing happened. */
                /* Copy the header from the input node. */
                memcpy(sac_node_get_header(output_node), sac_node_get_header(input_node), sizeof(sac_header_t));
                /* Free input node. If the node is shared, it will stay in the other queue and won't go back to the
                 *  free queue yet.
                 */
                queue_free_node(input_node);
                /* Update the size. */
                sac_node_set_payload_size(output_node, rv);
                /* Swap input_node and output_node. */
                input_node = output_node;
            } else {
                queue_free_node(output_node);
            }
            output_node = NULL;
        }
        process = process->next_process;
    } while (process != NULL);

    return input_node;
}

/** @brief Enqueue the current producer queue node.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[out] status    Status code.
 */
static void enqueue_producer_node(sac_pipeline_t *pipeline, sac_status_t *status)
{
    sac_endpoint_t *producer = pipeline->producer;
    queue_node_t *current_node = pipeline->producer->_internal.current_node;
    *status = SAC_OK;

    if (producer->cfg.use_encapsulation) {
        /* If produced audio is encapsulated, save the payload size locally. */
        sac_node_set_payload_size(current_node, sac_node_get_header(current_node)->payload_size);
    }

    /* There should at least be one node available in the free queue. If not, one of the producers has not processed the
     * previous node yet. We need to drop the current node.
     */
    do {
        /* Check if producers are full. */
        if (queue_get_length(producer->_internal.queue) >= queue_get_limit(producer->_internal.queue)) {
            /* Remove old packets to make place for new ones. */
            *status = SAC_WARN_PRODUCER_Q_FULL;
            pipeline->_statistics.producer_buffer_overflow_count++;
            queue_free_node(queue_dequeue_node(producer->_internal.queue));
        }
        producer = producer->next_endpoint;
    } while (producer != NULL);

    /* Enqueue node. */
    producer = pipeline->producer;
    do {
        queue_enqueue_node(producer->_internal.queue, current_node);
        producer = producer->next_endpoint;
    } while (producer != NULL);

    /* The current node is no longer been used by the producer. */
    pipeline->producer->_internal.current_node = NULL;
}

/** @brief Get a free producer queue node and apply the producer endpoint action.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[out] status    Status code.
 *  @return The amount of bytes produces.
 */
static uint16_t produce(sac_pipeline_t *pipeline, sac_status_t *status)
{
    sac_endpoint_t *producer = pipeline->producer;
    uint8_t *payload;
    uint16_t payload_size;

    *status = SAC_OK;

    producer->_internal.current_node = queue_get_free_node(producer->_internal.free_queue);
    if (producer->_internal.current_node == NULL) {
        pipeline->_statistics.producer_buffer_overflow_count++;
        *status = SAC_WARN_PRODUCER_Q_FULL;
        return 0;
    }

    payload_size = producer->cfg.audio_payload_size;
    if (producer->cfg.use_encapsulation) {
        payload = (uint8_t *)sac_node_get_header(producer->_internal.current_node);
        payload_size += sizeof(sac_header_t);
    } else {
        payload = (uint8_t *)sac_node_get_data(producer->_internal.current_node);
        sac_node_set_payload_size(producer->_internal.current_node, payload_size);
    }

    return producer->iface.action(producer->instance, payload, payload_size);
}

/** @brief Apply the consumer endpoint action on the current node.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  consumer  Pointer to the consumer endpoint.
 *  @param[out] status    Status code.
 *  @return The amount of bytes consumed.
 */
static uint16_t consume(sac_pipeline_t *pipeline, sac_endpoint_t *consumer, sac_status_t *status)
{
    uint8_t *payload;
    uint16_t payload_size, crc;

    *status = SAC_OK;

    if (consumer->_internal.current_node == NULL) {
        pipeline->_statistics.consumer_buffer_underflow_count++;
        consumer->_internal.buffering_complete = false;
        *status = SAC_WARN_CONSUMER_Q_EMPTY;
        return 0;
    } else {
        payload_size = sac_node_get_payload_size(consumer->_internal.current_node);
        if (consumer->cfg.use_encapsulation) {
            payload = (uint8_t *)sac_node_get_header(consumer->_internal.current_node);
            /* Update audio header's payload size before sending the packet. */
            sac_node_get_header(consumer->_internal.current_node)->payload_size = (uint8_t)payload_size;
            payload_size += sizeof(sac_header_t);
            ((sac_header_t *)payload)->tx_queue_level_high =
                (queue_get_length(consumer->_internal.queue) < TX_QUEUE_HIGH_LEVEL) ? 0 : 1;

            /* Update CRC. */
            ((sac_header_t *)payload)->crc4 = 0;
            ((sac_header_t *)payload)->reserved = 0;
            crc = crc4itu(0, payload, sizeof(sac_header_t));
            ((sac_header_t *)payload)->crc4 = crc;
        } else {
            payload = sac_node_get_data(consumer->_internal.current_node);
        }
    }

    return consumer->iface.action(consumer->instance, payload, payload_size);
}

/** @brief Execute the specified not delayed action consumer endpoint.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  consumer  Consumer instance.
 *  @param[out] status    Status code.
 */
static void consume_no_delay(sac_pipeline_t *pipeline, sac_endpoint_t *consumer, sac_status_t *status)
{
    uint16_t size;
    queue_node_t *node;

    *status = SAC_OK;

    if (consumer->_internal.buffering_complete == false) {
        *status = SAC_WARN_BUFFERING_NOT_COMPLETE;
        return;
    }

    /* Get the next node, if available, without dequeuing. */
    consumer->_internal.current_node = queue_get_node(consumer->_internal.queue);
    /* Start consumption of the node. */
    size = consume(pipeline, consumer, status);
    if (size > 0) {
        /* Consumed successfully, so dequeue and free. */
        node = queue_dequeue_node(consumer->_internal.queue);
        enter_critical();
        pipeline->_internal.samples_buffered_size -= sac_node_get_payload_size(node);
        exit_critical();
        queue_free_node(node);
    }
    consumer->_internal.current_node = NULL;
}

/** @brief Execute the specified delayed action consumer endpoint.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[in]  consumer  Consumer instance.
 *  @param[out] status    Status code.
 */
static void consume_delay(sac_pipeline_t *pipeline, sac_endpoint_t *consumer, sac_status_t *status)
{
    *status = SAC_OK;

    if (consumer->_internal.buffering_complete == false) {
        *status = SAC_WARN_BUFFERING_NOT_COMPLETE;
        return;
    }

    /* Free previous node. */
    queue_free_node(consumer->_internal.current_node);
    /* Get new node. */
    consumer->_internal.current_node = queue_dequeue_node(consumer->_internal.queue);
    if (consumer->_internal.current_node != NULL) {
        enter_critical();
        if (pipeline->cfg.mixer_option.output_mixer_pipeline) {
            for (uint8_t i = 0; i < MAX_NB_OF_INPUTS; i++) {
                if (pipeline->input_pipeline[i] != NULL) {
                    pipeline->input_pipeline[i]->_internal.samples_buffered_size -= sac_node_get_payload_size(
                        consumer->_internal.current_node);
                }
            }
        } else {
            pipeline->_internal.samples_buffered_size -= sac_node_get_payload_size(consumer->_internal.current_node);
        }
        exit_critical();
    }
    /* Start consumption of new node. */
    consume(pipeline, consumer, status);
}

/** @brief Mix the producers' audio packet.
 *
 *  @param[in]  pipeline  Pipeline instance.
 *  @param[out] status    Status code.
 *  @return Pointer to a queue node containing the mixed samples.
 */
static queue_node_t *start_mixing_process(sac_pipeline_t *pipeline, sac_status_t *status)
{
    queue_node_t *temp_node;
    queue_node_t *output_node;
    sac_endpoint_t *producer = pipeline->producer;
    uint8_t producer_index = 0;

    *status = SAC_OK;

    output_node = queue_get_free_node(pipeline->_internal.processing_queue);
    if (output_node == NULL) {
        *status = SAC_WARN_PROCESSING_Q_EMPTY;
        return NULL;
    }

    /* Loop on all the Output Producer Endpoints and load the Input Samples Queues. */
    do {
        /* Loop until there are enough samples to create an audio payload. */
        do {
            /* Verify if the producer has a packet. */
            if (queue_get_length(producer->_internal.queue) > 0) {
                /* If the queue has enough samples to make a payload no need to enqueue and append. */
                if (sac_mixer_module->input_samples_queue[producer_index].current_size <
                    sac_mixer_module->cfg.payload_size) {
                    /* Dequeue the packet and append it to the Input Samples Queue. */
                    temp_node = queue_dequeue_node(producer->_internal.queue);
                    sac_mixer_module_append_samples(&sac_mixer_module->input_samples_queue[producer_index],
                                                    sac_node_get_data(temp_node), sac_node_get_payload_size(temp_node));
                    queue_free_node(temp_node);
                }
            } else {
                /* If no packet is present, append silent samples to the Input Samples Queue. */
                uint8_t silent_samples_size = sac_mixer_module->cfg.payload_size -
                                              sac_mixer_module->input_samples_queue[producer_index].current_size;

                sac_mixer_module_append_silence(&sac_mixer_module->input_samples_queue[producer_index],
                                                silent_samples_size);

                enter_critical();
                pipeline->input_pipeline[producer_index]->_internal.samples_buffered_size += silent_samples_size;
                exit_critical();
            }
        } while (sac_mixer_module->input_samples_queue[producer_index].current_size <
                 sac_mixer_module->cfg.payload_size);

        producer_index++;
        producer = producer->next_endpoint;
    } while (producer != NULL);

    /* Once the Input Samples Queues are filled, mix them into the Output Packet Queue. */
    sac_mixer_module_mix_packets(sac_mixer_module);

    /* Apply the Output Packet to the node and return it to the processing stage. */
    memcpy(sac_node_get_data(output_node), sac_mixer_module->output_packet_buffer, sac_mixer_module->cfg.payload_size);
    sac_node_set_payload_size(output_node, sac_mixer_module->cfg.payload_size);

    return output_node;
}

/** @brief Find the last endpoint in the list.
 *
 *  @param[in] ep  Top level endpoint of the lsit.
 *  @return The last endpoint in the list.
 */
static sac_endpoint_t *find_last_endpoint(sac_endpoint_t *ep)
{
    /* Find the last endpoint in the list. */
    while (ep->next_endpoint != NULL) {
        ep = ep->next_endpoint;
    }

    return ep;
}
