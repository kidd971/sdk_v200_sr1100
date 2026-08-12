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

/*
 * Configuration guide:
 *   - MAIN_CHANNEL_OTA_UNCOMPRESSED_BIT_DEPTH: 20 or 24 bits.
 *       Packing stages are automatically enabled or bypassed depending on this value.
 *   - MAIN_CHANNEL_OTA_PACKED_BIT_DEPTH: Fallback mode 2 packed depth.
 *       Current application support is 16-bit.
 */
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

/* **** OTA SWC settings. **** */
#define MAIN_CHANNEL_OTA_UNCOMPRESSED_BIT_DEPTH 24
#define MAIN_CHANNEL_OTA_PACKED_BIT_DEPTH       16
/* **** Main Channel Settings. **** */
#define MAIN_CHANNEL_SAMPLE_RATE_HZ        96000
#define MAIN_CHANNEL_SAMPLE_COUNT          40
#define MAIN_CHANNEL_CHANNEL_COUNT         2
#define MAIN_CHANNEL_AUDIO_IFACE_BIT_DEPTH 24
/* Maximum Latency. */
#define MAIN_CHANNEL_MAX_LATENCY_MS 15
/* Fallback modes Latency. */
#define MAIN_CHANNEL_FBK_0_LATENCY_MS 5
#define MAIN_CHANNEL_FBK_1_LATENCY_MS 7
#define MAIN_CHANNEL_FBK_2_LATENCY_MS 10
#define MAIN_CHANNEL_FBK_3_LATENCY_MS 15
/* Fallback modes sample count. */
#define MAIN_CHANNEL_FBK_0_SAMPLE_COUNT 40
#define MAIN_CHANNEL_FBK_1_SAMPLE_COUNT 34
#define MAIN_CHANNEL_FBK_2_SAMPLE_COUNT 34
/* **** Bottom rung (mode 3) knobs **** */
/* Accumulator ratio for mode 3, as mul/div. This is the retransmission-headroom knob: the
 * accumulator sits BEFORE the resampler, so it alone decides how often a packet leaves, and
 * attempts per packet is just the coordinator's slot rate divided by the packet rate. Slots are
 * spent per packet, not per byte, so nothing else on the ladder -- not a lower sample rate, not
 * FEC, not ISI -- changes how many attempts a packet gets.
 *   23/10 -> 0.96 ms per packet -> 1043 pkt/s -> 3.0 attempts
 *   46/10 -> 1.92 ms per packet ->  522 pkt/s -> 6.0 attempts
 * The cost is 1.92 ms of packetisation delay inside mode 3's latency budget, and a payload that
 * grows in proportion: 100 B against the 54 B of 23/10. Both fit -- mode 0 already puts 242 B in
 * the same slot, and the SWC fallback thresholds stay in descending order at 206 / 138 / 100. */
#ifndef MAIN_CHANNEL_FBK_3_ACC_MUL
#define MAIN_CHANNEL_FBK_3_ACC_MUL 46
#endif
#define MAIN_CHANNEL_FBK_3_ACC_DIV 10
/* Resampler ratio on the bottom rung: 2 puts it at 48 kHz, which is what the rest of the ladder
 * already uses. Named so the arithmetic below reads, and so a 24 kHz rung is a change of this
 * number plus a second SRC instance rather than a rewrite of the derivation. */
#define MAIN_CHANNEL_FBK_3_RUNG_DIV 2
/* DERIVED -- never hand-written. The accumulator collects MAIN_CHANNEL_SAMPLE_COUNT x ACC_MUL/DIV
 * samples per channel at 96 kHz, and the resampler divides that by the rung ratio:
 *   40 x 46/10 = 184 @96 kHz, / 2 = 92 @48 kHz.
 *
 * This has to track MAIN_CHANNEL_FBK_3_ACC_MUL. The fallback stage copies this value into
 * pipeline->_internal.current_sample_count once per packet, and the headset's interpolator
 * rejects any packet whose size does not match (sac_src_cmsis.c:479) -- so a value that has
 * drifted out of step with the accumulator is not a glitch, it is every mode 3 packet dropped
 * and total silence on that rung. */
#define MAIN_CHANNEL_FBK_3_SAMPLE_COUNT                                                       \
    ((MAIN_CHANNEL_SAMPLE_COUNT * MAIN_CHANNEL_FBK_3_ACC_MUL / MAIN_CHANNEL_FBK_3_ACC_DIV) / \
     MAIN_CHANNEL_FBK_3_RUNG_DIV)

/* A header is added to audio samples during fallback. */
#define MAIN_CHANNEL_FALLBACK_HEADER_SIZE sizeof(sac_header_t)
/* A header is added to compressed audio samples during fallback. */
#define MAIN_CHANNEL_FALLBACK_COMPRESSION_HEADER_SIZE \
    (sizeof(sac_header_t) + SAC_COMPRESSION_HEADER_SIZE(MAIN_CHANNEL_CHANNEL_COUNT))

/* Calculated values. */
#define MAIN_CHANNEL_SWC_PAYLOAD_SIZE                                                 \
    SAC_CALCULATE_PAYLOAD_SIZE(MAIN_CHANNEL_SAMPLE_COUNT, MAIN_CHANNEL_CHANNEL_COUNT, \
                               MAIN_CHANNEL_OTA_UNCOMPRESSED_BIT_DEPTH)
#define MAIN_CHANNEL_USB_PAYLOAD_SIZE \
    SAC_CALCULATE_PAYLOAD_SIZE(MAIN_CHANNEL_SAMPLE_COUNT, MAIN_CHANNEL_CHANNEL_COUNT, MAIN_CHANNEL_USB_BIT_DEPTH)
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

