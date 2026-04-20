/** @file  quasar_error.h
 *  @brief Quasar BSP Error handling module.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_ERROR_H_
#define QUASAR_ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif

/* CONSTANTS ******************************************************************/
/* Current __COUNTER__ value. */
enum {
    QUASAR_COUNTER_BASE = __COUNTER__
};

/* MACROS *********************************************************************/
#ifndef ASSERT_QUASAR_BSP_STATUS
/** @brief ASSERT the return status of a Quasar BSP function.
 */
#define ASSERT_QUASAR_BSP_STATUS(quasar_status)        \
    do {                                               \
        if (quasar_status == QUASAR_OK) {              \
            /* Exit early. */                          \
            break;                                     \
        }                                              \
                                                       \
        if (quasar_status > QUASAR_OK) {               \
            /* Handle warning. */                      \
            quasar_bsp_warning_handler(quasar_status); \
            break;                                     \
        }                                              \
                                                       \
        /* Handle error. */                            \
        quasar_bsp_error_handler(quasar_status);       \
    } while (0)
#endif

/** @brief Create a local counter macro.
 */
#define QUASAR_COUNTER (__COUNTER__ - QUASAR_COUNTER_BASE)

/** @brief Automatically increase error count each time invoked.
 */
#define QUASAR_GENERATE_ERR_CODE (-(QUASAR_COUNTER))

/** @brief Check error condition and set error code with early return.
 */
#define QUASAR_BSP_CHECK_ERROR(cond, err_ptr, err_code, ret) \
    do {                                                     \
        if (cond) {                                          \
            if ((err_ptr) != NULL) {                         \
                *(err_ptr) = (err_code);                     \
            }                                                \
            ret;                                             \
        }                                                    \
    } while (0)

/* TYPES **********************************************************************/
/** @brief Quasar BSP Errors and Warnings.
 */
