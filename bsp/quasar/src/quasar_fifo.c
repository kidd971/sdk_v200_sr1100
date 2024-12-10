/** @file  quasar_fifo.c
 *  @brief This module provides the functions to used the FIFO buffers for UART and I2C
 * transmission and reception.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_fifo.h"

/* CONSTANTS ******************************************************************/
#define BYTE_SIZE   8

/* PUBLIC FUNCTIONS ***********************************************************/
void quasar_fifo_init(quasar_fifo_t *fifo_to_init)
{
    fifo_to_init->count = 0;
    fifo_to_init->index_in = 0;
    fifo_to_init->index_out = 0;
}

uint8_t quasar_fifo_push(quasar_fifo_t *fifo, uint8_t new_data)
{
    /* Check if fifo is full. */
    if (fifo->count >= QUASAR_FIFO_BUFFER_SIZE) {
        return 1;
    }

    /* Push data into the fifo. */
    fifo->data[fifo->index_in] = new_data;
    /* Adjust count and circular indexer. */
    fifo->index_in++;
    fifo->index_in %= QUASAR_FIFO_BUFFER_SIZE;
    fifo->count++;

    return 0;
}

uint8_t quasar_fifo_push_bytes(quasar_fifo_t *fifo, uint8_t *new_data_array, uint16_t size_array)
{
    /* Check if fifo is full. */
    if (((fifo->count) + size_array) >= QUASAR_FIFO_BUFFER_SIZE) {
        return 1;
    }

    /* Pushes up to each byte to the FIFO, starting with the MSB. */
    for (int i = (size_array - 1); i >= 0; i--) {
        quasar_fifo_push(fifo, new_data_array[i]);
    }

    return 0;
}

uint8_t quasar_fifo_pull(quasar_fifo_t *fifo, uint8_t *pulled_data)
{
    if (fifo->count <= 0) {
        return 1;
    }

    /* Pull data from the fifo. */
    *pulled_data = fifo->data[fifo->index_out];
    /* Adjust count and circular indexer. */
    fifo->index_out++;
    fifo->index_out %= QUASAR_FIFO_BUFFER_SIZE;
    fifo->count--;

    return 0;
}

uint64_t quasar_fifo_pull_bytes(quasar_fifo_t *fifo, uint16_t number_of_bytes)
{
    uint64_t pulled_bytes = 0;
    uint8_t pulled_byte = 0;
    uint64_t temp;

    /**
     *      Strategy :
     *
     *                56          48          40          32          24          16           8           0
     *                 |           |           |           |           |           |           |           |
     *       | 0000 0000 | 0000 0000 | 0000 0000 | 0000 0000 | 0000 0000 | 0000 0000 | 0000 0000 | 0000 0000 |
     *       |_____8_____|_____7_____|_____6_____|_____5_____|_____4_____|_____3_____|_____2_____|_____1_____|
     *
     *  reassembled_bytes = 0;
     *  reassembled_bytes |= (4ieme byte << 24);
     *  reassembled_bytes |= (3ieme byte << 16);
     *  reassembled_bytes |= (2ieme byte << 8);
     *  reassembled_bytes |= (1er byte << 0);
     *
     *  logical suite :
     *
     *      8 x 0 = 0
     *      8 x 1 = 8
     *      8 x 2 = 16
     *      8 x 3 = 24
     *      ...
     *      8 x 7 = 56
     */

    /* The number of bytes must be 8 or fewer in order to fit in a uint64_t */
    if (number_of_bytes > 8) {
        while (1);
        /* Trigger an exception  */
    }
    /* Pulled up to 8 bytes from the FIFO, placing each byte into its respective position in a uint64_t value, starting with MSB. */
    for (int i = (number_of_bytes - 1); i >= 0; i--) {
        /* Pull a byte */
        quasar_fifo_pull(fifo, &pulled_byte);
        /* Typecast to uint64_t to enable bit shifts beyond the 32 bits default of STM32 registers */
        temp = (uint64_t) pulled_byte;
        /* Each byte is left-shifted to its correct bit position */
        pulled_bytes |= (temp << (BYTE_SIZE * i));
    }

    return pulled_bytes;
}

uint32_t quasar_fifo_get_count(quasar_fifo_t *fifo)
{
    return fifo->count;
}
