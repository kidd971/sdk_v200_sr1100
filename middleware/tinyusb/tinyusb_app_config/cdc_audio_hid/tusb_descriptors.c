/** @file  tusb_descriptors.c
 *  @brief TinyUSB descriptor file.
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

/* INCLUDES *******************************************************************/
#include "tusb_descriptors.h"
#include "tinyusb_facade.h"
#include "tusb.h"

/* CONSTANTS ******************************************************************/
/* A combination of interfaces must have a unique product id, since PC will save device driver after the first plug.
 * Same VID/PID with different interface e.g MSC (first), then CDC (later) will possibly cause system error on PC.
 *
 * Auto ProductID layout's Bitmap:
 *   [MSB]         HID | MSC | CDC          [LSB]
 */
#define _PID_MAP(itf, n) ((CFG_TUD_##itf) << (n))
#ifndef TUSB_PID
#define TUSB_PID \
    (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4))
#endif

#ifndef TUSB_VID
#define TUSB_VID 0xCafe
#endif

/* BUSB Specification Release Number in Binary-Coded Decimal. */
#define USB_BCD 0x0200

/* Configuration descriptor total length. */
#define MONO_EPOUT_MONO_EPIN_CONFIG_TOTAL_LEN                                                                         \
    (TUD_CONFIG_DESC_LEN + CFG_TUD_CDC * TUD_CDC_DESC_LEN + CFG_TUD_AUDIO * TUD_AUDIO_MONO_EPOUT_MONO_EPIN_DESC_LEN + \
     CFG_TUD_HID * TUD_HID_DESC_LEN)
#define MONO_EPOUT_STEREO_EPIN_CONFIG_TOTAL_LEN             \
    (TUD_CONFIG_DESC_LEN + CFG_TUD_CDC * TUD_CDC_DESC_LEN + \
     CFG_TUD_AUDIO * TUD_AUDIO_MONO_EPOUT_STEREO_EPIN_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)
#define STEREO_EPOUT_MONO_EPIN_CONFIG_TOTAL_LEN             \
    (TUD_CONFIG_DESC_LEN + CFG_TUD_CDC * TUD_CDC_DESC_LEN + \
     CFG_TUD_AUDIO * TUD_AUDIO_STEREO_EPOUT_MONO_EPIN_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)
#define STEREO_EPOUT_STEREO_EPIN_CONFIG_TOTAL_LEN           \
    (TUD_CONFIG_DESC_LEN + CFG_TUD_CDC * TUD_CDC_DESC_LEN + \
     CFG_TUD_AUDIO * TUD_AUDIO_STEREO_EPOUT_STEREO_EPIN_DESC_LEN + CFG_TUD_HID * TUD_HID_DESC_LEN)
#define MONO_EPOUT_CONFIG_TOTAL_LEN                                                                         \
    (TUD_CONFIG_DESC_LEN + CFG_TUD_CDC * TUD_CDC_DESC_LEN + CFG_TUD_AUDIO * TUD_AUDIO_MONO_EPOUT_DESC_LEN + \
     CFG_TUD_HID * TUD_HID_DESC_LEN)
#define STEREO_EPOUT_CONFIG_TOTAL_LEN                                                                         \
    (TUD_CONFIG_DESC_LEN + CFG_TUD_CDC * TUD_CDC_DESC_LEN + CFG_TUD_AUDIO * TUD_AUDIO_STEREO_EPOUT_DESC_LEN + \
     CFG_TUD_HID * TUD_HID_DESC_LEN)
#define MONO_EPIN_CONFIG_TOTAL_LEN                                                                         \
    (TUD_CONFIG_DESC_LEN + CFG_TUD_CDC * TUD_CDC_DESC_LEN + CFG_TUD_AUDIO * TUD_AUDIO_MONO_EPIN_DESC_LEN + \
     CFG_TUD_HID * TUD_HID_DESC_LEN)
#define STEREO_EPIN_CONFIG_TOTAL_LEN                                                                         \
    (TUD_CONFIG_DESC_LEN + CFG_TUD_CDC * TUD_CDC_DESC_LEN + CFG_TUD_AUDIO * TUD_AUDIO_STEREO_EPIN_DESC_LEN + \
     CFG_TUD_HID * TUD_HID_DESC_LEN)

/* Endpoints. */
#define EPNUM_CDC_0_NOTIF 0x81
#define EPNUM_CDC_0_OUT   0x02
#define EPNUM_CDC_0_IN    0x82
#define EPNUM_AUDIO_IN    0x83
#define EPNUM_AUDIO_OUT   0x03
#define EPNUM_AUDIO_INT   0x84
#define EPNUM_HID         0x85

/** @brief Volume control values.
 */
enum volume_ctrl {
    VOLUME_CTRL_0_DB = 0,
    VOLUME_CTRL_10_DB = 2560,
    VOLUME_CTRL_20_DB = 5120,
    VOLUME_CTRL_30_DB = 7680,
    VOLUME_CTRL_40_DB = 10240,
    VOLUME_CTRL_50_DB = 12800,
    VOLUME_CTRL_60_DB = 15360,
    VOLUME_CTRL_70_DB = 17920,
    VOLUME_CTRL_80_DB = 20480,
    VOLUME_CTRL_90_DB = 23040,
    VOLUME_CTRL_100_DB = 25600,
    VOLUME_CTRL_SILENCE = 0x8000,
};

/* MONO STEREO defines */
#define IS_EPIN_MONO    (ep_in_cfg.nb_ch == 1)
#define IS_EPIN_STEREO  (ep_in_cfg.nb_ch == 2)
#define IS_EPOUT_MONO   (ep_out_cfg.nb_ch == 1)
#define IS_EPOUT_STEREO (ep_out_cfg.nb_ch == 2)

/* PRIVATE GLOBALS ************************************************************/
/** @brief Device descriptor.
 */
static tusb_desc_device_t desc_device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = USB_BCD,
    /* Use Interface Association Descriptor (IAD) for CDC
     * As required by USB Specs IAD's subclass must be common class (2) and protocol must be IAD (1)
     */
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = TUSB_VID,
    .idProduct = TUSB_PID,
    .bcdDevice = 0x0100,
    .iManufacturer = 0x01,
    .iProduct = 0x02,
    .iSerialNumber = 0x03,
    .bNumConfigurations = 0x01,
};

/* Audio configuration of the audio stream output of the host. */
static tud_audio_config_t ep_out_cfg;
/* Audio configuration of the audio stream input of the host. */
static tud_audio_config_t ep_in_cfg;
static bool is_ep_in_en;
static bool is_ep_out_en;

// List of supported sample rates
#if CFG_TUD_AUDIO_ENABLE_EP_OUT
#define SPK_N_SAMPLE_RATES 1
#endif
#if CFG_TUD_AUDIO_ENABLE_EP_IN
#define MIC_N_SAMPLE_RATES 1
#endif

// Audio controls
// Current states
#if CFG_TUD_AUDIO_ENABLE_EP_IN
int8_t epin_mute[CFG_TUD_AUDIO_FUNC_1_MAX_N_CHANNELS_IN + 1]; /* +1 for master channel 0. */
int16_t epin_volume[] = {[0 ...(CFG_TUD_AUDIO_FUNC_1_MAX_N_CHANNELS_IN + 1)] = -12 * 256}; /* +1 for master channel 0.
                                                                                            */
#endif
#if CFG_TUD_AUDIO_ENABLE_EP_OUT
int8_t epout_mute[CFG_TUD_AUDIO_FUNC_1_MAX_N_CHANNELS_OUT + 1]; /* +1 for master channel 0. */
int16_t epout_volume[] = {[0 ...(CFG_TUD_AUDIO_FUNC_1_MAX_N_CHANNELS_OUT + 1)] = -12 * 256}; /* +1 for master channel 0.
                                                                                              */
#endif

/** @brief HID report descriptor.
 */
