/** @file  evk_audio.c
 *  @brief Initialize audio related peripherals.
 *
 *  @copyright Copyright (C) 2021 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "evk_audio.h"
#include "evk_it.h"

/* TYPES **********************************************************************/
/** @brief SAI DMA transmit direction enum.
 */
typedef enum sai_direction {
    SAI_MCU_TO_CODEC,
    SAI_CODEC_TO_MCU,
} sai_direction_t;

/* PRIVATE GLOBALS ************************************************************/
static irq_callback sai_dma_tx_irq_callback;
static irq_callback sai_dma_rx_irq_callback;
I2C_HandleTypeDef hi2c1         = {
    .Instance                   = I2C1,
    .Init.AddressingMode        = I2C_ADDRESSINGMODE_7BIT,
    .Init.DualAddressMode       = I2C_DUALADDRESS_DISABLE,
    .Init.GeneralCallMode       = I2C_GENERALCALL_DISABLE,
    .Init.NoStretchMode         = I2C_NOSTRETCH_DISABLE,
    .Init.OwnAddress1           = 0,
    .Init.OwnAddress2           = 0,
    .Init.OwnAddress2Masks      = I2C_OA2_NOMASK,
    .Init.Timing                = AUDIO_I2C_TIMING
};

SAI_HandleTypeDef hsai_block_a1 = {
    .Instance                   = SAI1_Block_A,
    .Init.AudioMode             = SAI_MODESLAVE_RX,
    .Init.Synchro               = SAI_SYNCHRONOUS,
    .Init.OutputDrive           = SAI_OUTPUTDRIVE_DISABLE,
    .Init.FIFOThreshold         = SAI_FIFOTHRESHOLD_EMPTY,
    .Init.SynchroExt            = SAI_SYNCEXT_DISABLE,
    .Init.CompandingMode        = SAI_NOCOMPANDING,
    .Init.TriState              = SAI_OUTPUT_NOTRELEASED,
};

SAI_HandleTypeDef hsai_block_b1 = {
    .Instance                   = SAI1_Block_B,
    .Init.AudioMode             = SAI_MODESLAVE_TX,
    .Init.Synchro               = SAI_ASYNCHRONOUS,
    .Init.OutputDrive           = SAI_OUTPUTDRIVE_DISABLE,
    .Init.NoDivider             = SAI_MASTERDIVIDER_DISABLE,
    .Init.FIFOThreshold         = SAI_FIFOTHRESHOLD_FULL,
    .Init.SynchroExt            = SAI_SYNCEXT_DISABLE,
    .Init.CompandingMode        = SAI_NOCOMPANDING,
    .Init.TriState              = SAI_OUTPUT_NOTRELEASED,
    .Init.AudioFrequency        = SAI_AUDIO_FREQUENCY_MCKDIV,
    .Init.Mckdiv                = 0,
    .Init.MckOutput             = SAI_MCK_OUTPUT_ENABLE,
};

DMA_HandleTypeDef hdma_sai1_a   = {
    .Instance                   = DMA2_CHANNEL_SAI_RX,
    .Init.Request               = DMA_REQUEST_SAI1_A,
    .Init.Direction             = DMA_PERIPH_TO_MEMORY,
    .Init.PeriphInc             = DMA_PINC_DISABLE,
    .Init.MemInc                = DMA_MINC_ENABLE,
    .Init.PeriphDataAlignment   = DMA_PDATAALIGN_HALFWORD,
    .Init.MemDataAlignment      = DMA_MDATAALIGN_HALFWORD,
    .Init.Mode                  = DMA_NORMAL,
    .Init.Priority              = DMA_PRIORITY_VERY_HIGH
};

