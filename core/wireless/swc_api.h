/** @file  sr1100/swc_api.h
 *  @brief SPARK Wireless Core Application Programming Interface.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SWC_API_H_
#define SWC_API_H_

/* INCLUDES *******************************************************************/
#include <stdint.h>
#include "swc_def.h"
#include "swc_error.h"
#include "wps.h"
#include "wps_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/*! Destination address to use for broadcasting */
#define SWC_BROADCAST_ADDRESS 0xFF
/*! Legacy PAN ID is only 12-bit. The 4 MSBs are invalid and can't be used. */
#define SWC_LEGACY_PAN_ID_INVALID_RANGE 0xF000
/*! PAN ID is only 15-bit. The MSB is invalid and can't be used. */
#define SWC_PAN_ID_INVALID_RANGE 0x8000
/*! Reserved PAN ID use for pairing purpose. */
#define SWC_RESERVED_PAN_ID 0x0000
/*! Network ID mask. */
#define SWC_NETWORK_ID_MASK 0xFF
/*! Broadcast address is invalid during normal operation in network ID. */
#define SWC_INVALID_BROADCAST_NETWORK_ID 0xFF

/* TYPES **********************************************************************/
/** @brief Wireless Core configuration.
 */
typedef struct swc_cfg {
    /*! Network schedule as an array of timeslot durations in microseconds. */
    const uint32_t *timeslot_sequence;
    /*! Number of timeslots in the timeslot sequence. */
    uint32_t timeslot_sequence_length;
    /*! RF channels as an array of channel numbers. */
    const uint32_t *channel_sequence;
    /*! Number of channels in the channel sequence. */
    uint32_t channel_sequence_length;
    /*! Concurrency mode. */
    swc_concurrency_mode_t concurrency_mode;
    /*! Memory pool instance from which memory allocation is done. */
    uint8_t *memory_pool;
    /*! Memory pool size in bytes. */
    uint32_t memory_pool_size;
    /*! Chip rate. */
    swc_chip_rate_t chip_rate;
} swc_cfg_t;

/** @brief Wireless Core concurrency configuration.
 */
typedef struct swc_concurrency_cfg {
    /*! Enable random channel sequence concurrency mechanism. */
    bool random_channel_sequence_enabled;
    /*! Enable the random datarate offset concurrency mechanism. */
    bool rdo_enabled;
    /*! Enable the distributed desync concurrency mechanism. */
    bool ddcm_enabled;
} swc_concurrency_cfg_t;

/** @brief Wireless node configuration.
 */
typedef struct swc_node_cfg {
    /*! Network role. */
    swc_role_t role;
    /*! Personal area network 12-bit ID. */
    uint16_t pan_id;
    /*! Coordinator device's 8-bit address; Same as local_address if local device is the Coordinator. */
    uint8_t coordinator_address;
    /*! Local device's 8-bit address. */
    uint8_t local_address;
    /*! ISI mitigation level. */
    swc_isi_mitig_t isi_mitig;
    /*! Dynamic phy mode enable flag. */
    bool dynamic_phy_mode_enabled;
} swc_node_cfg_t;

/** @brief Wireless statistics.
 */
typedef struct swc_statistics {
    /*! Increments when an acknowledge frame is received after sending a packet. */
    uint32_t packet_sent_and_acked_count;
    /*! Increments when an acknowledge frame is not received after sending a packet. */
    uint32_t packet_sent_and_not_acked_count;
    /*! Increments when there is nothing to send at the start of a TX timeslot. */
    uint32_t no_packet_tranmission_count;
    /*! Increments when a packet is dropped by the Wireless Core due to its timeout mechanism. */
    uint32_t packet_dropped_count;
    /*! Increments for every TX timeslot the connection goes through. */
    uint32_t tx_timeslot_occurrence;
    /*! Percentage of TX timeslots used for transmission over the total number of TX timeslots. */
    float tx_used_capacity_pc;
    /*! Increments when a packet is received and the CRC checks. */
    uint32_t packet_successfully_received_count;
    /*! Increment when nothing is received at the start of a RX timeslot. */
    uint32_t no_packet_reception_count;
    /*! Increments for every RX timeslot the connection goes through. */
    uint32_t rx_timeslot_occurrence;
    /*! Increments when a packet is received but is discarded because it is a
     *   duplicate of a previously received packet.
     */
    uint32_t packet_duplicated_count;
    /*! Increments when a packet is received but is discarded because the frame data size is invalid. */
    uint32_t packet_invalid_data_size_count;
    /*! Increments when a packet is received but is discarded because the frame header size is invalid. */
    uint32_t packet_invalid_header_size_count;
    /*! Increments when a packet is received but is discarded because the transceiver marked it as corrupted. */
    uint32_t packet_rejected_count;
    /*! Increments when a packet is received but is discarded because the Wireless Core reception queue is full. */
    uint32_t packet_overrun_count;
    /*! Increments when a ACK packet is received with data. */
    uint32_t packet_ack_data_received_count;
    /*! Increments when a ACK packet is sent with data. */
    uint32_t packet_ack_data_send_count;
    /*! Increments when a timeslot's Clear-Channel-Assessment passes and transmission occurs normally. */
    uint32_t cca_pass_count;
    /*! Increments when a timeslot's Clear-Channel-Assessment fails and aborts (or forces) transmission. */
    uint32_t cca_fail_count;
    /*! Increments when a single Clear-Channel-Assessment trial fails. */
    uint32_t cca_try_fail_count;

    /*! Average of all raw RSSI codes from the radio register. */
    uint32_t rssi_avg_raw;
    /*! Average of all raw RNSI codes from the radio register. */
    uint32_t rnsi_avg_raw;

    /*! Instantaneous RSSI value represented in tenths of dB. */
    uint32_t rssi_inst;
    /*! Instantaneous RNSI value represented in tenths of dB. */
    uint32_t rnsi_inst;

    /*! Average of all RSSI values in tenths of dB. */
    uint32_t rssi_avg;
    /*! Average of all RNSI values in tenths of dB. */
    uint32_t rnsi_avg;
    /*! Average of all Link Margin values in tenth of dB. */
    uint32_t link_margin_avg;

    /*! RSSI block average in tenths of dB. */
    uint32_t rssi_block_avg_tenth_db;
    /*! RNSI block average in tenths of dB. */
    uint32_t rnsi_block_avg_tenth_db;
    /*! Link Margin block average in tenths of dB. */
    uint32_t link_margin_block_avg_tenth_db;

    /*! Number of bytes sent. */
    uint32_t bytes_sent;
    /*! Number of bytes received. */
    uint32_t bytes_received;
    /*! TX datarate in bits per seconds. */
    uint32_t tx_data_rate_bps;
    /*! RX datarate in bits per seconds. */
    uint32_t rx_data_rate_bps;
    /*! Tick value on statistics reset. */
    uint32_t tick_on_reset;
    /*! Tick value since last statistics reset. */
    uint32_t tick_since_reset;
} swc_statistics_t;

