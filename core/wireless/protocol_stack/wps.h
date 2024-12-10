/** @file  wps.h
 *  @brief SPARK Wireless Protocol Stack.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef WPS_H
#define WPS_H

/* INCLUDES *******************************************************************/
#include "wps_callback.h"
#include "wps_config.h"
#include "wps_def.h"
#include "wps_error.h"
#include "wps_mac.h"
#include "wps_phy.h"
#include "wps_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/* MACROS *********************************************************************/
/*! Bit mask to identify Auto reply timeslot (auto reply timeslot) */
#define BIT_AUTO_REPLY_TIMESLOT BIT(7)
/*! Time slot value mask */
#define TIMESLOT_VALUE_MASK 0x7F
/*! User MACRO to identify primary timeslot */
#define MAIN_TIMESLOT(x) ((x) & 0x7F)
/*! User MACRO to identify data in auto-reply timeslot (auto reply timeslot) */
#define AUTO_TIMESLOT(x) ((x) | BIT_AUTO_REPLY_TIMESLOT)

/* TYPES **********************************************************************/
/** @brief WPS instance.
 */
typedef struct wps wps_t;

/** @brief Wireless Protocol Stack connection configuration.
 */
typedef struct wps_connection_config {
    /* Address */
    /*! Current connection source address (Transmitting node address) */
    uint16_t source_address;
    /*! Current connection destination address (Receiving node address) */
    uint16_t destination_address;

    /* Buffer */
    /*! Queue size */
    uint16_t fifo_buffer_size;
    /*! Length of the WPS header in the current configuration */
    uint16_t header_length;
    /*! Frame length to send/receive. Set to header + max payload size.*/
    uint32_t frame_length;
    /*! Length of the WPS header for ACK frame in the current configuration */
    uint16_t ack_header_length;

    /*! Connection priority */
    uint8_t priority;
    /*! Ranging mode */
    wps_ranging_mode_t ranging_mode;
    /*! Credit control flow flag. */
    bool credit_fc_enabled;

    /*! Get free running timer */
    uint64_t (*get_tick)(void);
    /*! Tick frequency in Hertz. */
    uint32_t tick_frequency_hz;
} wps_connection_cfg_t;

/** @brief Wireless Protocol Stack connection header configuration.
 *
 */
typedef struct wps_header_cfg {
    /*! main connection flag. */
    bool main_connection;
    /*! rdo enabled flag. */
    bool rdo_enabled;
    /*! Ranging mode */
    wps_ranging_mode_t ranging_mode;
    /*! Connection ID flag. */
    bool connection_id;
    /*! Credit control flow flag. */
    bool credit_fc_enabled;
} wps_header_cfg_t;

/** @brief Wireless Protocol Stack structure.
 */
typedef struct wps {
    /*! WPS node instance */
    wps_node_t *node;
    /*! WPS channel sequence for channel hopping */
    channel_sequence_t channel_sequence;
    /*! WPS random channel sequence enable flag */
    bool random_channel_sequence_enabled;
    /*! WPS concurrent network ID */
    uint8_t network_id;

    /*! WPS MAC Layer instance */
    wps_mac_t mac;
    /*! WPS Layer 1 instance */
    wps_phy_t phy[WPS_RADIO_COUNT];
    /*! WPS chip rate */
    chip_rate_cfg_t chip_rate;
} wps_t;

/*! RF channel array */
typedef rf_channel_t (**rf_channel_array_t)[WPS_RADIO_COUNT];

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Convert time in us to PLL cycles.
 *
 *  @param[in] time_us   Time in us.
 *  @param[in] chip_rate Chip rate.
 *
 *  @return Number of PLL cycles.
 */
uint32_t wps_us_to_pll_cycle(uint32_t time_us, chip_rate_cfg_t chip_rate);

/** @brief Wireless Protocol Stack radio initialization.
 *
 *  @note radio_hal_t and SPI buffer must be already be assigned in wps_radio_t.
 *
 *  @param[in]  wps_radio  WPS radio instance.
 *  @param[in]  no_reset   If false, radio will be reset and calibration will be done.
 *  @param[in]  err        Pointer to the error code.
 */
void wps_radio_init(wps_radio_t *wps_radio, bool no_reset, sr_phy_error_t *err);

/** @brief Perform the calibration routine of the radio.
 *
 *  @note This will also read the NVM to get calibration dependancies.
 *
 *  @note Make sure that the HAL and the SPI buffer are properly initialized
 *        before calling this.
 *
 *  @param[in] wps_radio  WPS radio instance.
 */
void wps_radio_calibration(wps_radio_t *wps_radio);

/** @brief Get the radio's 64-bit serial number.
 *
 *  @param[in] wps_radio  WPS radio instance.
 *  @return Serial number.
 */
uint64_t wps_radio_get_serial_number(wps_radio_t *wps_radio);

/** @brief Get the radio's product id version.
 *
 *  @param[in] wps_radio  WPS radio instance.
 *  @return Product id version.
 */
uint8_t wps_radio_get_product_id_version(wps_radio_t *wps_radio);

