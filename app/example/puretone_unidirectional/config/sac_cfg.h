/** @file  sac_cfg.h
 *  @brief Configuration constants for the SPARK Audio Core.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SAC_CFG_H_
#define SAC_CFG_H_

#include "sac_compression.h"
#include "sac_utils.h"

/* CONSTANTS ******************************************************************/
/* **** Main Channel Settings. **** */

#define MAIN_CHANNEL_SAMPLE_RATE_HZ 96000
#define MAIN_CHANNEL_SAMPLE_COUNT   40
#define MAIN_CHANNEL_CHANNEL_COUNT  2
#define MAIN_CHANNEL_BIT_DEPTH      24
/* Maximum Latency. */
/* Caps every per-mode latency below, because the consumer endpoint queue is sized from it --
 * a mode asking for more than this wants a target the queue cannot physically hold. */
#define MAIN_CHANNEL_MAX_LATENCY_MS 40
/* Fallback modes Latency.
 *
 * Buffer depth is how long an outage the rung can ride out, and it is a different resource from
 * retransmission: once the consumer queue drains, no number of retries helps, because the retried
 * packet has missed its playback deadline. A hand or a body blocking the path lasts hundreds of
 * ms, against which the 15 ms the bottom rungs used to carry was never going to be enough.
 *
 * Only the bottom two widen. The cost -- 40 ms of audio delay -- is paid on the rung the link has
 * fallen to, never during normal playback, which is why modes 0-2 stay where they are. SPARK's
 * own ladder widens the same way on the way down. */
#define MAIN_CHANNEL_FBK_0_LATENCY_MS 5
#define MAIN_CHANNEL_FBK_1_LATENCY_MS 7
#define MAIN_CHANNEL_FBK_2_LATENCY_MS 10
#define MAIN_CHANNEL_FBK_3_LATENCY_MS 40
#define MAIN_CHANNEL_FBK_4_LATENCY_MS 40
/* Fallback modes sample count. */
#define MAIN_CHANNEL_FBK_0_SAMPLE_COUNT 40
#define MAIN_CHANNEL_FBK_1_SAMPLE_COUNT 34
#define MAIN_CHANNEL_FBK_2_SAMPLE_COUNT 34

/* **** Bottom rungs (modes 3 and 4) knobs **** */
/* Accumulator ratio, as mul/div. This is the retransmission-headroom knob, and the only one: the
 * accumulator sits BEFORE the resampler, so it alone decides how often a packet leaves, and
 * attempts per packet is just the coordinator's slot rate divided by the packet rate. Slots are
 * spent per packet, not per byte -- which is why nothing else on the ladder moved the close-range
 * dropouts. A lower sample rate, more FEC and more ISI all change how likely one transmission is
 * to survive; none of them change how many transmissions a packet gets.
 *   23/10 -> 0.96 ms per packet -> 1043 pkt/s -> 3.0 attempts
 *   46/10 -> 1.92 ms per packet ->  522 pkt/s -> 6.0 attempts
 * Both bottom rungs take 46/10. Their payloads still descend, because they differ by sample rate
 * rather than by packet rate: 100 B at 48 kHz against 54 B at 24 kHz. */
#define MAIN_CHANNEL_FBK_3_ACC_MUL 46
#define MAIN_CHANNEL_FBK_3_ACC_DIV 10
#define MAIN_CHANNEL_FBK_4_ACC_MUL 46
#define MAIN_CHANNEL_FBK_4_ACC_DIV 10
/* Resampler ratio for mode 3: 2 puts it at 48 kHz. */
#define MAIN_CHANNEL_FBK_3_RUNG_DIV 2
/* DERIVED -- never hand-written, for the reason given on mode 4's count below.
 *   40 x 46/10 = 184 @96 kHz, / 2 = 92 @48 kHz. */
#define MAIN_CHANNEL_FBK_3_SAMPLE_COUNT                                                       \
    ((MAIN_CHANNEL_SAMPLE_COUNT * MAIN_CHANNEL_FBK_3_ACC_MUL / MAIN_CHANNEL_FBK_3_ACC_DIV) / \
     MAIN_CHANNEL_FBK_3_RUNG_DIV)
