/** @file sr_radio_model.h
 *  @brief SR radio model and package defines.
 *
 *  Radio model and package descriptions.
 *
 *  @copyright Copyright (C) 2022 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is confidential and proprietary.
 *  @author    SPARK FW Team.
 */
#ifndef SR_RADIO_MODEL_H_
#define SR_RADIO_MODEL_H_

/* MACROS *********************************************************************/
#define KEY_TO_INDEX(k)                  (k - 1)
#define IS_SR1120(phy_model)             (phy_model == PHY_MODEL_SR1120)
#define IS_SR1020(phy_model)             (phy_model == PHY_MODEL_SR1020)
#define IS_SR1010(phy_model)             (phy_model == PHY_MODEL_SR1010)
#define IS_QFN28(phy_package)            (phy_package == PHY_PACKG_QFN28)
#define IS_QFN32(phy_package)            (phy_package == PHY_PACKG_QFN32)
#define IS_QFN48(phy_package)            (phy_package == PHY_PACKG_QFN48)
#define CHIP_ID_IS_BELOW(value, chip_id) (chip_id < value)

/* TYPES **********************************************************************/
typedef enum phy_model {
    /*! SR1010 phy model */
    PHY_MODEL_SR1010 = 0,
    /*! SR1020 phy model */
    PHY_MODEL_SR1020 = 1,
    /*! SR1120 phy model */
    PHY_MODEL_SR1120 = 2,
} phy_model_t;

typedef enum phy_package {
    /*! QFN28 package */
    PHY_PACKG_QFN28 = 0,
    /*! QFN48 package */
    PHY_PACKG_QFN48 = 1,
    /*! QFN32 package */
    PHY_PACKG_QFN32 = 2,
} phy_package_t;


#endif /* SR_RADIO_MODEL_H_ */
