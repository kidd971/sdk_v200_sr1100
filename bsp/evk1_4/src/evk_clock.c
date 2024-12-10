/** @file  evk_clock.c
 *  @brief This module controls clock related features.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "evk_clock.h"
#include "evk_it.h"
#include "evk_usb.h"
#include "evk_radio.h"
#include "stm32g4xx_ll_pwr.h"

/* CONSTANT *******************************************************************/
#define HSI_FREQ       16000000.0
#define FULLSPEED_FREQ 169984000.0

#define RATIO_FULLSPEED_HSI (FULLSPEED_FREQ / HSI_FREQ)
#define RATIO_HSI_FULLSPEED (HSI_FREQ / FULLSPEED_FREQ)

#define HSI_SYSTEM_FREQ_SYSTICK_ARR       42495
#define FULLSPEED_SYSTEM_FREQ_SYSTICK_ARR 3999

#define LSE_FREQ       32768.0
#define RATIO_TO_MILLI 1000
#define RTC_FACTOR (((1 / LSE_FREQ)) * RATIO_TO_MILLI * 16)

#define PRESCALER_FREE_RUNNING_TIMER_FULLSPEED ((FULLSPEED_FREQ / 8000.0) - 1)
#define PRESCALER_FREE_RUNNING_TIMER_HSI       ((HSI_FREQ / 8000.0) - 1)

/* EXTERNS ********************************************************************/
extern TIM_HandleTypeDef htim5;
extern TIM_HandleTypeDef htim6;

/* PRIVATE GLOBALS ************************************************************/
extern TIM_HandleTypeDef htim2;
evk_clk_freq_t clk_freq;
RTC_HandleTypeDef hrtc;
static bool is_no_max_speed;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void config_clock_freq(RCC_OscInitTypeDef *RCC_OscInitStruct,
                              RCC_ClkInitTypeDef *RCC_ClkInitStruct,
                              evk_clk_freq_t evk_clk_freq);
static void set_oscillator_pll_169_98mhz(RCC_OscInitTypeDef *RCC_OscInitStruct);
static void set_oscillator_pll_163_84mhz(RCC_OscInitTypeDef *RCC_OscInitStruct);
static void set_oscillator_pll_81_92mhz(RCC_OscInitTypeDef *RCC_OscInitStruct);
static void set_oscillator_pll_40_96mhz(RCC_OscInitTypeDef *RCC_OscInitStruct);
static void set_oscillator_pll_25_20mhz(RCC_OscInitTypeDef *RCC_OscInitStruct);
static void set_oscillator_pll_20_48mhz(RCC_OscInitTypeDef *RCC_OscInitStruct);
static void set_system_clock_pll(RCC_ClkInitTypeDef *RCC_ClkInitStruct);
static void system_clock_init(RCC_OscInitTypeDef *RCC_OscInitStruct,
                              RCC_ClkInitTypeDef *RCC_ClkInitStruct,
                              uint32_t flash_latency);
static void update_systick(void);
static void pwm_xtal_clk_config(void);
static bool is_pwm_freq_valid(uint32_t system_clock_freq);
static uint16_t calculate_xtal_clk_pwm_period(uint32_t system_clock_freq);
static void pwm_timer_xtal_clk_init(TIM_MasterConfigTypeDef *master_config,
                                    TIM_OC_InitTypeDef *config_oc,
                                    uint16_t pwm_period);
static void pwm_timer_xtal_clk_config(TIM_MasterConfigTypeDef *master_config,
                                      TIM_OC_InitTypeDef *config_oc);
static uint32_t get_voltage_scale(evk_clk_freq_t evk_clk_freq);
static uint32_t get_flash_latency(evk_clk_freq_t evk_clk_freq, uint32_t voltage_scale);
static void gpio_pll_sel_init(void);
static void gpio_xtal_clk_init(void);

/* PUBLIC FUNCTIONS ***********************************************************/
void evk_clock_ext_osc_init(void)
{
    gpio_pll_sel_init();
}

void evk_clock_xtal_init(void)
{
    pwm_xtal_clk_config();
}

void evk_clock_usb_init(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
    RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
    PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        Error_Handler();
    }
}

