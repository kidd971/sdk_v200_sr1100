/** @file  swc_api_extension.h
 *  @brief SPARK Wireless Core Application Programming Interface extension.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SWC_API_EXTENSION_H_
#define SWC_API_EXTENSION_H_

/* INCLUDES *******************************************************************/
#include "swc_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
typedef enum swc_ext_error {
    /*! No error occurred. */
    SWC_EXT_ERR_NONE = 0,
    /*! User tried to change configuration while the SWC is running. */
    SWC_EXT_ERR_CHANGING_CONFIG_WHILE_RUNNING,
    /*! A NULL pointer is passed as argument. */
    SWC_EXT_ERR_NULL_PTR,
    /*! The call of a wps function returned an error. */
    SWC_EXT_ERR_INTERNAL,
    /*! A channel is added on a connection using only auto-reply timeslots. */
    SWC_EXT_ERR_ADD_CHANNEL_ON_INVALID_CONNECTION,
    /*! The configured TX pulse count is invalid. */
    SWC_EXT_ERR_TX_PULSE_COUNT,
    /*! The configured TX pulse width is invalid. */
    SWC_EXT_ERR_TX_PULSE_WIDTH,
    /*! The configured TX pulse gain is invalid. */
    SWC_EXT_ERR_TX_PULSE_GAIN,
    /*! The configured RX pulse count is invalid. */
    SWC_EXT_ERR_RX_PULSE_COUNT,
    /*! The syncword index is out of range. */
    SWC_EXT_ERR_SYNCWORD_INDEX_OUT_OF_RANGE,
    /*! The syncword length is invalid. */
    SWC_EXT_ERR_SYNCWORD_LENGTH_INVALID,
    /*! The preamble length is out of range. */
    SWC_EXT_ERR_PREAMBLE_LENGTH_OUT_OF_RANGE,
    /*! The crc value is out of range. */
    SWC_EXT_ERR_CRC_VALUE_OUT_OF_RANGE,
    /*! The sync word bit cost is out of range. */
    SWC_EXT_ERR_SYNC_WORD_BIT_COST_OUT_OF_RANGE,
    /*! The sync word tolerance is out of range. */
    SWC_EXT_ERR_SYNC_WORD_TOLERANCE_OUT_OF_RANGE,
    /*! The CCA Treshold is out of range. */
    SWC_EXT_ERR_CCA_TRESHOLD_OUT_OF_RANGE,
    /*! The RDO increment steps is bigger than the total number of pll steps. */
    SWC_EXT_ERR_RDO_INC_STEP_INVALID,
    /*! The DDCM max timeslot value is out of range. */
    SWC_EXT_ERR_DDCM_MAX_TIMESLOT_OFFSET_OUT_OF_RANGE,
    /*! The DDCM sync loss value is out of range. */
    SWC_EXT_ERR_DDCM_SYNC_LOSS_VALUE_OUT_OF_RANGE,
    /*! The fallback threshold count is out of range. */
    SWC_EXT_ERR_FBK_THRESHOLD_COUNT_OUT_OF_RANGE,
    /*! The receiver gain value is out of range. */
    SWC_EXT_ERR_RX_GAIN_OUT_OF_RANGE,
    /*! The read request queue is full. */
    SWC_EXT_ERR_READ_REQUEST_QUEUE_FULL,
    /*! The write request queue is full. */
    SWC_EXT_ERR_WRITE_REQUEST_QUEUE_FULL,
    /*! The read/write requested address is out of range.*/
    SWC_EXT_ERR_REQUESTED_ADDRESS_REGISTER_OUT_OF_RANGE,
    /*! The requested radio is not available. */
    SWC_EXT_ERR_REQUESTED_RADIO_UNAVAILABLE,
    /*! The calibration process has not been initiated yet. */
    SWC_EXT_ERR_CALIBRATION_NOT_DONE,
    /*! The connection is not initialized. */
    SWC_EXT_ERR_UNINITIALIZED_CONNECTION,
    /*! The channel is not initialized. */
    SWC_EXT_ERR_UNINITIALIZED_CHANNEL,
} swc_ext_error_t;

typedef enum swc_ext_warning {
    /*! No warning occurred. */
    SWC_EXT_WARNING_NONE = 0,
    /*! Warning a pulse count of zero has been set to a transmitting connection, no transmission will be done. */
    SWC_EXT_WARNING_ZERO_PULSE_COUNT = (1 << 0),
    /*! Warning a pulse count is higher than usual, might result in bad RF communications. */
    SWC_EXT_WARNING_HIGH_PULSE_COUNT = (1 << 1),
    /*! Warning a value is out of regular value used. */
    SWC_EXT_WARNING_INTEGGAIN_VALUE_OUT_OF_LOOKUP_TABLE = (1 << 2),
} swc_ext_warning_t;

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Set the sync word index of the SPARK Wireless node.
 *
 *  @param[in] node          SPARK Wireless Core node structure.
 *  @param[in] syncword_idx  Sync word index.
 *  @param[out] err          SWC API extension error.
 *  @param[out] warn         SWC API extension warning.
 */
