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
/* **** Sine Wave Debug Capture (optional) **** */
/* Enabled via CMake preset "puretone-headset-quasar-u5a5-slave-rjf-sine-dbg",
 * or manually by uncommenting the line below.
 * Fills s_debug_buf[] (one full 1 kHz cycle, stereo) for debugger inspection. */
/* #define SINE_DEBUG_CAPTURE */

/* **** No Codec / No I2S MUX GPIO (optional) **** */
/* Enable via CMake by passing -DNO_CODEC=1 (or adding "NO_CODEC": "1" to CMakePresets.json).
 * Use for boards where I2S connects directly to the SOC with no external codec and no MUX GPIO
 * (e.g. OneOdio customer board). Skips MAX98091 I2C init/config and I2S mux GPIO control. */
/* #define NO_CODEC */

/* **** Main Channel Settings. **** */

#define MAIN_CHANNEL_SAMPLE_RATE_HZ 96000
#define MAIN_CHANNEL_SAMPLE_COUNT   40
#define MAIN_CHANNEL_CHANNEL_COUNT  2
#define MAIN_CHANNEL_BIT_DEPTH      24
/* Maximum Latency. */
/* Caps every per-mode latency below, because the consumer endpoint queue is sized from it --
 * raising a mode past this value asks for a target the queue cannot physically hold. */
#define MAIN_CHANNEL_MAX_LATENCY_MS 30
/* Fallback modes Latency. */
#define MAIN_CHANNEL_FBK_0_LATENCY_MS 5
#define MAIN_CHANNEL_FBK_1_LATENCY_MS 7
#define MAIN_CHANNEL_FBK_2_LATENCY_MS 10
/* Buffer depth is how long an outage the rung can ride out: once the consumer queue drains,
 * no amount of retransmission helps because the retried packet misses its playback deadline.
 * A hand or body blocking the path lasts hundreds of ms, against which 15 ms of buffer was
 * never going to be enough -- which is the piece the accumulator and ISI work did not address.
 * SPARK's ladder widens the same way on the way down (5 / 7 / 10 / 15 / 20 for their mono
 * rung), and the cost is only paid on the rung the link falls to, never in normal playback. */
#define MAIN_CHANNEL_FBK_3_LATENCY_MS 30
/* Fallback modes sample count. */
#define MAIN_CHANNEL_FBK_0_SAMPLE_COUNT 40
#define MAIN_CHANNEL_FBK_1_SAMPLE_COUNT 34
#define MAIN_CHANNEL_FBK_2_SAMPLE_COUNT 34
/* **** Mode 3 (bottom rung) knobs -- the three factors under comparison **** */

/* 1 puts the rung at 24 kHz (1:4 resampler), 0 leaves it at 48 kHz (the 1:2 the rest of the
 * ladder already uses).
 *
 * Worth knowing before reaching for this: 24 kHz does NOT buy retransmission headroom. Slots are
 * spent per packet, not per byte, so a smaller payload changes nothing about how many attempts a
 * packet gets -- only the accumulator does that, and it does it at either rate. What 24 kHz buys
 * is bitrate, 192 vs 384 kbps, which is a range argument rather than an obstruction one. What it
 * costs is a resampler swap at the 2<->3 boundary, which is audible as a soft pop because neither
 * new SRC instance has a discard stage warming it, and an anti-alias cutoff near 7 kHz that makes
 * the rung sound dull. */
#ifndef FBK3_RUNG_24K
#define FBK3_RUNG_24K 1
#endif

/* Accumulator ratio for mode 3, as mul/div. This is the retransmission-headroom knob: the
 * accumulator sits before the resampler, so it alone decides how often a packet leaves, and
 * attempts per packet is just the coordinator's 3111 slots/s divided by the packet rate.
 *   23/10 -> 0.96 ms per packet -> 1043 pkt/s -> 3.0 attempts
 *   46/10 -> 1.92 ms per packet ->  522 pkt/s -> 6.0 attempts */