/** @brief Wireless Quality of Service (QoS) indicators.
 */
typedef struct swc_qos_indicators {
    /*! Link margin average, in dB. */
    uint32_t link_margin;
    /*! Number of cca tries. */
    uint32_t cca_tries;
    /*! CCA delay between each retry, in PLL cycles. */
    uint32_t cca_retry_time;
    /*! ISI indicator, from 0 to 300 %. */
    uint32_t isi_indicator;
    /*! Total ISI indicator, from 0 to 300 %. */
    uint32_t total_isi_indicator;
    /*! PHY rate factor, to compute CCA times. */
    uint32_t phy_rate_factor;
    /*! Number of time TX had a payload to transmit. */
    uint32_t tx_tries;
    /*! Received count to compute average CCA times on RX side. */
    uint32_t received_count;
    /*! CCA on time, in PLL cycles. */
    uint32_t cca_on_time_pll_cycles;
    /*! Phase offset data. */
    uint32_t phase_offset_data[PHASE_OFFSET_BYTE_COUNT];
} swc_qos_indicators_t;

/** @brief Identifies each radio unit by a unique ID.
 *
 *  Each enum value corresponds to a specific radio HAL structure index, simplifying
 *  the selection and management of the appropriate radio hardware abstraction layer
 *  based on the radio ID.
 */
typedef enum swc_radio_id {
    /*! Radio 1 HAL structure index. */
    SWC_RADIO_ID_1 = 0,
    /*! Radio 2 HAL structure index. */
    SWC_RADIO_ID_2 = 1,
    /*!  Always keep this last to know the count of radio. */
    SWC_RADIO_ID_MAX = 2,
} swc_radio_id_t;

/** @brief Wireless connection configuration.
 */
typedef struct swc_connection_cfg {
    /*! Name of the connection as a character string. */
    const char *name;
    /*! Address of the transmitting node. */
    uint8_t source_address;
    /*! Address of the receiving node. */
    uint8_t destination_address;
    /*! Maximum size in bytes the payload can ever be. */
    uint8_t max_payload_size;
    /*! Queue size in number of frames. */
    uint16_t queue_size;
    /*! ID of timeslots used by the connection. */
    const int32_t *timeslot_id;
    /*! Number of timeslots used by the connection. */
    uint32_t timeslot_count;
} swc_connection_cfg_t;

/** @brief Wireless connection concurrency configuration.
 */
typedef struct swc_connection_concurrency_cfg {
    /*! Enable or disable the concurrency mechanism. */
    bool enabled;
    /*! Number of energy readings to do before the fail action is executed.
     *
     *  Maximum value is 16 if fail_action is set to SWC_CCA_ABORT_TX, 15 for SWC_CCA_FORCE_TX.
     */
    uint8_t try_count;
    /*! Amount of time between energy readings in increments of radio's PHY rate period (e.g. 48.8 ns at 20.48 MHz).
     *  This time include the energy reading measurement duration.
     *
     *  Maximum value is 2048 cycles + the number of cycles required for the energy reading.
     */
    uint16_t retry_time;
    /*! Action to do when the energy level sensed is still too high after the last energy sensing try. */
    swc_cca_fail_action_t fail_action;
} swc_connection_concurrency_cfg_t;

/** @brief Wireless connection fallback configuration.
 */
typedef struct swc_connection_fallback_cfg {
    /*! Enable or disable the fallback mechanism. */
    bool enabled;
    /*! Number of fallback modes. */
    uint8_t fallback_mode_count;
    /*! Array of payload size fallback threshold. in descending order. Array size must be equal to
     *   fallback_mode_count
     */
    uint8_t *thresholds;
    /*! Array of number of CCA tries. Array size must be equal to fallback_mode_count
     *
     *  Maximum CCA try value is 16 if fail_action is set to SWC_CCA_ABORT_TX, 15 for SWC_CCA_FORCE_TX.
     */
    uint8_t *cca_try_count;
} swc_connection_fallback_cfg_t;

/** @brief Wireless connection.
 */
typedef struct swc_connection {
    /*! Number of channels added to the connection. */
    uint8_t channel_count;
    /*! Wireless connection configuration. */
    swc_connection_cfg_t cfg;
    /*! Wireless connection statistics. */
    swc_statistics_t stats;
#if WPS_ENABLE_PHY_STATS_PER_BANDS
    /*! Wireless connection statistics per. bands*/
    swc_statistics_t *stats_per_bands;
    /*! Wireless connection QOS Indicators per bands. */
    swc_qos_indicators_t *qos_indicators_per_bands;
#endif
    /*! Low-level connection handle. */
    wps_connection_t *wps_conn_handle;
    /*! Flag to lock configuration of certain features when connection priority is enabled. */
    bool conn_priority_enabled;
    /*! Flag to lock configuration of certain features when slot priority is enabled. */
    bool slot_prio_enabled;
} swc_connection_t;

/** @brief Wireless channel configuration.
 */
typedef struct swc_channel_cfg {
    /*! Frequency of the channel in increments of 40.96 MHz (e.g., 183 for 7.5 GHz). */
    uint8_t frequency;
    /*! Pulses number of the transmitted frames, from 1 to 3. */
    uint8_t tx_pulse_count;
    /*! Pulses width of the transmitted frames, from 0 (narrow) to 7 (large). */
    uint8_t tx_pulse_width;
    /*! Pulses amplitude of the transmitted frames, from 0 (max gain: 0 dB) to 3 (min gain: -1.8 dB). */
    uint8_t tx_pulse_gain;
    /*! Pulses number of the received frames, from 1 to 3, corresponding to the tx_pulse_count
     *   of the incoming frames
     */
    uint8_t rx_pulse_count;
} swc_channel_cfg_t;

