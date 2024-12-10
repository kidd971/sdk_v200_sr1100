/** @file  quasar_i2c.c
 *  @brief This module configures I2C and provides functions to write and read into a I2C device register with or without interrupts.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_i2c.h"
#include <string.h>
#include "quasar_clock.h"
#include "quasar_def.h"
#include "quasar_fifo.h"

/* CONSTANTS ******************************************************************/
/* I2C timing configuration (refer to the STM32U5xx datasheet and reference design) */
#define QUASAR_I2C_TIMING   0xC0100615

/* PRIVATE GLOBALS ************************************************************/
/* Each cell of the array is the associated FIFO buffer for the selected I2C when using interrupt. */
static quasar_fifo_t quasar_i2c_fifo[_QUASAR_I2C_SELECTION_COUNT];

/* Pointer to address of I2C received data, pointing to the same location as "reg_value_ptr" in quasar_i2c_read. */
volatile uint8_t *val_tmp;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static I2C_TypeDef *i2c_get_instance(quasar_i2c_selection_t i2c_selection);
static void i2c_enable_clock(quasar_i2c_selection_t i2c_selection);
static void i2c_disable_clock(quasar_i2c_selection_t i2c_selection);
static IRQn_Type i2c_get_selected_irq(quasar_i2c_selection_t i2c_selection);
static void i2c_select_clock_source(quasar_i2c_selection_t i2c_selection, quasar_i2c_clk_source_t clk_source);
static uint8_t i2c_wait_for_flag(I2C_TypeDef *i2c_instance, uint32_t bit_pos, uint16_t timeout, uint8_t expected_status);
static void i2c_irq_handler_routine(I2C_TypeDef *i2c_instance, quasar_i2c_selection_t i2c_selection);

/* PUBLIC FUNCTIONS ***********************************************************/
void quasar_i2c_init(quasar_i2c_config_t i2c_config)
{
    I2C_TypeDef *i2c_instance = i2c_get_instance(i2c_config.i2c_selection);
    IRQn_Type i2c_irq;

    /*
     *  Init
     *
     *      1. Disable I2C : Clear I2C_CR1->PE
     *      2. Configure noise filter :
     *          Analog filter : Clear I2C_CR1->ANFOFF
     *          Digital filter : Configure bits I2C_CR1->DNF
     *      3. Configure timing :
     *          Configure I2C_TIMINGR->PRESC, I2C_TIMINGR->SCLDEL and I2C_TIMINGR->SDADEL
     *          Configure I2C_TIMINGR->SCLH and I2C_TIMINGR->SCLL
     *          Configure I2C_CR1->NOSTRETCH
     *      4. Enable I2C : Set I2C_CR1->PE
     */

    /* Configure GPIO for SCL and SDA */
    quasar_gpio_init(i2c_config.gpio_config_sda);
    quasar_gpio_init(i2c_config.gpio_config_scl);

    /* Select the clock source. */
    i2c_select_clock_source(i2c_config.i2c_selection, QUASAR_I2C_CLK_SOURCE_SYSCLK);

    /* Enable I2C Clock. */
    i2c_enable_clock(i2c_config.i2c_selection);

    /* Disable the I2C while configuring settings. */
    QUASAR_CLEAR_BIT(i2c_instance->CR1, I2C_CR1_PE_Msk);

    QUASAR_CLEAR_BIT(i2c_instance->CR1, I2C_CR1_ANFOFF);

    /* Configure I2C timing. */
    i2c_instance->TIMINGR = (uint32_t) QUASAR_I2C_TIMING;

    /* Enable the I2C interrupt flags and configure IRQ priority. */
    if (i2c_config.irq_priority != QUASAR_IRQ_PRIORITY_NONE) {

        /* Initialize fifo used to manage data for the selected i2c instance. */
        quasar_fifo_init(&quasar_i2c_fifo[i2c_config.i2c_selection]);

        /* Enable transmission and reception interrupt flags. */
        QUASAR_SET_BIT(i2c_instance->CR1, I2C_CR1_TXIE_Msk);
        QUASAR_SET_BIT(i2c_instance->CR1, I2C_CR1_TCIE_Msk);
        QUASAR_SET_BIT(i2c_instance->CR1, I2C_CR1_RXIE_Msk);

        i2c_irq = i2c_get_selected_irq(i2c_config.i2c_selection);

        /* Set the IRQ priority. */
        NVIC_SetPriority(i2c_irq, i2c_config.irq_priority);

        /* Enable the IRQ interrupt. */
        NVIC_EnableIRQ(i2c_irq);
    }

    /* Enable I2C. */
    i2c_instance->CR1 |= I2C_CR1_PE;
}

