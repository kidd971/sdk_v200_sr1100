/** @file  quasar_adc.c
 *  @brief This file provides firmware functions to manage ADC features on the Quasar board.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_adc.h"
#include "quasar_clock.h"

/* CONSTANTS ******************************************************************/
/*! The maximum ADC clock frequency (55 MHz). */
#define STM32U5_MAX_ADC_CLOCK_FREQ 55000000
/*! The minimum ADC clock frequency (5 MHz). */
#define STM32U5_MIN_ADC_CLOCK_FREQ 5000000
/*! The amount of time the ADC conversion process waits before timing out. */
#define ADC_TIMEOUT 100
/*! The average temperature slope in mV/C. */
#define STM32U5_TEMP_AVERAGE_SLOPE 2.5
/*! The voltage at 30C in millivolts. */
#define STM32U5_TEMP_VOLTAGE_AT_30C 752.0
/*! The temperature offset in degrees Celsius. */
#define STM32U5_TEMP_VOLTAGE_OFFSET 30.0
/*! 3300 mV reference / 12-bits (4096 - 1) ADC resolution */
#define VOLTAGE_REF_ADC_RES_RATIO 0.805861
/**
 *  The value of the battery voltage divider ratio depend on the voltage divider resistors.
 *
 *           VBAT
 *            |
 *            R1
 *            |_____ ADC_VBAT_TO_MCU
 *            |
 *            R2
 *            |
 *           GND
 *
 *  ADC[mv] = VBAT x (R2 / R2 + R1)
 *
 *  VBAT = ADC[mv] x 1 / (R2 / R2 + R1)      => (1 / (R2 / R2 + R1)) = battery_voltage_divider_ratio
 *
 *  ADC[mv] = ADC_resolution_ratio x ADC_raw_value
 *
 *  VBAT = ADC_resolution_ratio x ADC_raw_value * divider_ratio
 */
/*! Voltage divider resistors' ratio (R1 = 100k, R2 = 300k) */
#define BATTERY_VOLTAGE_DIVIDER_RATIO 1.333

/* MACROS *********************************************************************/
/*! Convert ADC raw value to battery level in millivolt. */
#define ADC_RAW_TO_BAT_MV(raw) (VOLTAGE_REF_ADC_RES_RATIO * BATTERY_VOLTAGE_DIVIDER_RATIO * (raw))
/*! Convert ADC raw value to board revision code. */
/**
 *  To convert the raw ADC value, the 3 MSB are taken (assuming the ADC resolution is configured to 12 bits).
 *
 *  0b 1111 1111 1111 >> 9
 *
 * =
 *
 *  0b 0000 0000 0111
 * &
 *  0b 0000 0000 0111
 * ____________________
 *      Board revision
 */
#define ADC_RAW_TO_BOARD_REV(raw) (((raw) >> 9) & 0x7)


/* PRIVATE GLOBALS ************************************************************/
static ADC_HandleTypeDef hadc1 = {
    .Instance = ADC1
};

static ADC_HandleTypeDef hadc2 = {
    .Instance = ADC2
};
static uint16_t adc_voltage_reference_in_mv = 3300;
static volatile uint32_t adc_value;
static volatile uint32_t battery_level_mv;
static volatile bool is_adc_value_ready;

static void (*adc1_irq_callback)(void);
static void (*adc2_irq_callback)(void);

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void adc_init(quasar_adc_cfg_t quasar_adc_cfg);
static void adc_deinit(quasar_adc_peripheral_t adc_peripheral);
static void adc_configure_gpio(quasar_gpio_port_t gpio_port, quasar_gpio_pin_t gpio_pin);
static void adc_end_battery_level_monitoring_callback(void);
static ADC_HandleTypeDef *adc_get_handle(quasar_adc_peripheral_t adc_peripheral);
static void configure_adc_peripheral_clock(void);
static uint32_t calculate_adc_clock_prescaler(void);
static void configure_adc_channel(ADC_HandleTypeDef *hadc, quasar_adc_channel_t adc_channel);
static void unselect_adc_channel(ADC_HandleTypeDef *hadc, quasar_adc_channel_t adc_channel);

