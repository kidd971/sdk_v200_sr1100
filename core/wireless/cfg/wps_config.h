/** @file  wps_config.h
 *  @brief Default configuration of the stack.
 *
 *  To define your own config, create a new file named exactly like this one,
 *  adjust the build system to include that new file instead of this one and
 *  define the macros below with the wanted value.
 *
 *  Boolean macros must be either set to true or false and checked in the code with #if (not #ifdef).
 *  They must only be defined if not already defined by another source (externally with the build system).
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef WPS_CONFIG_H_
#define WPS_CONFIG_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>

/* MACROS *********************************************************************/
/** @brief The radio model.
 *
 * Allowed define names are only SR1000 and SR1100.
 */
#if !defined(SR1000) && !defined(SR1100)
#define SR1000 true
#define SR1100 false
#endif

/** @brief The number of radios.
 */
#ifndef WPS_RADIO_COUNT
#define WPS_RADIO_COUNT 1
#endif

/** @brief Enable the gathering of PHY statistics.
 *
 *  @note  PHY stats are the one located in the Link Quality Indicator
 *         module. Disabling these stats will also completely disable the
 *         per bands PHY stats.
 */
#ifndef WPS_ENABLE_PHY_STATS
#define WPS_ENABLE_PHY_STATS true
#endif

/** @brief Enable the gathering of used timeslots statistics.
 *
 *  @note  When using an auto-sync connection, the Coordinator device will
 *         try to send empty frame when no frame from APP is available in
 *         the Xlayer. This macro here disable stats excluding
 *         these empty frame.
 */
#ifndef WPS_ENABLE_STATS_USED_TIMESLOTS
#define WPS_ENABLE_STATS_USED_TIMESLOTS true
#endif

/** @brief Enable the gathering of PHY statistics per bands.
 *
 *  @note  Do not enable these stats if PHY stats are
 *         disabled.
 */
#ifndef WPS_ENABLE_PHY_STATS_PER_BANDS
#if WPS_ENABLE_PHY_STATS
#define WPS_ENABLE_PHY_STATS_PER_BANDS true
#else
#define WPS_ENABLE_PHY_STATS_PER_BANDS false
#endif /* WPS_ENABLE_PHY_STATS */
#endif /* WPS_ENABLE_PHY_STATS_PER_BANDS */

#if !WPS_ENABLE_PHY_STATS && WPS_ENABLE_PHY_STATS_PER_BANDS
#error "WPS_ENABLE_PHY_STATS_PER_BANDS (per band stats) cannot be enabled if WPS_ENABLE_PHY_STATS (PHY stats) is disabled."
#endif

/** @brief Enable the gathering of Links statistics.
 *
 *  @note This will Enable the following statistics :
 *          - tx_sent         : Number of payload sent
 *          - tx_byte_sent    : Number of byte sent
 *          - tx_drop         : Number of payload dropped
 *          - rx_received     : Number of payload received
 *          - rx_byte_received: Number of byte received
 *          - rx_overrun      : Number of payload dropped because of an RX buffer overrun
 *          - cca_pass        : Number of CCA TX abort
 *          - cca_fail        : Number of CCA TX anyway
 */
#ifndef WPS_ENABLE_LINK_STATS
#define WPS_ENABLE_LINK_STATS true
#endif

/** @brief Disable the link-throttle feature.
 */
#ifndef WPS_DISABLE_LINK_THROTTLE
#define WPS_DISABLE_LINK_THROTTLE false
#endif

/** @brief Disable the fragmentation feature.
 *
 * If fragmentation is disabled, make sure the build system doesn't compile the wps_frag files.
 */
#ifndef WPS_DISABLE_FRAGMENTATION
#define WPS_DISABLE_FRAGMENTATION false
#endif

/* If your compiler is not GCC but supports GCC attributes (see __packed below),
 * you can remove this error check.
 */
#ifndef __GNUC__
#error "Only GCC's __attribute__ is supported."
#endif /* __GNUC__ */

/** @brief Macro to force struct packing.
 *
 * We only support compilers that uses the __attribute__ syntax.
 */
#ifndef __packed
#define __packed __attribute__((packed))
#endif

#endif /* WPS_CONFIG_H_ */