void evk_clock_rtc_for_wakeup_init(void)
{
    uint32_t tmpregister  = 0;

    /* Enable write access to Backup domain */
    __HAL_RCC_PWR_CLK_ENABLE();
    SET_BIT(PWR->CR1, PWR_CR1_DBP);
    while ((PWR->CR1 & PWR_CR1_DBP) == 0U);

    /* Initialize LSE oscillator */
    __HAL_RCC_LSE_CONFIG(RCC_LSE_ON);
    while (READ_BIT(RCC->BDCR, RCC_BDCR_LSERDY) == 0U);

    /* Clear backup registers */
    tmpregister = READ_BIT(RCC->BDCR, ~(RCC_BDCR_RTCSEL));
    __HAL_RCC_BACKUPRESET_FORCE();
    __HAL_RCC_BACKUPRESET_RELEASE();
    /* Restore the Content of BDCR register */
    RCC->BDCR = tmpregister;

    /*  Enable RTC and its clock */
    __HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSE);
    __HAL_RCC_RTCAPB_CLK_ENABLE();
    __HAL_RCC_RTC_ENABLE();

    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv = 127;
    hrtc.Init.SynchPrediv = 255;
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    hrtc.Init.OutPutPullUp = RTC_OUTPUT_PULLUP_NONE;
    if (HAL_RTC_Init(&hrtc) != HAL_OK) {
        Error_Handler();
    }

    NVIC_ClearPendingIRQ(RTC_WKUP_IRQn);
    HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);
}

void evk_clock_all_gpio_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
}

void evk_clock_ext_osc_deinit(void)
{
    evk_disable_ext_osc_clk();
}

void evk_clock_xtal_deinit(void)
{
    evk_disable_xtal_clk();
    __HAL_RCC_TIM5_CLK_DISABLE();
}

void evk_set_system_clock(evk_clk_freq_t evk_clk_freq)
{
    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_ClkInitTypeDef RCC_ClkInitStruct;

    uint32_t voltage_scale = 0;
    uint32_t flash_latency = 0;

    /* Used by the USB stack */
    clk_freq = evk_clk_freq;

    /* Disable clocks */
    HAL_RCC_DeInit();

    config_clock_freq(&RCC_OscInitStruct, &RCC_ClkInitStruct, evk_clk_freq);

    voltage_scale = get_voltage_scale(evk_clk_freq);
    HAL_PWREx_ControlVoltageScaling(voltage_scale);

    flash_latency = get_flash_latency(evk_clk_freq, voltage_scale);

    system_clock_init(&RCC_OscInitStruct, &RCC_ClkInitStruct, flash_latency);

    update_systick();
}

uint32_t evk_get_system_clock_freq(void)
{
    return SystemCoreClock;
}

void evk_usb_clock_deinit(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
    RCC_OscInitStruct.HSI48State = RCC_HSI48_OFF;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }
}

void evk_enable_ext_osc_clk(void)
{
    HAL_GPIO_WritePin(PLL_SEL_PORT, PLL_SEL_PIN, GPIO_PIN_SET);
}

void evk_disable_ext_osc_clk(void)
{
    HAL_GPIO_WritePin(PLL_SEL_PORT, PLL_SEL_PIN, GPIO_PIN_RESET);
}

void evk_enable_xtal_clk(void)
{
    if (HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_3) != HAL_OK) {
        Error_Handler();
    }
}

void evk_disable_xtal_clk(void)
{
    if (HAL_TIM_PWM_Stop(&htim5, TIM_CHANNEL_3) != HAL_OK) {
        Error_Handler();
    }
}

void SystemClock_Config(void)
{
    evk_set_system_clock(clk_freq);
    evk_usb_clock_init();
}

void evk_clock_set_pll_170Mhz_as_sysclk(void)
{
    evk_enter_critical();
    if (is_no_max_speed & ((RADIO_CS_PORT->ODR & RADIO_CS_PIN) != 0x00U)) {
        MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_SYSCLKSOURCE_PLLCLK);
        __HAL_TIM_DISABLE(&htim6);
        TIM6->PSC = PRESCALER_FREE_RUNNING_TIMER_FULLSPEED;
        TIM6->EGR = TIM_EGR_UG;
        __HAL_TIM_ENABLE(&htim6);
        TIM8->ARR = HSI_SYSTEM_FREQ_SYSTICK_ARR;
        TIM8->CNT = TIM8->CNT * RATIO_FULLSPEED_HSI;
        SystemCoreClock = CLK_169_98MHZ;
        is_no_max_speed = false;
    }
    evk_exit_critical();
}

