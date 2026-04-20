/** @file sr_nvm.h
 *  @brief SR non-volatile memory module.
 *
 *  Functions related to reading and writing the NVM and to its protocol.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is confidential and proprietary.
 *  @author    SPARK FW Team.
 */
#ifndef SR_NVM_H_
#define SR_NVM_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "sr_def.h"
#include "sr_radio_model.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
#define NVM_SIZE_BYTES               128
#define NVM_FIRST_ADDRESS            0
#define NVM_LAST_ADDRESS             127
#define NVM_LAST_BIT_POS             7
#define NVM_DELAY_AFTER_ROMEN_SET_MS 10
#define NVM_OVERWRITE_RETRY_COUNT    3
#define NVM_DEFAULT_LAYOUT           0

/* Product ID fields */
#define BITS_PID_MODEL   BITS16_1(3, 0)
#define BITS_PID_UNUSED  BITS16_1(7, 4)
#define BITS_PID_PACKAGE BITS16_2(3, 0)
#define BITS_PID_VERSION BITS16_2(7, 4)

/* Binning setup code definitions */
#define INTERNAL_SR1120_BINNING_SETUP_CODE 0x4443u /* DC */
#define ATE_BINNING_SETUP_CODE             0x4954u /* IT */

/*! NVM key entry count */
#define NVM_KEY_ENTRY_COUNT 8

/* TYPES **********************************************************************/
typedef enum phy_version {
    PHY_VERSION_0 = 0
} phy_version_t;

/** @brief NVM entry key enumeration
 *
 *  @note Keys start at 1
 */
typedef enum nvm_entry_key {
    /*! NVM delimiter. */
    NVM_KEY_TERMINATOR = 0x00,
    /*! Layout version NVM entry key, 8-bit value. */
    NVM_KEY_LAYOUT_VER = 0x01,
    /*! Serial number NVM entry key, 64-bit unique value with field : BINNING_SETUP_CODE[63..48], BINNING_SETUP_ID[47..40] and CHIP_ID[39..00]. */
    NVM_KEY_SERIAL_NO = 0x02,
    /*! Calibration NVM entry key, 4 VREF_TUNE value. */
    NVM_KEY_RESISTUNE = 0x03,
    /*! Product ID NVM entry key, 16-bit value with field : UNUSED[15..12], MODEL[11..8], VERSION[7..4] and PACKAGE[3..0]. */
    NVM_KEY_PRODUCT_ID = 0x04,
    /*! VREFTUNE NVM entry key, 4-bit unsigned value. Vreftune step: 12 mV. */
    NVM_KEY_VREF_ADJUST = 0x05,
    /*! IREFTUNE NVM entry key, 6-bit value (0x00 to 0x3F) to write in the V/I Reference and Delay Line Calibration registers. */
    NVM_KEY_IREFTUNE = 0x06,
    /*! RSSI offset NVM entry key, 8-bit signed value. RSSI offset: 1 dB step. */
    NVM_KEY_RSSI_OFFSET = 0x0D,
    /*! Power adjust offset NVM entry key, 24-bit value with field : OFFSET_9_2GHZ[23..20], OFFSET_8_7GHZ[19..16],
     *   OFFSET_8_2GHZ[15..12], OFFSET_7_7GHZ[11..08], OFFSET_7_2GHZ[7..4] and OFFSET_6_7GHZ[3..0].
     *   Each 4-bit value is signed (two's complement). Power offset: 0.5 dB step.
     */
    NVM_KEY_POWER_ADJUST = 0x0E,
    /*! End of NVM entry keys. */
    NVM_KEY_END,
    /*! Invalid NVM entry key. */
    NVM_KEY_INVALID = 0xFF
} nvm_entry_key_t;

/** @brief NVM entry key structure
 *
 */
typedef struct {
    /*! NVM entry key. */
    nvm_entry_key_t key;
    /*! User name for NVM entry */
    char *name;
    /*! Size of the NVM entry value, in 8-bit count */
    uint8_t size;
    /*! Pointer to value/buffer of the NVM entry */
    uint8_t *value;
} nvm_entry_t;

/** @brief NVM structure.
 *
 */
typedef struct {
    /*! Sorted NVM raw data, by entry key.*/
    nvm_entry_t entry[NVM_KEY_ENTRY_COUNT];
    /*! NVM raw data */
    uint8_t shadow_nvm[NVM_SIZE_BYTES];
} nvm_t;

/** @brief Decoded power adjust offsets from NVM key 0x0E.
 *
 *  Each field is a 4-bit two's complement value sign-extended to int8_t.
 *  Unit: 0.5 dB per step, range -8 to +7 (-4.0 dB to +3.5 dB).
 */
typedef struct {
    /*! Raw 24-bit assembled value. */
    uint32_t raw;
    /*! Power offset for the 6.7 GHz frequency band. bits [03:00] */
    int8_t ghz_6_7;
    /*! Power offset for the 7.2 GHz frequency band. bits [07:04] */
    int8_t ghz_7_2;
    /*! Power offset for the 7.7 GHz frequency band. bits [11:08] */
    int8_t ghz_7_7;
    /*! Power offset for the 8.2 GHz frequency band. bits [15:12] */
    int8_t ghz_8_2;
    /*! Power offset for the 8.7 GHz frequency band. bits [19:16] */
    int8_t ghz_8_7;
    /*! Power offset for the 9.2 GHz frequency band. bits [23:20] */
    int8_t ghz_9_2;
} nvm_power_adjust_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the NVM.
 *
 *  This populates an array in the MCU that mirrors
 *  the content of the NVM.
 *
 *  @param[in] radio  Radio's instance.
 *  @param[in] nvm    NVM structure.
 *  @retval true   NVM is properly initialized.
 *  @retval false  NVM is not properly initialized.
 */