/* Resampler ratio: 4 puts the rung at 24 kHz against the 96 kHz base. It has to be a single 1:4
 * and not two chained 1:2 -- the interpolation path validates its input against
 * pipeline->_internal.current_sample_count, which the fallback stage writes once per packet and
 * never updates between stages, so a second chained stage always sees twice what the check
 * expects and is rejected. */
#define MAIN_CHANNEL_FBK_4_RUNG_DIV 4
/* DERIVED -- never hand-written. The accumulator collects SAMPLE_COUNT x ACC_MUL/ACC_DIV samples
 * per channel at 96 kHz and the resampler divides by the rung ratio:
 *   40 x 46/10 = 184 @96 kHz, / 4 = 46 @24 kHz.
 * 184 / 4 divides exactly, which is required: the fallback stage copies this into
 * current_sample_count once per packet and the node's interpolator rejects any packet that does
 * not match, so a value out of step with the accumulator is not a glitch -- it is every mode 4
 * packet dropped and silence on that rung. */
#define MAIN_CHANNEL_FBK_4_SAMPLE_COUNT                                                       \
    ((MAIN_CHANNEL_SAMPLE_COUNT * MAIN_CHANNEL_FBK_4_ACC_MUL / MAIN_CHANNEL_FBK_4_ACC_DIV) / \
     MAIN_CHANNEL_FBK_4_RUNG_DIV)

/* A header is added to audio samples during fallback. */
#define MAIN_CHANNEL_FALLBACK_HEADER_SIZE sizeof(sac_header_t)
/* A header is added to compressed audio samples during fallback. */
#define MAIN_CHANNEL_FALLBACK_COMPRESSION_HEADER_SIZE \
    (sizeof(sac_header_t) + SAC_COMPRESSION_HEADER_SIZE(MAIN_CHANNEL_CHANNEL_COUNT))

/* Calculated values. */
#define MAIN_CHANNEL_SWC_PAYLOAD_SIZE \
    SAC_CALCULATE_PAYLOAD_SIZE(MAIN_CHANNEL_SAMPLE_COUNT, MAIN_CHANNEL_CHANNEL_COUNT, MAIN_CHANNEL_BIT_DEPTH)
#define MAIN_CHANNEL_I2S_PAYLOAD_SIZE \
    SAC_CALCULATE_PAYLOAD_SIZE(MAIN_CHANNEL_SAMPLE_COUNT, MAIN_CHANNEL_CHANNEL_COUNT, I2S_DMA_BIT_DEPTH)
/* Size of the latency queue used by the Audio Core for the main channel. */
#define MAIN_CHANNEL_LATENCY_QUEUE_SIZE                                                                        \
    SAC_CALCULATE_LATENCY_QUEUE_SIZE(MAIN_CHANNEL_MAX_LATENCY_MS, CODEC_LATENCY_MS, MAIN_CHANNEL_SAMPLE_COUNT, \
                                     MAIN_CHANNEL_SAMPLE_RATE_HZ)
#define MAIN_CHANNEL_FBK_0_LATENCY_QUEUE_SIZE                                                                    \
    SAC_CALCULATE_LATENCY_QUEUE_SIZE(MAIN_CHANNEL_FBK_0_LATENCY_MS, CODEC_LATENCY_MS, MAIN_CHANNEL_SAMPLE_COUNT, \
                                     MAIN_CHANNEL_SAMPLE_RATE_HZ)
#define MAIN_CHANNEL_FBK_1_LATENCY_QUEUE_SIZE                                                                    \
    SAC_CALCULATE_LATENCY_QUEUE_SIZE(MAIN_CHANNEL_FBK_1_LATENCY_MS, CODEC_LATENCY_MS, MAIN_CHANNEL_SAMPLE_COUNT, \
                                     MAIN_CHANNEL_SAMPLE_RATE_HZ)
