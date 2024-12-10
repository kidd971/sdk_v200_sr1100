/** @file  quasar_i2c.h
 *  @brief This module configures I2C and provides functions to write and read into a I2C device register with or without interrupts.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_I2C_H_
#define QUASAR_I2C_H_

/* INCLUDES *******************************************************************/
#include "quasar_gpio.h"
#include "quasar_it.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief List of all available clock sources for the I2C instances.
 *
 *  @note From the reference manual.
 *      - `0b00` : PCLK1    -> set 0
 *      - `0b01` : SYSCLK   -> set 1
 *      - `0b10` : HSI16    -> set 2
 *      - `0b11` : MSIK     -> set 3
 */
typedef enum quasar_i2c_clk_source {
    /*! Select PCLK1 as clock source. */
    QUASAR_I2C_CLK_SOURCE_PCLK1  = 0,
    /*! Select SYSCLK as clock source. */
    QUASAR_I2C_CLK_SOURCE_SYSCLK = 1,
    /*! Select HSI16 as clock source. */
    QUASAR_I2C_CLK_SOURCE_HSI16  = 2,
    /*! Select MSIK as clock source. */
    QUASAR_I2C_CLK_SOURCE_MSIK   = 3,

} quasar_i2c_clk_source_t;

/** @brief List of all available I2C instances and used as an index into Quasar
 *         FIFO buffer array.
 */
typedef enum quasar_i2c_selection {
    /*! Select I2C1 instance. */
    QUASAR_I2C_SELECTION_I2C1   = 0,
    /*! Select I2C2 instance. */
    QUASAR_I2C_SELECTION_I2C2   = 1,
    /*! Select I2C3 instance. */
    QUASAR_I2C_SELECTION_I2C3   = 2,
    /*! Select I2C4 instance. */
    QUASAR_I2C_SELECTION_I2C4   = 3,
    /*! Select I2C5 instance. */
    QUASAR_I2C_SELECTION_I2C5   = 4,
    /*! Select I2C6 instance. */
    QUASAR_I2C_SELECTION_I2C6   = 5,
    /*! Indicate the number of possible I2C selections. */
    _QUASAR_I2C_SELECTION_COUNT = 6
} quasar_i2c_selection_t;

/** @brief Quasar BSP I2C configuration.
 */
typedef struct quasar_i2c_config {
    /*! Selected I2C instance. */
    quasar_i2c_selection_t i2c_selection;
    /*! Selected GPIO configuration used for SDA. */
    quasar_gpio_config_t gpio_config_sda;
    /*! Selected GPIO configuration used for SCL. */
    quasar_gpio_config_t gpio_config_scl;
    /*! Available IRQ priority. */
    quasar_irq_priority_t irq_priority;
} quasar_i2c_config_t;

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Initialize the I2C peripheral.
 *
 *  @param[in] i2c_config  Configuration of the I2C peripheral.
 */
void quasar_i2c_init(quasar_i2c_config_t i2c_config);

/** @brief Deinitialize the I2C peripheral and its associated GPIOs.
 *
 *  @param[in] i2c_config  Configuration of the I2C peripheral.
 */
void quasar_i2c_deinit(quasar_i2c_config_t i2c_config);

/** @brief Initiate I2C transmission in order to write a value in a specific register of an I2C device using interrupts.
 *
 *  @note The selected I2C instance must have been initialized with a valid IRQ priority.
 *
 *  @param[in] i2c_selection  Selected I2C peripheral.
 *  @param[in] device_addr    Address of the target I2C device.
 *  @param[in] reg_addr       Address of the register within the I2C device to which the value will be written.
 *  @param[in] reg_value      Value to be written to the specified register.
 */
void quasar_i2c_write(quasar_i2c_selection_t i2c_selection, uint8_t device_addr, uint8_t reg_addr, uint8_t reg_value);

