/** @file  tusb_config.h
 *  @brief TinyUSB configuration file.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *             Copyright (C) 2025 Ha Thach (tinyusb.org).
 *  @license   Derivative: This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *             Original work: The MIT License (MIT)
 *
 *             Permission is hereby granted, free of charge, to any person obtaining a copy
 *             of this software and associated documentation files (the "Software"), to deal
 *             in the Software without restriction, including without limitation the rights
 *             to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *             copies of the Software, and to permit persons to whom the Software is
 *             furnished to do so, subject to the following conditions:
 *
 *             The above copyright notice and this permission notice shall be included in
 *             all copies or substantial portions of the Software.
 *
 *             THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *             IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *             FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *             AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *             LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *             OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *             THE SOFTWARE.
 *  @author    Derivative: SPARK FW Team.
 *             Original work: Ha Thach (tinyusb.org).
 */
#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

/* INCLUDES *******************************************************************/
#include <stdint.h>
#include "tusb_descriptors.h"

/* BOARD SPECIFIC CONFIGURATION. */

/* RHPort number used for device can be defined by hardware module, default to port 0. */
#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT 0
#endif

/* COMMON CONFIGURATION. */

/* Defined by hardware module. */
#ifndef CFG_TUSB_MCU
#error CFG_TUSB_MCU must be defined
#endif

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS OPT_OS_NONE
#endif

#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG 0
#endif

/* Enable Device stack. */
#define CFG_TUD_ENABLED 1

/* Default is max speed that hardware controller could support with on-chip PHY. */
#define CFG_TUD_MAX_SPEED BOARD_TUD_MAX_SPEED

/* USB DMA on some MCUs can only access a specific SRAM region with restriction on alignment.
 * Tinyusb use follows macros to declare transferring memory so that they can be put
 * into those specific section.
 * e.g
 * - CFG_TUSB_MEM SECTION : __section(".usb_ram")
 * - CFG_TUSB_MEM_ALIGN   : __aligned(4)
 */
#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN __aligned(4)
#endif

//--------------------------------------------------------------------
// DEVICE CONFIGURATION
//--------------------------------------------------------------------

/* Size in bytes of the endpoint 0 */
#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE 64
#endif

/* CLASS */
#define CFG_TUD_AUDIO  1
#define CFG_TUD_CDC    1
#define CFG_TUD_MSC    0
#define CFG_TUD_HID    1
#define CFG_TUD_MIDI   0
#define CFG_TUD_VENDOR 0

//--------------------------------------------------------------------
// CDC CLASS DRIVER CONFIGURATION
//--------------------------------------------------------------------

/* CDC FIFO size of TX and RX. */
#define CFG_TUD_CDC_RX_BUFSIZE (TUD_OPT_HIGH_SPEED ? 512 : 64)
#define CFG_TUD_CDC_TX_BUFSIZE 10000

/* CDC endpoint transfer buffer size, more is faster. */
#define CFG_TUD_CDC_EP_BUFSIZE (TUD_OPT_HIGH_SPEED ? 512 : 64)

/* HID endpoint buffer size. */
#define CFG_TUD_HID_EP_BUFSIZE (TUD_OPT_HIGH_SPEED ? 512 : 64)

//--------------------------------------------------------------------
// AUDIO CLASS DRIVER CONFIGURATION
//--------------------------------------------------------------------
#define CFG_TUD_AUDIO_FUNC_1_DESC_LEN tud_audio_get_func_1_desc_len()

/* How many formats are used, need to adjust USB descriptor if changed. */
#define CFG_TUD_AUDIO_FUNC_1_N_FORMATS 1

/* Both Audio endpoints are required with this configuration. */
#define CFG_TUD_AUDIO_ENABLE_EP_OUT       1
#define CFG_TUD_AUDIO_ENABLE_EP_IN        1

