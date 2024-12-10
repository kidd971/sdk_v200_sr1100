/** @file  xlayer_circular_data.h
 *  @brief xlayer circular data container.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef XLAYER_CIRCULAR_DATA_H
#define XLAYER_CIRCULAR_DATA_H

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/** @brief Additional bytes required to facilitate non-blocking SPI communication using a queue.
 * These 2 bytes are allocated to store the SPI register address and frame header size for
 * SPI transceiver transfers.
 */
#define XLAYER_QUEUE_SPI_COMM_ADDITIONAL_BYTES 2

/** @brief Offset index for SPI register address
 */
#define XLAYER_QUEUE_SPI_COMM_REG_POSITION_OFFSET 0

/** @brief Offset index for frame header size
 */
#define XLAYER_QUEUE_SPI_COMM_HEADER_SIZE_POSITION_OFFSET 1

/* TYPES **********************************************************************/
/** @brief Circular data container
 */
typedef struct xlayer_circular_data {
    /*! Pointer to data */
    uint8_t *buffer;
    /*! Size of the data */
    uint16_t buffer_size;
    /*! Index of the head */
    uint16_t head;
    /*! Index of the tail */
    uint16_t tail;
    /*! Index of the last head */
    uint16_t last_head;
} xlayer_circular_data_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize new circular data
 *
 *  @param[in] circ_data  Circular data to be initialized.
 *  @param[in] data       Pointer to buffer for the data.
 *  @param[in] size       Size of the data buffer.
 */
void xlayer_circular_data_init(xlayer_circular_data_t *circ_data, uint8_t *data, uint16_t size);

/** @brief Allocate space for the data in circular buffer
 *
 *  @param[in] circ_data         Desired xlayer_circular_data.
 *  @param[in] required_space    Size of the required space.
 *  @return Pointer to buffer with data. NULL, when there is no space.
 */
uint8_t *xlayer_circular_data_allocate_space(xlayer_circular_data_t *circ_data, uint16_t required_space);

/** @brief Free memory space in circular buffer
 *
 *  @note The free memory space must release according to allocation order.
 *        When an error is detected, 0 bytes are returned.
 *
 *  @param[in] circ_data   Desired xlayer_circular_data.
 *  @param[in] data        A pointer to a buffer that should be free.
 *  @param[in] free_bytes  Size of the space to be free.
 *  @return Number of bytes released, 0 in case of error.
 */
uint16_t xlayer_circular_data_free_space(xlayer_circular_data_t *circ_data, const uint8_t *data, uint16_t free_bytes);

/** @brief Free any existing data in circular data.
 *
 *  @param[in] circ_data  Desired xlayer_circular_data_t to flush.
 */
void xlayer_circular_data_flush(xlayer_circular_data_t *circ_data);

/** @brief Getter for reading circular data head
 *
 *  @param[in] circ_data  Desired xlayer_circular_data.
 *  @return Head position
 */
static inline uint16_t xlayer_circular_data_get_head(xlayer_circular_data_t *circ_data)
{
    return circ_data->head;
}

/** @brief Getter for reading circular data tail
 *
 *  @param[in] circ_data  Desired xlayer_circular_data.
 *  @return Tail position
 */
static inline uint16_t xlayer_circular_data_get_tail(xlayer_circular_data_t *circ_data)
{
    return circ_data->tail;
}

/** @brief Getter for reading circular data last head
 *
 *  @param[in] circ_data  Desired xlayer_circular_data.
 *  @return Last head position
 */
static inline uint16_t xlayer_circular_data_get_last_head(xlayer_circular_data_t *circ_data)
{
    return circ_data->last_head;
}

/** @brief A function that calculates the required space for a TX frame in the cyclic data buffer.
 *
 *  @param[in] queue_size        Queue size.
 *  @param[in] header_size       Frame header size.
 *  @param[in] max_payload_size  Maximum frame payload size.
 *  @return Number of bytes required by the TX circular data.
 */
static inline uint16_t xlayer_circular_data_get_tx_required_bytes(uint16_t queue_size, uint8_t header_size,
                                                                  uint16_t max_payload_size)
{
    return (queue_size * (XLAYER_QUEUE_SPI_COMM_ADDITIONAL_BYTES + (uint16_t)header_size + max_payload_size));
}

/** @brief A function that calculates the required space for a RX payload in the cyclic data buffer
 *
 *  @param[in] queue_size        Queue size.
 *  @param[in] max_payload_size  Maximum frame payload size.
 *  @return Number of bytes required by the RX circular data.
 */
static inline uint16_t xlayer_circular_data_get_rx_required_bytes(uint16_t queue_size, uint16_t max_payload_size)
{
    return (queue_size * max_payload_size);
}

#ifdef __cplusplus
}
#endif

#endif /* XLAYER_CIRCULAR_DATA_H */
