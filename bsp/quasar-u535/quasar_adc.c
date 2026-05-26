/** @file  quasar_adc.c
 *  @brief This file provides firmware functions to manage ADC features on the Quasar board.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
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
/*! Max raw value for the ADC peripheral. */
#define ADC_MAX_RAW_VALUE ((1 << 12) - 1)
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
 *  VBAT = ADC_resolution_ratio x ADC_raw_value x divider_ratio
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
    .Instance = ADC1,
};

#if !defined(STM32U535xx)
static ADC_HandleTypeDef hadc2 = {
    .Instance = ADC2,
};
#endif
static uint16_t adc_voltage_reference_in_mv = 3300;
static volatile uint32_t adc_value;
static volatile uint32_t battery_level_mv;
static volatile bool is_adc_value_ready;

static void (*adc1_irq_callback)(void);
#if !defined(STM32U535xx)
static void (*adc2_irq_callback)(void);
#endif

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void adc_init(quasar_adc_cfg_t quasar_adc_cfg, quasar_bsp_status_t *err);
static void adc_deinit(quasar_adc_peripheral_t adc_peripheral, quasar_bsp_status_t *err);
static void adc_configure_gpio(GPIO_TypeDef *gpio_port, quasar_gpio_pin_t gpio_pin);
static void adc_end_battery_level_monitoring_callback(void);
static ADC_HandleTypeDef *adc_get_handle(quasar_adc_peripheral_t adc_peripheral);
static void configure_adc_peripheral_clock(quasar_bsp_status_t *err);
static uint32_t calculate_adc_clock_prescaler(quasar_bsp_status_t *err);
static void configure_adc_channel(ADC_HandleTypeDef *hadc, quasar_adc_channel_t adc_channel, quasar_bsp_status_t *err);
static void unselect_adc_channel(ADC_HandleTypeDef *hadc, quasar_adc_channel_t adc_channel, quasar_bsp_status_t *err);

/* PUBLIC FUNCTIONS ***********************************************************/
quasar_revision_t quasar_adc_init(quasar_bsp_status_t *err)
{
    quasar_revision_t board_revision = UINT8_MAX;

    *err = QUASAR_OK;

    /* Initialize the GPIOs used for board revision and battery level monitoring. */
    adc_configure_gpio(QUASAR_DEF_ADC_BAT_PORT, QUASAR_DEF_ADC_BAT_PIN);
    // adc_configure_gpio(QUASAR_DEF_ADC_BOARD_REV_PORT, QUASAR_DEF_ADC_BOARD_REV_PIN);

    /* Configure the ADC1 with a 12-bit resolution. */
    quasar_adc_cfg_t quasar_adc_cfg = {
        .peripheral = QUASAR_ADC_PERIPHERAL_1,
        .resolution = QUASAR_ADC_RESOLUTION_12B,
    };
    adc_init(quasar_adc_cfg, err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return board_revision);

    /* Set the callback used after each data acquisition by interrupt method for the battery level monitoring. */
    quasar_adc_set_adc1_irq_callback(adc_end_battery_level_monitoring_callback);

    /* Reset values for battery level monitoring. */
    is_adc_value_ready = false;
    adc_value = 0;

    /* Configure channel for the board revision, get the converted value by polling method and unselect channel. */
    board_revision = quasar_adc_get_board_revision(err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return board_revision);

    /* Configure the channel for the battery level monitoring. */
    configure_adc_channel(&hadc1, QUASAR_DEF_ADC_CHANNEL_BATTERY_VOLTAGE, err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return board_revision);

    /* Since board revision won't change, only 1 data acquisition is necessary. */
    return board_revision;
}

void quasar_adc_deinit(quasar_bsp_status_t *err)
{
    *err = QUASAR_OK;

    adc_deinit(QUASAR_DEF_ADC_SELECTION_BATTERY_VOLTAGE, err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);

    __HAL_RCC_ADC12_CLK_DISABLE();

    quasar_gpio_deinit(QUASAR_DEF_ADC_BAT_PORT, QUASAR_DEF_ADC_BAT_PIN);
    // quasar_gpio_deinit(QUASAR_DEF_ADC_BOARD_REV_PORT, QUASAR_DEF_ADC_BOARD_REV_PIN);
}

