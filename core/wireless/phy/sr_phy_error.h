/** @file  sr_phy_error.h
 *  @brief SPARK Radio phy error codes.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef SR_PHY_ERROR_H_
#define SR_PHY_ERROR_H_

/* TYPES **********************************************************************/
typedef enum sr_phy_error {

    /******************************************************************************/
    /*                         Access Sequence error                              */
    /******************************************************************************/

    /*! No error */
    ACCESS_SEQUENCE_ERR_NONE = 0,
    /*! RX buffer null error */
    ACCESS_SEQUENCE_ERR_INVALID_RX_BUFFER,
    /*! TX buffer null error */
    ACCESS_SEQUENCE_ERR_INVALID_TX_BUFFER,
    /*! Sequence overflow error */
    ACCESS_SEQUENCE_ERR_TOO_MUCH_QUEUED_TRANSACTION,
    /*! Append after burst mode err */
    ACCESS_SEQUENCE_ERR_CANT_APPEND_IN_BURST_MODE,
    /*! Empty sequence error */
    ACCESS_SEQUENCE_ERR_EMPTY_SEQUENCE,
    /*! Busy error */
    ACCESS_SEQUENCE_ERR_BUSY,

    /******************************************************************************/
    /*                                 Spectral error                             */
    /******************************************************************************/

    /*! Spectral No error */
    SR_SPECTRAL_ERROR_NONE,
    /*! TX power is greater than 3. Valid range is 0-3.*/
    SR_SPECTRAL_ERROR_INVALID_TX_POWER,
    /*! Resulting pulse position needs to be within range 0-9. */
    SR_SPECTRAL_ERROR_INVALID_SPACING,
    /*! Maximum number of pulse cfg is 3. */
    SR_SPECTRAL_ERROR_INVALID_PULSE_CFG,

    /******************************************************************************/
    /*                                 Phy model error                            */
    /******************************************************************************/

    /*! No error */
    PHY_MODEL_ERROR_NONE,
    /*! Radio phy model not found. */
    PHY_MODEL_NOT_FOUND,
} sr_phy_error_t;

#endif /* SR_PHY_ERROR_H_ */
