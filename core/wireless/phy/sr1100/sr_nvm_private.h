/** @file sr_nvm_private.h
 *  @brief SR non-volatile memory private module.
 *
 *  Functions related to writing the NVM and to its protocol.
 *
 *  @copyright Copyright (C) 2020-2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is confidential and proprietary.
 *  @author    SPARK FW Team.
 */
#ifndef SR_NVM_PRIVATE_H_
#define SR_NVM_PRIVATE_H_

/* INCLUDES *******************************************************************/
#include <stdint.h>
#include "sr_def.h"
#include "sr_nvm.h"

/* TYPES **********************************************************************/
/** @brief NVM structure.
 *
 */
typedef struct {
    /*! Enable NVM VDD power supply  */
    void (*enable_vdd)(void);
    /*! Disable NVM VDD power supply */
    void (*disable_vdd)(void);
} nvm_vdd_hal_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
void sr_nvm_write(radio_t *radio, nvm_vdd_hal_t *vdd, uint8_t *buf, uint8_t addr_start, uint8_t addr_end);

#endif /* SR1000_NVM_PRIVATE_H_ */