#define MAIN_CHANNEL_FBK_2_LATENCY_QUEUE_SIZE                                                                    \
    SAC_CALCULATE_LATENCY_QUEUE_SIZE(MAIN_CHANNEL_FBK_2_LATENCY_MS, CODEC_LATENCY_MS, MAIN_CHANNEL_SAMPLE_COUNT, \
                                     MAIN_CHANNEL_SAMPLE_RATE_HZ)
#define MAIN_CHANNEL_FBK_3_LATENCY_QUEUE_SIZE                                                                    \
    SAC_CALCULATE_LATENCY_QUEUE_SIZE(MAIN_CHANNEL_FBK_3_LATENCY_MS, CODEC_LATENCY_MS, MAIN_CHANNEL_SAMPLE_COUNT, \
                                     MAIN_CHANNEL_SAMPLE_RATE_HZ)
#define MAIN_CHANNEL_FBK_4_LATENCY_QUEUE_SIZE                                                                    \
    SAC_CALCULATE_LATENCY_QUEUE_SIZE(MAIN_CHANNEL_FBK_4_LATENCY_MS, CODEC_LATENCY_MS, MAIN_CHANNEL_SAMPLE_COUNT, \
                                     MAIN_CHANNEL_SAMPLE_RATE_HZ)

/* Fallback modes payload size. Named per rung as well as collected into the array, so the
 * descending order that swc_connection_set_fallback_cfg() asserts on can be checked at compile
 * time instead of showing up as a red LED at init. */
#define MAIN_CHANNEL_FBK_1_PAYLOAD_SIZE                                                              \
    (SAC_CALCULATE_PAYLOAD_SIZE(MAIN_CHANNEL_FBK_1_SAMPLE_COUNT, MAIN_CHANNEL_CHANNEL_COUNT, 24) +   \
     MAIN_CHANNEL_FALLBACK_HEADER_SIZE)
#define MAIN_CHANNEL_FBK_2_PAYLOAD_SIZE                                                              \
    (SAC_CALCULATE_PAYLOAD_SIZE(MAIN_CHANNEL_FBK_2_SAMPLE_COUNT, MAIN_CHANNEL_CHANNEL_COUNT, 16) +   \
     MAIN_CHANNEL_FALLBACK_HEADER_SIZE)
#define MAIN_CHANNEL_FBK_3_PAYLOAD_SIZE                                                              \
    (SAC_CALCULATE_PAYLOAD_SIZE(MAIN_CHANNEL_FBK_3_SAMPLE_COUNT, MAIN_CHANNEL_CHANNEL_COUNT,         \
                                SAC_COMPRESSION_SAMPLE_RESOLUTION) +                                 \
     MAIN_CHANNEL_FALLBACK_COMPRESSION_HEADER_SIZE)
#define MAIN_CHANNEL_FBK_4_PAYLOAD_SIZE                                                              \
    (SAC_CALCULATE_PAYLOAD_SIZE(MAIN_CHANNEL_FBK_4_SAMPLE_COUNT, MAIN_CHANNEL_CHANNEL_COUNT,         \
                                SAC_COMPRESSION_SAMPLE_RESOLUTION) +                                 \
     MAIN_CHANNEL_FALLBACK_COMPRESSION_HEADER_SIZE)
#define MAIN_CHANNEL_FALLBACK_PAYLOAD_SIZE \
    {                                      \
        MAIN_CHANNEL_FBK_1_PAYLOAD_SIZE,   \
        MAIN_CHANNEL_FBK_2_PAYLOAD_SIZE,   \
        MAIN_CHANNEL_FBK_3_PAYLOAD_SIZE,   \
        MAIN_CHANNEL_FBK_4_PAYLOAD_SIZE,   \
    }

/* Accumulator settings. Sizes the accumulator buffer, so it must be the largest ratio any mode
 * asks for -- which is the bottom rungs'. */
#define MAIN_CHANNEL_MAX_ACC_MUL MAIN_CHANNEL_FBK_3_ACC_MUL
#define MAIN_CHANNEL_MAX_ACC_DIV MAIN_CHANNEL_FBK_3_ACC_DIV

