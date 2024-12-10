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
typedef enum circ_buff_error {
    CIRC_BUFF_ERR_NONE = 0,
    CIRC_BUFF_ERR_EMPTY,
    CIRC_BUFF_ERR_FULL
} circ_buff_error_t;

typedef struct {
    void    *in_idx;
    void    *out_idx;
    bool     buf_full;
    bool     buf_empty;
    uint32_t buf_capacity;
    uint8_t  item_size;
    void    *buffer;
    void    *buffer_end;
    uint32_t num_data;
    uint32_t free_space;
} circ_buffer_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize circular buffer.
 *
 *  User can provide an empty buf struct to the function. It will be initialized
 *  here.
 *
 *  @param[in] buf      Struct that keeps track of the buffer state.
 *  @param[in] buf_ptr  Pointer to start of the buffer.
 *  @param[in] capacity Maximum number of elements in the buffer.
 *  @param[in] size     Size of one element in the buffer.
 */
void uwb_circ_buff_init(circ_buffer_t *buf, void *buf_ptr, uint32_t capacity, uint8_t size);

/** @brief Push data to the circular buffer.
 *
 *  If the buffer is full, new data is discarded and an error is returned.
 *
 *  @param[in]  buf  Struct that keeps track of the buffer state.
 *  @param[in]  data Pointer to the data to push.
 *  @param[in]  size Number of elements to push
 *  @param[out] err  Pointer that receive an error code.
 */
void uwb_circ_buff_in(circ_buffer_t *buf, void *data, uint32_t size, circ_buff_error_t *err);

/** @brief Pull data from the circular buffer.
 *
 *  @param[in]  buf  Struct that keeps track of the buffer state.
 *  @param[out] data Pointer to write the pulled data.
 *  @param[in]  size Number of elements to pull
 *  @param[out] err  Pointer that receive an error code.
 */
void uwb_circ_buff_out(circ_buffer_t *buf, void *data, uint32_t size, circ_buff_error_t *err);

/** @brief Return true or false if the buffer is empty or not.
 *
 *  @param[in]  buf  Struct that keeps track of the buffer state.
 *  @return True if the buffer is empty. False otherwise.
 */
bool uwb_circ_buff_is_empty(circ_buffer_t *buf);

/** @brief Return true or false if the buffer is full or not.
 *
 *  @param[in]  buf  Struct that keeps track of the buffer state.
 *  @return True if the buffer is full. False otherwise.
 */
bool uwb_circ_buff_is_full(circ_buffer_t *buf);

/** @brief Return the number of elements in the buffer
 *
 *  @param[in]  buf  Struct that keeps track of the buffer state.
 *  @return Number of elements.
 */
uint32_t uwb_circ_buff_num_elements(circ_buffer_t *buf);

/** @brief Return the number of element that can be added to the buffer
 *         before it is full.
 *
 *  @param[in]  buf  Struct that keeps track of the buffer state.
 *  @return Number of free elements.
 */
uint32_t uwb_circ_buff_free_space(circ_buffer_t *buf);

#ifdef __cplusplus
}
#endif
#endif /* UWB_CIRCULAR_BUFFER_H_ */