void quasar_adc_set_voltage_reference(uint16_t voltage_reference_in_mv)
{
    adc_voltage_reference_in_mv = voltage_reference_in_mv;
}

uint16_t quasar_adc_get_voltage_reference(void)
{
    return adc_voltage_reference_in_mv;
}

uint32_t quasar_adc_start_conversion_polling(quasar_adc_peripheral_t adc_peripheral, quasar_adc_channel_t adc_channel,
                                             quasar_bsp_status_t *err)
{
    *err = QUASAR_OK;

    HAL_StatusTypeDef hal_error = HAL_OK;
    uint32_t adc_raw_value = UINT32_MAX;

    /* Get the handle of the chosen ADC peripheral. */
    ADC_HandleTypeDef *hadc = adc_get_handle(adc_peripheral);

    hal_error = HAL_ADC_Start(hadc);
    QUASAR_BSP_CHECK_ERROR(hal_error != HAL_OK, err, QUASAR_ERR_ADC_START, return adc_raw_value);

    /* Wait for the ADC conversion to finish.  */
    hal_error = HAL_ADC_PollForConversion(hadc, ADC_TIMEOUT);
    QUASAR_BSP_CHECK_ERROR(hal_error != HAL_OK, err, QUASAR_ERR_ADC_START, return adc_raw_value);

    /* Get the value stored in the ADC data register. */
    adc_raw_value = hadc->Instance->DR;
    QUASAR_BSP_CHECK_ERROR(adc_raw_value > ADC_MAX_RAW_VALUE, err, QUASAR_ERR_ADC, return adc_raw_value);

    /* Stop the ADC peripheral. */
    hal_error = HAL_ADC_Stop(hadc);
    QUASAR_BSP_CHECK_ERROR(hal_error != HAL_OK, err, QUASAR_ERR_ADC_START, return adc_raw_value);

    /* Unselect the selected ADC channel. */
    unselect_adc_channel(hadc, adc_channel, err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return adc_raw_value);

    return adc_raw_value;
}

void quasar_adc_start_conversion_it(quasar_adc_peripheral_t adc_peripheral, quasar_bsp_status_t *err)
{
    *err = QUASAR_OK;

    /* Get the handle of the chosen ADC peripheral. */
    ADC_HandleTypeDef *hadc = adc_get_handle(adc_peripheral);

    QUASAR_BSP_CHECK_ERROR(HAL_ADC_Start_IT(hadc) != HAL_OK, err, QUASAR_ERR_ADC_START, return);
}

uint16_t quasar_adc_get_battery_level_mv_polling(quasar_bsp_status_t *err)
{
    *err = QUASAR_OK;
    uint32_t adc_raw_value = UINT32_MAX;

    /* Configure the selected ADC channel. */
    configure_adc_channel(&hadc1, QUASAR_DEF_ADC_CHANNEL_BATTERY_VOLTAGE, err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return 0);

    /* Measure the voltage at ADC1 of the channel related to the battery level. */
    adc_raw_value = quasar_adc_start_conversion_polling(QUASAR_DEF_ADC_SELECTION_BATTERY_VOLTAGE,
                                                        QUASAR_DEF_ADC_CHANNEL_BATTERY_VOLTAGE, err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return 0);

    return ADC_RAW_TO_BAT_MV(adc_raw_value);
}

uint16_t quasar_adc_get_battery_level_mv_it(quasar_bsp_status_t *err)
{
    *err = QUASAR_OK;

    if (hadc1.State == HAL_ADC_STATE_RESET) {
        *err = QUASAR_ERR_ADC;
        return 0;
    }

    is_adc_value_ready = false;

    return ADC_RAW_TO_BAT_MV(adc_value);
}

