/** @file  bsp_validator.c
 *  @brief Validate the BSP implementation by running basic tests.
 *
 *  The tests uses the SPARK SR1120 Transceiver to validate proper
 *  implementation of the board's peripheral drivers.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "bsp_validator_facade.h"
#include "critical_section.h"
#include "sr_access.h"
#include "swc_api.h"
#include "swc_hal_facade.h"

/* CONSTANTS ******************************************************************/
#define LOG_LEVEL LOG_LEVEL_INFO

/* MACROS *********************************************************************/
/*! Retrieve the LSB of a 16 bits register value. */
#define LSB_VALUE(VALUE_16BITS) (VALUE_16BITS & 0x00FF)
/*! Retrieve the MSB of a 16 bits register value. */
#define MSB_VALUE(VALUE_16BITS) (VALUE_16BITS >> 8)

/*! Register field single bit mask. */
#define BIT(n)                 (1 << (n))
#define REG_READ_BURST         BIT(7)
#define REG_WRITE              BIT(6)
#define REG_WRITE_BURST        (BIT(7) | REG_WRITE)
#define SET_BIT_OFFSET(OFFSET) (1 << OFFSET)

/*! Registers fields used to configure the radio during tests. */
#define WAKEUPE_POSITION  8
#define SLPDEPTH_POSITION 14
#define GO_SLEEP_POSITION 0

/*! HARDDISABLES_IOCONFIG Register configuration for fast MISO.  */
#define HARDDISABLES_IOCONFIG_REGISTER  0x01
#define HARDDISABLES_IOCONFIG_FAST_MISO (BIT(12) | BIT(13))

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

/** @brief Hold the information about the interrupt sources.
 */
typedef struct interrupt_sources {
    /*! The polled IRQ pin statuses. */
    bool radio_pin_status[2];
    /*! Flags showing the radio IRQ callbacks have been triggered. */
    bool mocked_radio_irq_flag[2];
    /*! Flags showing the DMA IRQ callbacks count that have been triggered. */
    uint32_t mocked_radio_non_blocking_transfer_irq_count[2];
} interrupt_sources_t;

/* PRIVATE GLOBALS ************************************************************/
static const uint8_t DEFAULT_SFD[] = {0x1D, 0xC1, 0xA6, 0x5E};
static const uint8_t SFD_REGISTER = 0x30;
static const uint8_t INTERRUPT_FLAG_REGISTER = 0x10;
static const uint8_t SLEEP_CONFIG_REGISTER = 0x0F;
static const uint8_t MAIN_COMMAND_REGISTER = 0x3B;
static const uint8_t SFD_LENGTH = 4;

static const char *const LOG_LEVEL_STR[] = {"DBG : ", "INF : ", "ERR : "};
static const char TEST_RUN_STRING[] = "[ RUN      ] ";
static const char TEST_OK_STRING[] = "[       OK ] ";
static const char TEST_FAILED_STRING[] = "[   FAILED ] ";
static volatile bool mocked_radio_1_irq_flag;
static volatile bool mocked_radio_2_irq_flag;
static volatile uint32_t mocked_radio_1_dma_transfer_cb_count;
static volatile uint32_t mocked_radio_2_dma_transfer_cb_count;
static volatile bool mocked_context_switch_flag;

