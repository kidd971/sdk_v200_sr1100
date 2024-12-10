/** @file  sac_src_cmsis.c
 *  @brief Sampling rate converter processing stage using the CMSIS DSP software library.
 *
 *  @note This processing stage requires an Arm Cortex-M processor based device.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sac_src_cmsis.h"
#include <string.h>

/* CONSTANTS ******************************************************************/
/*! Must be dividable by all sac_src_factor since fir phaseLength is (NumTaps / ratio). */
#define FIR_NUMTAPS 24
/*
 * The filters used in this processing stage will introduce a delay equivalent to FIR_NUMTAPS samples.
 * This delay is the results of the FIR filters used for decimation and interpolation.
 *
 * Both the decimator and interpolator filters will introduce a delay of FIR_NUMTAPS divided by 2. For that reason half
 * of the accumutor will be applied at the decimator and will send the rest to the interpolator to apply its correction.
 */
#define FIR_SAMPLE_COUNT_CORRECTION_FACTOR 2

/* Half of initial sampling frequency */
static const int32_t fir_n24_c0_35_w_hamming_32bit[FIR_NUMTAPS] = {
    373831,     -5398190,  -9034464,  1428966,   27704979,  35768973,  -16611028, -102718003,
    -102343356, 94026137,  437870567, 712673411, 712673411, 437870567, 94026137,  -102343356,
    -102718003, -16611028, 35768973,  27704979,  1428966,   -9034464,  -5398190,  373831,
};

/*
 * Same filter with coefficient multiplied by a factor of 2
 * Used to compensate for the gain loss due to the interpolation zero-stuffing.
 */
static const int32_t fir_n24_c0_35_w_hamming_x2_gain_32bit[FIR_NUMTAPS] = {
    747662,     -10796381, -18068928, 2857933,    55409959,   71537947,  -33222056, -205436007,
    -204686713, 188052275, 875741134, 1425346823, 1425346823, 875741134, 188052275, -204686713,
    -205436007, -33222056, 71537947,  55409959,   2857933,    -18068928, -10796381, 747662,
};

/* Third of initial sampling frequency */
static const int32_t fir_n24_c0_20_w_hamming_32bit[FIR_NUMTAPS] = {
    3830811,   1944310,   -3254016,  -14643242, -29801855, -37819816, -21852163, 32441298,
    126699149, 244179808, 353138497, 418879042, 418879042, 353138497, 244179808, 126699149,
    32441298,  -21852163, -37819816, -29801855, -14643242, -3254016,  1944310,   3830811,
};

/*
 * Same filter with coefficient multiplied by a factor of 3
 * Used to compensate for the gain loss due to the interpolation zero-stuffing.
 */
static const int32_t fir_n24_c0_20_w_hamming_x3_gain_32bit[FIR_NUMTAPS] = {
    11492434,  5832931,   -9762050,   -43929727,  -89405567,  -113459450, -65556491, 97323896,
    380097448, 732539426, 1059415492, 1256637128, 1256637128, 1059415492, 732539426, 380097448,
    97323896,  -65556491, -113459450, -89405567,  -43929727,  -9762050,   5832931,   11492434,
};

/* Fourth of initial sampling frequency */
static const int32_t fir_n24_c0_15_w_hamming_32bit[FIR_NUMTAPS] = {
    -3624579,  -6158766,  -10307372, -13854923, -11480531, 3692190,   37194283,  90107443,
    157164413, 227093189, 285371665, 318544811, 318544811, 285371665, 227093189, 157164413,
    90107443,  37194283,  3692190,   -11480531, -13854923, -10307372, -6158766,  -3624579,
};

/*
 * Same filter with coefficient multiplied by a factor of 4
 * Used to compensate for the gain loss due to the interpolation zero-stuffing.
 */
