/** @file  sac_utils.c
 *  @brief Utility functions for the SPARK Audio Core.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sac_utils.h"

/* PUBLIC FUNCTIONS ***********************************************************/
uint8_t sac_get_sample_size_from_format(sac_sample_format_t sample_format)
{
    if (sample_format.sample_encoding == SAC_SAMPLE_UNPACKED) {
        return SAC_WORD_SIZE_BITS;
    } else {
        return sample_format.bit_depth;
    }
}

uint16_t sac_get_nb_packets_in_x_ms(uint16_t ms, uint16_t audio_payload_size, uint8_t nb_channel,
                                    sac_sample_format_t sample_format, uint32_t sampling_rate)
{
    uint16_t sample_count = (audio_payload_size * SAC_BYTE_SIZE_BITS) / sac_get_sample_size_from_format(sample_format);

    return ((ms / 1000.0) / ((sample_count / nb_channel) / (float)sampling_rate));
}

uint16_t sac_get_ms_in_x_packets(uint16_t nb_packet, uint16_t audio_payload_size, uint8_t nb_channel,
                                 sac_sample_format_t sample_format, uint32_t sampling_rate)
{
    uint16_t sample_count = (audio_payload_size * SAC_BYTE_SIZE_BITS) / sac_get_sample_size_from_format(sample_format);

    return 1000 * nb_packet * ((sample_count / nb_channel) / (float)sampling_rate);
}

bool sac_configure_packing(sac_packing_instance_t *instance, sac_sample_format_t producer_format,
                           sac_sample_format_t consumer_format, sac_status_t *status)
{
    *status = SAC_OK;

    /* No processing needed if formats are identical. */
    if ((producer_format.sample_encoding == consumer_format.sample_encoding) &&
        (producer_format.bit_depth == consumer_format.bit_depth)) {
        return false;
    }

    if (producer_format.sample_encoding == SAC_SAMPLE_UNPACKED) {
        if (consumer_format.sample_encoding == SAC_SAMPLE_UNPACKED) {
            /* Unpacked to unpacked with different bit depths is not a packing operation. */
            *status = SAC_ERR_BIT_DEPTH;
            return false;
        }
        /* Unpacked to Packed: PACK operation. */
        switch (producer_format.bit_depth) {
        case SAC_16BITS:
            /* 16-bit unpacked samples do not require packing. */
            return false;
        case SAC_18BITS:
            if (consumer_format.bit_depth == SAC_18BITS) {
                instance->packing_mode = SAC_PACK_18BITS;
                return true;
            }
            break;
        case SAC_20BITS:
            switch (consumer_format.bit_depth) {
            case SAC_16BITS:
                instance->packing_mode = SAC_PACK_20BITS_16BITS;
                return true;
            case SAC_20BITS:
                instance->packing_mode = SAC_PACK_20BITS;
                return true;
            default:
                break;
            }
            break;
        case SAC_24BITS:
            switch (consumer_format.bit_depth) {
            case SAC_16BITS:
                instance->packing_mode = SAC_PACK_24BITS_16BITS;
                return true;
            case SAC_20BITS:
                instance->packing_mode = SAC_PACK_24BITS_20BITS;
                return true;
            case SAC_24BITS:
                instance->packing_mode = SAC_PACK_24BITS;
                return true;
            default:
                break;
            }
            break;
        case SAC_32BITS:
            if (consumer_format.bit_depth == SAC_24BITS) {
                instance->packing_mode = SAC_PACK_32BITS_24BITS;
                return true;
            }
            break;
        default:
            break;
        }
    } else {
        /* Producer is packed. */
        if (consumer_format.sample_encoding == SAC_SAMPLE_PACKED) {
            /* Packed to Packed: SCALE operation. */
            switch (producer_format.bit_depth) {
            case SAC_16BITS:
                if (consumer_format.bit_depth == SAC_24BITS) {
                    instance->packing_mode = SAC_SCALE_16BITS_24BITS;
                    return true;
                }
                break;
            case SAC_20BITS:
                if (consumer_format.bit_depth == SAC_24BITS) {
                    instance->packing_mode = SAC_SCALE_20BITS_24BITS;
                    return true;
                }
                break;
            case SAC_24BITS:
                switch (consumer_format.bit_depth) {
                case SAC_16BITS:
                    instance->packing_mode = SAC_SCALE_24BITS_16BITS;
                    return true;
                case SAC_20BITS:
                    instance->packing_mode = SAC_SCALE_24BITS_20BITS;
                    return true;
                default:
                    break;
                }
                break;
            case SAC_32BITS:
                /* 32-bit packed samples do not require processing. */
                return false;
            default:
                break;
            }
        } else {
            /* Packed to Unpacked: UNPACK operation. */
            switch (producer_format.bit_depth) {
            case SAC_16BITS:
                switch (consumer_format.bit_depth) {
                case SAC_16BITS:
                    /* Same bit depth: no unpacking needed. */
                    return false;
                case SAC_20BITS:
                    instance->packing_mode = SAC_UNPACK_20BITS_16BITS;
                    return true;
                case SAC_24BITS:
                    instance->packing_mode = SAC_UNPACK_24BITS_16BITS;
                    return true;
                default:
                    break;
                }
                break;
            case SAC_18BITS:
                if (consumer_format.bit_depth == SAC_18BITS) {
                    instance->packing_mode = SAC_UNPACK_18BITS;
                    return true;
                }
                break;
            case SAC_20BITS:
                switch (consumer_format.bit_depth) {
                case SAC_20BITS:
                    instance->packing_mode = SAC_UNPACK_20BITS;
                    return true;
                case SAC_24BITS:
                    instance->packing_mode = SAC_UNPACK_24BITS_20BITS;
                    return true;
                default:
                    break;
                }
                break;
            case SAC_24BITS:
                if (consumer_format.bit_depth == SAC_24BITS) {
                    instance->packing_mode = SAC_UNPACK_24BITS;
                    return true;
                }
                break;
            case SAC_32BITS:
                /* 32-bit packed samples do not require processing. */
                return false;
            default:
                break;
            }
        }
    }

    /* Unsupported format combination. */
    *status = SAC_ERR_BIT_DEPTH;
    return false;
}