/** @brief Get the radio's product id model.
 *
 *  @param[in] wps_radio  WPS radio instance.
 *  @return Product id model.
 */
uint8_t wps_radio_get_product_id_model(wps_radio_t *wps_radio);

/** @brief Set vref_tune_offset_enabled flag.
 *
 *  The vref_tune_offset_enabled flag is used to fine-tune the output voltage of the main
 *  transceiver's reference buffer. This may be used to compensate for frequency, gain and
 *  overall performance variations between individual units.
 *
 *  @param[in] node                      WPS node instance..
 *  @param[in] vref_tune_offset_enabled  Enable/disable vref tune offset.
 */
void wps_set_vref_tune_offset_enabled_flag(wps_radio_t *node, bool vref_tune_offset_enabled);

/** @brief Initialize the callback queue.
 *
 *  The callback queue is used to store event actions waiting to
 *  be executed by the application.
 *
 *  The size of the callback buffer should be equal to the size
 *  of the biggest Xlayer queue.
 *
 *  @param[in] wps              Wireless Protocol Stack instance.
 *  @param[in] callback_buffer  Array of callback function pointer.
 *  @param[in] size             Array size.
 */
void wps_init_callback_queue(wps_t *wps, wps_callback_inst_t *callback_buffer, size_t size);

/** @brief Initialize the queue for the application request.
 *
 *  @param[in] wps             Wireless Protocol Stack instance.
 *  @param[in] request_buffer  Array of request pointer.
 *  @param[in] size            Size fo the request array.
 *  @param[in] request_config  Request arrays configuration instance.
 */
void wps_init_request_queue(wps_t *wps, xlayer_request_info_t *request_buffer, size_t size,
                            wps_request_config_info_t *request_config);

/** @brief Get the number of bytes needed to initialize xlayer mempool for TX communication.
 *
 *  @param[in] node  Node instance.
 *  @param[in] err   Pointer to the error code.
 */
uint32_t wps_get_xlayer_tx_queue_nb_bytes_needed(wps_node_t *node, wps_error_t *err);

/** @brief Get the number of bytes needed to initialize xlayer mempool for RX communication.
 *
 *  @param[in] node  Node instance.
 *  @param[in] err   Pointer to the error code.
 */
uint32_t wps_get_xlayer_rx_queue_nb_bytes_needed(wps_node_t *node, wps_error_t *err);

/** @brief Initialize cross layer free queue.
 *
 *  @param[in] node         Node instance.
 *  @param[in] mem_pool_tx  Memory pool for TX communication.
 *  @param[in] mem_pool_rx  Memory pool for RX communication.
 *  @param[in] err          Pointer to the error code.
 */
void wps_init_xlayer(wps_node_t *node, uint8_t *mem_pool_tx, uint8_t *mem_pool_rx, wps_error_t *err);

/** @brief Wireless Protocol Stack initialization.
 *
 *  Initialize the WPS and all the layers inside the WPS.
 *
 *  @param[in]  wps   Wireless Protocol Stack instance.
 *  @param[in]  node  Node instance.
 *  @param[out] err   Pointer to the error code.
 */
void wps_init(wps_t *wps, wps_node_t *node, wps_error_t *err);

/** @brief Wireless Protocol Stack process initialization.
 *
 *  Initialize the WPS process.
 *
 *  @param[in] wps   Wireless Protocol Stack instance.
 */
void wps_process_init(wps_t *wps);

/** @brief Set network syncing address.
 *
 *  @param[in]  wps      Wireless Protocol Stack instance.
 *  @param[in]  address  Syncing address.
 *  @param[out] err      Pointer to the error code.
 */
void wps_set_syncing_address(wps_t *wps, uint16_t address, wps_error_t *err);

/** @brief Set network ID.
 *
 *  @param[in]  wps         Wireless Protocol Stack instance.
 *  @param[in]  network_id  Network ID.
 *  @param[out] err         Pointer to the error code.
 */
void wps_set_network_id(wps_t *wps, uint8_t network_id, wps_error_t *err);

/** @brief Node configuration.
 *
 *  Configure the SPARK radio for proper communication. This goes
 *  through all of the packet configurations, the interrupt event,
 *  the sleep level and the internal radio timer source.
 *
 *  @param[in]  node   Node instance.
 *  @param[in]  radio  Radio instance.
 *  @param[in]  cfg    Node configuration.
 *  @param[out] err    Pointer to the error code.
 */
void wps_config_node(wps_node_t *node, wps_radio_t *radio, wps_node_cfg_t *cfg, wps_error_t *err);

/** @brief Configure network schedule.
 *
 *  Initialize the schedule object and convert the given duration
 *  to the time base of the SPARK radio.
 *
 *  @param[in]  wps                           Wireless Protocol Stack instance.
 *  @param[in]  timeslot_duration_pll_cycles  Timeslot duration array in pll cycles.
 *  @param[in]  timeslot                      Timeslot array used by schedule.
 *  @param[in]  schedule_size                 Schedule size. Timeslot's array size must match.
 *  @param[out] err                           Pointer to the error code.
 */