static const int32_t fir_n24_c0_15_w_hamming_x4_gain_32bit[FIR_NUMTAPS] = {
    -14498319, -24635067, -41229489,  -55419694,  -45922124,  14768761,   148777134, 360429775,
    628657654, 908372756, 1141486663, 1274179246, 1274179246, 1141486663, 908372756, 628657654,
    360429775, 148777134, 14768761,   -45922124,  -55419694,  -41229489,  -24635067, -14498319,
};

/* Sixth of initial sampling frequency. */
static const int32_t fir_n24_c0_10_w_hamming_32bit[FIR_NUMTAPS] = {
    -2390937,  -1094722,  1832137,   9139335,   23437783,  46326649,  77681972,  115325150,
    155197661, 192036148, 220405496, 235845147, 235845147, 220405496, 192036148, 155197661,
    115325150, 77681972,  46326649,  23437783,  9139335,   1832137,   -1094722,  -2390937,
};

/* Same filter with coefficient multiplied by a factor of 6
 * Used to compensate for the gain loss due to the interpolation zero-stuffing.
 */
static const int32_t fir_n24_c0_10_w_hamming_x6_gain_32bit[FIR_NUMTAPS] = {
    -14345622, -6568333,   10992826,   54836011,   140626703,  277959898,  466091835,  691950904,
    931185971, 1152216889, 1322432976, 1415070883, 1415070883, 1322432976, 1152216889, 931185971,
    691950904, 466091835,  277959898,  140626703,  54836011,   10992826,   -6568333,   -14345622,
};
/* PRIVATE FUNCTION PROTOTYPES ************************************************/
void set_word_size(src_cmsis_cfg_t *cmsis_cfg, uint8_t *input_sample_size_byte, uint8_t *output_sample_size_byte);