static swc_hal_validator_t swc_hal[2] = {
    {
        .set_reset_pin = swc_hal_radio_1_set_reset_pin,
        .reset_reset_pin = swc_hal_radio_1_reset_reset_pin,
        .end_transfer = swc_hal_radio_1_end_transfer,
        .begin_transfer = swc_hal_radio_1_begin_transfer,
        .transfer_half_duplex_rx_blocking = swc_hal_radio_1_transfer_half_duplex_rx_blocking,
        .transfer_half_duplex_tx_blocking = swc_hal_radio_1_transfer_half_duplex_tx_blocking,
        .transfer_full_duplex_blocking = swc_hal_radio_1_transfer_full_duplex_blocking,
        .transfer_half_duplex_rx_non_blocking = swc_hal_radio_1_transfer_half_duplex_rx_non_blocking,
        .transfer_half_duplex_tx_non_blocking = swc_hal_radio_1_transfer_half_duplex_tx_non_blocking,
        .transfer_full_duplex_non_blocking = swc_hal_radio_1_transfer_full_duplex_non_blocking,
        .set_access_mode_spi = swc_hal_radio_1_set_access_mode_spi,
        .set_access_mode_qspi = swc_hal_radio_1_set_access_mode_qspi,
        .is_transfer_busy = swc_hal_radio_1_is_transfer_busy,
        .read_irq_pin = swc_hal_radio_1_read_irq_pin,
        .radio_context_switch = swc_hal_radio_1_context_switch,
        .disable_radio_irq = swc_hal_radio_1_disable_irq_it,
        .enable_radio_irq = swc_hal_radio_1_enable_irq_it,
        .disable_radio_non_blocking_transfer_irq = swc_hal_radio_1_disable_non_blocking_transfer_irq_it,
        .enable_radio_non_blocking_transfer_irq = swc_hal_radio_1_enable_non_blocking_transfer_irq_it,
    },
    {
        .set_reset_pin = swc_hal_radio_2_set_reset_pin,
        .reset_reset_pin = swc_hal_radio_2_reset_reset_pin,
        .end_transfer = swc_hal_radio_2_end_transfer,
        .begin_transfer = swc_hal_radio_2_begin_transfer,
        .transfer_half_duplex_rx_blocking = NULL,
        .transfer_half_duplex_tx_blocking = NULL,
        .transfer_full_duplex_blocking = swc_hal_radio_2_transfer_full_duplex_blocking,
        .transfer_half_duplex_rx_non_blocking = NULL,
        .transfer_half_duplex_tx_non_blocking = NULL,
        .transfer_full_duplex_non_blocking = swc_hal_radio_2_transfer_full_duplex_non_blocking,
        .set_access_mode_spi = NULL,
        .set_access_mode_qspi = NULL,
        .is_transfer_busy = swc_hal_radio_2_is_transfer_busy,
        .read_irq_pin = swc_hal_radio_2_read_irq_pin,
        .radio_context_switch = swc_hal_radio_2_context_switch,
        .disable_radio_irq = swc_hal_radio_2_disable_irq_it,
        .enable_radio_irq = swc_hal_radio_2_enable_irq_it,
        .disable_radio_non_blocking_transfer_irq = swc_hal_radio_2_disable_non_blocking_transfer_irq_it,
        .enable_radio_non_blocking_transfer_irq = swc_hal_radio_2_enable_non_blocking_transfer_irq_it,
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
static void validate_disable_non_blocking_transfer_irq(bsp_radio_t radio_index);
static void validate_wireless_context_switch(void);
static void validate_trigger_transceiver_irq(bsp_radio_t radio_index);
static void validate_critical_section(bsp_radio_t radio_index);
static void validate_critical_section_context_switch(void);

/* Other functions */
static void enable_fast_miso(bsp_radio_t radio_index);
static void reset_transceiver(bsp_radio_t radio_index);
static void read_sfd(bsp_radio_t radio_index, uint8_t *sfd);
static void write_sfd(bsp_radio_t radio_index, uint8_t *sfd);
static void config_radio_wakeup_irq(bsp_radio_t radio_index);
static bool compare_reg_value(const uint8_t *buffer1, const uint8_t *buffer2, size_t size);
static bool reg_value_differ(const uint8_t *buffer1, const uint8_t *buffer2, size_t size);
static interrupt_sources_t get_interrupt_sources(bsp_radio_t radio_index);
static void mocked_radio_1_irq_callback(void);
static void mocked_radio_2_irq_callback(void);
static void mocked_radio_1_dma_transfer_callback(void);
static void mocked_radio_2_dma_transfer_callback(void);
static void mocked_context_switch_callback(void);
static void print_log(log_level_t level, const char *fmt, ...);
static void run_radio_1_bsp_validator_tests(void);
static void run_radio_2_bsp_validator_tests(void);

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Validate the BSP implementation by running basic tests.
 *
 *  The tests use the SPARK SR1120 Transceiver to validate proper
 *  implementations of the board peripheral drivers.
 */
int main(void)
{
    /* Staged LED markers to localize a no-UART hang without a debugger. Count
     * the blinks on PD7 to see how far boot got:
     *   5 only        -> facade_bsp_init() hung/asserted (e.g. radio2 init).
     *   5, 2          -> bsp_init OK but facade_uart_init() hung.
     *   5, 2, 3       -> both inits OK; problem is UART TX itself (clock/IRQ/
     *                    baud/wiring), not reaching main or init.
     * Marker A runs pre-init at MSI ~4 MHz (small delay); markers B/C run after
     * the clock is at 160 MHz (large delay) so they stay visible.
     */
    /* Power-on LED self-test: prove each RGB channel can physically light,
     * before any init runs. Watch for blue, then green, then red in turn; any
     * colour that never lights is a dead/miswired LED (or unbonded pin), not a
     * firmware-logic problem. Runs pre-init at MSI ~4 MHz. */
    facade_debug_led_blink(FACADE_DEBUG_LED_BLUE, 3, 3000000);
    facade_debug_led_blink(FACADE_DEBUG_LED_GREEN, 3, 3000000);
    facade_debug_led_blink(FACADE_DEBUG_LED_RED, 3, 3000000);

    facade_debug_led_blink(FACADE_DEBUG_LED_BLUE, 5, 3000000);   /* A: MCU alive (pre-init) */

    /* Initiate basic components. */
    facade_bsp_init();
    facade_debug_led_blink(FACADE_DEBUG_LED_BLUE, 2, 12000000);  /* B: bsp_init returned */

    facade_uart_init();
    facade_debug_led_blink(FACADE_DEBUG_LED_BLUE, 3, 12000000);  /* C: uart_init returned */

    /* Simple TX sanity string before the structured tests. */
    facade_log_io("\r\n=== BSP VALIDATOR UART ALIVE ===\r\n");

    swc_config_hardware_interface();

    run_radio_1_bsp_validator_tests();

    if (SWC_RADIO_COUNT == 2) {
        run_radio_2_bsp_validator_tests();
    }

    /*
     * These boards have no working UART, so report the key SPI result on the
     * RGB LED instead, using a different colour per radio so they are
     * distinguishable at a glance. Read each radio's SFD register and compare
     * it to the known reset default:
     *   BLUE blinks  -> radio 1 SPI OK  (dark = radio 1 SPI FAIL)
     *   GREEN blinks -> radio 2 SPI OK  (dark = radio 2 SPI FAIL / the SPI2 issue)
     * So blue+green = Layer-1 PASS; blue only = radio 2 dead; nothing = radio 1
     * dead. Blinked once after a short gap (clearly separate from the boot
     * 5/2/3 blue markers above), then the MCU idles - no repeating pattern.
     */
    {
        uint8_t sfd[5];
        bool radio1_ok;
        bool radio2_ok = true;

        memset(sfd, 0, sizeof(sfd));
        reset_transceiver(RADIO_ID_1);
        read_sfd(RADIO_ID_1, sfd);
        radio1_ok = compare_reg_value(&sfd[1], DEFAULT_SFD, SFD_LENGTH);

        if (SWC_RADIO_COUNT == 2) {
            memset(sfd, 0, sizeof(sfd));
            reset_transceiver(RADIO_ID_2);
            read_sfd(RADIO_ID_2, sfd);
            radio2_ok = compare_reg_value(&sfd[1], DEFAULT_SFD, SFD_LENGTH);
        }

        facade_time_delay(1500);
        if (radio1_ok) {
            facade_debug_led_blink(FACADE_DEBUG_LED_BLUE, 3, 4000000);
        }
        if ((SWC_RADIO_COUNT == 2) && radio2_ok) {
            facade_debug_led_blink(FACADE_DEBUG_LED_GREEN, 3, 4000000);
        }
    }

    while (1);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Run the SPI BSP validator tests for radio 1.
 */
static void run_radio_1_bsp_validator_tests(void)
{
    reset_transceiver(RADIO_ID_1);

    print_log(LOG_LEVEL_INFO, "[==========] Running BSP validator tests with radio 1.");
    swc_hal[RADIO_ID_1].disable_radio_irq();
    swc_hal_set_radio_1_irq_callback(mocked_radio_1_irq_callback);

    swc_hal[RADIO_ID_1].disable_radio_non_blocking_transfer_irq();
    swc_hal_set_radio_1_non_blocking_transfer_callback(mocked_radio_1_dma_transfer_callback);

    validate_spi_blocking(RADIO_ID_1);
    validate_cs(RADIO_ID_1);
    validate_reset_pin(RADIO_ID_1);
    validate_transceiver_irq_pin(RADIO_ID_1);
    validate_spi_dma(RADIO_ID_1);
    validate_disable_transceiver_irq(RADIO_ID_1);
    validate_disable_non_blocking_transfer_irq(RADIO_ID_1);
    validate_wireless_context_switch();
    validate_trigger_transceiver_irq(RADIO_ID_1);
    validate_critical_section(RADIO_ID_1);
    validate_critical_section_context_switch();
    print_log(LOG_LEVEL_INFO, "[==========] Done running all radio 1 tests.");
}

/** @brief Run the SPI BSP validator tests for radio 2.
 */
static void run_radio_2_bsp_validator_tests(void)
{
    reset_transceiver(RADIO_ID_2);

    print_log(LOG_LEVEL_INFO, "[==========] Running BSP validator tests with radio 2.");
    swc_hal[RADIO_ID_2].disable_radio_irq();
    swc_hal_set_radio_2_irq_callback(mocked_radio_2_irq_callback);

    swc_hal[RADIO_ID_2].disable_radio_non_blocking_transfer_irq();
    swc_hal_set_radio_2_non_blocking_transfer_callback(mocked_radio_2_dma_transfer_callback);

    validate_spi_blocking(RADIO_ID_2);
    validate_cs(RADIO_ID_2);
    validate_reset_pin(RADIO_ID_2);
    validate_transceiver_irq_pin(RADIO_ID_2);
    validate_spi_dma(RADIO_ID_2);
    validate_disable_transceiver_irq(RADIO_ID_2);
    validate_disable_non_blocking_transfer_irq(RADIO_ID_2);
    validate_trigger_transceiver_irq(RADIO_ID_2);
    validate_critical_section(RADIO_ID_2);
    print_log(LOG_LEVEL_INFO, "[==========] Done running all radio 2 tests.");
}

/** @brief Test the SPI blocking implementation.
 *
 *  The SPARK Wireless Core requires a basic SPI transfer blocking function.
 *  This test validates that the CS, SCLK, MOSI and MISO pins are well mapped
 *  and behave has expected by the transceiver.
 *
 *  Scenario :
 *      Use the SPI blocking method to read the SR11x0 SFD register and
 *      compare the read value with the known default value.
 *
 *  @param[in] radio_index  Selected radio index.
 */
static void validate_spi_blocking(bsp_radio_t radio_index)
{
    static const char TEST_NAME_STRING[] = "SPI blocking mode";
    uint8_t rx_data[5] = {0};

    print_log(LOG_LEVEL_INFO, "%s %s", TEST_RUN_STRING, TEST_NAME_STRING);
    reset_transceiver(radio_index);

    /* Read SFD in blocking mode. */
    read_sfd(radio_index, rx_data);

    /* Validate that the SFD is equal to the DEFAULT one. */
    if (!compare_reg_value(&rx_data[1], DEFAULT_SFD, SFD_LENGTH)) {
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
        /* Abort scenario. */
        return;
    }

    print_log(LOG_LEVEL_INFO, "%s %s", TEST_OK_STRING, TEST_NAME_STRING);
}

/** @brief Test the Chip Select implementation.
 *
 *  The SPARK Wireless Core requires full control over the SPI Chip Select pin.
 *  This test validates that the SPI transfer fails if the CS Pin in not controlled manually,
 *  and validate that the SPI succeeds when the CS Pin is manually toggled.
 *
 *  Scenario :
 *      Use the SPI blocking method to read the SFD register and compare
 *      the read value with the known default to make sure the operation works.
 *      Using SPI blocking method again to read back the SFD register without
 *      driving the CS low and making sure the received data is random.
 *      Overwrite the SFD once and check again that the read value is random.
 *
 *  @param[in] radio_index  Selected radio index.
 */
static void validate_cs(bsp_radio_t radio_index)
{
    static const char TEST_NAME_STRING[] = "SPI chip select";
    uint8_t rx_data[5] = {0};
    uint8_t tx_data[5] = {SFD_REGISTER | REG_READ_BURST, 0, 0, 0, 0};

    print_log(LOG_LEVEL_INFO, "%s %s", TEST_RUN_STRING, TEST_NAME_STRING);
    reset_transceiver(radio_index);

    /* Read SFD in blocking mode. */
    read_sfd(radio_index, rx_data);

    /* Validate that the SFD is equal to the writen one. */
    /* This validate that the SPI works as intended in normal operation. */
    if (reg_value_differ(&rx_data[1], DEFAULT_SFD, SFD_LENGTH)) {
        print_log(LOG_LEVEL_DEBUG, "Error during read SFD operation");
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
        /* Abort scenario. */
        return;
    }

    /* Read SFD without reseting the CS pin. */
    if (RADIO_QSPI_ENABLED) {
        swc_hal[radio_index].transfer_half_duplex_rx_blocking(tx_data[0], &rx_data[1], 4);
    } else {
        swc_hal[radio_index].transfer_full_duplex_blocking(tx_data, rx_data, 5);
    }

    /* Validate that the actual register value is different from the received random bytes. */
    /* This validate that the CS BEHAVIOUR works as intended. */
    if (!reg_value_differ(&rx_data[1], DEFAULT_SFD, (size_t)SFD_LENGTH)) {
        print_log(LOG_LEVEL_DEBUG, "The received SFD should be random and not the default.");
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
    }

    uint8_t new_sfd[4] = {0x01, 0x02, 0x03, 0x04};

    /* Write SFD in blocking mode. */
    write_sfd(radio_index, new_sfd);

    /* Read SFD in blocking mode. */
    read_sfd(radio_index, rx_data);

    if (reg_value_differ(&rx_data[1], new_sfd, SFD_LENGTH)) {
        print_log(LOG_LEVEL_DEBUG, "Error during Write or Read custom SFD operation");
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
        /* Abort scenario. */
        return;
    }

    /* Read SFD without reseting the CS pin. */
    if (RADIO_QSPI_ENABLED) {
        swc_hal[radio_index].transfer_half_duplex_rx_blocking(tx_data[0], &rx_data[1], 4);
    } else {
        swc_hal[radio_index].transfer_full_duplex_blocking(tx_data, rx_data, 5);
    }

    /* Validate that the actual register value is different from the received random bytes. */
    /* This validate that the CS BEHAVIOUR works as intended. */
    if (!reg_value_differ(&rx_data[1], new_sfd, (size_t)SFD_LENGTH)) {
        print_log(LOG_LEVEL_DEBUG, "The received SFD should be random and not the default.");
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
        /* Abort scenario. */
        return;
    }

    print_log(LOG_LEVEL_INFO, "%s %s", TEST_OK_STRING, TEST_NAME_STRING);
}

/** @brief Test the reset pin implementation.
 *
 *  Driving the Reset pin low resets the internal register of the transceiver
 *  to their default values. This test validates that the pin is well mapped
 *  and behave as the transceiver is expecting it.
 *
 *  Scenario :
 *      Write a custom SFD value to the transceiver register using the
 *      SPI Blocking method. Then read back these register to make sure that the
 *      operation works. Finally, reset the transceiver, then read the sycnword
 *      register and compare the value with the expected default one.
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

    /* Write SFD in blocking mode. */
    write_sfd(radio_index, tx_data);

    /* Read SFD in blocking mode. */
    read_sfd(radio_index, rx_data);

    if (!compare_reg_value(&rx_data[1], tx_data, SFD_LENGTH)) {
        print_log(LOG_LEVEL_DEBUG, "             Error during Write or Read custom SFD operation");
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
        /* Abort scenario. */
        return;
    }

    /* Reset Transceiver. */
    reset_transceiver(radio_index);

    /* Read SFD in blocking mode. */
    read_sfd(radio_index, rx_data);

    if (!compare_reg_value(&rx_data[1], DEFAULT_SFD, SFD_LENGTH)) {
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
        /* Abort scenario. */
        return;
    }

    print_log(LOG_LEVEL_INFO, "%s %s", TEST_OK_STRING, TEST_NAME_STRING);
}

/** @brief Test the transceiver IRQ pin callback read state implementations.
 *
 *  By default, when the transceiver generates an IRQ, it's IRQ Pin rises.
 *  When this happens, the BSP must read a high state on the connected MCU Pin.
 *  This state should be held until reset by the user.
 *  If enabled, a callback event should be called immediately when the
 *  IRQ pin is driven in its active state. This test validates that the IRQ pin
 *  state after an applicable event occurred on the transceiver side.
 *
 *  Scenario :
 *      Configure the transceiver to generate an IRQ when it wakes up from sleep.
 *      Read the MCU input pin state and validate it is correct. Additionally,
 *      set and enable the callback event and make sure it is triggered.
 *      The sequence of events is shown below:
 *
 *  1. Set IRQ callback function and enable the transceiver's IRQ on wake up event.
 *  2. Prepare the SPI frame with transceiver configurations and commands :
 *      a. Set up the interrupt flag to "wake up from sleep".
 *      b. Set up the sleep level to "shallow".
 *      c. Command the transceiver to go to sleep
 *  3. Transfer the payload to transceiver over SPI with the blocking method.
 *  4. Wait 1ms.
 *  5. Prepare the SPI frame with the "wake up" command and send it over SPI with the blocking method.
 *  6. Wait 10ms.
 *  7. Read transceiver's IRQ pin and assess its state.
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
    swc_hal[radio_index].begin_transfer();
    if (RADIO_QSPI_ENABLED) {
        swc_hal[radio_index].transfer_half_duplex_rx_blocking(tx_data[0], &rx_data[1], 2);
    } else {
        swc_hal[radio_index].transfer_full_duplex_blocking(tx_data, rx_data, 3);
    }
    swc_hal[radio_index].end_transfer();

    /* Write configurations in transceiver. */
    config_radio_wakeup_irq(radio_index);
    facade_time_delay(1);

    /* Wake up radio by clearing the SLEEP field of the register. */
    reg_value = 0;
    print_log(LOG_LEVEL_DEBUG, "             Main command reg value set to wake up: %d", reg_value);
    tx_data[0] = MAIN_COMMAND_REGISTER | REG_WRITE;
    tx_data[1] = LSB_VALUE(reg_value);

    swc_hal[radio_index].begin_transfer();
    if (RADIO_QSPI_ENABLED) {
        swc_hal[radio_index].transfer_half_duplex_tx_blocking(tx_data[0], &tx_data[1], 1);
    } else {
        swc_hal[radio_index].transfer_full_duplex_blocking(tx_data, rx_data, 2);
    }
    swc_hal[radio_index].end_transfer();

    facade_time_delay(10);

    interrupt_sources_t i_srcs = get_interrupt_sources(radio_index);

    if (i_srcs.mocked_radio_irq_flag[radio_index] && i_srcs.radio_pin_status[radio_index]) {
        print_log(LOG_LEVEL_INFO, "%s %s", TEST_OK_STRING, TEST_NAME_STRING);
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", i_srcs.mocked_radio_irq_flag[radio_index]);
    } else {
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", i_srcs.mocked_radio_irq_flag[radio_index]);
        print_log(LOG_LEVEL_DEBUG, "             Pin status %d", i_srcs.radio_pin_status[radio_index]);
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
    }
}

/** @brief Test the SPI DMA transfer.
 *
 *  The SPARK Wireless Core requires a second SPI transfer function.
 *  This implementation must allow a non-blocking data transfer over
 *  the SPI. If enabled, the transfer completion IRQ must
 *  trigger an IRQ event which calls the configured callback function. This test validates
 *  the SPI DMA driver, the SPI DMA complete callback setter
 *  function and the IRQ configuration for the transfer completion.
 *
 *  Scenario :
 *      Set and enable the SPI DMA complete callback. Use the SPI DMA method
 *      to read the SFD register. Wait 1ms and then validate that the
 *      SPI DMA complete callback was triggered and compare the read value with
 *      the known default.
 *
 *  @param[in] radio_index  Selected radio index.
 */
static void validate_spi_dma(bsp_radio_t radio_index)
{
    /* Validate RX non-blocking. */
    static const char TEST_NAME_STRING[] = "SPI DMA and transfer complete event";
    uint8_t tx_data[5] = {SFD_REGISTER | REG_READ_BURST, 0, 0, 0, 0};
    uint8_t rx_data[5] = {0};

    print_log(LOG_LEVEL_INFO, "%s %s", TEST_RUN_STRING, TEST_NAME_STRING);
    reset_transceiver(radio_index);
    swc_hal[radio_index].enable_radio_non_blocking_transfer_irq();

    /* Transfer payload to transceiver buffer register.*/
    swc_hal[radio_index].begin_transfer();
    if (RADIO_QSPI_ENABLED) {
        swc_hal[radio_index].transfer_half_duplex_rx_non_blocking(tx_data[0], &rx_data[1], 4);
    } else {
        swc_hal[radio_index].transfer_full_duplex_non_blocking(tx_data, rx_data, 5);
    }
    facade_time_delay(1);

    interrupt_sources_t i_srcs = get_interrupt_sources(radio_index);

    if (i_srcs.mocked_radio_non_blocking_transfer_irq_count[radio_index] != 1 &&
        compare_reg_value(&rx_data[1], DEFAULT_SFD, SFD_LENGTH)) {
        print_log(LOG_LEVEL_DEBUG, "             RX callback count was %d",
                  i_srcs.mocked_radio_non_blocking_transfer_irq_count[radio_index]);
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);

        swc_hal[radio_index].end_transfer();
        return;
    }

    swc_hal[radio_index].end_transfer();

    /* Validator TX non-blocking. */
    uint8_t new_sfd[4] = {0x01, 0x02, 0x03, 0x04};

    tx_data[0] = SFD_REGISTER | REG_WRITE_BURST;
    memcpy(&tx_data[1], new_sfd, SFD_LENGTH);

    memset(rx_data, 0, sizeof(rx_data));

    swc_hal[radio_index].begin_transfer();
    if (RADIO_QSPI_ENABLED) {
        swc_hal[radio_index].transfer_half_duplex_tx_non_blocking(tx_data[0], &tx_data[1], 4);
    } else {
        swc_hal[radio_index].transfer_full_duplex_non_blocking(tx_data, rx_data, 5);
    }

    facade_time_delay(1);

    i_srcs = get_interrupt_sources(radio_index);

    if (i_srcs.mocked_radio_non_blocking_transfer_irq_count[radio_index] == 1) {
        print_log(LOG_LEVEL_DEBUG, "             TX callback count was %d",
                  i_srcs.mocked_radio_non_blocking_transfer_irq_count[radio_index]);
        print_log(LOG_LEVEL_INFO, "%s %s", TEST_OK_STRING, TEST_NAME_STRING);
    } else {
        print_log(LOG_LEVEL_DEBUG, "             TX callback count was %d",
                  i_srcs.mocked_radio_non_blocking_transfer_irq_count[radio_index]);
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
    }

    swc_hal[radio_index].end_transfer();
}

