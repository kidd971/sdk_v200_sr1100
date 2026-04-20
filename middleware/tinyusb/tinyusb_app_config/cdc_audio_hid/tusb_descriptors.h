/** @file  tusb_config.h
 *  @brief TinyUSB descriptor.
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
#ifndef _USB_DESCRIPTORS_H_
#define _USB_DESCRIPTORS_H_

// Unit numbers are arbitrary selected
#define UAC2_ENTITY_EPOUT_CLOCK 0x04
#define UAC2_ENTITY_EPIN_CLOCK  0x05
// EPOUT path
#define UAC2_ENTITY_EPOUT_INPUT_TERMINAL  0x01
#define UAC2_ENTITY_EPOUT_FEATURE_UNIT    0x02
#define UAC2_ENTITY_EPOUT_OUTPUT_TERMINAL 0x03
// EPIN path
#define UAC2_ENTITY_EPIN_INPUT_TERMINAL  0x11
#define UAC2_ENTITY_EPIN_FEATURE_UNIT    0x12
#define UAC2_ENTITY_EPIN_OUTPUT_TERMINAL 0x13

#define UAC2_INTERVAL_IN                 1
#define UAC2_INTERVAL_OUT                1

#define USB_HID_MEDIA_REPORT_ID          0x01
#define USB_HID_CONTROL_DATA_REPORT_ID   0x02

/* String Descriptors */
enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
    STRID_CDC_IFACE,
    STRID_AUDIO_EPOUT_IFACE,
    STRID_AUDIO_EPIN_IFACE,
};

enum audio_epout_epin_iad {
    AUDIO_EPOUT_EPIN_ITF_NUM_AUDIO_CONTROL,
    AUDIO_EPOUT_EPIN_ITF_NUM_AUDIO_STREAMING_EPOUT,
    AUDIO_EPOUT_EPIN_ITF_NUM_AUDIO_STREAMING_EPIN,
    AUDIO_EPOUT_EPIN_ITF_NUM_TOTAL
};

enum audio_epout_iad {
    AUDIO_EPOUT_ITF_NUM_AUDIO_CONTROL,
    AUDIO_EPOUT_ITF_NUM_AUDIO_STREAMING_EPOUT,
    AUDIO_EPOUT_ITF_NUM_TOTAL
};

enum audio_epin_iad {
    AUDIO_EPIN_ITF_NUM_AUDIO_CONTROL,
    AUDIO_EPIN_ITF_NUM_AUDIO_STREAMING_EPIN,
    AUDIO_EPIN_ITF_NUM_TOTAL
};

enum usb_epout_epin_itf {
    ITF_EPOUT_EPIN_NUM_CDC_0 = 0,
    ITF_EPOUT_EPIN_NUM_CDC_0_DATA,
    ITF_EPOUT_EPIN_NUM_AUDIO_CONTROL,
    ITF_EPOUT_EPIN_NUM_AUDIO_STREAMING_EPOUT,
    ITF_EPOUT_EPIN_NUM_AUDIO_STREAMING_EPIN,
    ITF_EPOUT_EPIN_HID,
    ITF_EPOUT_EPIN_NUM_TOTAL
};

enum usb_epout_itf {
    ITF_EPOUT_NUM_CDC_0 = 0,
    ITF_EPOUT_NUM_CDC_0_DATA,
    ITF_EPOUT_NUM_AUDIO_CONTROL,
    ITF_EPOUT_NUM_AUDIO_STREAMING_EPOUT,
    ITF_EPOUT_HID,
    ITF_EPOUT_NUM_TOTAL
};

enum usb_epin_itf {
    ITF_EPIN_NUM_CDC_0 = 0,
    ITF_EPIN_NUM_CDC_0_DATA,
    ITF_EPIN_NUM_AUDIO_CONTROL,
    ITF_EPIN_NUM_AUDIO_STREAMING_EPIN,
    ITF_EPIN_HID,
    ITF_EPIN_NUM_TOTAL
};

// clang-format off
/* 51 bytes. */
#define AC_ITF_DESC_MONO_LEN (TUD_AUDIO20_DESC_CLK_SRC_LEN + \
                              TUD_AUDIO20_DESC_INPUT_TERM_LEN + \
                              TUD_AUDIO20_DESC_FEATURE_UNIT_LEN(1) + \
                              TUD_AUDIO20_DESC_OUTPUT_TERM_LEN)

/* 55 bytes. */
#define AC_ITF_DESC_STEREO_LEN (TUD_AUDIO20_DESC_CLK_SRC_LEN + \
                                TUD_AUDIO20_DESC_INPUT_TERM_LEN + \
                                TUD_AUDIO20_DESC_FEATURE_UNIT_LEN(2) + \
                                TUD_AUDIO20_DESC_OUTPUT_TERM_LEN)

/* 9+9+16+6+7+8=55 */
#define AUDIO_DESC_ITF_LEN (TUD_AUDIO20_DESC_STD_AS_LEN + /* Interface 1, Alternate 0 */ \
                            TUD_AUDIO20_DESC_STD_AS_LEN + /* Interface 1, Alternate 1 */ \
                            TUD_AUDIO20_DESC_CS_AS_INT_LEN + \
                            TUD_AUDIO20_DESC_TYPE_I_FORMAT_LEN + \
                            TUD_AUDIO20_DESC_STD_AS_ISO_EP_LEN + \
                            TUD_AUDIO20_DESC_CS_AS_ISO_EP_LEN)

/* Mono EPOUT, mono EPIN descriptor length. 8+9+9+51+51+7+55+55 = 245 */
#define TUD_AUDIO_MONO_EPOUT_MONO_EPIN_DESC_LEN (TUD_AUDIO20_DESC_IAD_LEN + \
                                                 TUD_AUDIO20_DESC_STD_AC_LEN + \
                                                 TUD_AUDIO20_DESC_CS_AC_LEN + \
                                                 AC_ITF_DESC_MONO_LEN + \
                                                 AC_ITF_DESC_MONO_LEN + \
                                                 TUD_AUDIO20_DESC_STD_AC_INT_EP_LEN + \
                                                 AUDIO_DESC_ITF_LEN + \
                                                 AUDIO_DESC_ITF_LEN)

