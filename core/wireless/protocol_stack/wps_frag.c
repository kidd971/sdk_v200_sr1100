/** @file  wps_frag.c
 *  @brief WPS Fragmentation module.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
               Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "wps_frag.h"
#include "wps.h"
#include "wps_config.h"

/* CONSTANTS ******************************************************************/
#define MAX_TRANSACTION_ID 32

/* TYPES **********************************************************************/
/** @brief WPS fragment transfer type.
 */
typedef enum fragment_transfer_type {
    /*! One fragment upper layer frame */
    FULL_FRAME_TRANSFER_TYPE = 0b000,
    /*! First or middle fragment */
    NON_LAST_FRAGMENT_TRANSFER_TYPE = 0b010,
    /*! Last fragment */
    LAST_FRAGMENT_TRANSFER_TYPE = 0b100,
    /*! Abort messsage */
    ABORT_TRANSFER_TYPE = 0b110,
} fragment_transfer_type_t;

/* Assumes a little-endian processor. */

/** @brief WPS fragment transaction control.
 */
typedef struct transaction_control {
    /*! Frame fragment transfer type */
    uint8_t transfer_type : 3;
    /*! Transaction unique ID */
    uint8_t transaction_id : 5;
} __packed transaction_control_t;

/** @brief WPS fragment full frame fragment structure.
 */
typedef struct full_frame {
    /*! Transaction control field */
    transaction_control_t transaction_control;
} __packed full_frame_t;

/** @brief WPS fragment first fragment structure.
 */
typedef struct first_fragment {
    /*! Transaction control field */
    transaction_control_t transaction_control;
    /*! Fragment index */
    uint8_t fragment_number;
    /*! Total upper layer frame size */
    uint16_t total_upper_layer_frame_size;
} __packed first_fragment_t;

/** @brief WPS fragment middle fragment structure.
 */
typedef struct middle_fragment {
    /*! Transaction control field */
    transaction_control_t transaction_control;
    /*! Fragment index */
    uint8_t fragment_number;
} __packed middle_fragment_t;

/** @brief WPS fragment last fragment structure.
 */
typedef struct last_fragment {
    /*! Transaction control field */
    transaction_control_t transaction_control;
    /*! Fragment index */
    uint8_t fragment_number;
} __packed last_fragment_t;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static inline void send_full_frame(wps_connection_t *connection, uint8_t transaction_id, const uint8_t *payload,
                                   size_t size, wps_error_t *err);
static inline size_t send_first_fragment(wps_connection_t *connection, uint8_t transaction_id, const uint8_t **payload,
                                         size_t size, wps_error_t *err);
static inline size_t send_middle_fragment(wps_connection_t *connection, uint8_t transaction_id, const uint8_t **payload,
                                          size_t size, uint8_t fragment_number, wps_error_t *err);
static inline void send_last_fragment(wps_connection_t *connection, uint8_t transaction_id, const uint8_t **payload,
                                      size_t size, uint8_t fragment_number, wps_error_t *err);

static void wps_frag_read_process(void *conn);
static void wps_frag_read_process_fail(wps_connection_t *connection, uint8_t transaction_id);
static void wps_frag_flush_last_transaction(wps_connection_t *connection);
static void wps_overrun_process(void *conn);
static wps_rx_frame frag_read(wps_connection_t *connection, wps_error_t *err);
static uint16_t wps_read_error_flush(wps_connection_t *connection, wps_error_t *err);
static bool check_queue_space(wps_connection_t *connection, size_t size);

static void wps_frag_tx_success_callback(void *conn);
static void wps_frag_tx_dropped_callback(void *conn);
static void wps_frag_tx_fail_callback(void *conn);
/* PUBLIC FUNCTIONS ***********************************************************/
void wps_frag_init(wps_connection_t *connection, void *meta_tx_buffer, uint32_t meta_tx_size)
{
    connection->frag.enabled        = true;
    connection->frag.fragment_index = 0;
    connection->frag.enqueued_count = 0;
    connection->frag.remaining_fragment = 0;

    connection->rx_queue = &connection->frag.xlayer_queue;
    circular_queue_init(&connection->frag.meta_data_queue_tx, meta_tx_buffer, meta_tx_size,
                        sizeof(uint16_t));
    xlayer_queue_init_queue(&connection->frag.xlayer_queue, connection->xlayer_queue.max_size, "frag queue");

    wps_set_rx_success_callback(connection, wps_frag_read_process, (void *)connection);
    wps_set_event_callback(connection, wps_overrun_process, (void *)connection);
}

