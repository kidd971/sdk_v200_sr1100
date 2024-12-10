/** @file  evk_dac.c
 *  @brief This module controls DAC features of SPARK EVK board.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "evk_dac.h"
#include "evk_it.h"

/* PRIVATE GLOBALS ************************************************************/
DAC_HandleTypeDef hdac1;

/* PUBLIC FUNCTIONS ***********************************************************/
void evk_exp_dac1_ch1_init(void)
{
    __HAL_RCC_DAC1_CLK_ENABLE();

    /* Init DAC */
    DAC_ChannelConfTypeDef sConfig = {0};

    hdac1.Instance = DAC1;
    if (HAL_DAC_Init(&hdac1) != HAL_OK) {
        Error_Handler();
    }

    /* DAC channel OUT1 config */
    sConfig.DAC_HighFrequency = DAC_HIGH_FREQUENCY_INTERFACE_MODE_AUTOMATIC;
    sConfig.DAC_DMADoubleDataMode = DISABLE;
    sConfig.DAC_SignedFormat = DISABLE;
    sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_ENABLE;
    sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
    sConfig.DAC_Trigger2 = DAC_TRIGGER_NONE;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_BOTH;
    sConfig.DAC_UserTrimming = DAC_TRIMMING_FACTORY;

    if (HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }
    __HAL_DAC_ENABLE(&hdac1, DAC1_CHANNEL_1);
}

void evk_exp_dac1_ch1_convert(uint16_t data)
{
    DAC1->DHR12R1 = data;
}
