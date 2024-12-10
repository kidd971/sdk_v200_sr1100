/** @file  wps_error.h
 *  @brief Wireless Protocol Stack error codes.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef WPS_ERROR_H_
#define WPS_ERROR_H_

/* TYPES **********************************************************************/
/** @brief WPS error enum definition.
 */
typedef enum wps_err {
    /*! No error */
    WPS_NO_ERROR = 0,
    /*! The WPS is not initialized.*/
    WPS_NOT_INIT_ERROR,
    /*! The WPS have received a frame, but the RX queue was full. Packet is discard and overrun is returned. */
    WPS_RX_OVERRUN_ERROR,
    /*! PHY state machine has received a signal that is not handled. */
    WPS_PHY_CRITICAL_ERROR,
    /*! There is no new frame to dequeue from the WPS connection. */
    WPS_QUEUE_EMPTY_ERROR,
    /*! The WPS connection queue is full, new frame have not been enqueued. */
    WPS_QUEUE_FULL_ERROR,
    /*! The size of the frame is wrong. */
    WPS_WRONG_TX_SIZE_ERROR,
    /*! The size of the frame is wrong. */
    WPS_WRONG_RX_SIZE_ERROR,
    /*! Connection throttle is not initialized. */
    WPS_CONN_THROTTLE_NOT_INITIALIZED_ERROR,
    /*! The WPS is already connected. */
    WPS_ALREADY_CONNECTED_ERROR,
    /*! The WPS is already disconnected. */
    WPS_ALREADY_DISCONNECTED_ERROR,
    /*! Channel sequence is not initialized. */
    WPS_CHANNEL_SEQUENCE_NOT_INITIALIZED_ERROR,
    /*! Radio is not initialized. Call wps_radio_init function. */
    WPS_RADIO_NOT_INITIALIZED_ERROR,
    /*! Acknowledge has to be enabled to use the Stop and Wait module. */
    WPS_ACK_DISABLED_ERROR,
    /*! WPS write request queue is full */
    WPS_WRITE_REQUEST_QUEUE_FULL,
    /*! WPS read request queue is full */
    WPS_READ_REQUEST_QUEUE_FULL,
    /*! WPS request queue is full */
    WPS_REQUEST_QUEUE_FULL,
    /*! Write request issued transfer to large */
    WPS_WRITE_REQUEST_XFER_TOO_LARGE,
    /*! Error during MAC fragmentation */
    WPS_FRAGMENT_ERROR,
    /*! Connection event */
    WPS_CONNECT_EVENT,
    /*! Disonnection event */
    WPS_DISCONNECT_EVENT,
    /*! Disconnection timeout error */
    WPS_DISCONNECT_TIMEOUT_ERROR,
    /*! The timeslot connection table is full */
    WPS_TIMESLOT_CONN_LIMIT_REACHED_ERROR,
    /*! WPS schedule ratio request queue is full */
    WPS_SCHEDULE_RATIO_REQUEST_QUEUE_FULL,
    /*! Not enough memory error */
    WPS_NOT_ENOUGH_MEMORY_ERROR,
    /*! CCA settings is invalid */
    WPS_INVALID_CCA_SETTINGS,
    /*! An error have occurred when initializing the network channel sequence. */
    WPS_CHANNEL_SEQUENCE_INIT_ERROR,
} wps_error_t;

#endif /* WPS_ERROR_H_ */