void quasar_i2c_deinit(quasar_i2c_config_t i2c_config)
{
    I2C_TypeDef *i2c_instance = i2c_get_instance(i2c_config.i2c_selection);
    IRQn_Type i2c_irq;

    /* Disable the I2C. */
    QUASAR_CLEAR_BIT(i2c_instance->CR1, I2C_CR1_PE_Msk);

    /* Disable the I2C interrupt flags. */
    if (i2c_config.irq_priority != QUASAR_IRQ_PRIORITY_NONE) {
        /* Disable the IRQ interrupt. */
        i2c_irq = i2c_get_selected_irq(i2c_config.i2c_selection);
        NVIC_DisableIRQ(i2c_irq);

        /* Disable transmission and reception interrupt flags. */
        QUASAR_CLEAR_BIT(i2c_instance->CR1, I2C_CR1_TXIE_Msk);
        QUASAR_CLEAR_BIT(i2c_instance->CR1, I2C_CR1_TCIE_Msk);
        QUASAR_CLEAR_BIT(i2c_instance->CR1, I2C_CR1_RXIE_Msk);
    }

    i2c_disable_clock(i2c_config.i2c_selection);

    quasar_gpio_deinit(i2c_config.gpio_config_scl.port, i2c_config.gpio_config_scl.pin);
    quasar_gpio_deinit(i2c_config.gpio_config_sda.port, i2c_config.gpio_config_sda.pin);
}