void swc_ext_set_syncword(const swc_node_t *const node, uint32_t sync_word_idx, swc_ext_error_t *err,
                          swc_ext_warning_t *warn);

/** @brief Set the sync word length of the SPARK Wireless node.
 *
 *  @param[in] node             SPARK Wireless Core node structure.
 *  @param[in] syncword_length  Sync word length.
 *  @param[out] err             SWC API extension error.
 *  @param[out] warn            SWC API extension warning.
 */
void swc_ext_set_syncword_length(const swc_node_t *const node, syncword_length_t syncword_length, swc_ext_error_t *err,
                                 swc_ext_warning_t *warn);

/** @brief Set the preamble length of the SPARK Wireless node.
 *
 *  @param[in] node          SPARK Wireless Core node structure.
 *  @param[in] preamble_len  Preamble length.
 *  @param[out] err          SWC API extension error.
 *  @param[out] warn         SWC API extension warning.
 */
void swc_ext_set_preamble_length(const swc_node_t *const node, uint32_t preamble_length, swc_ext_error_t *err,
                                 swc_ext_warning_t *warn);

/** @brief Set the CRC of the SPARK Wireless node.
 *
 *  @param[in] node   SPARK Wireless Core node structure.
 *  @param[in] crc    CRC value.
 *  @param[out] err   SWC API extension error.
 *  @param[out] warn  SWC API extension warning.
 */
void swc_ext_set_crc(const swc_node_t *const node, uint32_t crc, swc_ext_error_t *err, swc_ext_warning_t *warn);

/** @brief Enable SPARK Wireless Core random data rate offset.
 *
 *  @param[in] total_number_of_pll_steps  Total number of PLL steps.
 *  @param[in] increment_steps            Number of increment step between each iteration.
 *  @param[out] err                       SWC API extension error.
 *  @param[out] warn                      SWC API extension warning.
 */
void swc_ext_enable_rdo(uint32_t total_number_of_pll_steps, uint32_t increment_steps, swc_ext_error_t *err,
                        swc_ext_warning_t *warn);

/** @brief Disable SPARK Wireless Core random data rate offset.
 *
 *  @param[out] err   SWC API extension error.
 *  @param[out] warn  SWC API extension warning.
 */
void swc_ext_disable_rdo(swc_ext_error_t *err, swc_ext_warning_t *warn);

/** @brief Enable SPARK Wireless Core transmission jitter.
 *
 *  @param[in] node   SPARK Wireless Core node structure.
 *  @param[out] err   SWC API extension error.
 *  @param[out] warn  SWC API extension warning.
 */
void swc_ext_enable_tx_jitter(const swc_node_t *const node, swc_ext_error_t *err, swc_ext_warning_t *warn);

/** @brief Disable SPARK Wireless Core transmission jitter.
 *
 *  @param[in] node   SPARK Wireless Core node structure.
 *  @param[out] err   SWC API extension error.
 *  @param[out] warn  SWC API extension warning.
 */
void swc_ext_disable_tx_jitter(const swc_node_t *const node, swc_ext_error_t *err, swc_ext_warning_t *warn);

/** @brief Set and enable SPARK Wireless Core reception gain for a connection.
 *
 *  @param[in] conn     SPARK Wireless Core connection structure.
 *  @param[in] rx_gain  Reception amplifier gain.
 *  @param[out] err     SWC API extension error.
 *  @param[out] warn    SWC API extension warning.
 */
void swc_ext_connection_disable_gain_loop(const swc_connection_t *const conn, uint8_t rx_gain, swc_ext_error_t *err,
                                          swc_ext_warning_t *warn);

/** @brief Disable SPARK Wireless Core reception fixed gain for a connection, thus enabling gain loop.
 *
 *  @param[in] conn   SPARK Wireless Core connection structure.
 *  @param[out] err   SWC API extension error.
 *  @param[out] warn  SWC API extension warning.
 */
void swc_ext_connection_enable_gain_loop(const swc_connection_t *const conn, swc_ext_error_t *err,
                                         swc_ext_warning_t *warn);

