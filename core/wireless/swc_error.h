/** @file  swc_error.h
 *  @brief SPARK Wireless Core error codes.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SWC_ERROR_H_
#define SWC_ERROR_H_

/* CONSTANTS ******************************************************************/
/* Current __COUNTER__ value. */
enum { SWC_COUNTER_BASE = __COUNTER__ };

/* MACROS *********************************************************************/
#ifndef ASSERT_SWC_STATUS
/** @brief ASSERT the return status of a SWC function.
 */
#define ASSERT_SWC_STATUS(swc_status)        \
    do {                                     \
        if (swc_status == SWC_ERR_NONE) {    \
            /* Exit early. */                \
            break;                           \
        }                                    \
                                             \
        if (swc_status > 0) {                \
            /* Handle warning */             \
            swc_warning_handler(swc_status); \
            break;                           \
        }                                    \
                                             \
        /* Handle error */                   \
        swc_error_handler(swc_status);       \
    } while (0)
#endif

/** @brief Create a local counter macro.
 */
#define SWC_COUNTER (__COUNTER__ - SWC_COUNTER_BASE)

/** @brief Automatically increase error count each time invoked. Starts at -1.
 */
#define SWC_GENERATE_ERR_CODE (-SWC_COUNTER)

/* TYPES **********************************************************************/
/** @brief Wireless API error structure.
 */
