/** @file  tinyusb_audio.c
 *  @brief TinyUSB module with audio functionality.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "tusb.h"
#include "tusb_config.h"

/* CONSTANTS ******************************************************************/
/* USB media codes. */
#define USB_HID_NAN        0x00
#define USB_HID_PLAY_PAUSE 0x10
#define USB_HID_MUTE       0x20
#define USB_HID_VOL_UP     0x40
#define USB_HID_VOL_DOWN   0x80

/* PRIVATE GLOBALS ************************************************************/
static bool prod_started;
static uint16_t ep_in_target_fifo_size;

static volatile uint32_t mic_mute_count;
static uint8_t mute_buf[CFG_TUD_AUDIO_FUNC_1_EP_IN_SW_BUF_SZ / 2] = {0};

/*! Callback function to call on audio reception. */
static void (*rx_callback)(void);
/*! Fifo level threshold from which the speaker rx callback will be called. */
static uint32_t rx_callback_fifo_threshold;
/*! Whether a media key report has been sent and is awaiting completion. */
static bool media_key_sent;

/* PUBLIC FUNCTIONS ***********************************************************/
void tinyusb_audio_start_spk(void)
{
    prod_started = true;
}

void tinyusb_audio_stop_spk(void)
{
    prod_started = false;
}

void tinyusb_audio_set_ep_in_target_fifo_size(uint16_t target_fifo_size)
{
    ep_in_target_fifo_size = target_fifo_size;
}

void tinyusb_audio_set_speaker_rx_callback(void (*callback)(void), uint32_t fifo_threshold)
{
    if (fifo_threshold != 0) {
        rx_callback = callback;
        rx_callback_fifo_threshold = fifo_threshold;
    }
}

uint32_t tinyusb_audio_get_epin_fifo_size(void)
{
#if CFG_TUD_AUDIO_ENABLE_EP_IN
    tu_fifo_t *in_fifo = tud_audio_get_ep_in_ff();

    if (in_fifo == NULL) {
        return 0;
    }

    return tu_fifo_count(in_fifo);
#else
    return 0;
#endif
}

uint16_t tinyusb_audio_read(void *samples, uint16_t size)
{
#if CFG_TUD_AUDIO_ENABLE_EP_OUT
    uint16_t ret_size = 0;

    if (tud_audio_available() >= size) {
        ret_size = tud_audio_read(samples, size);
    }

    return ret_size;
#else
    (void)samples;
    (void)size;

    return 0;
#endif
}

uint16_t tinyusb_audio_write(void *samples, uint16_t size)
{
#if CFG_TUD_AUDIO_ENABLE_EP_IN
    /* Write audio data into USB FIFO. tinyusb FIFO will automatically overwrite old data once the queue is full. */
    if (mic_mute_count > 0) {
        /* Ignore incoming samples while playing mute buffer. */
        return tud_audio_write(mute_buf, size);
    } else {
        return tud_audio_write(samples, size);
    }
#else
    (void)data;
    (void)len;
#endif
    return 0;
}

void tinyusb_audio_send_media_key(bool play_pause, bool volume_up, bool volume_down)
{
    uint8_t report = (play_pause ? USB_HID_PLAY_PAUSE : 0) | (volume_up ? USB_HID_VOL_UP : 0) |
                     (volume_down ? USB_HID_VOL_DOWN : 0);

    if (tud_hid_report(USB_HID_MEDIA_REPORT_ID, &report, sizeof(report))) {
        media_key_sent = true;
    }
}

int8_t tinyusb_audio_get_epin_volume_db(void)
{
    return tud_audio_get_epin_volume_db();
}

int8_t tinyusb_audio_get_epout_volume_db(void)
{
    return tud_audio_get_epout_volume_db();
}

/* TINYUSB CALLBACK DEFINITIONS ***********************************************/
bool tud_audio_rx_done_isr(uint8_t rhport, uint16_t n_bytes_received, uint8_t func_id, uint8_t ep_out,
                           uint8_t cur_alt_setting)
{
    (void)rhport;
    (void)func_id;
    (void)ep_out;
    (void)cur_alt_setting;
    (void)n_bytes_received;

    if (rx_callback == NULL || !prod_started) {
        /* Application is not ready. Clear fifo. */
        tud_audio_clear_ep_out_ff();
        return true;
    }

    while (tud_audio_available() >= rx_callback_fifo_threshold) {
        rx_callback();
    }

    return true;
}

bool tud_audio_tx_done_isr(uint8_t rhport, uint16_t n_bytes_sent, uint8_t func_id, uint8_t ep_in,
                           uint8_t cur_alt_setting)
{
    (void)rhport;
    (void)func_id;
    (void)ep_in;
    (void)cur_alt_setting;

    tu_fifo_t *in_fifo = tud_audio_get_ep_in_ff();
    uint32_t fifo_lvl;

    if (in_fifo == NULL) {
        return false;
    }
    /* Update the target fifo size. */
    tud_audio_set_ep_in_fifo_threshold(ep_in_target_fifo_size);

    fifo_lvl = tu_fifo_count(in_fifo);

    if (fifo_lvl < n_bytes_sent || n_bytes_sent == 0) {
        /* There isn't enough bytes for the next transfer. Fill the ep in fifo with zeros. */
        tud_audio_write(mute_buf, sizeof(mute_buf));
        mic_mute_count = sizeof(mute_buf);
    } else {
        if (mic_mute_count > n_bytes_sent) {
            mic_mute_count -= n_bytes_sent;
        } else {
            mic_mute_count = 0;
        }
    }

    return true;
}

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint16_t len)
{
    (void)instance;
    (void)len;
    (void)report;
    (void)len;

    uint8_t report_code = 0;

    if ((report[0] == USB_HID_MEDIA_REPORT_ID) && media_key_sent) {
        media_key_sent = false;
        tud_hid_report(report[0], &report_code, sizeof(report_code));
    }
}
