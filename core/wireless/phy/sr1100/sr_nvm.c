/** @file sr_nvm.c
 *   @brief SR non-volatile memory module.
 *
 *   Functions related to reading and writing the NVM and to its protocol.
 *
 *   @copyright Copyright (C) 2018-2021 SPARK Microsystems International Inc. All rights reserved.
 *   @license   This source code is confidential and proprietary.
 *   @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sr_nvm.h"
#include <string.h>
#include "sr_access.h"
#include "sr_def.h"

/* CONSTANTS ******************************************************************/

#define NVM_KEY_LAYOUT_VER_NAME   "NVM Layout Ver"
#define NVM_KEY_SERIAL_NO_NAME    "Serial No     "
#define NVM_KEY_RESISTUNE_NAME    "Resistune     "
#define NVM_KEY_PRODUCT_ID_NAME   "Product ID    "
#define NVM_KEY_VREF_ADJUST_NAME  "Vref Adjust   "
#define NVM_KEY_IREFTUNE_NAME     "Ireftune      "
#define NVM_KEY_RSSI_OFFSET_NAME  "Rssi Offset   "
#define NVM_KEY_POWER_ADJUST_NAME "Power Adjust  "

#define NVM_POST_WRITE_DELAY_MS   150
#define NVM_BINNING_SETUP_MASK    0xFFFF000000000000
#define NVM_BINNING_SETUP_POS     48
#define NVM_CHIP_ID_MASK          0x000000FFFFFFFFFF

/* MACROS *********************************************************************/
static const uint8_t key_to_index[15] = {
    0xFF, /* 0x00 */
    0,    /* 0x01 */
    1,    /* 0x02 */
    2,    /* 0x03 */
    3,    /* 0x04 */
    4,    /* 0x05 */
    5,    /* 0x06 */
    0xFF, /* 0x07 */
    0xFF, /* 0x08 */
    0xFF, /* 0x09 */
    0xFF, /* 0x0A */
    0xFF, /* 0x0B */
    0xFF, /* 0x0C */
    6,    /* 0x0D */
    7     /* 0x0E */
};

#define BIT_CHECK(var, nbit) ((var) & (1 << ((nbit) - 1)))
#define DEFAULT_VREFTUNE     MASK2VAL(VREFTUNE_DEFAULT, BITS_VREFTUNE)
#define DEFAULT_IREFTUNE     MASK2VAL(IREFTUNE_DEFAULT, BITS_IREFTUNE)

/* PRIVATE GLOBALS ************************************************************/
static nvm_entry_t nvm_template[NVM_KEY_ENTRY_COUNT] = {
    /* 8 Keys are currently supported. */
    {NVM_KEY_LAYOUT_VER, NVM_KEY_LAYOUT_VER_NAME, 1, NULL},
    {NVM_KEY_SERIAL_NO, NVM_KEY_SERIAL_NO_NAME, 8, NULL},
    {NVM_KEY_RESISTUNE, NVM_KEY_RESISTUNE_NAME, 1, NULL},
    {NVM_KEY_PRODUCT_ID, NVM_KEY_PRODUCT_ID_NAME, 2, NULL},
    {NVM_KEY_VREF_ADJUST, NVM_KEY_VREF_ADJUST_NAME, 1, NULL},
    {NVM_KEY_IREFTUNE, NVM_KEY_IREFTUNE_NAME, 1, NULL},
    {NVM_KEY_RSSI_OFFSET, NVM_KEY_RSSI_OFFSET_NAME, 1, NULL},
    {NVM_KEY_POWER_ADJUST, NVM_KEY_POWER_ADJUST_NAME, 3, NULL}};

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void extract_values(nvm_t *nvm);