void wps_frag_send(wps_connection_t *connection, const uint8_t *payload, size_t size, wps_error_t *err)
{
    static uint8_t transaction_id;
    uint16_t fragment_number = 0;
    uint16_t *ptr_fragment_queue;
    *err = WPS_NO_ERROR;

    if (check_queue_space(connection, size) == false) {
        *err = WPS_QUEUE_FULL_ERROR;
        return;
    }

    if (size + sizeof(full_frame_t) <= connection->payload_size) {
        send_full_frame(connection, transaction_id, payload, size, err);
        if (*err != WPS_NO_ERROR) {
            return;
        }
    } else {

        size = send_first_fragment(connection, transaction_id, &payload, size, err);

        if (*err != WPS_NO_ERROR) {
            return;
        }
        while (size / (connection->payload_size - sizeof(middle_fragment_t))) {
            fragment_number++;
            size = send_middle_fragment(connection, transaction_id, &payload, size, fragment_number, err);
            if (*err != WPS_NO_ERROR) {
                return;
            }
        };
        fragment_number++;
        send_last_fragment(connection, transaction_id, &payload, size, fragment_number, err);
        if (*err != WPS_NO_ERROR) {
            return;
        }
    }

    if (fragment_number != 0) {
        ptr_fragment_queue = circular_queue_get_free_slot(&connection->frag.meta_data_queue_tx);
        if (ptr_fragment_queue != NULL) {
            *ptr_fragment_queue = fragment_number;
            /* Dont override remaining fragment number to send */
            if (connection->frag.remaining_fragment == 0) {
                connection->frag.remaining_fragment = *ptr_fragment_queue;
            }
            circular_queue_enqueue(&connection->frag.meta_data_queue_tx);
        }
    } else {
        ptr_fragment_queue = circular_queue_get_free_slot(&connection->frag.meta_data_queue_tx);
        if (ptr_fragment_queue != NULL) {
            *ptr_fragment_queue = 1;
            /* Dont override remaining fragment number to send */
            if (connection->frag.remaining_fragment == 0) {
                connection->frag.remaining_fragment = *ptr_fragment_queue;
            }
            circular_queue_enqueue(&connection->frag.meta_data_queue_tx);
        }
    }

    transaction_id = (transaction_id + 1) % MAX_TRANSACTION_ID;
}