/** @brief Wireless fallback channel configuration.
 */
typedef struct swc_fallback_channel_cfg {
    /*! Pulses number of the transmitted frames, from 1 to 3. */
    uint8_t tx_pulse_count;
    /*! Pulses width of the transmitted frames, from 0 (narrow) to 7 (large). */
    uint8_t tx_pulse_width;
    /*! Pulses amplitude of the transmitted frames, from 0 (max gain: 0 dB) to 3 (min gain: -1.8 dB). */
    uint8_t tx_pulse_gain;
} swc_fallback_channel_cfg_t;

/** @brief Wireless fallback information.
 */
typedef struct swc_fallback_info {
    /*! Link margin value. */
    uint8_t link_margin;
    /*! CCA fail count value. */
    uint32_t cca_fail_count;
    /*! Number of times all CCA attempts failed. */
    uint32_t cca_tx_fail_count;
    /*! Total number of tx dropped packets. */
    uint32_t tx_pkt_dropped;
    /*! Total number of CCA events. */
    uint32_t cca_event_count;
} swc_fallback_info_t;

/** @brief The SPARK Wireless Core status.
 */
typedef enum swc_status {
    /*! The Wireless Core is stopped. */
    SWC_STATUS_STOPPED,
    /*! The wireless core module is running. */
    SWC_STATUS_RUNNING,
} swc_status_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Wireless Core initialization.
 *
 *  This is the first API call that needs to be made when initializing and
 *  configuring the Wireless Core.
 *
 *  @param[in]  cfg       Wireless Core configuration.
 *  @param[in]  node_cfg  Wireless radio configuration.
 *  @param[in]  callback  A function pointer to the logic initiating context switches.
 *  @param[out] err       Wireless Core error code.
 */
void swc_init(swc_cfg_t cfg, swc_node_cfg_t node_cfg, void (*callback)(void), swc_error_t *const err);

/** @brief Enable certification mode for the SWC.
 *
 *  @note This feature utilizes the application configuration to activate certification mode
 *        for the radio. In this mode, the radio transmits at maximum power and emulates acknowledgments
 *        according to the application's settings.
 *
 *  @note Enabling certification mode disables any features that modify the radio's wake-up behavior,
 *        such as concurrency and latency optimization, to ensure proper acknowledgment emulation.
 *
 *  @note By default, certification mode is disabled.
 *
 *  @param[in]  enabled  Whether or not certication mode is enabled.
 *  @param[out] err      Pointer to a variable to store Wireless Core error code.
 */
void swc_set_certification_mode(bool enabled, swc_error_t *const err);

/** @brief Enable/disable fast synchronization for low data rate links.
 *
 *  @note This feature is not supported in a dual radio configuration.
 *
 *  @note By default, fast sync is disabled.
 *
 *  @param[in]  enabled  Whether or not fast synchronization for low data rate links is supported.
 *  @param[out] err      Wireless Core error code.
 */
void swc_set_fast_sync(bool enabled, swc_error_t *const err);

/** @brief Advance configuration for concurrency mechanism.
 *
 *  @note By default, random channel sequence and DDCM are enabled, while RDO is disabled.
 *
 *  @param[in]  cfg  Concurrency configuration.
 *  @param[out] err  Wireless Core error code.
 */
void swc_set_concurrency_cfg(swc_concurrency_cfg_t cfg, swc_error_t *const err);

/** @brief Enable the usage of legacy SFDs for backward compatibility.
 *
 *  @param[in]  legacy_pan_id  The PAN ID use in old firmware.
 *  @param[out] err            Wireless Core error code.
 */
void swc_node_enable_legacy_sfd(uint16_t legacy_pan_id, swc_error_t *const err);

/** @brief Initializes a specified radio module within the given node.
 *
 *  @note This function configures a radio module with default settings and performs necessary initializations
 *        including power cycling, reading from non-volatile memory (NVM), and calibrating the radio according
 *        to these settings. This must be called for every radio module connected to the device. Most systems
 *        operate a single radio, but a dual radio configuration is also supported for specific use cases.
 *
 *
 *  @param[in]  radio_id   The identifier of the radio module to initialize.
 *  @param[in]  calibrate  Choose between re-calibrating the module afresh or using previously saved calibration data.
 *  @param[out] err        Wireless Core error code.
 */
void swc_radio_module_init(swc_radio_id_t radio_id, bool calibrate, swc_error_t *const err);

/** @brief Set the state of the radio IRQ pin when asserted.
 *
 *  @note This function must be invoked after `swc_radio_module_init` to take affect.
 *
 *  @note By default, SWC_IRQ_ACTIVE_HIGH is used if this function is not called.
 *
 *  @param[in]  radio_id      Radio number.
 *  @param[in]  irq_polarity  Radio's IRQ polarity.
 *  @param[out] err           Wireless Core error code.
 */
void swc_node_set_radio_irq_polarity(swc_radio_id_t radio_id, swc_irq_polarity_t irq_polarity, swc_error_t *const err);

/** @brief Set the radio's SPI interface timing setting.
 *
 *  @note This function must be invoked after `swc_radio_module_init` to take affect.
 *
 *  @note By default, SWC_SPI_FAST is used if this function is not called.
 *
 *  @param[in]  radio_id  Radio number.
 *  @param[in]  spi_mode  Radio's SPI mode.
 *  @param[out] err       Wireless Core error code.
 */
void swc_node_set_radio_spi_mode(swc_radio_id_t radio_id, swc_spi_mode_t spi_mode, swc_error_t *const err);

/** @brief Set the radio's digital output driver impedance.
 *
 *  @note This function must be invoked after `swc_radio_module_init` to take affect.
 *
 *  @note By default, SWC_OUTIMPED_2 is used if this function is not called.
 *
 *  @param[in]  radio_id  Radio number.
 *  @param[in]  outimped  Radio's digital output driver impedance.
 *  @param[out] err       Wireless Core error code.
 */
void swc_node_set_radio_outimped(swc_radio_id_t radio_id, swc_outimped_t outimped, swc_error_t *const err);