void wps_config_network_schedule(wps_t *wps, uint32_t *timeslot_duration_pll_cycles, timeslot_t *timeslot, uint32_t schedule_size,
                                 wps_error_t *err);

/** @brief Reset schedule.
 *
 *  @param[in]  wps   Wireless Protocol Stack instance.
 *  @param[out] err   Pointer to the error code.
 */
void wps_reset_schedule(wps_t *wps, wps_error_t *err);

/** @brief Configure network channel sequence.
 *
 *  Initialize the channel sequence for the Channel Hopping
 *  module inside the Layer 2 of the WPS.
 *
 *  @param[in]  wps                      Wireless Protocol Stack instance.
 *  @param[in]  channel_sequence         Channel sequence.
 *  @param[in]  channel_sequence_buffer  Pointer to an allocated memory for channel hopping sequence.
 *  @param[in]  sequence_size            Channel sequence size.
 *  @param[out] err                      Pointer to the error code.
 */
void wps_config_network_channel_sequence(wps_t *wps, const uint32_t *channel_sequence, uint8_t *channel_sequence_buffer,
                                         uint32_t sequence_size, wps_error_t *err);

/** @brief Enable random channel sequence.
 *
 *  @param[in]  wps  Wireless Protocol Stack instance.
 *  @param[out] err  Pointer to the error code.
 */
void wps_enable_random_channel_sequence(wps_t *wps, wps_error_t *err);

/** @brief Disable random channel sequence.
 *
 *  @param[in]  wps  Wireless Protocol Stack instance.
 *  @param[out] err  Pointer to the error code.
 */
void wps_disable_random_channel_sequence(wps_t *wps, wps_error_t *err);

/** @brief Enable fast sync.
 *
 *  This allows the link to get synchronized faster when connections are not set to auto_sync.
 *
 *  The radio listens non-stop until it receives a frame then the TDMA schedule starts.
 *  WARNING: Available in IDLE sleep mode only.
 *
 *  @param[in]  wps  Wireless Protocol Stack instance.
 *  @param[out] err  Pointer to the error code.
 */
void wps_enable_fast_sync(wps_t *wps, wps_error_t *err);

/** @brief Disable fast sync.
 *
 *  @param[in]  wps  Wireless Protocol Stack instance.
 *  @param[out] err  Pointer to the error code.
 */
void wps_disable_fast_sync(wps_t *wps, wps_error_t *err);

/** @brief Initialize random data offset.
 *
 *  @param[in]  wps             Wireless Protocol Stack instance.
 *  @param[in]  rollover_value  Offset rollover value.
 *  @param[in]  increment_step  Number of timeslot between each offset increment.
 *  @param[out] err             Pointer to the error code.
 */
void wps_init_rdo(wps_t *wps, uint16_t rollover_value, uint16_t increment_step, wps_error_t *err);

/** @brief Enable random data offset.
 *
 *  @param[in]  wps  Wireless Protocol Stack instance.
 *  @param[out] err  Pointer to the error code.
 */
void wps_enable_rdo(wps_t *wps, wps_error_t *err);

/** @brief Disable random data offset.
 *
 *  @param[in]  wps  Wireless Protocol Stack instance.
 *  @param[out] err  Pointer to the error code.
 */
void wps_disable_rdo(wps_t *wps, wps_error_t *err);

/** @brief Enable the distributed desync concurrency mechanism.
 *
 *  @param[in]  wps                         Wireless Protocol Stack instance.
 *  @param[in]  max_timeslot_offset         Maximum offset to apply every timeslot in pll cycles.
 *  @param[in]  sync_loss_max_duration_pll  Maximum sync lost pll cycles before applying unsync tx
 *                                          offset.
 *  @param[out] err                         Pointer to the error code.
 */
void wps_enable_ddcm(wps_t *wps, uint16_t max_timeslot_offset, uint32_t sync_loss_max_duration_pll,
                     wps_error_t *err);

/** @brief Disable the distributed desync concurrency mechanism.
 *
 *  @param[in]  wps  Wireless Protocol Stack instance.
 *  @param[out] err  Pointer to the error code.
 */
void wps_disable_ddcm(wps_t *wps, wps_error_t *err);

/** @brief Configure connection status information.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[in]  status_cfg  Status configuration stucture.
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_config_status(wps_connection_t *connection, connect_status_cfg_t *status_cfg, wps_error_t *err);

/** @brief Get the connection header size.
 *
 *  @param[in] wps        Wireless Protocol Stack instance.
 *  @param[in] header_cfg Header configuration.
 *  @return    Header size.
 */
uint8_t wps_get_connection_header_size(wps_t *wps, wps_header_cfg_t header_cfg);

/** @brief Get the connection header size for an automatically created auto-reply frame data.
 *
 *  @param[in] wps         Wireless Protocol Stack instance.
 *  @param[in] header_cfg  Header configuration.
 *  @return  Header size.
 */
uint8_t wps_get_connection_ack_header_size(wps_t *wps, wps_header_cfg_t header_cfg);

