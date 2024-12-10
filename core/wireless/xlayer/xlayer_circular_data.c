/** @file  xlayer_circular_data.c
 *  @brief xlayer circular data container.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "xlayer_circular_data.h"

/* PUBLIC FUNCTIONS ***********************************************************/
void xlayer_circular_data_init(xlayer_circular_data_t *circ_data, uint8_t *data, uint16_t size)
{
    circ_data->buffer = data;
    circ_data->buffer_size = size;
    circ_data->head = 0;
    circ_data->tail = 0;
    circ_data->last_head = 0;
}

uint8_t *xlayer_circular_data_allocate_space(xlayer_circular_data_t *circ_data, uint16_t required_space)
{
    if (circ_data == NULL) {
        return NULL;
    }

    uint8_t *out_data = NULL;
    uint16_t last_head = circ_data->head;

    /* Find a contiguous memory area of ​​size 'required_space'. */
    if (circ_data->head >= circ_data->tail) {
        uint16_t free_bytes_to_end = circ_data->buffer_size - circ_data->head;
        uint16_t free_bytes_from_begin = circ_data->tail;

        if (free_bytes_to_end >= required_space) {
            out_data = circ_data->buffer + circ_data->head;
        } else if (free_bytes_from_begin >= required_space) {
            out_data = circ_data->buffer;
            last_head = 0;
        }
    } else {
        uint16_t free_bytes_middle = circ_data->tail - circ_data->head;

        if (free_bytes_middle >= required_space) {
            out_data = circ_data->buffer + circ_data->head;
        }
    }

    if (out_data != NULL) {
        circ_data->last_head = last_head;
        circ_data->head = last_head + required_space;
    }

    return out_data;
}

uint16_t xlayer_circular_data_free_space(xlayer_circular_data_t *circ_data, const uint8_t *data, uint16_t free_bytes)
{
    if (circ_data == NULL || data == NULL || free_bytes == 0) {
        return 0;
    }

    /* Calculate the index of space to be released */
    uint16_t free_buff_idx = data - circ_data->buffer;

    /* Check if the freed memory comes from the last allocated space (removing duplicate frame) */
    if (circ_data->last_head == free_buff_idx) {
        circ_data->head = circ_data->last_head;
        return free_bytes;
    }

    uint16_t tail = circ_data->tail;

    if (tail + free_bytes > circ_data->buffer_size) {
        tail = 0;
    }

    /* Check whether the freed placed memory is free using the appropriate order */
    if (free_buff_idx != tail) {
        return 0;
    }

    circ_data->tail = tail + free_bytes;

    return free_bytes;
}

void xlayer_circular_data_flush(xlayer_circular_data_t *circ_data)
{
    circ_data->head = 0;
    circ_data->tail = 0;
}