/* PUBLIC FUNCTIONS ***********************************************************/
bool sr_nvm_init(radio_t *radio, nvm_t *nvm)
{
    bool nvm_is_populated = false;

    /* Copy template to object */
    for (uint8_t i = 0; i < NVM_KEY_ENTRY_COUNT; ++i) {
        nvm->entry[i] = nvm_template[i];
    }

    sr_nvm_read(radio, nvm->shadow_nvm, NVM_FIRST_ADDRESS, NVM_LAST_ADDRESS);

    nvm_is_populated = (nvm->shadow_nvm[0] != NVM_KEY_TERMINATOR);
    if (nvm_is_populated) {
        extract_values(nvm);
    }

    return nvm_is_populated;
}

uint8_t *sr_nvm_get_value(nvm_entry_t *nvm_entry, nvm_entry_key_t key)
{
    if ((key == 0) || (key >= NVM_KEY_END)) {
        return NULL;
    }

    uint8_t index = key_to_index[key];

    if (index == NVM_KEY_INVALID) {
        return NULL;
    }

    return nvm_entry[index].value;
}

uint8_t sr_nvm_get_size(nvm_entry_t *nvm_entry, nvm_entry_key_t key)
{
    if ((key == 0) || (key >= NVM_KEY_END)) {
        return 0;
    }

    uint8_t index = key_to_index[key];

    if (index == NVM_KEY_INVALID) {
        return 0;
    }

    return nvm_entry[index].size;
}

char *sr_nvm_get_name(nvm_entry_t *nvm_entry, nvm_entry_key_t key)
{
    if ((key == 0) || (key >= NVM_KEY_END)) {
        return NULL;
    }

    uint8_t index = key_to_index[key];

    if (index == NVM_KEY_INVALID) {
        return NULL;
    }

    return nvm_entry[index].name;
}

int8_t sr_nvm_read(radio_t *radio, uint8_t *buf, uint8_t addr_start, uint8_t addr_end)
{
    uint8_t idx = 0;
    uint8_t addr_current = addr_start;
    uint8_t read_status = 0;

    sr_access_write_reg16(radio->radio_id, REG16_IF_BASEBAND_GAIN_LNA, REG16_IF_BASEBAND_GAIN_LNA_DEFAULT);

    do {
        /* Wake up the radio */
        sr_access_write_reg8(radio->radio_id, REG8_ACTIONS, 0x00);
        read_status = sr_access_read_reg8(radio->radio_id, REG8_POWER_STATE);
    } while (!GET_AWAKE(read_status));

    sr_nvm_power_up(radio);

    while (addr_current <= addr_end) {
        sr_access_write_reg8(radio->radio_id, REG8_NVM, BIT_ROMPWRSW | addr_current++);
        read_status = sr_access_read_reg8(radio->radio_id, REG8_NVM);
        buf[idx++] = read_status;
    }

    sr_nvm_power_down(radio);

    return 0;
}

uint8_t sr_nvm_get_product_id_model(nvm_t *nvm)
{
    uint8_t *ptr = NULL;
    uint16_t product_id = 0;
    uint8_t model = 0;

    ptr = sr_nvm_get_value(nvm->entry, NVM_KEY_PRODUCT_ID);

    if (ptr != NULL) {
        product_id = (ptr[1] << 8) | ptr[0]; /* TODO: Create a util for that */
        model = MASK2VAL(product_id, BITS_PID_MODEL);
    }

    return model;
}

uint8_t sr_nvm_get_product_id_version(nvm_t *nvm)
{
    uint8_t *ptr = NULL;
    uint16_t product_id = 0;
    uint8_t version = 0;

    ptr = sr_nvm_get_value(nvm->entry, NVM_KEY_PRODUCT_ID);

    if (ptr != NULL) {
        product_id = (ptr[1] << 8) | ptr[0]; /* TODO: Create a util for that */
        version = MASK2VAL(product_id, BITS_PID_VERSION);
    }

    return version;
}