/** @brief Create a connection between two nodes.
 *
 *  A connection is a unidirectional link between two nodes.
 *  The direction of the data will be determined by the relation
 *  between the source address of the connection and the current
 *  node address. For example, if source address is equal to
 *  this node address, the node will transmit during this
 *  connection.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[in]  node        Node instance.
 *  @param[in]  config      Connection configuration.
 *  @param[out] err         Pointer to the error code.
 */
void wps_create_connection(wps_connection_t *connection, wps_node_t *node, wps_connection_cfg_t *config,
                           wps_error_t *err);

/** @brief Enable connection fallback feature.
 *
 *  @param[in]  connection               Connection instance.
 *  @param[in]  threshold                Array of payload size for fallback threshold.
 *  @param[in]  threshold_count          Number of threshold.
 *  @param[in]  fallback_channel_buffer  Memory pointer to store fallback channels.
 *  @param[out] err                      Pointer to the error code.
 */
void wps_connection_enable_fallback(wps_connection_t *connection, uint8_t *threshold, uint8_t threshold_count,
                                    rf_channel_array_t fallback_channel_buffer, wps_error_t *err);

/** @brief Disable connection fallback.
 *
 * @param[in]  connection  Connection instance.
 * @param[out] err         Pointer to the error code.
 */
void wps_connection_disable_fallback(wps_connection_t *connection, wps_error_t *err);

/** @brief Configure the connection header.
 *
 *  @note Must be called after wps_create_connection and all
 *        enable/disable connection features functions.
 *        The wps header is variable. Enabled options are appended to the header.
 *
 *  @param[in]  wps        Wireless Protocol Stack instance.
 *  @param[in]  connection wps_connection_t instance.
 *  @param[in]  header_cfg Header configuration.
 *  @param[out] err        Pointer to the error code.
 */
void wps_configure_header_connection(wps_t *wps, wps_connection_t *connection, wps_header_cfg_t header_cfg, wps_error_t *err);

/** @brief Configure the header for ACK frame without dedicated auto-reply connection.
 *
 *  @note Must be called after wps_create_connection and all
 *        enable/disable connection features functions.
 *        The wps header is variable. Enabled options are appended to the header.
 *
 *  @param[in]  wps         Wireless Protocol Stack instance.
 *  @param[in]  connection  wps_connection_t instance.
 *  @param[in]  header_cfg  Header configuration.
 *  @param[out] err         Pointer to the error code.
 */
void wps_configure_header_acknowledge(wps_t *wps, wps_connection_t *connection, wps_header_cfg_t header_cfg,
                                                wps_error_t *err);

/** @brief Set connection's timeslot.
 *
 * A connection may send its payload via the MAIN_TIMESLOT or AUTO_TIMESLOT. It can't use both on
 * the same timeslot. Feature like retransmission are not available when using AUTO_TIMESLOT.
 *
 *  @param[in]  connection    Connection instance.
 *  @param[in]  network       Wireless Protocol Stack instance.
 *  @param[in]  timeslot_id   Timeslot id used by the connection.
 *  @param[in]  nb_timeslots  Number of timeslot used by the connection. Must match timeslot_id
 *                            array size.
 *  @param[out] err           Pointer to the error code.
 */
void wps_connection_set_timeslot(wps_connection_t *connection, wps_t *network, const int32_t *const timeslot_id,
                                 uint32_t nb_timeslots, wps_error_t *err);

/** @brief Set connection's timeslot priority.
 *
 *  @param[in]  connection      Connection instance.
 *  @param[in]  network         Wireless Protocol Stack instance.
 *  @param[in]  timeslot_id     Timeslot id used by the connection.
 *  @param[in]  nb_timeslots    Number of timeslot used by the connection. Must match timeslot_id
 *                              array size.
 *  @param[in]  slots_priority  Connection priority table set for timeslots. This parameter is optional.
 *                              If NULL, connection priority will be used instead.
 */
void wps_connection_set_timeslot_priority(wps_connection_t *connection, wps_t *network,
                                          const int32_t *const timeslot_id, uint32_t nb_timeslots,
                                          const uint8_t *const slots_priority);

/** @brief Configure connection's RF channel.
 *
 *  Configure the receiver's filter, transmission power and power amplifier.
 *
 *  @param[in]  connection      Connection instance.
 *  @param[in]  node            Node instance.
 *  @param[in]  channel_x       Channel number.
 *  @param[in]  config          Channel configuration.
 *  @param[out] err             Pointer to the error code.
 */
void wps_connection_config_channel(wps_connection_t *connection, wps_node_t *node, uint8_t channel_x,
                                   channel_cfg_t *config, wps_error_t *err);

/** @brief Configure connection's fallback RF channel.
 *
 *  Configure the receiver's filter, transmission power and power amplifier.
 *
 *  @param[in]  connection      Connection instance.
 *  @param[in]  node            Node instance.
 *  @param[in]  channel_x       Channel number.
 *  @param[in]  fallback_index  Fallback index.
 *  @param[in]  config          Channel configuration.
 *  @param[out] err             Pointer to the error code.
 */