DMA_HandleTypeDef hdma_sai1_b   = {
    .Instance                   = DMA2_CHANNEL_SAI_TX,
    .Init.Request               = DMA_REQUEST_SAI1_B,
    .Init.Direction             = DMA_MEMORY_TO_PERIPH,
    .Init.PeriphInc             = DMA_PINC_DISABLE,
    .Init.MemInc                = DMA_MINC_ENABLE,
    .Init.PeriphDataAlignment   = DMA_PDATAALIGN_HALFWORD,
    .Init.MemDataAlignment      = DMA_MDATAALIGN_HALFWORD,
    .Init.Mode                  = DMA_NORMAL,
    .Init.Priority              = DMA_PRIORITY_VERY_HIGH
};

/* PRIVATE FUNCTIONS PROTOTYPES ***********************************************/
static void init_audio_sai_clocks(void);
static void init_audio_sai_gpios(void);
static void configure_sai_rx_monostereo_mode(evk_sai_mono_stereo_t mode);
static void configure_sai_tx_monostereo_mode(evk_sai_mono_stereo_t mode);
static void init_audio_sai_peripheral(evk_sai_bit_depth_t sai_bit_depth);
static void init_audio_sai_nvic(void);
static void init_audio_sai_dma(void);
static void init_audio_i2c_clock(void);
static void init_audio_i2c_gpios(void);
static void init_audio_i2c_peripheral(void);
static void sai_dma_start_it(DMA_HandleTypeDef *hdma, uint32_t source_addr, uint32_t destination_addr, uint32_t sample_count,
                             sai_direction_t direction);
static void sai_dma_tx_complete_callback(DMA_HandleTypeDef *hdma);
static void sai_dma_rx_complete_callback(DMA_HandleTypeDef *hdma);
static int8_t sai_block_size_to_bytes(uint32_t sai_block_size);

/* PUBLIC FUNCTIONS ***********************************************************/
void evk_audio_i2c_write_byte_blocking(uint8_t dev_addr, uint8_t mem_addr, uint8_t data)
{
    uint8_t payload[2] = {
        mem_addr,
        data
    };

    HAL_I2C_Master_Transmit(&hi2c1, (uint16_t)dev_addr, (uint8_t *)payload, sizeof(payload), AUDIO_I2C_TIMEOUT_MS);
}

void evk_audio_i2c_read_byte_blocking(uint8_t dev_addr, uint8_t mem_addr, uint8_t *data)
{
    HAL_I2C_Mem_Read(&hi2c1, (uint16_t)dev_addr, mem_addr, I2C_MEMADD_SIZE_8BIT, data, 1, AUDIO_I2C_TIMEOUT_MS);
}

void evk_audio_i2c_init(void)
{
    init_audio_i2c_clock();
    init_audio_i2c_gpios();
    init_audio_i2c_peripheral();
}

void evk_audio_sai_configuration(evk_sai_config_t *sai_config)
{
    init_audio_sai_gpios();
    init_audio_sai_clocks();
    configure_sai_tx_monostereo_mode(sai_config->tx_sai_mono_stereo);
    configure_sai_rx_monostereo_mode(sai_config->rx_sai_mono_stereo);
    init_audio_sai_peripheral(sai_config->sai_bit_depth);
    init_audio_sai_nvic();
    init_audio_sai_dma();
}