uint8_t sr_nvm_get_product_id_package(nvm_t *nvm)
{
    uint8_t *ptr = NULL;
    uint16_t product_id = 0;
    uint8_t package = 0;

    ptr = sr_nvm_get_value(nvm->entry, NVM_KEY_PRODUCT_ID);

    if (ptr != NULL) {
        product_id = (ptr[1] << 8) | ptr[0]; /* TODO: Create a util for that */
        package = MASK2VAL(product_id, BITS_PID_PACKAGE);
    }

    return package;
}

uint8_t sr_nvm_get_resistune(nvm_t *nvm)
{
    uint8_t *ptr = NULL;
    uint8_t resistune = DEFAULT_VREFTUNE;

    ptr = sr_nvm_get_value(nvm->entry, NVM_KEY_RESISTUNE);

    if (ptr != NULL) {
        resistune = *ptr;
    } else {
        resistune = DEFAULT_VREFTUNE;
    }

    return resistune;
}

int8_t sr_nvm_get_vref_adjust_vref_tune_offset(nvm_t *nvm)
{
    uint8_t *ptr = NULL;
    int8_t vref_tune_offset = DEFAULT_VREFTUNE;

    ptr = sr_nvm_get_value(nvm->entry, NVM_KEY_VREF_ADJUST);

    if (ptr != NULL) {
        vref_tune_offset = *ptr;
    }

    return vref_tune_offset;
}

uint64_t sr_nvm_get_serial_number(nvm_t *nvm)
{
    uint8_t *ptr = NULL;
    uint64_t serial_num_lsb = 0;
    uint64_t serial_num_msb = 0;
    uint64_t serial_num = 0;

    ptr = sr_nvm_get_value(nvm->entry, NVM_KEY_SERIAL_NO);

    if (ptr != NULL) {
        serial_num_msb = (ptr[0] << 24) | (ptr[1] << 16) | (ptr[2] << 8) | ptr[3];
        serial_num_msb &= 0x00000000FFFFFFFF;
        serial_num_lsb = (ptr[4] << 24) | (ptr[5] << 16) | (ptr[6] << 8) | ptr[7];
        serial_num_lsb &= 0x00000000FFFFFFFF;
        serial_num = (serial_num_msb << 32) | serial_num_lsb;
    }

    return serial_num;
}

uint16_t sr_nvm_get_serial_number_binning_setup_code(nvm_t *nvm)
{
    uint64_t serial_number = 0;

    serial_number = sr_nvm_get_serial_number(nvm);
    return ((serial_number & NVM_BINNING_SETUP_MASK) >> NVM_BINNING_SETUP_POS);
}

uint64_t sr_nvm_get_serial_number_chip_id(nvm_t *nvm)
{
    uint64_t serial_number = 0;

    serial_number = sr_nvm_get_serial_number(nvm);
    return (serial_number & NVM_CHIP_ID_MASK);
}

uint8_t sr_nvm_get_layout_version(nvm_t *nvm)
{
    uint8_t *ptr = NULL;
    uint8_t layout_ver = 0;

    ptr = sr_nvm_get_value(nvm->entry, NVM_KEY_LAYOUT_VER);

    if (ptr != NULL) {
        layout_ver = *ptr;
    }

    return layout_ver;
}

uint8_t sr_nvm_get_ireftune(nvm_t *nvm)
{
    uint8_t *ptr = NULL;
    uint8_t ireftune = DEFAULT_IREFTUNE;

    ptr = sr_nvm_get_value(nvm->entry, NVM_KEY_IREFTUNE);

    if (ptr != NULL) {
        ireftune = *ptr;
    }

    return ireftune;
}

int8_t sr_nvm_get_rssi_offset(nvm_t *nvm)
{
    uint8_t *ptr = NULL;
    int8_t rssi_offset = 0;

    ptr = sr_nvm_get_value(nvm->entry, NVM_KEY_RSSI_OFFSET);

    if (ptr != NULL) {
        rssi_offset = *ptr;
    }

    return rssi_offset;
}