uint8_t const desc_hid_report1[] = {
    /* Usage Page (Consumer). */
    0x05,
    0x0C,
    /* Usage (Consumer Control). */
    0x09,
    0x01,
    /* Collection (Application). */
    0xA1,
    0x01,
    /* Report ID (1). */
    0x85,
    USB_HID_MEDIA_REPORT_ID,
    /* Usage Page (Consumer). */
    0x05,
    0x0C,
    /* Logical Minimum (0). */
    0x15,
    0x00,
    /* Logical Maximum (1). */
    0x25,
    0x01,
    /* Report Size (1). */
    0x75,
    0x01,
    /* Report Count (8). */
    0x95,
    0x08,
    /* Usage (Scan Next Track). */
    0x09,
    0xB5,
    /* Usage (Scan Previous Track). */
    0x09,
    0xB6,
    /* Usage (Stop). */
    0x09,
    0xB7,
    /* Usage (Eject). */
    0x09,
    0xB8,
    /* Usage (Play/Pause). */
    0x09,
    0xCD,
    /* Usage (Mute). */
    0x09,
    0xE2,
    /* Usage (Volume Increment). */
    0x09,
    0xE9,
    /* Usage (Volume Decrement). */
    0x09,
    0xEA,
    /* Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position). */
    0x81,
    0x02,
    /* Report ID (2). */
    0x85,
    USB_HID_CONTROL_DATA_REPORT_ID,
    /* Usage (Undefined). */
    0x09,
    0x00,
    /* Logical Minimum (0). */
    0x15,
    0x00,
    /* Logical Maximum (255). */
    0x25,
    0xFF,
    /* Report Size (8). */
    0x75,
    0x08,
    /* Report Count (63). */
    0x95,
    0x3F,
    /* Feature (Data,Var,Abs) */
    0xB1,
    0x02,
    /* End Collection. */
    0xC0,
};

typedef struct TU_ATTR_PACKED cdc_desc {
    tusb_desc_interface_assoc_t itf_assoc;
    tusb_desc_interface_t ctrl_itf;
    cdc_desc_func_header_t cdc_cs_itf_hdr;
    cdc_desc_func_call_management_t cdc_cs_itf_cm;
    cdc_desc_func_acm_t cdc_cs_itf_acm;
    cdc_desc_func_union_t cdc_cs_itf_union;
    tusb_desc_endpoint_t ctrl_ep;
    tusb_desc_interface_t data_itf;
    tusb_desc_endpoint_t out_ep;
    tusb_desc_endpoint_t in_ep;
} cdc_desc_t;

/// AUDIO Feature Unit Descriptor(4.7.2.8) for multiple channels
typedef struct TU_ATTR_PACKED audio_mono_epout_mono_epin_desc {
    tusb_desc_interface_assoc_t itf_assoc;
    tusb_desc_interface_t itf;
    audio20_desc_cs_ac_interface_t cs_ac_itf;
    audio20_desc_clock_source_t epout_clk;
    audio20_desc_input_terminal_t epout_input;
    audio20_desc_feature_unit_n_t(1) epout_fu;
    audio20_desc_output_terminal_t epout_output;
    audio20_desc_clock_source_t epin_clk;
    audio20_desc_input_terminal_t epin_input;
    audio20_desc_feature_unit_n_t(1) epin_fu;
    audio20_desc_output_terminal_t epin_output;
    tusb_desc_endpoint_t int_ep;
    tusb_desc_interface_t epout_alt0_itf;
    tusb_desc_interface_t epout_alt1_itf;
    audio20_desc_cs_as_interface_t epout_cs_as;
    audio20_desc_type_I_format_t epout_type_i_format;
    tusb_desc_endpoint_t epout_std_iso_ep;
    audio20_desc_cs_as_iso_data_ep_t epout_cs_as_iso_data_ep;
    tusb_desc_interface_t epin_alt0_itf;
    tusb_desc_interface_t epin_alt1_itf;
    audio20_desc_cs_as_interface_t epin_cs_as;
    audio20_desc_type_I_format_t epin_type_i_format;
    tusb_desc_endpoint_t epin_std_iso_ep;
    audio20_desc_cs_as_iso_data_ep_t epin_cs_as_iso_data_ep;
} audio_mono_epout_mono_epin_desc_t;

typedef struct TU_ATTR_PACKED audio_mono_epout_stereo_epin_desc {
    tusb_desc_interface_assoc_t itf_assoc;
    tusb_desc_interface_t itf;
    audio20_desc_cs_ac_interface_t cs_ac_itf;
    audio20_desc_clock_source_t epout_clk;
    audio20_desc_input_terminal_t epout_input;
    audio20_desc_feature_unit_n_t(1) epout_fu;
    audio20_desc_output_terminal_t epout_output;
    audio20_desc_clock_source_t epin_clk;
    audio20_desc_input_terminal_t epin_input;
    audio20_desc_feature_unit_n_t(2) epin_fu;
    audio20_desc_output_terminal_t epin_output;
    tusb_desc_endpoint_t int_ep;
    tusb_desc_interface_t epout_alt0_itf;
    tusb_desc_interface_t epout_alt1_itf;
    audio20_desc_cs_as_interface_t epout_cs_as;
    audio20_desc_type_I_format_t epout_type_i_format;
    tusb_desc_endpoint_t epout_std_iso_ep;
    audio20_desc_cs_as_iso_data_ep_t epout_cs_as_iso_data_ep;
    tusb_desc_interface_t epin_alt0_itf;
    tusb_desc_interface_t epin_alt1_itf;
    audio20_desc_cs_as_interface_t epin_cs_as;
    audio20_desc_type_I_format_t epin_type_i_format;
    tusb_desc_endpoint_t epin_std_iso_ep;
    audio20_desc_cs_as_iso_data_ep_t epin_cs_as_iso_data_ep;
} audio_mono_epout_stereo_epin_desc_t;

typedef struct TU_ATTR_PACKED audio_stereo_epout_mono_epin_desc {
    tusb_desc_interface_assoc_t itf_assoc;
    tusb_desc_interface_t itf;
    audio20_desc_cs_ac_interface_t cs_ac_itf;
    audio20_desc_clock_source_t epout_clk;
    audio20_desc_input_terminal_t epout_input;
    audio20_desc_feature_unit_n_t(2) epout_fu;
    audio20_desc_output_terminal_t epout_output;
    audio20_desc_clock_source_t epin_clk;
    audio20_desc_input_terminal_t epin_input;
    audio20_desc_feature_unit_n_t(1) epin_fu;
    audio20_desc_output_terminal_t epin_output;
    tusb_desc_endpoint_t int_ep;
    tusb_desc_interface_t epout_alt0_itf;
    tusb_desc_interface_t epout_alt1_itf;
    audio20_desc_cs_as_interface_t epout_cs_as;
    audio20_desc_type_I_format_t epout_type_i_format;
    tusb_desc_endpoint_t epout_std_iso_ep;
    audio20_desc_cs_as_iso_data_ep_t epout_cs_as_iso_data_ep;
    tusb_desc_interface_t epin_alt0_itf;
    tusb_desc_interface_t epin_alt1_itf;
    audio20_desc_cs_as_interface_t epin_cs_as;
    audio20_desc_type_I_format_t epin_type_i_format;
    tusb_desc_endpoint_t epin_std_iso_ep;
    audio20_desc_cs_as_iso_data_ep_t epin_cs_as_iso_data_ep;
} audio_stereo_epout_mono_epin_desc_t;

typedef struct TU_ATTR_PACKED audio_stereo_epout_stereo_epin_desc {
    tusb_desc_interface_assoc_t itf_assoc;
    tusb_desc_interface_t itf;
    audio20_desc_cs_ac_interface_t cs_ac_itf;
    audio20_desc_clock_source_t epout_clk;
    audio20_desc_input_terminal_t epout_input;
    audio20_desc_feature_unit_n_t(2) epout_fu;
    audio20_desc_output_terminal_t epout_output;
    audio20_desc_clock_source_t epin_clk;
    audio20_desc_input_terminal_t epin_input;
    audio20_desc_feature_unit_n_t(2) epin_fu;
    audio20_desc_output_terminal_t epin_output;
    tusb_desc_endpoint_t int_ep;
    tusb_desc_interface_t epout_alt0_itf;
    tusb_desc_interface_t epout_alt1_itf;
    audio20_desc_cs_as_interface_t epout_cs_as;
    audio20_desc_type_I_format_t epout_type_i_format;
    tusb_desc_endpoint_t epout_std_iso_ep;
    audio20_desc_cs_as_iso_data_ep_t epout_cs_as_iso_data_ep;
    tusb_desc_interface_t epin_alt0_itf;
    tusb_desc_interface_t epin_alt1_itf;
    audio20_desc_cs_as_interface_t epin_cs_as;
    audio20_desc_type_I_format_t epin_type_i_format;
    tusb_desc_endpoint_t epin_std_iso_ep;
    audio20_desc_cs_as_iso_data_ep_t epin_cs_as_iso_data_ep;
} audio_stereo_epout_stereo_epin_desc_t;