#define TUD_AUDIO_MONO_EPOUT_MONO_EPIN_DESCRIPTOR(_stridx, _epout, _epin, _epint) \
    /* Standard Interface Association Descriptor (IAD) */\
    TUD_AUDIO20_DESC_IAD(/*_firstitf*/ ITF_EPOUT_EPIN_NUM_AUDIO_CONTROL, /*_nitfs*/ AUDIO_EPOUT_EPIN_ITF_NUM_TOTAL, /*_stridx*/ 0x00),\
    /* Standard AC Interface Descriptor(4.7.1) */\
    TUD_AUDIO20_DESC_STD_AC(/*_itfnum*/ ITF_EPOUT_EPIN_NUM_AUDIO_CONTROL, /*_nEPs*/ 0x01, /*_stridx*/ _stridx),\
    /* Class-Specific AC Interface Header Descriptor(4.7.2) */\
    TUD_AUDIO20_DESC_CS_AC(/*_bcdADC*/ 0x0200, /*_category*/ AUDIO20_FUNC_HEADSET, /*_totallen*/ AC_ITF_DESC_MONO_LEN + AC_ITF_DESC_MONO_LEN, /*_ctrl*/ AUDIO20_CS_AS_INTERFACE_CTRL_LATENCY_POS),\
    /* Clock Source Descriptor(4.7.2.1) */\
    TUD_AUDIO20_DESC_CLK_SRC(/*_clkid*/ UAC2_ENTITY_EPOUT_CLOCK, /*_attr*/ 3, /*_ctrl*/ 7, /*_assocTerm*/ 0x00,  /*_stridx*/ 0x00),\
    /* Input Terminal Descriptor(4.7.2.4) */\
    TUD_AUDIO20_DESC_INPUT_TERM(/*_termid*/ UAC2_ENTITY_EPOUT_INPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_USB_STREAMING, /*_assocTerm*/ 0x00, /*_clkid*/ UAC2_ENTITY_EPOUT_CLOCK, /*_nchannelslogical*/ 0x01, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_idxchannelnames*/ 0x00, /*_ctrl*/ 0 * (AUDIO20_CTRL_R << AUDIO20_IN_TERM_CTRL_CONNECTOR_POS), /*_stridx*/ 0x00),\
    /* Feature Unit Descriptor(4.7.2.8) */\
    TUD_AUDIO20_DESC_FEATURE_UNIT(/*_unitid*/ UAC2_ENTITY_EPOUT_FEATURE_UNIT, /*_srcid*/ UAC2_ENTITY_EPOUT_INPUT_TERMINAL, /*_stridx*/ 0x00, /*_ctrlch0master*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS), /*_ctrlch1*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS)),\
    /* Output Terminal Descriptor(4.7.2.5) */\
    TUD_AUDIO20_DESC_OUTPUT_TERM(/*_termid*/ UAC2_ENTITY_EPOUT_OUTPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_OUT_HEADPHONES, /*_assocTerm*/ 0x00, /*_srcid*/ UAC2_ENTITY_EPOUT_FEATURE_UNIT, /*_clkid*/ UAC2_ENTITY_EPOUT_CLOCK, /*_ctrl*/ 0x0000, /*_stridx*/ 0x00),\
    /* Clock Source Descriptor(4.7.2.1) */\
    TUD_AUDIO20_DESC_CLK_SRC(/*_clkid*/ UAC2_ENTITY_EPIN_CLOCK, /*_attr*/ 3, /*_ctrl*/ 7, /*_assocTerm*/ 0x00,  /*_stridx*/ 0x00),\
    /* Input Terminal Descriptor(4.7.2.4) */\
    TUD_AUDIO20_DESC_INPUT_TERM(/*_termid*/ UAC2_ENTITY_EPIN_INPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_IN_GENERIC_MIC, /*_assocTerm*/ 0x00, /*_clkid*/ UAC2_ENTITY_EPIN_CLOCK, /*_nchannelslogical*/ 0x01, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_idxchannelnames*/ 0x00, /*_ctrl*/ 0 * (AUDIO20_CTRL_R << AUDIO20_IN_TERM_CTRL_CONNECTOR_POS), /*_stridx*/ 0x00),\
    /* Feature Unit Descriptor(4.7.2.8) */\
    TUD_AUDIO20_DESC_FEATURE_UNIT(/*_unitid*/ UAC2_ENTITY_EPIN_FEATURE_UNIT, /*_srcid*/ UAC2_ENTITY_EPIN_INPUT_TERMINAL, /*_stridx*/ 0x00, /*_ctrlch0master*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS), /*_ctrlch1*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS)),\
    /* Output Terminal Descriptor(4.7.2.5) */\
    TUD_AUDIO20_DESC_OUTPUT_TERM(/*_termid*/ UAC2_ENTITY_EPIN_OUTPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_USB_STREAMING, /*_assocTerm*/ 0x00, /*_srcid*/ UAC2_ENTITY_EPIN_FEATURE_UNIT, /*_clkid*/ UAC2_ENTITY_EPIN_CLOCK, /*_ctrl*/ 0x0000, /*_stridx*/ 0x00),\
    /* Standard AC Interrupt Endpoint Descriptor(4.8.2.1) */\
    TUD_AUDIO20_DESC_STD_AC_INT_EP(/*_ep*/ _epint, /*_interval: 2^(7-1) = 64 => 64*125 us = 8000 us*/ (TUD_OPT_HIGH_SPEED ? 0x7 : 0x1)), \
    /* Standard AS Interface Descriptor(4.9.1) */\
    /* Interface 1, Alternate 0 - default alternate setting with 0 bandwidth */\
    TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)(ITF_EPOUT_EPIN_NUM_AUDIO_STREAMING_EPOUT), /*_altset*/ 0x00, /*_nEPs*/ 0x00, /*_stridx*/ STRID_AUDIO_EPOUT_IFACE),\
    /* Standard AS Interface Descriptor(4.9.1) */\
    /* Interface 1, Alternate 1 - alternate interface for data streaming */\
    TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)(ITF_EPOUT_EPIN_NUM_AUDIO_STREAMING_EPOUT), /*_altset*/ 0x01, /*_nEPs*/ 0x01, /*_stridx*/ STRID_AUDIO_EPOUT_IFACE),\
    /* Class-Specific AS Interface Descriptor(4.9.2) */\
    TUD_AUDIO20_DESC_CS_AS_INT(/*_termid*/ UAC2_ENTITY_EPOUT_INPUT_TERMINAL, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_formattype*/ AUDIO20_FORMAT_TYPE_I, /*_formats*/ AUDIO20_DATA_FORMAT_TYPE_I_PCM, /*_nchannelsphysical*/ 0x01, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_stridx*/ 0x00),\
    /* Type I Format Type Descriptor(2.3.1.6 - Audio Formats) */\
    TUD_AUDIO20_DESC_TYPE_I_FORMAT(0, 0),\
    /* Standard AS Isochronous Audio Data Endpoint Descriptor(4.10.1.1) */\
    TUD_AUDIO20_DESC_STD_AS_ISO_EP(/*_ep*/ _epout, /*_attr*/ (uint8_t) ((uint8_t)TUSB_XFER_ISOCHRONOUS | (uint8_t)TUSB_ISO_EP_ATT_ADAPTIVE | (uint8_t)TUSB_ISO_EP_ATT_DATA), /*_maxEPsize*/ 0, /*_interval*/ UAC2_INTERVAL_OUT),\
    /* Class-Specific AS Isochronous Audio Data Endpoint Descriptor(4.10.1.2) */\
    TUD_AUDIO20_DESC_CS_AS_ISO_EP(/*_attr*/ AUDIO20_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_lockdelayunit*/ AUDIO20_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_MILLISEC, /*_lockdelay*/ 0x0001),\
    /* Standard AS Interface Descriptor(4.9.1) */\
    /* Interface 2, Alternate 0 - default alternate setting with 0 bandwidth */\
    TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)(ITF_EPOUT_EPIN_NUM_AUDIO_STREAMING_EPIN), /*_altset*/ 0x00, /*_nEPs*/ 0x00, /*_stridx*/ STRID_AUDIO_EPIN_IFACE),\
    /* Standard AS Interface Descriptor(4.9.1) */\
    /* Interface 2, Alternate 1 - alternate interface for data streaming */\
    TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)(ITF_EPOUT_EPIN_NUM_AUDIO_STREAMING_EPIN), /*_altset*/ 0x01, /*_nEPs*/ 0x01, /*_stridx*/ STRID_AUDIO_EPIN_IFACE),\
    /* Class-Specific AS Interface Descriptor(4.9.2) */\
    TUD_AUDIO20_DESC_CS_AS_INT(/*_termid*/ UAC2_ENTITY_EPIN_OUTPUT_TERMINAL, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_formattype*/ AUDIO20_FORMAT_TYPE_I, /*_formats*/ AUDIO20_DATA_FORMAT_TYPE_I_PCM, /*_nchannelsphysical*/ 0x01, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_stridx*/ 0x00),\
    /* Type I Format Type Descriptor(2.3.1.6 - Audio Formats) */\
    TUD_AUDIO20_DESC_TYPE_I_FORMAT(0, 0),\
    /* Standard AS Isochronous Audio Data Endpoint Descriptor(4.10.1.1) */\
    TUD_AUDIO20_DESC_STD_AS_ISO_EP(/*_ep*/ _epin, /*_attr*/ (uint8_t) ((uint8_t)TUSB_XFER_ISOCHRONOUS | (uint8_t)TUSB_ISO_EP_ATT_ASYNCHRONOUS | (uint8_t)TUSB_ISO_EP_ATT_DATA), /*_maxEPsize*/ 0, /*_interval*/ UAC2_INTERVAL_IN),\
    /* Class-Specific AS Isochronous Audio Data Endpoint Descriptor(4.10.1.2) */\
    TUD_AUDIO20_DESC_CS_AS_ISO_EP(/*_attr*/ AUDIO20_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_lockdelayunit*/ AUDIO20_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_UNDEFINED, /*_lockdelay*/ 0x0000)

/* Stereo EPOUT, mono EPIN descriptor length. 8+9+9+55+51+7+55+55 = 249 */
#define TUD_AUDIO_MONO_EPOUT_STEREO_EPIN_DESC_LEN (TUD_AUDIO20_DESC_IAD_LEN + \
                                                   TUD_AUDIO20_DESC_STD_AC_LEN + \
                                                   TUD_AUDIO20_DESC_CS_AC_LEN + \
                                                   AC_ITF_DESC_MONO_LEN + \
                                                   AC_ITF_DESC_STEREO_LEN + \
                                                   TUD_AUDIO20_DESC_STD_AC_INT_EP_LEN + \
                                                   AUDIO_DESC_ITF_LEN + \
                                                   AUDIO_DESC_ITF_LEN)

