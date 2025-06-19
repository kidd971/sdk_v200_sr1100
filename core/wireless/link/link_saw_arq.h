/** @file link_saw_arq.h
 *  @brief Stop and wait ARQ module.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef LINK_SAW_ARQ_H_
#define LINK_SAW_ARQ_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/*! TX device default Stop and wait index value */
#define TX_DEFAULT_SAW_INDEX 2
/*! RX device default Stop and wait index value */
#define RX_DEFAULT_SAW_INDEX 3

/* TYPES **********************************************************************/
/** @brief Stop and Wait ARQ
 */
typedef struct saw_arq {
    /*! Time to live in tick */
    uint64_t ttl_tick;
    /*! Time to live in amount of retries */
    uint16_t ttl_retries;
    /*! 2 bit sequence number to use by default. */
    uint8_t default_seq_num;
    /*! 2 bit sequence number */
    uint8_t seq_num;
    /*! Duplicate flag */
    bool duplicate;
    /*! Duplicate count */
    uint32_t duplicate_count;
    /*! Retry count */
    uint32_t retry_count;
    /*! Module enable flag */
    bool enable;
} saw_arq_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize SAW ARQ Object.
 *
 *  @param[in] saw_arq      SAW ARQ Object.
 *  @param[in] ttl_tick     Time to live in tick.
 *  @param[in] ttl_retries  Time to live in amount of retries.
 *  @param[in] board_seq    Board sequence based on source/dest address for sequence number init.
 *  @param[in] enable       Enable flag.
 */
void link_saw_arq_init(saw_arq_t *saw_arq, uint16_t ttl_tick, uint16_t ttl_retries, uint8_t init_board_seq,
                       bool enable);

/** @brief Reset the SAW ARQ object to default values.
 *
 *  @param[in] saw_arq  SAW ARQ Object.
 */
void link_saw_arq_reset(saw_arq_t *saw_arq);

/** @brief Is the current frame timeout.
 *
 *  @param[in] saw_arq     SAW ARQ Object.
 *  @param[in] frame       The frame.
 *  @param[in] time_stamp  Time stamp.
 */
bool link_saw_arq_is_frame_timeout(saw_arq_t *saw_arq, uint64_t time_stamp, uint16_t retry_count,
                                   uint64_t current_time);

/** @brief Get sequence number.
 *
 *  @param[in] saw_arq  SAW ARQ Object.
 *  @return sequence number.
 */
static inline uint8_t link_saw_arq_get_seq_num(saw_arq_t *saw_arq)
{
    return saw_arq->seq_num;
}

/** @brief Increment sequence number.
 *
 *  @param[in] saw_arq  SAW ARQ Object.
 */
static inline void link_saw_arq_inc_seq_num(saw_arq_t *saw_arq)
{
    saw_arq->seq_num &= 1;
    saw_arq->seq_num ^= 1;
}

/** @brief Update RX sequence number.
 *
 *  @param[in] saw_arq  SAW ARQ Object.
 *  @param[in] seq_num  Sequence number.
 */
static inline void link_saw_arq_update_rx_seq_num(saw_arq_t *saw_arq, uint8_t seq_num)
{
    saw_arq->duplicate = (seq_num == saw_arq->seq_num);
    saw_arq->seq_num = (seq_num & 1);
}

/** @brief Is the received frame a duplicate.
 *
 *  @param[in] saw_arq  SAW ARQ Object.
 */
bool link_saw_arq_is_rx_frame_duplicate(saw_arq_t *saw_arq);

/** @brief Get duplicate count.
 *
 *  @param[in] saw_arq  SAW ARQ Object.
 */
static inline uint32_t link_saw_arq_get_duplicate_count(saw_arq_t *saw_arq)
{
    return saw_arq->duplicate_count;
}

/** @brief Get total number of retries.
 *
 *  @param[in] saw_arq  SAW ARQ Object.
 */
static inline uint32_t link_saw_arq_get_retry_count(saw_arq_t *saw_arq)
{
    return saw_arq->retry_count;
}

/** @brief Reset the SaW ARQ object stats.
 *
 *  @param[in] saw_arq  SAW ARQ Object.
 */
static inline void link_saw_arq_reset_stats(saw_arq_t *saw_arq)
{
    saw_arq->retry_count = 0;
    saw_arq->duplicate_count = 0;
}

/** @brief Increment the SaW ARQ duplicate count.
 *
 *  @param[in] saw_arq  SAW ARQ Object.
 */
static inline void link_saw_arq_incr_duplicate_count(saw_arq_t *saw_arq)
{
    saw_arq->duplicate_count++;
}

#ifdef __cplusplus
}
#endif
#endif /* LINK_CHANNEL_HOPPING_H_ */