wps_rx_frame wps_frag_read(wps_connection_t *connection, uint8_t *payload, size_t max_size, wps_error_t *err)
{
    frag_t *frag_connection = &connection->frag;

    uint8_t *payload_tail                      = payload;
    wps_rx_frame frame                         = wps_read(connection, err);
    transaction_control_t *transaction_control = (transaction_control_t *)frame.payload;

    wps_rx_frame frame_out = {
        .payload = payload,
        .size    = 0,
    };

    if (*err != WPS_NO_ERROR) {
        frag_connection->enqueued_count--;
        return frame_out;
    }

    /* Read first fragment */
    switch (transaction_control->transfer_type) {
    case FULL_FRAME_TRANSFER_TYPE: {
        memcpy(payload, frame.payload + sizeof(full_frame_t), frame.size - sizeof(full_frame_t));
        frame_out.size = frame.size - sizeof(full_frame_t);
        wps_read_done(connection, err);
        frag_connection->enqueued_count--;

        return frame_out;
    }
    case NON_LAST_FRAGMENT_TRANSFER_TYPE: {
        first_fragment_t *first_fragment = (first_fragment_t *)frame.payload;
        size_t fragment_size             = frame.size - sizeof(first_fragment_t);

        if (first_fragment->fragment_number != 0) {
            wps_read_error_flush(connection, err);
            if (*err == WPS_NO_ERROR) {
                *err = WPS_FRAGMENT_ERROR;
            }
            frame_out.size    = 0;
            frame_out.payload = NULL;
            frag_connection->enqueued_count--;
            return frame_out;
        }

        frame_out.size = first_fragment->total_upper_layer_frame_size;

        if (frame_out.size > max_size) {
            wps_read_error_flush(connection, err);
            if (*err == WPS_NO_ERROR) {
                *err = WPS_WRONG_RX_SIZE_ERROR;
            }
            frame_out.size    = 0;
            frame_out.payload = NULL;
            frag_connection->enqueued_count--;
            return frame_out;
        }
        memcpy(payload_tail, frame.payload + sizeof(first_fragment_t), fragment_size);
        payload_tail += fragment_size;
        wps_read_done(connection, err);
        break;
    }
    default: {
        wps_read_error_flush(connection, err);
        if (*err == WPS_NO_ERROR) {
            *err = WPS_FRAGMENT_ERROR;
        }
        frame_out.size    = 0;
        frame_out.payload = NULL;
        return frame_out;
        break;
    }
    }

    /* Read the following fragments */
    do {
        frame = wps_read(connection, err);

        if (*err != WPS_NO_ERROR) {
            frame_out.size    = 0;
            frame_out.payload = NULL;
            frag_connection->enqueued_count--;
            return frame_out;
        }

        transaction_control = (transaction_control_t *)frame.payload;

        switch (transaction_control->transfer_type) {
        case NON_LAST_FRAGMENT_TRANSFER_TYPE: {
            size_t fragment_size = frame.size - sizeof(middle_fragment_t);

            memcpy(payload_tail, frame.payload + sizeof(middle_fragment_t), fragment_size);
            payload_tail += fragment_size;
            wps_read_done(connection, err);
            break;
        }
        case LAST_FRAGMENT_TRANSFER_TYPE: {
            size_t fragment_size = frame.size - sizeof(last_fragment_t);

            memcpy(payload_tail, frame.payload + sizeof(last_fragment_t), fragment_size);
            payload_tail += fragment_size;
            wps_read_done(connection, err);
            frag_connection->enqueued_count--;
            return frame_out;
        }
        default: {
            wps_read_error_flush(connection, err);
            if (*err == WPS_NO_ERROR) {
                *err = WPS_FRAGMENT_ERROR;
            }
            frame_out.size    = 0;
            frame_out.payload = NULL;
            frag_connection->enqueued_count--;
            return frame_out;
        }
        }
    } while (*err != WPS_QUEUE_EMPTY_ERROR);

    *err              = WPS_FRAGMENT_ERROR;
    frame_out.size    = 0;
    frame_out.payload = NULL;
    frag_connection->enqueued_count--;
    return frame_out;
}

uint16_t wps_frag_get_read_payload_size(wps_connection_t *connection, wps_error_t *err)
{
    frag_t *frag_connection                    = &connection->frag;
    wps_rx_frame frame                         = wps_read(connection, err);
    transaction_control_t *transaction_control = (transaction_control_t *)frame.payload;

    if (*err != WPS_NO_ERROR) {
        return 0;
    }

    /* Read first fragment */
    switch (transaction_control->transfer_type) {
    case FULL_FRAME_TRANSFER_TYPE: {
        return frame.size - sizeof(full_frame_t);
    }
    case NON_LAST_FRAGMENT_TRANSFER_TYPE: {
        first_fragment_t *first_fragment = (first_fragment_t *)frame.payload;

        if (first_fragment->fragment_number != 0) {
            uint16_t removed_frames = wps_read_error_flush(connection, err);

            if (*err == WPS_NO_ERROR) {
                *err = WPS_FRAGMENT_ERROR;
            }
            frag_connection->enqueued_count -= removed_frames;
            return 0;
        }

        return first_fragment->total_upper_layer_frame_size;
    }
    default: {
        uint16_t removed_frames = wps_read_error_flush(connection, err);

        if (*err == WPS_NO_ERROR) {
            *err = WPS_FRAGMENT_ERROR;
        }
        frag_connection->enqueued_count -= removed_frames;
        return 0;
        break;
    }
    }
}

void wps_frag_set_tx_success_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg)
{
    wps_set_tx_success_callback(connection, wps_frag_tx_success_callback, (void *)connection);
    connection->frag.tx_success_callback      = callback;
    connection->frag.tx_success_parg_callback = parg;
}

void wps_frag_set_tx_fail_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg)
{
    wps_set_tx_fail_callback(connection, wps_frag_tx_fail_callback, (void *)connection);

    connection->frag.tx_fail_callback      = callback;
    connection->frag.tx_fail_parg_callback = parg;
}

void wps_frag_set_tx_drop_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg)
{
    wps_set_tx_drop_callback(connection, wps_frag_tx_dropped_callback, (void *)connection);

    connection->frag.tx_drop_callback      = callback;
    connection->frag.tx_drop_parg_callback = parg;
}