#define TUD_AUDIO_MONO_EPOUT_STEREO_EPIN_DESCRIPTOR(_stridx, _epout, _epin, _epint) \
    /* Standard Interface Association Descriptor (IAD) */\
    TUD_AUDIO20_DESC_IAD(/*_firstitf*/ ITF_EPOUT_EPIN_NUM_AUDIO_CONTROL, /*_nitfs*/ AUDIO_EPOUT_EPIN_ITF_NUM_TOTAL, /*_stridx*/ 0x00),\
    /* Standard AC Interface Descriptor(4.7.1) */\
    TUD_AUDIO20_DESC_STD_AC(/*_itfnum*/ ITF_EPOUT_EPIN_NUM_AUDIO_CONTROL, /*_nEPs*/ 0x01, /*_stridx*/ _stridx),\
    /* Class-Specific AC Interface Header Descriptor(4.7.2) */\
    TUD_AUDIO20_DESC_CS_AC(/*_bcdADC*/ 0x0200, /*_category*/ AUDIO20_FUNC_HEADSET, /*_totallen*/ AC_ITF_DESC_STEREO_LEN + AC_ITF_DESC_MONO_LEN, /*_ctrl*/ AUDIO20_CS_AS_INTERFACE_CTRL_LATENCY_POS),\
    /* Clock Source Descriptor(4.7.2.1) */\
    TUD_AUDIO20_DESC_CLK_SRC(/*_clkid*/ UAC2_ENTITY_EPOUT_CLOCK, /*_attr*/ 3, /*_ctrl*/ 7, /*_assocTerm*/ 0x00,  /*_stridx*/ 0x00),\
    /* Input Terminal Descriptor(4.7.2.4) */\
    TUD_AUDIO20_DESC_INPUT_TERM(/*_termid*/ UAC2_ENTITY_EPOUT_INPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_USB_STREAMING, /*_assocTerm*/ 0x00, /*_clkid*/ UAC2_ENTITY_EPOUT_CLOCK, /*_nchannelslogical*/ 0x01, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_idxchannelnames*/ 0x00, /*_ctrl*/ 0 * (AUDIO20_CTRL_R << AUDIO20_IN_TERM_CTRL_CONNECTOR_POS), /*_stridx*/ 0x00),\
    /* Feature Unit Descriptor(4.7.2.8) */\
    TUD_AUDIO20_DESC_FEATURE_UNIT(/*_unitid*/ UAC2_ENTITY_EPOUT_FEATURE_UNIT, /*_srcid*/ UAC2_ENTITY_EPOUT_INPUT_TERMINAL, /*_stridx*/ 0x00, /*_ctrlch0master*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS), /*_ctrlch1*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS)),\
    /* Output Terminal Descriptor(4.7.2.5) */\
    TUD_AUDIO20_DESC_OUTPUT_TERM(/*_termid*/ UAC2_ENTITY_EPOUT_OUTPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_OUT_HEADPHONES, /*_assocTerm*/ 0x00, /*_srcid*/ UAC2_ENTITY_EPOUT_FEATURE_UNIT, /*_clkid*/ UAC2_ENTITY_EPOUT_CLOCK, /*_ctrl*/ 0x0000, /*_stridx*/ 0x00),\
    /* Clock Source Descriptor(4.7.2.1) */\
    TUD_AUDIO20_DESC_CLK_SRC(/*_clkid*/ UAC2_ENTITY_EPIN_CLOCK, /*_attr*/ 3, /*_ctrl*/ 7, /*_assocTerm*/ 0x00,  /*_stridx*/ 0x00),\
    /* Input Terminal Descriptor(4.7.2.4) */\
    TUD_AUDIO20_DESC_INPUT_TERM(/*_termid*/ UAC2_ENTITY_EPIN_INPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_IN_GENERIC_MIC, /*_assocTerm*/ 0x00, /*_clkid*/ UAC2_ENTITY_EPIN_CLOCK, /*_nchannelslogical*/ 0x02, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_idxchannelnames*/ 0x00, /*_ctrl*/ 0 * (AUDIO20_CTRL_R << AUDIO20_IN_TERM_CTRL_CONNECTOR_POS), /*_stridx*/ 0x00),\
    /* Feature Unit Descriptor(4.7.2.8) */\
    TUD_AUDIO20_DESC_FEATURE_UNIT(/*_unitid*/ UAC2_ENTITY_EPIN_FEATURE_UNIT, /*_srcid*/ UAC2_ENTITY_EPIN_INPUT_TERMINAL, /*_stridx*/ 0x00, /*_ctrlch0master*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS), /*_ctrlch1*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS), /*_ctrlch2*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS)),\
    /* Output Terminal Descriptor(4.7.2.5) */\
    TUD_AUDIO20_DESC_OUTPUT_TERM(/*_termid*/ UAC2_ENTITY_EPIN_OUTPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_USB_STREAMING, /*_assocTerm*/ 0x00, /*_srcid*/ UAC2_ENTITY_EPIN_FEATURE_UNIT, /*_clkid*/ UAC2_ENTITY_EPIN_CLOCK, /*_ctrl*/ 0x0000, /*_stridx*/ 0x00),\
    /* Standard AC Interrupt Endpoint Descriptor(4.8.2.1) */\
    TUD_AUDIO20_DESC_STD_AC_INT_EP(/*_ep*/ _epint, /*_interval: 2^(7-1) = 64 => 64*125 us = 8000 us*/ (TUD_OPT_HIGH_SPEED ? 0x7 : 0x1)), \
    /* Standard AS Interface Descriptor(4.9.1) */\
    /* Interface 1, Alternate 0 - default alternate setting with 0 bandwidth */\
    TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)(ITF_EPOUT_EPIN_NUM_AUDIO_STREAMING_EPOUT), /*_altset*/ 0x00, /*_nEPs*/ 0x00, /*_stridx*/ STRID_AUDIO_EPOUT_IFACE),\
    /* Standard AS Interface Descriptor(4.9.1) */\
    /* Interface 1, Alternate 1 - alternate interface for data streaming */\
    TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)(ITF_EPOUT_EPIN_NUM_AUDIO_STREAMING_EPOUT), /*_altset*/ 0x01, /*_nEPs*/ 0x01, /*_stridx*/ STRID_AUDIO_EPOUT_IFACE),\
    /* Class-Specific AS Interface Descriptor(4.9.2) */\
    TUD_AUDIO20_DESC_CS_AS_INT(/*_termid*/ UAC2_ENTITY_EPOUT_INPUT_TERMINAL, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_formattype*/ AUDIO20_FORMAT_TYPE_I, /*_formats*/ AUDIO20_DATA_FORMAT_TYPE_I_PCM, /*_nchannelsphysical*/ 0x01, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_stridx*/ 0x00),\
    /* Type I Format Type Descriptor(2.3.1.6 - Audio Formats) */\
    TUD_AUDIO20_DESC_TYPE_I_FORMAT(0, 0),\
    /* Standard AS Isochronous Audio Data Endpoint Descriptor(4.10.1.1) */\
    TUD_AUDIO20_DESC_STD_AS_ISO_EP(/*_ep*/ _epout, /*_attr*/ (uint8_t) ((uint8_t)TUSB_XFER_ISOCHRONOUS | (uint8_t)TUSB_ISO_EP_ATT_ADAPTIVE | (uint8_t)TUSB_ISO_EP_ATT_DATA), /*_maxEPsize*/ 0, /*_interval*/ UAC2_INTERVAL_OUT),\
    /* Class-Specific AS Isochronous Audio Data Endpoint Descriptor(4.10.1.2) */\
    TUD_AUDIO20_DESC_CS_AS_ISO_EP(/*_attr*/ AUDIO20_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_lockdelayunit*/ AUDIO20_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_MILLISEC, /*_lockdelay*/ 0x0001),\
    /* Standard AS Interface Descriptor(4.9.1) */\
    /* Interface 2, Alternate 0 - default alternate setting with 0 bandwidth */\
    TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)(ITF_EPOUT_EPIN_NUM_AUDIO_STREAMING_EPIN), /*_altset*/ 0x00, /*_nEPs*/ 0x00, /*_stridx*/ STRID_AUDIO_EPIN_IFACE),\
    /* Standard AS Interface Descriptor(4.9.1) */\
    /* Interface 2, Alternate 1 - alternate interface for data streaming */\
    TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)(ITF_EPOUT_EPIN_NUM_AUDIO_STREAMING_EPIN), /*_altset*/ 0x01, /*_nEPs*/ 0x01, /*_stridx*/ STRID_AUDIO_EPIN_IFACE),\
    /* Class-Specific AS Interface Descriptor(4.9.2) */\
    TUD_AUDIO20_DESC_CS_AS_INT(/*_termid*/ UAC2_ENTITY_EPIN_OUTPUT_TERMINAL, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_formattype*/ AUDIO20_FORMAT_TYPE_I, /*_formats*/ AUDIO20_DATA_FORMAT_TYPE_I_PCM, /*_nchannelsphysical*/ 0x02, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_stridx*/ 0x00),\
    /* Type I Format Type Descriptor(2.3.1.6 - Audio Formats) */\
    TUD_AUDIO20_DESC_TYPE_I_FORMAT(0, 0),\
    /* Standard AS Isochronous Audio Data Endpoint Descriptor(4.10.1.1) */\
    TUD_AUDIO20_DESC_STD_AS_ISO_EP(/*_ep*/ _epin, /*_attr*/ (uint8_t) ((uint8_t)TUSB_XFER_ISOCHRONOUS | (uint8_t)TUSB_ISO_EP_ATT_ASYNCHRONOUS | (uint8_t)TUSB_ISO_EP_ATT_DATA), /*_maxEPsize*/ 0, /*_interval*/ UAC2_INTERVAL_IN),\
    /* Class-Specific AS Isochronous Audio Data Endpoint Descriptor(4.10.1.2) */\
    TUD_AUDIO20_DESC_CS_AS_ISO_EP(/*_attr*/ AUDIO20_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_lockdelayunit*/ AUDIO20_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_UNDEFINED, /*_lockdelay*/ 0x0000)

/* Stereo EPOUT, mono EPIN descriptor length. 8+9+9+55+51+7+55+55 = 249 */
#define TUD_AUDIO_STEREO_EPOUT_MONO_EPIN_DESC_LEN (TUD_AUDIO20_DESC_IAD_LEN + \
                                                   TUD_AUDIO20_DESC_STD_AC_LEN + \
                                                   TUD_AUDIO20_DESC_CS_AC_LEN + \
                                                   AC_ITF_DESC_STEREO_LEN + \
                                                   AC_ITF_DESC_MONO_LEN + \
                                                   TUD_AUDIO20_DESC_STD_AC_INT_EP_LEN + \
                                                   AUDIO_DESC_ITF_LEN + \
                                                   AUDIO_DESC_ITF_LEN)