/** @brief Test the disable IRQ feature of the transceiver IRQ pin.
 *
 *  This test validates that the set callback function is not called when
 *  the transceiver generates an IRQ while the user chooses to disable this event.
 *
 *  Scenario :
 *      Configure the transceiver to generate an IRQ when it wakes up from sleep.
 *      Disable the MCU IRQ mapped to the transceiver's IRQ pin. Read the MCU
 *      input pin state and assess its state. Validate that the configured
 *      callback is not executed. The sequence of events is shown below:
 *
 *  1. Set IRQ callback function and disable the transceiver's IRQ on wake up event.
 *  2. Prepare the SPI frame with transceiver configurations and commands :
 *      a. Set interrupt flag to "wake up from sleep".
 *      b. Set sleep level to "shallow".
 *      c. Command the transceiver to go in sleep.
 *  1. Transfer payload to transceiver over SPI using the blocking method.
 *  2. Wait 1ms.
 *  3. Prepare the SPI frame with wake up command and send it over SPI using the blocking method.
 *  4. Wait 10ms.
 *  5. Read the transceiver's IRQ pin state and assess its state.
 *  6. Validate that the IRQ callback was not executed.
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
    swc_hal[radio_index].begin_transfer();
    if (RADIO_QSPI_ENABLED) {
        swc_hal[radio_index].transfer_half_duplex_rx_blocking(tx_data[0], &rx_data[1], 2);
    } else {
        swc_hal[radio_index].transfer_full_duplex_blocking(tx_data, rx_data, 3);
    }
    swc_hal[radio_index].end_transfer();

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
    swc_hal[radio_index].begin_transfer();
    if (RADIO_QSPI_ENABLED) {
        swc_hal[radio_index].transfer_half_duplex_tx_blocking(tx_data[0], &tx_data[1], 7);
    } else {
        swc_hal[radio_index].transfer_full_duplex_blocking(tx_data, rx_data, 8);
    }
    swc_hal[radio_index].end_transfer();

    facade_time_delay(1);

    /* Wake up radio by clearing the SLEEP field of the register.*/
    reg_value = 0;
    print_log(LOG_LEVEL_DEBUG, "             Main command reg value set to wake up: %d", reg_value);
    tx_data[0] = MAIN_COMMAND_REGISTER | REG_WRITE;
    tx_data[1] = reg_value;

    /* Transfer command to transceiver. */
    swc_hal[radio_index].begin_transfer();
    if (RADIO_QSPI_ENABLED) {
        swc_hal[radio_index].transfer_half_duplex_tx_blocking(tx_data[0], &tx_data[1], 1);
    } else {
        swc_hal[radio_index].transfer_full_duplex_blocking(tx_data, rx_data, 2);
    }
    swc_hal[radio_index].end_transfer();

    facade_time_delay(25);

    interrupt_sources_t i_srcs = get_interrupt_sources(radio_index);

    if (!i_srcs.mocked_radio_irq_flag[radio_index] && i_srcs.radio_pin_status[radio_index]) {
        print_log(LOG_LEVEL_DEBUG, "             Pin status %d", i_srcs.radio_pin_status[radio_index]);
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", i_srcs.mocked_radio_irq_flag[radio_index]);
        print_log(LOG_LEVEL_INFO, "%s %s", TEST_OK_STRING, TEST_NAME_STRING);
    } else {
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", i_srcs.mocked_radio_irq_flag[radio_index]);
        print_log(LOG_LEVEL_DEBUG, "             Pin status %d", i_srcs.radio_pin_status[radio_index]);
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
    }
}