void evk_clock_set_hsi_as_sysclk(void)
{
    evk_enter_critical();
    if (!is_no_max_speed & ((RADIO_CS_PORT->ODR & RADIO_CS_PIN) != 0x00U)) {
        TIM8->ARR = FULLSPEED_SYSTEM_FREQ_SYSTICK_ARR;
        TIM8->CNT = TIM8->CNT * RATIO_HSI_FULLSPEED;
        __HAL_TIM_DISABLE(&htim6);
        TIM6->PSC = PRESCALER_FREE_RUNNING_TIMER_HSI;
        TIM6->EGR = TIM_EGR_UG;
        __HAL_TIM_ENABLE(&htim6);
        MODIFY_REG(RCC->CFGR, RCC_CFGR_SW, RCC_SYSCLKSOURCE_HSI);
        SystemCoreClock = CLK_16MHZ;
        is_no_max_speed = true;
    }
    evk_exit_critical();
}

void evk_clock_set_hsi_as_sysclk_protected(void)
{
    if (!evk_radio_is_spi_busy()) {
        evk_clock_set_hsi_as_sysclk();
    }
    __WFI();
}

void evk_clock_start_rtc_for_wakeup(uint32_t ms)
{
    if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, ms / RTC_FACTOR, RTC_WAKEUPCLOCK_RTCCLK_DIV16) != HAL_OK) {
        Error_Handler();
    }
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initializes the Global MSP.
 */
void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    /* Disable the internal Pull-Up in Dead Battery pins of UCPD peripheral */
    LL_PWR_DisableDeadBatteryPD();
}

/** @brief Select clock initializations depending on the frequency.
 *
 *  @param[in] RCC_OscInitStruct  RCC Oscillator configuration structure definition.
 *  @param[in] RCC_ClkInitStruct  RCC System/AHB/APB clock config structure definition.
 *  @param[in] evk_clk_freq       Clock frequency selection.
 */
static void config_clock_freq(RCC_OscInitTypeDef *RCC_OscInitStruct,
                              RCC_ClkInitTypeDef *RCC_ClkInitStruct,
                              evk_clk_freq_t evk_clk_freq)
{
    switch (evk_clk_freq) {
    case CLK_169_98MHZ:
        set_oscillator_pll_169_98mhz(RCC_OscInitStruct);
        set_system_clock_pll(RCC_ClkInitStruct);
        break;
    case CLK_163_84MHZ:
        set_oscillator_pll_163_84mhz(RCC_OscInitStruct);
        set_system_clock_pll(RCC_ClkInitStruct);
        break;
    case CLK_81_92MHZ:
        set_oscillator_pll_81_92mhz(RCC_OscInitStruct);
        set_system_clock_pll(RCC_ClkInitStruct);
        break;
    case CLK_40_96MHZ:
        set_oscillator_pll_40_96mhz(RCC_OscInitStruct);
        set_system_clock_pll(RCC_ClkInitStruct);
        break;
    case CLK_25_20MHZ:
        set_oscillator_pll_25_20mhz(RCC_OscInitStruct);
        set_system_clock_pll(RCC_ClkInitStruct);
        break;
    case CLK_20_48MHZ:
        set_oscillator_pll_20_48mhz(RCC_OscInitStruct);
        set_system_clock_pll(RCC_ClkInitStruct);
        break;
    case CLK_16MHZ:
        set_system_clock_pll(RCC_ClkInitStruct);
        RCC_ClkInitStruct->SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
        break;
    default:
        Error_Handler();
        break;
    }
}

/** @brief Initialize the PLL clock to 169.984MHz.
 *
 *  @param[in] RCC_OscInitStruct  RCC Oscillator configuration structure definition.
 */