#ifndef MAIN_CHANNEL_FBK_3_ACC_MUL
#define MAIN_CHANNEL_FBK_3_ACC_MUL 46
#endif
#define MAIN_CHANNEL_FBK_3_ACC_DIV 10

/* Derived, never hand-written. The fallback stage copies this into
 * pipeline->_internal.current_sample_count once per packet and the headset's interpolator rejects
 * anything that does not match (sac_src_cmsis.c:479), so a value that has drifted out of step with
 * the accumulator is not a glitch -- it is every mode 3 packet dropped and total silence on that
 * rung. That bug has already been written once by hand; deriving it makes it unwritable.
 *
 * Both divisions come out exact for the ratios above: 40 x 46/10 = 184, and 184 is divisible by
 * both 2 and 4. Check that still holds if the accumulator ratio changes. */
#define MAIN_CHANNEL_FBK_3_RUNG_DIV (FBK3_RUNG_24K ? 4 : 2)
#define MAIN_CHANNEL_FBK_3_SAMPLE_COUNT                                                          \
    (((MAIN_CHANNEL_SAMPLE_COUNT * MAIN_CHANNEL_FBK_3_ACC_MUL) / MAIN_CHANNEL_FBK_3_ACC_DIV) /   \
     MAIN_CHANNEL_FBK_3_RUNG_DIV)

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

/* Fallback modes payload size. */
#define MAIN_CHANNEL_FALLBACK_PAYLOAD_SIZE                                                            \
    {                                                                                                 \
        SAC_CALCULATE_PAYLOAD_SIZE(MAIN_CHANNEL_FBK_1_SAMPLE_COUNT, MAIN_CHANNEL_CHANNEL_COUNT, 24) + \
            MAIN_CHANNEL_FALLBACK_HEADER_SIZE,                                                        \
        SAC_CALCULATE_PAYLOAD_SIZE(MAIN_CHANNEL_FBK_2_SAMPLE_COUNT, MAIN_CHANNEL_CHANNEL_COUNT, 16) + \
            MAIN_CHANNEL_FALLBACK_HEADER_SIZE,                                                        \
        SAC_CALCULATE_PAYLOAD_SIZE(MAIN_CHANNEL_FBK_3_SAMPLE_COUNT, MAIN_CHANNEL_CHANNEL_COUNT,       \
                                   SAC_COMPRESSION_SAMPLE_RESOLUTION) +                               \
            MAIN_CHANNEL_FALLBACK_COMPRESSION_HEADER_SIZE,                                            \
    }

/* Accumulator settings.
 *
 * The accumulator sits BEFORE the resampler, so it is the only stage that changes how often a
 * packet leaves: it releases one once it has collected N x 40 samples/ch at 96 kHz, whatever the
 * resampler downstream turns those into. That makes it the only control over packet rate, and
 * packet rate is what buys retransmission headroom -- the schedule hands the coordinator a fixed
 * 7 slots per 2.25 ms cycle, one frame per slot under stop-and-wait ARQ, so attempts per packet
 * is simply slots/s divided by packets/s.
 *
 * Mode 3 doubled from 23/10 to 46/10 for that reason. It is the same trick as folding two packets
 * into one, and it lands the payload at exactly the 54 B it already was: the extra samples are
 * paid for by carrying one header and one pair of ADPCM states instead of two. The SWC fallback
 * thresholds therefore do not move.
 *
 *   mode 3 before:  92 samples/ch @96k -> 0.96 ms per packet -> 1043 pkt/s -> 3.0 attempts
 *   mode 3 after:  184 samples/ch @96k -> 1.92 ms per packet ->  522 pkt/s -> 6.0 attempts
 *
 * This is the knob every earlier attempt missed. Dropping the rung to 24 kHz, raising FEC and
 * raising ISI all changed how likely a single transmission is to survive; none of them changed
 * how many transmissions a packet gets, which is what a sudden obstruction actually consumes. */