/** @brief Set the callback function to indicate that the Wireless Core is ready to enter to lower power mode.
 *
 *  @note By default, NULL is used if this function is not called, meaning no callback is invoked on transition to low
 *        power mode.
 *
 *  @param[in]  cb    Callback function.
 *  @param[out] err   Wireless Core error code.
 */
void swc_node_set_low_power_callback(void (*cb)(void *node), swc_error_t *const err);

/** @brief Set the sleep depth the transceiver will put itself when not active.
 *
 *  @note By default, SLEEP_IDLE is used if this function is not called.
 *
 *  @param[in]  sleep_level  Sleep level.
 *  @param[out] err          Wireless Core error code.
 */
void swc_node_set_sleep_level(swc_sleep_level_t sleep_level, swc_error_t *const err);

/** @brief Return state if low power mode can be activated.
 *
 *  @note Use swc_node_is_low_power_allowed function to find out whether low power mode can be active.
 *
 *  @param[out] err  Wireless Core error code.
 *  @return State if low power mode can be activated.
 */
bool swc_node_is_low_power_allowed(swc_error_t *const err);

/** @brief Get the radio's 64-bit serial number.
 *
 *  The serial number has been assigned during manufacturing and is unique
 *  among all SPARK transceivers of the same model.
 *
 *  @param[out] err  Wireless Core error code.
 *  @return Serial number.
 */
uint64_t swc_node_get_radio_serial_number(swc_error_t *const err);

/** @brief Get the radio's 8-bit product model.
 *
 *  The serial number has been assigned during manufacturing and is unique
 *  among all SPARK transceivers of the same model.
 *
 *  @param[out] err  Wireless Core error code.
 *  @return Product model.
 */
uint8_t swc_node_get_radio_product_model(swc_error_t *const err);

/** @brief Get the radio's 8-bit product version.
 *
 *  The serial number has been assigned during manufacturing and is unique
 *  among all SPARK transceivers of the same model.
 *
 *  @param[out] err  Wireless Core error code.
 *  @return Product version.
 */
uint8_t swc_node_get_radio_product_version(swc_error_t *const err);

/** @brief Get a formatted string of the radio's nvm content.
 *
 *  @param[in]  buffer  Buffer where to put the formatted string.
 *  @param[in]  size    Size of the buffer.
 *  @param[out] err     Wireless Core error code.
 *  @return The formated string length, excluding the NULL terminator.
 */
int swc_format_radio_nvm(char *const buffer, uint16_t size, swc_error_t *const err);

#if (WPS_RADIO_COUNT == 2)
/** @brief Get the second radio's 64-bit serial number.
 *
 *  The serial number has been assigned during manufacturing and is unique
 *  among all SPARK transceivers of the same model.
 *
 *  @param[out] err  Wireless Core error code.
 *  @return Serial number.
 */
uint64_t swc_node_get_radio2_serial_number(swc_error_t *const err);

/** @brief Get the second radio's 8-bit product model.
 *
 *  The serial number has been assigned during manufacturing and is unique
 *  among all SPARK transceivers of the same model.
 *
 *  @param[out] err  Wireless Core error code.
 *  @return Product model.
 */
uint8_t swc_node_get_radio2_product_model(swc_error_t *const err);

/** @brief Get the second radio's 8-bit product version.
 *
 *  The serial number has been assigned during manufacturing and is unique
 *  among all SPARK transceivers of the same model.
 *
 *  @param[out] err  Wireless Core error code.
 *  @return Product version.
 */
uint8_t swc_node_get_radio2_product_version(swc_error_t *const err);

/** @brief Get a formatted string of the second radio's nvm content.
 *
 *  @param[in]  buffer  Buffer where to put the formatted string.
 *  @param[in]  size    Size of the buffer.
 *  @param[out] err     Wireless Core error code.
 *  @return The formated string length, excluding the NULL terminator.
 */
int swc_format_radio2_nvm(char *const buffer, uint16_t size, swc_error_t *const err);
#endif

/** @brief Initialize a connection.
 *
 *  A connection abstracts a one-way data flow between 2 devices (e.g., a coordinator and a node).
 *
 *  @param[in]  cfg  Wireless connection configuration.
 *  @param[out] err  Wireless Core error code.
 *  @return Connection handle.
 */
swc_connection_t *swc_connection_init(swc_connection_cfg_t cfg, swc_error_t *const err);

/** @brief Get a beacon connection configuration.
 *
 *  @param[in] source_address  Source address/beacon transmitter's address.
 *  @param[in] timeslot_id     Array of timeslot IDs used by the connection.
 *  @param[in] timeslot_count  Number of timeslots used by the connection.
 *
 *  @return Pre-configured SWC connection configuration instance for a beacon.
 */
swc_connection_cfg_t swc_get_beacon_connection_config(uint8_t source_address, const int32_t *const timeslot_id,
                                                      uint8_t timeslot_count);

/** @brief Configure channels to use for a wireless connection.
 *
 *  @param[in]  conn  Connection handle.
 *  @param[in]  cfg   Wireless channel configuration.
 *  @param[out] err   Wireless Core error code.
 */
void swc_connection_add_channel(swc_connection_t *const conn, swc_channel_cfg_t cfg, swc_error_t *const err);

/** @brief Configure a fallback channel to use for a wireless connection.
 *
 *  @note This function can only be used on TX connections.
 *
 *  @param[in]  conn            Connection handle.
 *  @param[in]  main_cfg        Base channel configuration without fallback.
 *  @param[in]  cfg             Wireless channel configuration.
 *  @param[in]  channel_index   Channel index.
 *  @param[in]  fallback_index  Fallback index. 0 is the fallback with the highest payload size threshold.
 *  @param[out] err             Wireless Core error code.
 */
void swc_connection_add_fallback_channel(const swc_connection_t *const conn, swc_channel_cfg_t main_cfg,
                                         swc_fallback_channel_cfg_t cfg, uint8_t channel_index, uint8_t fallback_index,
                                         swc_error_t *const err);

