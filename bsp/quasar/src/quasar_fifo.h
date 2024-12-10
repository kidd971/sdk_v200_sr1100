/** @file  quasar_fifo.h
 *  @brief This module provides the functions to used the FIFO buffers for UART and I2C
 * transmission and reception.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_FIFO_H_
#define QUASAR_FIFO_H_

/* INCLUDES *******************************************************************/
#include "quasar_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/*! Size of the data array used in each FIFO instance, AKA the FIFO buffer capacity. */
#define QUASAR_FIFO_BUFFER_SIZE  4096

/* TYPES **********************************************************************/
/** @brief Structure representing a First-In-First-Out buffer for the Quasar.
 */
typedef struct quasar_fifo {
    /*! The FIFO Buffer. */
    volatile uint8_t data[QUASAR_FIFO_BUFFER_SIZE];
    /*! Number of elements currently in the FIFO buffer. */
    volatile uint16_t count;
    /*! Index for inserting new elements into the FIFO buffer. */
    volatile uint16_t index_in;
    /*! Index for extracting elements from the FIFO buffer. */
    volatile uint16_t index_out;
} quasar_fifo_t;

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Initialize the Quasar FIFO buffer.
 *
 *  @param[in] fifo_to_init  Quasar FIFO buffer to be initialized.
 */
void quasar_fifo_init(quasar_fifo_t *fifo_to_init);

/** @brief Pushes a byte into the Quasar FIFO buffer.
 *
 *  @param[in] fifo     Quasar FIFO buffer to push data into.
 *  @param[in] new_data New byte to be added to the FIFO buffer.
 *  @return Return 0 if the push was successful or non-zero value if the buffer is full.
 */
uint8_t quasar_fifo_push(quasar_fifo_t *fifo, uint8_t new_data);

/** @brief Pushes multiple bytes into the Quasar FIFO buffer.
 *
 *  @param[in] fifo            Quasar FIFO buffer to push data into.
 *  @param[in] new_data_array  Array of new bytes to be added to the FIFO buffer.
 *  @param[in] size_array      Size of the array which is also the number of bytes to be added to the FIFO buffer.
 *  @return Return 0 if the push was successful or non-zero value if the buffer is full.
 */
uint8_t quasar_fifo_push_bytes(quasar_fifo_t *fifo, uint8_t *new_data_array, uint16_t size_array);

/** @brief Pull a byte from a Quasar FIFO buffer.
 *
 *  @param[in,out] fifo     Quasar FIFO buffer from which data will be pulled.
 *  @param[out] pulled_data Pointer to store the pulled data element.
 *  @return Returns 0 if the pull was successful or non-zero value if the buffer is empty.
 */
uint8_t quasar_fifo_pull(quasar_fifo_t *fifo, uint8_t *pulled_data);

/** @brief Pull multiple bytes from a Quasar FIFO buffer.
 *
 *  @param[in,out] fifo         Quasar FIFO buffer from which data will be pulled.
 *  @param[in] number_of_bytes  Number of bytes to be pulled from the FIFO buffer.
 *  @return Return the pulled bytes from the FIFO.
 */
uint64_t quasar_fifo_pull_bytes(quasar_fifo_t *fifo, uint16_t number_of_bytes);

/** @brief Get the current count of the elements in a Quasar FIFO buffer.
 *
 *  @param[in,out] fifo  Quasar FIFO buffer to query.
 *  @return The count of the elements in the FIFO buffer.
 */
uint32_t quasar_fifo_get_count(quasar_fifo_t *fifo);

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_FIFO_H_ */
