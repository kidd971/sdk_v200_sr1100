/** @file  link_credit_flow_ctrl.h
 *  @brief Link Credit Control Flow module.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef LINK_CREDIT_FLOW_CTRL_H
#define LINK_CREDIT_FLOW_CTRL_H

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/** @brief Threshold for frames that were skipped due to no credits available.
 * Once it is exceeded, a frame containing only header data is sent to the other side.
 */
#define CREDIT_FLOW_CTRL_SKIPPED_FRAMES_THRESHOLD (3)

/** @brief Enable/disable Credit Control Flow statistics
 */
#define CREDIT_FLOW_CTRL_STAT_ENABLE (0)

/* TYPES **********************************************************************/
/** @brief Link Credit Control FLow data
 */
typedef struct credit_flow_ctrl {
    /*! Denotes whether the Credit Control Flow is enabled */
    bool enabled;
    /*! Number of credits available */
    uint8_t credits_count;
    /*! The counter incremented when a frame was skipped due to no credits available */
    uint8_t skipped_frames_count;
    /*! The counter incremented when an auto-reply was not sent */
    uint8_t notify_missed_credits_count;

#if CREDIT_FLOW_CTRL_STAT_ENABLE
    /*! Total number of `skipped_frames_count` */
    uint32_t skipped_frames_count_total;
#endif
} credit_flow_ctrl_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize Credit Control Flow instance.
 *
 *  @param[in] credit_flow_ctrl    Credit Control Flow instance.
 *  @param[in] enabled             Denotes whether the Credit Control Flow functionality is enabled.
 *  @param[in] init_credits_count  Initial number of credits available.
 */
void link_credit_flow_ctrl_init(credit_flow_ctrl_t *credit_flow_ctrl, bool enabled, uint8_t init_credits_count);

/** @brief A handle that checks whether the available credits are enough to send the frame with the payload to the other
 * side.
 *
 *  @param[in] credit_flow_ctrl  Credit Control Flow instance.
 *  @return True if the frame can be sent, false otherwise.
 */
static inline bool link_credit_flow_ctrl_is_available(credit_flow_ctrl_t *credit_flow_ctrl)
{
    if (credit_flow_ctrl->enabled && credit_flow_ctrl->credits_count == 0) {
        credit_flow_ctrl->skipped_frames_count++;

#if CREDIT_FLOW_CTRL_STAT_ENABLE
        credit_flow_ctrl->skipped_frames_count_total++;
#endif /* CREDIT_FLOW_CTRL_STAT_ENABLE*/
        return false;
    }

    return true;
}

/** @brief A handle that decrement number of credits after receiving ACK.
 *
 *  @param[in] credit_flow_ctrl  Credit Control Flow instance.
 */
static inline void link_credit_flow_ctrl_frame_ack_received(credit_flow_ctrl_t *credit_flow_ctrl)
{
    if (credit_flow_ctrl->credits_count > 0) {
        credit_flow_ctrl->credits_count--;
    }
    credit_flow_ctrl->skipped_frames_count = 0;
}

/** @brief Check whether the skipped frames threshold has been exceeded.
 *
 *  @param[in] credit_flow_ctrl  Credit Control Flow instance.
 *  @return True if the skipped frames threshold has been exceeded, false otherwise.
 */
static inline bool link_credit_flow_ctrl_is_skipped_frames_exceed(credit_flow_ctrl_t *credit_flow_ctrl)
{
    if (credit_flow_ctrl->enabled &&
        credit_flow_ctrl->skipped_frames_count > CREDIT_FLOW_CTRL_SKIPPED_FRAMES_THRESHOLD) {
        return true;
    }

    return false;
}

/** @brief A handle that decrements number of credits after receiving ACK and clears number of
 * `notify_missed_credits_count` after sent auto reply frame.
 *
 *  @param[in] credit_flow_ctrl  Credit Control Flow instance.
 */
static inline void link_credit_flow_ctrl_auto_frame_sent(credit_flow_ctrl_t *credit_flow_ctrl)
{
    if (credit_flow_ctrl->credits_count > 0) {
        credit_flow_ctrl->credits_count--;
    }
    credit_flow_ctrl->notify_missed_credits_count = 0;
}

#ifdef __cplusplus
}
#endif

#endif /* LINK_CREDIT_FLOW_CTRL_H */