static void set_oscillator_pll_169_98mhz(RCC_OscInitTypeDef *RCC_OscInitStruct)
{
    RCC_OscInitStruct->OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct->HSEState = RCC_HSE_ON;
    RCC_OscInitStruct->PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct->PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct->PLL.PLLM = RCC_PLLM_DIV3;
    RCC_OscInitStruct->PLL.PLLN = 83;
    RCC_OscInitStruct->PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct->PLL.PLLQ = RCC_PLLQ_DIV6;
    RCC_OscInitStruct->PLL.PLLR = RCC_PLLR_DIV2;
}

/** @brief Initialize the PLL clock to 163.84MHz.
 *
 *  @param[in] RCC_OscInitStruct  RCC Oscillator configuration structure definition.
 */
static void set_oscillator_pll_163_84mhz(RCC_OscInitTypeDef *RCC_OscInitStruct)
{
    RCC_OscInitStruct->OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct->HSEState = RCC_HSE_ON;
    RCC_OscInitStruct->PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct->PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct->PLL.PLLM = RCC_PLLM_DIV3;
    RCC_OscInitStruct->PLL.PLLN = 80;
    RCC_OscInitStruct->PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct->PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct->PLL.PLLR = RCC_PLLR_DIV2;
}

/** @brief Initialize the PLL clock to 81.92MHz.
 *
 *  @param[in] RCC_OscInitStruct  RCC Oscillator configuration structure definition.
 */
static void set_oscillator_pll_81_92mhz(RCC_OscInitTypeDef *RCC_OscInitStruct)
{
    RCC_OscInitStruct->OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct->HSEState = RCC_HSE_ON;
    RCC_OscInitStruct->PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct->PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct->PLL.PLLM = RCC_PLLM_DIV3;
    RCC_OscInitStruct->PLL.PLLN = 80;
    RCC_OscInitStruct->PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct->PLL.PLLQ = RCC_PLLQ_DIV4;
    RCC_OscInitStruct->PLL.PLLR = RCC_PLLR_DIV4;
}

/** @brief Initialize the PLL clock to 40.96MHz.
 *
 *  @param[in] RCC_OscInitStruct  RCC Oscillator configuration structure definition.
 */
static void set_oscillator_pll_40_96mhz(RCC_OscInitTypeDef *RCC_OscInitStruct)
{
    RCC_OscInitStruct->OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct->HSEState = RCC_HSE_ON;
    RCC_OscInitStruct->PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct->PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct->PLL.PLLM = RCC_PLLM_DIV3;
    RCC_OscInitStruct->PLL.PLLN = 80;
    RCC_OscInitStruct->PLL.PLLP = RCC_PLLP_DIV8;
    RCC_OscInitStruct->PLL.PLLQ = RCC_PLLQ_DIV8;
    RCC_OscInitStruct->PLL.PLLR = RCC_PLLR_DIV8;
}

/** @brief Initialize the PLL clock to 25.20MHz.
 *
 *  @param[in] RCC_OscInitStruct  RCC Oscillator configuration structure definition.
 */
static void set_oscillator_pll_25_20mhz(RCC_OscInitTypeDef *RCC_OscInitStruct)
{
    RCC_OscInitStruct->OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct->HSEState = RCC_HSE_ON;
    RCC_OscInitStruct->PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct->PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct->PLL.PLLM = RCC_PLLM_DIV3;
    RCC_OscInitStruct->PLL.PLLN = 50;
    RCC_OscInitStruct->PLL.PLLP = RCC_PLLP_DIV8;
    RCC_OscInitStruct->PLL.PLLQ = RCC_PLLQ_DIV8;
    RCC_OscInitStruct->PLL.PLLR = RCC_PLLR_DIV8;
}

/** @brief Initialize the PLL clock to 20.48MHz.
 *
 *  @param[in] RCC_OscInitStruct  RCC Oscillator configuration structure definition.
 */
static void set_oscillator_pll_20_48mhz(RCC_OscInitTypeDef *RCC_OscInitStruct)
{
    RCC_OscInitStruct->OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct->HSEState = RCC_HSE_ON;
    RCC_OscInitStruct->PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct->PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct->PLL.PLLM = RCC_PLLM_DIV3;
    RCC_OscInitStruct->PLL.PLLN = 40;
    RCC_OscInitStruct->PLL.PLLP = RCC_PLLP_DIV8;
    RCC_OscInitStruct->PLL.PLLQ = RCC_PLLQ_DIV8;
    RCC_OscInitStruct->PLL.PLLR = RCC_PLLR_DIV8;
}

