/** @file link_channel_hopping.h
 *  @brief Channel hopping module.
 *
 *  @copyright Copyright (C) 2020 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef LINK_CHANNEL_HOPPING_H_
#define LINK_CHANNEL_HOPPING_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
typedef struct channel_sequence {
    /*! Array for each channel. */
    const uint32_t *channel;
    /*! Channel hopping sequence size. */
    uint32_t sequence_size;
    /*! Number of unique channel in sequence. */
    uint8_t channel_number;
    /* Allocated channel sequence buffer of the sequence size. */
    uint8_t *channel_sequence_buffer;
} channel_sequence_t;

typedef struct channel_hopping {
    /*! The index of the current channel */
    uint8_t hop_seq_index;
    /*! The channel hopping sequence */
    channel_sequence_t *channel_sequence;
    /*! Channel lookup table to randomize channel hopping sequence */
    uint8_t *channel_lookup_table;
    /*! Middle channel index for fast sync. */
    uint8_t middle_channel_idx;
} channel_hopping_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize channel hopping object.
 *
 *  @param[in] channel_hopping          Channel hopping object.
 *  @param[in] channel_sequence         The channel hopping sequence table.
 *  @param[in] random_sequence_enabled  Random sequence enable flag.
 *  @param[in] random_sequence_seed     Random sequence seed.
 */
void link_channel_hopping_init(channel_hopping_t *channel_hopping, channel_sequence_t *channel_sequence, bool random_sequence_enabled,
                               uint8_t random_sequence_seed);

/** @brief Increment channel hopping sequence index.
 *
 *  @param[in] channel_hopping  Channel hopping object.
 */
static inline void link_channel_hopping_increment_sequence(channel_hopping_t *channel_hopping, uint8_t increment)
{
    channel_hopping->hop_seq_index = (channel_hopping->hop_seq_index + increment) % channel_hopping->channel_sequence->sequence_size;
}

/** @brief Set current channel hopping sequence index.
 *
 *  @param[in] channel_hopping  Channel hopping object.
 *  @param[in] seq_index        The desired channel hopping sequence index.
 */
static inline void link_channel_hopping_set_seq_index(channel_hopping_t *channel_hopping, uint8_t seq_index)
{
    channel_hopping->hop_seq_index = seq_index;
}

/** @brief Get current channel hopping sequence index.
 *
 *  @param[in] channel_hopping  Channel hopping object.
 *  @return The current channel hopping sequence index.
 */
static inline uint8_t link_channel_hopping_get_seq_index(channel_hopping_t *channel_hopping)
{
    return channel_hopping->hop_seq_index;
}

/** @brief Get current channel.
 *
 *  @param[in] channel_hopping  Channel hopping object.
 *  @return The current channel.
 */
static inline uint32_t link_channel_hopping_get_channel(channel_hopping_t *channel_hopping)
{
    return channel_hopping->channel_lookup_table[channel_hopping->channel_sequence->channel[channel_hopping->hop_seq_index]];
}

#ifdef __cplusplus
}
#endif

#endif /* LINK_CHANNEL_HOPPING_H_ */