typedef struct TU_ATTR_PACKED audio_mono_epout_desc {
    tusb_desc_interface_assoc_t itf_assoc;
    tusb_desc_interface_t itf;
    audio20_desc_cs_ac_interface_t cs_ac_itf;
    audio20_desc_clock_source_t epout_clk;
    audio20_desc_input_terminal_t epout_input;
    audio20_desc_feature_unit_n_t(1) epout_fu;
    audio20_desc_output_terminal_t epout_output;
    tusb_desc_endpoint_t int_ep;
    tusb_desc_interface_t epout_alt0_itf;
    tusb_desc_interface_t epout_alt1_itf;
    audio20_desc_cs_as_interface_t epout_cs_as;
    audio20_desc_type_I_format_t epout_type_i_format;
    tusb_desc_endpoint_t epout_std_iso_ep;
    audio20_desc_cs_as_iso_data_ep_t epout_cs_as_iso_data_ep;
} audio_mono_epout_desc_t;

typedef struct TU_ATTR_PACKED audio_stereo_epout_desc {
    tusb_desc_interface_assoc_t itf_assoc;
    tusb_desc_interface_t itf;
    audio20_desc_cs_ac_interface_t cs_ac_itf;
    audio20_desc_clock_source_t epout_clk;
    audio20_desc_input_terminal_t epout_input;
    audio20_desc_feature_unit_n_t(2) epout_fu;
    audio20_desc_output_terminal_t epout_output;
    tusb_desc_endpoint_t int_ep;
    tusb_desc_interface_t epout_alt0_itf;
    tusb_desc_interface_t epout_alt1_itf;
    audio20_desc_cs_as_interface_t epout_cs_as;
    audio20_desc_type_I_format_t epout_type_i_format;
    tusb_desc_endpoint_t epout_std_iso_ep;
    audio20_desc_cs_as_iso_data_ep_t epout_cs_as_iso_data_ep;
} audio_stereo_epout_desc_t;

typedef struct TU_ATTR_PACKED audio_mono_epin_desc {
    tusb_desc_interface_assoc_t itf_assoc;
    tusb_desc_interface_t itf;
    audio20_desc_cs_ac_interface_t cs_ac_itf;
    audio20_desc_clock_source_t epin_clk;
    audio20_desc_input_terminal_t epin_input;
    audio20_desc_feature_unit_n_t(1) epin_fu;
    audio20_desc_output_terminal_t epin_output;
    tusb_desc_endpoint_t int_ep;
    tusb_desc_interface_t epin_alt0_itf;
    tusb_desc_interface_t epin_alt1_itf;
    audio20_desc_cs_as_interface_t epin_cs_as;
    audio20_desc_type_I_format_t epin_type_i_format;
    tusb_desc_endpoint_t epin_std_iso_ep;
    audio20_desc_cs_as_iso_data_ep_t epin_cs_as_iso_data_ep;
} audio_mono_epin_desc_t;

typedef struct TU_ATTR_PACKED audio_stereo_epin_desc {
    tusb_desc_interface_assoc_t itf_assoc;
    tusb_desc_interface_t itf;
    audio20_desc_cs_ac_interface_t cs_ac_itf;
    audio20_desc_clock_source_t epin_clk;
    audio20_desc_input_terminal_t epin_input;
    audio20_desc_feature_unit_n_t(2) epin_fu;
    audio20_desc_output_terminal_t epin_output;
    tusb_desc_endpoint_t int_ep;
    tusb_desc_interface_t epin_alt0_itf;
    tusb_desc_interface_t epin_alt1_itf;
    audio20_desc_cs_as_interface_t epin_cs_as;
    audio20_desc_type_I_format_t epin_type_i_format;
    tusb_desc_endpoint_t epin_std_iso_ep;
    audio20_desc_cs_as_iso_data_ep_t epin_cs_as_iso_data_ep;
} audio_stereo_epin_desc_t;

typedef struct TU_ATTR_PACKED hid_desc {
    tusb_desc_interface_t itf;
    tusb_hid_descriptor_hid_t hid_desc;
    tusb_desc_endpoint_t ep;
} hid_desc_t;

typedef struct TU_ATTR_PACKED mono_epout_mono_epin_device_configuration {
    tusb_desc_configuration_t config_descriptor;
    cdc_desc_t cdc_desc;
    audio_mono_epout_mono_epin_desc_t audio_desc;
    hid_desc_t hid_desc;
} mono_epout_mono_epin_device_configuration_t;

typedef struct TU_ATTR_PACKED mono_epout_stereo_epin_device_configuration {
    tusb_desc_configuration_t config_descriptor;
    cdc_desc_t cdc_desc;
    audio_mono_epout_stereo_epin_desc_t audio_desc;
    hid_desc_t hid_desc;
} mono_epout_stereo_epin_device_configuration_t;

typedef struct TU_ATTR_PACKED stereo_epout_mono_epin_device_configuration {
    tusb_desc_configuration_t config_descriptor;
    cdc_desc_t cdc_desc;
    audio_stereo_epout_mono_epin_desc_t audio_desc;
    hid_desc_t hid_desc;
} stereo_epout_mono_epin_device_configuration_t;

typedef struct TU_ATTR_PACKED stereo_epout_stereo_epin_device_configuration {
    tusb_desc_configuration_t config_descriptor;
    cdc_desc_t cdc_desc;
    audio_stereo_epout_stereo_epin_desc_t audio_desc;
    hid_desc_t hid_desc;
} stereo_epout_stereo_epin_device_configuration_t;

typedef struct TU_ATTR_PACKED mono_epout_device_configuration {
    tusb_desc_configuration_t config_descriptor;
    cdc_desc_t cdc_desc;
    audio_mono_epout_desc_t audio_desc;
    hid_desc_t hid_desc;
} mono_epout_device_configuration_t;

typedef struct TU_ATTR_PACKED stereo_epout_device_configuration {
    tusb_desc_configuration_t config_descriptor;
    cdc_desc_t cdc_desc;
    audio_stereo_epout_desc_t audio_desc;
    hid_desc_t hid_desc;
} stereo_epout_device_configuration_t;

typedef struct TU_ATTR_PACKED mono_epin_device_configuration {
    tusb_desc_configuration_t config_descriptor;
    cdc_desc_t cdc_desc;
    audio_mono_epin_desc_t audio_desc;
    hid_desc_t hid_desc;
} mono_epin_device_configuration_t;

typedef struct TU_ATTR_PACKED stereo_epin_device_configuration {
    tusb_desc_configuration_t config_descriptor;
    cdc_desc_t cdc_desc;
    audio_stereo_epin_desc_t audio_desc;
    hid_desc_t hid_desc;
} stereo_epin_device_configuration_t;

/** @brief High speed device configutation.
 */