void wps_frag_set_rx_success_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg)
{
    connection->frag.rx_success_callback      = callback;
    connection->frag.rx_success_parg_callback = parg;
}

void wps_frag_set_rx_fail_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg)
{
    connection->frag.rx_fail_callback      = callback;
    connection->frag.rx_fail_parg_callback = parg;
}

void wps_frag_set_event_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg)
{
    connection->frag.event_callback      = callback;
    connection->frag.event_parg_callback = parg;
}

uint16_t wps_frag_get_fifo_size(wps_connection_t *connection)
{
    return connection->frag.enqueued_count;
}

/* PRIVATE FUNCTION ***********************************************************/
/** @brief Send full frame fragment.
 *
 *  @note Send a one fragment upper layer frame.
 *
 *  @param[in] connection      Connection instance.
 *  @param[in] transaction_id  transaction ID.
 *  @param[in] payload         Fragment payload.
 *  @param[in] size            Fragment payload size.
 *  @param[in] err             Pointer to the error code.
 */
static inline void send_full_frame(wps_connection_t *connection, uint8_t transaction_id, const uint8_t *payload,
                                   size_t size, wps_error_t *err)
{
    uint8_t *payload_in;
    full_frame_t *fragment;

    wps_get_free_slot(connection, &payload_in, size + sizeof(full_frame_t), err);
    if (*err != WPS_NO_ERROR) {
        return;
    }
    fragment = (full_frame_t *)payload_in;

    fragment->transaction_control.transfer_type  = FULL_FRAME_TRANSFER_TYPE;
    fragment->transaction_control.transaction_id = transaction_id;

    memcpy(payload_in + sizeof(full_frame_t), payload, size);
    wps_send(connection, payload_in, size + sizeof(full_frame_t), err);
}

/** @brief Send first frame fragment.
 *
 *  @param[in] connection      Connection instance.
 *  @param[in] transaction_id  transaction ID.
 *  @param[in] payload         Fragment payload.
 *  @param[in] size            Fragment payload size.
 *  @param[in] err             Pointer to the error code.
 *  @return New payload size.
 */
static inline size_t send_first_fragment(wps_connection_t *connection, uint8_t transaction_id, const uint8_t **payload,
                                         size_t size, wps_error_t *err)
{
    uint8_t *payload_in;
    first_fragment_t *fragment;
    size_t fragment_size = connection->payload_size - sizeof(first_fragment_t);

    wps_get_free_slot(connection, &payload_in, connection->payload_size, err);
    if (*err != WPS_NO_ERROR) {
        return size;
    }
    fragment = (first_fragment_t *)payload_in;

    fragment->transaction_control.transfer_type  = NON_LAST_FRAGMENT_TRANSFER_TYPE;
    fragment->transaction_control.transaction_id = transaction_id;
    fragment->fragment_number                    = 0;
    fragment->total_upper_layer_frame_size       = size;

    memcpy(payload_in + sizeof(first_fragment_t), *payload, fragment_size);
    *payload += fragment_size;
    wps_send(connection, payload_in, connection->payload_size, err);
    if (*err != WPS_NO_ERROR) {
        return size;
    }
    return size - fragment_size;
}

/** @brief Send middle frame fragment.
 *
 *  @param[in] connection       Connection instance.
 *  @param[in] transaction_id   transaction ID.
 *  @param[in] payload          Fragment payload.
 *  @param[in] size             Fragment payload size.
 *  @param[in] fragment_number  Index of the fragment.
 *  @param[in] err              Pointer to the error code.
 *  @return New payload size.
 */
static inline size_t send_middle_fragment(wps_connection_t *connection, uint8_t transaction_id, const uint8_t **payload,
                                          size_t size, uint8_t fragment_number, wps_error_t *err)
{
    uint8_t *payload_in;
    middle_fragment_t *fragment;

    wps_get_free_slot(connection, &payload_in, connection->payload_size, err);
    if (*err != WPS_NO_ERROR) {
        return size;
    }
    fragment             = (middle_fragment_t *)payload_in;
    size_t fragment_size = connection->payload_size - sizeof(middle_fragment_t);

    fragment->transaction_control.transfer_type  = NON_LAST_FRAGMENT_TRANSFER_TYPE;
    fragment->transaction_control.transaction_id = transaction_id;
    fragment->fragment_number                    = fragment_number;

    memcpy(payload_in + sizeof(middle_fragment_t), *payload, fragment_size);
    wps_send(connection, payload_in, connection->payload_size, err);
    if (*err != WPS_NO_ERROR) {
        return size;
    }
    *payload += fragment_size;
    return size - fragment_size;
}