/* Fallback modes payload size. Named per rung as well as collected into the array, so the
 * descending-order requirement the SWC asserts on can be checked at compile time. */
#define MAIN_CHANNEL_FBK_1_PAYLOAD_SIZE                                                             \
    (SAC_CALCULATE_PAYLOAD_SIZE(MAIN_CHANNEL_FBK_1_SAMPLE_COUNT, MAIN_CHANNEL_CHANNEL_COUNT,        \
                                MAIN_CHANNEL_OTA_UNCOMPRESSED_BIT_DEPTH) +                          \
     MAIN_CHANNEL_FALLBACK_HEADER_SIZE)
#define MAIN_CHANNEL_FBK_2_PAYLOAD_SIZE                                                             \
    (SAC_CALCULATE_PAYLOAD_SIZE(MAIN_CHANNEL_FBK_2_SAMPLE_COUNT, MAIN_CHANNEL_CHANNEL_COUNT,        \
                                MAIN_CHANNEL_OTA_PACKED_BIT_DEPTH) +                                \
     MAIN_CHANNEL_FALLBACK_HEADER_SIZE)
#define MAIN_CHANNEL_FBK_3_PAYLOAD_SIZE                                                             \
    (SAC_CALCULATE_PAYLOAD_SIZE(MAIN_CHANNEL_FBK_3_SAMPLE_COUNT, MAIN_CHANNEL_CHANNEL_COUNT,        \
                                SAC_COMPRESSION_SAMPLE_RESOLUTION) +                                \
     MAIN_CHANNEL_FALLBACK_COMPRESSION_HEADER_SIZE)
#define MAIN_CHANNEL_FALLBACK_PAYLOAD_SIZE \
    {                                      \
        MAIN_CHANNEL_FBK_1_PAYLOAD_SIZE,   \
        MAIN_CHANNEL_FBK_2_PAYLOAD_SIZE,   \
        MAIN_CHANNEL_FBK_3_PAYLOAD_SIZE,   \
    }

/* Accumulator settings. Sizes the accumulator buffer, so it must be the largest ratio any mode
 * asks for -- which is the bottom rung's. */
#define MAIN_CHANNEL_MAX_ACC_MUL MAIN_CHANNEL_FBK_3_ACC_MUL
#define MAIN_CHANNEL_MAX_ACC_DIV MAIN_CHANNEL_FBK_3_ACC_DIV

#define MAIN_CHANNEL_ACC_MUL           \
    {                                  \
        1,                             \
        17,                            \
        17,                            \
        MAIN_CHANNEL_FBK_3_ACC_MUL,    \
    }
#define MAIN_CHANNEL_ACC_DIV           \
    {                                  \
        1,                             \
        10,                            \
        10,                            \
        MAIN_CHANNEL_FBK_3_ACC_DIV,    \
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
#define MAIN_CHANNEL_FALLBACK_LATENCY_USB_FIFO_SIZE                                                            \
    {                                                                                                      \
        (MAIN_CHANNEL_FBK_0_LATENCY_QUEUE_SIZE * ((MAIN_CHANNEL_USB_BIT_DEPTH + 7) / SAC_BYTE_SIZE_BITS)), \
        (MAIN_CHANNEL_FBK_1_LATENCY_QUEUE_SIZE * ((MAIN_CHANNEL_USB_BIT_DEPTH + 7) / SAC_BYTE_SIZE_BITS)), \
        (MAIN_CHANNEL_FBK_2_LATENCY_QUEUE_SIZE * ((MAIN_CHANNEL_USB_BIT_DEPTH + 7) / SAC_BYTE_SIZE_BITS)), \
        (MAIN_CHANNEL_FBK_3_LATENCY_QUEUE_SIZE * ((MAIN_CHANNEL_USB_BIT_DEPTH + 7) / SAC_BYTE_SIZE_BITS)), \
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
#define BACK_CHANNEL_FALLBACK_LATENCY_USB_FIFO_SIZE                                                        \
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
#define I2S_BIT_DEPTH MAIN_CHANNEL_AUDIO_IFACE_BIT_DEPTH
/* I2S DMA bit depth is defined by the capabilities of the DMA transfer.
 * The DMA can transport data on either a byte (8-bit), a half-word (16-bit) or a word (32-bit)
 * Packing Audio processing is required if a stream uses a bit depth smaller than the DMA bit depth.
 */
#define I2S_DMA_BIT_DEPTH (((I2S_BIT_DEPTH) <= 8) ? 8 : (((I2S_BIT_DEPTH) <= 16) ? 16 : (32)))
/* Latency induced by the codec's ADC and DAC. */
#define CODEC_LATENCY_MS 1

/* **** USB Settings. **** */
#define MAIN_CHANNEL_USB_BIT_DEPTH MAIN_CHANNEL_AUDIO_IFACE_BIT_DEPTH
/* Number of extra packets to buffer when receiving from USB FS. */
#define MAIN_CHANNEL_USB_FS_PRODUCER_BUFFERING (((MAIN_CHANNEL_SAMPLE_RATE_HZ / 1000) / MAIN_CHANNEL_SAMPLE_COUNT))
/* Number of extra packets to buffer when receiving from USB FS. */
#define BACK_CHANNEL_USB_FS_PRODUCER_BUFFERING (((BACK_CHANNEL_SAMPLE_RATE_HZ / 1000) / BACK_CHANNEL_SAMPLE_COUNT))

#endif /* SAC_CFG_H_ */