bool sr_nvm_init(radio_t *radio, nvm_t *nvm);

/** @brief Get a value from the NVM.
 *
 *  A call to sr_nvm_init() must have been made
 *  prior to using this function.
 *
 *  @param[in] nvm_entry  NVM entry table.
 *  @param[in] key        The key associated with the value.
 *  @return A pointer to the array of bytes containing the value or NULL not found.
 */
uint8_t *sr_nvm_get_value(nvm_entry_t *nvm_entry, nvm_entry_key_t key);

/** @brief Get the size of value in NVM entry table.
 *
 *  @param[in] nvm_entry  NVM entry table.
 *  @param[in] key        The key associated with the value.
 *  @return The size of the data or 0 if the key is invalid.
 */
uint8_t sr_nvm_get_size(nvm_entry_t *nvm_entry, nvm_entry_key_t key);

/** @brief Get the name of a key.
 *
 *  @param[in] nvm_entry  NVM entry table.
 *  @param[in] key        The key (1 to NVM_ENTRY_COUNT) for which the name is required.
 *  @return Pointer to name string.
 */
char *sr_nvm_get_name(nvm_entry_t *nvm_entry, nvm_entry_key_t key);

/** @brief Read a range of NVM locations.
 *
 *  @param[in]  radio       Radio's instance.
 *  @param[out] buf         Values read from the NVM.
 *  @param[in]  addr_start  Address of the first memory location.
 *  @param[in]  addr_end    Address of the last memory location.
 */
int8_t sr_nvm_read(radio_t *radio, uint8_t *buf, uint8_t addr_start, uint8_t addr_end);

/** @brief Power up the NVM.
 *
 *  @param[in] radio  Radio's instance.
 */
void sr_nvm_power_up(radio_t *radio);

/** @brief Power down the NVM.
 *
 *  @param[in] radio  Radio's instance.
 */
void sr_nvm_power_down(radio_t *radio);

/** @brief Get the product id model from the NVM.
 *
 *  @param[in] nvm  NVM structure.
 *  @return Product ID model.
 */
uint8_t sr_nvm_get_product_id_model(nvm_t *nvm);

/** @brief Get the product id version from the NVM.
 *
 *  @param[in] nvm  NVM structure.
 *  @return Product ID version.
 */
uint8_t sr_nvm_get_product_id_version(nvm_t *nvm);

/** @brief Get the product id package from the NVM.
 *
 *  @param[in] nvm  NVM structure.
 *  @return Product ID package.
 */
uint8_t sr_nvm_get_product_id_package(nvm_t *nvm);

/** @brief Get the resistune value from the NVM.
 *
 *  @param[in] nvm  NVM structure.
 *  @return Resistune value.
 */
uint8_t sr_nvm_get_resistune(nvm_t *nvm);

/** @brief Get the Ireftune value from the NVM.
 *
 *  @param[in] nvm  NVM structure.
 *  @return Ireftune value.
 */
uint8_t sr_nvm_get_ireftune(nvm_t *nvm);

/** @brief Get the rssi offset value from the NVM.
 *
 *  @param[in] nvm  NVM structure.
 *  @return RSSI offset value.
 */
int8_t sr_nvm_get_rssi_offset(nvm_t *nvm);

/** @brief Get the vref tune offset value from the NVM.
 *
 *  @param[in] nvm  NVM structure.
 *  @return Vref tune offset value.
 */
int8_t sr_nvm_get_vref_adjust_vref_tune_offset(nvm_t *nvm);

/** @brief Get the layout version from the NVM.
 *
 *  @param[in] nvm  NVM structure.
 *  @return Layout version value.
 */
uint8_t sr_nvm_get_layout_version(nvm_t *nvm);

/** @brief Get the unique 64 bit serial number from the NVM.
 *
 *  @param[in] nvm  NVM structure.
 *  @return Serial number value.
 */
uint64_t sr_nvm_get_serial_number(nvm_t *nvm);

/** @brief Get the 16-bit binning setup code from the NVM.
 *  @Note  The binning setup code is a serial number field.
 *
 *  @param[in] nvm  NVM structure.
 *  @return Binning setup code.
 */
uint16_t sr_nvm_get_serial_number_binning_setup_code(nvm_t *nvm);

/** @brief Get the 40-bit chip ID from the NVM.
 *  @Note  The chip ID is a serial number field.
 *
 *  @param[in] nvm  NVM structure.
 *  @return Chip ID.
 */
uint64_t sr_nvm_get_serial_number_chip_id(nvm_t *nvm);

/** @brief Get the decoded power adjust offsets from the NVM.
 *
 *  Reads the raw 24-bit NVM value and decodes it into individual signed per-band offsets.
 *
 *  bits [23:20] = 9.2 GHz offset
 *  bits [19:16] = 8.7 GHz offset
 *  bits [15:12] = 8.2 GHz offset
 *  bits [11:08] = 7.7 GHz offset
 *  bits [07:04] = 7.2 GHz offset
 *  bits [03:00] = 6.7 GHz offset
 *
 *  @param[in] nvm  NVM structure.
 *  @return Struct with the raw value and one signed offset per frequency band.
 */
nvm_power_adjust_t sr_nvm_get_power_adjust(nvm_t *nvm);

#ifdef __cplusplus
}
#endif

#endif /* SR_NVM_H_ */