void evk_audio_sai_deinit(void)
{
    /* Deinit DMA. */
    if (HAL_DMA_DeInit(&hdma_sai1_a) != HAL_OK) {
        while (1);
    }

    if (HAL_DMA_DeInit(&hdma_sai1_b) != HAL_OK) {
        while (1);
    }

    /* Deinit NVIC. */
    HAL_NVIC_DisableIRQ(NVIC_SAI_DMA_TX_CPLT);
    HAL_NVIC_DisableIRQ(NVIC_SAI_DMA_RX_CPLT);

    /* Deinit SAI. */
    if (HAL_SAI_DeInit(&hsai_block_a1) != HAL_OK) {
        while (1);
    }

    if (HAL_SAI_DeInit(&hsai_block_b1) != HAL_OK) {
        while (1);
    }

    /* Deinit GPIOs. */
    HAL_GPIO_DeInit(AUDIO_MCO_CLK_PORT, AUDIO_MCO_CLK_PIN);
    HAL_GPIO_DeInit(AUDIO_SAI_IN_CLK_PORT, AUDIO_SAI_IN_CLK_PIN);
    HAL_GPIO_DeInit(AUDIO_SAI_MCLK_PORT, AUDIO_SAI_MCLK_PIN);
    HAL_GPIO_DeInit(AUDIO_SAI_SCK_PORT, AUDIO_SAI_SCK_PIN);
    HAL_GPIO_DeInit(AUDIO_SAI_FS_PORT, AUDIO_SAI_FS_PIN);
    HAL_GPIO_DeInit(AUDIO_SAI_SD_A_PORT, AUDIO_SAI_SD_A_PIN);
    HAL_GPIO_DeInit(AUDIO_SAI_SD_B_PORT, AUDIO_SAI_SD_B_PIN);

    /* Deinit clock. */
    __HAL_RCC_SAI1_CLK_DISABLE();
}

void evk_audio_sai_write_non_blocking(uint8_t *data, uint16_t size)
{
    uint32_t sample_count = size / sai_block_size_to_bytes(hsai_block_b1.Init.DataSize);

    sai_dma_start_it(&hdma_sai1_b, (uint32_t)data, (uint32_t)&hsai_block_b1.Instance->DR, sample_count, SAI_MCU_TO_CODEC);

    /* Enable SAI DMA Request */
    hsai_block_b1.Instance->CR1 |= SAI_xCR1_DMAEN;

    /* Enable SAI peripheral */
    __HAL_SAI_ENABLE(&hsai_block_b1);
}

void evk_audio_sai_read_non_blocking(uint8_t *data, uint16_t size)
{
    uint32_t sample_count = size / sai_block_size_to_bytes(hsai_block_a1.Init.DataSize);

    sai_dma_start_it(&hdma_sai1_a, (uint32_t)&hsai_block_a1.Instance->DR, (uint32_t)data, sample_count, SAI_CODEC_TO_MCU);

    /* Enable SAI DMA Request */
    hsai_block_a1.Instance->CR1 |= SAI_xCR1_DMAEN;

    /* Enable SAI peripheral */
    __HAL_SAI_ENABLE(&hsai_block_a1);
}

void evk_audio_set_sai_tx_dma_cplt_callback(irq_callback callback)
{
    sai_dma_tx_irq_callback = callback;
}

void evk_audio_set_sai_rx_dma_cplt_callback(irq_callback callback)
{
    sai_dma_rx_irq_callback = callback;
}

void evk_audio_sai_start_write_non_blocking(void)
{
    sai_dma_tx_irq_callback();
}

void evk_audio_sai_start_read_non_blocking(void)
{
    sai_dma_rx_irq_callback();
}

void evk_audio_sai_stop_write_non_blocking(void)
{
    if (hsai_block_b1.hdmatx->Instance->CCR & (DMA_IT_TC | DMA_IT_TE)) {
        __HAL_DMA_DISABLE(&hdma_sai1_b);
    }
}

void evk_audio_sai_stop_read_non_blocking(void)
{
    if (hsai_block_a1.hdmarx->Instance->CCR & (DMA_IT_TC | DMA_IT_TE)) {
        __HAL_DMA_DISABLE(&hdma_sai1_a);
    }
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize the expansion port's SAI clock.
 */
static void init_audio_sai_clocks(void)
{
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    __HAL_RCC_SAI1_CLK_ENABLE();
    __HAL_RCC_I2C2_CLK_ENABLE();

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_SAI1;
    PeriphClkInit.Sai1ClockSelection   = RCC_SAI1CLKSOURCE_EXT;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        Error_Handler();
    }

    HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSE, RCC_MCODIV_1);
}

/** @brief Initialize the expansion port's SAI GPIOs.
 */