/* Per USB specs: high speed capable device must report device_qualifier and other_speed_configuration */
uint8_t mono_epout_mono_epin_desc_configuration[] = {
    /* Config number, interface count, string index, total length, attribute, power in mA */
    TUD_CONFIG_DESCRIPTOR(1, ITF_EPOUT_EPIN_NUM_TOTAL, 0, MONO_EPOUT_MONO_EPIN_CONFIG_TOTAL_LEN, 0x00, 100),
    /* CDC: Interface number, string index, EP notification address and size, EP data address (out, in) and size. */
    TUD_CDC_DESCRIPTOR(ITF_EPOUT_EPIN_NUM_CDC_0, STRID_CDC_IFACE, EPNUM_CDC_0_NOTIF, 8, EPNUM_CDC_0_OUT, EPNUM_CDC_0_IN,
                       (TUD_OPT_HIGH_SPEED ? 512 : 64)),
    /* String index, EP Out, EP In, EPInt address. */
    TUD_AUDIO_MONO_EPOUT_MONO_EPIN_DESCRIPTOR(STRID_PRODUCT, EPNUM_AUDIO_OUT, EPNUM_AUDIO_IN, EPNUM_AUDIO_INT),
    /* HID: Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval. */
    TUD_HID_DESCRIPTOR(ITF_EPOUT_EPIN_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report1), EPNUM_HID,
                       CFG_TUD_HID_EP_BUFSIZE, 2),
};
/* Per USB specs: high speed capable device must report device_qualifier and other_speed_configuration */
uint8_t mono_epout_stereo_epin_desc_configuration[] = {
    /* Config number, interface count, string index, total length, attribute, power in mA */
    TUD_CONFIG_DESCRIPTOR(1, ITF_EPOUT_EPIN_NUM_TOTAL, 0, STEREO_EPOUT_MONO_EPIN_CONFIG_TOTAL_LEN, 0x00, 100),
    /* CDC: Interface number, string index, EP notification address and size, EP data address (out, in) and size. */
    TUD_CDC_DESCRIPTOR(ITF_EPOUT_EPIN_NUM_CDC_0, STRID_CDC_IFACE, EPNUM_CDC_0_NOTIF, 8, EPNUM_CDC_0_OUT, EPNUM_CDC_0_IN,
                       (TUD_OPT_HIGH_SPEED ? 512 : 64)),
    /* String index, EP Out, EP In, EPInt address. */
    TUD_AUDIO_MONO_EPOUT_STEREO_EPIN_DESCRIPTOR(STRID_PRODUCT, EPNUM_AUDIO_OUT, EPNUM_AUDIO_IN, EPNUM_AUDIO_INT),
    /* HID: Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval. */
    TUD_HID_DESCRIPTOR(ITF_EPOUT_EPIN_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report1), EPNUM_HID,
                       CFG_TUD_HID_EP_BUFSIZE, 2),
};
uint8_t stereo_epout_mono_epin_desc_configuration[] = {
    /* Config number, interface count, string index, total length, attribute, power in mA */
    TUD_CONFIG_DESCRIPTOR(1, ITF_EPOUT_EPIN_NUM_TOTAL, 0, STEREO_EPOUT_MONO_EPIN_CONFIG_TOTAL_LEN, 0x00, 100),
    /* CDC: Interface number, string index, EP notification address and size, EP data address (out, in) and size. */
    TUD_CDC_DESCRIPTOR(ITF_EPOUT_EPIN_NUM_CDC_0, STRID_CDC_IFACE, EPNUM_CDC_0_NOTIF, 8, EPNUM_CDC_0_OUT, EPNUM_CDC_0_IN,
                       (TUD_OPT_HIGH_SPEED ? 512 : 64)),
    /* String index, EP Out, EP In, EPInt address. */
    TUD_AUDIO_STEREO_EPOUT_MONO_EPIN_DESCRIPTOR(STRID_PRODUCT, EPNUM_AUDIO_OUT, EPNUM_AUDIO_IN, EPNUM_AUDIO_INT),
    /* HID: Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval. */
    TUD_HID_DESCRIPTOR(ITF_EPOUT_EPIN_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report1), EPNUM_HID,
                       CFG_TUD_HID_EP_BUFSIZE, 2),
};
uint8_t stereo_epout_stereo_epin_desc_configuration[] = {
    /* Config number, interface count, string index, total length, attribute, power in mA */
    TUD_CONFIG_DESCRIPTOR(1, ITF_EPOUT_EPIN_NUM_TOTAL, 0, STEREO_EPOUT_STEREO_EPIN_CONFIG_TOTAL_LEN, 0x00, 100),
    /* CDC: Interface number, string index, EP notification address and size, EP data address (out, in) and size. */
    TUD_CDC_DESCRIPTOR(ITF_EPOUT_EPIN_NUM_CDC_0, STRID_CDC_IFACE, EPNUM_CDC_0_NOTIF, 8, EPNUM_CDC_0_OUT, EPNUM_CDC_0_IN,
                       (TUD_OPT_HIGH_SPEED ? 512 : 64)),
    /* String index, EP Out, EP In, EPInt address. */
    TUD_AUDIO_STEREO_EPOUT_STEREO_EPIN_DESCRIPTOR(STRID_PRODUCT, EPNUM_AUDIO_OUT, EPNUM_AUDIO_IN, EPNUM_AUDIO_INT),
    /* HID: Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval. */
    TUD_HID_DESCRIPTOR(ITF_EPOUT_EPIN_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report1), EPNUM_HID,
                       CFG_TUD_HID_EP_BUFSIZE, 2),
};
uint8_t mono_epout_desc_configuration[] = {
    /* Config number, interface count, string index, total length, attribute, power in mA */
    TUD_CONFIG_DESCRIPTOR(1, ITF_EPOUT_NUM_TOTAL, 0, STEREO_EPOUT_CONFIG_TOTAL_LEN, 0x00, 100),
    /* CDC: Interface number, string index, EP notification address and size, EP data address (out, in) and size. */
    TUD_CDC_DESCRIPTOR(ITF_EPOUT_NUM_CDC_0, STRID_CDC_IFACE, EPNUM_CDC_0_NOTIF, 8, EPNUM_CDC_0_OUT, EPNUM_CDC_0_IN,
                       (TUD_OPT_HIGH_SPEED ? 512 : 64)),
    /* String index, EP Out, EPInt address. */
    TUD_AUDIO_MONO_EPOUT_DESCRIPTOR(STRID_PRODUCT, EPNUM_AUDIO_OUT, EPNUM_AUDIO_INT),
    /* HID: Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval. */
    TUD_HID_DESCRIPTOR(ITF_EPOUT_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report1), EPNUM_HID,
                       CFG_TUD_HID_EP_BUFSIZE, 2),
};
uint8_t stereo_epout_desc_configuration[] = {
    /* Config number, interface count, string index, total length, attribute, power in mA */
    TUD_CONFIG_DESCRIPTOR(1, ITF_EPOUT_NUM_TOTAL, 0, STEREO_EPOUT_CONFIG_TOTAL_LEN, 0x00, 100),
    /* CDC: Interface number, string index, EP notification address and size, EP data address (out, in) and size. */
    TUD_CDC_DESCRIPTOR(ITF_EPOUT_NUM_CDC_0, STRID_CDC_IFACE, EPNUM_CDC_0_NOTIF, 8, EPNUM_CDC_0_OUT, EPNUM_CDC_0_IN,
                       (TUD_OPT_HIGH_SPEED ? 512 : 64)),
    /* String index, EP Out, EPInt address. */
    TUD_AUDIO_STEREO_EPOUT_DESCRIPTOR(STRID_PRODUCT, EPNUM_AUDIO_OUT, EPNUM_AUDIO_INT),
    /* HID: Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval. */
    TUD_HID_DESCRIPTOR(ITF_EPOUT_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report1), EPNUM_HID,
                       CFG_TUD_HID_EP_BUFSIZE, 2),
};
uint8_t mono_epin_desc_configuration[] = {
    /* Config number, interface count, string index, total length, attribute, power in mA */
    TUD_CONFIG_DESCRIPTOR(1, ITF_EPIN_NUM_TOTAL, 0, MONO_EPIN_CONFIG_TOTAL_LEN, 0x00, 100),
    /* CDC: Interface number, string index, EP notification address and size, EP data address (out, in) and size. */
    TUD_CDC_DESCRIPTOR(ITF_EPOUT_NUM_CDC_0, STRID_CDC_IFACE, EPNUM_CDC_0_NOTIF, 8, EPNUM_CDC_0_OUT, EPNUM_CDC_0_IN,
                       (TUD_OPT_HIGH_SPEED ? 512 : 64)),
    /* String index, EP Out, EPInt address. */
    TUD_AUDIO_MONO_EPIN_DESCRIPTOR(STRID_PRODUCT, EPNUM_AUDIO_IN, EPNUM_AUDIO_INT),
    /* HID: Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval. */
    TUD_HID_DESCRIPTOR(ITF_EPOUT_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report1), EPNUM_HID,
                       CFG_TUD_HID_EP_BUFSIZE, 2),
};
uint8_t stereo_epin_desc_configuration[] = {
    /* Config number, interface count, string index, total length, attribute, power in mA */
    TUD_CONFIG_DESCRIPTOR(1, ITF_EPIN_NUM_TOTAL, 0, STEREO_EPIN_CONFIG_TOTAL_LEN, 0x00, 100),
    /* CDC: Interface number, string index, EP notification address and size, EP data address (out, in) and size. */
    TUD_CDC_DESCRIPTOR(ITF_EPOUT_NUM_CDC_0, STRID_CDC_IFACE, EPNUM_CDC_0_NOTIF, 8, EPNUM_CDC_0_OUT, EPNUM_CDC_0_IN,
                       (TUD_OPT_HIGH_SPEED ? 512 : 64)),
    /* String index, EP Out, EPInt address. */
    TUD_AUDIO_STEREO_EPIN_DESCRIPTOR(STRID_PRODUCT, EPNUM_AUDIO_IN, EPNUM_AUDIO_INT),
    /* HID: Interface number, string index, protocol, report descriptor len, EP In address, size & polling interval. */
    TUD_HID_DESCRIPTOR(ITF_EPOUT_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report1), EPNUM_HID,
                       CFG_TUD_HID_EP_BUFSIZE, 2),
};

/* device qualifier is mostly similar to device descriptor since we don't change configuration based on speed */
tusb_desc_device_qualifier_t const desc_device_qualifier = {
    .bLength = sizeof(tusb_desc_device_qualifier_t),
    .bDescriptorType = TUSB_DESC_DEVICE_QUALIFIER,
    .bcdUSB = USB_BCD,
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .bNumConfigurations = 0x01,
    .bReserved = 0x00,
};