typedef enum quasar_bsp_status {
    /*! No error nor warning occurred. */
    QUASAR_OK = 0,

    /*! quasar_adc.c generic error. */
    QUASAR_ERR_ADC = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_adc.c ADC handle null error. */
    QUASAR_ERR_ADC_HANDLE_NULL = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_adc.c ADC init error. */
    QUASAR_ERR_ADC_INIT = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_adc.c ADC calibration error. */
    QUASAR_ERR_ADC_CALIBRATION = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_adc.c ADC deinit error. */
    QUASAR_ERR_ADC_DEINIT = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_adc.c ADC clock config error. */
    QUASAR_ERR_ADC_CLOCK_CONFIG = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_adc.c ADC clock frequency error. */
    QUASAR_ERR_ADC_CLOCK_FREQ = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_adc.c ADC channel config error. */
    QUASAR_ERR_ADC_CHANNEL_CONFIG = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_adc.c ADC start conversion error. */
    QUASAR_ERR_ADC_START = QUASAR_GENERATE_ERR_CODE,

    /*! quasar_audio.c generic error. */
    QUASAR_ERR_AUDIO = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_audio.c invalid bit depth error. */
    QUASAR_ERR_AUDIO_INVALID_BIT_DEPTH = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_audio.c invalid audio frequency error. */
    QUASAR_ERR_AUDIO_INVALID_FREQUENCY = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_audio.c SAI TX init protocol error. */
    QUASAR_ERR_AUDIO_SAI_TX_INIT = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_audio.c SAI RX init protocol error. */
    QUASAR_ERR_AUDIO_SAI_RX_INIT = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_audio.c SAI TX deinit error. */
    QUASAR_ERR_AUDIO_SAI_TX_DEINIT = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_audio.c SAI RX deinit error. */
    QUASAR_ERR_AUDIO_SAI_RX_DEINIT = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_audio.c SAI clock config error. */
    QUASAR_ERR_AUDIO_SAI_CLOCK_CONFIG = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_audio.c DMA TX init error. */
    QUASAR_ERR_AUDIO_DMA_TX_INIT = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_audio.c DMA RX init error. */
    QUASAR_ERR_AUDIO_DMA_RX_INIT = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_audio.c DMA TX deinit error. */
    QUASAR_ERR_AUDIO_DMA_TX_DEINIT = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_audio.c DMA RX deinit error. */
    QUASAR_ERR_AUDIO_DMA_RX_DEINIT = QUASAR_GENERATE_ERR_CODE,

    /*! quasar_button.c error. */
    QUASAR_ERR_BUTTON = QUASAR_GENERATE_ERR_CODE,

    /*! quasar_clock.c error. */
    QUASAR_ERR_CLOCK_INIT = QUASAR_GENERATE_ERR_CODE,

    /*! quasar_debug.c error. */
    QUASAR_ERR_DEBUG = QUASAR_GENERATE_ERR_CODE,

    /*! quasar_dma.c generic error. */
    QUASAR_ERR_DMA = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_dma.c invalid peripheral selection error. */
    QUASAR_ERR_DMA_INVALID_PERIPHERAL = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_dma.c invalid UART selection error. */
    QUASAR_ERR_DMA_INVALID_UART_SELECTION = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_dma.c invalid SPI selection error. */
    QUASAR_ERR_DMA_INVALID_SPI_SELECTION = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_dma.c DMA TX init error. */
    QUASAR_ERR_DMA_TX_INIT = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_dma.c DMA RX init error. */
    QUASAR_ERR_DMA_RX_INIT = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_dma.c DMA TX deinit error. */
    QUASAR_ERR_DMA_TX_DEINIT = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_dma.c DMA RX deinit error. */
    QUASAR_ERR_DMA_RX_DEINIT = QUASAR_GENERATE_ERR_CODE,

    /*! quasar_fifo.c generic error. */
    QUASAR_ERR_FIFO = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_fifo.c invalid number of bytes error. */
    QUASAR_ERR_FIFO_INVALID_BYTE_COUNT = QUASAR_GENERATE_ERR_CODE,

    /*! quasar_gpio.c error. */
    QUASAR_ERR_GPIO = QUASAR_GENERATE_ERR_CODE,

    /*! quasar_i2c.c generic error. */
    QUASAR_ERR_I2C = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_i2c.c I2C busy timeout error. */
    QUASAR_ERR_I2C_BUSY_TIMEOUT = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_i2c.c I2C TXIS timeout error. */
    QUASAR_ERR_I2C_TXIS_TIMEOUT = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_i2c.c I2C TC timeout error. */
    QUASAR_ERR_I2C_TC_TIMEOUT = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_i2c.c I2C RXNE timeout error. */
    QUASAR_ERR_I2C_RXNE_TIMEOUT = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_i2c.c I2C FIFO pull error in IRQ handler. */
    QUASAR_ERR_I2C_IRQ_FIFO_PULL = QUASAR_GENERATE_ERR_CODE,

    /*! quasar_it.c error. */
    QUASAR_ERR_IT = QUASAR_GENERATE_ERR_CODE,

    /*! quasar_led.c error. */
    QUASAR_ERR_LED = QUASAR_GENERATE_ERR_CODE,

    /*! quasar_memory.c error. */
    QUASAR_ERR_MEMORY = QUASAR_GENERATE_ERR_CODE,

    /*! quasar_power.c error. */
    QUASAR_ERR_POWER = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_power.c generic error. */
    QUASAR_ERR_POWER_CONFIG_SUPPLY = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_power.c unsupported board revision error. */
    QUASAR_ERR_POWER_UNSUPPORTED_REVISION = QUASAR_GENERATE_ERR_CODE,

    /*! quasar_profiler.c error. */
    QUASAR_ERR_PROFILER = QUASAR_GENERATE_ERR_CODE,

    /*! quasar_pwm.c error. */
    QUASAR_ERR_PWM = QUASAR_GENERATE_ERR_CODE,

    /*! quasar_radio.c generic error. */
    QUASAR_ERR_RADIO = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_radio.c unsupported board revision error. */
    QUASAR_ERR_RADIO_UNSUPPORTED_REVISION = QUASAR_GENERATE_ERR_CODE,

    /*! quasar_rgb.c error. */
    QUASAR_ERR_RGB = QUASAR_GENERATE_ERR_CODE,

    /*! quasar_spi.c generic error. */
    QUASAR_ERR_SPI = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_spi.c SPI init error. */
    QUASAR_ERR_SPI_INIT = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_spi.c SPI deinit error. */
    QUASAR_ERR_SPI_DEINIT = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_spi.c SPI baudrate config error. */
    QUASAR_ERR_SPI_BAUDRATE_CONFIG = QUASAR_GENERATE_ERR_CODE,

    /*! quasar_timer.c error. */
    QUASAR_ERR_TIMER = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_timer_ext.c error. */
    QUASAR_ERR_TIMER_EXT = QUASAR_GENERATE_ERR_CODE,

    /*! quasar_uart.c generic error. */
    QUASAR_ERR_UART = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_uart.c UART transmit DMA error. */
    QUASAR_ERR_UART_TRANSMIT_DMA = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_uart.c UART transmit blocking error. */
    QUASAR_ERR_UART_TRANSMIT_BLOCKING = QUASAR_GENERATE_ERR_CODE,

    /*! quasar_usb.c generic error. */
    QUASAR_ERR_USB = QUASAR_GENERATE_ERR_CODE,
    /*! quasar_usb.c USB peripheral clock config error. */
    QUASAR_ERR_USB_PERIPH_CLK_CONFIG = QUASAR_GENERATE_ERR_CODE,

} quasar_bsp_status_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Handle a Quasar BSP error.
 *
 *  @note This function has a weak definition in quasar_error.c. The function can be redifined in the application to
 *        change the error handling behaviour.
 *
 *  @param[in] quasar_status  Quasar BSP status code.
 */
void quasar_bsp_error_handler(quasar_bsp_status_t quasar_status);

/** @brief Handle a Quasar BSP warning.
 *
 *  @note This function has a weak definition in quasar_error.c. The function can be redifined in the application to
 *        change the warning handling behaviour.
 *
 *  @param[in] quasar_status  Quasar BSP status code.
 */
void quasar_bsp_warning_handler(quasar_bsp_status_t quasar_status);

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_ERROR_H_ */