quasar_revision_t quasar_adc_get_board_revision(quasar_bsp_status_t *err)
{
    *err = QUASAR_OK;
    uint32_t adc_raw_value = UINT32_MAX;

    /* Configure the selected ADC channel. */
    configure_adc_channel(&hadc1, QUASAR_DEF_ADC_CHANNEL_BOARD_REVISION, err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return UINT8_MAX);

    /* Measure the voltage at ADC1 of the channel related to the board revision. */
    adc_raw_value = quasar_adc_start_conversion_polling(QUASAR_DEF_ADC_SELECTION_BOARD_REVISION,
                                                        QUASAR_DEF_ADC_CHANNEL_BOARD_REVISION, err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return UINT8_MAX);

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

#if !defined(STM32U535xx)
void quasar_adc_set_adc2_irq_callback(void (*irq_callback)(void))
{
    adc2_irq_callback = irq_callback;
}
#endif

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize the ADC peripheral.
 *
 *  @note If the voltage reference changes at runtime, the init must be done.
 *  @note The associated GPIOs must be initialized separately since the number of GPIOs needed is unknown.
 *
 *  @param[in]  quasar_adc_cfg  The ADC peripheral configuration.
 *  @param[out] err             Pointer to store error status.
 */
static void adc_init(quasar_adc_cfg_t quasar_adc_cfg, quasar_bsp_status_t *err)
{
    *err = QUASAR_OK;

    /* Get the handle of the chosen ADC peripheral. */
    ADC_HandleTypeDef *hadc = adc_get_handle(quasar_adc_cfg.peripheral);

    QUASAR_BSP_CHECK_ERROR(hadc == NULL, err, QUASAR_ERR_ADC_HANDLE_NULL, return);

    /* Enable VDDA supply for ADC operation. */
    HAL_PWREx_EnableVddA();

    /* Configure the ADC peripheral clock. */
    configure_adc_peripheral_clock(err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);

    /* Initialize the ADC peripheral. */
    hadc->Init.ClockPrescaler = calculate_adc_clock_prescaler(err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);

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

    QUASAR_BSP_CHECK_ERROR(HAL_ADC_Init(hadc) != HAL_OK, err, QUASAR_ERR_ADC_INIT, return);

    /* Enable and set priority of ADC interrupt. */
#if defined(STM32U535xx)
    HAL_NVIC_SetPriority(ADC1_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(ADC1_IRQn);
#else
    HAL_NVIC_SetPriority(ADC1_2_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(ADC1_2_IRQn);
#endif

    /* Get a better precision by calibrating at init. If ADC is used for critical monitoring,
     * calibration should be done before each data acquisition.
     */
    QUASAR_BSP_CHECK_ERROR(HAL_ADCEx_Calibration_Start(&hadc1, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK, err,
                           QUASAR_ERR_ADC_CALIBRATION, return);
}

/** @brief Deinitialize the ADC peripheral.
 *
 *  @param[in]  adc_peripheral  The ADC peripheral to deinitialize.
 *  @param[out] err             Pointer to store error status.
 */
static void adc_deinit(quasar_adc_peripheral_t adc_peripheral, quasar_bsp_status_t *err)
{

    *err = QUASAR_OK;

    ADC_HandleTypeDef *adc_handle = adc_get_handle(adc_peripheral);
    QUASAR_BSP_CHECK_ERROR(HAL_ADC_DeInit(adc_handle) != HAL_OK, err, QUASAR_ERR_ADC_DEINIT, return);
}

/** @brief Configure the specified GPIO pin for ADC usage.
 *
 *  @param[in] gpio_port  The reference GPIO port of the HAL to be configured.
 *  @param[in] gpio_pin   The GPIO pin to configure.
 */
static void adc_configure_gpio(GPIO_TypeDef *gpio_port, quasar_gpio_pin_t gpio_pin)
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
#if !defined(STM32U535xx)
    case QUASAR_ADC_PERIPHERAL_2:
        adc_handle = &hadc2;
        break;
#endif
    default:
        /* Unimplemented peripheral. */
        break;
    }

    return adc_handle;
}

/** @brief Initialize and configure the ADC peripheral clock.
 *
 *  @param[out] err  Pointer to store error status.
 */
static void configure_adc_peripheral_clock(quasar_bsp_status_t *err)
{
    *err = QUASAR_OK;

    HAL_StatusTypeDef hal_error = HAL_OK;
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    __HAL_RCC_ADC12_CLK_ENABLE();

    /* Initializes the peripherals clock */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADCDAC;
    PeriphClkInit.AdcDacClockSelection = RCC_ADCDACCLKSOURCE_SYSCLK;

    hal_error = HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
    QUASAR_BSP_CHECK_ERROR(hal_error != HAL_OK, err, QUASAR_ERR_ADC_CLOCK_CONFIG, return);
}

/** @brief Calculate the ADC clock prescaler to be within the acceptable clock frequency.
 *
 *  From datasheet: ADC Clock must be between 5 MHz to 55 MHz.
 *  Must verify if clock is within parameter and adjust clock prescaler otherwise.
 *
 *  @param[out] err  Pointer to store error status.
 *  @return The ADC clock prescaler value.
 */
static uint32_t calculate_adc_clock_prescaler(quasar_bsp_status_t *err)
{
    *err = QUASAR_OK;

    uint32_t system_clock = 0;
    uint32_t prescaler = ADC_CLOCK_ASYNC_DIV1;

    system_clock = quasar_clock_get_system_clock_freq();

    if (system_clock > STM32U5_MAX_ADC_CLOCK_FREQ) {
        /* DIV4 was chosen as it is compatible with clock frequencies from >55 MHz to 170 MHz. */
        prescaler = ADC_CLOCK_ASYNC_DIV4;
    } else if (system_clock < STM32U5_MIN_ADC_CLOCK_FREQ) {
        /* Error: ADC must be between 5 to 55 MHz. */
        *err = QUASAR_ERR_ADC_CLOCK_FREQ;
        return prescaler;
    }

    return prescaler;
}

/** @brief Configure the ADC channel for single readings.
 *
 *  @param[in]  hadc         The selected ADC handle.
 *  @param[in]  adc_channel  The ADC channel to configure.
 *  @param[out] err          Pointer to store error status.
 */
static void configure_adc_channel(ADC_HandleTypeDef *hadc, quasar_adc_channel_t adc_channel, quasar_bsp_status_t *err)
{
    *err = QUASAR_OK;

    ADC_ChannelConfTypeDef sConfig = {0};

    /* Configure Regular Channel */
    sConfig.Channel = adc_channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_68CYCLES;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    QUASAR_BSP_CHECK_ERROR(HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK, err, QUASAR_ERR_ADC_CHANNEL_CONFIG, return);
}

/** @brief Unselect the ADC channel.
 *
 *  @param[in]  hadc         The selected ADC handle.
 *  @param[in]  adc_channel  The ADC channel to unselect.
 *  @param[out] err          Pointer to store error status.
 */
static void unselect_adc_channel(ADC_HandleTypeDef *hadc, quasar_adc_channel_t adc_channel, quasar_bsp_status_t *err)
{
    *err = QUASAR_OK;

    ADC_ChannelConfTypeDef sConfig = {0};

    /* Configure Regular Channel. */
    sConfig.Channel = adc_channel;
    sConfig.Rank = 0;
    QUASAR_BSP_CHECK_ERROR(HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK, err, QUASAR_ERR_ADC_CHANNEL_CONFIG, return);
}

/* ST HAL FUNCTIONS IMPLEMENTATIONS ********************************************/
/** @brief Implementation of weak alias for the ADC interrupt handler.
 */
#if defined(STM32U535xx)
void ADC1_IRQHandler(void)
{
    HAL_ADC_IRQHandler(&hadc1);
}
#else
void ADC1_2_IRQHandler(void)
{
    HAL_ADC_IRQHandler(&hadc1);
    HAL_ADC_IRQHandler(&hadc2);
}
#endif

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
#if !defined(STM32U535xx)
    if (adc2_irq_callback != NULL) {
        adc2_irq_callback();
    }
#endif
}