/* array of pointer to string descriptors */
// clang-format off
char const *string_desc_arr[] = {
    /* 0: is supported language is English (0x0409). */
    (const char[]) { 0x09, 0x04 },
    /* 1: Manufacturer */
    "SPARK Microsystems Inc.",
    /* 2: Product */
    TUSB_PRODUCT_NAME,
    /* 3: Serials will use unique ID if possible */
    (char[]){'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', '\000'},
    /* 4: CDC Interface */
    "TinyUSB CDC",
    /* 5: Audio Interface */
    "TinyUSB Speakers",
    /* 6: Audio Interface */
    "TinyUSB Microphone",
};
// clang-format on

static uint16_t _desc_str[32 + 1];

/* PUBLIC FUNCTIONS ***********************************************************/
char *tusb_get_string_descriptor(uint8_t index)
{
    return (char *)string_desc_arr[index];
}

/* Invoked when received GET DEVICE DESCRIPTOR
 * Application return pointer to descriptor
 */
uint8_t const *tud_descriptor_device_cb(void)
{
    return (uint8_t const *)&desc_device;
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t itf)
{
    if (itf == 0) {
        return desc_hid_report1;
    }

    return NULL;
}
/* Invoked when received GET DEVICE QUALIFIER DESCRIPTOR request
 * Application return pointer to descriptor, whose contents must exist long enough for transfer to complete.
 * device_qualifier descriptor describes information about a high-speed capable device that would
 * change if the device were operating at the other speed. If not highspeed capable stall this request.
 */
uint8_t const *tud_descriptor_device_qualifier_cb(void)
{
    return (uint8_t const *)&desc_device_qualifier;
}

/*! Limited config for FULL SPEED. */
uint8_t const desc_fs_configuration[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    // TUSB_DESC_CONFIGURATION
    9,
    TUSB_DESC_OTHER_SPEED_CONFIG,
    U16_TO_U8S_LE(TUD_CONFIG_DESC_LEN + CFG_TUD_CDC * TUD_CDC_DESC_LEN),
    2,
    1,
    0,
    TU_BIT(7) | 0x00,
    (100) / 2,
    /* CDC: Interface number, string index, EP notification address and size, EP data address (out, in) and size. */
    TUD_CDC_DESCRIPTOR(ITF_EPOUT_EPIN_NUM_CDC_0, STRID_CDC_IFACE, EPNUM_CDC_0_NOTIF, 8, EPNUM_CDC_0_OUT, EPNUM_CDC_0_IN,
                       64),
};

/* Invoked when received GET OTHER SEED CONFIGURATION DESCRIPTOR request
 * Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
 * Configuration descriptor in the other speed e.g if high speed then this is for full speed and vice versa
 */
uint8_t const *tud_descriptor_other_speed_configuration_cb(uint8_t index)
{
    (void)index; /* for multiple configurations */

    if (is_ep_in_en && is_ep_out_en) {
        if (IS_EPOUT_STEREO && IS_EPIN_MONO) {
            return stereo_epout_mono_epin_desc_configuration;
        } else if (IS_EPOUT_MONO && IS_EPIN_STEREO) {
            return mono_epout_stereo_epin_desc_configuration;
        } else if (IS_EPOUT_MONO && IS_EPIN_MONO) {
            return mono_epout_mono_epin_desc_configuration;
        } else if (IS_EPOUT_STEREO && IS_EPIN_STEREO) {
            return stereo_epout_stereo_epin_desc_configuration;
        }
    } else if (is_ep_out_en) {
        if (IS_EPOUT_MONO) {
            return mono_epout_desc_configuration;
        } else if (IS_EPOUT_STEREO) {
            return stereo_epout_desc_configuration;
        }
    } else if (is_ep_in_en) {
        if (IS_EPIN_MONO) {
            return mono_epin_desc_configuration;
        } else if (IS_EPIN_STEREO) {
            return stereo_epin_desc_configuration;
        }
    }
    return NULL;
}

uint16_t tud_audio_get_func_1_desc_len(void)
{
    if (is_ep_in_en && is_ep_out_en) {
        if (IS_EPOUT_STEREO && IS_EPIN_MONO) {
            return TUD_AUDIO_STEREO_EPOUT_MONO_EPIN_DESC_LEN;
        } else if (IS_EPOUT_MONO && IS_EPIN_STEREO) {
            return TUD_AUDIO_MONO_EPOUT_STEREO_EPIN_DESC_LEN;
        } else if (IS_EPOUT_MONO && IS_EPIN_MONO) {
            return TUD_AUDIO_MONO_EPOUT_MONO_EPIN_DESC_LEN;
        } else if (IS_EPOUT_STEREO && IS_EPIN_STEREO) {
            return TUD_AUDIO_STEREO_EPOUT_STEREO_EPIN_DESC_LEN;
        }
    } else if (is_ep_out_en) {
        if (IS_EPOUT_MONO) {
            return TUD_AUDIO_MONO_EPOUT_DESC_LEN;
        } else if (IS_EPOUT_STEREO) {
            return TUD_AUDIO_STEREO_EPOUT_DESC_LEN;
        }
    } else if (is_ep_in_en) {
        if (IS_EPIN_MONO) {
            return TUD_AUDIO_MONO_EPIN_DESC_LEN;
        } else if (IS_EPIN_STEREO) {
            return TUD_AUDIO_STEREO_EPIN_DESC_LEN;
        }
    }

    return 0;
}

/* Invoked when received GET CONFIGURATION DESCRIPTOR
 * Application return pointer to descriptor
 * Descriptor contents must exist long enough for transfer to complete
 */
uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index; /* for multiple configurations */

    tusb_desc_endpoint_t *epin_std_iso_ep = NULL;
    audio20_desc_type_I_format_t *epin_type_i_format = NULL;
    tusb_desc_endpoint_t *epout_std_iso_ep = NULL;
    audio20_desc_type_I_format_t *epout_type_i_format = NULL;

    uint8_t *descriptor = NULL;

    if (is_ep_in_en && is_ep_out_en) {
        /* **** Bidirectional configuration **** */
        if (IS_EPOUT_STEREO && IS_EPIN_MONO) {
            /* Validator descriptor configuration matches defined sizes. */
            TU_ASSERT(sizeof(stereo_epout_mono_epin_desc_configuration) == STEREO_EPOUT_MONO_EPIN_CONFIG_TOTAL_LEN);
            TU_ASSERT(sizeof(stereo_epout_mono_epin_device_configuration_t) == STEREO_EPOUT_MONO_EPIN_CONFIG_TOTAL_LEN);

            /* Save descriptor pointer. */
            descriptor = stereo_epout_mono_epin_desc_configuration;

            stereo_epout_mono_epin_device_configuration_t *dev_cfg =
                (stereo_epout_mono_epin_device_configuration_t *)descriptor;

            /* Save ep in config pointers. */
            epin_std_iso_ep = &dev_cfg->audio_desc.epin_std_iso_ep;
            epin_type_i_format = &dev_cfg->audio_desc.epin_type_i_format;

            /* Save ep out config pointers. */
            epout_std_iso_ep = &dev_cfg->audio_desc.epout_std_iso_ep;
            epout_type_i_format = &dev_cfg->audio_desc.epout_type_i_format;
        } else if (IS_EPOUT_MONO && IS_EPIN_STEREO) {
            /* Validator descriptor configuration matches defined sizes. */
            TU_ASSERT(sizeof(mono_epout_stereo_epin_desc_configuration) == MONO_EPOUT_STEREO_EPIN_CONFIG_TOTAL_LEN);
            TU_ASSERT(sizeof(mono_epout_stereo_epin_device_configuration_t) == MONO_EPOUT_STEREO_EPIN_CONFIG_TOTAL_LEN);

            /* Save descriptor pointer. */
            descriptor = mono_epout_stereo_epin_desc_configuration;

            mono_epout_stereo_epin_device_configuration_t *dev_cfg =
                (mono_epout_stereo_epin_device_configuration_t *)descriptor;

            /* Save ep in config pointers. */
            epin_std_iso_ep = &dev_cfg->audio_desc.epin_std_iso_ep;
            epin_type_i_format = &dev_cfg->audio_desc.epin_type_i_format;

            /* Save ep out config pointers. */
            epout_std_iso_ep = &dev_cfg->audio_desc.epout_std_iso_ep;
            epout_type_i_format = &dev_cfg->audio_desc.epout_type_i_format;
        } else if (IS_EPOUT_MONO && IS_EPIN_MONO) {
            /* Validator descriptor configuration matches defined sizes. */
            TU_ASSERT(sizeof(mono_epout_mono_epin_desc_configuration) == MONO_EPOUT_MONO_EPIN_CONFIG_TOTAL_LEN);
            TU_ASSERT(sizeof(mono_epout_mono_epin_device_configuration_t) == MONO_EPOUT_MONO_EPIN_CONFIG_TOTAL_LEN);

            /* Save descriptor pointer. */
            descriptor = mono_epout_mono_epin_desc_configuration;

            mono_epout_mono_epin_device_configuration_t *dev_cfg =
                (mono_epout_mono_epin_device_configuration_t *)descriptor;

            /* Save ep in config pointers. */
            epin_std_iso_ep = &dev_cfg->audio_desc.epin_std_iso_ep;
            epin_type_i_format = &dev_cfg->audio_desc.epin_type_i_format;

            /* Save ep out config pointers. */
            epout_std_iso_ep = &dev_cfg->audio_desc.epout_std_iso_ep;
            epout_type_i_format = &dev_cfg->audio_desc.epout_type_i_format;
        } else if (IS_EPOUT_STEREO && IS_EPIN_STEREO) {
            /* Validator descriptor configuration matches defined sizes. */
            TU_ASSERT(sizeof(stereo_epout_stereo_epin_desc_configuration) == STEREO_EPOUT_STEREO_EPIN_CONFIG_TOTAL_LEN);
            TU_ASSERT(sizeof(stereo_epout_stereo_epin_device_configuration_t) ==
                      STEREO_EPOUT_STEREO_EPIN_CONFIG_TOTAL_LEN);

            /* Save descriptor pointer. */
            descriptor = stereo_epout_stereo_epin_desc_configuration;

            stereo_epout_stereo_epin_device_configuration_t *dev_cfg =
                (stereo_epout_stereo_epin_device_configuration_t *)descriptor;

            /* Save ep in config pointers. */
            epin_std_iso_ep = &dev_cfg->audio_desc.epin_std_iso_ep;
            epin_type_i_format = &dev_cfg->audio_desc.epin_type_i_format;

            /* Save ep out config pointers. */
            epout_std_iso_ep = &dev_cfg->audio_desc.epout_std_iso_ep;
            epout_type_i_format = &dev_cfg->audio_desc.epout_type_i_format;
        }
    } else if (is_ep_out_en) {
        /* **** Unidirectional EPOUT configuration **** */
        if (IS_EPOUT_MONO) {
            /* Validator descriptor configuration matches defined sizes. */
            TU_ASSERT(sizeof(mono_epout_desc_configuration) == MONO_EPOUT_CONFIG_TOTAL_LEN);
            TU_ASSERT(sizeof(mono_epout_device_configuration_t) == MONO_EPOUT_CONFIG_TOTAL_LEN);

            /* Save descriptor pointer. */
            descriptor = mono_epout_desc_configuration;

            stereo_epout_device_configuration_t *dev_cfg = (stereo_epout_device_configuration_t *)descriptor;

            /* Save ep out config pointers. */
            epout_std_iso_ep = &dev_cfg->audio_desc.epout_std_iso_ep;
            epout_type_i_format = &dev_cfg->audio_desc.epout_type_i_format;
        } else if (IS_EPOUT_STEREO) {
            /* Validator descriptor configuration matches defined sizes. */
            TU_ASSERT(sizeof(stereo_epout_desc_configuration) == STEREO_EPOUT_CONFIG_TOTAL_LEN);
            TU_ASSERT(sizeof(stereo_epout_device_configuration_t) == STEREO_EPOUT_CONFIG_TOTAL_LEN);

            /* Save descriptor pointer. */
            descriptor = stereo_epout_desc_configuration;

            stereo_epout_device_configuration_t *dev_cfg = (stereo_epout_device_configuration_t *)descriptor;

            /* Save ep out config pointers. */
            epout_std_iso_ep = &dev_cfg->audio_desc.epout_std_iso_ep;
            epout_type_i_format = &dev_cfg->audio_desc.epout_type_i_format;
        }
    } else if (is_ep_in_en) {
        /* **** Unidirectional EPIN configuration **** */
        if (IS_EPIN_MONO) {
            /* Validator descriptor configuration matches defined sizes. */
            TU_ASSERT(sizeof(mono_epin_desc_configuration) == MONO_EPIN_CONFIG_TOTAL_LEN);
            TU_ASSERT(sizeof(mono_epin_device_configuration_t) == MONO_EPIN_CONFIG_TOTAL_LEN);

            /* Save descriptor pointer. */
            descriptor = mono_epin_desc_configuration;

            mono_epin_device_configuration_t *dev_cfg = (mono_epin_device_configuration_t *)descriptor;

            /* Save ep in config pointers. */
            epin_std_iso_ep = &dev_cfg->audio_desc.epin_std_iso_ep;
            epin_type_i_format = &dev_cfg->audio_desc.epin_type_i_format;
        } else if (IS_EPIN_STEREO) {
            /* Validator descriptor configuration matches defined sizes. */
            TU_ASSERT(sizeof(stereo_epin_desc_configuration) == STEREO_EPIN_CONFIG_TOTAL_LEN);
            TU_ASSERT(sizeof(stereo_epin_device_configuration_t) == STEREO_EPIN_CONFIG_TOTAL_LEN);

            /* Save descriptor pointer. */
            descriptor = stereo_epin_desc_configuration;

            stereo_epin_device_configuration_t *dev_cfg = (stereo_epin_device_configuration_t *)descriptor;

            /* Save ep in config pointers. */
            epin_std_iso_ep = &dev_cfg->audio_desc.epin_std_iso_ep;
            epin_type_i_format = &dev_cfg->audio_desc.epin_type_i_format;
        }
    }

    /* Configure ep in. */
    if ((epin_std_iso_ep != NULL) && (epin_type_i_format != NULL)) {
        epin_std_iso_ep->wMaxPacketSize = (1 << (UAC2_INTERVAL_IN - 1)) *
                                          TUD_AUDIO_EP_SIZE(TUD_OPT_HIGH_SPEED, ep_in_cfg.sampling_rate,
                                                            ep_in_cfg.bytes_per_sample, ep_in_cfg.nb_ch);
        epin_type_i_format->bSubslotSize = ep_in_cfg.bytes_per_sample;
        epin_type_i_format->bBitResolution = ep_in_cfg.resolution;
    }

    /* Configure ep out. */
    if ((epout_std_iso_ep != NULL) && (epout_type_i_format != NULL)) {
        epout_std_iso_ep->wMaxPacketSize = (1 << (UAC2_INTERVAL_OUT - 1)) *
                                           TUD_AUDIO_EP_SIZE(TUD_OPT_HIGH_SPEED, ep_out_cfg.sampling_rate,
                                                             ep_out_cfg.bytes_per_sample, ep_out_cfg.nb_ch);
        epout_type_i_format->bSubslotSize = ep_out_cfg.bytes_per_sample;
        epout_type_i_format->bBitResolution = ep_out_cfg.resolution;
    }

    return descriptor;
}

/* Invoked when received GET STRING DESCRIPTOR request */
/* Application return pointer to descriptor, whose contents must exist long enough for transfer to complete */
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;
    const char *str = string_desc_arr[index];
    uint8_t chr_count;

    switch (index) {
    case STRID_LANGID:
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
        break;
    case STRID_SERIAL:
        chr_count = tusb_get_serial_id((uint8_t *)_desc_str + 2, 32);
        break;
    default:
        /*
         * Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
         * https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors
         */
        if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))) {
            return NULL;
        }

        /* Cap at max char. */
        chr_count = (uint8_t)strlen(str);
        if (chr_count > 31) {
            chr_count = 31;
        }

        /* Convert ASCII string into UTF-16. */
        for (uint8_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
        break;
    }

    /* First byte is length (including header), second byte is string type. */
    _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));

    return _desc_str;
}