#define TUD_AUDIO_STEREO_EPOUT_MONO_EPIN_DESCRIPTOR(_stridx, _epout, _epin, _epint) \
    /* Standard Interface Association Descriptor (IAD) */\
    TUD_AUDIO20_DESC_IAD(/*_firstitf*/ ITF_EPOUT_EPIN_NUM_AUDIO_CONTROL, /*_nitfs*/ AUDIO_EPOUT_EPIN_ITF_NUM_TOTAL, /*_stridx*/ 0x00),\
    /* Standard AC Interface Descriptor(4.7.1) */\
    TUD_AUDIO20_DESC_STD_AC(/*_itfnum*/ ITF_EPOUT_EPIN_NUM_AUDIO_CONTROL, /*_nEPs*/ 0x01, /*_stridx*/ _stridx),\
    /* Class-Specific AC Interface Header Descriptor(4.7.2) */\
    TUD_AUDIO20_DESC_CS_AC(/*_bcdADC*/ 0x0200, /*_category*/ AUDIO20_FUNC_HEADSET, /*_totallen*/ AC_ITF_DESC_STEREO_LEN + AC_ITF_DESC_MONO_LEN, /*_ctrl*/ AUDIO20_CS_AS_INTERFACE_CTRL_LATENCY_POS),\
    /* Clock Source Descriptor(4.7.2.1) */\
    TUD_AUDIO20_DESC_CLK_SRC(/*_clkid*/ UAC2_ENTITY_EPOUT_CLOCK, /*_attr*/ 3, /*_ctrl*/ 7, /*_assocTerm*/ 0x00,  /*_stridx*/ 0x00),\
    /* Input Terminal Descriptor(4.7.2.4) */\
    TUD_AUDIO20_DESC_INPUT_TERM(/*_termid*/ UAC2_ENTITY_EPOUT_INPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_USB_STREAMING, /*_assocTerm*/ 0x00, /*_clkid*/ UAC2_ENTITY_EPOUT_CLOCK, /*_nchannelslogical*/ 0x02, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_idxchannelnames*/ 0x00, /*_ctrl*/ 0 * (AUDIO20_CTRL_R << AUDIO20_IN_TERM_CTRL_CONNECTOR_POS), /*_stridx*/ 0x00),\
    /* Feature Unit Descriptor(4.7.2.8) */\
    TUD_AUDIO20_DESC_FEATURE_UNIT(/*_unitid*/ UAC2_ENTITY_EPOUT_FEATURE_UNIT, /*_srcid*/ UAC2_ENTITY_EPOUT_INPUT_TERMINAL, /*_stridx*/ 0x00, /*_ctrlch0master*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS), /*_ctrlch1*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS), /*_ctrlch2*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS)),\
    /* Output Terminal Descriptor(4.7.2.5) */\
    TUD_AUDIO20_DESC_OUTPUT_TERM(/*_termid*/ UAC2_ENTITY_EPOUT_OUTPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_OUT_HEADPHONES, /*_assocTerm*/ 0x00, /*_srcid*/ UAC2_ENTITY_EPOUT_FEATURE_UNIT, /*_clkid*/ UAC2_ENTITY_EPOUT_CLOCK, /*_ctrl*/ 0x0000, /*_stridx*/ 0x00),\
    /* Clock Source Descriptor(4.7.2.1) */\
    TUD_AUDIO20_DESC_CLK_SRC(/*_clkid*/ UAC2_ENTITY_EPIN_CLOCK, /*_attr*/ 3, /*_ctrl*/ 7, /*_assocTerm*/ 0x00,  /*_stridx*/ 0x00),\
    /* Input Terminal Descriptor(4.7.2.4) */\
    TUD_AUDIO20_DESC_INPUT_TERM(/*_termid*/ UAC2_ENTITY_EPIN_INPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_IN_GENERIC_MIC, /*_assocTerm*/ 0x00, /*_clkid*/ UAC2_ENTITY_EPIN_CLOCK, /*_nchannelslogical*/ 0x01, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_idxchannelnames*/ 0x00, /*_ctrl*/ 0 * (AUDIO20_CTRL_R << AUDIO20_IN_TERM_CTRL_CONNECTOR_POS), /*_stridx*/ 0x00),\
    /* Feature Unit Descriptor(4.7.2.8) */\
    TUD_AUDIO20_DESC_FEATURE_UNIT(/*_unitid*/ UAC2_ENTITY_EPIN_FEATURE_UNIT, /*_srcid*/ UAC2_ENTITY_EPIN_INPUT_TERMINAL, /*_stridx*/ 0x00, /*_ctrlch0master*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS), /*_ctrlch1*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS)),\
    /* Output Terminal Descriptor(4.7.2.5) */\
    TUD_AUDIO20_DESC_OUTPUT_TERM(/*_termid*/ UAC2_ENTITY_EPIN_OUTPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_USB_STREAMING, /*_assocTerm*/ 0x00, /*_srcid*/ UAC2_ENTITY_EPIN_FEATURE_UNIT, /*_clkid*/ UAC2_ENTITY_EPIN_CLOCK, /*_ctrl*/ 0x0000, /*_stridx*/ 0x00),\
    /* Standard AC Interrupt Endpoint Descriptor(4.8.2.1) */\
    TUD_AUDIO20_DESC_STD_AC_INT_EP(/*_ep*/ _epint, /*_interval: 2^(7-1) = 64 => 64*125 us = 8000 us*/ (TUD_OPT_HIGH_SPEED ? 0x7 : 0x1)), \
    /* Standard AS Interface Descriptor(4.9.1) */\
    /* Interface 1, Alternate 0 - default alternate setting with 0 bandwidth */\
    TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)(ITF_EPOUT_EPIN_NUM_AUDIO_STREAMING_EPOUT), /*_altset*/ 0x00, /*_nEPs*/ 0x00, /*_stridx*/ STRID_AUDIO_EPOUT_IFACE),\
    /* Standard AS Interface Descriptor(4.9.1) */\
    /* Interface 1, Alternate 1 - alternate interface for data streaming */\
    TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)(ITF_EPOUT_EPIN_NUM_AUDIO_STREAMING_EPOUT), /*_altset*/ 0x01, /*_nEPs*/ 0x01, /*_stridx*/ STRID_AUDIO_EPOUT_IFACE),\
    /* Class-Specific AS Interface Descriptor(4.9.2) */\
    TUD_AUDIO20_DESC_CS_AS_INT(/*_termid*/ UAC2_ENTITY_EPOUT_INPUT_TERMINAL, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_formattype*/ AUDIO20_FORMAT_TYPE_I, /*_formats*/ AUDIO20_DATA_FORMAT_TYPE_I_PCM, /*_nchannelsphysical*/ 0x02, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_stridx*/ 0x00),\
    /* Type I Format Type Descriptor(2.3.1.6 - Audio Formats) */\
    TUD_AUDIO20_DESC_TYPE_I_FORMAT(0, 0),\
    /* Standard AS Isochronous Audio Data Endpoint Descriptor(4.10.1.1) */\
    TUD_AUDIO20_DESC_STD_AS_ISO_EP(/*_ep*/ _epout, /*_attr*/ (uint8_t) ((uint8_t)TUSB_XFER_ISOCHRONOUS | (uint8_t)TUSB_ISO_EP_ATT_ADAPTIVE | (uint8_t)TUSB_ISO_EP_ATT_DATA), /*_maxEPsize*/ 0, /*_interval*/ UAC2_INTERVAL_OUT),\
    /* Class-Specific AS Isochronous Audio Data Endpoint Descriptor(4.10.1.2) */\
    TUD_AUDIO20_DESC_CS_AS_ISO_EP(/*_attr*/ AUDIO20_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_lockdelayunit*/ AUDIO20_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_MILLISEC, /*_lockdelay*/ 0x0001),\
    /* Standard AS Interface Descriptor(4.9.1) */\
    /* Interface 2, Alternate 0 - default alternate setting with 0 bandwidth */\
    TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)(ITF_EPOUT_EPIN_NUM_AUDIO_STREAMING_EPIN), /*_altset*/ 0x00, /*_nEPs*/ 0x00, /*_stridx*/ STRID_AUDIO_EPIN_IFACE),\
    /* Standard AS Interface Descriptor(4.9.1) */\
    /* Interface 2, Alternate 1 - alternate interface for data streaming */\
    TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)(ITF_EPOUT_EPIN_NUM_AUDIO_STREAMING_EPIN), /*_altset*/ 0x01, /*_nEPs*/ 0x01, /*_stridx*/ STRID_AUDIO_EPIN_IFACE),\
    /* Class-Specific AS Interface Descriptor(4.9.2) */\
    TUD_AUDIO20_DESC_CS_AS_INT(/*_termid*/ UAC2_ENTITY_EPIN_OUTPUT_TERMINAL, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_formattype*/ AUDIO20_FORMAT_TYPE_I, /*_formats*/ AUDIO20_DATA_FORMAT_TYPE_I_PCM, /*_nchannelsphysical*/ 0x01, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_stridx*/ 0x00),\
    /* Type I Format Type Descriptor(2.3.1.6 - Audio Formats) */\
    TUD_AUDIO20_DESC_TYPE_I_FORMAT(0, 0),\
    /* Standard AS Isochronous Audio Data Endpoint Descriptor(4.10.1.1) */\
    TUD_AUDIO20_DESC_STD_AS_ISO_EP(/*_ep*/ _epin, /*_attr*/ (uint8_t) ((uint8_t)TUSB_XFER_ISOCHRONOUS | (uint8_t)TUSB_ISO_EP_ATT_ASYNCHRONOUS | (uint8_t)TUSB_ISO_EP_ATT_DATA), /*_maxEPsize*/ 0, /*_interval*/ UAC2_INTERVAL_IN),\
    /* Class-Specific AS Isochronous Audio Data Endpoint Descriptor(4.10.1.2) */\
    TUD_AUDIO20_DESC_CS_AS_ISO_EP(/*_attr*/ AUDIO20_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_lockdelayunit*/ AUDIO20_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_UNDEFINED, /*_lockdelay*/ 0x0000)