/** @brief Disable PLL SPARK Wireless Core node.
 *
 *  @param[in] node   SPARK Wireless Core node structure.
 *  @param[out] err   SWC API extension error.
 *  @param[out] warn  SWC API extension warning.
 */
void swc_ext_pll_disable(const swc_node_t *const node, swc_ext_error_t *err, swc_ext_warning_t *warn);

/** @brief Enable PLL SPARK Wireless Core node.
 *
 *  @param[in] node   SPARK Wireless Core node structure.
 *  @param[out] err   SWC API extension error.
 *  @param[out] warn  SWC API extension warning.
 */
void swc_ext_pll_enable(const swc_node_t *const node, swc_ext_error_t *err, swc_ext_warning_t *warn);

/** @brief Enable XTAL clock SPARK Wireless Core node.
 *
 *  @param[in] node   SPARK Wireless Core node structure.
 *  @param[out] err   SWC API extension error.
 *  @param[out] warn  SWC API extension warning.
 */
void swc_ext_xtal_enable(swc_node_t *node, swc_ext_error_t *err, swc_ext_warning_t *warn);

/** @brief Disable XTAL clock SPARK Wireless Core node.
 *
 *  @param[in] node   SPARK Wireless Core node structure.
 *  @param[out] err   SWC API extension error.
 *  @param[out] warn  SWC API extension warning.
 */
void swc_ext_xtal_disable(const swc_node_t *const node, swc_ext_error_t *err, swc_ext_warning_t *warn);

/** @brief Set a channel configuration on a connection.
 *
 *  @param[in] node          SPARK Wireless Core node structure.
 *  @param[in] conn          SPARK Wireless Core connection structure to apply new channel.
 *  @param[in] new_channel   New channel configuration structure.
 *  @param[in] chan_idx      Channel Index.
 *  @param[out] err          SWC API extension error.
 *  @param[out] warn         SWC API extension warning.
 *  @param[in] fallback_idx  Configure the fallback channel if true.
 */
void swc_ext_set_connection_channel(const swc_node_t *const node, const swc_connection_t *const conn,
                                    channel_cfg_t new_channel, uint8_t chan_idx, uint8_t fallback_idx,
                                    swc_ext_error_t *err, swc_ext_warning_t *warn);

/** @brief Set the number of most significant bit of the address to use for network id purposes.
 *
 *  @param[in] msbits_count  Most significant bits count.
 *  @param[out] err          SWC API extension error.
 *  @param[out] warn         SWC API extension warning.
 */
void swc_ext_set_network_msbit_count(uint8_t msbits_count, swc_ext_error_t *err, swc_ext_warning_t *warn);

/** @brief Set the chip rate of the SPARK Wireless Core node.
 *
 *  @param[in] node       SPARK Wireless Core node structure.
 *  @param[in] chip_rate  Chip rate value.
 *  @param[out] err       SWC API extension error.
 *  @param[out] warn      SWC API extension warning.

 */
void swc_ext_set_chip_rate(swc_node_t *node, chip_rate_cfg_t chip_rate, swc_ext_error_t *err, swc_ext_warning_t *warn);

/** @brief Return integgain value based on radio chip rate, modulation and pulse_count
 *
 *  @param[in] chip_rate    Radio PHY Rate.
 *  @param[in] pulse_count  Channel pulse count.
 *  @param[out] err         SWC API extension error.
 *  @param[out] warn        SWC API extension warning.
 *  @return Channel integgain.
 */
uint8_t swc_ext_get_integgain(chip_rate_cfg_t chip_rate, uint8_t pulse_count, swc_ext_error_t *err,
                              swc_ext_warning_t *warn);

/** @brief Enable Distributed De-syncronization Concurrency Mechanism.
 *
 *  @param[in] max_timeslot_offset         Maximum offset to apply every timeslot in pll cycles.
 *  @param[in] sync_loss_max_duration_pll  Maximum sync lost pll cycles before applying unsync tx offset.
 *  @param[out] err                        SWC API extension error.
 *  @param[out] warn                       SWC API extension warning.
 */
void swc_ext_enable_ddcm(uint16_t max_timeslot_offset, uint32_t sync_loss_max_duration_pll, swc_ext_error_t *err,
                         swc_ext_warning_t *warn);

/** @brief Disable Distributed De-syncronization Concurrency Mechanism.
 *
 *  @param[out] err   SWC API extension error.
 *  @param[out] warn  SWC API extension warning.
 */
void swc_ext_disable_ddcm(swc_ext_error_t *err, swc_ext_warning_t *warn);

