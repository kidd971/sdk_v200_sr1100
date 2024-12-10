/** @file  wps_mac_certification.c
 *  @brief Wireless Protocol Stack MAC certification module.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "wps.h"
#include "wps_mac.h"

/* CONSTANTS ******************************************************************/
/** @brief Certification pattern bytes
 */
#define PHY_CERTIF_BYTE0 0x6F
#define PHY_CERTIF_BYTE1 0x0A

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void wps_mac_certification_auto_reply_conn_config(wps_connection_t *conn_main,
                                                         wps_connection_t *conn_auto);

/* PUBLIC FUNCTIONS ***********************************************************/
void wps_mac_certification_init(void *wps_mac)
{
    wps_mac_t *mac = (wps_mac_t *)wps_mac;

    wps_connection_t *connection_main;
    wps_connection_t *connection_auto;
    timeslot_t *prev_timeslot;
    timeslot_t *current_timeslot;
    timeslot_t *time_slot;
    uint8_t initial_index = mac->scheduler.current_time_slot_num;
    uint8_t current_index = 255;
    uint32_t rx_air_time;

    mac->node_role = NETWORK_COORDINATOR;

    /* Update rx timeslot duration to delay ack by expected rx packet air time. */
    while (initial_index != current_index) {
        prev_timeslot = link_scheduler_get_previous_timeslot_index(&mac->scheduler);
        current_timeslot = link_scheduler_get_current_timeslot(&mac->scheduler);
        connection_main = link_scheduler_get_current_main_connection(&mac->scheduler, mac->main_connection_id);
        if (connection_main != NULL) {
            if ((connection_main->source_address != mac->local_address) && connection_main->ack_enable) {

                /* Update timeslot duration to delay ack by expected rx packet air time. */
                uint32_t syncword_bits = mac->tdma_sync.sync_word_size_bits;
                uint32_t preamble_bits = mac->tdma_sync.preamble_size_bits;
                bool iook = (connection_main->frame_cfg.modulation == MODULATION_IOOK) ? true : false;
                bool two_bit_ppm = (connection_main->frame_cfg.modulation == MODULATION_2BITPPM) ? true : false;
                uint8_t fec = FEC_TYPE_TO_RAW(connection_main->frame_cfg.fec);
                uint8_t address_bits = 16;
                uint8_t chip_repet = CHIP_REPET_TO_RAW(connection_main->frame_cfg.chip_repet);
                uint8_t isi_mitig = ISI_TYPE_TO_RAW(mac->tdma_sync.isi_mitig);
#ifdef SR1000
                uint8_t crc_bits = 16;
#else
                uint8_t crc_bits = 31;
#endif
                rx_air_time = wps_utils_get_delayed_wakeup_event(preamble_bits, syncword_bits, iook, fec, two_bit_ppm,
                                                                 chip_repet, isi_mitig, address_bits,
                                                                 connection_main->payload_size +
                                                                     connection_main->header_size,
                                                                 crc_bits, 0, 0, false, 0);

                prev_timeslot->duration_pll_cycles += rx_air_time;
                current_timeslot->duration_pll_cycles -= rx_air_time;
            }
        }
        link_scheduler_increment_time_slot(&mac->scheduler);
        current_index = mac->scheduler.current_time_slot_num;
    }

    current_index = 255;
    while (initial_index != current_index) {
        connection_main = link_scheduler_get_current_main_connection(&mac->scheduler,
                                                                     mac->main_connection_id);
        if (connection_main != NULL) {
            if (connection_main->source_address == mac->local_address) {
                connection_main->certification_mode_enabled = true;
                /* Disable connection acknowledge to avoid garanteed delivery conflitcts. */
                connection_main->ack_enable = false;
                connection_main->stop_and_wait_arq.enable = false;
                wps_mac_certification_send(connection_main);
            } else if (connection_main->ack_enable) {
                uint16_t temp = connection_main->source_address;

                connection_main->source_address             = mac->local_address;
                connection_main->destination_address        = temp;
                connection_main->payload_size               = 0;
                connection_main->header_size                = 0;
                connection_main->certification_mode_enabled = true;
                /* Disable connection acknowledge to avoid garanteed delivery conflitcts. */
                connection_main->ack_enable = false;
                connection_main->stop_and_wait_arq.enable = false;
                wps_mac_certification_send(connection_main);
            }
        }
        connection_auto = link_scheduler_get_current_auto_connection(&mac->scheduler,
                                                                     mac->auto_connection_id);
        if (connection_auto != NULL) {
            if (connection_auto->source_address == mac->local_address) {
                wps_mac_certification_auto_reply_conn_config(connection_main, connection_auto);
                time_slot = link_scheduler_get_current_timeslot(&mac->scheduler);
                time_slot->auto_connection_count = 0;
                time_slot->main_connection_count = 1;
                connection_main->certification_mode_enabled = true;
                /* Disable connection acknowledge to avoid garanteed delivery conflitcts. */
                connection_main->ack_enable = false;
                connection_main->stop_and_wait_arq.enable = false;
                wps_mac_certification_send(connection_main);
            }
        }
        link_scheduler_increment_time_slot(&mac->scheduler);
        current_index = mac->scheduler.current_time_slot_num;
    }
}

/** @brief Send certification frame on connection.
 *
 *  @param[in] connection  Connection.
 */
void wps_mac_certification_send(wps_connection_t *connection)
{
    wps_error_t wps_err = WPS_NO_ERROR;
    uint8_t *data       = NULL;

    wps_get_free_slot(connection, &data, connection->payload_size, &wps_err);
    if ((wps_err != WPS_NO_ERROR) || (data == NULL)) {
        /* Queue is full. */
        return;
    }
    for (uint8_t i = 0; i < connection->payload_size; i++) {
        /* Send maximum power */
        if (connection->link_protocol.max_buffer_size % 2 == 0) {
            data[i] = (i % 2 == 0) ? PHY_CERTIF_BYTE0 : PHY_CERTIF_BYTE1;
        } else {
            data[i] = (i % 2 == 0) ? PHY_CERTIF_BYTE1 : PHY_CERTIF_BYTE0;
        }
    }
    wps_send(connection, data, connection->payload_size, &wps_err);
}

void wps_mac_certification_fill_header(uint8_t *header, uint8_t header_size)
{
    for (uint8_t i = 0; i < header_size; i++) {
        header[i] = (i % 2 == 0) ? PHY_CERTIF_BYTE0 : PHY_CERTIF_BYTE1;
    }
}

/* PRIVATE STATE FUNCTIONS ****************************************************/
/** @brief Configure certification auto-reply connection.
 *
 *  @param[in] conn_main  Connection main.
 *  @param[in] conn_auto  Connection auto-reply.
 */
static void wps_mac_certification_auto_reply_conn_config(wps_connection_t *conn_main,
                                                         wps_connection_t *conn_auto)
{
    memcpy(&conn_auto->frame_cfg, &conn_main->frame_cfg, sizeof(frame_cfg_t));
    memcpy(&conn_auto->channel, &conn_main->channel,
           sizeof(rf_channel_t) * conn_main->max_channel_count * WPS_RADIO_COUNT);
    memcpy(conn_main, conn_auto, sizeof(wps_connection_t));
}