/* PUBLIC FUNCTIONS ***********************************************************/
quasar_revision_t quasar_adc_init(void)
{
    quasar_revision_t board_revision = 0xFF;

    /* Initialize the GPIOs used for board revision and battery level monitoring. */
    adc_configure_gpio(QUASAR_DEF_ADC_BAT_PORT, QUASAR_DEF_ADC_BAT_PIN);
    adc_configure_gpio(QUASAR_DEF_ADC_BOARD_REV_PORT, QUASAR_DEF_ADC_BOARD_REV_PIN);

    /* Configure the ADC1 with a 12-bit resolution. */
    quasar_adc_cfg_t quasar_adc_cfg = {
        .peripheral = QUASAR_ADC_PERIPHERAL_1,
        .resolution = QUASAR_ADC_RESOLUTION_12B,
    };
    adc_init(quasar_adc_cfg);

    /* Set the callback used after each data acquisition by interrupt method for the battery level monitoring. */
    quasar_adc_set_adc1_irq_callback(adc_end_battery_level_monitoring_callback);

    /* Reset values for battery level monitoring. */
    is_adc_value_ready = false;
    adc_value = 0;

    /* Configure channel for the board revision, get the converted value by polling method and unselect channel. */
    board_revision = quasar_adc_get_board_revision();

    /* Configure the channel for the battery level monitoring. */
    configure_adc_channel(&hadc1, QUASAR_DEF_ADC_CHANNEL_BATTERY_VOLTAGE);

    /* Since board revision won't change, only 1 data acquisition is necessary. */
    return board_revision;
}

void quasar_adc_deinit(void)
{
    adc_deinit(QUASAR_DEF_ADC_SELECTION_BATTERY_VOLTAGE);

    __HAL_RCC_ADC12_CLK_DISABLE();

    quasar_gpio_deinit(QUASAR_DEF_ADC_BAT_PORT, QUASAR_DEF_ADC_BAT_PIN);
    quasar_gpio_deinit(QUASAR_DEF_ADC_BOARD_REV_PORT, QUASAR_DEF_ADC_BOARD_REV_PIN);
}

void quasar_adc_set_voltage_reference(uint16_t voltage_reference_in_mv)
{
    adc_voltage_reference_in_mv = voltage_reference_in_mv;
}

uint16_t quasar_adc_get_voltage_reference(void)
{
    return adc_voltage_reference_in_mv;
}

uint32_t quasar_adc_start_conversion_polling(quasar_adc_peripheral_t adc_peripheral, quasar_adc_channel_t adc_channel)
{
    HAL_StatusTypeDef hal_error = HAL_OK;
    uint32_t adc_raw_value;

    /* Get the handle of the chosen ADC peripheral. */
    ADC_HandleTypeDef *hadc = adc_get_handle(adc_peripheral);

    hal_error = HAL_ADC_Start(hadc);
    if (hal_error != HAL_OK) {
        while (1);
    }

    /* Wait for the ADC conversion to finish.  */
    hal_error = HAL_ADC_PollForConversion(hadc, ADC_TIMEOUT);
    if (hal_error != HAL_OK) {
        while (1);
    }

    /* Get the value stored in the ADC data register. */
    adc_raw_value = hadc->Instance->DR;

    /* Stop the ADC peripheral. */
    hal_error = HAL_ADC_Stop(hadc);
    if (hal_error != HAL_OK) {
        while (1);
    }

    /* Unselect the selected ADC channel. */
    unselect_adc_channel(hadc, adc_channel);

    return adc_raw_value;
}

void quasar_adc_start_conversion_it(quasar_adc_peripheral_t adc_peripheral)
{
    /* Get the handle of the chosen ADC peripheral. */
    ADC_HandleTypeDef *hadc = adc_get_handle(adc_peripheral);

    if (HAL_ADC_Start_IT(hadc) != HAL_OK) {
        while (1);
    }
}

uint16_t quasar_adc_get_battery_level_mv_polling(void)
{
    uint32_t adc_raw_value = 0;

    /* Configure the selected ADC channel. */
    configure_adc_channel(&hadc1, QUASAR_DEF_ADC_CHANNEL_BATTERY_VOLTAGE);

    /* Measure the voltage at ADC1 of the channel related to the battery level. */
    adc_raw_value = quasar_adc_start_conversion_polling(QUASAR_DEF_ADC_SELECTION_BATTERY_VOLTAGE,
                                                        QUASAR_DEF_ADC_CHANNEL_BATTERY_VOLTAGE);
    return ADC_RAW_TO_BAT_MV(adc_raw_value);
}

uint16_t quasar_adc_get_battery_level_mv_it(void)
{
    if (hadc1.State == HAL_ADC_STATE_RESET) {
        return 0;
    }

    is_adc_value_ready = false;

    return ADC_RAW_TO_BAT_MV(adc_value);
}

