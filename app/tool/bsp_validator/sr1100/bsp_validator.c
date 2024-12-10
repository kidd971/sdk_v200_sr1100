/** @file  bsp_validator.c
 *  @brief Validate the BSP implementation by running basic tests.
 *
 *  The tests uses the SPARK SR1120 Transceiver to validate proper
 *  implementation of the board's peripheral drivers.
 *
 *  @copyright Copyright (C) 2024 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "bsp_validator_facade.h"
#include "critical_section.h"

/* CONSTANTS ******************************************************************/
#define LOG_LEVEL LOG_LEVEL_INFO

/* MACROS *********************************************************************/
/*!< Retrieve the LSB of a 16 bits register value. */
#define LSB_VALUE(VALUE_16BITS) (VALUE_16BITS & 0x00FF)
/*!< Retrieve the MSB of a 16 bits register value. */
#define MSB_VALUE(VALUE_16BITS) (VALUE_16BITS >> 8)

/*!< Register field single bit mask. */
#define BIT(n)                 (1 << (n))
#define REG_READ_BURST         BIT(7)
#define REG_WRITE              BIT(6)
#define REG_WRITE_BURST        (BIT(7) | REG_WRITE)
#define SET_BIT_OFFSET(OFFSET) (1 << OFFSET)

/*!< Registers fields used to configure the radio during tests. */
#define WAKEUPE_POSITION  8
#define SLPDEPTH_POSITION 14
#define GO_SLEEP_POSITION 0

/* TYPES **********************************************************************/
typedef enum level {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_ERR,
} log_level_t;

/** @brief List available radio index.
 */
typedef enum bsp_radio {
    /*! Select the radio index 1. */
    RADIO_ID_1 = 0,
    /*! Select the radio index 2. */
    RADIO_ID_2 = 1,
} bsp_radio_t;

/* PRIVATE GLOBALS ************************************************************/
static const uint8_t DEFAULT_SYNCWORD[] = {0x1D, 0xC1, 0xA6, 0x5E};
static const uint8_t SYNCWORD_REGISTER = 0x30;
static const uint8_t INTERRUPT_FLAG_REGISTER = 0x10;
static const uint8_t SLEEP_CONFIG_REGISTER = 0x0F;
static const uint8_t MAIN_COMMAND_REGISTER = 0x3B;
static const uint8_t SYNCWORD_LENGTH = 4;

static const char *const LOG_LEVEL_STR[] = {"DBG : ", "INF : ", "ERR : "};
static const char TEST_RUN_STRING[] = "[ RUN      ] ";
static const char TEST_OK_STRING[] = "[       OK ] ";
static const char TEST_FAILED_STRING[] = "[   FAILED ] ";
static volatile bool mocked_radio_1_irq_flag;
static volatile bool mocked_radio_2_irq_flag;
static volatile bool mocked_radio_1_dma_rx_flag;
static volatile bool mocked_radio_2_dma_rx_flag;
static volatile bool mocked_context_switch_flag;

static swc_hal_validator_t swc_hal[2] = {
    {
        .set_reset_pin = swc_hal_radio_1_set_reset_pin,
        .reset_reset_pin = swc_hal_radio_1_reset_reset_pin,
        .set_cs = swc_hal_radio_1_spi_set_cs,
        .reset_cs = swc_hal_radio_1_spi_reset_cs,
        .transfer_full_duplex_blocking = swc_hal_radio_1_spi_transfer_full_duplex_blocking,
        .transfer_full_duplex_non_blocking = swc_hal_radio_1_spi_transfer_full_duplex_non_blocking,
        .is_spi_busy = swc_hal_radio_1_is_spi_busy,
        .read_irq_pin = swc_hal_radio_1_read_irq_pin,
        .radio_context_switch = swc_hal_radio_1_context_switch,
        .disable_radio_irq = swc_hal_radio_1_disable_irq_it,
        .enable_radio_irq = swc_hal_radio_1_enable_irq_it,
        .disable_radio_dma_irq = swc_hal_radio_1_disable_dma_irq_it,
        .enable_radio_dma_irq = swc_hal_radio_1_enable_dma_irq_it,
    },
    {
        .set_reset_pin = swc_hal_radio_2_set_reset_pin,
        .reset_reset_pin = swc_hal_radio_2_reset_reset_pin,
        .set_cs = swc_hal_radio_2_spi_set_cs,
        .reset_cs = swc_hal_radio_2_spi_reset_cs,
        .transfer_full_duplex_blocking = swc_hal_radio_2_spi_transfer_full_duplex_blocking,
        .transfer_full_duplex_non_blocking = swc_hal_radio_2_spi_transfer_full_duplex_non_blocking,
        .is_spi_busy = swc_hal_radio_2_is_spi_busy,
        .read_irq_pin = swc_hal_radio_2_read_irq_pin,
        .radio_context_switch = swc_hal_radio_2_context_switch,
        .disable_radio_irq = swc_hal_radio_2_disable_irq_it,
        .enable_radio_irq = swc_hal_radio_2_enable_irq_it,
        .disable_radio_dma_irq = swc_hal_radio_2_disable_dma_irq_it,
        .enable_radio_dma_irq = swc_hal_radio_2_enable_dma_irq_it,
    },
};

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
/* Validator functions. */
static void validate_spi_blocking(bsp_radio_t radio_index);
static void validate_cs(bsp_radio_t radio_index);
static void validate_reset_pin(bsp_radio_t radio_index);
static void validate_transceiver_irq_pin(bsp_radio_t radio_index);
static void validate_spi_dma(bsp_radio_t radio_index);
static void validate_disable_transceiver_irq(bsp_radio_t radio_index);
static void validate_disable_dma_irq(bsp_radio_t radio_index);
static void validate_wireless_context_switch(void);
static void validate_trigger_transceiver_irq(bsp_radio_t radio_index);
static void validate_critical_section(bsp_radio_t radio_index);
static void validate_critical_section_context_switch(void);