nvm_power_adjust_t sr_nvm_get_power_adjust(nvm_t *nvm)
{
    nvm_power_adjust_t pwr_adjust = {0};
    uint8_t *ptr = NULL;
    uint8_t offset_temp;

    ptr = sr_nvm_get_value(nvm->entry, NVM_KEY_POWER_ADJUST);

    if (ptr != NULL) {
        /*
         * Assemble the 3 NVM bytes into a 24-bit little-endian value.
         * Six 4-bit signed offsets are packed as follows:
         *
         *  bits [23:20] = 9.2 GHz offset
         *  bits [19:16] = 8.7 GHz offset
         *  bits [15:12] = 8.2 GHz offset
         *  bits [11:08] = 7.7 GHz offset
         *  bits [07:04] = 7.2 GHz offset
         *  bits [03:00] = 6.7 GHz offset
         */
        pwr_adjust.raw = (uint32_t)ptr[0] | ((uint32_t)ptr[1] << 8) | ((uint32_t)ptr[2] << 16);
    }

    offset_temp = (pwr_adjust.raw >> 0) & 0x0F;
    pwr_adjust.ghz_6_7 = (offset_temp & 0x08) ? (int8_t)(offset_temp | 0xF0) : (int8_t)offset_temp;

    offset_temp = (pwr_adjust.raw >> 4) & 0x0F;
    pwr_adjust.ghz_7_2 = (offset_temp & 0x08) ? (int8_t)(offset_temp | 0xF0) : (int8_t)offset_temp;

    offset_temp = (pwr_adjust.raw >> 8) & 0x0F;
    pwr_adjust.ghz_7_7 = (offset_temp & 0x08) ? (int8_t)(offset_temp | 0xF0) : (int8_t)offset_temp;

    offset_temp = (pwr_adjust.raw >> 12) & 0x0F;
    pwr_adjust.ghz_8_2 = (offset_temp & 0x08) ? (int8_t)(offset_temp | 0xF0) : (int8_t)offset_temp;

    offset_temp = (pwr_adjust.raw >> 16) & 0x0F;
    pwr_adjust.ghz_8_7 = (offset_temp & 0x08) ? (int8_t)(offset_temp | 0xF0) : (int8_t)offset_temp;

    offset_temp = (pwr_adjust.raw >> 20) & 0x0F;
    pwr_adjust.ghz_9_2 = (offset_temp & 0x08) ? (int8_t)(offset_temp | 0xF0) : (int8_t)offset_temp;

    return pwr_adjust;
}

void sr_nvm_power_up(radio_t *radio)
{
    sr_access_write_reg8(radio->radio_id, REG8_NVM, BIT_ROMPWRSW);
    sr_utils_wait_delay(NVM_DELAY_AFTER_ROMEN_SET_MS);
}

void sr_nvm_power_down(radio_t *radio)
{
    sr_access_write_reg8(radio->radio_id, REG8_NVM, 0x00);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Extract usable values from the NVM.
 *
 *  This populates the NVM array value.
 *  @param[in] nvm  NVM structure.
 */
static void extract_values(nvm_t *nvm)
{
    uint8_t key = 0, size = 0, addr = 0;

    addr = NVM_FIRST_ADDRESS;
    do {
        key = nvm->shadow_nvm[addr++]; /* First valid value should be a key */
        if (key == NVM_KEY_INVALID) {
            if (addr > NVM_LAST_ADDRESS) { /* Misburned code has been "erased", check next address */
                break;
            }
            continue;
        } else if (key == NVM_KEY_TERMINATOR) {
            break;
        }
        size = sr_nvm_get_size(nvm->entry, key);
        if (size == 0) {
            /* Unknown key, abort parsing */
            break;
        }
        /* First byte of the value is right after the key */
        nvm->entry[key_to_index[key]].value = &(nvm->shadow_nvm[addr]);
        /* Skip to next key */
        addr += size;
        /* Stop when the terminator or end of NVM is reached */
    } while ((nvm->shadow_nvm[addr] != NVM_KEY_TERMINATOR) && (addr <= NVM_LAST_ADDRESS));
}
