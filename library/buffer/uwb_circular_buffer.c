/** @file  uwb_circular_buffer.c
 *  @brief Circular buffer.
 *
 *  @copyright Copyright (C) 2020 SPARK Microsystems International Inc.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include <string.h>
#include "uwb_circular_buffer.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void uwb_circ_buff_init(circ_buffer_t *buf, void *buf_ptr, uint32_t capacity, uint8_t size)
{
    buf->buf_full     = false;
    buf->buf_empty    = true;
    buf->buf_capacity = capacity;
    buf->item_size    = size;
    buf->buffer       = buf_ptr;
    buf->num_data     = 0;
    buf->free_space   = capacity;
    buf->in_idx       = buf->buffer;
    buf->out_idx      = buf->buffer;
    buf->buffer_end   = (char *)buf->buffer + capacity * size;
}

void uwb_circ_buff_in(circ_buffer_t *buf, void *data, uint32_t size, circ_buff_error_t *err)
{
    uint16_t copied_len = 0;
    uint16_t cpy_size   = (buf->item_size * size);

    *err = CIRC_BUFF_ERR_NONE;

    if (size > buf->free_space) {
        *err = CIRC_BUFF_ERR_FULL;
        return;
    }

    buf->buf_empty = false;

    if ((buf->buffer_end - buf->in_idx) < cpy_size) {
        uint8_t remaining_size = (buf->buffer_end - buf->in_idx) / buf->item_size;

        copied_len = (buf->buffer_end - buf->in_idx);
        cpy_size -= (buf->buffer_end - buf->in_idx);
        size -= remaining_size;
        uwb_circ_buff_in(buf, data, remaining_size, err);
        if (*err != CIRC_BUFF_ERR_NONE) {
            return;
        }
    }

    memcpy(buf->in_idx, data + copied_len, cpy_size);
    buf->in_idx = (char *)buf->in_idx + cpy_size;
    if (buf->in_idx == buf->buffer_end) {
        buf->in_idx = buf->buffer;
    }
    if (buf->buf_full) {
        buf->out_idx = buf->in_idx;
    } else {
        buf->free_space -= size;
        buf->num_data += size;
    }

    if (buf->in_idx == buf->out_idx) {
        buf->buf_full = true;
    }
}

void uwb_circ_buff_out(circ_buffer_t *buf, void *data, uint32_t size, circ_buff_error_t *err)
{
    uint16_t copied_len = 0;
    uint16_t cpy_size   = (buf->item_size * size);

    *err = CIRC_BUFF_ERR_NONE;

    if (buf->buf_empty) {
        *err = CIRC_BUFF_ERR_EMPTY;
        return;
    }

    buf->buf_full = false;

    if ((buf->buffer_end - buf->out_idx) < cpy_size) {
        uint8_t remaining_size = (buf->buffer_end - buf->out_idx) / buf->item_size;

        copied_len = (buf->buffer_end - buf->out_idx);
        cpy_size -= (buf->buffer_end - buf->out_idx);
        size -= remaining_size;
        uwb_circ_buff_out(buf, data, remaining_size, err);
        if (*err != CIRC_BUFF_ERR_NONE) {
            return;
        }
    }

    memcpy(data + copied_len, buf->out_idx, cpy_size);
    buf->out_idx = (char *)buf->out_idx + cpy_size;
    if (buf->out_idx == buf->buffer_end)
        buf->out_idx = buf->buffer;

    buf->num_data -= size;
    buf->free_space += size;

    if (buf->out_idx == buf->in_idx) {
        buf->buf_empty = true;
    }
}

bool uwb_circ_buff_is_empty(circ_buffer_t *buf)
{
    return buf->buf_empty;
}

bool uwb_circ_buff_is_full(circ_buffer_t *buf)
{
    return buf->buf_full;
}

uint32_t uwb_circ_buff_num_elements(circ_buffer_t *buf)
{
    return buf->num_data;
}

uint32_t uwb_circ_buff_free_space(circ_buffer_t *buf)
{
    return buf->free_space;
}