/** @brief Initializes the CPU, AHB and APB busses clocks.
 *
 *  @param[in] RCC_ClkInitStruct  RCC System/AHB/APB clock config structure definition.
 */
static void set_system_clock_pll(RCC_ClkInitTypeDef *RCC_ClkInitStruct)
{
    RCC_ClkInitStruct->ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct->SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct->AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct->APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct->APB2CLKDivider = RCC_HCLK_DIV1;
}

static void system_clock_init(RCC_OscInitTypeDef *RCC_OscInitStruct,
                              RCC_ClkInitTypeDef *RCC_ClkInitStruct,
                              uint32_t flash_latency)
{
    if (HAL_RCC_OscConfig(RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_RCC_ClockConfig(RCC_ClkInitStruct, flash_latency) != HAL_OK) {
        Error_Handler();
    }
}

/** @brief Adjust the systick with the clock frequency.
 */
static void update_systick(void)
{
    HAL_SYSTICK_Config(evk_get_system_clock_freq() / 1000);
    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);
}

/** @brief Initializes the PWM timer for the XTAL CLK.
 *
 *  XTAL clock frequency is 32.768kHz.
 */
static void pwm_xtal_clk_config(void)
{
    TIM_MasterConfigTypeDef master_config = {0};
    TIM_OC_InitTypeDef config_oc = {0};
    uint32_t system_clock_freq = 0;
    uint16_t pwm_period = 0;

    system_clock_freq = evk_get_system_clock_freq();

    if (is_pwm_freq_valid(system_clock_freq)) {
        pwm_period = calculate_xtal_clk_pwm_period(system_clock_freq);

        /* Start TIM5 clock */
        __HAL_RCC_TIM5_CLK_ENABLE();

        /* Initialize the PWM timer */
        pwm_timer_xtal_clk_init(&master_config, &config_oc, pwm_period);

        /* Configure the PWM timer */
        pwm_timer_xtal_clk_config(&master_config, &config_oc);

        /* Initialize XTAL_CLK GPIO */
        gpio_xtal_clk_init();
    } else {
        /* Not a valid frequency for XTAL CLK PWM */
        Error_Handler();
    }
}

/** @brief Verify if the system frequency is valid for PWM generation.
 *
 *  The system frequency must be a multiple of 32768.
 *
 *  @return True if system clock is a multiple of 32768, otherwise returns false.
 */
static bool is_pwm_freq_valid(uint32_t system_clock_freq)
{
    bool is_valid;

    if ((system_clock_freq % XTAL_CLK_FREQ) == 0) {
        is_valid = true;
    } else {
        is_valid = false;
    }

    return is_valid;
}

/** @brief Calculate the PWM period for the XTAL CLK generation.
 *
 *  @return The XTAL CLK PWM's period.
 */
static uint16_t calculate_xtal_clk_pwm_period(uint32_t system_clock_freq)
{
    return (system_clock_freq / XTAL_CLK_FREQ);
}

/** @brief Fill all the parameters to generate the XTAL CLK PWM.
 *
 *  @param[in] master_config  TIM Master configuration Structure definition.
 *  @param[in] config_oc      TIM Output Compare Configuration Structure definition.
 *  @param[in] pwm_period     The period at which the PWM is running.
 */
static void pwm_timer_xtal_clk_init(TIM_MasterConfigTypeDef *master_config,
                                    TIM_OC_InitTypeDef *config_oc,
                                    uint16_t pwm_period)
{
    htim5.Instance = TIM5;
    htim5.Init.Prescaler = 0;
    htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim5.Init.Period = pwm_period - 1;
    htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    master_config->MasterOutputTrigger = TIM_TRGO_RESET;
    master_config->MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    config_oc->OCMode = TIM_OCMODE_PWM1;
    config_oc->Pulse = (pwm_period / 2) - 1;
    config_oc->OCPolarity = TIM_OCPOLARITY_HIGH;
    config_oc->OCFastMode = TIM_OCFAST_DISABLE;
}