/* PUBLIC FUNCTIONS ***********************************************************/
void sac_src_cmsis_init(void *instance, const char *name, sac_pipeline_t *pipeline, mem_pool_t *mem_pool,
                        sac_status_t *status)
{
    (void)pipeline;
    (void)name;

    int32_t *fir_state;
    const int32_t *fir_coeff_decimation;
    const int32_t *fir_coeff_interpolation;
    src_cmsis_instance_t *src_instance = instance;
    fir_decimate_instance_t *decimate_instance;
    fir_interpolate_instance_t *interpolate_instance;
    fir_sample_format_t *input_format;
    fir_sample_format_t *output_format;
    uint8_t input_sample_size_byte;
    uint8_t output_sample_size_byte;
    uint32_t block_size;
    uint32_t allocation_size;
    uint16_t discard_accumulator_size;
    uint8_t bit_depth;
    filtering_functions_error_t fir_err;

    *status = SAC_OK;

    if (src_instance == NULL) {
        *status = SAC_ERR_NULL_PTR;
        return;
    }

    if (src_instance->cfg.payload_size == 0) {
        *status = SAC_ERR_PROCESSING_STAGE_INIT;
        return;
    }

    if (src_instance->cfg.channel_count == 0) {
        *status = SAC_ERR_PROCESSING_STAGE_INIT;
        return;
    }

    switch (src_instance->cfg.input_sample_format.bit_depth) {
    case SAC_16BITS:
    case SAC_24BITS:
        break;
    default:
        /* Invalid input bit depth. */
        *status = SAC_ERR_PROCESSING_STAGE_INIT;
        return;
    }

    switch (src_instance->cfg.output_sample_format.bit_depth) {
    case SAC_16BITS:
    case SAC_24BITS:
        break;
    default:
        /* Invalid output bit depth. */
        *status = SAC_ERR_PROCESSING_STAGE_INIT;
        return;
    }

    if ((src_instance->cfg.multiply_ratio == 1) && (src_instance->cfg.divide_ratio == 1)) {
        *status = SAC_ERR_PROCESSING_STAGE_INIT;
        return;
    }

    set_word_size(&src_instance->cfg, &input_sample_size_byte, &output_sample_size_byte);

    switch (src_instance->cfg.multiply_ratio) {
    case SAC_SRC_ONE:
        fir_coeff_interpolation = NULL;
        break;
    case SAC_SRC_TWO:
        fir_coeff_interpolation = fir_n24_c0_35_w_hamming_x2_gain_32bit;
        break;
    case SAC_SRC_THREE:
        fir_coeff_interpolation = fir_n24_c0_20_w_hamming_x3_gain_32bit;
        break;
    case SAC_SRC_FOUR:
        fir_coeff_interpolation = fir_n24_c0_15_w_hamming_x4_gain_32bit;
        break;
    case SAC_SRC_SIX:
        fir_coeff_interpolation = fir_n24_c0_10_w_hamming_x6_gain_32bit;
        break;
    default:
        /* Invalid ratio. */
        *status = SAC_ERR_PROCESSING_STAGE_INIT;
        return;
    }

    if (src_instance->cfg.multiply_ratio > SAC_SRC_ONE) {
        /* Allocate interpolate instance memory. */
        allocation_size = sizeof(fir_interpolate_instance_t) * src_instance->cfg.channel_count;
        src_instance->_internal.interpolate_instances = mem_pool_malloc(mem_pool, allocation_size);
        if (src_instance->_internal.interpolate_instances == NULL) {
            *status = SAC_ERR_NOT_ENOUGH_MEMORY;
            return;
        }

        block_size = (src_instance->cfg.payload_size / input_sample_size_byte);
        interpolate_instance = src_instance->_internal.interpolate_instances;
        for (uint8_t i = 0; i < src_instance->cfg.channel_count; i++) {
            /* Allocate FIR state memory. */
            allocation_size = sizeof(int32_t) * (FIR_NUMTAPS + block_size);
            fir_state = mem_pool_malloc(mem_pool, allocation_size);
            if (fir_state == NULL) {
                *status = SAC_ERR_NOT_ENOUGH_MEMORY;
                return;
            }

            /* Input format assignment. */
            input_format = &interpolate_instance[i].input_sample_format;
            bit_depth = src_instance->cfg.input_sample_format.bit_depth;
            input_format->bit_depth = bit_depth;
            input_format->sample_size_byte = input_sample_size_byte;
            if (bit_depth == SAC_16BITS) {
                input_format->sample_bitshift = FIR_BITSHIFT_16BITS;
                input_format->sample_mask = FIR_MASK_16BITS;
            } else {
                input_format->sample_bitshift = FIR_BITSHIFT_24BITS;
                input_format->sample_mask = FIR_MASK_24BITS;
            }

            /* Output format assignment. */
            /* If the SRC instance performs both interpolation and decimation,
             * the same input format is used for both filters.
             */
            output_format = &interpolate_instance[i].output_sample_format;
            if (src_instance->cfg.divide_ratio > SAC_SRC_ONE) {
                output_format->bit_depth = input_format->bit_depth;
                output_format->sample_size_byte = input_format->sample_size_byte;
                output_format->sample_bitshift = input_format->sample_bitshift;
            } else {
                bit_depth = src_instance->cfg.output_sample_format.bit_depth;
                output_format->bit_depth = bit_depth;
                output_format->sample_size_byte = output_sample_size_byte;
                if (bit_depth == SAC_16BITS) {
                    output_format->sample_bitshift = FIR_BITSHIFT_16BITS;
                } else {
                    output_format->sample_bitshift = FIR_BITSHIFT_24BITS;
                }
            }

            /* Initialize interpolate instance. */
            fir_err = fir_interpolate_init(&src_instance->_internal.interpolate_instances[i],
                                           src_instance->cfg.multiply_ratio, FIR_NUMTAPS, fir_coeff_interpolation,
                                           fir_state, block_size);
            if (fir_err != FILTERING_FUNCTION_ERR_NONE) {
                *status = SAC_ERR_PROCESSING_STAGE_INIT;
                return;
            }
        }
    }

    switch (src_instance->cfg.divide_ratio) {
    case SAC_SRC_ONE:
        fir_coeff_decimation = NULL;
        break;
    case SAC_SRC_TWO:
        fir_coeff_decimation = fir_n24_c0_35_w_hamming_32bit;
        break;
    case SAC_SRC_THREE:
        fir_coeff_decimation = fir_n24_c0_20_w_hamming_32bit;
        break;
    case SAC_SRC_FOUR:
        fir_coeff_decimation = fir_n24_c0_15_w_hamming_32bit;
        break;
    case SAC_SRC_SIX:
        fir_coeff_decimation = fir_n24_c0_10_w_hamming_32bit;
        break;
    default:
        /* Invalid ratio. */
        *status = SAC_ERR_PROCESSING_STAGE_INIT;
        return;
    }

    if (src_instance->cfg.divide_ratio > SAC_SRC_ONE) {
        if (src_instance->cfg.multiply_ratio > SAC_SRC_ONE) {
            allocation_size = sizeof(int32_t) * (src_instance->cfg.payload_size / input_sample_size_byte) *
                              src_instance->cfg.multiply_ratio;
            src_instance->_internal.multiply_out_buffer = mem_pool_malloc(mem_pool, allocation_size);
            if (src_instance->_internal.multiply_out_buffer == NULL) {
                *status = SAC_ERR_NOT_ENOUGH_MEMORY;
                return;
            }
        }

        /* Allocate decimate instance memory. */
        allocation_size = sizeof(fir_decimate_instance_t) * src_instance->cfg.channel_count;
        src_instance->_internal.decimate_instances = mem_pool_malloc(mem_pool, allocation_size);
        if (src_instance->_internal.decimate_instances == NULL) {
            *status = SAC_ERR_NOT_ENOUGH_MEMORY;
            return;
        }

        block_size = (src_instance->cfg.payload_size * src_instance->cfg.multiply_ratio) / input_sample_size_byte;
        decimate_instance = src_instance->_internal.decimate_instances;
        for (uint8_t i = 0; i < src_instance->cfg.channel_count; i++) {
            /* Allocate FIR state memory. */
            allocation_size = sizeof(int32_t) * (FIR_NUMTAPS + block_size);
            fir_state = mem_pool_malloc(mem_pool, allocation_size);
            if (fir_state == NULL) {
                *status = SAC_ERR_NOT_ENOUGH_MEMORY;
                return;
            }

            /* Input format assignment. */
            input_format = &decimate_instance[i].input_sample_format;
            if (src_instance->cfg.multiply_ratio > SAC_SRC_ONE) {
                memcpy(input_format, &src_instance->_internal.interpolate_instances->input_sample_format,
                       sizeof(fir_sample_format_t));
            } else {
                bit_depth = src_instance->cfg.input_sample_format.bit_depth;
                input_format->bit_depth = bit_depth;
                input_format->sample_size_byte = input_sample_size_byte;
                if (bit_depth == SAC_16BITS) {
                    input_format->sample_bitshift = FIR_BITSHIFT_16BITS;
                    input_format->sample_mask = FIR_MASK_16BITS;
                } else {
                    input_format->sample_bitshift = FIR_BITSHIFT_24BITS;
                    input_format->sample_mask = FIR_MASK_24BITS;
                }
            }

            /* Output format assignment. */
            bit_depth = src_instance->cfg.output_sample_format.bit_depth;
            decimate_instance[i].output_sample_format.bit_depth = bit_depth;
            decimate_instance[i].output_sample_format.sample_size_byte = output_sample_size_byte;

            /* Output format assignment. */
            output_format = &decimate_instance[i].output_sample_format;
            bit_depth = src_instance->cfg.output_sample_format.bit_depth;
            output_format->bit_depth = bit_depth;
            output_format->sample_size_byte = output_sample_size_byte;
            if (bit_depth == SAC_16BITS) {
                output_format->sample_bitshift = FIR_BITSHIFT_16BITS;
            } else {
                output_format->sample_bitshift = FIR_BITSHIFT_24BITS;
            }

            /* Initialize decimate instance. */
            fir_err = fir_decimate_init(&src_instance->_internal.decimate_instances[i], FIR_NUMTAPS,
                                        src_instance->cfg.divide_ratio, fir_coeff_decimation, fir_state, block_size);
            if (fir_err != FILTERING_FUNCTION_ERR_NONE) {
                *status = SAC_ERR_PROCESSING_STAGE_INIT;
                return;
            }
        }
    }

    if (src_instance->cfg.divide_ratio > SAC_SRC_ONE) {
        discard_accumulator_size = src_instance->cfg.channel_count * FIR_NUMTAPS * input_sample_size_byte;
        if (src_instance->cfg.payload_size < discard_accumulator_size) {
            /* Invalid Configuration. */
            *status = SAC_ERR_PROCESSING_STAGE_INIT;
            return;
        }
        src_instance->_internal.discard_accumulator_size = discard_accumulator_size;
        src_instance->_internal.discard_accumulator = mem_pool_malloc(mem_pool, discard_accumulator_size);
        if (src_instance->_internal.discard_accumulator == NULL) {
            *status = SAC_ERR_NOT_ENOUGH_MEMORY;
            return;
        }
        memset(src_instance->_internal.discard_accumulator, 0, src_instance->_internal.discard_accumulator_size);
    }
}