/** @brief Enable fallback mechanism.
 *
 *  @note Fallback can have multiple layer.
 *
 *  @param[in] conn             Connection to apply fallback on.
 *  @param[in] threshold        Threshold array for each fallback layer.
 *  @param[in] threshold_count  Number of Threshold fallback layer.
 *  @param[out] err             SWC API extension error.
 *  @param[out] warn            SWC API extension warning.
 */
void swc_ext_enable_fallback(const swc_connection_t *const conn, const uint8_t *const threshold,
                             uint8_t threshold_count, swc_ext_error_t *err, swc_ext_warning_t *warn);

/** @brief Disable fallback mechanism.
 *
 *  @param[in] conn             Connection to apply fallback on.
 *  @param[out] err             SWC API extension error.
 *  @param[out] warn            SWC API extension warning.
 */
void swc_ext_disable_fallback(const swc_connection_t *const conn, swc_ext_error_t *err, swc_ext_warning_t *warn);

/** @brief Set CCA threshold.
 *
 *  @param[in] conn           Connection to apply CCA threshold on.
 *  @param[in] cca_threshold  CCA threshold.
 *  @param[in] err            SWC API extension error.
 *  @param[in] warn           SWC API extension warning.
 */
void swc_ext_set_cca_treshold(const swc_connection_t *const conn, const uint8_t cca_threshold, swc_ext_error_t *err,
                              swc_ext_warning_t *warn);

/** @brief Set fallback CCA try count.
 *
 *  @param[in] conn             Connection to apply fallback on.
 *  @param[in] fbk_try_count    CCA fallback try count.
 *  @param[out] err             SWC API extension error.
 *  @param[out] warn            SWC API extension warning.
 */
void swc_ext_set_fallback_cca_try_count(const swc_connection_t *const conn, const uint8_t *const fbk_try_count,
                                        swc_ext_error_t *err, swc_ext_warning_t *warn);

/** @brief Request a read radio register on the WPS.
 *
 *  @note Next time WPS has done preparing the
 *        timeslot, the request will be enqueued so
 *        that the following SPI transfer will
 *        contain the read request The request don't
 *        work if the SWC is not running.
 *
 *  @param[in]  target_register  Target radio register.
 *  @param[out] rx_buffer        Buffer containing register data.
 *  @param[out] xfer_cmplt       Bool to notify app that transfer is complete.
 *  @param[out] err              SWC API extension error.
 */
void swc_ext_request_register_read(uint8_t target_register, uint16_t *const rx_buffer, bool *const xfer_cmplt,
                                   swc_ext_error_t *err);

/** @brief Request a write register on the WPS.
 *
 *  @note Next time WPS has done preparing the
 *        timeslot, the request will be enqueued so
 *        that the following SPI transfer will
 *        contain the write request if cfg is WRITE_ONCE.
 *        If cfg is WRITE_PERIODIC, the register will be
 *        writen every time slot with the value.
 *
 *  @param[in] target_register  Starting radio register.
 *  @param[in] data             Byte to send.
 *  @param[in] cfg              Write config.
 *  @param[out] err             SWC API extension error.
 */
void swc_ext_request_register_write(uint8_t target_register, uint16_t data, reg_write_cfg_t cfg, swc_ext_error_t *err);

/** @brief Clear periodic register write.
 *
 *  @note Clear periodic write register queue.
 *
 *  @param[out] err  SWC API extension error.
 */
void swc_ext_clear_register_write(swc_ext_error_t *err);

/** @brief Return calibration variable of target radio.
 *
 *  @param[in]  radio_num  Radio number.
 *  @param[out] err        SWC API extension error.
 *  @return  Calibration variable structure.
 */
calib_vars_t *swc_ext_get_calib_vars(uint8_t radio_num, swc_ext_error_t *err);

/** @brief Return the radio channel structure for a given connection.
 *
 *  @param[in]  connection    Connection to get the channel info.
 *  @param[in]  channel_num   Channel number.
 *  @param[in]  fallback_idx  Fallback index number.
 *  @param[in]  radio_num     Radio number.
 *  @param[out] err           SWC API extension error.
 *  @return  Channel info structure.
 */
rf_channel_t *swc_ext_connection_get_channel_info(const swc_connection_t *const connection, uint8_t channel_num,
                                                  uint8_t fallback_idx, uint8_t radio_num, swc_ext_error_t *err);

/** @brief Return number of configured channel.
 *
 *  @param[out] err  SWC API extension error.
 *  @return  Number of unique configured channel from channel sequence.
 */
uint8_t swc_ext_get_number_of_configured_channel(swc_ext_error_t *err);

#ifdef __cplusplus
}
#endif

#endif /* SWC_API_EXTENSION_H_ */