/** @brief Send last frame fragment.
 *
 *  @param[in] connection       Connection instance.
 *  @param[in] transaction_id  transaction ID.
 *  @param[in] payload          Fragment payload.
 *  @param[in] size             Fragment payload size.
 *  @param[in] fragment_number  Index of the fragment.
 *  @param[in] err              Pointer to the error code.
 */
static inline void send_last_fragment(wps_connection_t *connection, uint8_t transaction_id, const uint8_t **payload,
                                      size_t size, uint8_t fragment_number, wps_error_t *err)
{
    uint8_t *payload_in;
    last_fragment_t *fragment;

    wps_get_free_slot(connection, &payload_in, size + sizeof(last_fragment_t), err);
    if (*err != WPS_NO_ERROR) {
        return;
    }
    fragment = (last_fragment_t *)payload_in;

    fragment->transaction_control.transfer_type  = LAST_FRAGMENT_TRANSFER_TYPE;
    fragment->transaction_control.transaction_id = transaction_id;
    fragment->fragment_number                    = fragment_number;

    memcpy(payload_in + sizeof(last_fragment_t), *payload, size);
    wps_send(connection, payload_in, size + sizeof(last_fragment_t), err);
    if (*err != WPS_NO_ERROR) {
        return;
    }
}

/** @brief  State machine to handle new received frame.
 *
 *  @param[in] conn  Fragmentation connection instance.
 */
static void wps_frag_read_process(void *conn)
{
    wps_rx_frame frame;
    wps_error_t err;
    wps_connection_t *connection = (wps_connection_t *)conn;

    frame = frag_read(connection, &err);

    transaction_control_t *transaction_control = (transaction_control_t *)frame.payload;

    /* Flush fragment if the current frame is dropped */
    if (connection->frag.dropped_frame) {
        if (transaction_control->transfer_type != FULL_FRAME_TRANSFER_TYPE) {
            middle_fragment_t *middle_fragment = (middle_fragment_t *)frame.payload;

            if (middle_fragment->transaction_control.transaction_id != connection->frag.transaction_id) {
                wps_frag_read_process_fail(connection, middle_fragment->transaction_control.transaction_id);
                return;
            }
            if (middle_fragment->fragment_number != connection->frag.fragment_index) {
                wps_read_done(connection, &err);
                return;
            }
        } else {
            connection->frag.dropped_frame = false;
        }
    }

    switch (transaction_control->transfer_type) {
    case FULL_FRAME_TRANSFER_TYPE: {
        if (connection->frag.rx_success_callback != NULL) {
            connection->frag.enqueued_count++;
            connection->frag.rx_success_callback(connection->frag.rx_success_parg_callback);
        }
        break;
    }
    case NON_LAST_FRAGMENT_TRANSFER_TYPE: {
        middle_fragment_t *middle_fragment = (middle_fragment_t *)frame.payload;

        if (middle_fragment->fragment_number == connection->frag.fragment_index) {
            if (middle_fragment->transaction_control.transaction_id == connection->frag.transaction_id) {
                connection->frag.fragment_index++;
            } else if ((middle_fragment->fragment_number == 0)) {
                connection->frag.transaction_id = middle_fragment->transaction_control.transaction_id;
                connection->frag.fragment_index++;
            } else {
                wps_frag_read_process_fail(connection, middle_fragment->transaction_control.transaction_id);
            }
        } else if (middle_fragment->transaction_control.transaction_id != connection->frag.transaction_id) {
            wps_frag_flush_last_transaction(connection);
        } else {
            wps_frag_read_process_fail(connection, middle_fragment->transaction_control.transaction_id);
        }
        break;
    }
    case LAST_FRAGMENT_TRANSFER_TYPE: {
        last_fragment_t *last_fragment = (last_fragment_t *)frame.payload;

        if (last_fragment->fragment_number == connection->frag.fragment_index) {
            connection->frag.fragment_index = 0;
            if (connection->frag.rx_success_callback != NULL) {
                connection->frag.enqueued_count++;
                connection->frag.rx_success_callback(connection->frag.rx_success_parg_callback);
            }
        } else {
            wps_frag_read_process_fail(connection, last_fragment->transaction_control.transaction_id);
        }
        break;
    }
    default: {
        wps_frag_read_process_fail(connection, connection->frag.transaction_id);
        break;
    }
    }
}