/** @brief Configure XTAL CLK PWM.
 *
 *  @param[in] master_config  TIM Master configuration Structure definition.
 *  @param[in] config_oc      TIM Output Compare Configuration Structure definition.
 */
static void pwm_timer_xtal_clk_config(TIM_MasterConfigTypeDef *master_config, TIM_OC_InitTypeDef *config_oc)
{
        if (HAL_TIM_PWM_Init(&htim5) != HAL_OK) {
            Error_Handler();
        }

        if (HAL_TIMEx_MasterConfigSynchronization(&htim5, master_config) != HAL_OK) {
            Error_Handler();
        }

        if (HAL_TIM_PWM_ConfigChannel(&htim5, config_oc, TIM_CHANNEL_3) != HAL_OK) {
            Error_Handler();
        }
}

/** @brief Get the voltage scale depending on the frequency.
 *
 *  @param[in] evk_clk_freq  Current board frequency.
 *  @return Voltage scale.
 */
static uint32_t get_voltage_scale(evk_clk_freq_t evk_clk_freq)
{
    uint32_t voltage_scale = 0;

    if (evk_clk_freq < RANGE2_LOWPOW_LIMIT) {
        voltage_scale = PWR_REGULATOR_VOLTAGE_SCALE2;
    } else if (evk_clk_freq <= RANGE1_BOOST1_LIMIT) {
        voltage_scale = PWR_REGULATOR_VOLTAGE_SCALE1;
    } else if (evk_clk_freq <= RANGE1_BOOST0_LIMIT) {
        voltage_scale = PWR_REGULATOR_VOLTAGE_SCALE1_BOOST;
    } else {
        /* Unsupported frequency */
        Error_Handler();
    }

    return voltage_scale;
}

/** @brief Get the flash latency depending on the frequency and voltage scale.
 *
 *  The voltage scale must be set before calling this function.
 *
 *  @param[in] evk_clk_freq   Current board frequency.
 *  @param[in] voltage_scale  Current voltage scale.
 *  @return Flash latency.
 */
static uint32_t get_flash_latency(evk_clk_freq_t evk_clk_freq, uint32_t voltage_scale)
{
    uint32_t flash_latency = 0;

    if (voltage_scale == PWR_REGULATOR_VOLTAGE_SCALE1_BOOST) {
        if (evk_clk_freq <= RANGE1_BOOST_0WS_FREQ) {
            flash_latency = FLASH_ACR_LATENCY_0WS;
        } else if (evk_clk_freq <= RANGE1_BOOST_1WS_FREQ) {
            flash_latency = FLASH_ACR_LATENCY_1WS;
        } else if (evk_clk_freq <= RANGE1_BOOST_2WS_FREQ) {
            flash_latency = FLASH_ACR_LATENCY_2WS;
        } else if (evk_clk_freq <= RANGE1_BOOST_3WS_FREQ) {
            flash_latency = FLASH_ACR_LATENCY_3WS;
        } else if (evk_clk_freq <= RANGE1_BOOST_4WS_FREQ) {
            flash_latency = FLASH_ACR_LATENCY_4WS;
        } else {
            /* Unsupported frequency */
            Error_Handler();
        }
    } else if (voltage_scale == PWR_REGULATOR_VOLTAGE_SCALE1) {
        if (evk_clk_freq <= RANGE1_0WS_FREQ) {
            flash_latency = FLASH_ACR_LATENCY_0WS;
        } else if (evk_clk_freq <= RANGE1_1WS_FREQ) {
            flash_latency = FLASH_ACR_LATENCY_1WS;
        } else if (evk_clk_freq <= RANGE1_2WS_FREQ) {
            flash_latency = FLASH_ACR_LATENCY_2WS;
        } else if (evk_clk_freq <= RANGE1_3WS_FREQ) {
            flash_latency = FLASH_ACR_LATENCY_3WS;
        } else if (evk_clk_freq <= RANGE1_4WS_FREQ) {
            flash_latency = FLASH_ACR_LATENCY_4WS;
        } else {
            /* Unsupported frequency */
            Error_Handler();
        }
    } else if (voltage_scale == PWR_REGULATOR_VOLTAGE_SCALE2) {
        if (evk_clk_freq <= RANGE2_0WS_FREQ) {
            flash_latency = FLASH_ACR_LATENCY_0WS;
        } else if (evk_clk_freq <= RANGE2_1WS_FREQ) {
            flash_latency = FLASH_ACR_LATENCY_1WS;
        } else if (evk_clk_freq <= RANGE2_2WS_FREQ) {
            flash_latency = FLASH_ACR_LATENCY_2WS;
        } else {
            /* Unsupported frequency */
            Error_Handler();
        }
    } else {
        /* voltage_scale is not defined */
        Error_Handler();
    }
    return flash_latency;
}