void wps_connection_config_fallback_channel(wps_connection_t *connection, wps_node_t *node, uint8_t channel_x,
                                            uint8_t fallback_index, channel_cfg_t *config, wps_error_t *err);

/** @brief Configure connection's frame modulation and FEC level.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[in]  modulation  Modulation.
 *  @param[in]  chip_repet  Chip repetition.
 *  @param[in]  fec         FEC level.
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_config_frame(wps_connection_t *connection, modulation_t modulation,
                                 chip_repetition_t chip_repet, fec_level_t fec, wps_error_t *err);

/** @brief Enable acknowledgment for connection's packet.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_enable_ack(wps_connection_t *connection, wps_error_t *err);

/** @brief Disable acknowledgment for connection's packet.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_disable_ack(wps_connection_t *connection, wps_error_t *err);

/** @brief Enable phases fetching in the radio.
 *
 *  @param[in]  connection         Connection instance.
 *  @param[in]  phase_info_buffer  Array of thresholds.
 *  @param[in]  max_sample_size    Sample count to accumulate before next queue element.
 *  @param[out] err                Pointer to the error code.
 */
void wps_connection_enable_phases_aquisition(wps_connection_t *connection, phase_infos_t *phase_info_buffer,
                                             uint8_t max_sample_size, wps_error_t *err);

/** @brief Enable Stop and Wait (SaW) and Automatic Repeat Request (ARQ) for connection's packet.
 *
 *  @note This function must be called after wps_connection_enable_ack.
 *
 *  @param[in]  connection           Connection instance.
 *  @param[in]  local_address        Current node address.
 *  @param[in]  retry                Maximum number of retry.
 *  @param[in]  deadline             Deadline in ticks. Packet is dropped when deadline is reached.
 *  @param[out] err                  Pointer to the error code.
 */
void wps_connection_enable_stop_and_wait_arq(wps_connection_t *connection, uint16_t local_address,
                                             uint32_t retry, uint32_t deadline, wps_error_t *err);

/** @brief Disable Stop and Wait (SaW) and Automatic Repeat Request (ARQ) for connection's packet.
 *
 *  Retransmission are disabled when disabling the SaW ARQ.
 *  Incomplete transmission will be notified to user every
 *  time a packet is NACK.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_disable_stop_and_wait_arq(wps_connection_t *connection, wps_error_t *err);

/** @brief Enable auto-sync mode.
 *
 * If this mode is enabled, when the cross-layer queue is empty,
 * a small frame containing the MAC header will be sent over the air for the current time slot.
 * This can be useful to keep synchronization when no application data is being sent.
 *
 * Warning: When the auto-sync is disabled, the user should provide a consistent data rate
 * output on the coordinator in order to keep synchronization in the network.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_enable_auto_sync(wps_connection_t *connection, wps_error_t *err);

/** @brief Disable auto-sync mode.
 *
 * If this mode is disabled, when the cross-layer queue is empty,
 * no data will be sent over the air, the radio will simply wake up and
 * the WPS will prepare the next time slot.
 *
 * Warning: When the auto-sync is disabled, the user should provide a consistent data rate
 * output on the coordinator in order to keep synchronization in the network.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_disable_auto_sync(wps_connection_t *connection, wps_error_t *err);

/** @brief Enable a connection's Clear Channel Assessment (CCA).
 *
 *  @param[in]  connection             Connection instance.
 *  @param[in]  threshold              CCA threshold.
 *  @param[in]  retry_time_pll_cycles  CCA retry time.
 *  @param[in]  max_try_count          CCA try count.
 *  @param[in]  fail_action            CCA fail action.
 *  @param[in]  cca_on_time_pll_cycle  CCA on time, in PLL cycle.
 *  @param[out] err                    Pointer to the error code.
 */
void wps_connection_enable_cca(wps_connection_t *connection, uint8_t threshold, uint16_t retry_time_pll_cycles,
                               uint8_t max_try_count, cca_fail_action_t fail_action, uint8_t cca_on_time_pll_cycle,
                               wps_error_t *err);

/** @brief Disable connection Clear Channel Assessment (CCA).
 *
 *  @note To properly disable CCA, the CCA module needs to be disabled with a threshold of 0xff.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_disable_cca(wps_connection_t *connection, wps_error_t *err);

/** @brief Disable gain loop (SR1120 feature only).
 *
 *  @param[in]  connection  Connection instance.
 *  @param[in]  rx_gain     Fixed RX gain.
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_disable_gain_loop(wps_connection_t *connection, uint8_t rx_gain,
                                      wps_error_t *err);

/** @brief Enable gain loop.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_enable_gain_loop(wps_connection_t *connection, wps_error_t *err);

/** @brief Optimize the latency of the target connection using empty timeslot.
 *
 *  @note This will add a delay to the wakeup event in case of an empty timeslot. This
 *        will add processing to the application so that there is more time to enqueue a
 *        frame in the WPS. This will result in a lower minimum latency.
 *
 *  @param[in] connection        Target connection.
 *  @param[in] ack_payload_size  ACK payload size.
 *  @param[in] node              WPS node.
 *  @param[in] extended_addr_en  Using extended address mode (16 bits).
 *  @param[in] extended_crc_en   Using extended CRC mode (32 bits).
 *  @param[in] err               WPS error.
 */
