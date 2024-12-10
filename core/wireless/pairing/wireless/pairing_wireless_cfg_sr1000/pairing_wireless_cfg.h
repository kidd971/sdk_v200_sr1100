/** @file  pairing_wireless_cfg.h
 *  @brief Pairing specific configuration constants for the SPARK Wireless Core.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef PAIRING_WIRELESS_CFG_H_
#define PAIRING_WIRELESS_CFG_H_

/* CONSTANTS ******************************************************************/
/* Pairing Addresses. */
/*! PAN ID reserved for pairing operation. */
#define PAIRING_PAN_ID        0x000
/*! Coordinator address used for pairing operation. */
#define PAIRING_COORD_ADDRESS 0x01
/*! Node address used for pairing operation. */
#define PAIRING_NODE_ADDRESS  0x02

/* Output power configuration. */
/*! Pulse count of transmitted data frames. */
#define PAIRING_TX_DATA_PULSE_COUNT 2
/*! Pulse width of transmitted data frames. */
#define PAIRING_TX_DATA_PULSE_WIDTH 7
/*! Pulse gain of transmitted data frames. */
#define PAIRING_TX_DATA_PULSE_GAIN  0
/*! Pulse count of transmitted ACK frames. */
#define PAIRING_TX_ACK_PULSE_COUNT  2
/*! Pulse width of transmitted ACK frames. */
#define PAIRING_TX_ACK_PULSE_WIDTH  7
/*! Pulse gain of transmitted ACK frames. */
#define PAIRING_TX_ACK_PULSE_GAIN   0

/* Input power configuration. */
/*! Pulses count of received ACK frames. */
#define PAIRING_RX_ACK_PULSE_COUNT  2
/*! Pulses count of received data frames. */
#define PAIRING_RX_DATA_PULSE_COUNT 2

/* Clear Channel Assessment (CCA) configuration. */
/*! Energy level considered too high for a successful transmission. */
#define PAIRING_CCA_THRESHOLD   25
/*! Number of energy readings to do before the fail action is executed. */
#define PAIRING_CCA_TRY_COUNT   10
/*! Amount of time between energy readings. (962 * 48.8 ns -> 47 ms) */
#define PAIRING_CCA_RETRY_TIME  962
/*! Clear Channel Assessment Fail Action. */
#define PAIRING_CCA_FAIL_ACTION SWC_CCA_ABORT_TX

/* SWC queue size. */
/*! Queue size for the RX and TX fifo. */
#define PAIRING_DATA_QUEUE_SIZE 2

/* Schedule configuration. */
/** @brief Pairing schedule timing.
 */
#define PAIRING_SCHEDULE { \
    1000, 1000             \
}

/** @brief Pairing coordinator to node timeslots.
 */
#define COORD_TO_NODE_TIMESLOTS { \
    MAIN_TIMESLOT(0)              \
}

/** @brief Pairing node to coordinator timeslots.
 */
#define NODE_TO_COORD_TIMESLOTS { \
    MAIN_TIMESLOT(1)              \
}

/** @brief Pairing channels to meet FCC and ETSI regulations.
 */
#define PAIRING_CHANNEL_FREQ_FCC_ETSI { \
    171                                 \
}

/** @brief Pairing channels to meet ARIB regulations.
 */
#define PAIRING_CHANNEL_FREQ_ARIB { \
    207                             \
}

/** @brief Pairing channels sequence.
 */
#define PAIRING_CHANNEL_SEQUENCE { \
    0                              \
}
#endif /* PAIRING_WIRELESS_CFG_h_ */