/** @brief Handle error on frag read process.
 *
 *  @param[in] frag_connection  Fragmentation connection instance.
 */
static void wps_frag_flush_last_transaction(wps_connection_t *connection)
{
    wps_error_t err = 0;
    first_fragment_t *first_fragment;
    uint8_t transaction_id_to_flush;
    wps_rx_frame frame = wps_read(connection, &err);

    if (err == WPS_NO_ERROR) {
        first_fragment = (first_fragment_t *)frame.payload;
        transaction_id_to_flush = first_fragment->transaction_control.transaction_id;
        do {
            frame = wps_read(connection, &err);
            if (err != WPS_NO_ERROR) {
                break;
            }
            first_fragment = (first_fragment_t *)frame.payload;
            if (transaction_id_to_flush == first_fragment->transaction_control.transaction_id) {
                wps_read_done(connection, &err);
            } else {
                middle_fragment_t *middle_fragment = (middle_fragment_t *)frame.payload;

                connection->frag.transaction_id = middle_fragment->transaction_control.transaction_id;
                connection->frag.fragment_index = 1;
                break;
            }
        } while (err != WPS_QUEUE_EMPTY_ERROR);
    }
}

/** @brief Handle error on frag read process.
 *
 *  @param[in] frag_connection  Fragmentation connection instance.
 */
static void wps_frag_read_process_fail(wps_connection_t *connection, uint8_t transaction_id)
{
    wps_error_t err = 0;

    connection->frag.dropped_frame  = true;
    connection->frag.fragment_index = 0;

    first_fragment_t *first_fragment;

    do {
        wps_rx_frame frame = wps_read(connection, &err);

        if (err != WPS_NO_ERROR) {
            break;
        }
        first_fragment = (first_fragment_t *)frame.payload;
        if ((first_fragment->transaction_control.transfer_type == FULL_FRAME_TRANSFER_TYPE) ||
            ((first_fragment->transaction_control.transfer_type == NON_LAST_FRAGMENT_TRANSFER_TYPE) &&
             (first_fragment->fragment_number == 0) && (connection->frag.transaction_id != transaction_id))) {
            if (first_fragment->transaction_control.transfer_type == FULL_FRAME_TRANSFER_TYPE) {
                if (connection->frag.rx_success_callback != NULL) {
                    connection->frag.enqueued_count++;
                    connection->frag.rx_success_callback(connection->frag.rx_success_parg_callback);
                }
            } else if (first_fragment->transaction_control.transfer_type == NON_LAST_FRAGMENT_TRANSFER_TYPE) {
                middle_fragment_t *middle_fragment = (middle_fragment_t *)frame.payload;

                connection->frag.dropped_frame  = false;
                connection->frag.transaction_id = middle_fragment->transaction_control.transaction_id;
                connection->frag.fragment_index = 1;
            }
            break;
        }
        wps_read_done(connection, &err);
    } while (err != WPS_QUEUE_EMPTY_ERROR);

    if (connection->frag.rx_fail_callback != NULL) {
        connection->frag.rx_fail_callback(connection->frag.rx_fail_parg_callback);
    }
}

/** @brief Handle overrun events.
 *
 *  @param[in] conn  Fragmentation connection instance.
 */
static void wps_overrun_process(void *conn)
{
    wps_connection_t *connection = (wps_connection_t *)conn;
    wps_error_t err;

    switch (connection->wps_error) {
    case WPS_RX_OVERRUN_ERROR:
        do {
            wps_read(connection, &err);
            if (err != WPS_NO_ERROR) {
                break;
            }
            wps_read_done(connection, &err);
        } while (err != WPS_QUEUE_EMPTY_ERROR);
        break;
    default:
        break;
    }

    if (connection->frag.event_callback != NULL) {
        connection->frag.event_callback(connection->frag.event_parg_callback);
    }
    connection->wps_error = WPS_NO_ERROR;
}