/* Mode 3 is the largest ratio on the ladder, so it also sets the cap the buffers are sized from. */
#define MAIN_CHANNEL_MAX_ACC_MUL MAIN_CHANNEL_FBK_3_ACC_MUL
#define MAIN_CHANNEL_MAX_ACC_DIV MAIN_CHANNEL_FBK_3_ACC_DIV

#define MAIN_CHANNEL_ACC_MUL          \
    {                                 \
        1,                            \
        17,                           \
        17,                           \
        MAIN_CHANNEL_FBK_3_ACC_MUL,   \
    }
#define MAIN_CHANNEL_ACC_DIV          \
    {                                 \
        1,                            \
        10,                           \
        10,                           \
        MAIN_CHANNEL_FBK_3_ACC_DIV,   \
    }

/* Fallback latency. */
#define MAIN_CHANNEL_FALLBACK_LATENCY_QUEUE_SIZE \
    {                                            \
        MAIN_CHANNEL_FBK_0_LATENCY_QUEUE_SIZE,   \
        MAIN_CHANNEL_FBK_1_LATENCY_QUEUE_SIZE,   \
        MAIN_CHANNEL_FBK_2_LATENCY_QUEUE_SIZE,   \
        MAIN_CHANNEL_FBK_3_LATENCY_QUEUE_SIZE,   \
    }

/* Fallback latency fifo size. */
#define MAIN_CHANNEL_FALLBACK_LATENCY_FIFO_SIZE                                                        \
    {                                                                                                  \
        (MAIN_CHANNEL_FBK_0_LATENCY_QUEUE_SIZE * ((MAIN_CHANNEL_BIT_DEPTH + 7) / SAC_BYTE_SIZE_BITS)), \
        (MAIN_CHANNEL_FBK_1_LATENCY_QUEUE_SIZE * ((MAIN_CHANNEL_BIT_DEPTH + 7) / SAC_BYTE_SIZE_BITS)), \
        (MAIN_CHANNEL_FBK_2_LATENCY_QUEUE_SIZE * ((MAIN_CHANNEL_BIT_DEPTH + 7) / SAC_BYTE_SIZE_BITS)), \
        (MAIN_CHANNEL_FBK_3_LATENCY_QUEUE_SIZE * ((MAIN_CHANNEL_BIT_DEPTH + 7) / SAC_BYTE_SIZE_BITS)), \
    }

/* **** Back Channel Settings. **** */

#define BACK_CHANNEL_SAMPLE_RATE_HZ 48000
#define BACK_CHANNEL_SAMPLE_COUNT   68
#define BACK_CHANNEL_CHANNEL_COUNT  1
#define BACK_CHANNEL_BIT_DEPTH      16
/* Maximum Latency. */
#define BACK_CHANNEL_MAX_LATENCY_MS 15
/* Fallback modes Latency. */
#define BACK_CHANNEL_FBK_0_LATENCY_MS 10
#define BACK_CHANNEL_FBK_1_LATENCY_MS 15
/* Fallback mode sample count. */
#define BACK_CHANNEL_FBK_1_SAMPLE_COUNT 120

/* A header is added to compressed audio samples during fallback. */
#define BACK_CHANNEL_FALLBACK_COMPRESSION_HEADER_SIZE \
    (sizeof(sac_header_t) + SAC_COMPRESSION_HEADER_SIZE(BACK_CHANNEL_CHANNEL_COUNT))

/* Calculated values. */
#define BACK_CHANNEL_SWC_PAYLOAD_SIZE \
    SAC_CALCULATE_PAYLOAD_SIZE(BACK_CHANNEL_SAMPLE_COUNT, BACK_CHANNEL_CHANNEL_COUNT, BACK_CHANNEL_BIT_DEPTH)
