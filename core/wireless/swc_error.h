/** @file  swc_error.h
 *  @brief SPARK Wireless Core error codes.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SWC_ERROR_H_
#define SWC_ERROR_H_

/* TYPES **********************************************************************/
/** @brief Wireless API error structure.
 */
typedef enum swc_error {
    /*! No error occurred */
    SWC_ERR_NONE = 0,
    /*! The Wireless Core is not initialized */
    SWC_ERR_NOT_INITIALIZED,
    /*! Not enough memory is allocated by the application for a full Wireless Core initialization */
    SWC_ERR_NOT_ENOUGH_MEMORY,
    /*! A NULL pointer is passed as argument */
    SWC_ERR_NULL_PTR,
    /*! Fast sync and dual radio are enabled but are incompatible */
    SWC_ERR_FAST_SYNC_WITH_DUAL_RADIO,
    /*! The configured PAN ID is invalid */
    SWC_ERR_PAN_ID,
    /*! The configured network role is invalid */
    SWC_ERR_NETWORK_ROLE,
    /*! The configured sleep level is invalid */
    SWC_ERR_SLEEP_LEVEL,
    /*! The configured IRQ polarity is invalid */
    SWC_ERR_IRQ_POLARITY,
    /*! The configured SPI mode is invalid */
    SWC_ERR_SPI_MODE,
    /*! The configured modulation is invalid */
    SWC_ERR_MODULATION,
    /*! The configured FEC ratio is invalid */
    SWC_ERR_FEC_RATIO,
    /*! The configured CCA fail action is invalid */
    SWC_ERR_CCA_FAIL_ACTION,
    /*! The configured CCA threshold is invalid */
    SWC_ERR_CCA_THRESHOLD,
    /*! The configured CCA parameters are invalid */
    SWC_ERR_CCA_INVALID_PARAMETERS,
    /*! The configured local address is invalid */
    SWC_ERR_LOCAL_ADDRESS,
    /*! The configured source address is invalid */
    SWC_ERR_SOURCE_ADDRESS,
    /*! The configured destination address is invalid */
    SWC_ERR_DESTINATION_ADDRESS,
    /*! None of the connection's addresses match with the local device's address */
    SWC_ERR_CONNECTION_ADDRESS,
    /*! ARQ is enabled while ACK is not */
    SWC_ERR_ARQ_WITH_ACK_DISABLED,
    /*! Link throttling is enabled on an RX connection */
    SWC_ERR_THROTTLING_ON_RX_CONNECTION,
    /*! Link throttling is disabled on this connection */
    SWC_ERR_THROTTLING_NOT_SUPPORTED,
    /*! Payload memory allocation is not enabled on a RX connection */
    SWC_ERR_NO_PAYLOAD_MEM_ALLOC_ON_RX_CONNECTION,
    /*! The configured TX pulse count is invalid */
    SWC_ERR_TX_PULSE_COUNT,
    /*! The configured TX pulse count offset is invalid */
    SWC_ERR_TX_PULSE_COUNT_OFFSET,
    /*! The configured TX pulse width is invalid */
    SWC_ERR_TX_PULSE_WIDTH,
    /*! The configured TX pulse width offset is invalid */
    SWC_ERR_TX_PULSE_WIDTH_OFFSET,
    /*! The configured TX pulse gain is invalid */
    SWC_ERR_TX_PULSE_GAIN,
    /*! The configured TX pulse gain offset is invalid */
    SWC_ERR_TX_GAIN_OFFSET,
    /*! The configured RX pulse count is invalid */
    SWC_ERR_RX_PULSE_COUNT,
    /*! There is no more payload buffer available from the queue */
    SWC_ERR_NO_BUFFER_AVAILABLE,
    /*! A channel is added on a connection using only auto-reply timeslots */
    SWC_ERR_ADD_CHANNEL_ON_INVALID_CONNECTION,
    /*! There is an internal Wireless Core error */
    SWC_ERR_INTERNAL,
    /*! The Wireless Core is already connected. */
    SWC_ERR_ALREADY_CONNECTED,
    /*! The Wireless Core is not connected. */
    SWC_ERR_NOT_CONNECTED,
    /*! The Wireless Core failed to disconnect within the timeout value. */
    SWC_ERR_DISCONNECT_TIMEOUT,
    /*! The configured payload size exceeds the maximum value for the current connection configuration. */
    SWC_ERR_PAYLOAD_TOO_BIG,
    /*! Dual radio is used but swc_node_radio_add() is not called twice */
    SWC_ERR_SECOND_RADIO_NOT_INIT,
    /*! The function call is not supported when the frame fragmentation is enable on the connection.
     *  swc_connection_receive_to_buffer() should be use instead.
     */
    SWC_ERR_FRAGMENTATION_NOT_SUPPORTED,
    /*! The configured output driver impedance is invalid */
    SWC_ERR_OUTIMPED,
    /*! User tried to send on a connection that can't send */
    SWC_ERR_SEND_ON_RX_CONN,
    /*! Input parameter is out of acceptable value */
    SWC_ERR_ZERO_TIMESLOT_SEQ_LEN,
    /*! Zero value was given to channel sequence length */
    SWC_ERR_ZERO_CHAN_SEQ_LEN,
    /*! Minimum queue size requirement not met */
    SWC_ERR_MIN_QUEUE_SIZE,
    /*! Zero was given to timeslot count */
    SWC_ERR_ZERO_TIMESLOT_COUNT,
    /*! Zero was given as timeslot duration for 1 or more timeslots */
    SWC_ERR_NULL_TIMESLOT_DURATION,
    /*! User tried to change configuration while the SWC is running */
    SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING,
    /*! The queue of the sender is full */
    SWC_ERR_SEND_QUEUE_FULL,
    /*! The payload sent is greater than the available space */
    SWC_ERR_SIZE_TOO_BIG,
    /*! The queue of the receiver is empty */
    SWC_ERR_RECEIVE_QUEUE_EMPTY,
    /*! New payload received and dropped because the RX queue is full */
    SWC_ERR_RX_OVERRUN,
    /*! The maximum number of connection assigned to the time slot was already reached */
    SWC_ERR_TIMESLOT_CONN_LIMIT_REACHED,
    /*! The connection priority is not equal to zero while the connection ID protocol is disabled. */
    SWC_ERR_NON_ZERO_PRIORITY_WITHOUT_CONN_ID,
    /*! Some connection fields must be identical in order for these connections to be used on the same time slot using
     *  the connection priority feature.
     */
    SWC_ERR_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD,
    /*! The multi radio hal initialization is invalid. */
    SWC_ERR_MULTI_RADIO_HAL_INVALID,
    /*! The priority of the connection is higher than the maximum priority. */
    SWC_ERR_MAX_CONN_PRIORITY,
    /*! The provided buffer size is too small to execute the operation. */
    SWC_ERR_BUFFER_SIZE_TOO_SMALL,
    /*! The one of the specified parameter/s has an invalid value. */
    SWC_ERR_INVALID_PARAMETER,
    /*! The priority configuration of the connection is not allowed. */
    SWC_ERR_NOT_ALLOWED_CONN_PRIORITY_CONFIGURATION,
    /*! Connection priority feature is not enable on each connection sharing the same timeslot. */
    SWC_ERR_PRIO_NOT_ENABLE_ON_ALL_CONN,
    /*! The computed delay for the latency optimization feature is too high. */
    SWC_ERR_OPTIMIZATION_DELAY_TO_HIGH,
    /*! The configured chip rate is invalid. */
    SWC_ERR_CHIP_RATE,
    /*! The ranging feature is not supported. */
    SWC_ERR_RANGING_NOT_SUPPORTED,
    /*! Credit low control is enabled while ACK is not */
    SWC_ERR_CREDIT_FLOW_CTRL_WITH_ACK_DISABLED,
    /*! The context switch trigger has not been initialized. */
    SWC_ERR_CONTEXT_SWITCH_TRIGGER_IS_NULL,
    /*! The Wireless core doesn't support acknowledge on anauto-reply connection. */
    SWC_ERR_ACK_NOT_SUPPORTED_IN_AUTO_REPLY_CONNECTION,
    /*! Ranging settings are added on a TX connection or a connection using a main timeslot. */
    SWC_ERR_ADD_RANGING_SETTINGS_ON_INVALID_CONNECTION,
    /*! The radio ID provided is out of range. */
    SWC_ERR_RADIO_ID_INVALID,
    /*! No saved calibration data found in HEAP; calibration required. */
    SWC_ERR_CALIBRATION_MISSING,
    /*! Radio model not found. */
    SWC_ERR_RADIO_NOT_FOUND,
    /*! Sleep level for one of the time slots is incorrect. */
    SWC_ERR_INCORRECT_TS_SLEEP_LEVEL,
    /*! Pulse configuration for 27MHz is not valid */
    SWC_ERR_INVALID_PULSE_CONFIG_27M,
    /*! Operation must be done before SWC setup. */
    SWC_ERR_INVALID_OPERATION_AFTER_SETUP,
    /*! No channel are initialized in one of the connections */
    SWC_ERR_NO_CHANNEL_INIT
} swc_error_t;

#endif /* SWC_ERROR_H_ */