/** @brief Initiate I2C read to retrieve a value from a specific register of an I2C device using interrupts.
 *
 *  @note The selected I2C instance must have been initialized with a valid IRQ priority.
 *
 *  @param[in]  i2c_selection  Selected I2C peripheral.
 *  @param[in]  device_addr    Address of the target I2C device.
 *  @param[in]  reg_addr       Address of the register within the I2C device from which the value will be read.
 *  @param[out] reg_value_ptr  Pointer to store the retrieved value.
 */
void quasar_i2c_read(quasar_i2c_selection_t i2c_selection, uint8_t device_addr, uint8_t reg_addr, uint8_t *reg_value_ptr);

/** @brief Initiate I2C transmission in order to write a value in a specific register of an I2C device.
 *
 *  @note The selected I2C instance must have been initialized without IRQ priority (QUASAR_IRQ_NONE).
 *
 *  @param[in] i2c_selection  Selected I2C peripheral.
 *  @param[in] device_addr    Address of the target I2C device.
 *  @param[in] reg_addr       Address of the register within the I2C device to which the value will be written.
 *  @param[in] reg_value      Value to be written to the specified register.
 *  @param[in] retry_count    Number of loop iterations expected in each cycle of the I2C transaction.
 */
void quasar_i2c_write_blocking(quasar_i2c_selection_t i2c_selection, uint8_t device_addr, uint8_t reg_addr, uint8_t reg_value,
                               uint16_t retry_count);

/** @brief Initiate I2C read to retrieve a value from a specific register of an I2C device.
 *
 *  @note The selected I2C instance must have been initialized without IRQ priority (QUASAR_IRQ_NONE).
 *
 *  @param[in]  i2c_selection  Selected I2C peripheral.
 *  @param[in]  device_addr    Address of the target I2C device.
 *  @param[in]  reg_addr       Address of the register within the I2C device from which the value will be read.
 *  @param[out] reg_value_ptr  Pointer to store the retrieved value.
 *  @param[in]  retry_count    Number of loop iterations expected in each cycle of the I2C transaction.
 */
void quasar_i2c_read_blocking(quasar_i2c_selection_t i2c_selection, uint8_t device_addr, uint8_t reg_addr, uint8_t *reg_value_ptr,
                              uint16_t retry_count);

/** @brief Initiate I2C transmission in order to write a value in a specific register of an I2C device.
 *
 *  @note The selected I2C instance must have been initialized without IRQ priority (QUASAR_IRQ_NONE).
 *
 *  @param[in] i2c_selection     Selected I2C peripheral.
 *  @param[in] device_addr       Address of the target I2C device.
 *  @param[in] reg_addr_start    Address of the register to start the burst transaction.
 *  @param[in] reg_values_array  Array of the values to be written to the registers.
 *  @param[in] size              Size of the array which also indicate the number of registers to be written.
 *  @param[in] retry_count       Number of loop iterations expected in each cycle of the I2C transaction.
 */
void quasar_i2c_write_burst_blocking(quasar_i2c_selection_t i2c_selection, uint8_t device_addr, uint8_t reg_addr_start,
                                     uint8_t *reg_values_array, uint8_t size, uint16_t retry_count);

/** @brief Initiate I2C read to retrieve a value from a specific register of an I2C device.
 *
 *  @note The selected I2C instance must have been initialized without IRQ priority (QUASAR_IRQ_NONE).
 *
 *  @param[in] i2c_selection     Selected I2C peripheral.
 *  @param[in] device_addr       Address of the target I2C device.
 *  @param[in] reg_addr_start    Address of the register to start the burst transaction.
 *  @param[in] reg_values_array  Array to store the values of registers that are to be read.
 *  @param[in] size              Size of the array which also indicate the number of registers to be read.
 *  @param[in] retry_count       Number of loop iterations expected in each cycle of the I2C transaction.
 */
void quasar_i2c_read_burst_blocking(quasar_i2c_selection_t i2c_selection, uint8_t device_addr, uint8_t reg_addr_start,
                                    uint8_t *reg_values_array, uint8_t size, uint16_t retry_count);

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_I2C_H_ */