/** @brief Set the callback function to execute after a successful transmission.
 *
 *  @note If ACKs are enabled, this callback is triggered when the ACK frame is received.
 *        If ACKs are disabled, it triggers every time the frame is sent (if the callback is configured).
 *
 *  @note Connection callback setter serve two functions:
 *        - They need to be called during connection initialization to let the SWC setup
 *          know the memory that need to be allocated for the callback.
 *        - They can be called at any time to change the callback function ONLY if they have been
 *          previously allocated during the SWC setup.
 *
 *  @note By default, NULL is used if this function is not called, meaning no callback is invoked on TX success.
 *
 *  @param[in]  conn  Connection handle.
 *  @param[in]  cb    Callback function.
 *  @param[in]  arg   Argument to pass to the callback function.
 *  @param[out] err   Wireless Core error code.
 */
void swc_connection_set_tx_success_callback(swc_connection_t *const conn, void (*cb)(void *conn, void *arg), void *arg,
                                            swc_error_t *const err);

/** @brief Set the callback function to execute after an unsuccessful transmission.
 *
 *  @note If ACKs are enabled, this callback is triggered if an ACK is not received after a transmission.
 *        If ACKs are disabled, it never triggers since every transmission is considered a success.
 *
 *  @note Connection callback setter serve two functions:
 *        - They need to be called during connection initialization to let the SWC setup
 *          know the memory that need to be allocated for the callback.
 *        - They can be called at any time to change the callback function ONLY if they have been
 *          previously allocated during the SWC setup.
 *
 *  @note By default, NULL is used if this function is not called, meaning no callback is invoked on TX fail.
 *
 *  @param[in]  conn  Connection handle.
 *  @param[in]  cb    Callback function.
 *  @param[in]  arg   Argument to pass to the callback function.
 *  @param[out] err   Wireless Core error code.
 */
void swc_connection_set_tx_fail_callback(swc_connection_t *const conn, void (*cb)(void *conn, void *arg), void *arg,
                                         swc_error_t *const err);

/** @brief Set the callback function to execute after the ARQ module discards a frame.
 *
 *  @note If ARQ is enabled, the frame can be dropped because the time deadline or the retry count deadline has been
 * reached. If ARQ is disabled, it never triggers.
 *
 *  @note Connection callback setter serve two functions:
 *        - They need to be called during connection initialization to let the SWC setup
 *          know the memory that need to be allocated for the callback.
 *        - They can be called at any time to change the callback function ONLY if they have been
 *          previously allocated during the SWC setup.
 *
 *  @note By default, NULL is used if this function is not called, meaning no callback is invoked on TX dropped.
 *
 *  @param[in]  conn  Connection handle.
 *  @param[in]  cb    Callback function.
 *  @param[in]  arg   Argument to pass to the callback function.
 *  @param[out] err   Wireless Core error code.
 */
void swc_connection_set_tx_dropped_callback(swc_connection_t *const conn, void (*cb)(void *conn, void *arg), void *arg,
                                            swc_error_t *const err);

/** @brief Set the callback function to execute after a successful frame reception.
 *
 *  @note A reception is considered successful when the frame destination address matches the
 *        local address or the broadcast address and the CRC checks.
 *
 *  @note Connection callback setter serve two functions:
 *        - They need to be called during connection initialization to let the SWC setup
 *          know the memory that need to be allocated for the callback.
 *        - They can be called at any time to change the callback function ONLY if they have been
 *          previously allocated during the SWC setup.
 *
 *  @note By default, NULL is used if this function is not called, meaning no callback is invoked on RX success.
 *
 *  @param[in]  conn  Connection handle.
 *  @param[in]  cb    Callback function.
 *  @param[in]  arg   Argument to pass to the callback function.
 *  @param[out] err   Wireless Core error code.
 */
void swc_connection_set_rx_success_callback(swc_connection_t *const conn, void (*cb)(void *conn, void *arg), void *arg,
                                            swc_error_t *const err);

/** @brief Set the callback function to execute after an event occur on the connection.
 *
 *  @note Use swc_get_event function to know which event occurred.
 *
 *  @note Connection callback setter serve two functions:
 *        - They need to be called during connection initialization to let the SWC setup
 *          know the memory that need to be allocated for the callback.
 *        - They can be called at any time to change the callback function ONLY if they have been
 *          previously allocated during the SWC setup.
 *
 *  @note By default, NULL is used if this function is not called, meaning no callback is invoked on event (ex:
 *        connect or disconnect).
 *
 *  @param[in]  conn  Connection handle.
 *  @param[in]  cb    Callback function.
 *  @param[in]  arg   Argument to pass to the callback function.
 *  @param[out] err   Wireless Core error code.
 */
void swc_connection_set_event_callback(swc_connection_t *const conn, void (*cb)(void *conn, void *arg), void *arg,
                                       swc_error_t *const err);

/** @brief Optimize latency in target connection.
 *
 *  @note This feature delays the wakeup of the transceiver each time the queue is empty,
 *        allowing more time for the application context to generate a frame. This results
 *        in an overall better latency by reducing the minimum time it takes for the SWC
 *        to process a frame.
 *
 *  @note The delay resulting from this feature is equal to the sum of the CCA time and the
 *        air time of the frame, which include the auto-reply part. The CCA time is based on
 *        the cca_try_count and the cca_retry_time, which are part of the connection_cfg.
 *
 *  @note This feature can't be enabled when certification mode is enabled since it modify the
 *        wake up of the radio which is critical to properly emulate acknowledge.
 *
 *  @param[in]  conn                     Connection handle.
 *  @param[in]  auto_reply_payload_size  Size of the payload in the auto-reply(if any).
 *  @param[out] err                      Wireless Core error code.
 */
void swc_connection_optimized_latency(const swc_connection_t *const conn, uint8_t auto_reply_payload_size,
                                      swc_error_t *const err);

#if !WPS_DISABLE_FRAGMENTATION
/** @brief Enable fragmentation on the target connection.
 *
 *  @note This needs to be implemented on both sides of the connection so that
 *        the transmitter (TX) can fragment the packet, and the receiver (RX)
 *        can aggregate the received payload.
 *
 *  @note Once this feature is enabled, users will be able to send payloads
 *        larger than the provided maximum payload size during connection configuration
 *        when using the swc_connection_send method.
 *
 *  @note By default, fragmentation is disabled. Fragmentation compilation flag must be enabled to use this feature.
 *
 *  @param[in]  conn  Connection handle.
 *  @param[out] err   Wireless Core error code.
 */
void swc_connection_set_fragmentation(const swc_connection_t *const conn, swc_error_t *const err);
#endif