void wps_connection_optimize_latency(wps_connection_t *connection, uint8_t ack_payload_size,
                                     wps_node_t *node, bool extended_addr_en, bool extended_crc_en,  wps_error_t *err);

/** @brief Enable Credit Flow Control for connection's packet.
 *
 *  @note This function must be called after wps_connection_enable_ack.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[in] has_main_ts  Denotes if a connection is using at least on main timeslot
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_enable_credit_flow_ctrl(wps_connection_t *connection, bool has_main_ts, wps_error_t *err);

/** @brief Disable Credit Flow Control for connection's packet.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[out] err         Pointer to the error code.
 */
void wps_connection_disable_credit_flow_ctrl(wps_connection_t *connection, wps_error_t *err);

/** @brief Set the callback function to execute when a payload is successfully transmitted.
 *
 *  @note The Core has successfully sent a frame. If ACKs are enabled, this callback is triggered when the
 *        ACK frame is received. If ACKs are disabled, it triggers every time the frame is sent.
 *
 *  @param[in] connection  Pointer to the connection.
 *  @param[in] callback    Function pointer to the callback.
 *  @param[in] parg        Void pointer argument for the callback.
 */
void wps_set_tx_success_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg);

/** @brief Set the callback function to execute when the WPS fail to transmit a frame.
 *
 *  @note An ACK frame was expected and was not received. If ACK is not enabled, this never triggers.
 *
 *  @param[in] connection  Pointer to the connection.
 *  @param[in] callback    Function pointer to the callback.
 *  @param[in] parg        Void pointer argument for the callback.
 */
void wps_set_tx_fail_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg);

/** @brief Set the callback function to execute when the WPS drops a frame.
 *
 *  @note The Core has discarded the frame because the deadline configured for the ARQ
 *        (either in time or in number of retries) has been met. If ARQ is not enabled,
 *        this never triggers.
 *
 *  @param[in] connection  Pointer to the connection.
 *  @param[in] callback    Function pointer to the callback.
 *  @param[in] parg        Void pointer argument for the callback.
 */
void wps_set_tx_drop_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg);

/** @brief Set the callback function to execute when the WPS successfully receives a frame.
 *
 *  @note The Core has successfully received a frame. The address matches its local address or the
 *        broadcast address and the CRC checked. The frame is ready to be picked up from the RX FIFO.
 *
 *  @param[in] connection  Pointer to the connection.
 *  @param[in] callback    Function pointer to the callback.
 *  @param[in] parg        Void pointer argument for the callback.
 */
void wps_set_rx_success_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg);

/** @brief Set the callback function to execute when the WPS successfully accumulates all ranging samples.
 *
 *  @note The required number of ranging data samples is reached.
 *
 *  @param[in] connection  Pointer to the connection.
 *  @param[in] callback    Function pointer to the callback.
 *  @param[in] parg        Void pointer argument for the callback.
 */
void wps_set_ranging_data_ready_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg);

/** @brief Set the event callback of a connection.
 *
 *  @note This callback report error that occurs in the WPS.
 *        Possible error are shown in the wps_error_t structure.
 *
 *  @param[in] connection  Pointer to the connection.
 *  @param[in] callback    Function pointer to the callback.
 *  @param[in] parg        Void pointer argument for the callback.
 */
void wps_set_event_callback(wps_connection_t *connection, void (*callback)(void *parg), void *parg);

/** @brief Connect node to network.
 *
 *  Setup the radio internal timer and reset every layer in the WPS.
 *
 *  @param[in]  wps  Wireless Protocol Stack instance.
 *  @param[out] err  Pointer to the error code.
 */
void wps_connect(wps_t *wps, wps_error_t *err);

/** @brief Disconnect node from network.
 *
 *  Put radio to sleep and disable internal radio timer
 *  to disconnect the radio from the network.
 *
 *  @param[in]  wps  Wireless Protocol Stack instance.
 *  @param[out] err  Pointer to the error code.
 */
void wps_disconnect(wps_t *wps, wps_error_t *err);

/** @brief Reset the WPS when a crash occurs.
 *
 *  When a crash occurs the WPS is disconnected and then reconnected.
 *
 *  @param[in]  wps  Wireless Protocol Stack instance.
 *  @param[out] err  Pointer to the error code.
 */
void wps_reset(wps_t *wps, wps_error_t *err);

/** @brief Halt connection to network.
 *
 *  The node stays synchronized but doesn't send / receive application payload.
 *
 *  @param[in]  wps  Wireless Protocol Stack instance.
 *  @param[out] err  Pointer to the error code.
 */
void wps_halt(wps_t *wps, wps_error_t *err);