/** @brief Initialize PLL_SEL pin to control the 20.48MHz oscillator.
 *
 *  The oscillator is used as an external PLL for the SR radio.
 */
static void gpio_pll_sel_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(PLL_SEL_PORT, PLL_SEL_PIN, GPIO_PIN_RESET);

    /* Configure GPIO pins */
    GPIO_InitStruct.Pin   = PLL_SEL_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    HAL_GPIO_Init(PLL_SEL_PORT, &GPIO_InitStruct);
}

/** @brief Initialize XTAL_CLK pin to use it as a PWM.
 */
static void gpio_xtal_clk_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = XTAL_CLK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF2_TIM5;
    HAL_GPIO_Init(XTAL_CLK_PORT, &GPIO_InitStruct);
}

/** @brief This function configures the TIM2 as a time base source.
 *         The time source is configured to have 1 ms time base with a dedicated
 *         Tick interrupt priority.
 *  @note This function is called automatically at the beginning of program after
 *        reset by HAL_Init() or at any time when clock is configured, by HAL_RCC_ClockConfig().
 *  @param[in] TickPriority  Tick interrupt priority.
 *  @retval HAL status.
 */
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    RCC_ClkInitTypeDef    clkconfig;
    uint32_t              uwTimclock = 0;
    uint32_t              uwPrescalerValue = 0;
    uint32_t              pFLatency;

    /* Configure the TIM2 IRQ priority */
    HAL_NVIC_SetPriority(TIM2_IRQn, TickPriority, 10);
    uwTickPrio = TickPriority;

    /* Enable the TIM2 global Interrupt */
    HAL_NVIC_EnableIRQ(TIM2_IRQn);

    /* Enable TIM2 clock */
    __HAL_RCC_TIM2_CLK_ENABLE();

    /* Get clock configuration */
    HAL_RCC_GetClockConfig(&clkconfig, &pFLatency);

    /* Compute TIM2 clock */
    uwTimclock = HAL_RCC_GetPCLK1Freq();

    /* Compute the prescaler value to have TIM2 counter clock equal to 1MHz */
    uwPrescalerValue = (uint32_t)((uwTimclock / 1000000) - 1);

    /* Initialize TIM2 */
    htim2.Instance = TIM2;

    /* Initialize TIMx peripheral as follow:
     * + Period = [(TIM2CLK/1000) - 1]. to have a (1/1000) s time base.
     * + Prescaler = (uwTimclock/1000000 - 1) to have a 1MHz counter clock.
     * + ClockDivision = 0
     * + Counter direction = Up
     */
    htim2.Init.Period = (1000000 / 1000) - 1;
    htim2.Init.Prescaler = uwPrescalerValue;
    htim2.Init.ClockDivision = 0;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    if (HAL_TIM_Base_Init(&htim2) == HAL_OK) {
        /* Start the TIM time Base generation in interrupt mode */
        return HAL_TIM_Base_Start_IT(&htim2);
    }

    /* Return function status */
    return HAL_ERROR;
}

/** @brief Suspend Tick increment.
 *
 *  @note Disable the tick increment by disabling TIM2 update interrupt.
 */
void HAL_SuspendTick(void)
{
    /* Disable TIM2 update Interrupt */
    __HAL_TIM_DISABLE_IT(&htim2, TIM_IT_UPDATE);
}

/** @brief Resume Tick increment.
 *
 *  @note Enable the tick increment by enabling TIM2 update interrupt.
 */
void HAL_ResumeTick(void)
{
    /* Enable TIM2 Update interrupt */
    __HAL_TIM_ENABLE_IT(&htim2, TIM_IT_UPDATE);
}