/** @brief Test the SPI DMA transfer while transfer complete interrupt is disabled.
 *
 *  The SPARK Wireless Core requires the ability to disable the SPI DMA complete interrupt.
 *  This test validates that the SPI DMA complete can correctly be deactivated.
 *
 *  Scenario :
 *      Set and disable the SPI DMA complete callback. Use the SPI DMA method
 *      to read the SFD register. Wait 1ms and then validate that the
 *      SPI DMA complete callback was not triggered and compare the read value with
 *      the known default.
 *
 *  @param[in] radio_index  Selected radio index.
 */
static void validate_disable_non_blocking_transfer_irq(bsp_radio_t radio_index)
{
    static const char TEST_NAME_STRING[] = "Disabling SPI DMA complete IRQ event";
    uint8_t tx_data[5] = {SFD_REGISTER | REG_READ_BURST, 0, 0, 0, 0};
    uint8_t rx_data[5] = {0};

    print_log(LOG_LEVEL_INFO, "%s %s", TEST_RUN_STRING, TEST_NAME_STRING);
    reset_transceiver(radio_index);

    swc_hal[radio_index].disable_radio_non_blocking_transfer_irq();

    /* Transfer payload to transceiver buffer register.*/
    swc_hal[radio_index].begin_transfer();
    if (RADIO_QSPI_ENABLED) {
        swc_hal[radio_index].transfer_half_duplex_tx_non_blocking(tx_data[0], &tx_data[1], 4);
    } else {
        swc_hal[radio_index].transfer_full_duplex_non_blocking(tx_data, rx_data, 5);
    }
    facade_time_delay(1);

    interrupt_sources_t i_srcs = get_interrupt_sources(radio_index);

    if (i_srcs.mocked_radio_non_blocking_transfer_irq_count[radio_index] == 0) {
        print_log(LOG_LEVEL_DEBUG, "             Callback count was %d",
                  i_srcs.mocked_radio_non_blocking_transfer_irq_count[radio_index]);
        print_log(LOG_LEVEL_INFO, "%s %s", TEST_OK_STRING, TEST_NAME_STRING);
    } else {
        print_log(LOG_LEVEL_DEBUG, "             Callback count was %d",
                  i_srcs.mocked_radio_non_blocking_transfer_irq_count[radio_index]);
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
    }

    swc_hal[radio_index].end_transfer();
}