/** @brief Resume connection to network.
 *
 *  @param[in]  wps  Wireless Protocol Stack instance.
 *  @param[out] err  Pointer to the error code.
 */
void wps_resume(wps_t *wps, wps_error_t *err);

/** @brief Get buffer from the wps queue to hold the payload
 *
 *  The usage of this function is optional. If you don't want to
 *  use the WPS queue to hold the tx payload, when wps_create_connection
 *  is called, set the config.frame_length value to WPS_RADIO_HEADER_SIZE.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[in]  payload     Pointer to the empty payload memory.
 *  @param[in]  size        Required payload size.
 *  @param[out] err         Pointer to the error code.
 */
void wps_get_free_slot(wps_connection_t *connection, uint8_t **payload, uint16_t size,
                       wps_error_t *err);

/** @brief Send payload over the air.
 *
 *  Enqueue a node in the connection Xlayer and WPS will
 *  send at next available timeslot.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[in]  payload     Application payload to send over the air.
 *  @param[in]  size        Payload size in bytes.
 *  @param[out] err         Pointer to the error code.
 */
void wps_send(wps_connection_t *connection, const uint8_t *payload, uint8_t size, wps_error_t *err);

/** @brief Read last received frame.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[out] err         Pointer to the error code.
 *  @return WPS Received frame structure, including payload and size.
 */
wps_rx_frame wps_read(wps_connection_t *connection, wps_error_t *err);

/** @brief Remove the frame from the receiver FIFO.
 *
 *  @param[in]  connection  Connection instance.
 *  @param[out] err         Pointer to the error code.
 */
void wps_read_done(wps_connection_t *connection, wps_error_t *err);

/** @brief Copy the received frame to the payload buffer and free the queue.
 *
 *  @param[in]  connection Pointer to a wps_connection_t instance.
 *  @param[out] payload    Pointer to the output buffer
 *  @param[in]  max_size   Size of the output buffer.
 *  @param[in]  err        Wireless Core error code.
 *  @return WPS Received frame structure, including payload and size.
 */
wps_rx_frame wps_read_to_buffer(wps_connection_t *connection, uint8_t *payload, size_t max_size, wps_error_t *err);

/** @brief Get the received frame payload size.
 *
 *  @param[in]  connection Pointer to a wps_connection_t instance.
 *  @param[in]  err        Wireless Core error code.
 *  @return WPS Received frame size.
 */
uint16_t wps_get_read_payload_size(wps_connection_t *connection, wps_error_t *err);

/** @brief Return the used space of the connection Xlayer queue.
 *
 *  @param[in] connection  Connection instance.
 *  @return Xlayer queue's used space.
 */
uint32_t wps_get_fifo_size(wps_connection_t *connection);

/** @brief Return the free space of the connection Xlayer queue.
 *
 *  @param[in] connection  Connection instance.
 *  @return Xlayer queues's free space.
 */
uint32_t wps_get_fifo_free_space(wps_connection_t *connection);

/** @brief Return if the connection is connected or disconnected.
 *
 *  @param[in] connection  Connection handle.
 *  @return True if the connection is connected, False otherwise.
 */
bool wps_get_connect_status(wps_connection_t *connection);

/** @brief Get the current WPS error.
 *
 *  @note This function should only be called from
 *        the evt_callback that should be implemented
 *        in the application. The output error is of type
 *        wps_error_t.
 *
 *  @param[in] connection  Current connection with an error.
 *  @return Current WPS error.
 */
wps_error_t wps_get_error(wps_connection_t *connection);

/** @brief Get the current WPS event.
 *
 *   @param[in] connection  Current connection with an error.
 *   @return Current WPS event.
 */
wps_event_t wps_get_event(wps_connection_t *connection);

/** @brief Initialize the connection throttle feature.
 *
 *  @note The pattern member of the connection struct need
 *        to be allocated to at least WPS_PATTERN_THROTTLE_GRANULARITY * sizeof(bool).
 *
 *  @param[in]  connection  Current connection with an error.
 *  @param[out] err         Pointer to the error code.
 */
void wps_init_connection_throttle(wps_connection_t *connection, wps_error_t *err);

/** @brief Set the active timeslot ratio of the given connection.
 *
 *  @note Connection pattern member should been initialized and
 *        allocated before calling this method.
 *
 *  @param[in]  wps            Wireless Protocol Stack instance.
 *  @param[in]  connection     Current connection with an error.
 *  @param[in]  ratio_percent  Active timeslot ratio, in percent.
 *  @param[out] err            Pointer to the error code.
 */
void wps_set_active_ratio(wps_t *wps, wps_connection_t *connection, uint8_t ratio_percent, wps_error_t *err);

/** @brief Issue a write register request to the WPS.
 *
 *  @note Next time WPS is ready, it will write to the
 *        requested register(s).
 *
 *  @param[in]  wps           Wireless Protocol Stack instance.
 *  @param[in]  starting_reg  Starting register.
 *  @param[in]  data          Byte to send.
 *  @param[in]  cfg           Write cfg.
 *  @param[out] err           Pointer to the error code.
 */
