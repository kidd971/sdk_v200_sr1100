/** @file  sr1000_circular_buffer.h
 *  @brief Circular buffer.
 *
 *  @copyright Copyright (C) 2020-2021 SPARK Microsystems International Inc.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef UWB_CIRCULAR_BUFFER_H_
#define UWB_CIRCULAR_BUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>

/* TYPES **********************************************************************/
/** @brief Circular buffer error codes.
 */
typedef enum circ_buff_error {
    /*! No error. */
    CIRC_BUFF_ERR_NONE = 0,
    /*! Buffer is empty. */
    CIRC_BUFF_ERR_EMPTY,
    /*! Buffer is full. */
    CIRC_BUFF_ERR_FULL
} circ_buff_error_t;

/** @brief Circular buffer instance.
 */
typedef struct {
    /*! Pointer to the next write position. */
    void *in_idx;
    /*! Pointer to the next read position. */
    void *out_idx;
    /*! True when the buffer is full. */
    bool buf_full;
    /*! True when the buffer is empty. */
    bool buf_empty;
    /*! Maximum number of elements the buffer can hold. */
    uint32_t buf_capacity;
    /*! Size in bytes of a single element. */
    uint8_t item_size;
    /*! Pointer to the start of the backing storage. */
    void *buffer;
    /*! Pointer past the end of the backing storage. */
    void *buffer_end;
    /*! Current number of elements stored. */
    uint32_t num_data;
    /*! Number of elements that can still be added. */
    uint32_t free_space;
} circ_buffer_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize circular buffer.
 *
 *  @note User can provide an empty buf struct to the function. It will be initialized here.
 *
 *  @param[in] buf       Struct that keeps track of the buffer state.
 *  @param[in] buf_ptr   Pointer to start of the buffer.
 *  @param[in] capacity  Maximum number of elements in the buffer.
 *  @param[in] size      Size of one element in the buffer.
 */
void uwb_circ_buff_init(circ_buffer_t *buf, void *buf_ptr, uint32_t capacity, uint8_t size);

/** @brief Push data to the circular buffer.
 *
 *  @note If the buffer is full, new data is discarded and an error is returned.
 *
 *  @param[in]  buf   Struct that keeps track of the buffer state.
 *  @param[in]  data  Pointer to the data to push.
 *  @param[in]  size  Number of elements to push.
 *  @param[out] err   Pointer that receive an error code.
 */
void uwb_circ_buff_in(circ_buffer_t *buf, void *data, uint32_t size, circ_buff_error_t *err);

/** @brief Pull data from the circular buffer.
 *
 *  @param[in]  buf   Struct that keeps track of the buffer state.
 *  @param[out] data  Pointer to write the pulled data.
 *  @param[in]  size  Number of elements to pull.
 *  @param[out] err   Pointer that receive an error code.
 */
void uwb_circ_buff_out(circ_buffer_t *buf, void *data, uint32_t size, circ_buff_error_t *err);

/** @brief Return true or false if the buffer is empty or not.
 *
 *  @param[in] buf  Struct that keeps track of the buffer state.
 *  @return True if the buffer is empty. False otherwise.
 */
bool uwb_circ_buff_is_empty(const circ_buffer_t *buf);

/** @brief Return true or false if the buffer is full or not.
 *
 *  @param[in] buf  Struct that keeps track of the buffer state.
 *  @return True if the buffer is full. False otherwise.
 */
bool uwb_circ_buff_is_full(const circ_buffer_t *buf);

/** @brief Return the number of elements in the buffer.
 *
 *  @param[in] buf  Struct that keeps track of the buffer state.
 *  @return Number of elements.
 */
uint32_t uwb_circ_buff_num_elements(const circ_buffer_t *buf);

/** @brief Return the number of element that can be added to the buffer before it is full.
 *
 *  @param[in] buf  Struct that keeps track of the buffer state.
 *  @return Number of free elements.
 */
uint32_t uwb_circ_buff_free_space(const circ_buffer_t *buf);

#ifdef __cplusplus
}
#endif
#endif /* UWB_CIRCULAR_BUFFER_H_ */