/** @brief Test baremetal context switch mecanisme.
 *
 *  The Wireless Core requires a mechanism to schedule user-configurable callback execution.
 *  This test validates the callback setter function and the custom callback execution.
 *
 *  Scenario :
 *      Set the context switch callback function, then trigger a context switch.
 *      Wait 1ms and validate the callback execution.
 */
static void validate_wireless_context_switch(void)
{
    static const char TEST_NAME_STRING[] = "Context Switch event";

    print_log(LOG_LEVEL_INFO, "%s %s", TEST_RUN_STRING, TEST_NAME_STRING);
    facade_set_context_switch_handler(mocked_context_switch_callback);
    facade_context_switch_trigger();

    if (mocked_context_switch_flag) {
        print_log(LOG_LEVEL_DEBUG, "              Callback status was %d", mocked_context_switch_flag);
        print_log(LOG_LEVEL_INFO, "%s %s", TEST_OK_STRING, TEST_NAME_STRING);
    } else {
        print_log(LOG_LEVEL_DEBUG, "              Callback status was %d", mocked_context_switch_flag);
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
        if (mocked_context_switch_flag) {
            print_log(LOG_LEVEL_DEBUG, "              Make sure context switch trigger uses synchronization barriers.");
        }
    }
    mocked_context_switch_flag = false;
}