#define CFG_TUD_AUDIO_ENABLE_INTERRUPT_EP 1
#if CFG_TUD_AUDIO_ENABLE_EP_OUT
/* Audio format type I specifications. */
#ifndef CFG_TUD_AUDIO_FUNC_1_MAX_N_CHANNELS_OUT
#define CFG_TUD_AUDIO_FUNC_1_MAX_N_CHANNELS_OUT 2
#endif
#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX \
    ((1 << (UAC2_INTERVAL_OUT - 1)) * TUD_AUDIO_EP_SIZE(TUD_OPT_HIGH_SPEED, 96000, 3, CFG_TUD_AUDIO_FUNC_1_MAX_N_CHANNELS_OUT))
#endif
#ifndef CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ
#define CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ (2*(TUD_OPT_HIGH_SPEED ? 8 : 1) * CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX)
#endif

#if CFG_TUD_AUDIO_ENABLE_EP_IN
/* Audio format type I specifications. */
#ifndef CFG_TUD_AUDIO_FUNC_1_MAX_N_CHANNELS_IN
#define CFG_TUD_AUDIO_FUNC_1_MAX_N_CHANNELS_IN 2
#endif
#define CFG_TUD_AUDIO_FUNC_1_EP_IN_SZ_MAX \
    ((1 << (UAC2_INTERVAL_IN - 1)) * TUD_AUDIO_EP_SIZE(TUD_OPT_HIGH_SPEED, 96000, 3, CFG_TUD_AUDIO_FUNC_1_MAX_N_CHANNELS_IN))
#endif

#ifndef CFG_TUD_AUDIO_MAX_LATENCY_MS
#define CFG_TUD_AUDIO_MAX_LATENCY_MS 15
#endif

#ifndef CFG_TUD_AUDIO_FUNC_1_EP_IN_SW_BUF_SZ
#define CFG_TUD_AUDIO_FUNC_1_EP_IN_SW_BUF_SZ \
    (((TUD_OPT_HIGH_SPEED ? 8 : 1) * (CFG_TUD_AUDIO_FUNC_1_EP_IN_SZ_MAX)) * CFG_TUD_AUDIO_MAX_LATENCY_MS * 2)
#endif

/* Number of Standard AS Interface Descriptors (4.9.1) defined per audio function - this is required to be able to
 * remember the current alternate settings of these interfaces - We restrict us here to have a constant number for
 * all audio functions (which means this has to be the maximum number of AS interfaces an audio function has and a
 * second audio function with less AS interfaces just wastes a few bytes)
 */
#define CFG_TUD_AUDIO_FUNC_1_N_AS_INT 1

/* Size of control request buffers. */
#define CFG_TUD_AUDIO_FUNC_1_CTRL_BUF_SZ 64

/** @brief Configuration structure for an audio stream.
 */
typedef struct tud_audio_config {
    /*! Sampling rate of the audio stream. */
    uint32_t sampling_rate;
    /* Bytes per samples of the audio stream. */
    uint8_t bytes_per_sample;
    /* Resolution in bits of the audio stream. */
    uint8_t resolution;
    /* Number of channels in the audio stream. */
    uint8_t nb_ch;
} tud_audio_config_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Get a pointer to a string descriptor.
 *
 *  @param[in] index  Index of the string descriptor.
 *  @return Pointer to the string descriptor.
 */
char *tusb_get_string_descriptor(uint8_t index);

/** @brief Get the output endpoint volume in db.
 *
 *  @return Output endpoint volume in db.
 */
int8_t tud_audio_get_epout_volume_db(void);

/** @brief Get the input endpoint volume in db.
 *
 *  @return Input endpoint volume in db.
 */
int8_t tud_audio_get_epin_volume_db(void);

/** @brief Set EPOUT audio configuration (Host output).
 *
 *  @param[in] cfg  EPOUT audio configuration structure.
 */
void tud_audio_set_epout_cfg(tud_audio_config_t cfg);

/** @brief Set EPIN audio configuration (Host input).
 *
 *  @param[in] cfg  EPIN audio configuration structure.
 */
void tud_audio_set_epin_cfg(tud_audio_config_t cfg);

/** @brief Get the audio descriptor length.
 */
uint16_t tud_audio_get_func_1_desc_len(void);

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_CONFIG_H_ */