static void init_audio_sai_gpios(void)
{
    GPIO_InitTypeDef mc0_config = {
        .Pin = AUDIO_MCO_CLK_PIN,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_MEDIUM,
        .Alternate = GPIO_AF0_MCO
    };
    HAL_GPIO_Init(AUDIO_MCO_CLK_PORT, &mc0_config);

    GPIO_InitTypeDef clk_in_config = {
        .Pin = AUDIO_SAI_IN_CLK_PIN,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_MEDIUM,
        .Alternate = GPIO_AF5_SPI2
    };
    HAL_GPIO_Init(AUDIO_SAI_IN_CLK_PORT, &clk_in_config);

    GPIO_InitTypeDef mclk_config = {
        .Pin = AUDIO_SAI_MCLK_PIN,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .Alternate = GPIO_AF14_SAI1
    };
    HAL_GPIO_Init(AUDIO_SAI_MCLK_PORT, &mclk_config);

    GPIO_InitTypeDef sck_config = {
        .Pin = AUDIO_SAI_SCK_PIN,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_HIGH,
        .Alternate = GPIO_AF14_SAI1
    };
    HAL_GPIO_Init(AUDIO_SAI_SCK_PORT, &sck_config);

    GPIO_InitTypeDef fs_config = {
        .Pin = AUDIO_SAI_FS_PIN,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_HIGH,
        .Alternate = GPIO_AF14_SAI1
    };
    HAL_GPIO_Init(AUDIO_SAI_FS_PORT, &fs_config);

    GPIO_InitTypeDef sda_config = {
        .Pin = AUDIO_SAI_SD_A_PIN,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_HIGH,
        .Alternate = GPIO_AF14_SAI1
    };
    HAL_GPIO_Init(AUDIO_SAI_SD_A_PORT, &sda_config);

    GPIO_InitTypeDef sdb_config = {
        .Pin = AUDIO_SAI_SD_B_PIN,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_HIGH,
        .Alternate = GPIO_AF12_SAI1
    };
    HAL_GPIO_Init(AUDIO_SAI_SD_B_PORT, &sdb_config);
}

/** @brief Set the expansion port's SAI Rx to Mono or Stereo.
 *
 *  @param[in] mode  Select Mono or Stereo mode.
 */
static void configure_sai_rx_monostereo_mode(evk_sai_mono_stereo_t mode)
{
    if (mode == EVK_SAI_MODE_MONO) {
        hsai_block_a1.Init.MonoStereoMode = SAI_MONOMODE;
    } else {
        hsai_block_a1.Init.MonoStereoMode = SAI_STEREOMODE;
    }
}

/** @brief Set the expansion port's SAI Tx to Mono or Stereo.
 *
 *  @param[in] mode  Select Mono or Stereo mode.
 */
static void configure_sai_tx_monostereo_mode(evk_sai_mono_stereo_t mode)
{
    if (mode == EVK_SAI_MODE_MONO) {
        hsai_block_b1.Init.MonoStereoMode = SAI_MONOMODE;
    } else {
        hsai_block_b1.Init.MonoStereoMode = SAI_STEREOMODE;
    }
}

/** @brief Initialize the expansion port's SAI peripheral.
 *
 *  @param[in] bit_depth  Audio bit depth.
 */