void sac_src_cmsis_discard_init(void *instance, const char *name, sac_pipeline_t *pipeline, mem_pool_t *mem_pool,
                                sac_status_t *status)
{
    (void)name;
    (void)pipeline;

    int16_t discard_accumulator_size;
    src_cmsis_instance_t *src_instance = instance;
    uint8_t input_sample_size_byte;

    if (src_instance->cfg.input_sample_format.sample_encoding == SAC_SAMPLE_PACKED) {
        input_sample_size_byte = src_instance->cfg.input_sample_format.bit_depth / SAC_BYTE_SIZE_BITS;
    } else {
        input_sample_size_byte = SAC_WORD_SIZE_BYTE;
    }

    if (src_instance->cfg.divide_ratio > SAC_SRC_ONE) {
        discard_accumulator_size = src_instance->cfg.channel_count * FIR_NUMTAPS * input_sample_size_byte;
        if (src_instance->cfg.payload_size < discard_accumulator_size) {
            /* Invalid Configuration. */
            *status = SAC_ERR_PROCESSING_STAGE_INIT;
            return;
        }
        src_instance->_internal.discard_accumulator_size = discard_accumulator_size;
        src_instance->_internal.discard_accumulator = mem_pool_malloc(mem_pool, discard_accumulator_size);
        if (src_instance->_internal.discard_accumulator == NULL) {
            *status = SAC_ERR_NOT_ENOUGH_MEMORY;
            return;
        }
        memset(src_instance->_internal.discard_accumulator, 0, src_instance->_internal.discard_accumulator_size);
    }
}