/** @brief Enable/disable ACK exchange on target connection.
 *
 *  @note This need to be enable on both device (TX and RX) in order to have
 *        proper ACK exchange.
 *
 *  @note To send payload inside the ACK of a RX connection, configure a TX connection
 *        that share the same timeslot as the RX connection and make sure that the AUTO_TIMESLOT macro
 *        is used when defining the timeslot.
 *
 *  @note To receive payload inside the ACK of TX connection, configure a RX connection
 *        that share the same timeslot as the TX connection and make sure that the AUTO_TIMESLOT macro
 *        is used when defining the timeslot.
 *
 *  @note By default, a main timeslot has an acknowledge. An auto-reply never has an acknowledge.
 *
 *  @param[in]  conn     Connection handle.
 *  @param[in]  enabled  Whether or not ACK frames are sent (RX connection) or receive (TX connection).
 *  @param[out] err      Wireless Core error code.
 */
void swc_connection_set_acknowledgement(const swc_connection_t *const conn, bool enabled, swc_error_t *const err);

/** @brief Enable/disable credit flow control on target connection.
 *
 *  @note This function must be called after swc_connection_set_acknowledgement.
 *
 *  @note By default, credit flow control is disabled.
 *
 *  @param[in]  conn     Connection handle.
 *  @param[in]  enabled  Whether or not credit flow control frames is enabled on target connection.
 *  @param[out] err      Wireless Core error code.
 */
void swc_connection_set_credit_flow_ctrl(const swc_connection_t *const conn, bool enabled, swc_error_t *const err);

/** @brief Enable retransmission of frame with or without condition to drop the frame.
 *
 *  @note Setting `try_deadline` and `time_deadline` to 0 will result in a Guaranteed delivery transmission mode.
 *
 *  @note Do nothing if ACK is disabled using `swc_connection_set_acknowledgement`.
 *
 *  @note The time increment for the parameter `time_deadline` is in number of tick from the
 *        get_tick() hal function.
 *
 *  @note By default, the retransmission is set to 0, which means infinite retries (guaranteed delivery).
 *
 *  @param[in]  conn           Connection handle.
 *  @param[in]  enabled        Whether or not retransmission are enabled on target connection.
 *  @param[in]  try_deadline   Maximum number of tries (0 is infinite) before a frame is dropped.
 *  @param[in]  time_deadline  Maximum amount of time (0 is infinite) before a frame is dropped.
 *  @param[out] err            Wireless Core error code.
 */
void swc_connection_set_retransmission(const swc_connection_t *const conn, bool enabled, uint32_t try_deadline,
                                       uint32_t time_deadline, swc_error_t *const err);

/** @brief Return the event notified by the event callback.
 *
 *  @param[in] conn  Connection handle.
 *  @return Event.
 */
swc_event_t swc_get_event(const swc_connection_t *const conn);

/** @brief Return the error notified by the event callback.
 *
 *  @param[in] conn  Connection handle.
 *  @return Error.
 */
swc_error_t swc_get_event_error(const swc_connection_t *const conn);

/** @brief Set the percentage of allocated timeslots to use.
 *
 *  @note The throttling feature reduces the usable bandwidth in order to reduce power consumption.
 *        By default, the active ratio is set to 100%. For example. if a ratio of 50% is set, only
 *        1 timeslot out of 2 will be usable by the connection. The transceiver will stay asleep
 *        in unused timeslots.
 *
 *  @param[in]  conn          Connection handle.
 *  @param[in]  active_ratio  Percentage of the allocated timeslots to use, from 0 to 100.
 *  @param[out] err           Wireless Core error code.
 */
void swc_connection_set_throttling_active_ratio(const swc_connection_t *const conn, uint8_t active_ratio,
                                                swc_error_t *const err);

/** @brief Set the frame forward error correction (FEC) ratio for a connection.
 *
 *  @note By default, SWC_FEC_1_5_0_0 is used if this function is not called.
 *
 *  @param[in]  conn   Connection handle.
 *  @param[in]  ratio  FEC ratio.
 *  @param[out] err    Wireless Core error code.
 */
void swc_connection_set_fec_ratio(const swc_connection_t *const conn, swc_fec_ratio_t ratio, swc_error_t *const err);

/** @brief Set the connection's modulation.
 *
 *  @note By default, SWC_MOD_IOOK is used if this function is not called.
 *
 *  @param[in]  conn        Connection handle.
 *  @param[in]  modulation  Connection's modulation.
 *  @param[out] err         Wireless Core error code.
 */
void swc_connection_set_modulation(const swc_connection_t *const conn, swc_modulation_t modulation,
                                   swc_error_t *const err);

/** @brief Enable/disable auto-sync mode.
 *
 *  @note By default, auto-sync is disabled.
 *
 *  @param[in]  conn     Connection handle.
 *  @param[in]  enabled  Whether or not auto-sync is supported on the connection.
 *  @param[out] err      Wireless Core error code.
 */
void swc_connection_set_auto_sync(const swc_connection_t *const conn, bool enabled, swc_error_t *const err);

/** @brief Enable connection data rate throttling.
 *
 *  @note By default, throttling is disabled.
 *
 *  @param[in]  conn  Connection handle.
 *  @param[out] err   Wireless Core error code.
 */
void swc_connection_set_throttling(const swc_connection_t *const conn, swc_error_t *const err);

/** @brief Enable connection priority on a connection.
 *
 *  @note The same priority is used for all the assigned slots of the connection.
 *        'priority' and 'slots_priority' cannot be used at the same time.
 *
 *  @note The connection priority feature should be the last API call to configure the connection.
 *        Everything that is set after this call will result in an error.
 *
 *  @note By default, there is no priority between the connections. The first connection added in a timeslot will always
 *        have the highest priority.
 *
 *  @param[in]  conn      Connection handle.
 *  @param[in]  priority  Connections' priority.
 *  @param[out] err       Wireless Core error code.
 */
void swc_connection_set_connection_priority(swc_connection_t *const conn, uint8_t priority, swc_error_t *const err);

