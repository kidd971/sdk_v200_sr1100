/** @file link_random_datarate_offset.h
 *  @brief Random datarate offset(RDO) algorithm.
 *
 *  This algorithm is use for the concurrency to delay the sync value
 *  between device. It is use by the WPS Layer 2 internal connection.
 *  The output value of this algorithm is sent between device WPS.
 *
 *  How it work : The current device received a random datarate offset
 *                from another WPS and use this value when updating the
 *                TDMA timeslot time. The value increment every timeslot
 *                and reset when given rollover value is met.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef LINK_RANDOM_DATARATE_OFFSET_H_
#define LINK_RANDOM_DATARATE_OFFSET_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief Random datarate offset module instance.
 */
typedef struct link_rdo {
    /*! Current offset value, in PLL cycle. */
    uint16_t offset;
    /*! Offset rollover value. */
    uint16_t rollover_n;
    /*! Number of timeslot between each offset increment. */
    uint16_t increment_step;
    /*! Offset value split for uint8_t transfer. */
    uint8_t offset_u8[sizeof(uint16_t)];
    /*! RDO enable flag. */
    bool enabled;
} link_rdo_t;

/* PUBLIC GLOBALS *************************************************************/
/** @brief Initialize the RDO module.
 *
 *  @note When RDO is disable, every call to the link_rdo_get_offset will
 *        return 0.
 *  @note Rollover value should be higher than 0. If 0 is input, the default
 *        rollover value will be set (15).
 *
 *  @param[in] link_rdo               RDO module instance.
 *  @param[in] target_rollover_value  Offset rollover value.
 *  @param[in] target_increment_step  Number of timeslot between each offset increment.
 */
void link_rdo_init(link_rdo_t *link_rdo, uint16_t target_rollover_value, uint16_t target_increment_step);

/** @brief Enable the RDO module.
 *
 *  @param[in] link_rdo  RDO module instance.
 */
void link_rdo_enable(link_rdo_t *link_rdo);

/** @brief Disable the RDO module.
 *
 *  @param[in] link_rdo  RDO module instance.
 */
void link_rdo_disable(link_rdo_t *link_rdo);

/** @brief Send the offset through the link protocol.
 *
 *  @note This method needs to be of the same form as
 *        the link protocol send_payload function
 *        pointer.
 *  @note This increment the offset value by 1 before sending
 *        and reset to 0 when the rollover value is met.
 *
 *  @param[in]  link_rdo        RDO module instance.
 *  @param[out] buffer_to_send  Pointer to the buffer that will be sent through the link protocol.
 */
void link_rdo_send_offset(link_rdo_t *link_rdo, uint8_t *buffer_to_send);

/** @brief Send the offset through the link protocol.
 *
 *  @note This method needs to be of the same form as
 *        link protocol receive_payload function
 *        pointer.
 *
 *  @note Module offset is set only if size received is valid
 *        based on the offset size (2 byte max).
 *
 *  @note This increment the current offset value by 1
 *        in case the packet is missed. The value should
 *        be overwrite by the received offset.
 *
 *  @param[in] link_rdo            RDO module instance.
 *  @param[in] buffer_to_received  Received offset through the link protocol.
 */
void link_rdo_set_offset(link_rdo_t *link_rdo, uint8_t *buffer_to_received);

/** @brief Get the current RDO offset.
 *
 *  @param[in] link_rdo  RDO module instance.
 *  @return RDO offset, in PLL cycle.
 */
uint16_t link_rdo_get_offset(link_rdo_t *link_rdo);

/** @brief Update the RDO offset value.
 *
 *  @note This increment the offset value by 1
 *        and reset to 0 when the rollover value
 *        is met.
 *
 *  @param[in] link_rdo  RDO module instance.
 */
void link_rdo_update_offset(link_rdo_t *link_rdo);

#ifdef __cplusplus
}
#endif
#endif /* LINK_RANDOM_DATARATE_OFFSET_H_ */