#define BACK_CHANNEL_I2S_SAMPLE_COUNT ((BACK_CHANNEL_SAMPLE_COUNT * I2S_SAMPLE_RATE_HZ) / BACK_CHANNEL_SAMPLE_RATE_HZ)
#define BACK_CHANNEL_I2S_PAYLOAD_SIZE \
    SAC_CALCULATE_PAYLOAD_SIZE(BACK_CHANNEL_I2S_SAMPLE_COUNT, BACK_CHANNEL_CHANNEL_COUNT, I2S_DMA_BIT_DEPTH)
/* Size of the latency queue used by the Audio Core for the back channel. */
#define BACK_CHANNEL_LATENCY_QUEUE_SIZE                                                                        \
    SAC_CALCULATE_LATENCY_QUEUE_SIZE(BACK_CHANNEL_MAX_LATENCY_MS, CODEC_LATENCY_MS, BACK_CHANNEL_SAMPLE_COUNT, \
                                     BACK_CHANNEL_SAMPLE_RATE_HZ)
#define BACK_CHANNEL_FBK_0_LATENCY_QUEUE_SIZE                                                                    \
    SAC_CALCULATE_LATENCY_QUEUE_SIZE(BACK_CHANNEL_FBK_0_LATENCY_MS, CODEC_LATENCY_MS, BACK_CHANNEL_SAMPLE_COUNT, \
                                     BACK_CHANNEL_SAMPLE_RATE_HZ)
#define BACK_CHANNEL_FBK_1_LATENCY_QUEUE_SIZE                                                                    \
    SAC_CALCULATE_LATENCY_QUEUE_SIZE(BACK_CHANNEL_FBK_1_LATENCY_MS, CODEC_LATENCY_MS, BACK_CHANNEL_SAMPLE_COUNT, \
                                     BACK_CHANNEL_SAMPLE_RATE_HZ)

/* Fallback modes payload size. */
#define BACK_CHANNEL_FALLBACK_PAYLOAD_SIZE                                                   \
    (SAC_CALCULATE_PAYLOAD_SIZE(BACK_CHANNEL_FBK_1_SAMPLE_COUNT, BACK_CHANNEL_CHANNEL_COUNT, \
                                SAC_COMPRESSION_SAMPLE_RESOLUTION) +                         \
     BACK_CHANNEL_FALLBACK_COMPRESSION_HEADER_SIZE)

/* Accumulator settings. */
#define BACK_CHANNEL_MAX_ACC_MUL 30
#define BACK_CHANNEL_MAX_ACC_DIV 17

#define BACK_CHANNEL_ACC_MUL \
    {                        \
        1,                   \
        30,                  \
    }
#define BACK_CHANNEL_ACC_DIV \
    {                        \
        1,                   \
        17,                  \
    }

/* Fallback latency queue size. */
#define BACK_CHANNEL_FALLBACK_LATENCY_QUEUE_SIZE \
    {                                            \
        BACK_CHANNEL_FBK_0_LATENCY_QUEUE_SIZE,   \
        BACK_CHANNEL_FBK_1_LATENCY_QUEUE_SIZE,   \
    }

/* Fallback latency fifo size. */
#define BACK_CHANNEL_FALLBACK_LATENCY_FIFO_SIZE                                                        \
    {                                                                                                  \
        (BACK_CHANNEL_FBK_0_LATENCY_QUEUE_SIZE * ((BACK_CHANNEL_BIT_DEPTH + 7) / SAC_BYTE_SIZE_BITS)), \
        (BACK_CHANNEL_FBK_1_LATENCY_QUEUE_SIZE * ((BACK_CHANNEL_BIT_DEPTH + 7) / SAC_BYTE_SIZE_BITS)), \
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
/* Number of extra packets to buffer when receiving from USB FS. */
#define BACK_CHANNEL_USB_FS_PRODUCER_BUFFERING (((BACK_CHANNEL_SAMPLE_RATE_HZ / 1000) / BACK_CHANNEL_SAMPLE_COUNT))

#endif /* SAC_CFG_H_ */