/* Stereo EPOUT, stereo EPIN descriptor length. 8+9+9+55+55+7+55+55 = 253 */
#define TUD_AUDIO_STEREO_EPOUT_STEREO_EPIN_DESC_LEN (TUD_AUDIO20_DESC_IAD_LEN + \
                                                     TUD_AUDIO20_DESC_STD_AC_LEN + \
                                                     TUD_AUDIO20_DESC_CS_AC_LEN + \
                                                     AC_ITF_DESC_STEREO_LEN + \
                                                     AC_ITF_DESC_STEREO_LEN + \
                                                     TUD_AUDIO20_DESC_STD_AC_INT_EP_LEN + \
                                                     AUDIO_DESC_ITF_LEN + \
                                                     AUDIO_DESC_ITF_LEN)

#define TUD_AUDIO_STEREO_EPOUT_STEREO_EPIN_DESCRIPTOR(_stridx, _epout, _epin, _epint) \
    /* Standard Interface Association Descriptor (IAD) */\
    TUD_AUDIO20_DESC_IAD(/*_firstitf*/ ITF_EPOUT_EPIN_NUM_AUDIO_CONTROL, /*_nitfs*/ AUDIO_EPOUT_EPIN_ITF_NUM_TOTAL, /*_stridx*/ 0x00),\
    /* Standard AC Interface Descriptor(4.7.1) */\
    TUD_AUDIO20_DESC_STD_AC(/*_itfnum*/ ITF_EPOUT_EPIN_NUM_AUDIO_CONTROL, /*_nEPs*/ 0x01, /*_stridx*/ _stridx),\
    /* Class-Specific AC Interface Header Descriptor(4.7.2) */\
    TUD_AUDIO20_DESC_CS_AC(/*_bcdADC*/ 0x0200, /*_category*/ AUDIO20_FUNC_HEADSET, /*_totallen*/ AC_ITF_DESC_STEREO_LEN + AC_ITF_DESC_STEREO_LEN, /*_ctrl*/ AUDIO20_CS_AS_INTERFACE_CTRL_LATENCY_POS),\
    /* Clock Source Descriptor(4.7.2.1) */\
    TUD_AUDIO20_DESC_CLK_SRC(/*_clkid*/ UAC2_ENTITY_EPOUT_CLOCK, /*_attr*/ 3, /*_ctrl*/ 7, /*_assocTerm*/ 0x00,  /*_stridx*/ 0x00),\
    /* Input Terminal Descriptor(4.7.2.4) */\
    TUD_AUDIO20_DESC_INPUT_TERM(/*_termid*/ UAC2_ENTITY_EPOUT_INPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_USB_STREAMING, /*_assocTerm*/ 0x00, /*_clkid*/ UAC2_ENTITY_EPOUT_CLOCK, /*_nchannelslogical*/ 0x02, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_idxchannelnames*/ 0x00, /*_ctrl*/ 0 * (AUDIO20_CTRL_R << AUDIO20_IN_TERM_CTRL_CONNECTOR_POS), /*_stridx*/ 0x00),\
    /* Feature Unit Descriptor(4.7.2.8) */\
    TUD_AUDIO20_DESC_FEATURE_UNIT(/*_unitid*/ UAC2_ENTITY_EPOUT_FEATURE_UNIT, /*_srcid*/ UAC2_ENTITY_EPOUT_INPUT_TERMINAL, /*_stridx*/ 0x00, /*_ctrlch0master*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS), /*_ctrlch1*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS), /*_ctrlch2*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS)),\
    /* Output Terminal Descriptor(4.7.2.5) */\
    TUD_AUDIO20_DESC_OUTPUT_TERM(/*_termid*/ UAC2_ENTITY_EPOUT_OUTPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_OUT_HEADPHONES, /*_assocTerm*/ 0x00, /*_srcid*/ UAC2_ENTITY_EPOUT_FEATURE_UNIT, /*_clkid*/ UAC2_ENTITY_EPOUT_CLOCK, /*_ctrl*/ 0x0000, /*_stridx*/ 0x00),\
    /* Clock Source Descriptor(4.7.2.1) */\
    TUD_AUDIO20_DESC_CLK_SRC(/*_clkid*/ UAC2_ENTITY_EPIN_CLOCK, /*_attr*/ 3, /*_ctrl*/ 7, /*_assocTerm*/ 0x00,  /*_stridx*/ 0x00),\
    /* Input Terminal Descriptor(4.7.2.4) */\
    TUD_AUDIO20_DESC_INPUT_TERM(/*_termid*/ UAC2_ENTITY_EPIN_INPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_IN_GENERIC_MIC, /*_assocTerm*/ 0x00, /*_clkid*/ UAC2_ENTITY_EPIN_CLOCK, /*_nchannelslogical*/ 0x02, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_idxchannelnames*/ 0x00, /*_ctrl*/ 0 * (AUDIO20_CTRL_R << AUDIO20_IN_TERM_CTRL_CONNECTOR_POS), /*_stridx*/ 0x00),\
    /* Feature Unit Descriptor(4.7.2.8) */\
    TUD_AUDIO20_DESC_FEATURE_UNIT(/*_unitid*/ UAC2_ENTITY_EPIN_FEATURE_UNIT, /*_srcid*/ UAC2_ENTITY_EPIN_INPUT_TERMINAL, /*_stridx*/ 0x00, /*_ctrlch0master*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS), /*_ctrlch1*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS), /*_ctrlch2*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS)),\
    /* Output Terminal Descriptor(4.7.2.5) */\
    TUD_AUDIO20_DESC_OUTPUT_TERM(/*_termid*/ UAC2_ENTITY_EPIN_OUTPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_USB_STREAMING, /*_assocTerm*/ 0x00, /*_srcid*/ UAC2_ENTITY_EPIN_FEATURE_UNIT, /*_clkid*/ UAC2_ENTITY_EPIN_CLOCK, /*_ctrl*/ 0x0000, /*_stridx*/ 0x00),\
    /* Standard AC Interrupt Endpoint Descriptor(4.8.2.1) */\
    TUD_AUDIO20_DESC_STD_AC_INT_EP(/*_ep*/ _epint, /*_interval: 2^(7-1) = 64 => 64*125 us = 8000 us*/ (TUD_OPT_HIGH_SPEED ? 0x7 : 0x1)), \
    /* Standard AS Interface Descriptor(4.9.1) */\
    /* Interface 1, Alternate 0 - default alternate setting with 0 bandwidth */\
    TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)(ITF_EPOUT_EPIN_NUM_AUDIO_STREAMING_EPOUT), /*_altset*/ 0x00, /*_nEPs*/ 0x00, /*_stridx*/ STRID_AUDIO_EPOUT_IFACE),\
    /* Standard AS Interface Descriptor(4.9.1) */\
    /* Interface 1, Alternate 1 - alternate interface for data streaming */\
    TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)(ITF_EPOUT_EPIN_NUM_AUDIO_STREAMING_EPOUT), /*_altset*/ 0x01, /*_nEPs*/ 0x01, /*_stridx*/ STRID_AUDIO_EPOUT_IFACE),\
    /* Class-Specific AS Interface Descriptor(4.9.2) */\
    TUD_AUDIO20_DESC_CS_AS_INT(/*_termid*/ UAC2_ENTITY_EPOUT_INPUT_TERMINAL, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_formattype*/ AUDIO20_FORMAT_TYPE_I, /*_formats*/ AUDIO20_DATA_FORMAT_TYPE_I_PCM, /*_nchannelsphysical*/ 0x02, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_stridx*/ 0x00),\
    /* Type I Format Type Descriptor(2.3.1.6 - Audio Formats) */\
    TUD_AUDIO20_DESC_TYPE_I_FORMAT(0, 0),\
    /* Standard AS Isochronous Audio Data Endpoint Descriptor(4.10.1.1) */\
    TUD_AUDIO20_DESC_STD_AS_ISO_EP(/*_ep*/ _epout, /*_attr*/ (uint8_t) ((uint8_t)TUSB_XFER_ISOCHRONOUS | (uint8_t)TUSB_ISO_EP_ATT_ADAPTIVE | (uint8_t)TUSB_ISO_EP_ATT_DATA), /*_maxEPsize*/ 0, /*_interval*/ UAC2_INTERVAL_OUT),\
    /* Class-Specific AS Isochronous Audio Data Endpoint Descriptor(4.10.1.2) */\
    TUD_AUDIO20_DESC_CS_AS_ISO_EP(/*_attr*/ AUDIO20_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_lockdelayunit*/ AUDIO20_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_MILLISEC, /*_lockdelay*/ 0x0001),\
    /* Standard AS Interface Descriptor(4.9.1) */\
    /* Interface 2, Alternate 0 - default alternate setting with 0 bandwidth */\
    TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)(ITF_EPOUT_EPIN_NUM_AUDIO_STREAMING_EPIN), /*_altset*/ 0x00, /*_nEPs*/ 0x00, /*_stridx*/ STRID_AUDIO_EPIN_IFACE),\
    /* Standard AS Interface Descriptor(4.9.1) */\
    /* Interface 2, Alternate 1 - alternate interface for data streaming */\
    TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)(ITF_EPOUT_EPIN_NUM_AUDIO_STREAMING_EPIN), /*_altset*/ 0x01, /*_nEPs*/ 0x01, /*_stridx*/ STRID_AUDIO_EPIN_IFACE),\
    /* Class-Specific AS Interface Descriptor(4.9.2) */\
    TUD_AUDIO20_DESC_CS_AS_INT(/*_termid*/ UAC2_ENTITY_EPIN_OUTPUT_TERMINAL, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_formattype*/ AUDIO20_FORMAT_TYPE_I, /*_formats*/ AUDIO20_DATA_FORMAT_TYPE_I_PCM, /*_nchannelsphysical*/ 0x02, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_stridx*/ 0x00),\
    /* Type I Format Type Descriptor(2.3.1.6 - Audio Formats) */\
    TUD_AUDIO20_DESC_TYPE_I_FORMAT(0, 0),\
    /* Standard AS Isochronous Audio Data Endpoint Descriptor(4.10.1.1) */\
    TUD_AUDIO20_DESC_STD_AS_ISO_EP(/*_ep*/ _epin, /*_attr*/ (uint8_t) ((uint8_t)TUSB_XFER_ISOCHRONOUS | (uint8_t)TUSB_ISO_EP_ATT_ASYNCHRONOUS | (uint8_t)TUSB_ISO_EP_ATT_DATA), /*_maxEPsize*/ 0, /*_interval*/ UAC2_INTERVAL_IN),\
    /* Class-Specific AS Isochronous Audio Data Endpoint Descriptor(4.10.1.2) */\
    TUD_AUDIO20_DESC_CS_AS_ISO_EP(/*_attr*/ AUDIO20_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_lockdelayunit*/ AUDIO20_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_UNDEFINED, /*_lockdelay*/ 0x0000)

