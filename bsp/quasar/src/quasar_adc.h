/** @file  quasar_adc.h
 *  @brief This file contain the functions to configure and use the ADC peripheral.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_ADC_H_
#define QUASAR_ADC_H_

/* INCLUDES *******************************************************************/
#include "quasar_def.h"
#include <stdbool.h>
#include "quasar_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief The ADC peripheral.
 *
 *  Note: The ADC4 was not included since its implementation is different.
 */
typedef enum quasar_adc_peripheral {
    /*! The ADC1 peripheral. */
    QUASAR_ADC_PERIPHERAL_1,
    /*! The ADC2 peripheral. */
    QUASAR_ADC_PERIPHERAL_2,
} quasar_adc_peripheral_t;

/** @brief The ADC resolution.
 */
typedef enum quasar_adc_resolution {
    /*! The ADC resolution is set to 14 bits. */
    QUASAR_ADC_RESOLUTION_14B = ADC_RESOLUTION_14B,
    /*! The ADC resolution is set to 12 bits. */
    QUASAR_ADC_RESOLUTION_12B = ADC_RESOLUTION_12B,
    /*! The ADC resolution is set to 10 bits. */
    QUASAR_ADC_RESOLUTION_10B = ADC_RESOLUTION_10B,
    /*! The ADC resolution is set to 8 bits. */
    QUASAR_ADC_RESOLUTION_8B = ADC_RESOLUTION_8B,
} quasar_adc_resolution_t;

/** @brief The ADC peripheral configuration.
 */
typedef struct quasar_adc_cfg {
    /*! The ADC peripheral to configure. */
    quasar_adc_peripheral_t peripheral;
    /*! The ADC's peripheral resolution. */
    quasar_adc_resolution_t resolution;
} quasar_adc_cfg_t;

/** @brief The available ADC channels.
 */
typedef enum quasar_adc_channel {
    /*! The ADC channel 1 */
    QUASAR_ADC_CHANNEL_1  = ADC_CHANNEL_1,
    /*! The ADC channel 2 */
    QUASAR_ADC_CHANNEL_2  = ADC_CHANNEL_2,
    /*! The ADC channel 3 */
    QUASAR_ADC_CHANNEL_3  = ADC_CHANNEL_3,
    /*! The ADC channel 4 */
    QUASAR_ADC_CHANNEL_4  = ADC_CHANNEL_4,
    /*! The ADC channel 5 */
    QUASAR_ADC_CHANNEL_5  = ADC_CHANNEL_5,
    /*! The ADC channel 6 */
    QUASAR_ADC_CHANNEL_6  = ADC_CHANNEL_6,
    /*! The ADC channel 7 */
    QUASAR_ADC_CHANNEL_7  = ADC_CHANNEL_7,
    /*! The ADC channel 8 */
    QUASAR_ADC_CHANNEL_8  = ADC_CHANNEL_8,
    /*! The ADC channel 9 */
    QUASAR_ADC_CHANNEL_9  = ADC_CHANNEL_9,
    /*! The ADC channel 10 */
    QUASAR_ADC_CHANNEL_10 = ADC_CHANNEL_10,
    /*! The ADC channel 11 */
    QUASAR_ADC_CHANNEL_11 = ADC_CHANNEL_11,
    /*! The ADC channel 12 */
    QUASAR_ADC_CHANNEL_12 = ADC_CHANNEL_12,
    /*! The ADC channel 13 */
    QUASAR_ADC_CHANNEL_13 = ADC_CHANNEL_13,
    /*! The ADC channel 14 */
    QUASAR_ADC_CHANNEL_14 = ADC_CHANNEL_14,
    /*! The ADC channel 15 */
    QUASAR_ADC_CHANNEL_15 = ADC_CHANNEL_15,
    /*! The ADC channel 16 */
    QUASAR_ADC_CHANNEL_16 = ADC_CHANNEL_16,
    /*! The ADC channel 17 */
    QUASAR_ADC_CHANNEL_17 = ADC_CHANNEL_17,
    /*! MCU internal temperature sensor. */
    QUASAR_ADC_CHANNEL_TEMPSENSOR = ADC_CHANNEL_TEMPSENSOR,
} quasar_adc_channel_t;