/** @brief Test the triggering of transceiver IRQ.
 *
 *  The Wireless Core should be able to Pend into the Transceiver IRQ.
 *
 *  Scenario :
 *      Set a mocked callback function to called when the transceiver
 *      generates an IRQ and enable the IRQ interrupt, then Pend on this IRQ.
 *      Wait 100ms and validates that the callback function is called.
 *
 *  @param[in] radio_index  Selected radio index.
 */
static void validate_trigger_transceiver_irq(bsp_radio_t radio_index)
{
    static const char TEST_NAME_STRING[] = "Set pending transceiver ISR";

    print_log(LOG_LEVEL_INFO, "%s %s", TEST_RUN_STRING, TEST_NAME_STRING);
    reset_transceiver(radio_index);
    swc_hal[radio_index].enable_radio_irq();
    swc_hal[radio_index].radio_context_switch();

    interrupt_sources_t i_srcs = get_interrupt_sources(radio_index);

    if (i_srcs.mocked_radio_irq_flag[radio_index]) {
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", i_srcs.mocked_radio_irq_flag[radio_index]);
        print_log(LOG_LEVEL_INFO, "%s %s", TEST_OK_STRING, TEST_NAME_STRING);
    } else {
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", i_srcs.mocked_radio_irq_flag[radio_index]);
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
    }
}

/** @brief Test the enter/exit critical section feature.
 *
 *  The Wireless Core requires the ability to enter/exit critical sections.
 *
 *  Scenario :
 *      Set the transceiver IRQ callback and validates IRQ callback actually works.
 *      Then enter critical section and generate and transceiver IRQ by pending
 *      on it. Afterwards, validate that the callback function was not called.
 *      Finally Exit the critical section and validate that the transceiver
 *      callback was called.
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
    swc_hal[radio_index].radio_context_switch();

    interrupt_sources_t i_srcs = get_interrupt_sources(radio_index);

    if (!i_srcs.mocked_radio_irq_flag[radio_index]) {
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", i_srcs.mocked_radio_irq_flag[radio_index]);
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
        /* Abort scenario. */
        return;
    }

    /* Enter critical section and retrigger the transceiver IRQ. */
    CRITICAL_SECTION_ENTER();
    swc_hal[radio_index].radio_context_switch();

    i_srcs = get_interrupt_sources(radio_index);

    if (i_srcs.mocked_radio_irq_flag[radio_index]) {
        CRITICAL_SECTION_EXIT();
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", i_srcs.mocked_radio_irq_flag[radio_index]);
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
        CRITICAL_SECTION_EXIT();
        /* Abort scenario. */
        return;
    }

    CRITICAL_SECTION_EXIT();

    i_srcs = get_interrupt_sources(radio_index);

    if (i_srcs.mocked_radio_irq_flag[radio_index]) {
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", i_srcs.mocked_radio_irq_flag[radio_index]);
        print_log(LOG_LEVEL_INFO, "%s %s", TEST_OK_STRING, TEST_NAME_STRING);
    } else {
        print_log(LOG_LEVEL_DEBUG, "             Callback status was %d", i_srcs.mocked_radio_irq_flag[radio_index]);
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
    }
}