static void init_audio_sai_peripheral(evk_sai_bit_depth_t sai_bit_depth)
{
    if (HAL_SAI_InitProtocol(&hsai_block_a1, SAI_I2S_LSBJUSTIFIED, sai_bit_depth, EVK_SAI_NB_OF_SLOTS) != HAL_OK) {
        Error_Handler();
    }
    /* Set the frame synchronization polarity to active-low (falling edge). The STM32G4 HAL function
     * hardcodes it to active-high (rising edge) when the protocol is SAI_I2S_LSBJUSTIFIED, hence the
     * need to override it this way if active-low is desired.
     */
    __HAL_SAI_DISABLE(&hsai_block_a1);
    hsai_block_a1.Instance->FRCR &= ~SAI_xFRCR_FSPOL;

    if (HAL_SAI_InitProtocol(&hsai_block_b1, SAI_I2S_LSBJUSTIFIED, sai_bit_depth, EVK_SAI_NB_OF_SLOTS) != HAL_OK) {
        Error_Handler();
    }
    /* Set the frame synchronization polarity to active-low (falling edge). The STM32G4 HAL function
     * hardcodes it to active-high (rising edge) when the protocol is SAI_I2S_LSBJUSTIFIED, hence the
     * need to override it this way if active-low is desired.
     */
    __HAL_SAI_DISABLE(&hsai_block_b1);
    hsai_block_b1.Instance->FRCR &= ~SAI_xFRCR_FSPOL;

    if (sai_bit_depth != EVK_SAI_BIT_DEPTH_16BITS) {
        /* If not aligned on half word, align on word */
        hdma_sai1_a.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
        hdma_sai1_a.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
        hdma_sai1_b.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
        hdma_sai1_b.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    }
}

/** @brief Initialize the expansion port's SAI DMA's NVIC.
 */
static void init_audio_sai_nvic(void)
{
    HAL_NVIC_SetPriority(NVIC_SAI_DMA_RX_CPLT, PRIO_AUDIO_SAI_DMA_RX_IRQ, PRIO_AUDIO_SAI_DMA_RX_IRQ);
    HAL_NVIC_EnableIRQ(NVIC_SAI_DMA_RX_CPLT);

    HAL_NVIC_SetPriority(NVIC_SAI_DMA_TX_CPLT, PRIO_AUDIO_SAI_DMA_TX_IRQ, PRIO_AUDIO_SAI_DMA_TX_IRQ);
    HAL_NVIC_EnableIRQ(NVIC_SAI_DMA_TX_CPLT);
}

/** @brief Initialize the DMA controller for the expansion port's SAI peripheral.
 *
 *  Initialize the SAI into Master Tx and Master Rx.
 */
static void init_audio_sai_dma(void)
{
    __HAL_RCC_DMA2_CLK_ENABLE();
    if (HAL_DMA_Init(&hdma_sai1_a) != HAL_OK) {
        Error_Handler();
    }
    __HAL_LINKDMA(&hsai_block_a1, hdmarx, hdma_sai1_a);
    hdma_sai1_a.XferCpltCallback = sai_dma_rx_complete_callback;

    if (HAL_DMA_Init(&hdma_sai1_b) != HAL_OK) {
        Error_Handler();
    }
    __HAL_LINKDMA(&hsai_block_b1, hdmatx, hdma_sai1_b);
    hdma_sai1_b.XferCpltCallback = sai_dma_tx_complete_callback;
}

/** @brief Initialized the audio I2C clock.
 */
static void init_audio_i2c_clock(void)
{
    __HAL_RCC_I2C1_CLK_ENABLE();
}

/** @brief Initialized the audio I2C GPIOs.
 */
static void init_audio_i2c_gpios(void)
{
    GPIO_InitTypeDef scl_i2c_config = {
        .Pin = AUDIO_I2C_SCL_PIN,
        .Mode = GPIO_MODE_AF_OD,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
        .Alternate = GPIO_AF4_I2C1
    };
    HAL_GPIO_Init(AUDIO_I2C_SCL_PORT, &scl_i2c_config);

    GPIO_InitTypeDef sda_i2c_config = {
        .Pin = AUDIO_I2C_SDA_PIN,
        .Mode = GPIO_MODE_AF_OD,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
        .Alternate = GPIO_AF4_I2C1
    };
    HAL_GPIO_Init(AUDIO_I2C_SDA_PORT, &sda_i2c_config);
}

/** @brief Initialized the audio I2C peripheral.
 */
static void init_audio_i2c_peripheral(void)
{
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }

    /* Configure Analogue filter */
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        Error_Handler();
    }
    /* Configure Digital filter */
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) {
        Error_Handler();
    }
}