/** @brief Quasar board supported revisions.
 */
typedef enum quasar_revision {
    QUASAR_REVA = 1,
    QUASAR_REVB = 2,
    _QUASAR_REV_COUNT,
} quasar_revision_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief Initialize the ADC peripheral for the Quasar main board, including GPIOs used for board revision and battery
 *         level monitoring. Initiate an ADC acquisition by polling method to get the board revision, and configure the
 *         ADC channel for battery level monitoring.
 *
 *  @return Board revision.
 */
quasar_revision_t quasar_adc_init(void);

/** @brief Deinitialize the ADC peripheral used for battery monitoring and board revision, deinitialize the GPIOs used
 *         for ADC, and disable the RCC clock for ADC1 and ADC2.
 */
void quasar_adc_deinit(void);

/** @brief Set the ADC voltage reference.
 *
 *  By default, the voltage reference is set to 3300 mV.
 *
 *  @param[in] voltage_reference_in_mv  The board's voltage reference in millivolts.
 */
void quasar_adc_set_voltage_reference(uint16_t voltage_reference_in_mv);

/** @brief Get the ADC voltage reference.
 *
 *  @return The ADC voltage reference in millivolts.
 */
uint16_t quasar_adc_get_voltage_reference(void);

/** @brief Start an ADC acquisition and return the raw value. After the data acquisition, the channel is unselected to
 *         free the peripheral for other uses.
 *
 *  - The ADC channel must be selected before calling this function.
 *  - The GPIO related to the ADC channel must be configured before calling this function.
 *
 *  @param[in] adc_peripheral  The selected ADC peripheral.
 *  @param[in] adc_channel     The selected ADC channel.
 *  @return The raw ADC value. This value needs to be converted depending on the usage.
 */
uint32_t quasar_adc_start_conversion_polling(quasar_adc_peripheral_t adc_peripheral, quasar_adc_channel_t adc_channel);

/** @brief Start an ADC acquisition by interrupt.
 *
 *  - The ADC channel must be selected before calling this function.
 *  - The GPIO related to the ADC channel must be configured before calling this function.
 *  - The value must be retrieved when the associated IRQ handler is called.
 *  - After the data acquisition, the channel must be unselected to free the peripheral for other uses.

 *  @param[in] adc_peripheral  The selected ADC peripheral.
 */
void quasar_adc_start_conversion_it(quasar_adc_peripheral_t adc_peripheral);

/** @brief Retrieve the battery level by initiating an ADC acquisition by polling method. The channel is configured and
 *         unselected before and after the data acquisition.
 *
 *  @return The battery voltage in millivolt.
 */
uint16_t quasar_adc_get_battery_level_mv_polling(void);

/** @brief Retrieve the local variable of the battery level.
 *
 *  - To use this function, start an ADC acquisition by interrupt first and retrieve the data in the associated IRQ
 *    handler, updating the local variable that contains the battery level.
 *
 *  @return The battery voltage in millivolt.
 */
uint16_t quasar_adc_get_battery_level_mv_it(void);

/** @brief Retrieve the board revision by initiating an ADC acquisition by polling method. The channel is configured and
 *         unselected before and after the data acquisition.
 *
 *  @return Board revision.
 */
quasar_revision_t quasar_adc_get_board_revision(void);

/** @brief Verify if the battery level value has been updated.
 *
 *  Use this function to validate that the ADC had time to acquire the data. It is mainly useful for the interrupt mode.
 *
 *  @return True if the battery level has been updated.
 */
bool quasar_adc_is_battery_level_value_ready(void);

/** @brief Sets the function callback for ADC1 interrupt. This callback is called at the end of the data acquisition,
 *         allowing data retrieval and finalizing the transaction by, for example, unselecting the channel.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_adc_set_adc1_irq_callback(void (*irq_callback)(void));

/** @brief Sets the function callback for ADC2 interrupt. This callback is called at the end of the data acquisition,
 *         allowing data retrieval and finalizing the transaction by, for example, unselecting the channel.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_adc_set_adc2_irq_callback(void (*irq_callback)(void));

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_ADC_H_ */
