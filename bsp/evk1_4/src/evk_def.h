/** @file  evk_def.h
 *  @brief This defines MCU pinout and other control definitions.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef EVK_DEF_H_
#define EVK_DEF_H_

/* INCLUDES *******************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "stm32g4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/* Buttons */
#define BTN0_PORT            GPIOB
#define BTN0_PIN             GPIO_PIN_8

#define BTN1_PORT            GPIOC
#define BTN1_PIN             GPIO_PIN_3

/* LEDs */
#define LED0_PORT            GPIOC
#define LED0_PIN             GPIO_PIN_5

#define LED1_PORT            GPIOB
#define LED1_PIN             GPIO_PIN_10

#define LED2_PORT            GPIOC
#define LED2_PIN             GPIO_PIN_6

/* External Clock Select */
#define PLL_SEL_PORT         GPIOA
#define PLL_SEL_PIN          GPIO_PIN_9
#define XTAL_CLK_PORT        GPIOA
#define XTAL_CLK_PIN         GPIO_PIN_2

#define XTAL_CLK_FREQ        32768

/* USB */
#define USB_DETECT_PORT      GPIOA
#define USB_DETECT_PIN       GPIO_PIN_0

/* Power Management */
#define VOLTAGE_SEL_PORT     GPIOD
#define VOLTAGE_SEL_PIN      GPIO_PIN_2

/* Radio */
#define RADIO_SHUTDOWN_PORT  GPIOB
#define RADIO_SHUTDOWN_PIN   GPIO_PIN_1

#define RADIO_RESET_PORT     GPIOB
#define RADIO_RESET_PIN      GPIO_PIN_0

#define RADIO_IRQ_PORT       GPIOB
#define RADIO_IRQ_PIN        GPIO_PIN_2

#define RADIO_SCK_PORT       GPIOB
#define RADIO_SCK_PIN        GPIO_PIN_13
#define RADIO_SCK_AF         GPIO_AF5_SPI2

#define RADIO_MOSI_PORT      GPIOB
#define RADIO_MOSI_PIN       GPIO_PIN_15
#define RADIO_MOSI_AF        GPIO_AF5_SPI2

#define RADIO_MISO_PORT      GPIOB
#define RADIO_MISO_PIN       GPIO_PIN_14
#define RADIO_MISO_AF        GPIO_AF5_SPI2

#define RADIO_CS_PORT        GPIOB
#define RADIO_CS_PIN         GPIO_PIN_12

#define RADIO_DEBUG_EN_PORT  GPIOB
#define RADIO_DEBUG_EN_PIN   GPIO_PIN_9

/* Expansion IO */
#define EXP_SCK_PORT     GPIOA
#define EXP_SCK_PIN      GPIO_PIN_5
#define EXP_SCK_AF       GPIO_AF5_SPI1

#define EXP_MOSI_PORT    GPIOA
#define EXP_MOSI_PIN     GPIO_PIN_7
#define EXP_MOSI_AF      GPIO_AF5_SPI1

#define EXP_MISO_PORT    GPIOA
#define EXP_MISO_PIN     GPIO_PIN_6
#define EXP_MISO_AF      GPIO_AF5_SPI1

#define EXP_CS_PORT      GPIOA
#define EXP_CS_PIN       GPIO_PIN_4

#define EXP_PC7_PORT     GPIOC
#define EXP_PC7_PIN      GPIO_PIN_7

#define EXP_PC8_PORT     GPIOC
#define EXP_PC8_PIN      GPIO_PIN_8

#define EXP_PC10_PORT    GPIOC
#define EXP_PC10_PIN     GPIO_PIN_10

#define EXP_PC11_PORT    GPIOC
#define EXP_PC11_PIN     GPIO_PIN_11

#define EXP_PA3_PORT     GPIOA
#define EXP_PA3_PIN      GPIO_PIN_3

#define EXP_UART_RX_PORT GPIOC
#define EXP_UART_RX_PIN  GPIO_PIN_0

#define EXP_UART_TX_PORT GPIOC
#define EXP_UART_TX_PIN  GPIO_PIN_1

