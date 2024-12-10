/** @file  tusb_descriptors.c
 *  @brief TinyUSB descriptor file.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *             Copyright (C) 2019 Ha Thach (tinyusb.org).
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
#include "board_api.h"
#include "tiny_usb_facade.h"
#include "tusb.h"

/* CONSTANTS ******************************************************************/
/* A combination of interfaces must have a unique product id, since PC will save device driver after the first plug.
 * Same VID/PID with different interface e.g MSC (first), then CDC (later) will possibly cause system error on PC.
 *
 * Auto ProductID layout's Bitmap:
 *   [MSB]         HID | MSC | CDC          [LSB]
 */
#define _PID_MAP(itf, n)  ((CFG_TUD_##itf) << (n))
#ifndef TUSB_PID
#define TUSB_PID             (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | \
                           _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4))
#endif
#ifndef TUSB_VID
#define TUSB_VID             0xCafe
#endif
/* BUSB Specification Release Number in Binary-Coded Decimal. */
#define USB_BCD 0x0200

/* Configuration descriptor total length. */
#define CONFIG_TOTAL_LEN (TUD_CONFIG_DESC_LEN + CFG_TUD_CDC * TUD_CDC_DESC_LEN)

#define EPNUM_CDC_0_NOTIF   0x81
#define EPNUM_CDC_0_OUT     0x02
#define EPNUM_CDC_0_IN      0x82

/* Configuration Descriptor */
enum {
    ITF_NUM_CDC_0 = 0,
    ITF_NUM_CDC_0_DATA,
    ITF_NUM_TOTAL
};

/* PRIVATE GLOBALS ************************************************************/
/** @brief Device descriptor.
 */
tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = USB_BCD,

    /* Use Interface Association Descriptor (IAD) for CDC
     * As required by USB Specs IAD's subclass must be common class (2) and protocol must be IAD (1)
     */
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = TUSB_VID,
    .idProduct          = TUSB_PID,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

/** @brief Full speed device configutation.
 */
uint8_t const desc_fs_configuration[] = {
    /* Config number, interface count, string index, total length, attribute, power in mA */
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

    /* CDC: Interface number, string index, EP notification address and size, EP data address (out, in) and size. */
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_0, 4, EPNUM_CDC_0_NOTIF, 8, EPNUM_CDC_0_OUT, EPNUM_CDC_0_IN, 64),
};

/** @brief Hish speed device configutation.
 */
#if TUD_OPT_HIGH_SPEED
/* Per USB specs: high speed capable device must report device_qualifier and other_speed_configuration */

uint8_t const desc_hs_configuration[] = {
    /* Config number, interface count, string index, total length, attribute, power in mA */
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

    /* CDC: Interface number, string index, EP notification address and size, EP data address (out, in) and size. */
    TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_0, 4, EPNUM_CDC_0_NOTIF, 8, EPNUM_CDC_0_OUT, EPNUM_CDC_0_IN, 512),
};

/* device qualifier is mostly similar to device descriptor since we don't change configuration based on speed */
tusb_desc_device_qualifier_t const desc_device_qualifier = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = USB_BCD,

    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,

    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .bNumConfigurations = 0x01,
    .bReserved          = 0x00
};

/* String Descriptors */
enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
};

/* array of pointer to string descriptors */
char const *string_desc_arr[] = {
    (const char[]) { 0x09, 0x04 }, /* 0: is supported language is English (0x0409) */
    "SPARK Microsystems Inc.",     /* 1: Manufacturer */
    TUSB_PRODUCT_NAME,             /* 2: Product */
    (char[]) {'0', '1', '2', '3',
              '4', '5', '6', '7',
              '8', '9', 'A', 'B',
              'C', 'D', 'E', 'F',
              '\000'},             /* 3: Serials will use unique ID if possible */
    "TinyUSB CDC",                 /* 4: CDC Interface */
};

static uint16_t _desc_str[32 + 1];

/* PUBLIC FUNCTIONS ***********************************************************/
/* Invoked when received GET DEVICE QUALIFIER DESCRIPTOR request
 * Application return pointer to descriptor, whose contents must exist long enough for transfer to complete.
 * device_qualifier descriptor describes information about a high-speed capable device that would
 * change if the device were operating at the other speed. If not highspeed capable stall this request.
 */
uint8_t const *tud_descriptor_device_qualifier_cb(void)
{
    return (uint8_t const *)&desc_device_qualifier;
}

/* Invoked when received GET OTHER SEED CONFIGURATION DESCRIPTOR request
 * Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
 * Configuration descriptor in the other speed e.g if high speed then this is for full speed and vice versa
 */
uint8_t const *tud_descriptor_other_speed_configuration_cb(uint8_t index)
{
    (void)index; /* for multiple configurations */

    /* if link speed is high return fullspeed config, and vice versa */
    return (tud_speed_get() == TUSB_SPEED_HIGH) ? desc_fs_configuration : desc_hs_configuration;
}

#endif /* highspeed */

/* Invoked when received GET CONFIGURATION DESCRIPTOR
 * Application return pointer to descriptor
 * Descriptor contents must exist long enough for transfer to complete
 */
uint8_t const *tud_descriptor_configuration_cb(uint8_t index)
{
    (void)index; /* for multiple configurations */

#if TUD_OPT_HIGH_SPEED
  /* Although we are highspeed, host may be fullspeed. */
    return (tud_speed_get() == TUSB_SPEED_HIGH) ? desc_hs_configuration : desc_fs_configuration;
#else
    return desc_fs_configuration;
#endif
}

/* Invoked when received GET DEVICE DESCRIPTOR
 * Application return pointer to descriptor
 */
uint8_t const *tud_descriptor_device_cb(void)
{
    return (uint8_t const *)&desc_device;
}

/* Invoked when received GET STRING DESCRIPTOR request */
/* Application return pointer to descriptor, whose contents must exist long enough for transfer to complete */
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void)langid;
    size_t chr_count;

    switch (index) {
    case STRID_LANGID:
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
        break;

    case STRID_SERIAL:
        chr_count = tuh_get_serial_id((uint8_t*)_desc_str + 2, 32);
        break;

    default:
        /* Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors. */
        /* https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors */

        if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))) {
            return NULL;
        }

        const char *str = string_desc_arr[index];

        /* Cap at max char */
        chr_count = strlen(str);
        size_t const max_count = sizeof(_desc_str) / sizeof(_desc_str[0]) - 1; // -1 for string type

        if (chr_count > max_count) {
            chr_count = max_count;
        }

        /* Convert ASCII string into UTF-16 */
        for (size_t i = 0; i < chr_count; i++) {
            _desc_str[1 + i] = str[i];
        }
        break;
    }

    /* first byte is length (including header), second byte is string type */
    _desc_str[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * chr_count + 2));

    return _desc_str;
}