/* Stereo EPOUT descriptor length. 8+9+9+51+7+55 = 139 */
#define TUD_AUDIO_MONO_EPOUT_DESC_LEN (TUD_AUDIO20_DESC_IAD_LEN + \
                                       TUD_AUDIO20_DESC_STD_AC_LEN + \
                                       TUD_AUDIO20_DESC_CS_AC_LEN + \
                                       AC_ITF_DESC_STEREO_LEN + \
                                       TUD_AUDIO20_DESC_STD_AC_INT_EP_LEN + \
                                       AUDIO_DESC_ITF_LEN)

#define TUD_AUDIO_MONO_EPOUT_DESCRIPTOR(_stridx, _epout, _epint) \
    /* Standard Interface Association Descriptor (IAD) */\
    TUD_AUDIO20_DESC_IAD(/*_firstitf*/ ITF_EPOUT_NUM_AUDIO_CONTROL, /*_nitfs*/ AUDIO_EPOUT_ITF_NUM_TOTAL, /*_stridx*/ 0x00),\
    /* Standard AC Interface Descriptor(4.7.1) */\
    TUD_AUDIO20_DESC_STD_AC(/*_itfnum*/ ITF_EPOUT_NUM_AUDIO_CONTROL, /*_nEPs*/ 0x01, /*_stridx*/ _stridx),\
    /* Class-Specific AC Interface Header Descriptor(4.7.2) */\
    TUD_AUDIO20_DESC_CS_AC(/*_bcdADC*/ 0x0200, /*_category*/ AUDIO20_FUNC_HEADSET, /*_totallen*/ AC_ITF_DESC_STEREO_LEN, /*_ctrl*/ AUDIO20_CS_AS_INTERFACE_CTRL_LATENCY_POS),\
    /* Clock Source Descriptor(4.7.2.1) */\
    TUD_AUDIO20_DESC_CLK_SRC(/*_clkid*/ UAC2_ENTITY_EPOUT_CLOCK, /*_attr*/ 3, /*_ctrl*/ 7, /*_assocTerm*/ 0x00,  /*_stridx*/ 0x00),\
    /* Input Terminal Descriptor(4.7.2.4) */\
    TUD_AUDIO20_DESC_INPUT_TERM(/*_termid*/ UAC2_ENTITY_EPOUT_INPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_USB_STREAMING, /*_assocTerm*/ 0x00, /*_clkid*/ UAC2_ENTITY_EPOUT_CLOCK, /*_nchannelslogical*/ 0x01, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_idxchannelnames*/ 0x00, /*_ctrl*/ 0 * (AUDIO20_CTRL_R << AUDIO20_IN_TERM_CTRL_CONNECTOR_POS), /*_stridx*/ 0x00),\
    /* Feature Unit Descriptor(4.7.2.8) */\
    TUD_AUDIO20_DESC_FEATURE_UNIT(/*_unitid*/ UAC2_ENTITY_EPOUT_FEATURE_UNIT, /*_srcid*/ UAC2_ENTITY_EPOUT_INPUT_TERMINAL, /*_stridx*/ 0x00, /*_ctrlch0master*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS), /*_ctrlch1*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS)),\
    /* Output Terminal Descriptor(4.7.2.5) */\
    TUD_AUDIO20_DESC_OUTPUT_TERM(/*_termid*/ UAC2_ENTITY_EPOUT_OUTPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_OUT_HEADPHONES, /*_assocTerm*/ 0x00, /*_srcid*/ UAC2_ENTITY_EPOUT_FEATURE_UNIT, /*_clkid*/ UAC2_ENTITY_EPOUT_CLOCK, /*_ctrl*/ 0x0000, /*_stridx*/ 0x00),\
    /* Standard AC Interrupt Endpoint Descriptor(4.8.2.1) */\
    TUD_AUDIO20_DESC_STD_AC_INT_EP(/*_ep*/ _epint, /*_interval: 2^(7-1) = 64 => 64*125 us = 8000 us*/ (TUD_OPT_HIGH_SPEED ? 0x7 : 0x1)), \
    /* Standard AS Interface Descriptor(4.9.1) */\
    /* Interface 1, Alternate 0 - default alternate setting with 0 bandwidth */\
    TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)(ITF_EPOUT_NUM_AUDIO_STREAMING_EPOUT), /*_altset*/ 0x00, /*_nEPs*/ 0x00, /*_stridx*/ STRID_AUDIO_EPOUT_IFACE),\
    /* Standard AS Interface Descriptor(4.9.1) */\
    /* Interface 1, Alternate 1 - alternate interface for data streaming */\
    TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)(ITF_EPOUT_NUM_AUDIO_STREAMING_EPOUT), /*_altset*/ 0x01, /*_nEPs*/ 0x01, /*_stridx*/ STRID_AUDIO_EPOUT_IFACE),\
    /* Class-Specific AS Interface Descriptor(4.9.2) */\
    TUD_AUDIO20_DESC_CS_AS_INT(/*_termid*/ UAC2_ENTITY_EPOUT_INPUT_TERMINAL, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_formattype*/ AUDIO20_FORMAT_TYPE_I, /*_formats*/ AUDIO20_DATA_FORMAT_TYPE_I_PCM, /*_nchannelsphysical*/ 0x01, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_stridx*/ 0x00),\
    /* Type I Format Type Descriptor(2.3.1.6 - Audio Formats) */\
    TUD_AUDIO20_DESC_TYPE_I_FORMAT(0, 0),\
    /* Standard AS Isochronous Audio Data Endpoint Descriptor(4.10.1.1) */\
    TUD_AUDIO20_DESC_STD_AS_ISO_EP(/*_ep*/ _epout, /*_attr*/ (uint8_t) ((uint8_t)TUSB_XFER_ISOCHRONOUS | (uint8_t)TUSB_ISO_EP_ATT_ADAPTIVE | (uint8_t)TUSB_ISO_EP_ATT_DATA), /*_maxEPsize*/ CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX, /*_interval*/ UAC2_INTERVAL_OUT),\
    /* Class-Specific AS Isochronous Audio Data Endpoint Descriptor(4.10.1.2) */\
    TUD_AUDIO20_DESC_CS_AS_ISO_EP(/*_attr*/ AUDIO20_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_lockdelayunit*/ AUDIO20_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_MILLISEC, /*_lockdelay*/ 0x0001)

/* Stereo EPOUT descriptor length. 8+9+9+55+7+55 = 143 */
#define TUD_AUDIO_STEREO_EPOUT_DESC_LEN (TUD_AUDIO20_DESC_IAD_LEN + \
                                         TUD_AUDIO20_DESC_STD_AC_LEN + \
                                         TUD_AUDIO20_DESC_CS_AC_LEN + \
                                         AC_ITF_DESC_STEREO_LEN + \
                                         TUD_AUDIO20_DESC_STD_AC_INT_EP_LEN + \
                                         AUDIO_DESC_ITF_LEN)