/** @brief Read fragment from the fragment queue.
 *
 *  @param[in] queue  Fragment queue.
 *  @param[in] err    Pointer to the error code.
 *  @return WPS Received frame structure, including payload and size.
 */
static wps_rx_frame frag_read(wps_connection_t *connection, wps_error_t *err)
{
    wps_rx_frame frame_out;
    xlayer_queue_node_t *node;
    xlayer_frame_t *frame;

    *err = WPS_NO_ERROR;

    node  = xlayer_queue_dequeue_node(&connection->frag.xlayer_queue);
    frame = &node->xlayer.frame;

    frame_out.payload = (frame->payload_begin_it);
    frame_out.size    = frame->payload_end_it - frame->payload_begin_it;

    xlayer_queue_enqueue_node(&connection->xlayer_queue, node);

    return frame_out;
}

/** @brief Flush the frame after a read error.
 *
 *  @param[in] connection  Connection instance.
 *  @param[in] err         Pointer to the error code.
 *  @return Number of the flushed frames.
 */
static uint16_t wps_read_error_flush(wps_connection_t *connection, wps_error_t *err)
{
    transaction_control_t transaction_control;
    wps_rx_frame frame;
    uint16_t removed_frames = 0;

    do {
        frame = wps_read(connection, err);
        if (*err != WPS_NO_ERROR) {
            return removed_frames;
        }
        memcpy(&transaction_control, frame.payload, sizeof(transaction_control));
        wps_read_done(connection, err);
        if (*err != WPS_NO_ERROR) {
            break;
        }
        removed_frames++;
    } while (transaction_control.transfer_type != LAST_FRAGMENT_TRANSFER_TYPE);

    return removed_frames;
}

/** @brief Check if the payload fits in the queue.
 *
 *  @param[in] connection  Connection instance.
 *  @param[in] size        Total payload size.
 *
 *  @return True if the payload fit in queue, False otherwise.
 */
static bool check_queue_space(wps_connection_t *connection, size_t size)
{
    uint32_t free_space = wps_get_fifo_free_space(connection);
    uint32_t nb_fragment;

    if (size + sizeof(middle_fragment_t) <= connection->payload_size) {
        nb_fragment = 1;
    } else {
        nb_fragment                = (size / connection->payload_size) + 1;
        uint8_t last_fragment_size = (size % connection->payload_size) +
                                     (sizeof(middle_fragment_t) + (nb_fragment * sizeof(middle_fragment_t)));

        while (last_fragment_size > connection->payload_size) {
            nb_fragment++;
            last_fragment_size = (size % connection->payload_size) +
                                 (sizeof(middle_fragment_t) + (nb_fragment * sizeof(middle_fragment_t)));
        }
    }

    if (free_space < nb_fragment) {
        return false;
    } else {
        return true;
    }
}

/** @brief Set the TX success callback of a connection.
 *
 *  @param[in] conn  Fragmentation connection instance.
 */
static void wps_frag_tx_success_callback(void *conn)
{
    wps_connection_t *connection = (wps_connection_t *)conn;
    uint16_t *ptr_fragment_queue;

    if (connection->frag.remaining_fragment) {
        connection->frag.remaining_fragment--;
    } else {
        if (connection->frag.tx_success_callback != NULL) {
            connection->frag.tx_success_callback(connection->frag.tx_success_parg_callback);
        }
        if (circular_queue_dequeue(&connection->frag.meta_data_queue_tx)) {
            ptr_fragment_queue = circular_queue_front(&connection->frag.meta_data_queue_tx);
            if (ptr_fragment_queue != NULL) {
                connection->frag.remaining_fragment = *ptr_fragment_queue;
            }
        }
    }
}

/** @brief Set the TX drop callback of a connection.
 *
 *  @param[in] conn  Fragmentation connection instance.
 */
static void wps_frag_tx_dropped_callback(void *conn)
{
    wps_connection_t *connection = (wps_connection_t *)conn;

    if (connection->tx_drop_callback != NULL) {
        connection->tx_drop_callback(connection->tx_drop_parg_callback);
    }
}

/** @brief Set the TX fail callback of a connection.
 *
 *  @param[in] conn  Fragmentation connection instance.
 */
static void wps_frag_tx_fail_callback(void *conn)
{
    wps_connection_t *connection = (wps_connection_t *)conn;

    if (connection->tx_fail_callback != NULL) {
        connection->tx_fail_callback(connection->tx_fail_parg_callback);
    }
}