/** @brief Test the enter/exit critical section feature to make sure it disable the.
 *  context switch.
 *
 *  Scenario :
 *      Set context switch IRQ callback and while
 *      in a critical section, trigger a context switch.
 *      Validate that the callback function is not called.
 *      Exit the critical section and validate that the context switch callback
 *      is called.
 */
static void validate_critical_section_context_switch(void)
{
    static const char TEST_NAME_STRING[] = "Context Switch event combined with Enter / Exit critical section";

    mocked_context_switch_flag = false;

    print_log(LOG_LEVEL_INFO, "%s %s", TEST_RUN_STRING, TEST_NAME_STRING);
    facade_set_context_switch_handler(mocked_context_switch_callback);
    facade_context_switch_trigger();

    if (!mocked_context_switch_flag) {
        print_log(LOG_LEVEL_DEBUG, "              Callback status was %d", mocked_context_switch_flag);
        print_log(LOG_LEVEL_ERR, "%s %s", TEST_FAILED_STRING, TEST_NAME_STRING);
        if (mocked_context_switch_flag) {
            print_log(LOG_LEVEL_DEBUG, "              Make sure context switch trigger uses synchronization barriers.");
        }
        /* Abort scenario. */
        return;
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
 *  Return the buffer comparison's result. If PW_LOG module is
 *  set to LOG_LEVEL_DEBUG level first 4 bytes will be print onto the serial port.
 *
 *  @param[in] buffer1  Pointer to the first data buffer to be compared.
 *  @param[in] buffer2  Pointer to the second data buffer to be compared.
 *  @param[in] size     Number of bytes to be compared.
 *
 *  @retval True  If values are equal.
 *  @retval False  If values are not equal.
 */
static bool compare_reg_value(const uint8_t *buffer1, const uint8_t *buffer2, size_t size)
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

/** @brief Verify if the register values are different.
 *
 *  @param[in] buffer1  Pointer to the first data buffer to be compared.
 *  @param[in] buffer2  Pointer to the second data buffer to be compared.
 *  @param[in] size     Number of bytes to be compared.
 *
 *  @retval True  If values are different.
 *  @retval False  If values are not equal.
 */
static bool reg_value_differ(const uint8_t *buffer1, const uint8_t *buffer2, size_t size)
{
    if (memcmp(buffer1, buffer2, size) != 0) {
        print_log(LOG_LEVEL_DEBUG, "             Values differ.");
        return true;
    }

    print_log(LOG_LEVEL_DEBUG, "             Compare values are equal.");
    print_log(LOG_LEVEL_DEBUG, "             Register value: %x %x %x %x", buffer1[0], buffer1[1], buffer1[2],
              buffer1[3]);
    print_log(LOG_LEVEL_DEBUG, "             Compare values: %x %x %x %x", buffer2[0], buffer2[1], buffer2[2],
              buffer2[3]);
    return false;
}

/** @brief Get all the interrupt sources such as radio IRQ pin and DMA IRQ.
 *
 *  @param[in] radio_index  The radio index to get the IRQ sources for.
 *
 *  @return The interrupt sources.
 */
static interrupt_sources_t get_interrupt_sources(bsp_radio_t radio_index)
{
    interrupt_sources_t i_srcs;

    i_srcs.radio_pin_status[radio_index] = swc_hal[radio_index].read_irq_pin();

    switch (radio_index) {
    case RADIO_ID_1:
        i_srcs.mocked_radio_irq_flag[radio_index] = mocked_radio_1_irq_flag;
        i_srcs.mocked_radio_non_blocking_transfer_irq_count[radio_index] = mocked_radio_1_dma_transfer_cb_count;
        mocked_radio_1_irq_flag = false;
        mocked_radio_1_dma_transfer_cb_count = 0;
        break;
    case RADIO_ID_2:
        i_srcs.mocked_radio_irq_flag[radio_index] = mocked_radio_2_irq_flag;
        i_srcs.mocked_radio_non_blocking_transfer_irq_count[radio_index] = mocked_radio_2_dma_transfer_cb_count;
        mocked_radio_2_irq_flag = false;
        mocked_radio_2_dma_transfer_cb_count = 0;
        break;
    default:
        while (1);
    }

    return i_srcs;
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

    /* Set the peripheral communication mode to SPI since the radio just got reset. */
    sr_access_set_mode(radio_index, SPI);

    /* Enable Fast MISO after radio reset. */
    enable_fast_miso(radio_index);

    /* Configure communication mode with the radio. */

    /* Get the current config in the register. */
    uint16_t register_value = sr_access_read_reg16(radio_index, REG16_HARDDISABLES_IOCONFIG);

    /* Add the desired access mode (SPI/QSPI). */
    register_value = register_value & ~BITS_QSPI;
    register_value |= RADIO_QSPI_ENABLED ? QSPI_0b10 : QSPI_0b00;
    sr_access_write_reg16(radio_index, REG16_HARDDISABLES_IOCONFIG, register_value);

    /* Turn on the appropriate access mode. */
    sr_access_set_mode(radio_index, RADIO_QSPI_ENABLED ? QSPI : SPI);
}

/** @brief Enable Fast MISO to make sure SPI reads are accurate.
 *
 *  @param[in] radio_index  Selected radio index.
 */
static void enable_fast_miso(bsp_radio_t radio_index)
{
    uint8_t tx_data[3] = {HARDDISABLES_IOCONFIG_REGISTER | REG_WRITE, 0, 0};
    uint8_t rx_data[3] = {0, 0, 0};

    /* Write. */
    *((uint16_t *)(&tx_data[1])) |= HARDDISABLES_IOCONFIG_FAST_MISO;
    swc_hal[radio_index].begin_transfer();
    if (RADIO_QSPI_ENABLED) {
        swc_hal[radio_index].transfer_half_duplex_tx_blocking(tx_data[0], &tx_data[1], sizeof(tx_data) - 1);
    } else {
        swc_hal[radio_index].transfer_full_duplex_blocking(tx_data, rx_data, sizeof(tx_data));
    }
    swc_hal[radio_index].end_transfer();
}

/** @brief Read the SFD register.
 *
 *  Read the SFD register with SPI blocking mode. The CS pin is reset/set
 *  for this operation.
 *
 *  @param[in]  radio_index  Selected radio index.
 *  @param[out] sfd          Pointer to the SFD value.
 */
void read_sfd(bsp_radio_t radio_index, uint8_t *sfd)
{
    uint8_t tx_data[5] = {SFD_REGISTER | REG_READ_BURST, 0, 0, 0, 0};

    /* Read SFD. */
    swc_hal[radio_index].begin_transfer();
    if (RADIO_QSPI_ENABLED) {
        swc_hal[radio_index].transfer_half_duplex_rx_blocking(tx_data[0], sfd + 1, 4);
    } else {
        swc_hal[radio_index].transfer_full_duplex_blocking(tx_data, sfd, 5);
    }

    swc_hal[radio_index].end_transfer();
}

/** @brief Write to the SFD register.
 *
 *  Write to the SFD register with SPI blocking mode. The CS pin is reset/set
 *  for this operation.
 *
 *  @param[in]  radio_index  Selected radio index.
 *  @param[out] sfd          Pointer to the SFD value.
 */
void write_sfd(bsp_radio_t radio_index, uint8_t *sfd)
{
    uint8_t tx_data[5] = {SFD_REGISTER | REG_WRITE_BURST, 0, 0, 0, 0};
    uint8_t rx_data[5] = {0};

    memcpy(&tx_data[1], sfd, SFD_LENGTH);

    swc_hal[radio_index].begin_transfer();
    if (RADIO_QSPI_ENABLED) {
        swc_hal[radio_index].transfer_half_duplex_tx_blocking(tx_data[0], &tx_data[1], 4);
    } else {
        swc_hal[radio_index].transfer_full_duplex_blocking(tx_data, rx_data, 5);
    }
    swc_hal[radio_index].end_transfer();
}

/** @brief Configure the radio to enter sleep mode and create an IRQ when it wakes up.
 *
 *  @param[in] radio_index  The radio index.
 */
static void config_radio_wakeup_irq(bsp_radio_t radio_index)
{
    uint8_t tx_data[9] = {0};
    uint8_t rx_data[9] = {0};
    uint16_t reg_value = 0;

    /* Set interrupt flag to wake up from sleep. */
    reg_value = (uint16_t)SET_BIT_OFFSET(WAKEUPE_POSITION);
    print_log(LOG_LEVEL_DEBUG, "             Interrupt flag reg value set: %d", reg_value);
    tx_data[0] = INTERRUPT_FLAG_REGISTER | REG_WRITE;
    tx_data[1] = LSB_VALUE(reg_value);
    tx_data[2] = MSB_VALUE(reg_value);

    if (RADIO_QSPI_ENABLED) {
        swc_hal[radio_index].begin_transfer();
        swc_hal[radio_index].transfer_half_duplex_tx_blocking(tx_data[0], &tx_data[1], 2);
        swc_hal[radio_index].end_transfer();
    }

    /* Set sleep level. */
    reg_value = (uint16_t)SET_BIT_OFFSET(SLPDEPTH_POSITION);
    print_log(LOG_LEVEL_DEBUG, "             Sleep configuration reg value set: %d", reg_value);
    tx_data[3] = SLEEP_CONFIG_REGISTER | REG_WRITE;
    tx_data[4] = LSB_VALUE(reg_value);
    tx_data[5] = MSB_VALUE(reg_value);

    if (RADIO_QSPI_ENABLED) {
        swc_hal[radio_index].begin_transfer();
        swc_hal[radio_index].transfer_half_duplex_tx_blocking(tx_data[3], &tx_data[4], 2);
        swc_hal[radio_index].end_transfer();
    }

    /* Set the "Go to Sleep" bit to send the transceiver to sleep. This register is 8 bits only. */
    reg_value = (uint16_t)SET_BIT_OFFSET(GO_SLEEP_POSITION);
    print_log(LOG_LEVEL_DEBUG, "             Main command reg value set to go sleep: %d", reg_value);
    tx_data[6] = MAIN_COMMAND_REGISTER | REG_WRITE;
    tx_data[7] = LSB_VALUE(reg_value);

    if (RADIO_QSPI_ENABLED) {
        swc_hal[radio_index].begin_transfer();
        swc_hal[radio_index].transfer_half_duplex_tx_blocking(tx_data[6], &tx_data[7], 1);
        swc_hal[radio_index].end_transfer();
        return;
    }

    /* Normal SPI operation. */
    swc_hal[radio_index].begin_transfer();
    swc_hal[radio_index].transfer_full_duplex_blocking(tx_data, rx_data, 8);
    swc_hal[radio_index].end_transfer();
}

/** @brief Mock radio 1 interrupt IRQ callback.
 *
 *  Set the flag that attests that the callback was called.
 */
static void mocked_radio_1_irq_callback(void)
{
    mocked_radio_1_irq_flag = true;
}

/** @brief Mock radio 2 interrupt IRQ callback.
 *
 *  Set the flag that attests that the callback was called.
 */
static void mocked_radio_2_irq_callback(void)
{
    mocked_radio_2_irq_flag = true;
}

/** @brief Mock radio 1 DMA transfer callback.
 *
 *  Set the flag that attests that the callback was called.
 */
static void mocked_radio_1_dma_transfer_callback(void)
{
    mocked_radio_1_dma_transfer_cb_count += 1;
}

/** @brief Mock radio 2 DMA transfer callback.
 *
 *  Set the flag that attests that the callback was called.
 */
static void mocked_radio_2_dma_transfer_callback(void)
{
    mocked_radio_2_dma_transfer_cb_count += 1;
}

/** @brief Mock context switch callback.
 *
 *  Set the flag that attests that the callback was called.
 */
static void mocked_context_switch_callback(void)
{
    mocked_context_switch_flag = true;
}

/** @brief Write new print_log.
 *
 *  @param[in] level            Desired print_log level.
 *   @li       LOG_LEVEL_DEBUG  Debug logging level.
 *   @li       LOG_LEVEL_INFO   Info logging level.
 *   @li       ERROR            Error logging level.
 *  @param[in] fmt              Pointer to the string to print.
 *  @param[in] ...              Arguments for the string.
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