#define EXP_I2C_SCL_PORT GPIOA
#define EXP_I2C_SCL_PIN  GPIO_PIN_15

#define EXP_I2C_SDA_PORT GPIOB
#define EXP_I2C_SDA_PIN  GPIO_PIN_7

/* Audio module */
#define AUDIO_I2C_SDA_PORT    GPIOB
#define AUDIO_I2C_SDA_PIN     GPIO_PIN_7
#define AUDIO_I2C_SCL_PORT    GPIOA
#define AUDIO_I2C_SCL_PIN     GPIO_PIN_15
#define AUDIO_I2C_TIMING      0x30A0A7FB

#define AUDIO_SAI_IN_CLK_PORT GPIOC
#define AUDIO_SAI_IN_CLK_PIN  GPIO_PIN_9
#define AUDIO_MCO_CLK_PORT    GPIOA
#define AUDIO_MCO_CLK_PIN     GPIO_PIN_8
#define AUDIO_SAI_SD_A_PORT   GPIOA
#define AUDIO_SAI_SD_A_PIN    GPIO_PIN_10
#define AUDIO_SAI_SCK_PORT    GPIOB
#define AUDIO_SAI_SCK_PIN     GPIO_PIN_3
#define AUDIO_SAI_MCLK_PORT   GPIOB
#define AUDIO_SAI_MCLK_PIN    GPIO_PIN_4
#define AUDIO_SAI_SD_B_PORT   GPIOB
#define AUDIO_SAI_SD_B_PIN    GPIO_PIN_5
#define AUDIO_SAI_FS_PORT     GPIOB
#define AUDIO_SAI_FS_PIN      GPIO_PIN_6

/* Timeout values */
#define AUDIO_I2C_TIMEOUT_MS 200
#define EXP_SPI_TIMEOUT_MS   10
#define RADIO_SPI_TIMEOUT_MS 10
#define LOG_UART_TIMEOUT_MS  10

 /* Priorities */
#define PRIO_APP_TIMER_HIGH_IRQ   1
#define PRIO_AUDIO_SAI_DMA_TX_IRQ 1
#define PRIO_AUDIO_SAI_DMA_RX_IRQ 1
#define PRIO_SYSTICK_IRQ          1
#define PRIO_UART4_IRQ            1
#define PRIO_RADIO_IRQ            2
#define PRIO_RADIO_DMA_RX_CPLT    2
#define PRIO_RADIO_DMA_TX_CPLT    2
#define PRIO_USB_LP_IRQ           8
#define PRIO_LPUART_IRQ           11
#define PRIO_LPUART_DMA_TX_CPLT   11
#define PRIO_LPUART_DMA_RX_CPLT   11
#define PRIO_USB_DET_IRQ          12
#define PRIO_PEND_SV_IRQ          13
#define PRIO_APP_TIMER_IRQ        14
#define PRIO_BASIC_MS_TIMER_IRQ   15
#define PRIO_TIMER_NO_IRQ         0xFFFFFFFF

/* DMA Controller channel */
#define DMA1_CHANNEL_RADIO_TX   DMA1_Channel1
#define DMA1_CHANNEL_RADIO_RX   DMA1_Channel2
#define DMA1_CHANNEL_LPUART_TX  DMA1_Channel5
#define DMA1_CHANNEL_LPUART_RX  DMA1_Channel6
#define DMA2_CHANNEL_SAI_RX     DMA2_Channel5
#define DMA2_CHANNEL_SAI_TX     DMA2_Channel6

