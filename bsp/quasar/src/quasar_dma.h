/** @file  quasar_dma.h
 *  @brief This module sets up DMA transactions and provides DMA mode read/write functions.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_DMA_H_
#define QUASAR_DMA_H_

/* INCLUDES *******************************************************************/
#include "quasar_it.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief List of all available channel of the general purpose DMA 1 instances.
 */
typedef enum quasar_dma_selection {
    /*! Select channel 0 of the general purpose DMA 1 instance. */
    QUASAR_DMA_SELECTION_GPDMA1_CHANNEL0 = 0,
    /*! Select channel 1 of the general purpose DMA 1 instance. */
    QUASAR_DMA_SELECTION_GPDMA1_CHANNEL1 = 1,
    /*! Select channel 2 of the general purpose DMA 1 instance. */
    QUASAR_DMA_SELECTION_GPDMA1_CHANNEL2 = 2,
    /*! Select channel 3 of the general purpose DMA 1 instance. */
    QUASAR_DMA_SELECTION_GPDMA1_CHANNEL3 = 3,
    /*! Select channel 4 of the general purpose DMA 1 instance. */
    QUASAR_DMA_SELECTION_GPDMA1_CHANNEL4 = 4,
    /*! Select channel 5 of the general purpose DMA 1 instance. */
    QUASAR_DMA_SELECTION_GPDMA1_CHANNEL5 = 5,
    /*! Select channel 6 of the general purpose DMA 1 instance. */
    QUASAR_DMA_SELECTION_GPDMA1_CHANNEL6 = 6,
    /*! Select channel 7 of the general purpose DMA 1 instance. */
    QUASAR_DMA_SELECTION_GPDMA1_CHANNEL7 = 7,
    /*! Select channel 8 of the general purpose DMA 1 instance. */
    QUASAR_DMA_SELECTION_GPDMA1_CHANNEL8 = 8,
    /*! Select channel 9 of the general purpose DMA 1 instance. */
    QUASAR_DMA_SELECTION_GPDMA1_CHANNEL9  = 9,
    /*! Select channel 10 of the general purpose DMA 1 instance. */
    QUASAR_DMA_SELECTION_GPDMA1_CHANNEL10 = 10,
    /*! Select channel 11 of the general purpose DMA 1 instance. */
    QUASAR_DMA_SELECTION_GPDMA1_CHANNEL11 = 11,
    /*! Select channel 12 of the general purpose DMA 1 instance. */
    QUASAR_DMA_SELECTION_GPDMA1_CHANNEL12 = 12,
    /*! Select channel 13 of the general purpose DMA 1 instance. */
    QUASAR_DMA_SELECTION_GPDMA1_CHANNEL13 = 13,
    /*! Select channel 14 of the general purpose DMA 1 instance. */
    QUASAR_DMA_SELECTION_GPDMA1_CHANNEL14 = 14,
    /*! Select channel 15 of the general purpose DMA 1 instance. */
    QUASAR_DMA_SELECTION_GPDMA1_CHANNEL15 = 15,
    /*! Do not select any channel general purpose DMA 1 instance if only tx or rx is used. */
    QUASAR_DMA_SELECTION_NOT_USED         = 16
} quasar_dma_selection_t;

/** @brief List of all available peripherals that can be used as source or/and
 *         destination for DMA transfers.
 */
typedef enum quasar_dma_peripheral {
    /*! Select the UART peripheral to be used for the DMA transfer. */
    QUASAR_DMA_PERIPHERAL_UART = 0,
    /*! Select the SPI peripheral to be used for the DMA transfer. */
    QUASAR_DMA_PERIPHERAL_SPI = 1,
} quasar_dma_peripheral_t;

/** @brief Quasar BSP DMA configuration.
 */
typedef struct quasar_dma_config {
    /*! Selected DMA instance for transmission. */
    quasar_dma_selection_t dma_selection_tx;
    /*! Selected DMA instance for reception. */
    quasar_dma_selection_t dma_selection_rx;
    /*! Selected DMA peripheral (This driver configure the DMA transfer mode as memory-to-peripheral vice-versa). */
    quasar_dma_peripheral_t dma_peripheral;
    /*! Selected peripheral instance. */
    uint8_t peripheral_selection;
    /*! Available IRQ priority. */
    quasar_irq_priority_t irq_priority;
} quasar_dma_config_t;

/* PUBLIC FUNCTIONS ***********************************************************/
/** @brief Initializes the DMA to operate in either peripheral to memory or
 *         memory to peripheral mode.
 *
 *  @note Currently, only peripherals of type UART are
 *        supported with this DMA configuration.
 *
 *  @param[in] dma_config  Configuration of the DMA peripheral.
 */
void quasar_dma_init(quasar_dma_config_t dma_config);

/** @brief Deinitializes the DMA, disabling its operation and interrupt handling.
 *
 *  @param[in] dma_config  Configuration of the DMA peripheral.
 */
void quasar_dma_deinit(quasar_dma_config_t dma_config);

/** @brief Enable the DMA's global interrupt.
 *
 *  @param[in] dma_selection  Selected DMA.
 */
void quasar_dma_enable_irq(quasar_dma_selection_t dma_selection);

/** @brief Disable the DMA's global interrupt.
 *
 *  @param[in] dma_selection  Selected DMA.
 */
void quasar_dma_disable_irq(quasar_dma_selection_t dma_selection);

/** @brief This function sets the function callback for the GPDMA1 Channel 0 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_dma_set_channel0_dma_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the GPDMA1 Channel 1 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_dma_set_channel1_dma_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the GPDMA1 Channel 2 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_dma_set_channel2_dma_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the GPDMA1 Channel 3 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_dma_set_channel3_dma_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the GPDMA1 Channel 4 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_dma_set_channel4_dma_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the GPDMA1 Channel 5 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_dma_set_channel5_dma_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the GPDMA1 Channel 6 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_dma_set_channel6_dma_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the GPDMA1 Channel 7 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_dma_set_channel7_dma_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the GPDMA1 Channel 8 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_dma_set_channel8_dma_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the GPDMA1 Channel 9 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_dma_set_channel9_dma_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the GPDMA1 Channel 10 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_dma_set_channel10_dma_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the GPDMA1 Channel 11 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_dma_set_channel11_dma_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the GPDMA1 Channel 12 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_dma_set_channel12_dma_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the GPDMA1 Channel 13 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_dma_set_channel13_dma_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the GPDMA1 Channel 14 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_dma_set_channel14_dma_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the GPDMA1 Channel 15 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_dma_set_channel15_dma_callback(void (*irq_callback)(void));

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_DMA_H_ */