#define MAIN_CHANNEL_ACC_MUL           \
    {                                  \
        1,                             \
        17,                            \
        17,                            \
        MAIN_CHANNEL_FBK_3_ACC_MUL,    \
        MAIN_CHANNEL_FBK_4_ACC_MUL,    \
    }
#define MAIN_CHANNEL_ACC_DIV           \
    {                                  \
        1,                             \
        10,                            \
        10,                            \
        MAIN_CHANNEL_FBK_3_ACC_DIV,    \
        MAIN_CHANNEL_FBK_4_ACC_DIV,    \
    }

/* Fallback latency. */
#define MAIN_CHANNEL_FALLBACK_LATENCY_QUEUE_SIZE \
    {                                            \
        MAIN_CHANNEL_FBK_0_LATENCY_QUEUE_SIZE,   \
        MAIN_CHANNEL_FBK_1_LATENCY_QUEUE_SIZE,   \
        MAIN_CHANNEL_FBK_2_LATENCY_QUEUE_SIZE,   \
        MAIN_CHANNEL_FBK_3_LATENCY_QUEUE_SIZE,   \
        MAIN_CHANNEL_FBK_4_LATENCY_QUEUE_SIZE,   \
    }

/* Fallback latency fifo size. */
#define MAIN_CHANNEL_FALLBACK_LATENCY_FIFO_SIZE                                                        \
    {                                                                                                  \
        (MAIN_CHANNEL_FBK_0_LATENCY_QUEUE_SIZE * ((MAIN_CHANNEL_BIT_DEPTH + 7) / SAC_BYTE_SIZE_BITS)), \
        (MAIN_CHANNEL_FBK_1_LATENCY_QUEUE_SIZE * ((MAIN_CHANNEL_BIT_DEPTH + 7) / SAC_BYTE_SIZE_BITS)), \
        (MAIN_CHANNEL_FBK_2_LATENCY_QUEUE_SIZE * ((MAIN_CHANNEL_BIT_DEPTH + 7) / SAC_BYTE_SIZE_BITS)), \
        (MAIN_CHANNEL_FBK_3_LATENCY_QUEUE_SIZE * ((MAIN_CHANNEL_BIT_DEPTH + 7) / SAC_BYTE_SIZE_BITS)), \
        (MAIN_CHANNEL_FBK_4_LATENCY_QUEUE_SIZE * ((MAIN_CHANNEL_BIT_DEPTH + 7) / SAC_BYTE_SIZE_BITS)), \
    }

/* **** I2S Settings. **** */

/* The I2S sample rate will be shared by all audio streams.
 * SRC Audio processing is required if a stream uses a different sample rate.
 */
#define I2S_SAMPLE_RATE_HZ MAIN_CHANNEL_SAMPLE_RATE_HZ
/* The I2S bit depth will be shared by all audio streams.
 * Packing Audio processing is required if a stream uses a different bit depth.
 */
#define I2S_BIT_DEPTH MAIN_CHANNEL_BIT_DEPTH
/* I2S DMA bit depth is defined by the capabilities of the DMA transfer.
 * The DMA can transport data on either a byte (8-bit), a half-word (16-bit) or a word (32-bit)
 * Packing Audio processing is required if a stream uses a bit depth smaller than the DMA bit depth.
 */
#define I2S_DMA_BIT_DEPTH (((I2S_BIT_DEPTH) <= 8) ? 8 : (((I2S_BIT_DEPTH) <= 16) ? 16 : (32)))
/* Latency induced by the codec's ADC and DAC. */
#define CODEC_LATENCY_MS 1

/* **** USB Settings. **** */
/* Number of extra packets to buffer when receiving from USB FS. */
#define MAIN_CHANNEL_USB_FS_PRODUCER_BUFFERING (((MAIN_CHANNEL_SAMPLE_RATE_HZ / 1000) / MAIN_CHANNEL_SAMPLE_COUNT))

#endif /* SAC_CFG_H_ */