/* Other functions */
static void reset_transceiver(bsp_radio_t radio_index);
static void read_syncword(bsp_radio_t radio_index, uint8_t *syncword);
static void write_syncword(bsp_radio_t radio_index, uint8_t *syncword);
static bool compare_reg_value(const uint8_t *buffer1, const uint8_t *buffer2, size_t size);
static void mocked_radio_1_irq_callback(void);
static void mocked_radio_2_irq_callback(void);
static void mocked_radio_1_dma_rx_callback(void);
static void mocked_radio_2_dma_rx_callback(void);
static void mocked_context_switch_callback(void);
static void print_log(log_level_t level, const char *fmt, ...);

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Validate the BSP implementation by running basic tests.
 *
 *  The tests use the SPARK SR1020 Transceiver to validate proper
 *  implementations of the board peripheral drivers.
 */
int main(void)
{
    /* Initiate basic components. */
    facade_bsp_init();
    facade_uart_init();

    print_log(LOG_LEVEL_INFO, "[==========] Running BSP validator tests with radio 1.");
    swc_hal[RADIO_ID_1].disable_radio_irq();
    swc_hal_set_radio_1_irq_callback(mocked_radio_1_irq_callback);

    swc_hal[RADIO_ID_1].disable_radio_dma_irq();
    swc_hal_set_radio_1_dma_rx_callback(mocked_radio_1_dma_rx_callback);

    validate_spi_blocking(RADIO_ID_1);
    validate_cs(RADIO_ID_1);
    validate_reset_pin(RADIO_ID_1);
    validate_transceiver_irq_pin(RADIO_ID_1);
    validate_spi_dma(RADIO_ID_1);
    validate_disable_transceiver_irq(RADIO_ID_1);
    validate_disable_dma_irq(RADIO_ID_1);
    validate_wireless_context_switch();
    validate_trigger_transceiver_irq(RADIO_ID_1);
    validate_critical_section(RADIO_ID_1);
    validate_critical_section_context_switch();
    print_log(LOG_LEVEL_INFO, "[==========] Done running all tests.");

    if (SWC_RADIO_COUNT == 2) {
        print_log(LOG_LEVEL_INFO, "[==========] Running BSP validator tests with radio 2.");
        swc_hal[RADIO_ID_2].disable_radio_irq();
        swc_hal_set_radio_2_irq_callback(mocked_radio_2_irq_callback);

        swc_hal[RADIO_ID_2].disable_radio_dma_irq();
        swc_hal_set_radio_2_dma_rx_callback(mocked_radio_2_dma_rx_callback);

        validate_spi_blocking(RADIO_ID_2);
        validate_cs(RADIO_ID_2);
        validate_reset_pin(RADIO_ID_2);
        validate_transceiver_irq_pin(RADIO_ID_2);
        validate_spi_dma(RADIO_ID_2);
        validate_disable_transceiver_irq(RADIO_ID_2);
        validate_disable_dma_irq(RADIO_ID_2);
        validate_trigger_transceiver_irq(RADIO_ID_2);
        validate_critical_section(RADIO_ID_2);
        print_log(LOG_LEVEL_INFO, "[==========] Done running all tests.");
    }

    while (1) {};
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Test the SPI blocking implementation.
 *
 *   The SPARK Wireless Core requires a basic SPI transfer blocking function.
 *   This test validates that the CS, SCLK, MOSI and MISO pins are well mapped
 *   and behave has expected by the transceiver.
 *
 *   Scenario :
 *   Use the SPI blocking method to read the SR10x0 syncword register and
 *   compare the read value with the known default value.
 *
 *  @param[in] radio_index  Selected radio index.
 */
static void validate_spi_blocking(bsp_radio_t radio_index)
{
    static const char TEST_NAME_STRING[] = "SPI blocking mode";
    uint8_t rx_data[5] = {0};

    print_log(LOG_LEVEL_INFO, "%s %s", TEST_RUN_STRING, TEST_NAME_STRING);
    reset_transceiver(radio_index);

    /* Read Syncword in blocking mode. */
    read_syncword(radio_index, rx_data);

    /* Validate that the SYNCWORD is equal to the DEFAULT one. */
    if (compare_reg_value(&rx_data[1], DEFAULT_SYNCWORD, SYNCWORD_LENGTH)) {
        print_log(LOG_LEVEL_INFO, "%s %s", TEST_OK_STRING, TEST_NAME_STRING);
    } else {
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
    }
}

/** @brief Test the Chip Select implementation.
 *
 *   The SPARK Wireless Core requires full control over the SPI Chip Select pin.
 *   This tes validates that the SPI transfer fails if the CS Pin in not controlled manually,
 *   and validate that the SPI succeeds when the CS Pin is manually toggled.
 *
 *   Scenario :
 *   Use the SPI blocking method to read the syncword register and compare
 *   the read value with the known default to make sure the operation works.
 *   Using SPI blocking method again to read back the syncword register without
 *   driving the CS low and making sure the output is not equal to the default
 *   syncword value.
 *
 *  @param[in] radio_index  Selected radio index.
 */
static void validate_cs(bsp_radio_t radio_index)
{
    static const char TEST_NAME_STRING[] = "SPI chip select";
    uint8_t rx_data[5] = {0};
    uint8_t tx_data[5] = {SYNCWORD_REGISTER | REG_READ_BURST, 0, 0, 0, 0};
    uint8_t empty_payload[4] = {0};

    print_log(LOG_LEVEL_INFO, "%s %s", TEST_RUN_STRING, TEST_NAME_STRING);
    reset_transceiver(radio_index);

    /* Read Syncword in blocking mode. */
    read_syncword(radio_index, rx_data);

    /* Validate that the SYNCWORD is equal to the writen one. */
    /* This validate that the SPI works as intended in normal operation. */
    if (compare_reg_value(&rx_data[1], DEFAULT_SYNCWORD, SYNCWORD_LENGTH) == 0) {
        print_log(LOG_LEVEL_DEBUG, "             Error during read syncword operation");
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
        return; /* Abort Scenario. */
    }

    /* Read Syncword without reseting the CS pin. */
    swc_hal[radio_index].transfer_full_duplex_blocking(tx_data, rx_data, 5);

    /* Validate that the latest SYNCWORD read equal to 0x0000. */
    /* This validate that the CS BEHAVIOUR works as intended. */
    if (compare_reg_value(&rx_data[1], empty_payload, (size_t)SYNCWORD_LENGTH)) {
        print_log(LOG_LEVEL_INFO, "%s %s", TEST_OK_STRING, TEST_NAME_STRING);
    } else {
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
    }
}

/** @brief Test the reset pin implementation.
 *
 *   Driving the Reset pin low resets the internal register of the transceiver
 *   to their default values. This test validates that the pin is well mapped
 *   and behave as the transceiver is expecting it.
 *
 *   Scenario :
 *   Write a custom syncword value to the transceiver register using the
 *   SPI Blocking method. Then read back these register to make sure that the
 *   operation works. Finally, reset the transceiver, then read the sycnword
 *   register and compare the value with the expected default one.
 *
 *  @param[in] radio_index  Selected radio index.
 */
static void validate_reset_pin(bsp_radio_t radio_index)
{
    static const char TEST_NAME_STRING[] = "Transceiver reset pin";
    uint8_t tx_data[4] = {0x01, 0x02, 0x03, 0x04};
    uint8_t rx_data[5] = {0};

    print_log(LOG_LEVEL_INFO, "%s %s", TEST_RUN_STRING, TEST_NAME_STRING);
    reset_transceiver(radio_index);

    /* Write Syncword in blocking mode. */
    write_syncword(radio_index, tx_data);

    /* Read Syncword in blocking mode. */
    read_syncword(radio_index, rx_data);

    if (!compare_reg_value(&rx_data[1], tx_data, SYNCWORD_LENGTH)) {
        print_log(LOG_LEVEL_DEBUG, "             Error during Write or Read custom syncword operation");
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
        return; /* Abort Scenario. */
    }

    /* Reset Transceiver. */
    reset_transceiver(radio_index);

    /* Read Syncword in blocking mode. */
    read_syncword(radio_index, rx_data);

    if (compare_reg_value(&rx_data[1], DEFAULT_SYNCWORD, SYNCWORD_LENGTH)) {
        print_log(LOG_LEVEL_INFO, "%s %s", TEST_OK_STRING, TEST_NAME_STRING);
    } else {
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
    }
}

/** @brief Test the transceiver IRQ pin callback read state implementations.
 *
 *   By default, when the transceiver generates an IRQ, it's IRQ Pin rises.
 *   When this happens, the BSP must read a high state on the connected MCU Pin.
 *   This state should be held until reset by the user.
 *   If enabled, a callback event should be called immediately when the
 *   IRQ pin is driven in its active state. This test validates that the IRQ pin
 *   state after an applicable event occurred on the transceiver side.
 *
 *   Scenario :
 *   Configure the transceiver to generate an IRQ when it wakes up from sleep.
 *   Read the MCU input pin state and validate it is correct. Additionally,
 *   set and enable the callback event and make sure it is triggered.
 *   The sequence of events is shown below:
 *
 *   1. Set IRQ callback function and enable the transceiver's IRQ on wake up event.
 *   2. Prepare the SPI frame with transceiver configurations and commands :
 *      a. Set up the interrupt flag to "wake up from sleep".
 *      b. Set up the sleep level to "shallow".
 *      c. Command the transceiver to go to sleep
 *   3. Transfer the payload to transceiver over SPI with the blocking method.
 *   4. Wait 1ms.
 *   5. Prepare the SPI frame with the "wake up" command and send it over SPI with the blocking method.
 *   6. Wait 10ms.
 *   7. Read transceiver's IRQ pin and assess its state.
 *
 *  @param[in] radio_index  Selected radio index.
 */
static void validate_transceiver_irq_pin(bsp_radio_t radio_index)
{
    static const char TEST_NAME_STRING[] = "Transceiver IRQ pin and event";
    uint8_t tx_data[9] = {0};
    uint8_t rx_data[9] = {0};
    uint16_t reg_value = 0;

    print_log(LOG_LEVEL_INFO, "%s %s", TEST_RUN_STRING, TEST_NAME_STRING);
    reset_transceiver(radio_index);
    swc_hal[radio_index].enable_radio_irq();

    /* Read the interrupt flag register to clear all pending flags. */
    tx_data[0] = INTERRUPT_FLAG_REGISTER;
    swc_hal[radio_index].reset_cs();
    swc_hal[radio_index].transfer_full_duplex_blocking(tx_data, rx_data, 3);
    swc_hal[radio_index].set_cs();

    /* Set interrupt flag to wake up from sleep. */
    reg_value = (uint16_t)SET_BIT_OFFSET(WAKEUPE_POSITION);
    print_log(LOG_LEVEL_DEBUG, "             Interrupt flag reg value set: %d", reg_value);
    tx_data[0] = INTERRUPT_FLAG_REGISTER | REG_WRITE;
    tx_data[1] = LSB_VALUE(reg_value);
    tx_data[2] = MSB_VALUE(reg_value);

    /* Set sleep level. */
    reg_value = (uint16_t)SET_BIT_OFFSET(SLPDEPTH_POSITION);
    print_log(LOG_LEVEL_DEBUG, "             Sleep configuration reg value set: %d", reg_value);
    tx_data[3] = SLEEP_CONFIG_REGISTER | REG_WRITE;
    tx_data[4] = LSB_VALUE(reg_value);
    tx_data[5] = MSB_VALUE(reg_value);

    /* Set the "Go to Sleep" bit to send the transceiver to sleep. This register is 8 bits only. */
    reg_value = (uint16_t)SET_BIT_OFFSET(GO_SLEEP_POSITION);
    print_log(LOG_LEVEL_DEBUG, "             Main command reg value set to go sleep: %d", reg_value);
    tx_data[6] = MAIN_COMMAND_REGISTER | REG_WRITE;
    tx_data[7] = LSB_VALUE(reg_value);

    /* Write configurations in transceiver. */
    swc_hal[radio_index].reset_cs();
    swc_hal[radio_index].transfer_full_duplex_blocking(tx_data, rx_data, 8);
    swc_hal[radio_index].set_cs();
    facade_time_delay(1);

    /* Wake up radio by clearing the SLEEP field of the register. */
    reg_value = 0;
    print_log(LOG_LEVEL_DEBUG, "             Main command reg value set to wake up: %d", reg_value);
    tx_data[0] = MAIN_COMMAND_REGISTER | REG_WRITE;
    tx_data[1] = LSB_VALUE(reg_value);

    swc_hal[radio_index].reset_cs();
    swc_hal[radio_index].transfer_full_duplex_blocking(tx_data, rx_data, 2);
    swc_hal[radio_index].set_cs();

    facade_time_delay(10);

    bool mocked_irq_flag;
    bool pin_status = swc_hal[radio_index].read_irq_pin();

    if (radio_index == RADIO_ID_1) {
        mocked_irq_flag = mocked_radio_1_irq_flag;
        mocked_radio_1_irq_flag = false;
    } else if (radio_index == RADIO_ID_2) {
        mocked_irq_flag = mocked_radio_2_irq_flag;
        mocked_radio_2_irq_flag = false;
    }

    if (mocked_irq_flag && pin_status) {
        print_log(LOG_LEVEL_INFO, "%s %s", TEST_OK_STRING, TEST_NAME_STRING);
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", mocked_irq_flag);
    } else {
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", mocked_irq_flag);
        print_log(LOG_LEVEL_DEBUG, "             Pin status %d", pin_status);
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
    }
}

/** @brief Test the SPI DMA transfer.
 *
 *   The SPARK Wireless Core requires a second SPI transfer function.
 *   This implementation must allow a non-blocking data transfer over
 *   the SPI. If enabled, the transfer completion IRQ must
 *   trigger an IRQ event which calls the configured callback function. This test validates
 *   the SPI DMA driver, the SPI DMA complete callback setter
 *   function and the IRQ configuration for the transfer completion.
 *
 *   Scenario :
 *   Set and enable the SPI DMA complete callback. Use the SPI DMA method
 *   to read the syncword register. Wait 1ms and then validate that the
 *   SPI DMA complete callback was triggered and compare the read value with
 *   the known default.
 *
 *  @param[in] radio_index  Selected radio index.
 */
static void validate_spi_dma(bsp_radio_t radio_index)
{
    static const char TEST_NAME_STRING[] = "SPI DMA and transfer complete event";
    uint8_t tx_data[5] = {SYNCWORD_REGISTER | REG_READ_BURST};
    uint8_t rx_data[5] = {0};

    print_log(LOG_LEVEL_INFO, "%s %s", TEST_RUN_STRING, TEST_NAME_STRING);
    reset_transceiver(radio_index);
    swc_hal[radio_index].enable_radio_dma_irq();

    /* Transfer payload to transceiver buffer register.*/
    swc_hal[radio_index].reset_cs();
    swc_hal[radio_index].transfer_full_duplex_non_blocking(tx_data, rx_data, 5);
    facade_time_delay(1);

    bool mocked_dma_rx_flag;

    if (radio_index == RADIO_ID_1) {
        mocked_dma_rx_flag = mocked_radio_1_dma_rx_flag;
        mocked_radio_1_dma_rx_flag = false;
    } else if (radio_index == RADIO_ID_2) {
        mocked_dma_rx_flag = mocked_radio_2_dma_rx_flag;
        mocked_radio_2_dma_rx_flag = false;
    }

    if (mocked_dma_rx_flag && compare_reg_value(&rx_data[1], DEFAULT_SYNCWORD, SYNCWORD_LENGTH)) {
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", mocked_dma_rx_flag);
        print_log(LOG_LEVEL_INFO, "%s %s", TEST_OK_STRING, TEST_NAME_STRING);
    } else {
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", mocked_dma_rx_flag);
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
    }

    swc_hal[radio_index].set_cs();
}

/** @brief Test the disable IRQ feature of the transceiver IRQ pin.
 *
 *   This test validates that the set callback function is not called when
 *   the transceiver generates an IRQ while the user chooses to disable this event.
 *
 *   Scenario :
 *   Configure the transceiver to generate an IRQ when it wakes up from sleep.
 *   Disable the MCU IRQ mapped to the transceiver's IRQ pin. Read the MCU
 *   input pin state and assess its state. Validate that the configured
 *   callback is not executed. The sequence of events is shown below:
 *
 *   1. Set IRQ callback function and disable the transceiver's IRQ on wake up event.
 *   2. Prepare the SPI frame with transceiver configurations and commands :
 *      a. Set interrupt flag to "wake up from sleep".
 *      b. Set sleep level to "shallow".
 *      c. Command the transceiver to go in sleep.
 *   1. Transfer payload to transceiver over SPI using the blocking method.
 *   2. Wait 1ms.
 *   3. Prepare the SPI frame with wake up command and send it over SPI using the blocking method.
 *   4. Wait 10ms.
 *   5. Read the transceiver's IRQ pin state and assess its state.
 *   6. Validate that the IRQ callback was not executed.
 *
 *  @param[in] radio_index  Selected radio index.
 */
static void validate_disable_transceiver_irq(bsp_radio_t radio_index)
{
    static const char TEST_NAME_STRING[] = "Disabling transceiver IRQ event";
    uint8_t tx_data[9] = {0};
    uint8_t rx_data[9] = {0};
    uint16_t reg_value = 0;

    print_log(LOG_LEVEL_INFO, "%s %s", TEST_RUN_STRING, TEST_NAME_STRING);
    reset_transceiver(radio_index);
    swc_hal[radio_index].disable_radio_irq();

    /* Read the interrupt flag register to clear all pending flags. */
    tx_data[0] = INTERRUPT_FLAG_REGISTER;
    swc_hal[radio_index].reset_cs();
    swc_hal[radio_index].transfer_full_duplex_blocking(tx_data, rx_data, 3);
    swc_hal[radio_index].set_cs();

    /* Set interrupt flag to wake up from sleep. */
    reg_value = (uint16_t)SET_BIT_OFFSET(WAKEUPE_POSITION);
    print_log(LOG_LEVEL_DEBUG, "             Interrupt flag reg value set: %d", reg_value);
    tx_data[0] = INTERRUPT_FLAG_REGISTER | REG_WRITE;
    tx_data[1] = LSB_VALUE(reg_value);
    tx_data[2] = MSB_VALUE(reg_value);

    /* Setup sleep level. */
    reg_value = (uint16_t)SET_BIT_OFFSET(SLPDEPTH_POSITION);
    print_log(LOG_LEVEL_DEBUG, "             Sleep configuration reg value set: %d", reg_value);
    tx_data[3] = SLEEP_CONFIG_REGISTER | REG_WRITE;
    tx_data[4] = LSB_VALUE(reg_value);
    tx_data[5] = MSB_VALUE(reg_value);

    /* Setup Go to Sleep. */
    reg_value = (uint16_t)SET_BIT_OFFSET(GO_SLEEP_POSITION);
    print_log(LOG_LEVEL_DEBUG, "             Main command reg value set to go sleep: %d", reg_value);
    tx_data[6] = MAIN_COMMAND_REGISTER | REG_WRITE;
    tx_data[7] = reg_value;

    /* Transfer configurations to the transceiver. */
    swc_hal[radio_index].reset_cs();
    swc_hal[radio_index].transfer_full_duplex_blocking(tx_data, rx_data, 8);
    swc_hal[radio_index].set_cs();

    facade_time_delay(1);

    /* Wake up radio by clearing the SLEEP field of the register.*/
    reg_value = 0;
    print_log(LOG_LEVEL_DEBUG, "             Main command reg value set to wake up: %d", reg_value);
    tx_data[0] = MAIN_COMMAND_REGISTER | REG_WRITE;
    tx_data[1] = reg_value;

    /* Transfer command to transceiver. */
    swc_hal[radio_index].reset_cs();
    swc_hal[radio_index].transfer_full_duplex_blocking(tx_data, rx_data, 2);
    swc_hal[radio_index].set_cs();

    facade_time_delay(25);

    bool mocked_irq_flag;
    bool pin_status = swc_hal[radio_index].read_irq_pin();

    if (radio_index == RADIO_ID_1) {
        mocked_irq_flag = mocked_radio_1_irq_flag;
        mocked_radio_1_irq_flag = false;
    } else if (radio_index == RADIO_ID_2) {
        mocked_irq_flag = mocked_radio_2_irq_flag;
        mocked_radio_2_irq_flag = false;
    }

    if (!mocked_irq_flag && pin_status) {
        print_log(LOG_LEVEL_DEBUG, "             Pin status %d", pin_status);
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", mocked_irq_flag);
        print_log(LOG_LEVEL_INFO, "%s %s", TEST_OK_STRING, TEST_NAME_STRING);
    } else {
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", mocked_irq_flag);
        print_log(LOG_LEVEL_DEBUG, "             Pin status %d", pin_status);
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
    }
}

/** @brief Test the SPI DMA transfer while transfer complete interrupt is disabled.
 *
 *   The SPARK Wireless Core requires the ability to disable the SPI DMA complete interrupt.
 *   This test validates that the SPI DMA complete can correctly be deactivated.
 *
 *   Scenario :
 *   Set and disable the SPI DMA complete callback. Use the SPI DMA method
 *   to read the syncword register. Wait 1ms and then validate that the
 *   SPI DMA complete callback was not triggered and compare the read value with
 *   the known default.
 *
 *  @param[in] radio_index  Selected radio index.
 */
static void validate_disable_dma_irq(bsp_radio_t radio_index)
{
    static const char TEST_NAME_STRING[] = "Disabling SPI DMA complete IRQ event";
    uint8_t tx_data[5] = {SYNCWORD_REGISTER | REG_READ_BURST};
    uint8_t rx_data[5] = {0};

    print_log(LOG_LEVEL_INFO, "%s %s", TEST_RUN_STRING, TEST_NAME_STRING);
    reset_transceiver(radio_index);

    swc_hal[radio_index].disable_radio_dma_irq();

    /* Transfer payload to transceiver buffer register.*/
    swc_hal[radio_index].reset_cs();
    swc_hal[radio_index].transfer_full_duplex_non_blocking(tx_data, rx_data, 5);
    facade_time_delay(1);

    bool mocked_dma_rx_flag;

    if (radio_index == RADIO_ID_1) {
        mocked_dma_rx_flag = mocked_radio_1_dma_rx_flag;
        mocked_radio_1_dma_rx_flag = false;
    } else if (radio_index == RADIO_ID_2) {
        mocked_dma_rx_flag = mocked_radio_2_dma_rx_flag;
        mocked_radio_2_dma_rx_flag = false;
    }

    if (!mocked_dma_rx_flag) {
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", mocked_dma_rx_flag);
        print_log(LOG_LEVEL_INFO, "%s %s", TEST_OK_STRING, TEST_NAME_STRING);
    } else {
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", mocked_dma_rx_flag);
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
    }

    swc_hal[radio_index].set_cs();
}

/** @brief Test baremetal context switch mecanisme.
 *
 *   The Wireless Core requires a mechanism to schedule user-configurable callback execution.
 *   This test validates the callback setter function and the custom callback execution.
 *
 *   Scenario :
 *   Set the context switch callback function, then trigger a context switch.
 *   Wait 1ms and validate the callback execution.
 */
static void validate_wireless_context_switch(void)
{
    static const char TEST_NAME_STRING[] = "Context Switch event";

    print_log(LOG_LEVEL_INFO, "%s %s", TEST_RUN_STRING, TEST_NAME_STRING);
    facade_set_context_switch_handler(mocked_context_switch_callback);
    facade_context_switch_trigger();
    facade_time_delay(1);

    if (mocked_context_switch_flag) {
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", mocked_context_switch_flag);
        print_log(LOG_LEVEL_INFO, "%s %s", TEST_OK_STRING, TEST_NAME_STRING);
    } else {
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", mocked_context_switch_flag);
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
    }
    mocked_context_switch_flag = false;
}

/** @brief Test the triggering of transceiver IRQ.
 *
 *   The Wireless Core should be able to Pend into the Transceiver IRQ.
 *
 *   Scenario :
 *   Set a mocked callback function to called when the transceiver
 *   generates an IRQ and enable the IRQ interrupt, then Pend on this IRQ.
 *   Wait 100ms and validates that the callback function is called.
 *
 *  @param[in] radio_index  Selected radio index.
 */
static void validate_trigger_transceiver_irq(bsp_radio_t radio_index)
{
    static const char TEST_NAME_STRING[] = "Set pending transceiver ISR";

    print_log(LOG_LEVEL_INFO, "%s %s", TEST_RUN_STRING, TEST_NAME_STRING);
    reset_transceiver(radio_index);
    swc_hal[radio_index].enable_radio_irq();
    facade_time_delay(1);
    swc_hal[radio_index].radio_context_switch();

    bool mocked_irq_flag;

    if (radio_index == RADIO_ID_1) {
        mocked_irq_flag = mocked_radio_1_irq_flag;
        mocked_radio_1_irq_flag = false;
    } else if (radio_index == RADIO_ID_2) {
        mocked_irq_flag = mocked_radio_2_irq_flag;
        mocked_radio_2_irq_flag = false;
    }

    if (mocked_irq_flag) {
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", mocked_irq_flag);
        print_log(LOG_LEVEL_INFO, "%s %s", TEST_OK_STRING, TEST_NAME_STRING);
    } else {
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", mocked_irq_flag);
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
    }
}

/** @brief Test the enter/exit critical section feature.
 *
 *   The Wireless Core requires the ability to enter/exit critical sections.
 *
 *   Scenario :
 *   Set the transceiver IRQ callback and validates IRQ callback actually works.
 *   Then enter critical section and generate and transceiver IRQ by pending
 *   on it. Afterwards, validate that the callback function was not called.
 *   Finally Exit the critical section and validate that the transceiver
 *   callback was called.
 *
 *  @param[in] radio_index  Selected radio index.
 */
static void validate_critical_section(bsp_radio_t radio_index)
{
    static const char TEST_NAME_STRING[] = "Enter / Exit critical section";

    print_log(LOG_LEVEL_INFO, "%s %s", TEST_RUN_STRING, TEST_NAME_STRING);

    /* This is done to make sure that the IRQ works correctly. */
    reset_transceiver(radio_index);
    swc_hal[radio_index].enable_radio_irq();
    facade_time_delay(1);
    swc_hal[radio_index].radio_context_switch();

    bool mocked_irq_flag;

    if (radio_index == RADIO_ID_1) {
        mocked_irq_flag = mocked_radio_1_irq_flag;
        mocked_radio_1_irq_flag = false;
    } else if (radio_index == RADIO_ID_2) {
        mocked_irq_flag = mocked_radio_2_irq_flag;
        mocked_radio_2_irq_flag = false;
    }

    if (!mocked_irq_flag) {
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", mocked_irq_flag);
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
        return; /* Abort scenario. */
    }
    mocked_irq_flag = false;

    /* Enter critical section and retrigger the transceiver IRQ. */
    CRITICAL_SECTION_ENTER();
    swc_hal[radio_index].radio_context_switch();

    if (radio_index == RADIO_ID_1) {
        mocked_irq_flag = mocked_radio_1_irq_flag;
        mocked_radio_1_irq_flag = false;
    } else if (radio_index == RADIO_ID_2) {
        mocked_irq_flag = mocked_radio_2_irq_flag;
        mocked_radio_2_irq_flag = false;
    }

    if (mocked_irq_flag) {
        CRITICAL_SECTION_EXIT();
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", mocked_irq_flag);
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
        CRITICAL_SECTION_EXIT();
        return; /* Abort scenario. */
    }

    CRITICAL_SECTION_EXIT();
    facade_time_delay(1);

    if (radio_index == RADIO_ID_1) {
        mocked_irq_flag = mocked_radio_1_irq_flag;
        mocked_radio_1_irq_flag = false;
    } else if (radio_index == RADIO_ID_2) {
        mocked_irq_flag = mocked_radio_2_irq_flag;
        mocked_radio_2_irq_flag = false;
    }

    if (mocked_irq_flag) {
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", mocked_irq_flag);
        print_log(LOG_LEVEL_INFO, "%s %s", TEST_OK_STRING, TEST_NAME_STRING);
    } else {
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", mocked_irq_flag);
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
    }
}

/** @brief Test the enter/exit critical section feature to make sure it disable the
 *         context switch.
 *
 *   Scenario :
 *   Set context switch IRQ callback and while
 *   in a critical section, trigger a context switch.
 *   Validate that the callback function is not called.
 *   Exit the critical section and validate that the context switch callback
 *   is called.
 */
static void validate_critical_section_context_switch(void)
{
    static const char TEST_NAME_STRING[] = "Context Switch event combined with Enter / Exit critical section";

    print_log(LOG_LEVEL_INFO, "%s %s", TEST_RUN_STRING, TEST_NAME_STRING);
    facade_set_context_switch_handler(mocked_context_switch_callback);
    facade_time_delay(1);
    facade_context_switch_trigger();

    if (!mocked_context_switch_flag) {
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", mocked_context_switch_flag);
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
        return; /* Abort scenario. */
    }
    mocked_context_switch_flag = false;

    CRITICAL_SECTION_ENTER();
    facade_context_switch_trigger();

    if (mocked_context_switch_flag) {
        CRITICAL_SECTION_EXIT();
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", mocked_context_switch_flag);
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
    }

    CRITICAL_SECTION_EXIT();
    facade_time_delay(1);

    if (mocked_context_switch_flag) {
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", mocked_context_switch_flag);
        print_log(LOG_LEVEL_INFO, "%s %s", TEST_OK_STRING, TEST_NAME_STRING);
    } else {
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", mocked_context_switch_flag);
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
    }
    mocked_context_switch_flag = false;
}

/** @brief Compare the content of two data buffers.
 *
 *   Return the buffer comparison's result. If PW_LOG module is
 *   set to LOG_LEVEL_DEBUG level first 4 bytes will be print onto the serial port.
 *
 *  @param buffer1   Pointer to the first data buffer to be compared.
 *  @param buffer2   Pointer to the second data buffer to be compared.
 *  @param size        Number of bytes to be compared.
 *
 * @return True if values are equal, false if values are not equal.
 */
bool compare_reg_value(const uint8_t *buffer1, const uint8_t *buffer2, size_t size)
{
    if (memcmp(buffer1, buffer2, size) == 0) {
        print_log(LOG_LEVEL_DEBUG, "             Values are equal.");
        return true;
    }

    print_log(LOG_LEVEL_DEBUG, "             Compare values are not equal.");
    print_log(LOG_LEVEL_DEBUG, "             Register value: %x %x %x %x", buffer1[0], buffer1[1], buffer1[2],
              buffer1[3]);
    print_log(LOG_LEVEL_DEBUG, "             Compare values: %x %x %x %x", buffer2[0], buffer2[1], buffer2[2],
              buffer2[3]);
    return false;
}

/** @brief Reset the transceiver using 50ms dwell delays.
 *
 *  @param[in] radio_index  Selected radio index.
 */
static void reset_transceiver(bsp_radio_t radio_index)
{
    swc_hal[radio_index].reset_reset_pin();
    facade_time_delay(50);
    swc_hal[radio_index].set_reset_pin();
    facade_time_delay(50);
}

/** @brief Read the syncword register.
 *
 *   Read the syncword register with SPI blocking mode. The CS pin is reset/set
 *   for this operation.
 *
 *  @param[in]  radio_index  Selected radio index.
 *  @param[out] syncword     Pointer to the syncword value.
 */
void read_syncword(bsp_radio_t radio_index, uint8_t *syncword)
{
    uint8_t tx_data[5] = {SYNCWORD_REGISTER | REG_READ_BURST, 0, 0, 0, 0};

    /* Read Syncword. */
    swc_hal[radio_index].reset_cs();
    swc_hal[radio_index].transfer_full_duplex_blocking(tx_data, syncword, 5);
    swc_hal[radio_index].set_cs();
}

/** @brief Write to the syncword register.
 *
 *   Write to the syncword register with SPI blocking mode. The CS pin is reset/set
 *   for this operation.
 *
 *  @param[in]  radio_index  Selected radio index.
 *  @param[out] syncword     Pointer to the syncword value.
 */
void write_syncword(bsp_radio_t radio_index, uint8_t *syncword)
{
    uint8_t tx_data[5];
    uint8_t rx_data[5];

    tx_data[0] = (SYNCWORD_REGISTER | REG_WRITE_BURST);
    memcpy(&tx_data[1], syncword, SYNCWORD_LENGTH);

    swc_hal[radio_index].reset_cs();
    swc_hal[radio_index].transfer_full_duplex_blocking(tx_data, rx_data, 5);
    swc_hal[radio_index].set_cs();
}

/** @brief Mock radio 1 interrupt IRQ callback.
 *
 *   Set the flag that attests that the callback was called.
 */
static void mocked_radio_1_irq_callback(void)
{
    mocked_radio_1_irq_flag = true;
}

/** @brief Mock radio 2 interrupt IRQ callback.
 *
 *   Set the flag that attests that the callback was called.
 */
static void mocked_radio_2_irq_callback(void)
{
    mocked_radio_2_irq_flag = true;
}

/** @brief Mock radio 1 DMA RX callback.
 *
 *   Set the flag that attests that the callback was called.
 */
static void mocked_radio_1_dma_rx_callback(void)
{
    mocked_radio_1_dma_rx_flag = true;
}

/** @brief Mock radio 2 DMA RX callback.
 *
 *   Set the flag that attests that the callback was called.
 */
static void mocked_radio_2_dma_rx_callback(void)
{
    mocked_radio_2_dma_rx_flag = true;
}

/** @brief Mock context switch callback.
 *
 *   Set the flag that attests that the callback was called.
 */
static void mocked_context_switch_callback(void)
{
    mocked_context_switch_flag = true;
}

/** @brief Write new print_log.
 *
 *  @param[in]  level Desired print_log level.
 *      @li LOG_LEVEL_DEBUG,
 *      @li LOG_LEVEL_INFO,
 *      @li ERROR,
 *  @param[in] fmt    Pointer to the string to print.
 *  @param[in] ...    Arguments for the string.
 */
static void print_log(log_level_t level, const char *fmt, ...)
{
    char log_buf[128];
    size_t str_size = 0;
    va_list args;

    va_start(args, fmt);
    if (level >= LOG_LEVEL) {
        str_size += snprintf(log_buf + str_size, 128 - str_size, "%s", LOG_LEVEL_STR[level]);
        str_size += vsnprintf(log_buf + str_size, 128 - str_size, fmt, args);
        snprintf(log_buf + str_size, 128 - str_size, "\n\r");

        facade_log_io(log_buf);
    }
    va_end(args);
}