// Helper for clock get requests
static bool tud_audio_clock_get_request(uint8_t rhport, audio20_control_request_t const *request)
{
    if (request->bEntityID == UAC2_ENTITY_EPOUT_CLOCK) {
#if CFG_TUD_AUDIO_ENABLE_EP_OUT
        if (request->bControlSelector == AUDIO20_CS_CTRL_SAM_FREQ) {
            if (request->bRequest == AUDIO20_CS_REQ_CUR) {
                TU_LOG1("Clock get current freq %" PRIu32 "\r\n", ep_out_cfg.sampling_rate);

                audio20_control_cur_4_t curf = {(int32_t)tu_htole32(ep_out_cfg.sampling_rate)};
                return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request,
                                                                  &curf, sizeof(curf));
            } else if (request->bRequest == AUDIO20_CS_REQ_RANGE) {
                audio20_control_range_4_n_t(SPK_N_SAMPLE_RATES)
                    rangef = {.wNumSubRanges = tu_htole16(SPK_N_SAMPLE_RATES)};
                TU_LOG1("Clock get %d freq ranges\r\n", SPK_N_SAMPLE_RATES);
                for (uint8_t i = 0; i < SPK_N_SAMPLE_RATES; i++) {
                    rangef.subrange[i].bMin = (int32_t)ep_out_cfg.sampling_rate;
                    rangef.subrange[i].bMax = (int32_t)ep_out_cfg.sampling_rate;
                    rangef.subrange[i].bRes = 0;
                    TU_LOG1("Range %d (%d, %d, %d)\r\n", i, (int)rangef.subrange[i].bMin, (int)rangef.subrange[i].bMax,
                            (int)rangef.subrange[i].bRes);
                }

                return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request,
                                                                  &rangef, sizeof(rangef));
            }
        } else if (request->bControlSelector == AUDIO20_CS_CTRL_CLK_VALID && request->bRequest == AUDIO20_CS_REQ_CUR) {
            audio20_control_cur_1_t cur_valid = {.bCur = 1};
            TU_LOG1("Clock get is valid %u\r\n", cur_valid.bCur);
            return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request,
                                                              &cur_valid, sizeof(cur_valid));
        }
        TU_LOG1("Clock get request not supported, entity = %u, selector = %u, request = %u\r\n", request->bEntityID,
                request->bControlSelector, request->bRequest);
        return false;
#endif
    } else if (request->bEntityID == UAC2_ENTITY_EPIN_CLOCK) {
#if CFG_TUD_AUDIO_ENABLE_EP_IN
        if (request->bControlSelector == AUDIO20_CS_CTRL_SAM_FREQ) {
            if (request->bRequest == AUDIO20_CS_REQ_CUR) {
                TU_LOG1("MIC Clock get current freq %" PRIu32 "\r\n", ep_in_cfg.sampling_rate);

                audio20_control_cur_4_t curf = {(int32_t)tu_htole32(ep_in_cfg.sampling_rate)};
                return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request,
                                                                  &curf, sizeof(curf));
            } else if (request->bRequest == AUDIO20_CS_REQ_RANGE) {
                audio20_control_range_4_n_t(MIC_N_SAMPLE_RATES)
                    rangef = {.wNumSubRanges = tu_htole16(MIC_N_SAMPLE_RATES)};
                TU_LOG1("Clock get %d freq ranges\r\n", MIC_N_SAMPLE_RATES);
                for (uint8_t i = 0; i < MIC_N_SAMPLE_RATES; i++) {
                    rangef.subrange[i].bMin = (int32_t)ep_in_cfg.sampling_rate;
                    rangef.subrange[i].bMax = (int32_t)ep_in_cfg.sampling_rate;
                    rangef.subrange[i].bRes = 0;
                    TU_LOG1("Range %d (%d, %d, %d)\r\n", i, (int)rangef.subrange[i].bMin, (int)rangef.subrange[i].bMax,
                            (int)rangef.subrange[i].bRes);
                }

                return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request,
                                                                  &rangef, sizeof(rangef));
            }
        } else if (request->bControlSelector == AUDIO20_CS_CTRL_CLK_VALID && request->bRequest == AUDIO20_CS_REQ_CUR) {
            audio20_control_cur_1_t cur_valid = {.bCur = 1};
            TU_LOG1("MIC Clock get is valid %u\r\n", cur_valid.bCur);
            return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request,
                                                              &cur_valid, sizeof(cur_valid));
        }
        TU_LOG1("MIC Clock get request not supported, entity = %u, selector = %u, request = %u\r\n", request->bEntityID,
                request->bControlSelector, request->bRequest);
        return false;
#endif
    }
    return false;
}

// Helper for clock set requests
static bool tud_audio_clock_set_request(uint8_t rhport, audio20_control_request_t const *request, uint8_t const *buf)
{
    (void)rhport;

    if (request->bEntityID == UAC2_ENTITY_EPOUT_CLOCK) {
#if CFG_TUD_AUDIO_ENABLE_EP_OUT
        TU_VERIFY(request->bRequest == AUDIO20_CS_REQ_CUR);

        if (request->bControlSelector == AUDIO20_CS_CTRL_SAM_FREQ) {
            TU_VERIFY(request->wLength == sizeof(audio20_control_cur_4_t));
            if (ep_out_cfg.sampling_rate == (uint32_t)((audio20_control_cur_4_t const *)buf)->bCur) {
                TU_LOG1("Clock set current freq: %" PRIu32 "\r\n", ep_out_cfg.sampling_rate);
                return true;
            } else {
                TU_LOG1("Clock set request not supported, entity = %u, selector = %u, request = %u\r\n",
                        request->bEntityID, request->bControlSelector, request->bRequest);
                return false;
            }
        } else {
            TU_LOG1("Clock set request not supported, entity = %u, selector = %u, request = %u\r\n", request->bEntityID,
                    request->bControlSelector, request->bRequest);
            return false;
        }
#endif
    } else if (request->bEntityID == UAC2_ENTITY_EPIN_CLOCK) {
#if CFG_TUD_AUDIO_ENABLE_EP_IN
        TU_VERIFY(request->bRequest == AUDIO20_CS_REQ_CUR);

        if (request->bControlSelector == AUDIO20_CS_CTRL_SAM_FREQ) {
            TU_VERIFY(request->wLength == sizeof(audio20_control_cur_4_t));
            if (ep_in_cfg.sampling_rate == (uint32_t)((audio20_control_cur_4_t const *)buf)->bCur) {
                TU_LOG1("Clock set current freq: %" PRIu32 "\r\n", ep_in_cfg.sampling_rate);
                return true;
            } else {
                TU_LOG1("Clock set request not supported, entity = %u, selector = %u, request = %u\r\n",
                        request->bEntityID, request->bControlSelector, request->bRequest);
                return false;
            }
        } else {
            TU_LOG1("MIC Clock set request not supported, entity = %u, selector = %u, request = %u\r\n",
                    request->bEntityID, request->bControlSelector, request->bRequest);
            return false;
        }
#endif
    }
    return false;
}

// Helper for feature unit get requests
static bool tud_audio_feature_unit_get_request(uint8_t rhport, audio20_control_request_t const *request)
{
    if (request->bEntityID == UAC2_ENTITY_EPOUT_FEATURE_UNIT) {
#if CFG_TUD_AUDIO_ENABLE_EP_OUT
        if (request->bControlSelector == AUDIO20_FU_CTRL_MUTE && request->bRequest == AUDIO20_CS_REQ_CUR) {
            audio20_control_cur_1_t mute1 = {.bCur = epout_mute[request->bChannelNumber]};
            TU_LOG1("Get channel %u epout_mute %d\r\n", request->bChannelNumber, mute1.bCur);
            return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &mute1,
                                                              sizeof(mute1));
        } else if (request->bControlSelector == AUDIO20_FU_CTRL_VOLUME) {
            if (request->bRequest == AUDIO20_CS_REQ_RANGE) {
                audio20_control_range_2_n_t(1) range_vol = {.wNumSubRanges = tu_htole16(1),
                                                            .subrange[0] = {.bMin = tu_htole16(-VOLUME_CTRL_100_DB),
                                                                            tu_htole16(VOLUME_CTRL_0_DB),
                                                                            tu_htole16(256)}};
                TU_LOG1("Get channel %u epout_volume range (%d, %d, %u) dB\r\n", request->bChannelNumber,
                        range_vol.subrange[0].bMin / 256, range_vol.subrange[0].bMax / 256,
                        range_vol.subrange[0].bRes / 256);
                return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request,
                                                                  &range_vol, sizeof(range_vol));
            } else if (request->bRequest == AUDIO20_CS_REQ_CUR) {
                audio20_control_cur_2_t cur_vol = {.bCur = tu_htole16(epout_volume[request->bChannelNumber])};
                TU_LOG1("Get channel %u epout_volume %d dB\r\n", request->bChannelNumber, cur_vol.bCur / 256);
                return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request,
                                                                  &cur_vol, sizeof(cur_vol));
            }
        }