void wps_request_write_register(wps_t *wps, uint8_t starting_reg, uint16_t data,
                                reg_write_cfg_t cfg, wps_error_t *err);

/** @brief Clear periodic register write.
 *
 *  @param[in]  wps  Wireless Protocol Stack instance.
 */
void wps_clear_write_register(wps_t *wps);

/** @brief Issue a read register request to the WPS
 *
 *   @param[in] wps              Wireless Protocol Stack instance.
 *   @param[in] target_register  Radio Target register.
 *   @param[in] rx_buffer        Buffer containing radio register data.
 *   @param[in] xfer_cmplt       Bool to notify application that the transfer is complete.
 *   @param[in] err              WPS Error.
 */
void wps_request_read_register(wps_t *wps, uint8_t target_register, uint16_t *rx_buffer,
                               bool *xfer_cmplt, wps_error_t *err);

/** @brief Process the wps callback
 *
 * This function should be called in a context with higher
 * priority than the application, but lower priority than the
 * radio IRQs. You can run it in a really high priority RTOS
 * task or something like pendsv.
 *
 *  @param[in] wps  Wireless Protocol Stack instance.
 */
void wps_process_callback(wps_t *wps);

/** @brief Get total number of CCA events.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Total CCA events.
 */
uint32_t wps_get_phy_total_cca_events(wps_connection_t *connection);

/** @brief Get total number of CCA fail events.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Total CCA fail count.
 */
uint32_t wps_get_phy_total_cca_fail_count(wps_connection_t *connection);

/** @brief Get total number of CCA TX fail events.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Total CCA tx fail count.
 */
uint32_t wps_get_phy_total_cca_tx_fail_count(wps_connection_t *connection);

/** @brief Get total number of CCA TX packets dropped.
 *
 *  @param[in] connection  WPS connection object.
 *  @return Total CCA tx packets dropped.
 */
uint32_t wps_get_phy_total_pkt_dropped(wps_connection_t *connection);

/** @brief Read accumulated phase metrics.
 *
 *  @param[in] connection  WPS connection object.
 *  @param[out] err        Pointer to the error code.
 *  @return WPS phase received frame structure, including payload and size.
 */
wps_phase_frame wps_read_phase(wps_connection_t *connection, wps_error_t *err);

/** @brief Remove lasted phase frame from the phase queue.
 *
 *  @param[in] connection  WPS connection object.
 *  @param[out] err        Pointer to the error code.
 */
void wps_read_phase_done(wps_connection_t *connection, wps_error_t *err);

/** @brief Return the count of unique channel in the configure channel sequence.
 *
 *  @param[in] wps  Wireless Protocol Stack instance.
 *  @param[in] err  Pointer to the error code.
 *  @return  Number of unique channel in the configure channel sequence.
 */
uint8_t wps_get_channel_count(wps_t *wps, wps_error_t *err);

/** @brief Radio IRQ signal.
 *
 *  Notify the WPS of a context switch.
 *
 *  @param[in] wps  Wireless Protocol Stack instance.
 */
static inline void wps_radio_irq(wps_t *wps)
{
    if (wps->mac.signal == WPS_DISCONNECT) {
        /* IRQ happened during disconnect. */
        return;
    }

    wps->node->low_power_allowed = false;
    wps->mac.signal = WPS_RADIO_IRQ;
    wps_phy_set_input_signal(wps->phy, PHY_SIGNAL_RADIO_IRQ);
    wps_phy_process(wps->phy);
}

/** @brief SPI transfer complete.
 *
 *  Notify the WPS of a DMA transfer complete interrupt.
 *
 *  @param[in] wps  Wireless Protocol Stack instance.
 */
static inline void wps_transfer_complete(wps_t *wps)
{
    wps->mac.signal = WPS_TRANSFER_COMPLETE;
    wps_phy_set_input_signal(wps->phy, PHY_SIGNAL_DMA_CMPLT);
    wps_phy_process(wps->phy);
}

#if WPS_RADIO_COUNT > 1

/** @brief Initialize the multi-radio BSP.
 *
 *  @param[in]  multi_cfg  Multi radio config.
 *  @param[in]  chip_rate  Chip rate.
 *  @param[out] err        Pointer to the error code.
 */
void wps_multi_init(wps_multi_cfg_t multi_cfg, chip_rate_cfg_t chip_rate, wps_error_t *err);

/** @brief Process the MCU timer interrupt for Radio synchronization.
 *
 *  @param[in] wps  Wireless Protocol Stack instance.
 */
static inline void wps_multi_radio_timer_process(wps_t *wps)
{
    wps_phy_multi_process_radio_timer(wps->phy);
}

/** @brief Set which radio raises the SPI or radio interrupt.
 *
 *  @note This function should always be called before wps_process.
 *
 *  @param[in] index  Radio Index.
 */
static inline void wps_set_irq_index(uint8_t index)
{
    wps_phy_multi_set_current_radio_idx(index);
}

#endif

#ifdef __cplusplus
}
#endif

#endif /* WPS_H */