/** @brief Start the DMA transfer to or from the SAI peripheral.
 *
 *  @param[in] hdma              Pointer of the DMA_handler of the SAI.
 *  @param[in] source_addr       The source memory Buffer address.
 *  @param[in] destination_addr  The destination memory Buffer address.
 *  @param[in] sample_count      The number of audio samples.
 *  @param[in] direction         From the Memory to Peripheral or from the Peripheral to the Memory.
 */
static void sai_dma_start_it(DMA_HandleTypeDef *hdma, uint32_t source_addr, uint32_t destination_addr, uint32_t sample_count,
                             sai_direction_t direction)
{
    /* Disable the peripheral */
    __HAL_DMA_DISABLE(hdma);

    /* Clear the DMAMUX synchro overrun flag */
    hdma->DMAmuxChannelStatus->CFR = hdma->DMAmuxChannelStatusMask;

    if (hdma->DMAmuxRequestGen != 0U) {
        /* Clear the DMAMUX request generator overrun flag */
        hdma->DMAmuxRequestGenStatus->RGCFR = hdma->DMAmuxRequestGenStatusMask;
    }
    /* Clear all flags */
    hdma->DmaBaseAddress->IFCR = (DMA_ISR_GIF1 << (hdma->ChannelIndex & 0x1FU));

    /* Configure DMA Channel data length */
    hdma->Instance->CNDTR = sample_count;

    if (direction == SAI_MCU_TO_CODEC) {
        /* Configure DMA Channel destination address */
        hdma->Instance->CPAR = destination_addr;

        /* Configure DMA Channel source address */
        hdma->Instance->CMAR = source_addr;

    } else {
        /* Configure DMA Channel source address */
        hdma->Instance->CPAR = source_addr;

        /* Configure DMA Channel destination address */
        hdma->Instance->CMAR = destination_addr;
    }

    /* Enable transfert complete and Transfert error */
    __HAL_DMA_ENABLE_IT(hdma, DMA_IT_TC);

    /* Check if DMAMUX Synchronization is enabled */
    if ((hdma->DMAmuxChannel->CCR & DMAMUX_CxCR_SE) != 0U) {
        /* Enable DMAMUX sync overrun IT*/
        hdma->DMAmuxChannel->CCR |= DMAMUX_CxCR_SOIE;
    }

    /* If using DMAMUX request generator, enable the DMAMUX request generator overrun IT */
    if (hdma->DMAmuxRequestGen != 0U) {
        hdma->DMAmuxRequestGen->RGCR |= DMAMUX_RGxCR_OIE;
    }

    /* Enable the Peripheral */
    __HAL_DMA_ENABLE(hdma);
}

/** @brief SAI DMA TX complete callback implementation.
 */
static void sai_dma_tx_complete_callback(DMA_HandleTypeDef *hdma)
{
    /* Disable SAI Tx DMA Request */
    hdma->Instance->CCR &= (uint32_t)(~SAI_xCR1_DMAEN);
    sai_dma_tx_irq_callback();
}

/** @brief SAI DMA RX complete callback implementation.
 */
static void sai_dma_rx_complete_callback(DMA_HandleTypeDef *hdma)
{
    /* Disable SAI Rx DMA Request */
    hdma->Instance->CCR &= (uint32_t)(~SAI_xCR1_DMAEN);
    sai_dma_rx_irq_callback();
}

/** @brief Convert a SAI block size value to bytes.
 *
 *  @param[in] sai_block_size  This parameter can be a value of SAI_Block_Data_Size
 *                             defined in stm32g4xx_hal_sai.h.
 *  @return Size in bytes, -1 if error.
 */
static int8_t sai_block_size_to_bytes(uint32_t sai_block_size)
{
    switch (sai_block_size) {
    case SAI_DATASIZE_8:
        return 1;
    case SAI_DATASIZE_16:
        return 2;
    case SAI_DATASIZE_24:
    case SAI_DATASIZE_32:
        return 4;
    case SAI_DATASIZE_10:
    case SAI_DATASIZE_20:
    default:
        return -1;
    }
}