#endif
    } else if (request->bEntityID == UAC2_ENTITY_EPIN_FEATURE_UNIT) {
#if CFG_TUD_AUDIO_ENABLE_EP_IN
        if (request->bControlSelector == AUDIO20_FU_CTRL_MUTE && request->bRequest == AUDIO20_CS_REQ_CUR) {
            audio20_control_cur_1_t mute1 = {.bCur = epin_mute[request->bChannelNumber]};
            TU_LOG1("Get channel %u epin_mute %d\r\n", request->bChannelNumber, mute1.bCur);
            return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request, &mute1,
                                                              sizeof(mute1));
        } else if (request->bControlSelector == AUDIO20_FU_CTRL_VOLUME) {
            if (request->bRequest == AUDIO20_CS_REQ_RANGE) {
                audio20_control_range_2_n_t(1) range_vol = {.wNumSubRanges = tu_htole16(1),
                                                            .subrange[0] = {.bMin = tu_htole16(-VOLUME_CTRL_100_DB),
                                                                            tu_htole16(VOLUME_CTRL_0_DB),
                                                                            tu_htole16(256)}};
                TU_LOG1("Get channel %u epin_volume range (%d, %d, %u) dB\r\n", request->bChannelNumber,
                        range_vol.subrange[0].bMin / 256, range_vol.subrange[0].bMax / 256,
                        range_vol.subrange[0].bRes / 256);
                return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request,
                                                                  &range_vol, sizeof(range_vol));
            } else if (request->bRequest == AUDIO20_CS_REQ_CUR) {
                audio20_control_cur_2_t cur_vol = {.bCur = tu_htole16(epin_volume[request->bChannelNumber])};
                TU_LOG1("Get channel %u epin_volume %d dB\r\n", request->bChannelNumber, cur_vol.bCur / 256);
                return tud_audio_buffer_and_schedule_control_xfer(rhport, (tusb_control_request_t const *)request,
                                                                  &cur_vol, sizeof(cur_vol));
            }
        }
#endif
    }
    TU_LOG1("Feature unit get request not supported, rhport = %u, entity = %u, selector = %u, request = %u\r\n",
            request->bEntityID, rhport, request->bControlSelector, request->bRequest);

    return false;
}

// Helper for feature unit set requests
static bool tud_audio_feature_unit_set_request(uint8_t rhport, audio20_control_request_t const *request,
                                               uint8_t const *buf)
{
    (void)rhport;

    TU_VERIFY(request->bRequest == AUDIO20_CS_REQ_CUR);
    if (request->bEntityID == UAC2_ENTITY_EPOUT_FEATURE_UNIT) {
#if CFG_TUD_AUDIO_ENABLE_EP_OUT
        if (request->bControlSelector == AUDIO20_FU_CTRL_MUTE) {
            TU_VERIFY(request->wLength == sizeof(audio20_control_cur_1_t));

            epout_mute[request->bChannelNumber] = ((audio20_control_cur_1_t const *)buf)->bCur;

            TU_LOG1("Set channel %d Mute: %d\r\n", request->bChannelNumber, epout_mute[request->bChannelNumber]);

            return true;
        } else if (request->bControlSelector == AUDIO20_FU_CTRL_VOLUME) {
            TU_VERIFY(request->wLength == sizeof(audio20_control_cur_2_t));

            epout_volume[request->bChannelNumber] = ((audio20_control_cur_2_t const *)buf)->bCur;

            TU_LOG1("Set channel %d epout_volume: %d dB\r\n", request->bChannelNumber,
                    epout_volume[request->bChannelNumber] / 256);

            return true;
        }
#endif
    } else if (request->bEntityID == UAC2_ENTITY_EPIN_FEATURE_UNIT) {
#if CFG_TUD_AUDIO_ENABLE_EP_IN
        if (request->bControlSelector == AUDIO20_FU_CTRL_MUTE) {
            TU_VERIFY(request->wLength == sizeof(audio20_control_cur_1_t));

            epin_mute[request->bChannelNumber] = ((audio20_control_cur_1_t const *)buf)->bCur;

            TU_LOG1("Set channel %d Mute: %d\r\n", request->bChannelNumber, epin_mute[request->bChannelNumber]);

            return true;
        } else if (request->bControlSelector == AUDIO20_FU_CTRL_VOLUME) {
            TU_VERIFY(request->wLength == sizeof(audio20_control_cur_2_t));

            epin_volume[request->bChannelNumber] = ((audio20_control_cur_2_t const *)buf)->bCur;

            TU_LOG1("Set channel %d epin_volume: %d dB\r\n", request->bChannelNumber,
                    epin_volume[request->bChannelNumber] / 256);

            return true;
        }
#endif
    }

    TU_LOG1("Feature unit set request not supported, rhport = %u, entity = %u, selector = %u, request = %u\r\n",
            request->bEntityID, rhport, request->bControlSelector, request->bRequest);

    return false;
}

#define USB_AUDIO_VOLUME_TO_DB(vol) ((vol) / 256)
#if CFG_TUD_AUDIO_ENABLE_EP_OUT
int8_t tud_audio_get_epout_volume_db(void)
{
    if (epout_mute[1]) {
        return USB_AUDIO_VOLUME_TO_DB(VOLUME_CTRL_SILENCE);
    } else {
        return USB_AUDIO_VOLUME_TO_DB(epout_volume[1]);
    }
}
#endif
#if CFG_TUD_AUDIO_ENABLE_EP_IN
int8_t tud_audio_get_epin_volume_db(void)
{
    if (epin_mute[1]) {
        return USB_AUDIO_VOLUME_TO_DB(VOLUME_CTRL_SILENCE);
    } else {
        return USB_AUDIO_VOLUME_TO_DB(epin_volume[1]);
    }
}
#endif

//--------------------------------------------------------------------+
// Application Callback API Implementations
//--------------------------------------------------------------------+

// Invoked when audio class specific get request received for an entity
bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request)
{
    audio20_control_request_t const *request = (audio20_control_request_t const *)p_request;

    if ((request->bEntityID == UAC2_ENTITY_EPOUT_CLOCK) || (request->bEntityID == UAC2_ENTITY_EPIN_CLOCK))
        return tud_audio_clock_get_request(rhport, request);
    if ((request->bEntityID == UAC2_ENTITY_EPOUT_FEATURE_UNIT) || (request->bEntityID == UAC2_ENTITY_EPIN_FEATURE_UNIT))
        return tud_audio_feature_unit_get_request(rhport, request);
    else {
        TU_LOG1("Get request not handled, entity = %d, selector = %d, request = %d\r\n", request->bEntityID,
                request->bControlSelector, request->bRequest);
    }
    return false;
}

// Invoked when audio class specific set request received for an entity
bool tud_audio_set_req_entity_cb(uint8_t rhport, tusb_control_request_t const *p_request, uint8_t *buf)
{
    audio20_control_request_t const *request = (audio20_control_request_t const *)p_request;

    if ((request->bEntityID == UAC2_ENTITY_EPOUT_FEATURE_UNIT) || (request->bEntityID == UAC2_ENTITY_EPIN_FEATURE_UNIT))
        return tud_audio_feature_unit_set_request(rhport, request, buf);
    if ((request->bEntityID == UAC2_ENTITY_EPOUT_CLOCK) || (request->bEntityID == UAC2_ENTITY_EPIN_CLOCK))
        return tud_audio_clock_set_request(rhport, request, buf);
    TU_LOG1("Set request not handled, entity = %d, selector = %d, request = %d\r\n", request->bEntityID,
            request->bControlSelector, request->bRequest);

    return false;
}

void tud_audio_set_epout_cfg(tud_audio_config_t cfg)
{
    /* Validate audio configuration. */
    is_ep_out_en = !((cfg.sampling_rate == 0) || (cfg.resolution == 0) || (cfg.bytes_per_sample == 0) ||
                     (cfg.nb_ch == 0) || (cfg.nb_ch > CFG_TUD_AUDIO_FUNC_1_MAX_N_CHANNELS_OUT));

    if (is_ep_out_en) {
        ep_out_cfg = cfg;
    }
}

void tud_audio_set_epin_cfg(tud_audio_config_t cfg)
{
    /* Validate audio configuration. */
    is_ep_in_en = !((cfg.sampling_rate == 0) || (cfg.resolution == 0) || (cfg.bytes_per_sample == 0) ||
                    (cfg.nb_ch == 0) || (cfg.nb_ch > CFG_TUD_AUDIO_FUNC_1_MAX_N_CHANNELS_IN));

    if (is_ep_in_en) {
        ep_in_cfg = cfg;
    }
}

__attribute__((weak)) uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type,
                                                     uint8_t *buffer, uint16_t reqlen)
{
    (void)itf;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)reqlen;

    return 0;
}

__attribute__((weak)) void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type,
                                                 uint8_t const *buffer, uint16_t bufsize)
{
    (void)itf;
    (void)report_id;
    (void)report_type;
    (void)buffer;
    (void)bufsize;
}