void quasar_i2c_write(quasar_i2c_selection_t i2c_selection, uint8_t device_addr, uint8_t reg_addr, uint8_t reg_value)
{
    I2C_TypeDef *i2c_instance = i2c_get_instance(i2c_selection);
    uint32_t config_to_push;
    uint8_t array_to_push[4] = {0};

    /*
     *  Data transfer
     *      1. Wait until the instance is no longer BUSY    -> Read the state of the BUSY bit in ISR
     *      2. Configure the transaction                    -> Set SADD & NBYTES in CR2   (NBYYTES : number of times writing to TXDR = 2 (reg_addr & reg_value))
     *      3. START condition                              -> Set the START bit in CR2
     *      4. Wait for the ACK                             -> Read the state of the TXIS bit in ISR
     *      5. Transmit the register address                -> Write reg_addr into TXDR
     *      6. Wait for the ACK                             -> Read the state of the TXIS bit in ISR
     *      7. Transmit the register value                  -> Write reg_value into TXDR
     *      8. Wait for the ACK                             -> Read the state of the TC bit in ISR (TC instead of TXIS because NBYTES = 2 and we've sent 2 = End of transaction)
     */

    /* Push the configuration of the I2C write transaction into the FIFO. (Registers are 4 bytes long in a 32 bits architecture) */
    config_to_push = ((((uint32_t) 2) << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES) | (((uint32_t) device_addr) & I2C_CR2_SADD);
    /* Place the config_to_push (uint32_t) into an array of uint8_t in order to pass the correct parameter to the fifo push function. */
    memcpy(array_to_push, &config_to_push, 4);

    quasar_it_enter_critical();
    quasar_fifo_push_bytes(&(quasar_i2c_fifo[i2c_selection]), array_to_push, 4);
    /* Push the address of the register to be written into the FIFO. */
    quasar_fifo_push(&(quasar_i2c_fifo[i2c_selection]), reg_addr);
    /* Push the value of the register to be written into the FIFO. */
    quasar_fifo_push(&(quasar_i2c_fifo[i2c_selection]), reg_value);
    quasar_it_exit_critical();

    /* Wait until the I2C instance is no longer busy */
    if (i2c_wait_for_flag(i2c_instance, I2C_ISR_BUSY_Pos, 250, 0) != 0) {
        while (1);
        /* Trigger an exception. */
    }

    /* Configure the I2C write transaction and initiate the START condition. */
    quasar_it_enter_critical();
    i2c_instance->CR2 = (uint32_t) quasar_fifo_pull_bytes(&(quasar_i2c_fifo[i2c_selection]), 4);
    quasar_it_exit_critical();

    QUASAR_SET_BIT(i2c_instance->CR2, I2C_CR2_START_Msk);
}

void quasar_i2c_read(quasar_i2c_selection_t i2c_selection, uint8_t device_addr, uint8_t reg_addr, uint8_t *reg_value_ptr)
{
    I2C_TypeDef *i2c_instance = i2c_get_instance(i2c_selection);
    uint32_t config_to_push;
    uint8_t array_to_push[4] = {0};

    /*
     *  Data Read
     *      1. Wait until the instance is no longer BUSY    -> Read the state of the BUSY bit in ISR
     *      2. Configure the transaction                    -> Set SADD & NBYTES in CR2 (NBYTES : number of times writing to TXDR = 1 (reg_addr))
     *      3. START condition                              -> Set the START bit in CR2
     *      4. Wait for the ACK                             -> Read the state of the TXIS bit in ISR
     *      5. Transmit the register address                -> Write reg_addr into TXDR
     *      6. Wait for the ACK                             -> Read the state of the TC bit in ISR   (Because NBYTES = 1 and we sent 1)
     *      7. RESTART condition                            -> Configure SADD & NBYTES & RDWRN in CR2 (NBYTES : number of times reading from RXDR (reg_value))
     *      8. Wait for the ACK                             -> Read the state of the RXNE bit in ISR  (in RX mode, !Attention! it's slower than sending -> Add some time to the timeout)
     *      9. Retrieve the register value                  -> Read the register value from RXDR
     *      10. Wait for a NACK                             -> Read the state of the TC bit in ISR
     *      11. STOP condition                              -> Set the STOP bit in CR2
     */

    /* Push the configuration of the I2C write transaction into the FIFO (registers are 4 bytes long in a 32 bits architecture). */
    config_to_push = ((((uint32_t) 1) << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES) | (((uint32_t) device_addr) & I2C_CR2_SADD);
    /* Place the config_to_push (uint32_t) into an array of uint8_t in order to pass the correct parameter to the fifo push function. */
    memcpy(array_to_push, &config_to_push, 4);

    quasar_it_enter_critical();
    quasar_fifo_push_bytes(&(quasar_i2c_fifo[i2c_selection]), array_to_push, 4);
    /* Push the address of the register to be written into the FIFO. */
    quasar_fifo_push(&(quasar_i2c_fifo[i2c_selection]), reg_addr);
    quasar_it_exit_critical();

    /* Push the configuration of the I2C read transaction into the FIFO (registers are 4 bytes long in a 32 bits architecture). */
    config_to_push = ((((uint32_t) 1) << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES) | I2C_CR2_RD_WRN | ((uint32_t) device_addr & I2C_CR2_SADD);
    /* Place the config_to_push (uint32_t) into an array of uint8_t in order to pass the correct parameter to the fifo push function. */
    memcpy(array_to_push, &config_to_push, 4);

    quasar_it_enter_critical();
    quasar_fifo_push_bytes(&(quasar_i2c_fifo[i2c_selection]), array_to_push, 4);
    /* Place the reg_value_ptr (pointer : 32 bits) into an array of uint8_t in order to pass the correct parameter to the fifo push function. */
    memcpy(array_to_push, &reg_value_ptr, 4);
    /* Push a pointer to store the received register value (pointers are 4 bytes long in 32 bits architecture). */
    quasar_fifo_push_bytes(&(quasar_i2c_fifo[i2c_selection]), array_to_push, 4);
    quasar_it_exit_critical();

    /* Wait until the I2C instance is no longer busy. */
    if (i2c_wait_for_flag(i2c_instance, I2C_ISR_BUSY_Pos, 250, 0) != 0) {
        while (1);
        /* Trigger an exception. */
    }

    /* Configure the I2C write transaction and initiate the START condition. */
    quasar_it_enter_critical();
    i2c_instance->CR2 = (uint32_t) quasar_fifo_pull_bytes(&(quasar_i2c_fifo[i2c_selection]), 4);
    quasar_it_exit_critical();

    QUASAR_SET_BIT(i2c_instance->CR2, I2C_CR2_START_Msk);
}

void quasar_i2c_write_blocking(quasar_i2c_selection_t i2c_selection, uint8_t device_addr, uint8_t reg_addr, uint8_t reg_value,
                               uint16_t retry_count)
{
    I2C_TypeDef *i2c_instance = i2c_get_instance(i2c_selection);

    /* Wait until the I2C instance is no longer busy. */
    if (i2c_wait_for_flag(i2c_instance, I2C_ISR_BUSY_Pos, retry_count, 0) != 0) {
        while (1);
        /* Trigger an exception. */
    }

    /* Configure the I2C write transaction and initiate the START condition. */
    i2c_instance->CR2 = ((((uint32_t) 2) << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES) |
                        (((uint32_t) device_addr) & I2C_CR2_SADD);
    QUASAR_SET_BIT(i2c_instance->CR2, I2C_CR2_START_Msk);


    /* Wait for TXDR register to be ready for data. */
    if (i2c_wait_for_flag(i2c_instance, I2C_ISR_TXIS_Pos, retry_count, 1) != 0) {
        while (1);
        /* Trigger an exception.  */
    }

    /* Transmit the address of the register to be written. */
    i2c_instance->TXDR = ((uint32_t) reg_addr & I2C_TXDR_TXDATA);

    /* Wait for TXDR register to be ready for data. */
    if (i2c_wait_for_flag(i2c_instance, I2C_ISR_TXIS_Pos, retry_count, 1) != 0) {
        while (1);
        /* Trigger an exception. */
    }

    /* Transmit the value to be written in the register. */
    i2c_instance->TXDR = ((uint32_t) reg_value & I2C_TXDR_TXDATA);

    /* Wait for transfer to complete (NBYTES has been transmitted). */
    if (i2c_wait_for_flag(i2c_instance, I2C_ISR_TC_Pos, retry_count, 1) != 0) {
        while (1);
        /* Trigger an exception. */
    }

    /* Initiate the STOP condition. */
    QUASAR_SET_BIT(i2c_instance->CR2, I2C_CR2_STOP_Msk);

    /* After a write transaction, regardless of what we wait for (STOPF, BUSY, TC), it requires 1 ms delay. */
    HAL_Delay(5);
}

void quasar_i2c_read_blocking(quasar_i2c_selection_t i2c_selection, uint8_t device_addr, uint8_t reg_addr, uint8_t *reg_value_ptr,
                              uint16_t retry_count)
{
    I2C_TypeDef *i2c_instance = i2c_get_instance(i2c_selection);

    /* Wait until the I2C instance is no longer busy. */
    if (i2c_wait_for_flag(i2c_instance, I2C_ISR_BUSY_Pos, retry_count, 0) != 0) {
        while (1);
        /* Trigger an exception  */
    }

    /* Configure the I2C write transaction and initiate the START condition. */
    i2c_instance->CR2 = ((((uint32_t) 1) << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES) |
                        (((uint32_t) device_addr) & I2C_CR2_SADD);
    QUASAR_SET_BIT(i2c_instance->CR2, I2C_CR2_START_Msk);

    /* Wait for TXDR register to be ready for data. */
    if (i2c_wait_for_flag(i2c_instance, I2C_ISR_TXIS_Pos, retry_count, 1) != 0) {
        while (1);
        /* Trigger an exception  */
    }

    /* Transmit the address of the register to be read. */
    i2c_instance->TXDR = ((uint32_t) reg_addr & I2C_TXDR_TXDATA);

    /* Wait for transfer to complete (NBYTES has been transmitted). */
    if (i2c_wait_for_flag(i2c_instance, I2C_ISR_TC_Pos, retry_count, 1) != 0) {
        while (1);
        /* Trigger an exception  */
    }

    /* Configure the I2C read transaction and initiate the RESTART condition. */
    i2c_instance->CR2 = ((((uint32_t) 1) << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES) |
                         I2C_CR2_RD_WRN | (((uint32_t) device_addr) & I2C_CR2_SADD);
    QUASAR_SET_BIT(i2c_instance->CR2, I2C_CR2_START_Msk);

    /* Wait for received data in RXDR register to be ready  */
    if (i2c_wait_for_flag(i2c_instance, I2C_ISR_RXNE_Pos, (retry_count + 500), 1) != 0) {
        while (1);
        /* Trigger an exception  */
    }

    /* Retrieve the received register value. */
    *reg_value_ptr = (uint8_t)(i2c_instance->RXDR & I2C_RXDR_RXDATA);

    /* Wait for transfer (NBYTES has been received) to complete. */
    if (i2c_wait_for_flag(i2c_instance, I2C_ISR_TC_Pos, retry_count, 1) != 0) {
        while (1);
        /* Trigger an exception  */
    }

    /* Initiate the stop condition. */
    QUASAR_SET_BIT(i2c_instance->CR2, I2C_CR2_STOP_Msk);
}

void quasar_i2c_write_burst_blocking(quasar_i2c_selection_t i2c_selection, uint8_t device_addr, uint8_t reg_addr_start,
                                     uint8_t *reg_values_array, uint8_t size, uint16_t retry_count)
{
    I2C_TypeDef *i2c_instance = i2c_get_instance(i2c_selection);

    /* Wait until the I2C instance is no longer busy. */
    if (i2c_wait_for_flag(i2c_instance, I2C_ISR_BUSY_Pos, retry_count, 0) != 0) {
        while (1);
        /* Trigger an exception. */
    }

    /* Configure the I2C write transaction and initiate the START condition. */
    i2c_instance->CR2 = ((((uint32_t) (size + 1)) << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES) |
                        (((uint32_t) device_addr) & I2C_CR2_SADD);
    QUASAR_SET_BIT(i2c_instance->CR2, I2C_CR2_START_Msk);

    /* Wait for TXDR register to be ready for data. */
    if (i2c_wait_for_flag(i2c_instance, I2C_ISR_TXIS_Pos, retry_count, 1) != 0) {
        while (1);
        /* Trigger an exception.  */
    }

    /* Transmit the address of the register to be written. */
    i2c_instance->TXDR = ((uint32_t) reg_addr_start & I2C_TXDR_TXDATA);

    for (int i = 0; i < size; i++) {
        /* Wait for TXDR register to be ready for data. */
        if (i2c_wait_for_flag(i2c_instance, I2C_ISR_TXIS_Pos, retry_count, 1) != 0) {
            while (1);
            /* Trigger an exception. */
        }

        /* Transmit the value to be written in the register. */
        i2c_instance->TXDR = ((uint32_t) reg_values_array[i] & I2C_TXDR_TXDATA);
    }

    /* Wait for transfer to complete (NBYTES has been transmitted). */
    if (i2c_wait_for_flag(i2c_instance, I2C_ISR_TC_Pos, retry_count, 1) != 0) {
        while (1);
        /* Trigger an exception. */
    }

    /* Initiate the STOP condition. */
    QUASAR_SET_BIT(i2c_instance->CR2, I2C_CR2_STOP_Msk);

    /* For an obscure reason, after a write transaction, regardless of what we wait for (STOPF, BUSY, TC), it requires 2 ms. */
    HAL_Delay(5);
}

void quasar_i2c_read_burst_blocking(quasar_i2c_selection_t i2c_selection, uint8_t device_addr, uint8_t reg_addr_start,
                                    uint8_t *reg_values_array, uint8_t size, uint16_t retry_count)
{
    I2C_TypeDef *i2c_instance = i2c_get_instance(i2c_selection);

    /* Wait until the I2C instance is no longer busy. */
    if (i2c_wait_for_flag(i2c_instance, I2C_ISR_BUSY_Pos, retry_count, 0) != 0) {
        while (1);
        /* Trigger an exception  */
    }

    /* Configure the I2C write transaction and initiate the START condition. */
    i2c_instance->CR2 = ((((uint32_t) 1) << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES) |
                        (((uint32_t) device_addr) & I2C_CR2_SADD);
    QUASAR_SET_BIT(i2c_instance->CR2, I2C_CR2_START_Msk);

    /* Wait for TXDR register to be ready for data. */
    if (i2c_wait_for_flag(i2c_instance, I2C_ISR_TXIS_Pos, retry_count, 1) != 0) {
        while (1);
        /* Trigger an exception  */
    }

    /* Transmit the address of the register to be read. */
    i2c_instance->TXDR = ((uint32_t) reg_addr_start & I2C_TXDR_TXDATA);

    /* Wait for transfer to complete (NBYTES has been transmitted). */
    if (i2c_wait_for_flag(i2c_instance, I2C_ISR_TC_Pos, retry_count, 1) != 0) {
        while (1);
        /* Trigger an exception  */
    }

    /* Configure the I2C read transaction and initiate the RESTART condition. */
    i2c_instance->CR2 = ((((uint32_t) size) << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES) |
                         I2C_CR2_RD_WRN | (((uint32_t) device_addr) & I2C_CR2_SADD);
    QUASAR_SET_BIT(i2c_instance->CR2, I2C_CR2_START_Msk);

    for (int i = 0; i < size; i++) {
        /* Wait for received data in RXDR register to be ready */
        if (i2c_wait_for_flag(i2c_instance, I2C_ISR_RXNE_Pos, (retry_count + 500), 1) != 0) {
            while (1);
        /* Trigger an exception  */
        }

        /* Retrieve the received register value. */
        reg_values_array[i] = (uint8_t)(i2c_instance->RXDR & I2C_RXDR_RXDATA);
    }

    /* Wait for transfer (NBYTES has been received) to complete. */
    if (i2c_wait_for_flag(i2c_instance, I2C_ISR_TC_Pos, retry_count, 1) != 0) {
        while (1);
        /* Trigger an exception  */
    }

    /* Initiate the stop condition. */
    QUASAR_SET_BIT(i2c_instance->CR2, I2C_CR2_STOP_Msk);

    /* Wait until the I2C instance is no longer busy. */
    if (i2c_wait_for_flag(i2c_instance, I2C_ISR_BUSY_Pos, retry_count, 0) != 0) {
        while (1);
        /* Trigger an exception. */
    }
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Return the instance to the selected I2C.
 *
 *  @param[in] i2c_selection  Selected I2C to get associated instance.
 *  @return Selected I2C instance.
 */
static I2C_TypeDef *i2c_get_instance(quasar_i2c_selection_t i2c_selection)
{
    I2C_TypeDef *i2c_instance = NULL;

    switch (i2c_selection) {
    case QUASAR_I2C_SELECTION_I2C1:
        i2c_instance = I2C1;
        break;
    case QUASAR_I2C_SELECTION_I2C2:
        i2c_instance = I2C2;
        break;
    case QUASAR_I2C_SELECTION_I2C3:
        i2c_instance = I2C3;
        break;
    case QUASAR_I2C_SELECTION_I2C4:
        i2c_instance = I2C4;
        break;
    case QUASAR_I2C_SELECTION_I2C5:
        i2c_instance = I2C5;
        break;
    case QUASAR_I2C_SELECTION_I2C6:
        i2c_instance = I2C6;
        break;
    default:
        /* Unimplemented */
        break;
    }

    return i2c_instance;
}

/** @brief Enables the clock for the selected I2C.
 *
 *  @param[in] i2c_selection  Selected I2C to enable the clock.
 */
static void i2c_enable_clock(quasar_i2c_selection_t i2c_selection)
{
    switch (i2c_selection) {
    case QUASAR_I2C_SELECTION_I2C1:
        __HAL_RCC_I2C1_CLK_ENABLE();
        break;
    case QUASAR_I2C_SELECTION_I2C2:
        __HAL_RCC_I2C2_CLK_ENABLE();
        break;
    case QUASAR_I2C_SELECTION_I2C3:
        __HAL_RCC_I2C3_CLK_ENABLE();
        break;
    case QUASAR_I2C_SELECTION_I2C4:
        __HAL_RCC_I2C4_CLK_ENABLE();
        break;
    case QUASAR_I2C_SELECTION_I2C5:
        __HAL_RCC_I2C5_CLK_ENABLE();
        break;
    case QUASAR_I2C_SELECTION_I2C6:
        __HAL_RCC_I2C6_CLK_ENABLE();
        break;
    default:
        /* Unimplemented */
        break;
    }
}

/** @brief Disables the clock for the selected I2C.
 *
 *  @param[in] i2c_selection  Selected I2C to disable the clock.
 */
static void i2c_disable_clock(quasar_i2c_selection_t i2c_selection)
{
    switch (i2c_selection) {
    case QUASAR_I2C_SELECTION_I2C1:
        __HAL_RCC_I2C1_CLK_DISABLE();
        break;
    case QUASAR_I2C_SELECTION_I2C2:
        __HAL_RCC_I2C2_CLK_DISABLE();
        break;
    case QUASAR_I2C_SELECTION_I2C3:
        __HAL_RCC_I2C3_CLK_DISABLE();
        break;
    case QUASAR_I2C_SELECTION_I2C4:
        __HAL_RCC_I2C4_CLK_DISABLE();
        break;
    case QUASAR_I2C_SELECTION_I2C5:
        __HAL_RCC_I2C5_CLK_DISABLE();
        break;
    case QUASAR_I2C_SELECTION_I2C6:
        __HAL_RCC_I2C6_CLK_DISABLE();
        break;
    default:
        /* Unimplemented */
        break;
    }
}

/** @brief Return the selected I2C's global interrupt.
 *
 *  @param[in] i2c_selection  Selected I2C to get associated global interrupt.
 *  @return Selected I2C global interrupt.
 */
static IRQn_Type i2c_get_selected_irq(quasar_i2c_selection_t i2c_selection)
{
    IRQn_Type i2c_irq = {0};

    switch (i2c_selection) {
    case QUASAR_I2C_SELECTION_I2C1:
        i2c_irq = I2C1_EV_IRQn;
        break;
    case QUASAR_I2C_SELECTION_I2C2:
        i2c_irq = I2C2_EV_IRQn;
        break;
    case QUASAR_I2C_SELECTION_I2C3:
        i2c_irq = I2C3_EV_IRQn;
        break;
    case QUASAR_I2C_SELECTION_I2C4:
        i2c_irq = I2C4_EV_IRQn;
        break;
    case QUASAR_I2C_SELECTION_I2C5:
        i2c_irq = I2C5_EV_IRQn;
        break;
    case QUASAR_I2C_SELECTION_I2C6:
        i2c_irq = I2C6_EV_IRQn;
        break;
    default:
        /* Unimplemented */
        break;
    }

    return i2c_irq;
}

/** @brief Select the clock source for the selected I2C.
 *
 *  @param[in] i2c_selection  Selected I2C to enable the clock.
 *  @param[in] clk_source     Clock source of the selected I2C.
 */
static void i2c_select_clock_source(quasar_i2c_selection_t i2c_selection, quasar_i2c_clk_source_t clk_source)
{
    /*
     *   I2C1    : RCC_CCIPR1
     *   I2C2    : RCC_CCIPR1
     *   I2C3    : RCC_CCIPR3
     *   I2C4    : RCC_CCIPR1
     *   I2C5    : RCC_CCIPR2
     *   I2C6    : RCC_CCIPR2
     */
    switch (i2c_selection) {
    case QUASAR_I2C_SELECTION_I2C1:
        QUASAR_WRITE_BITS(RCC->CCIPR1, RCC_CCIPR1_I2C1SEL_Msk, RCC_CCIPR1_I2C1SEL_Pos, clk_source);
        break;
    case QUASAR_I2C_SELECTION_I2C2:
        QUASAR_WRITE_BITS(RCC->CCIPR1, RCC_CCIPR1_I2C2SEL_Msk, RCC_CCIPR1_I2C2SEL_Pos, clk_source);
        break;
    case QUASAR_I2C_SELECTION_I2C3:
        QUASAR_WRITE_BITS(RCC->CCIPR3, RCC_CCIPR3_I2C3SEL_Msk, RCC_CCIPR3_I2C3SEL_Pos, clk_source);
        break;
    case QUASAR_I2C_SELECTION_I2C4:
        QUASAR_WRITE_BITS(RCC->CCIPR1, RCC_CCIPR1_I2C4SEL_Msk, RCC_CCIPR1_I2C4SEL_Pos, clk_source);
        break;
    case QUASAR_I2C_SELECTION_I2C5:
        QUASAR_WRITE_BITS(RCC->CCIPR2, RCC_CCIPR2_I2C5SEL_Msk, RCC_CCIPR2_I2C5SEL_Pos, clk_source);
        break;
    case QUASAR_I2C_SELECTION_I2C6:
        QUASAR_WRITE_BITS(RCC->CCIPR2, RCC_CCIPR2_I2C6SEL_Msk, RCC_CCIPR2_I2C6SEL_Pos, clk_source);
        break;
    default:
        /* Unimplemented */
        break;
    }
}

/** @brief Wait for a specific flag of an I2C instance to reach the desired status.
 *
 *  @note - The expected_status has to be 0 or 1.
 *        - If this function returns false, check flag in the ISR register to understand the cause
 *        - Some flags take time to be noticed. The timeout should be set to more than 200 for
 *          transmission and more than 500 for reception.
 *        - When breakpoint are placed, sometimes the behaviour is not the same as expected.
 *
 *  @param[in] i2c_instance    Selected I2C instance.
 *  @param[in] bit_pos         Position of the bit flag to be checked.
 *  @param[in] timeout         Number of loop iterations expected to wait for the flagto reach the desired status.
 *  @param[in] expected_status Desired status to be waited for.
 */
static uint8_t i2c_wait_for_flag(I2C_TypeDef *i2c_instance, uint32_t bit_pos, uint16_t timeout, uint8_t expected_status)
{
    uint16_t counter = 0;
    uint32_t status = 0;

    do {
        status = QUASAR_READ_BIT(i2c_instance->ISR, bit_pos);
        counter++;
        if (counter >= timeout) {
            return 1;
            break;
        }
    } while ((status != expected_status) && (counter <= timeout));

    return 0;
}

/** @brief I2C interrupt routine for reception and transmission.
 *
 *  @note The I2C interrupt routine is a state machine based on the status flag
 *  of the selected I2C instance. I2C reading and writing are multi-step processes.
 *
 *  In the case of a transmission (TXIS), data is pulled from an
 *  intermediate FIFO and placed into the transmission register (TXDR)
 *  to allow for the transmission of the next data.
 *
 *  In the case of a completed transmission (TC), there are two options:
 *      Either a retransmission (RESTART) is needed in the case of
 *  a completed transmission in a read operation. In this scenario, the
 *  configuration of the CR2 register is pulled from the intermediate FIFO
 *  and placed into the relevant register. Then, the START condition is initiated
 *  to allow for a RESTART.
 *      Or the transaction needs to be terminated. In this scenario,
 *  the STOP condition is initiated to conclude the transaction.
 *
 *  In the case of a reception (RXNE), data is retrieved from the reception
 *  register (RXDR) and placed into the output variable of the read function
 *  by directly writing to the address of this variable (a pointer that was
 *  previously fetched from the intermediate FIFO).
 *
 *  @param[in] i2c_instance   Selected I2C instance.
 *  @param[in] i2c_selection  Selected I2C selection.
 */
static void i2c_irq_handler_routine(I2C_TypeDef *i2c_instance, quasar_i2c_selection_t i2c_selection)
{
    uint32_t status;
    uint8_t pulled_byte;
    uint64_t received_ptr;

    /* Retrieve status of the ongoing I2C transaction. */
    status = i2c_instance->ISR;

    switch (status & ((uint32_t) (I2C_ISR_TXIS | I2C_ISR_TC | I2C_ISR_RXNE))) {
    /* In case RXNE and TC are at the same time. */
    case (I2C_ISR_TC | I2C_ISR_RXNE):

        /* For RXNE : Retrieve the received register value (pointer are 4 bytes long in 32 bits architecture). */
        received_ptr = quasar_fifo_pull_bytes(&(quasar_i2c_fifo[i2c_selection]), 4);
        /* Place the received pointer (the parameter of the read function of I2C) into the right variable. */
        memcpy((uint8_t *)&val_tmp, &received_ptr, 4);
        /* Write the value of the received data into the parameter received_value from the read function of I2C*/
        *val_tmp = (uint8_t)(i2c_instance->RXDR & I2C_RXDR_RXDATA);

        /* For TC : Initiate the STOP condition. */
        if (quasar_fifo_get_count(&(quasar_i2c_fifo[i2c_selection])) == 0) {
            QUASAR_SET_BIT(i2c_instance->CR2, I2C_CR2_STOP_Msk);
        }
        /* Or : first write transaction of a read query just finished. */
        else {
            /* Configure the I2C read transaction and initiate the RESTART condition.*/
            i2c_instance->CR2 = (uint32_t) quasar_fifo_pull_bytes(&(quasar_i2c_fifo[i2c_selection]), 4);

            QUASAR_SET_BIT(i2c_instance->CR2, I2C_CR2_START_Msk);
        }
        break;

    /* In case TC flag is set : A write or read transaction have been finished. */
    case I2C_ISR_TC:
        /* If there is no more data to be transmit, initiate a STOP condition. */
        if (quasar_fifo_get_count(&(quasar_i2c_fifo[i2c_selection])) == 0) {
            QUASAR_SET_BIT(i2c_instance->CR2, I2C_CR2_STOP_Msk);
        } else {
            /* If there is still data to transmit, configure the I2C read transaction and initiate a START condition. */
            i2c_instance->CR2 = (uint32_t) quasar_fifo_pull_bytes(&(quasar_i2c_fifo[i2c_selection]), 4);
            QUASAR_SET_BIT(i2c_instance->CR2, I2C_CR2_START_Msk);
        }
        break;

    /* In case TXIS flag is set : A write transaction is not finished and there is still data to transmit. */
    case I2C_ISR_TXIS:
        /* Transmit the next data */
        quasar_fifo_pull(&(quasar_i2c_fifo[i2c_selection]), &pulled_byte);
        i2c_instance->TXDR = (uint8_t) (pulled_byte & I2C_TXDR_TXDATA);
        break;

    /* In case RXNE flag is set : A read transaction just finished. */
    case I2C_ISR_RXNE:
        /* Retrieve the received register value. */
        received_ptr = quasar_fifo_pull_bytes(&(quasar_i2c_fifo[i2c_selection]), 4);
        /* Place the received pointer (the parameter of the read function of I2C) into the right variable. */
        memcpy((uint8_t *)&val_tmp, &received_ptr, 4);
        /* Write the value of the received data into the parameter received_value from the read function of I2C*/
        *val_tmp = (uint8_t)(i2c_instance->RXDR & I2C_RXDR_RXDATA);
        break;

    default:
        /* Exceptions are not managed. */
        break;
    }
}

/* ST HAL FUNCTIONS IMPLEMENTATION ********************************************/
/** @brief This function handles I2C1 event interrupt.
 */
void I2C1_EV_IRQHandler(void)
{
    i2c_irq_handler_routine(I2C1, QUASAR_I2C_SELECTION_I2C1);
}

/** @brief This function handles I2C1 error interrupt.
 */
void I2C1_ER_IRQHandler(void)
{
    /* Exceptions are not managed. */
}

/** @brief This function handles I2C2 event interrupt.
 */
void I2C2_EV_IRQHandler(void)
{
    i2c_irq_handler_routine(I2C2, QUASAR_I2C_SELECTION_I2C2);
}

/** @brief This function handles I2C2 error interrupt.
 */
void I2C2_ER_IRQHandler(void)
{
    /* Exceptions are not managed. */
}

/** @brief This function handles I2C3 event interrupt.
 */
void I2C3_EV_IRQHandler(void)
{
    i2c_irq_handler_routine(I2C3, QUASAR_I2C_SELECTION_I2C3);
}

/** @brief This function handles I2C3 error interrupt.
 */
void I2C3_ER_IRQHandler(void)
{
    /* Exceptions are not managed. */
}

/** @brief This function handles I2C4 event interrupt.
 */
void I2C4_EV_IRQHandler(void)
{
    i2c_irq_handler_routine(I2C4, QUASAR_I2C_SELECTION_I2C4);
}

/** @brief This function handles I2C4 error interrupt.
 */
void I2C4_ER_IRQHandler(void)
{
    /* Exceptions are not managed. */
}

/** @brief This function handles I2C5 event interrupt.
 */
void I2C5_EV_IRQHandler(void)
{
    i2c_irq_handler_routine(I2C5, QUASAR_I2C_SELECTION_I2C5);
}

/** @brief This function handles I2C5 error interrupt.
 */
void I2C5_ER_IRQHandler(void)
{
    /* Exceptions are not managed. */
}

/** @brief This function handles I2C6 event interrupt.
 */
void I2C6_EV_IRQHandler(void)
{
    i2c_irq_handler_routine(I2C6, QUASAR_I2C_SELECTION_I2C6);
}

/** @brief This function handles I2C6 error interrupt.
 */
void I2C6_ER_IRQHandler(void)
{
    /* Exceptions are not managed. */
}