#define TUD_AUDIO_STEREO_EPOUT_DESCRIPTOR(_stridx, _epout, _epint) \
    /* Standard Interface Association Descriptor (IAD) */\
    TUD_AUDIO20_DESC_IAD(/*_firstitf*/ ITF_EPOUT_NUM_AUDIO_CONTROL, /*_nitfs*/ AUDIO_EPOUT_ITF_NUM_TOTAL, /*_stridx*/ 0x00),\
    /* Standard AC Interface Descriptor(4.7.1) */\
    TUD_AUDIO20_DESC_STD_AC(/*_itfnum*/ ITF_EPOUT_NUM_AUDIO_CONTROL, /*_nEPs*/ 0x01, /*_stridx*/ _stridx),\
    /* Class-Specific AC Interface Header Descriptor(4.7.2) */\
    TUD_AUDIO20_DESC_CS_AC(/*_bcdADC*/ 0x0200, /*_category*/ AUDIO20_FUNC_HEADSET, /*_totallen*/ AC_ITF_DESC_STEREO_LEN, /*_ctrl*/ AUDIO20_CS_AS_INTERFACE_CTRL_LATENCY_POS),\
    /* Clock Source Descriptor(4.7.2.1) */\
    TUD_AUDIO20_DESC_CLK_SRC(/*_clkid*/ UAC2_ENTITY_EPOUT_CLOCK, /*_attr*/ 3, /*_ctrl*/ 7, /*_assocTerm*/ 0x00,  /*_stridx*/ 0x00),\
    /* Input Terminal Descriptor(4.7.2.4) */\
    TUD_AUDIO20_DESC_INPUT_TERM(/*_termid*/ UAC2_ENTITY_EPOUT_INPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_USB_STREAMING, /*_assocTerm*/ 0x00, /*_clkid*/ UAC2_ENTITY_EPOUT_CLOCK, /*_nchannelslogical*/ 0x02, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_idxchannelnames*/ 0x00, /*_ctrl*/ 0 * (AUDIO20_CTRL_R << AUDIO20_IN_TERM_CTRL_CONNECTOR_POS), /*_stridx*/ 0x00),\
    /* Feature Unit Descriptor(4.7.2.8) */\
    TUD_AUDIO20_DESC_FEATURE_UNIT(/*_unitid*/ UAC2_ENTITY_EPOUT_FEATURE_UNIT, /*_srcid*/ UAC2_ENTITY_EPOUT_INPUT_TERMINAL, /*_stridx*/ 0x00, /*_ctrlch0master*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS), /*_ctrlch1*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS), /*_ctrlch2*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS)),\
    /* Output Terminal Descriptor(4.7.2.5) */\
    TUD_AUDIO20_DESC_OUTPUT_TERM(/*_termid*/ UAC2_ENTITY_EPOUT_OUTPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_OUT_HEADPHONES, /*_assocTerm*/ 0x00, /*_srcid*/ UAC2_ENTITY_EPOUT_FEATURE_UNIT, /*_clkid*/ UAC2_ENTITY_EPOUT_CLOCK, /*_ctrl*/ 0x0000, /*_stridx*/ 0x00),\
    /* Standard AC Interrupt Endpoint Descriptor(4.8.2.1) */\
    TUD_AUDIO20_DESC_STD_AC_INT_EP(/*_ep*/ _epint, /*_interval: 2^(7-1) = 64 => 64*125 us = 8000 us*/ (TUD_OPT_HIGH_SPEED ? 0x7 : 0x1)), \
    /* Standard AS Interface Descriptor(4.9.1) */\
    /* Interface 1, Alternate 0 - default alternate setting with 0 bandwidth */\
    TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)(ITF_EPOUT_NUM_AUDIO_STREAMING_EPOUT), /*_altset*/ 0x00, /*_nEPs*/ 0x00, /*_stridx*/ STRID_AUDIO_EPOUT_IFACE),\
    /* Standard AS Interface Descriptor(4.9.1) */\
    /* Interface 1, Alternate 1 - alternate interface for data streaming */\
    TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)(ITF_EPOUT_NUM_AUDIO_STREAMING_EPOUT), /*_altset*/ 0x01, /*_nEPs*/ 0x01, /*_stridx*/ STRID_AUDIO_EPOUT_IFACE),\
    /* Class-Specific AS Interface Descriptor(4.9.2) */\
    TUD_AUDIO20_DESC_CS_AS_INT(/*_termid*/ UAC2_ENTITY_EPOUT_INPUT_TERMINAL, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_formattype*/ AUDIO20_FORMAT_TYPE_I, /*_formats*/ AUDIO20_DATA_FORMAT_TYPE_I_PCM, /*_nchannelsphysical*/ 0x02, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_stridx*/ 0x00),\
    /* Type I Format Type Descriptor(2.3.1.6 - Audio Formats) */\
    TUD_AUDIO20_DESC_TYPE_I_FORMAT(0, 0),\
    /* Standard AS Isochronous Audio Data Endpoint Descriptor(4.10.1.1) */\
    TUD_AUDIO20_DESC_STD_AS_ISO_EP(/*_ep*/ _epout, /*_attr*/ (uint8_t) ((uint8_t)TUSB_XFER_ISOCHRONOUS | (uint8_t)TUSB_ISO_EP_ATT_ADAPTIVE | (uint8_t)TUSB_ISO_EP_ATT_DATA), /*_maxEPsize*/ CFG_TUD_AUDIO_FUNC_1_EP_OUT_SZ_MAX, /*_interval*/ UAC2_INTERVAL_OUT),\
    /* Class-Specific AS Isochronous Audio Data Endpoint Descriptor(4.10.1.2) */\
    TUD_AUDIO20_DESC_CS_AS_ISO_EP(/*_attr*/ AUDIO20_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_lockdelayunit*/ AUDIO20_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_MILLISEC, /*_lockdelay*/ 0x0001)


/* Mono EPIN descriptor length. 8+9+9+51+7+55 = 139 */
#define TUD_AUDIO_MONO_EPIN_DESC_LEN (TUD_AUDIO20_DESC_IAD_LEN + \
                                      TUD_AUDIO20_DESC_STD_AC_LEN + \
                                      TUD_AUDIO20_DESC_CS_AC_LEN + \
                                      AC_ITF_DESC_MONO_LEN + \
                                      TUD_AUDIO20_DESC_STD_AC_INT_EP_LEN + \
                                      AUDIO_DESC_ITF_LEN)

#define TUD_AUDIO_MONO_EPIN_DESCRIPTOR(_stridx, _epin, _epint) \
    /* Standard Interface Association Descriptor (IAD) */\
    TUD_AUDIO20_DESC_IAD(/*_firstitf*/ ITF_EPIN_NUM_AUDIO_CONTROL, /*_nitfs*/ AUDIO_EPIN_ITF_NUM_TOTAL, /*_stridx*/ 0x00),\
    /* Standard AC Interface Descriptor(4.7.1) */\
    TUD_AUDIO20_DESC_STD_AC(/*_itfnum*/ ITF_EPIN_NUM_AUDIO_CONTROL, /*_nEPs*/ 0x01, /*_stridx*/ _stridx),\
    /* Class-Specific AC Interface Header Descriptor(4.7.2) */\
    TUD_AUDIO20_DESC_CS_AC(/*_bcdADC*/ 0x0200, /*_category*/ AUDIO20_FUNC_MICROPHONE, /*_totallen*/ AC_ITF_DESC_MONO_LEN, /*_ctrl*/ AUDIO20_CS_AS_INTERFACE_CTRL_LATENCY_POS),\
    /* Clock Source Descriptor(4.7.2.1) */\
    TUD_AUDIO20_DESC_CLK_SRC(/*_clkid*/ UAC2_ENTITY_EPIN_CLOCK, /*_attr*/ 3, /*_ctrl*/ 7, /*_assocTerm*/ 0x00,  /*_stridx*/ 0x00),\
    /* Input Terminal Descriptor(4.7.2.4) */\
    TUD_AUDIO20_DESC_INPUT_TERM(/*_termid*/ UAC2_ENTITY_EPIN_INPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_IN_GENERIC_MIC, /*_assocTerm*/ 0x00, /*_clkid*/ UAC2_ENTITY_EPIN_CLOCK, /*_nchannelslogical*/ 0x01, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_idxchannelnames*/ 0x00, /*_ctrl*/ 0 * (AUDIO20_CTRL_R << AUDIO20_IN_TERM_CTRL_CONNECTOR_POS), /*_stridx*/ 0x00),\
    /* Feature Unit Descriptor(4.7.2.8) */\
    TUD_AUDIO20_DESC_FEATURE_UNIT(/*_unitid*/ UAC2_ENTITY_EPIN_FEATURE_UNIT, /*_srcid*/ UAC2_ENTITY_EPIN_INPUT_TERMINAL, /*_stridx*/ 0x00, /*_ctrlch0master*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS), /*_ctrlch1*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS)),\
    /* Output Terminal Descriptor(4.7.2.5) */\
    TUD_AUDIO20_DESC_OUTPUT_TERM(/*_termid*/ UAC2_ENTITY_EPIN_OUTPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_USB_STREAMING, /*_assocTerm*/ 0x00, /*_srcid*/ UAC2_ENTITY_EPIN_FEATURE_UNIT, /*_clkid*/ UAC2_ENTITY_EPIN_CLOCK, /*_ctrl*/ 0x0000, /*_stridx*/ 0x00),\
    /* Standard AC Interrupt Endpoint Descriptor(4.8.2.1) */\
    TUD_AUDIO20_DESC_STD_AC_INT_EP(/*_ep*/ _epint, /*_interval: 2^(7-1) = 64 => 64*125 us = 8000 us*/ (TUD_OPT_HIGH_SPEED ? 0x7 : 0x1)), \
    /* Standard AS Interface Descriptor(4.9.1) */\
    /* Interface 2, Alternate 0 - default alternate setting with 0 bandwidth */\
    TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)(ITF_EPIN_NUM_AUDIO_STREAMING_EPIN), /*_altset*/ 0x00, /*_nEPs*/ 0x00, /*_stridx*/ STRID_AUDIO_EPIN_IFACE),\
    /* Standard AS Interface Descriptor(4.9.1) */\
    /* Interface 2, Alternate 1 - alternate interface for data streaming */\
    TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)(ITF_EPIN_NUM_AUDIO_STREAMING_EPIN), /*_altset*/ 0x01, /*_nEPs*/ 0x01, /*_stridx*/ STRID_AUDIO_EPIN_IFACE),\
    /* Class-Specific AS Interface Descriptor(4.9.2) */\
    TUD_AUDIO20_DESC_CS_AS_INT(/*_termid*/ UAC2_ENTITY_EPIN_OUTPUT_TERMINAL, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_formattype*/ AUDIO20_FORMAT_TYPE_I, /*_formats*/ AUDIO20_DATA_FORMAT_TYPE_I_PCM, /*_nchannelsphysical*/ 0x01, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_stridx*/ 0x00),\
    /* Type I Format Type Descriptor(2.3.1.6 - Audio Formats) */\
    TUD_AUDIO20_DESC_TYPE_I_FORMAT(0, 0),\
    /* Standard AS Isochronous Audio Data Endpoint Descriptor(4.10.1.1) */\
    TUD_AUDIO20_DESC_STD_AS_ISO_EP(/*_ep*/ _epin, /*_attr*/ (uint8_t) ((uint8_t)TUSB_XFER_ISOCHRONOUS | (uint8_t)TUSB_ISO_EP_ATT_ASYNCHRONOUS | (uint8_t)TUSB_ISO_EP_ATT_DATA), /*_maxEPsize*/ 0, /*_interval*/ UAC2_INTERVAL_IN),\
    /* Class-Specific AS Isochronous Audio Data Endpoint Descriptor(4.10.1.2) */\
    TUD_AUDIO20_DESC_CS_AS_ISO_EP(/*_attr*/ AUDIO20_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_lockdelayunit*/ AUDIO20_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_UNDEFINED, /*_lockdelay*/ 0x0000)