uint16_t sac_src_cmsis_process(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in,
                               uint16_t size, uint8_t *data_out, sac_status_t *status)
{
    (void)pipeline;
    (void)header;

    uint16_t sample_count_in = 0;
    uint16_t sample_count_out = 0;
    uint16_t accumulator_sample_count = 0;
    uint16_t expected_discard_input_size;
    src_cmsis_instance_t *src_instance = instance;
    uint8_t input_sample_size_byte;
    uint8_t output_sample_size_byte;
    uint16_t data_in_idx;
    uint16_t data_out_idx;
    uint8_t *audio_in;
    uint8_t *audio_out;

    *status = SAC_OK;

    set_word_size(&src_instance->cfg, &input_sample_size_byte, &output_sample_size_byte);

    sample_count_in = size / input_sample_size_byte;

    if (src_instance->cfg.multiply_ratio > SAC_SRC_ONE) {
        accumulator_sample_count = ((FIR_NUMTAPS / src_instance->cfg.multiply_ratio) * src_instance->cfg.channel_count);

        /* Validate input payload size. */
        if (size != src_instance->cfg.payload_size) {
            /* Input size different than what was expected. */
            expected_discard_input_size = (src_instance->cfg.payload_size / input_sample_size_byte) +
                                          (accumulator_sample_count / FIR_SAMPLE_COUNT_CORRECTION_FACTOR);
            if (sample_count_in == expected_discard_input_size) {
                /* Discard transition packet. */
                src_instance->_internal.discard_active = true;
            } else {
                /* Invalid packet size. */
                *status = SAC_ERR_INVALID_PACKET_SIZE;
                return 0;
            }
        }

        audio_in = data_in;
        if (src_instance->cfg.divide_ratio > SAC_SRC_ONE) {
            audio_out = src_instance->_internal.multiply_out_buffer;
        } else {
            audio_out = data_out;
        }

        if (src_instance->_internal.discard_active) {
            /*
             * When switching from SRC discard to process, the decimator will send it's accumulator content manually
             * decimated. The accumulator content allows the interpolator process to simulate latency of FIR filter and
             * feed the interpolator.
             */
            /* Apply artificial delay of decimator on top of manual decimation. */
            audio_in += (accumulator_sample_count / FIR_SAMPLE_COUNT_CORRECTION_FACTOR) * input_sample_size_byte;
            /* Apply sample_count_in correction. */
            sample_count_in -= accumulator_sample_count / FIR_SAMPLE_COUNT_CORRECTION_FACTOR;
        }

        for (uint8_t i = 0; i < src_instance->cfg.channel_count; i++) {
            fir_interpolate(&src_instance->_internal.interpolate_instances[i], audio_in, audio_out,
                            sample_count_in / src_instance->cfg.channel_count, i, src_instance->cfg.channel_count);
        }
        sample_count_out = sample_count_in * src_instance->cfg.multiply_ratio;
    }

    if (src_instance->cfg.divide_ratio > SAC_SRC_ONE) {
        if (src_instance->cfg.multiply_ratio > SAC_SRC_ONE) {
            sample_count_in = sample_count_out;
            audio_in = src_instance->_internal.multiply_out_buffer;
        } else {
            audio_in = data_in;
        }
        audio_out = data_out;
        for (uint8_t i = 0; i < src_instance->cfg.channel_count; i++) {
            fir_decimate(&src_instance->_internal.decimate_instances[i], audio_in, audio_out,
                         sample_count_in / src_instance->cfg.channel_count, i, src_instance->cfg.channel_count);
        }
        sample_count_out = sample_count_in / src_instance->cfg.divide_ratio;
    }

    if (src_instance->_internal.discard_active) {
        /* Copy accumulator samples in output buffer after discard process ended. */
        src_instance->_internal.discard_active = false;

        if (src_instance->cfg.multiply_ratio > SAC_SRC_ONE) {
            /* Manual interpolation of input. */
            for (uint16_t i = 0; i < sample_count_in; i += src_instance->cfg.channel_count) {
                for (uint8_t j = 0; j < src_instance->cfg.channel_count; j++) {
                    for (uint8_t k = 0; k < src_instance->cfg.multiply_ratio; k++) {
                        data_out_idx =
                            (((i * src_instance->cfg.multiply_ratio) + j + (src_instance->cfg.channel_count * k))) *
                            output_sample_size_byte;
                        data_in_idx = (i + j) * input_sample_size_byte;
                        for (uint8_t l = 0; l < output_sample_size_byte; l++) {
                            data_out[data_out_idx + l] = data_in[data_in_idx + l];
                        }
                    }
                }
            }
        }
        if (src_instance->cfg.divide_ratio > SAC_SRC_ONE) {
            /*
             * When switching from SRC discard to process, the decimator will send it's accumulator content manually
             * decimated. The accumulator content allows the interpolator process to simulate latency of FIR filter and
             * feed the interpolator.
             */
            /* Manual decimation of accumulator. */
            accumulator_sample_count = (FIR_NUMTAPS / src_instance->cfg.divide_ratio) * src_instance->cfg.channel_count;
            for (uint16_t i = 0; i < accumulator_sample_count; i += src_instance->cfg.channel_count) {
                for (uint8_t j = 0; j < src_instance->cfg.channel_count; j++) {
                    data_in_idx = ((i * src_instance->cfg.divide_ratio) + j) * input_sample_size_byte;
                    data_out_idx = (i + j) * output_sample_size_byte;
                    for (uint8_t k = 0; k < output_sample_size_byte; k++) {
                        data_out[data_out_idx + k] = src_instance->_internal.discard_accumulator[data_in_idx + k];
                    }
                }
            }
            /* Manual decimation of input. */
            for (uint16_t i = 0; i < sample_count_out - (accumulator_sample_count / FIR_SAMPLE_COUNT_CORRECTION_FACTOR);
                 i += src_instance->cfg.channel_count) {
                for (uint8_t j = 0; j < src_instance->cfg.channel_count; j++) {
                    data_in_idx = ((i * src_instance->cfg.divide_ratio) + j) * input_sample_size_byte;
                    data_out_idx = ((accumulator_sample_count) + i + j) * output_sample_size_byte;
                    for (uint8_t k = 0; k < output_sample_size_byte; k++) {
                        data_out[data_out_idx + k] = data_in[data_in_idx + k];
                    }
                }
            }
            /* Apply sample_count_out correction. */
            sample_count_out += (accumulator_sample_count / FIR_SAMPLE_COUNT_CORRECTION_FACTOR);
        }
    }

    if (src_instance->cfg.divide_ratio > SAC_SRC_ONE) {
        /* Load discard accumulator with last FIR_NUMTAPS samples. */
        memcpy(src_instance->_internal.discard_accumulator,
               &data_in[size - src_instance->_internal.discard_accumulator_size],
               src_instance->_internal.discard_accumulator_size);
    }

    return (sample_count_out * output_sample_size_byte);
}