/** @brief Enable slots priority on a connection.
 *
 *  @note Priorities for the particular timeslots is used by the connection.
 *        'priority' and 'slots_priority' cannot be used at the same time.
 *
 *  @note The slots priority feature should be the last API call to configure the connection.
 *        Everything that is set after this call will result in an error.
 *
 *  @note By default, this type of priority is not used.
 *
 *  @param[in]  conn            Connection handle.
 *  @param[in]  slots_priority  Connections's timeslots priorities.
 *  @param[out] err             Wireless Core error code.
 */
void swc_connection_set_connection_slots_priority(swc_connection_t *const conn, const uint8_t *slots_priority,
                                                  swc_error_t *const err);

/** @brief Advance concurrency configuration for connections.
 *
 *  @note By default, concurrency mechanisms are enabled when certification mode is disabled.
 *
 *  @param[in]  conn  Connection handle.
 *  @param[in]  cfg   Concurrency configuration.
 *  @param[out] err   Wireless Core error code.
 */
void swc_connection_set_concurrency_cfg(const swc_connection_t *const conn,
                                        const swc_connection_concurrency_cfg_t *const cfg, swc_error_t *const err);

/** @brief Set the connection's fallback configuration.
 *
 *  @note By default, fallback is disabled.
 *
 *  @param[in]  conn  Connection handle.
 *  @param[in]  cfg   Fallback configuration.
 *  @param[out] err   Wireless Core error code.
 */
void swc_connection_set_fallback_cfg(swc_connection_t *const conn, const swc_connection_fallback_cfg_t *const cfg,
                                     swc_error_t *const err);

/** @brief Enable or disable sending of sync frame on syncing when the connection queue
 *         is not empty.
 *
 *  @note This prevents the device from polluting the channel with full packet when
 *        unsync.
 *
 *  @note This feature is enabled by default and disabled when doing certification.
 *
 *  @note By default, sync frames will be sent when syncing without consuming actual data.
 *
 *  @param[in]  conn     SPARK Wireless Core connection structure.
 *  @param[in]  enabled  Enable or disable the feature.
 *  @param[out] err      Wireless Core error code.
 */
void swc_connection_set_tx_sync_frame_on_syncing(const swc_connection_t *const conn, bool enabled,
                                                 swc_error_t *const err);

/** @brief Set sleep level for each individual time slots.
 *
 *  @note By default, the sleep level is IDLE.
 *
 *  @param[in]  sleep_level  Sleep level table.
 *  @param[out] err          Wireless Core error code.
 */
void swc_set_time_slots_sleep_level(const swc_sleep_level_t *const sleep_level, swc_error_t *const err);

/** @brief Get a buffer from the connection queue.
 *
 *  This function allocates a buffer of the predefined maximum payload size, regardless of the current packet size.
 *
 *  @param[in]  conn            Connection handle.
 *  @param[out] payload_buffer  Free payload buffer if available, NULL otherwise.
 *  @param[out] err             Wireless Core error code.
 */
void swc_connection_get_payload_buffer(const swc_connection_t *const conn, uint8_t **const payload_buffer,
                                       swc_error_t *const err);

/** @brief Allocate a buffer from the connection queue with required size.
 *
 *  This function treats the queue as a continuous memory block, allocating only the exact memory required for each
 *  packet. The pointer progresses within the queue's memory space as packets are allocated one after another. This
 *  behavior could allow an application to split its payload into multiple packets to reduce their size.
 *
 *  Since the pointer advances by variable sizes, the allocated memory might not be aligned to the max payload size.
 *  For example, if the queue is sized for two 64-byte packets but the first is 2 bytes and the second is 64 bytes,
 *  freeing the 2-byte packet only leaves space for 62 bytes due to the 2-byte offset of the second packet. The queue
 *  fails to allocate memory for the third packet even if total memory is available.
 *
 *  @param[in]  conn            Connection handle.
 *  @param[out] payload_buffer  Free payload buffer with required size if available, NULL otherwise.
 *  @param[in]  payload_size    Required size of the buffer.
 *  @param[out] err             Wireless Core error code.
 */
void swc_connection_allocate_payload_buffer(const swc_connection_t *const conn, uint8_t **const payload_buffer,
                                            uint16_t payload_size, swc_error_t *const err);

/** @brief Enqueue a payload buffer in the connection transmission queue.
 *
 *  @param[in]  conn            Connection handle.
 *  @param[in]  payload_buffer  Buffer containing the payload to transmit.
 *  @param[in]  size            Size of the payload.
 *  @param[out] err             Wireless Core error code.
 */
void swc_connection_send(const swc_connection_t *const conn, const uint8_t *const payload_buffer, uint16_t size,
                         swc_error_t *const err);

/** @brief Retrieve a payload buffer from the connection reception queue.
 *
 *  @param[in]  conn            Connection handle.
 *  @param[in]  payload_buffer  Address of the buffer where to put the payload.
 *  @param[out] err             Wireless Core error code.
 *  @return Size of the payload.
 */
uint16_t swc_connection_receive(const swc_connection_t *const conn, uint8_t **const payload_buffer,
                                swc_error_t *const err);

/** @brief Get the payload size from the connection reception queue.
 *
 *  @param[in]  conn  Connection handle.
 *  @param[out] err   Wireless Core error code.
 *  @return Size of the received payload.
 */
uint16_t swc_connection_receive_get_payload_size(const swc_connection_t *const conn, swc_error_t *const err);

/** @brief Free the last received payload buffer from the connection reception queue.
 *
 *  @param[in]  conn  Connection handle.
 *  @param[out] err   Wireless Core error code.
 */
void swc_connection_receive_complete(const swc_connection_t *const conn, swc_error_t *const err);

/** @brief The reserved addresses are unlocked and can be used.
 */
void swc_reserved_address_unlock(void);

/** @brief The reserved addresses are locked and can't be used.
 */
void swc_reserved_address_lock(void);

/** @brief Copy the received frame to the payload buffer and free the queue.
 *
 *  @param[in]  conn     Connection handle.
 *  @param[out] payload  Pointer to the output buffer.
 *  @param[in]  size     Size of the output buffer.
 *  @param[in]  err      Wireless Core error code.
 *  @return Size of the payload.
 */
uint16_t swc_connection_receive_to_buffer(const swc_connection_t *const conn, uint8_t *const payload, uint16_t size,
                                          swc_error_t *const err);

