/** @file link_channel_hopping.c
 *  @brief Channel hopping module.
 *
 *  @copyright Copyright (C) 2020 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "link_channel_hopping.h"
#include <stdlib.h>
#include <string.h>

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static bool is_in_table(uint8_t *table, uint8_t size, uint8_t channel);
static void generate_freq_table(uint8_t *table, const uint32_t *channels, uint8_t *channel_count, uint8_t size,
                                uint8_t channel_number);
static void generate_random_hop_sequence(uint8_t *table_out, uint8_t *table_in, uint8_t channel_count, uint8_t seed);

/* PUBLIC FUNCTIONS ***********************************************************/
void link_channel_hopping_init(channel_hopping_t *channel_hopping, channel_sequence_t *channel_sequence, bool random_sequence_enabled,
                               uint8_t random_sequence_seed)
{
    if (channel_sequence->channel_sequence_buffer == NULL) {
        return;
    }

    uint8_t channel_number = channel_sequence->channel_number;
    uint8_t freq_table1[channel_number];
    uint8_t freq_table2[channel_number];
    uint8_t channel_count;

    /* Initialize freq tables */
    memset(freq_table1, 0, sizeof(uint8_t) * channel_number);
    memset(freq_table2, 0, sizeof(uint8_t) * channel_number);

    /* Initialize channel hopping instance */
    memset(channel_hopping, 0, sizeof(channel_hopping_t));
    channel_hopping->channel_sequence = channel_sequence;
    channel_hopping->middle_channel_idx = channel_sequence->sequence_size / 2;
    channel_hopping->channel_lookup_table = channel_sequence->channel_sequence_buffer;
    channel_hopping->hop_seq_index = 0;

    generate_freq_table(freq_table1, channel_hopping->channel_sequence->channel, &channel_count,
                        channel_hopping->channel_sequence->sequence_size, channel_number);

    if (random_sequence_enabled) {
        generate_random_hop_sequence(freq_table2, freq_table1, channel_count, random_sequence_seed);
    } else {
        memcpy(freq_table2, freq_table1, channel_count); // * sizeof(*freq_table1) is 1 so no need for multiplication.
    }

    generate_freq_table(freq_table1, channel_hopping->channel_sequence->channel, &channel_count,
                        channel_hopping->channel_sequence->sequence_size, channel_number);

    for (uint8_t i = 0; i < channel_number; i++) {
        channel_hopping->channel_lookup_table[freq_table1[i]] = freq_table2[i];
    }
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Determine if the channel provided is in the table.
 *
 *  @param[in] table    The table.
 *  @param[in] size     The size of the table.
 *  @param[in] channel  The channel.
 *  @return is_in_table.
 */
static bool is_in_table(uint8_t *table, uint8_t size, uint8_t channel)
{
    for (uint8_t i = 0; i < size; i++) {
        if (table[i] == channel) {
            return true;
        }
    }

    return false;
}

/** @brief Generate initial frequency table.
 *
 *  @param[in] table           The frequency table to be generated.
 *  @param[in] channels        The channel table.
 *  @param[in] channel_count   The number of channels counted inside the sequence.
 *  @param[in] size            The size of the channel sequence.
 *  @param[in] channel_number  The number of different channel in the sequence.
 */
static void generate_freq_table(uint8_t *table, const uint32_t *channels, uint8_t *channel_count, uint8_t size,
                                uint8_t channel_number)
{
    *channel_count = 0;
    for (uint8_t i = 0; i < size; i++) {
        if (!is_in_table(table, channel_number, channels[i])) {
            table[*channel_count] = channels[i];
            (*channel_count)++;
        }
    }
}

/** @brief Generate random channel hopping sequence.
 *
 *  @param[in] table_out      The output table.
 *  @param[in] table_in       The input table.
 *  @param[in] channel_count  The number of channels counted inside the sequence.
 *  @param[in] seed           The randomization seed.
 */
static void generate_random_hop_sequence(uint8_t *table_out, uint8_t *table_in, uint8_t channel_count, uint8_t seed)
{
    uint8_t rand_num;

    srand(seed + 2); /* Avoid seed value 1 which reset the seed by adding 2*/
    for (uint8_t i = 0; i < channel_count; i++) {
        rand_num     = rand() % (channel_count - i);
        table_out[i] = table_in[rand_num];
        for (uint8_t j = rand_num; j < channel_count; j++) {
            table_in[j] = table_in[j + 1];
        }
    }
}