quasar_revision_t quasar_adc_get_board_revision(void)
{
    uint32_t adc_raw_value = 0;

    /* Configure the selected ADC channel. */
    configure_adc_channel(&hadc1, QUASAR_DEF_ADC_CHANNEL_BOARD_REVISION);

    /* Measure the voltage at ADC1 of the channel related to the board revision. */
    adc_raw_value = quasar_adc_start_conversion_polling(QUASAR_DEF_ADC_SELECTION_BOARD_REVISION,
                                                        QUASAR_DEF_ADC_CHANNEL_BOARD_REVISION);
    return ADC_RAW_TO_BOARD_REV(adc_raw_value);
}

bool quasar_adc_is_battery_level_value_ready(void)
{
    if (hadc1.State == HAL_ADC_STATE_RESET) {
        return false;
    }

   return is_adc_value_ready;
}

void quasar_adc_set_adc1_irq_callback(void (*irq_callback)(void))
{
    adc1_irq_callback = irq_callback;
}

void quasar_adc_set_adc2_irq_callback(void (*irq_callback)(void))
{
    adc2_irq_callback = irq_callback;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize the ADC peripheral.
 *
 *  @note If the voltage reference changes at runtime, the init must be done.
 *  @note The associated GPIOs must be initialized separately since the number of GPIOs needed is unknown.
 *
 *  @param[in] quasar_adc_cfg  The ADC peripheral configuration.
 */
static void adc_init(quasar_adc_cfg_t quasar_adc_cfg)
{
    /* Get the handle of the chosen ADC peripheral. */
    ADC_HandleTypeDef *hadc = adc_get_handle(quasar_adc_cfg.peripheral);

    if (hadc == NULL) {
        while (1);
    }

    /* Enable VDDA supply for ADC operation. */
    HAL_PWREx_EnableVddA();

    /* Configure the ADC peripheral clock. */
    configure_adc_peripheral_clock();

    /* Initialize the ADC peripheral. */
    hadc->Init.ClockPrescaler = calculate_adc_clock_prescaler();
    hadc->Init.Resolution = quasar_adc_cfg.resolution;
    hadc->Init.GainCompensation = 0;
    hadc->Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc->Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc->Init.LowPowerAutoWait = DISABLE;
    hadc->Init.ContinuousConvMode = DISABLE;
    hadc->Init.NbrOfConversion = 1;
    hadc->Init.DiscontinuousConvMode = DISABLE;
    hadc->Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc->Init.DMAContinuousRequests = DISABLE;
    hadc->Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;
    hadc->Init.Overrun = ADC_OVR_DATA_PRESERVED;
    hadc->Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
    hadc->Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
    hadc->Init.OversamplingMode = DISABLE;
    hadc->Init.DataAlign = ADC_DATAALIGN_RIGHT;

    if (HAL_ADC_Init(hadc) != HAL_OK) {
        while (1);
    }

    /* Enable and set priority of ADC1 and ADC2 interrupt. */
    HAL_NVIC_SetPriority(ADC1_2_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(ADC1_2_IRQn);

    /* Get a better precision by calibrating at init. If ADC is used for critical monitoring,
     * calibration should be done before each data acquisition.
     */
    if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK) {
        while (1);
    }
}

/** @brief Deinitialize the ADC peripheral.
 *
 *  @param[in] adc_peripheral  The ADC peripheral to deinitialize.
 */
static void adc_deinit(quasar_adc_peripheral_t adc_peripheral)
{
    ADC_HandleTypeDef *adc_handle = adc_get_handle(adc_peripheral);

    if (HAL_ADC_DeInit(adc_handle) != HAL_OK) {
        while (1);
    }
}

/** @brief Configure the specified GPIO pin for ADC usage.
 *
 *  @param[in] port  The GPIO port to configure.
 *  @param[in] pin   The GPIO pin to configure.
 */
static void adc_configure_gpio(quasar_gpio_port_t gpio_port, quasar_gpio_pin_t gpio_pin)
{
    quasar_gpio_config_t adc_gpio = {
        .port = gpio_port,
        .pin = gpio_pin,
        .mode = QUASAR_GPIO_MODE_ANALOG,
        .type = QUASAR_GPIO_TYPE_PP,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_LOW,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(adc_gpio);
}

/** @brief Close an ADC transaction in interrupt mode.
 */
static void adc_end_battery_level_monitoring_callback(void)
{
    /* Stop the ADC peripheral. */
    if (HAL_ADC_Stop_IT(&hadc1) != HAL_OK) {
        return;
    }
    adc_value = HAL_ADC_GetValue(&hadc1);

    /* Unselect the selected ADC channel. */
    unselect_adc_channel(&hadc1, QUASAR_DEF_ADC_CHANNEL_BATTERY_VOLTAGE);

    is_adc_value_ready = true;
}

/** @brief Return the handle from the selected ADC peripheral.
 *
 *  @param[in] adc_peripheral  Available ADC peripheral.
 *  @return Selected ADC handle.
 */
static ADC_HandleTypeDef *adc_get_handle(quasar_adc_peripheral_t adc_peripheral)
{
    ADC_HandleTypeDef *adc_handle = NULL;

    switch (adc_peripheral) {
    case QUASAR_ADC_PERIPHERAL_1:
        adc_handle = &hadc1;
        break;
    case QUASAR_ADC_PERIPHERAL_2:
        adc_handle = &hadc2;
        break;
    default:
        /* Unimplemented dma. */
        break;
    }

    return adc_handle;
}

/** @brief Initialize and configure the ADC peripheral clock.
 */
static void configure_adc_peripheral_clock(void)
{
    HAL_StatusTypeDef hal_error = HAL_OK;
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    __HAL_RCC_ADC12_CLK_ENABLE();

    /* Initializes the peripherals clock */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADCDAC;
    PeriphClkInit.AdcDacClockSelection = RCC_ADCDACCLKSOURCE_SYSCLK;

    hal_error = HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
    if (hal_error != HAL_OK) {
        while (1);
    }
}

/** @brief Calculate the ADC clock prescaler to be within the acceptable clock frequency.
 *
 *  From datasheet: ADC Clock must be between 5 MHz to 55 MHz.
 *  Must verify if clock is within parameter and adjust clock prescaler otherwise.
 *
 *  @return The ADC clock prescaler value.
 */
static uint32_t calculate_adc_clock_prescaler(void)
{
    uint32_t system_clock;
    uint32_t prescaler = ADC_CLOCK_ASYNC_DIV1;

    system_clock = quasar_clock_get_system_clock_freq();

    if (system_clock > STM32U5_MAX_ADC_CLOCK_FREQ) {
        /* DIV4 was chosen as it is compatible with clock frequencies from >55 MHz to 170 MHz. */
        prescaler = ADC_CLOCK_ASYNC_DIV4;
    } else if (system_clock < STM32U5_MIN_ADC_CLOCK_FREQ) {
        while (1);
        /* Error: ADC must be between 5 to 55 MHz. */
    }

    return prescaler;
}

/** @brief Configure the ADC channel for single readings.
 *
 *  @param[in] hadc         The selected ADC handle.
 *  @param[in] adc_channel  The ADC channel to configure.
 */
static void configure_adc_channel(ADC_HandleTypeDef *hadc, quasar_adc_channel_t adc_channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    /* Configure Regular Channel */
    sConfig.Channel = adc_channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_68CYCLES;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK) {
        while (1);
    }
}

/** @brief Unselect the ADC channel.
 *
 *  @param[in] hadc         The selected ADC handle.
 *  @param[in] adc_channel  The ADC channel to unselect.
 */
static void unselect_adc_channel(ADC_HandleTypeDef *hadc, quasar_adc_channel_t adc_channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    /* Configure Regular Channel */
    sConfig.Channel = adc_channel;
    sConfig.Rank = 0;
    if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK) {
        while (1);
    }
}

/* ST HAL FUNCTIONS IMPLEMENTATIONS ********************************************/
/** @brief Implementation of weak alias for the ADC1 and ADC2 interrupt handler.
 */
void ADC1_2_IRQHandler(void)
{
    HAL_ADC_IRQHandler(&hadc1);
    HAL_ADC_IRQHandler(&hadc2);
}

/** @brief Callback when the ADC conversion is completed in interrupt mode.
 *
 *  @param[in] hadc  The ADC handle.
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    (void)hadc;

    /* Stop interrupt data acquisition, retrieve the ADC raw value and unselect the current channel. */
    if (adc1_irq_callback != NULL) {
        adc1_irq_callback();
    }
    if (adc2_irq_callback != NULL) {
        adc2_irq_callback();
    }
}