typedef enum swc_error {
    /*! No error nor warning occurred. */
    SWC_ERR_NONE = 0,

    /*! Warnings (positive values) */
    /*! The configured TX pulse count could potentially cause unexpected behavior. Use at your own risk */
    SWC_WARN_TX_PULSE_COUNT,
    /*! The configured TX pulse count offset could potentially cause unexpected behavior. Use at your own risk */
    SWC_WARN_TX_PULSE_COUNT_OFFSET,
    /*! The configured RX pulse count could potentially cause unexpected behavior. Use at your own risk */
    SWC_WARN_RX_PULSE_COUNT,
    /*! There is no more payload buffer available from the queue */
    SWC_WARN_NO_BUFFER_AVAILABLE,
    /*! The Wireless Core is already connected. */
    SWC_WARN_ALREADY_CONNECTED,
    /*! The queue of the sender is full */
    SWC_WARN_SEND_QUEUE_FULL,
    /*! New payload received and dropped because the RX queue is full */
    SWC_WARN_RX_OVERRUN,
    /*! The Wireless Core is already disconnected. */
    SWC_WARN_ALREADY_DISCONNECTED,

    /*! Errors (negative values) */
    /*! The Wireless Core is not initialized */
    SWC_ERR_NOT_INITIALIZED = SWC_GENERATE_ERR_CODE,
    /*! Not enough memory is allocated by the application for a full Wireless Core initialization */
    SWC_ERR_NOT_ENOUGH_MEMORY = SWC_GENERATE_ERR_CODE,
    /*! A NULL pointer is passed as argument */
    SWC_ERR_NULL_PTR = SWC_GENERATE_ERR_CODE,
    /*! Fast sync and dual radio are enabled but are incompatible */
    SWC_ERR_FAST_SYNC_WITH_DUAL_RADIO = SWC_GENERATE_ERR_CODE,
    /*! The configured PAN ID is invalid */
    SWC_ERR_PAN_ID = SWC_GENERATE_ERR_CODE,
    /*! The configured network role is invalid */
    SWC_ERR_NETWORK_ROLE = SWC_GENERATE_ERR_CODE,
    /*! The configured sleep level is invalid */
    SWC_ERR_SLEEP_LEVEL = SWC_GENERATE_ERR_CODE,
    /*! The configured IRQ polarity is invalid */
    SWC_ERR_IRQ_POLARITY = SWC_GENERATE_ERR_CODE,
    /*! The configured SPI mode is invalid */
    SWC_ERR_SPI_MODE = SWC_GENERATE_ERR_CODE,
    /*! The configured modulation is invalid */
    SWC_ERR_MODULATION = SWC_GENERATE_ERR_CODE,
    /*! The configured FEC ratio is invalid */
    SWC_ERR_FEC_RATIO = SWC_GENERATE_ERR_CODE,
    /*! The configured CCA retry time is bigger than the maximum allowed */
    SWC_ERR_CCA_RETRY_TIME = SWC_GENERATE_ERR_CODE,
    /*! The configured CCA try count is bigger than the maximum allowed */
    SWC_ERR_CCA_TRY_COUNT = SWC_GENERATE_ERR_CODE,
    /*! The configured CCA fail action is invalid */
    SWC_ERR_CCA_FAIL_ACTION = SWC_GENERATE_ERR_CODE,
    /*! The configured CCA parameters are invalid */
    SWC_ERR_CCA_INVALID_PARAMETERS = SWC_GENERATE_ERR_CODE,
    /*! The configured local address is invalid */
    SWC_ERR_LOCAL_ADDRESS = SWC_GENERATE_ERR_CODE,
    /*! The configured source address is invalid */
    SWC_ERR_SOURCE_ADDRESS = SWC_GENERATE_ERR_CODE,
    /*! The configured destination address is invalid */
    SWC_ERR_DESTINATION_ADDRESS = SWC_GENERATE_ERR_CODE,
    /*! None of the connection's addresses match with the local device's address */
    SWC_ERR_CONNECTION_ADDRESS = SWC_GENERATE_ERR_CODE,
    /*! ARQ is enabled while ACK is not */
    SWC_ERR_ARQ_WITH_ACK_DISABLED = SWC_GENERATE_ERR_CODE,
    /*! Link throttling is disabled on this connection */
    SWC_ERR_THROTTLING_NOT_SUPPORTED = SWC_GENERATE_ERR_CODE,
    /*! The configured TX pulse width is invalid */
    SWC_ERR_TX_PULSE_WIDTH = SWC_GENERATE_ERR_CODE,
    /*! The configured TX pulse width offset is invalid */
    SWC_ERR_TX_PULSE_WIDTH_OFFSET = SWC_GENERATE_ERR_CODE,
    /*! The configured TX pulse gain is invalid */
    SWC_ERR_TX_PULSE_GAIN = SWC_GENERATE_ERR_CODE,
    /*! The configured TX pulse gain offset is invalid */
    SWC_ERR_TX_GAIN_OFFSET = SWC_GENERATE_ERR_CODE,
    /*! A channel is added on a connection using only auto-reply timeslots */
    SWC_ERR_ADD_CHANNEL_ON_INVALID_CONNECTION = SWC_GENERATE_ERR_CODE,
    /*! There is an internal Wireless Core error */
    SWC_ERR_INTERNAL = SWC_GENERATE_ERR_CODE,
    /*! The Wireless Core is not connected. */
    SWC_ERR_NOT_CONNECTED = SWC_GENERATE_ERR_CODE,
    /*! The Wireless Core failed to disconnect within the timeout value. */
    SWC_ERR_DISCONNECT_TIMEOUT = SWC_GENERATE_ERR_CODE,
    /*! The configured payload size exceeds the maximum value for the current connection configuration. */
    SWC_ERR_PAYLOAD_TOO_BIG = SWC_GENERATE_ERR_CODE,
    /*! Dual radio is used but swc_node_radio_add() is not called twice */
    SWC_ERR_SECOND_RADIO_NOT_INIT = SWC_GENERATE_ERR_CODE,
    /*! The function call is not supported when the frame fragmentation is enabled on the connection.
     *  swc_connection_receive_to_buffer() should be used instead.
     */
    SWC_ERR_FRAGMENTATION_NOT_SUPPORTED = SWC_GENERATE_ERR_CODE,
    /*! The configured output driver impedance is invalid */
    SWC_ERR_OUTIMPED = SWC_GENERATE_ERR_CODE,
    /*! User tried to call a TX connection function on a RX connection. */
    SWC_ERR_TX_CONN_ACTION_ON_RX_CONN = SWC_GENERATE_ERR_CODE,
    /*! Input parameter is out of acceptable value */
    SWC_ERR_ZERO_TIMESLOT_SEQ_LEN = SWC_GENERATE_ERR_CODE,
    /*! Zero value was given to channel sequence length */
    SWC_ERR_ZERO_CHAN_SEQ_LEN = SWC_GENERATE_ERR_CODE,
    /*! Minimum queue size requirement not met */
    SWC_ERR_MIN_QUEUE_SIZE = SWC_GENERATE_ERR_CODE,
    /*! Zero was given to timeslot count */
    SWC_ERR_ZERO_TIMESLOT_COUNT = SWC_GENERATE_ERR_CODE,
    /*! Zero was given as timeslot duration for 1 or more timeslots */
    SWC_ERR_NULL_TIMESLOT_DURATION = SWC_GENERATE_ERR_CODE,
    /*! User tried to change configuration while the SWC is running */
    SWC_ERR_CHANGING_CONFIG_WHILE_RUNNING = SWC_GENERATE_ERR_CODE,
    /*! The payload sent is greater than the available space */
    SWC_ERR_SIZE_TOO_BIG = SWC_GENERATE_ERR_CODE,
    /*! The queue of the receiver is empty */
    SWC_ERR_RECEIVE_QUEUE_EMPTY = SWC_GENERATE_ERR_CODE,
    /*! The maximum number of connections assigned to the time slot was already reached */
    SWC_ERR_TIMESLOT_CONN_LIMIT_REACHED = SWC_GENERATE_ERR_CODE,
    /*! Some connection fields must be identical in order for these connections to be used on the same time slot using
     *  the connection priority feature.
     */
    SWC_ERR_NON_MATCHING_SAME_TIMESLOT_CONN_FIELD = SWC_GENERATE_ERR_CODE,
    /*! The priority of the connection is higher than the maximum priority. */
    SWC_ERR_MAX_CONN_PRIORITY = SWC_GENERATE_ERR_CODE,
    /*! The provided buffer size is too small to execute the operation. */
    SWC_ERR_BUFFER_SIZE_TOO_SMALL = SWC_GENERATE_ERR_CODE,
    /*! The one of the specified parameter/s has an invalid value. */
    SWC_ERR_INVALID_PARAMETER = SWC_GENERATE_ERR_CODE,
    /*! The priority configuration of the connection is not allowed. */
    SWC_ERR_NOT_ALLOWED_CONN_PRIORITY_CONFIGURATION = SWC_GENERATE_ERR_CODE,
    /*! Connection priority feature is not enabled on each connection sharing the same timeslot. */
    SWC_ERR_PRIO_NOT_ENABLE_ON_ALL_CONN = SWC_GENERATE_ERR_CODE,
    /*! The computed delay for the latency optimization feature is too high. */
    SWC_ERR_OPTIMIZATION_DELAY_TO_HIGH = SWC_GENERATE_ERR_CODE,
    /*! The configured chip rate is invalid. */
    SWC_ERR_CHIP_RATE = SWC_GENERATE_ERR_CODE,
    /*! Credit flow control is enabled while ACK is not */
    SWC_ERR_CREDIT_FLOW_CTRL_WITH_ACK_DISABLED = SWC_GENERATE_ERR_CODE,
    /*! The context switch trigger has not been initialized. */
    SWC_ERR_CONTEXT_SWITCH_TRIGGER_IS_NULL = SWC_GENERATE_ERR_CODE,
    /*! The Wireless core doesn't support acknowledge on an auto-reply connection. */
    SWC_ERR_ACK_NOT_SUPPORTED_IN_AUTO_REPLY_CONNECTION = SWC_GENERATE_ERR_CODE,
    /*! The radio ID provided is out of range. */
    SWC_ERR_RADIO_ID_INVALID = SWC_GENERATE_ERR_CODE,
    /*! No saved calibration data found in HEAP; calibration required. */
    SWC_ERR_CALIBRATION_MISSING = SWC_GENERATE_ERR_CODE,
    /*! Radio model not found. */
    SWC_ERR_RADIO_NOT_FOUND = SWC_GENERATE_ERR_CODE,
    /*! Sleep level for one of the time slots is incorrect. */
    SWC_ERR_INVALID_TIMESLOT_SLEEP_LEVEL = SWC_GENERATE_ERR_CODE,
    /*! Operation must be done before SWC setup. */
    SWC_ERR_INVALID_OPERATION_AFTER_SETUP = SWC_GENERATE_ERR_CODE,
    /*! No channels are initialized in one of the connections */
    SWC_ERR_NO_CHANNEL_INIT = SWC_GENERATE_ERR_CODE,
    /*! Selected channel is out of range for the current radio model. */
    SWC_ERR_CHANNEL_OUT_OF_RANGE = SWC_GENERATE_ERR_CODE,
    /*! PHY mode was requested while the feature is disabled. */
    SWC_ERR_DYNAMIC_PHY_MODE_DISABLED = SWC_GENERATE_ERR_CODE,
    /*! Connection hasn't been allocated and the pointer is null */
    SWC_ERR_UNINITIALIZED_CONNECTION = SWC_GENERATE_ERR_CODE,
    /*! Failed to create connection due to internal error.*/
    SWC_ERR_CONNECTION_CREATION_FAILED = SWC_GENERATE_ERR_CODE,
    /*! Failed to configure frame due to internal error.*/
    SWC_ERR_FRAME_CONFIGURATION_FAILED = SWC_GENERATE_ERR_CODE,
    /*! Failed to set callback due to internal error.*/
    SWC_ERR_FAILED_TO_SET_CALLBACK = SWC_GENERATE_ERR_CODE,
    /*! The node instance hasn't been initialized before. */
    SWC_ERR_NODE_NOT_INITIALIZED = SWC_GENERATE_ERR_CODE,
    /*! Network ID doesn't match the Network ID set during node init. */
    SWC_ERR_NOT_MATCHING_NETWORK_ID = SWC_GENERATE_ERR_CODE,
    /*! User tried to flush a connection's queue while the SWC is running */
    SWC_ERR_FLUSH_QUEUE_WHILE_RUNNING = SWC_GENERATE_ERR_CODE,
    /*! Connection settings cannot be modified after the connection priority or slot priority
     *  has been initialized. Ensure that these feature API calls are the last ones made
     *  when configuring a connection.
     */
    SWC_ERR_INVALID_OPERATION_AFTER_SWC_LOCK = SWC_GENERATE_ERR_CODE,
    /* Slot priority feature cannot be used if connection priority is enabled. */
    SWC_ERR_INVALID_OPERATION_CONN_PRIO_ENABLED = SWC_GENERATE_ERR_CODE,
    /*! Connection priority feature cannot be used if slot priority is enabled. */
    SWC_ERR_INVALID_OPERATION_SLOT_PRIO_ENABLED = SWC_GENERATE_ERR_CODE,
    /*! User tried to call an RX connection function on a TX connection. */
    SWC_ERR_RX_CONN_ACTION_ON_TX_CONN = SWC_GENERATE_ERR_CODE,
} swc_error_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Handle an SWC error.
 *
 *  @note This function has a weak definition in swc_error.c. The function can be redifined in the application to change
 *        the error handling behaviour.
 *
 *  @param[in] swc_status  SWC status code.
 */
void swc_error_handler(swc_error_t swc_status);

/** @brief Handle an SWC warning.
 *
 *  @note This function has a weak definition in swc_error.c. The function can be redifined in the application to change
 *        the warning handling behaviour.
 *
 *  @param[in] swc_status  SWC status code.
 */
void swc_warning_handler(swc_error_t swc_status);

#endif /* SWC_ERROR_H_ */