/* Stereo EPIN descriptor length. 8+9+9+55+7+55 = 143 */
#define TUD_AUDIO_STEREO_EPIN_DESC_LEN (TUD_AUDIO20_DESC_IAD_LEN + \
                                        TUD_AUDIO20_DESC_STD_AC_LEN + \
                                        TUD_AUDIO20_DESC_CS_AC_LEN + \
                                        AC_ITF_DESC_STEREO_LEN + \
                                        TUD_AUDIO20_DESC_STD_AC_INT_EP_LEN + \
                                        AUDIO_DESC_ITF_LEN)

#define TUD_AUDIO_STEREO_EPIN_DESCRIPTOR(_stridx, _epin, _epint) \
    /* Standard Interface Association Descriptor (IAD) */\
    TUD_AUDIO20_DESC_IAD(/*_firstitf*/ ITF_EPIN_NUM_AUDIO_CONTROL, /*_nitfs*/ AUDIO_EPIN_ITF_NUM_TOTAL, /*_stridx*/ 0x00),\
    /* Standard AC Interface Descriptor(4.7.1) */\
    TUD_AUDIO20_DESC_STD_AC(/*_itfnum*/ ITF_EPIN_NUM_AUDIO_CONTROL, /*_nEPs*/ 0x01, /*_stridx*/ _stridx),\
    /* Class-Specific AC Interface Header Descriptor(4.7.2) */\
    TUD_AUDIO20_DESC_CS_AC(/*_bcdADC*/ 0x0200, /*_category*/ AUDIO20_FUNC_MICROPHONE, /*_totallen*/ AC_ITF_DESC_STEREO_LEN, /*_ctrl*/ AUDIO20_CS_AS_INTERFACE_CTRL_LATENCY_POS),\
    /* Clock Source Descriptor(4.7.2.1) */\
    TUD_AUDIO20_DESC_CLK_SRC(/*_clkid*/ UAC2_ENTITY_EPIN_CLOCK, /*_attr*/ 3, /*_ctrl*/ 7, /*_assocTerm*/ 0x00,  /*_stridx*/ 0x00),\
    /* Input Terminal Descriptor(4.7.2.4) */\
    TUD_AUDIO20_DESC_INPUT_TERM(/*_termid*/ UAC2_ENTITY_EPIN_INPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_IN_GENERIC_MIC, /*_assocTerm*/ 0x00, /*_clkid*/ UAC2_ENTITY_EPIN_CLOCK, /*_nchannelslogical*/ 0x02, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_idxchannelnames*/ 0x00, /*_ctrl*/ 0 * (AUDIO20_CTRL_R << AUDIO20_IN_TERM_CTRL_CONNECTOR_POS), /*_stridx*/ 0x00),\
    /* Feature Unit Descriptor(4.7.2.8) */\
    TUD_AUDIO20_DESC_FEATURE_UNIT(/*_unitid*/ UAC2_ENTITY_EPIN_FEATURE_UNIT, /*_srcid*/ UAC2_ENTITY_EPIN_INPUT_TERMINAL, /*_stridx*/ 0x00, /*_ctrlch0master*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS), /*_ctrlch1*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS), /*_ctrlch2*/ (AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_MUTE_POS | AUDIO20_CTRL_RW << AUDIO20_FEATURE_UNIT_CTRL_VOLUME_POS)),\
    /* Output Terminal Descriptor(4.7.2.5) */\
    TUD_AUDIO20_DESC_OUTPUT_TERM(/*_termid*/ UAC2_ENTITY_EPIN_OUTPUT_TERMINAL, /*_termtype*/ AUDIO_TERM_TYPE_USB_STREAMING, /*_assocTerm*/ 0x00, /*_srcid*/ UAC2_ENTITY_EPIN_FEATURE_UNIT, /*_clkid*/ UAC2_ENTITY_EPIN_CLOCK, /*_ctrl*/ 0x0000, /*_stridx*/ 0x00),\
    /* Standard AC Interrupt Endpoint Descriptor(4.8.2.1) */\
    TUD_AUDIO20_DESC_STD_AC_INT_EP(/*_ep*/ _epint, /*_interval: 2^(7-1) = 64 => 64*125 us = 8000 us*/ (TUD_OPT_HIGH_SPEED ? 0x7 : 0x1)), \
    /* Standard AS Interface Descriptor(4.9.1) */\
    /* Interface 2, Alternate 0 - default alternate setting with 0 bandwidth */\
    TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)(ITF_EPIN_NUM_AUDIO_STREAMING_EPIN), /*_altset*/ 0x00, /*_nEPs*/ 0x00, /*_stridx*/ STRID_AUDIO_EPIN_IFACE),\
    /* Standard AS Interface Descriptor(4.9.1) */\
    /* Interface 2, Alternate 1 - alternate interface for data streaming */\
    TUD_AUDIO20_DESC_STD_AS_INT(/*_itfnum*/ (uint8_t)(ITF_EPIN_NUM_AUDIO_STREAMING_EPIN), /*_altset*/ 0x01, /*_nEPs*/ 0x01, /*_stridx*/ STRID_AUDIO_EPIN_IFACE),\
    /* Class-Specific AS Interface Descriptor(4.9.2) */\
    TUD_AUDIO20_DESC_CS_AS_INT(/*_termid*/ UAC2_ENTITY_EPIN_OUTPUT_TERMINAL, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_formattype*/ AUDIO20_FORMAT_TYPE_I, /*_formats*/ AUDIO20_DATA_FORMAT_TYPE_I_PCM, /*_nchannelsphysical*/ 0x02, /*_channelcfg*/ AUDIO20_CHANNEL_CONFIG_NON_PREDEFINED, /*_stridx*/ 0x00),\
    /* Type I Format Type Descriptor(2.3.1.6 - Audio Formats) */\
    TUD_AUDIO20_DESC_TYPE_I_FORMAT(0, 0),\
    /* Standard AS Isochronous Audio Data Endpoint Descriptor(4.10.1.1) */\
    TUD_AUDIO20_DESC_STD_AS_ISO_EP(/*_ep*/ _epin, /*_attr*/ (uint8_t) ((uint8_t)TUSB_XFER_ISOCHRONOUS | (uint8_t)TUSB_ISO_EP_ATT_ASYNCHRONOUS | (uint8_t)TUSB_ISO_EP_ATT_DATA), /*_maxEPsize*/ 0, /*_interval*/ UAC2_INTERVAL_IN),\
    /* Class-Specific AS Isochronous Audio Data Endpoint Descriptor(4.10.1.2) */\
    TUD_AUDIO20_DESC_CS_AS_ISO_EP(/*_attr*/ AUDIO20_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK, /*_ctrl*/ AUDIO20_CTRL_NONE, /*_lockdelayunit*/ AUDIO20_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_UNDEFINED, /*_lockdelay*/ 0x0000)

// clang-format on

#endif /* _USB_DESCRIPTORS_H_ */