uint16_t sac_src_cmsis_process_discard(void *instance, sac_pipeline_t *pipeline, sac_header_t *header, uint8_t *data_in,
                                       uint16_t size, uint8_t *data_out, sac_status_t *status)
{
    (void)pipeline;
    (void)header;

    src_cmsis_instance_t *src_instance = instance;
    uint16_t ret = 0;

    *status = SAC_OK;
    src_instance->_internal.discard_active = true;

    /*
     * When discarting data, the decimator will apply a latency equivaltent to the FIR filters.
     * An accumulator will keep the last samples of the previous packet to be played at beginning
     * of next packet.
     */
    if (src_instance->cfg.divide_ratio > SAC_SRC_ONE) {
        /* Copy accumulator samples in output buffer. */
        memcpy(data_out, src_instance->_internal.discard_accumulator, src_instance->_internal.discard_accumulator_size);

        /* Copy input samples in output buffer. */
        memcpy(&data_out[src_instance->_internal.discard_accumulator_size], data_in,
               size - src_instance->_internal.discard_accumulator_size);

        /* Load discard accumulator with last FIR_NUMTAPS samples. */
        memcpy(src_instance->_internal.discard_accumulator,
               &data_in[size - src_instance->_internal.discard_accumulator_size],
               src_instance->_internal.discard_accumulator_size);

        ret = size;
    }

    return ret;
}

/* PRIVATE FUNCTIONS **********************************************************/
void set_word_size(src_cmsis_cfg_t *cmsis_cfg, uint8_t *input_sample_size_byte, uint8_t *output_sample_size_byte)
{
    if (cmsis_cfg->input_sample_format.sample_encoding == SAC_SAMPLE_PACKED) {
        *input_sample_size_byte = cmsis_cfg->input_sample_format.bit_depth / SAC_BYTE_SIZE_BITS;
    } else {
        *input_sample_size_byte = SAC_WORD_SIZE_BYTE;
    }
    if (cmsis_cfg->output_sample_format.sample_encoding == SAC_SAMPLE_PACKED) {
        *output_sample_size_byte = cmsis_cfg->output_sample_format.bit_depth / SAC_BYTE_SIZE_BITS;
    } else {
        *output_sample_size_byte = SAC_WORD_SIZE_BYTE;
    }
}
