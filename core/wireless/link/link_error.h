/** @file  link_error.h
 *  @brief SPARK Radio link error codes.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef LINK_ERROR_H_
#define LINK_ERROR_H_

/* TYPES **********************************************************************/
typedef enum link_error {
    /*! No error */
    LINK_PROTO_NO_ERROR = 0,
    /*! No buffer provided during initialization */
    LINK_PROTO_NO_BUFFER,
    /*! Cant add more than 10 protocol in the link protocol */
    LINK_PROTO_TOO_MANY_PROTO,
    /*! No more space in the provided buffer when adding protocol */
    LINK_PROTO_NO_MORE_SPACE,
} link_error_t;

#endif /* LINK_ERROR_H_ */