/** @brief Get the number of frame in the connection queue.
 *
 *  @param[in] conn  Connection handle.
 *  @param[in] err   Wireless Core error code.
 *  @return The number of frame in the connection queue.
 */
uint16_t swc_connection_get_enqueued_count(const swc_connection_t *const conn, swc_error_t *const err);

/** @brief Return if the connection is connected or disconnected.
 *
 *  @param[in] conn  Connection handle.
 *  @param[in] err   Wireless Core error code.
 *  @return True if the connection is connected, False otherwise.
 */
bool swc_connection_get_connect_status(const swc_connection_t *const conn, swc_error_t *const err);

/** @brief Flush the packets in the connection's queue.
 *
 *  @note This function can only be ran when the wireless core is disconnected.
 *
 *  @param[in]  conn  Connection handle.
 *  @param[out] err   Wireless Core error code.
 */
void swc_connection_flush_queue(const swc_connection_t *conn, swc_error_t *err);

/** @brief Wireless Core setup.
 *
 *  This is the last API call that needs to be made when initializing and
 *  configuring the Wireless Core.
 *
 *  @param[out] err  Wireless Core error code.
 */
void swc_setup(swc_error_t *const err);

/** @brief Get the wireless core status.
 *
 *  @return The wireless core status.
 */
swc_status_t swc_get_status(void);

/** @brief Start Wireless Core process.
 *
 *  This is called once the Wireless Core initialization and configuration
 *  is done (i.e., after swc_setup()).
 *
 *  @param[out] err  Wireless Core error code.
 */
void swc_connect(swc_error_t *const err);

/** @brief Stop Wireless Core process.
 *
 *  @param[out] err  Wireless Core error code.
 */
void swc_disconnect(swc_error_t *const err);

/** @brief Get information used when the fallback mode is enabled.
 *
 *  This is used by a node receiving data through a connection
 *  with fallback enabled. The information retrieved here must
 *  be sent back to the original sender usually through an
 *  auto-reply timeslot.
 *
 *  @param[in]  conn  Connection handle.
 *  @param[out] err   Wireless Core error code.
 *  @return Fallback information.
 */
swc_fallback_info_t swc_connection_get_fallback_info(const swc_connection_t *const conn, swc_error_t *const err);

/** @brief Get the number of bytes allocated in the memory pool.
 *
 *  @return Number of bytes allocated in the memory pool.
 */
uint32_t swc_get_allocated_bytes(void);

/** @brief Free the memory to reconfigure the wireless core.
 */
void swc_free_memory(void);

/** @brief Function to call to process the Wireless Core callback queue.
 *
 *  The callbacks are TX ACK, TX NACK and RX. It is suggested to configure
 *  a software interrupt (SWI) and put this function into its IRQ handler.
 *  The Wireless Core can then trigger it with a call to the context_switch()
 *  function from its HAL structure.
 */
void swc_connection_callbacks_processing_handler(void);

/** @brief Send a tx flush request to the connection.
 *
 *  @param[in] conn  SWC connection.
 */
void swc_send_tx_flush_request(const swc_connection_t *const conn);

/** @brief Set PHY mode.
 *
 *  @note By default, the radio has a 20.48 MHz PHY rate and 0 ISI mitigation pause. This function is only used when the
 *        dynamic PHY rate feature is enabled.
 *
 *  @param[in]  phy_mode  Requested PHY mode.
 *  @param[out] err       Wireless Core error code.
 */
void swc_set_phy_mode(swc_phy_mode_t phy_mode, swc_error_t *const err);

/** @brief Get the current PHY mode used by the MAC layer.
 *
 *  @param[out] err  Wireless Core error code.
 *  @return Current PHY mode.
 */
swc_phy_mode_t swc_get_current_phy_mode(swc_error_t *const err);

/** @brief Get the current chip rate used by the MAC layer.
 *
 *  @param[out] err  Wireless Core error code.
 *  @return Current chip rate.
 */
swc_chip_rate_t swc_get_current_chip_rate(swc_error_t *err);

/** @brief Get the current ISI mitigation used by the MAC layer.
 *
 *  @param[out] err  Wireless Core error code.
 *  @return Current ISI mitigation.
 */
swc_isi_mitig_t swc_get_current_isi_mitig(swc_error_t *err);

#if (WPS_RADIO_COUNT == 1)
/** @brief Function to call in the external interrupt handler servicing the radio IRQ.
 */
void swc_radio_irq_handler(void);

/** @brief Function to call in the radio DMA SPI receive complete IRQ handler.
 */
void swc_radio_spi_receive_complete_handler(void);
#elif (WPS_RADIO_COUNT == 2)
/** @brief Function to call in the external interrupt handler servicing radio #1 IRQ.
 */
void swc_radio1_irq_handler(void);

/** @brief Function to call in radio #1 DMA SPI receive complete IRQ handler.
 */
void swc_radio1_spi_receive_complete_handler(void);

/** @brief Function to call in the external interrupt handler servicing radio #2 IRQ.
 */
void swc_radio2_irq_handler(void);

/** @brief Function to call in radio #2 DMA SPI receive complete IRQ handler.
 */
void swc_radio2_spi_receive_complete_handler(void);

/** @brief Callback for the Radio sync timer.
 */
void swc_radio_synchronization_timer_callback(void);

/** @brief Set the Multi-Radio selection mode to use.
 *
 *  @note By default, MULTI_RADIO_SELECT_MODE_ALGO is used which enables automatic radio selection.
 *
 *  @param[in]  multi_radio_select_mode  Multi-Radio selection mode.
 *  @param[out] err                      Wireless Core error code.
 */
void swc_set_multi_radio_select_mode(multi_radio_select_mode_t multi_radio_select_mode, swc_error_t *const err);

/** @brief Get the Multi-Radio selection mode.
 *
 *  @param[out] err  Wireless Core error code.
 *  @return Multi-Radio Selection mode.
 */
multi_radio_select_mode_t swc_get_multi_radio_select_mode(swc_error_t *const err);
#else
#error "Number of radios must be either 1 or 2"
#endif

/** @brief Links the necessary functions to allow the wireless core interact with the radio(s).
 *
 *  @retval true  Link success.
 *  @retval false Link fail.
 */
bool swc_config_hardware_interface(void);

#ifdef __cplusplus
}
#endif

#endif /* SWC_API_H_ */