/* Nested vector interrupt controller ID */
#define NVIC_PENDSV_IRQ        PendSV_IRQn
#define NVIC_TIMER_20          TIM20_UP_IRQn
#define NVIC_SYSTICK_TIMER     TIM8_UP_IRQn
#define NVIC_USB_LP_IRQ        USB_LP_IRQn
#define NVIC_USB_DET_IRQ       EXTI0_IRQn
#define NVIC_RADIO_IRQ         EXTI2_IRQn
#define NVIC_RADIO_DMA_TX_CPLT DMA1_Channel1_IRQn
#define NVIC_RADIO_DMA_RX_CPLT DMA1_Channel2_IRQn
#define NVIC_UART4_IRQ         UART4_IRQn
#define NVIC_LPUART_IRQ        LPUART1_IRQn
#define NVIC_LPUART_TX_CPLT    DMA1_Channel5_IRQn
#define NVIC_LPUART_RX_CPLT    DMA1_Channel6_IRQn
#define NVIC_SAI_DMA_RX_CPLT   DMA2_Channel5_IRQn
#define NVIC_SAI_DMA_TX_CPLT   DMA2_Channel6_IRQn

/* Wait latency clock frequency limit*/
#define RANGE1_BOOST_0WS_FREQ 34000000
#define RANGE1_BOOST_1WS_FREQ 68000000
#define RANGE1_BOOST_2WS_FREQ 102000000
#define RANGE1_BOOST_3WS_FREQ 136000000
#define RANGE1_BOOST_4WS_FREQ 170000000

#define RANGE1_0WS_FREQ 30000000
#define RANGE1_1WS_FREQ 60000000
#define RANGE1_2WS_FREQ 90000000
#define RANGE1_3WS_FREQ 120000000
#define RANGE1_4WS_FREQ 150000000

#define RANGE2_0WS_FREQ 12000000
#define RANGE2_1WS_FREQ 24000000
#define RANGE2_2WS_FREQ 26000000

/* Voltage scale clock frequency limit */
#define RANGE2_LOWPOW_LIMIT 26000000
#define RANGE1_BOOST1_LIMIT 150000000
#define RANGE1_BOOST0_LIMIT 170000000

/* TYPES **********************************************************************/
/* Alias to ST HAL GPIO_TypeDef to avoid checkpatch getting confused */
typedef GPIO_TypeDef GPIO_TypeDef_t;

/** @brief Board's LED enumeration.
 */
typedef enum evk_led {
    LED0,
    LED1,
    LED2
} evk_led_t;

/** @brief Board's debug GPIO enumeration.
 */
typedef enum evk_dbg {
    DBG0,
    DBG1,
    DBG2,
    DBG3
} evk_dbg_t;

/** @brief Board's debug pins enumeration.
 */
typedef enum evk_debug_pin {
    DEBUG_PC7,
    DEBUG_PC8,
    DEBUG_PC10,
    DEBUG_PC11
} evk_debug_pin_t;

/** @brief Board's User button enumeration.
 */
typedef enum evk_btn {
    BTN1,
    BTN2,
} evk_btn_t;

/** @brief Board's VDD enumeration.
 */
typedef enum evk_vdd {
    VDD_1V8,
    VDD_3V3
} evk_vdd_t;

/** @brief Board's supported clock frequency.
 */
typedef enum evk_clk_freq {
    CLK_169_98MHZ = 169984000,
    CLK_163_84MHZ = 163840000,
    CLK_81_92MHZ  = 81920000,
    CLK_40_96MHZ  = 40960000,
    CLK_25_20MHZ  = 25200000,
    CLK_20_48MHZ  = 20480000,
    CLK_16MHZ     = 16000000
} evk_clk_freq_t;

/** @brief Board's SPI Baudrate prescaler enumeration.
 */
typedef enum spi_prescaler {
    SPI_PRESCALER_2   = SPI_BAUDRATEPRESCALER_2,
    SPI_PRESCALER_4   = SPI_BAUDRATEPRESCALER_4,
    SPI_PRESCALER_8   = SPI_BAUDRATEPRESCALER_8,
    SPI_PRESCALER_16  = SPI_BAUDRATEPRESCALER_16,
    SPI_PRESCALER_32  = SPI_BAUDRATEPRESCALER_32,
    SPI_PRESCALER_64  = SPI_BAUDRATEPRESCALER_64,
    SPI_PRESCALER_128 = SPI_BAUDRATEPRESCALER_128,
    SPI_PRESCALER_256 = SPI_BAUDRATEPRESCALER_256
} spi_prescaler_t;

#ifdef __cplusplus
}
#endif

#endif /* EVK_DEF_H_ */

