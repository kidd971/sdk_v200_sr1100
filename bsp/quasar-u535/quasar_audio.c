/** @file  quasar_audio.c
 *  @brief Initialize audio related peripherals.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_audio.h"
#include "quasar_clock.h"
#include "quasar_gpio.h"
#include "quasar_i2c.h"
#include "quasar_it.h"

/* CONSTANTS ******************************************************************/
/*! Number of retry for the codec's I2C communication. */
#define AUDIO_I2C_RETRY_COUNT 1000
/*! Number of slots to use for I2S. */
#define I2S_NB_OF_SLOTS 2
/*! Max audio frequency supported by the SAI peripheral. */
#define SAI_MAX_AUDIO_FREQUENCY QUASAR_SAI_AUDIO_FREQUENCY_96K

/* PRIVATE GLOBALS ************************************************************/
static void (*sai_dma_rx_irq_callback)(void);
static void (*sai_dma_tx_irq_callback)(void);

SAI_HandleTypeDef hsai_tx = {
    .Instance = SAI1_Block_A,
    .Init.Synchro = SAI_ASYNCHRONOUS,
    .Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLED,
    .Init.NoDivider = SAI_MASTERDIVIDER_DISABLE,
    .Init.SynchroExt = SAI_SYNCEXT_DISABLE,
    .Init.FIFOThreshold = SAI_FIFOTHRESHOLD_FULL,
    .Init.CompandingMode = SAI_NOCOMPANDING,
    .Init.TriState = SAI_OUTPUT_NOTRELEASED,
};
SAI_HandleTypeDef hsai_rx = {
    .Instance = SAI1_Block_B,
    .Init.AudioMode = SAI_MODESLAVE_RX,
    .Init.Synchro = SAI_SYNCHRONOUS,
    .Init.OutputDrive = SAI_OUTPUTDRIVE_DISABLED,
    .Init.FIFOThreshold = SAI_FIFOTHRESHOLD_EMPTY,
    .Init.CompandingMode = SAI_NOCOMPANDING,
    .Init.TriState = SAI_OUTPUT_NOTRELEASED,
};
DMA_HandleTypeDef hdma_sai_tx = {
    .Instance = QUASAR_DEF_GPDMA1_CHANNEL_SAI_TX,
    .Init.Request = GPDMA1_REQUEST_SAI1_A,
    .Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST,
    .Init.Direction = DMA_MEMORY_TO_PERIPH,
    .Init.SrcInc = DMA_SINC_INCREMENTED, /* Memory */
    .Init.DestInc = DMA_DINC_FIXED,      /* SAI periph */
    .Init.SrcDataWidth = DMA_SRC_DATAWIDTH_HALFWORD,
    .Init.DestDataWidth = DMA_DEST_DATAWIDTH_HALFWORD,
    .Init.Priority = DMA_HIGH_PRIORITY,
    .Init.SrcBurstLength = 1,
    .Init.DestBurstLength = 1,
    .Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0,
    .Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER,
    .Init.Mode = DMA_NORMAL,
};
DMA_HandleTypeDef hdma_sai_rx = {
    .Instance = QUASAR_DEF_GPDMA1_CHANNEL_SAI_RX,
    .Init.Request = GPDMA1_REQUEST_SAI1_B,
    .Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST,
    .Init.Direction = DMA_PERIPH_TO_MEMORY,
    .Init.SrcInc = DMA_SINC_FIXED,        /* SAI periph */
    .Init.DestInc = DMA_DINC_INCREMENTED, /* Memory */
    .Init.SrcDataWidth = DMA_SRC_DATAWIDTH_HALFWORD,
    .Init.DestDataWidth = DMA_DEST_DATAWIDTH_HALFWORD,
    .Init.Priority = DMA_HIGH_PRIORITY,
    .Init.SrcBurstLength = 1,
    .Init.DestBurstLength = 1,
    .Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0,
    .Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER,
    .Init.Mode = DMA_NORMAL,
};

/* Enables retrieval of all the necessary GPIOs for the audio SAI during initialization and deinitialization. */
typedef struct audio_sai_gpios_config {
    quasar_gpio_config_t gpio_mclk_config;
    quasar_gpio_config_t gpio_sck_config;
    quasar_gpio_config_t gpio_fs_config;
    quasar_gpio_config_t gpio_sda_config;
    quasar_gpio_config_t gpio_sdb_config;
} audio_sai_gpios_config_t;

/* PRIVATE FUNCTIONS PROTOTYPES ***********************************************/
static void audio_init_sai_clocks(quasar_bsp_status_t *err);
static void configure_sai_rx_monostereo_mode(quasar_sai_mono_stereo_t mode);
static void configure_sai_tx_monostereo_mode(quasar_sai_mono_stereo_t mode);
static void audio_init_sai_nvic(void);
static void audio_deinit_sai_nvic(void);
static void audio_init_sai_dma(quasar_bsp_status_t *err);
static void audio_deinit_sai_dma(quasar_bsp_status_t *err);
static void sai_dma_start_it(DMA_HandleTypeDef *hdma, uint32_t source_addr, uint32_t destination_addr, uint32_t size);
static void sai_dma_tx_complete_callback(DMA_HandleTypeDef *hdma);
static void sai_dma_rx_complete_callback(DMA_HandleTypeDef *hdma);
static quasar_i2c_config_t audio_get_i2c_config(void);
static audio_sai_gpios_config_t audio_get_sai_gpios_config(void);
// audio_init_codec_mux_gpio: not needed, PC2 is NC, I2S goes directly to expansion port.

/* PUBLIC FUNCTIONS ***********************************************************/
void quasar_audio_init_sai(quasar_sai_config_t sai_config, quasar_bsp_status_t *err)
{
    *err = QUASAR_OK;

    /* U535: PC2 (MUX_SEL) is NC — I2S goes directly to expansion port, no MUX init needed. */

    /* Validate bit depth. */
    switch (sai_config.sai_bit_depth) {
    case QUASAR_SAI_BIT_DEPTH_16BITS:
    case QUASAR_SAI_BIT_DEPTH_16BITS_EXT:
    case QUASAR_SAI_BIT_DEPTH_24BITS:
    case QUASAR_SAI_BIT_DEPTH_32BITS:
        break;
    default:
        /* Invalid audio bit depth. */
        QUASAR_BSP_CHECK_ERROR(true, err, QUASAR_ERR_AUDIO_INVALID_BIT_DEPTH, return);
    }

    /* Configure hsai_tx. */
    if (sai_config.sai_mode == QUASAR_SAI_MASTER_MODE) {
        switch (sai_config.sai_audio_frequency) {
        case QUASAR_SAI_AUDIO_FREQUENCY_96K:
        case QUASAR_SAI_AUDIO_FREQUENCY_48K:
        case QUASAR_SAI_AUDIO_FREQUENCY_32K:
        case QUASAR_SAI_AUDIO_FREQUENCY_16K:
        case QUASAR_SAI_AUDIO_FREQUENCY_8K:
            break;
        default:
            /* Invalid audio frequency for master configuration. */
            QUASAR_BSP_CHECK_ERROR(true, err, QUASAR_ERR_AUDIO_INVALID_FREQUENCY, return);
        }

        /* Initialize audio frequency. */
        hsai_tx.Init.AudioFrequency = sai_config.sai_audio_frequency;
        /* Initialize audio mode. */
        hsai_tx.Init.AudioMode = SAI_MODEMASTER_TX;
        hsai_tx.Init.MckOutput = SAI_MCK_OUTPUT_ENABLE;
    } else {
        /* Initialize MCK frequency to SAI_CLK. */
        hsai_tx.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_MCKDIV;
        hsai_tx.Init.Mckdiv = 0;
        /* Initialize audio mode. */
        hsai_tx.Init.AudioMode = SAI_MODESLAVE_TX;
        if (sai_config.sai_mode == QUASAR_SAI_SLAVE_MODE_MCLK) {
            hsai_tx.Init.MckOutput = SAI_MCK_OUTPUT_ENABLE;
        } else {
            hsai_tx.Init.MckOutput = SAI_MCK_OUTPUT_DISABLE;
        }
    }

    /* Get the configurations of the GPIOs associated with the SAI for audio. */
    audio_sai_gpios_config_t sai_gpios_config = audio_get_sai_gpios_config();

    /* Initialize each GPIO previously retrieved. */
    quasar_gpio_init(sai_gpios_config.gpio_mclk_config);
    quasar_gpio_init(sai_gpios_config.gpio_sck_config);
    quasar_gpio_init(sai_gpios_config.gpio_fs_config);
    quasar_gpio_init(sai_gpios_config.gpio_sda_config);
    quasar_gpio_init(sai_gpios_config.gpio_sdb_config);

    /* Initialize the clocks for the SAI. */
    audio_init_sai_clocks(err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);
    /* Configure SAI modes and initialize SAI peripherals. */
    configure_sai_tx_monostereo_mode(sai_config.tx_sai_mono_stereo);
    configure_sai_rx_monostereo_mode(sai_config.rx_sai_mono_stereo);

    /* Initialize hal SAI protocol layer. */
    QUASAR_BSP_CHECK_ERROR(HAL_SAI_InitProtocol(&hsai_tx, sai_config.sai_protocol, sai_config.sai_bit_depth,
                                                I2S_NB_OF_SLOTS) != HAL_OK,
                           err, QUASAR_ERR_AUDIO_SAI_TX_INIT, return);
    QUASAR_BSP_CHECK_ERROR(HAL_SAI_InitProtocol(&hsai_rx, sai_config.sai_protocol, sai_config.sai_bit_depth,
                                                I2S_NB_OF_SLOTS) != HAL_OK,
                           err, QUASAR_ERR_AUDIO_SAI_RX_INIT, return);

    /* Adapt DMA data with based on the SAI bit depth. Bit depth between 16-bit and 32-bit will be transported using
     * 32-bit words by the DMA. Data will be padded with zeros.
     */
    if (sai_config.sai_bit_depth != QUASAR_SAI_BIT_DEPTH_16BITS) {
        /* If not aligned on half word, align on word */
        hdma_sai_tx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_WORD;
        hdma_sai_tx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_WORD;
        hdma_sai_rx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_WORD;
        hdma_sai_rx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_WORD;
    }

    if (sai_config.sai_mode == QUASAR_SAI_MASTER_MODE) {
        /* Start master tx peripheral to activate clocks. */
        __HAL_SAI_ENABLE(&hsai_tx);
        /* Dummy data transfer to activate clocks. */
        hsai_tx.Instance->DR = 0;
        /* Lock RX to the first FS frame to prevent L/R inversion at boot. */
        __HAL_SAI_ENABLE(&hsai_rx);
    } else {
        /* Set the frame synchronization polarity to active-low (falling edge). The STM32U5 HAL function
         * hardcodes it to active-high (rising edge) when the protocol is SAI_I2S_LSBJUSTIFIED, hence the
         * need to override it this way if active-low is desired. This fixes an issue where the LR channels
         * are randomly inversed at each boot.
         */
        __HAL_SAI_DISABLE(&hsai_tx);
        hsai_tx.Instance->FRCR &= ~SAI_xFRCR_FSPOL;
        __HAL_SAI_DISABLE(&hsai_rx);
        hsai_rx.Instance->FRCR &= ~SAI_xFRCR_FSPOL;
    }

    /* Initialize the IRQ priorities and enable them in the NVIC. */
    audio_init_sai_nvic();
    /* Initialize the SAI DMA. */
    audio_init_sai_dma(err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);

    /* Temporarily set the GPIOs to input while programming their functionalities. */
    quasar_gpio_config_t linein_detect = {
        .port = QUASAR_DEF_LINEIN_DETECT_PORT,
        .pin = QUASAR_DEF_LINEIN_DETECT_PIN,
        .mode = QUASAR_GPIO_MODE_INPUT,
        .type = QUASAR_GPIO_TYPE_NONE,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(linein_detect);
    quasar_gpio_config_t audio_irq = {
        .port = QUASAR_DEF_AUDIO_IRQ_PORT,
        .pin = QUASAR_DEF_AUDIO_IRQ_PIN,
        .mode = QUASAR_GPIO_MODE_INPUT,
        .type = QUASAR_GPIO_TYPE_NONE,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate = QUASAR_GPIO_ALTERNATE_NONE,
    };
    quasar_gpio_init(audio_irq);
}

void quasar_audio_set_i2s_mux_selection(quasar_i2s_mux_select_t mux_select)
{
    (void)mux_select;
    /* U535: PC2 (MUX_SEL) is NC — I2S goes directly to expansion port pins. */
}

void quasar_audio_deinit_sai(quasar_bsp_status_t *err)
{
    *err = QUASAR_OK;

    audio_sai_gpios_config_t sai_gpios_config = audio_get_sai_gpios_config();

    audio_deinit_sai_dma(err);
    QUASAR_BSP_CHECK_ERROR(*err != QUASAR_OK, err, *err, return);

    audio_deinit_sai_nvic();

    QUASAR_BSP_CHECK_ERROR(HAL_SAI_DeInit(&hsai_tx) != HAL_OK, err, QUASAR_ERR_AUDIO_SAI_TX_DEINIT, return);

    QUASAR_BSP_CHECK_ERROR(HAL_SAI_DeInit(&hsai_rx) != HAL_OK, err, QUASAR_ERR_AUDIO_SAI_RX_DEINIT, return);

    quasar_gpio_deinit(sai_gpios_config.gpio_mclk_config.port, sai_gpios_config.gpio_mclk_config.pin);
    quasar_gpio_deinit(sai_gpios_config.gpio_sck_config.port, sai_gpios_config.gpio_sck_config.pin);
    quasar_gpio_deinit(sai_gpios_config.gpio_fs_config.port, sai_gpios_config.gpio_fs_config.pin);
    quasar_gpio_deinit(sai_gpios_config.gpio_sda_config.port, sai_gpios_config.gpio_sda_config.pin);
    quasar_gpio_deinit(sai_gpios_config.gpio_sdb_config.port, sai_gpios_config.gpio_sdb_config.pin);

    __HAL_RCC_SAI1_CLK_DISABLE();
}

void quasar_audio_init_i2c(void)
{
    /* U535 has no on-board codec — I2C codec interface not used. */
}

void quasar_audio_deinit_i2c(void)
{
    /* U535 has no on-board codec — I2C codec interface not used. */
}

void quasar_audio_i2c_write_byte_blocking(uint8_t dev_addr, uint8_t mem_addr, uint8_t data, quasar_bsp_status_t *err)
{
    /* U535 has no on-board codec — silently succeed so upper layers do not assert. */
    (void)dev_addr;
    (void)mem_addr;
    (void)data;
    *err = QUASAR_OK;
}

void quasar_audio_i2c_read_byte_blocking(uint8_t dev_addr, uint8_t mem_addr, uint8_t *data, quasar_bsp_status_t *err)
{
    /* U535 has no on-board codec — return zero and silently succeed. */
    (void)dev_addr;
    (void)mem_addr;
    if (data != NULL) *data = 0;
    *err = QUASAR_OK;
}

void quasar_audio_sai_write_non_blocking(uint8_t *data, uint16_t size)
{
    /* Check SAI error flags. */
    if (hsai_tx.Instance->SR & (SAI_xSR_LFSDET | SAI_xSR_AFSDET | SAI_xSR_OVRUDR)) {
        /* Error detected. */
        do {
            /* Disable SAI block and make sure it is fully disabled. */
            hsai_tx.Instance->CR1 &= ~SAI_xCR1_SAIEN;
        } while ((hsai_tx.Instance->CR1 & SAI_xCR1_SAIEN));
        /* Flush FIFO. */
        hsai_tx.Instance->CR2 |= SAI_xCR2_FFLUSH;
        /* Clear Error flags. */
        hsai_tx.Instance->CLRFR |= (SAI_xCLRFR_CLFSDET | SAI_xCLRFR_CAFSDET | SAI_xCLRFR_COVRUDR);
    }

    sai_dma_start_it(&hdma_sai_tx, (uint32_t)data, (uint32_t)&hsai_tx.Instance->DR, size);

    /* Enable SAI DMA Request */
    hsai_tx.Instance->CR1 |= SAI_xCR1_DMAEN;

    /* Enable SAI peripheral */
    __HAL_SAI_ENABLE(&hsai_tx);
}

void quasar_audio_sai_read_non_blocking(uint8_t *data, uint16_t size)
{
    /* Check SAI error flags. */
    if (hsai_rx.Instance->SR & (SAI_xSR_LFSDET | SAI_xSR_AFSDET | SAI_xSR_OVRUDR)) {
        if (hsai_tx.Init.AudioMode == SAI_MODEMASTER_TX) {
            /* Master: flush only — disabling RX breaks the FS phase lock set at init. */
            hsai_rx.Instance->CR2 |= SAI_xCR2_FFLUSH;
            hsai_rx.Instance->CLRFR |= (SAI_xCLRFR_CLFSDET | SAI_xCLRFR_CAFSDET | SAI_xCLRFR_COVRUDR);
        } else {
            /* Slave: disable → flush → re-enable. */
            do {
                hsai_rx.Instance->CR1 &= ~SAI_xCR1_SAIEN;
            } while ((hsai_rx.Instance->CR1 & SAI_xCR1_SAIEN));
            hsai_rx.Instance->CR2 |= SAI_xCR2_FFLUSH;
            hsai_rx.Instance->CLRFR |= (SAI_xCLRFR_CLFSDET | SAI_xCLRFR_CAFSDET | SAI_xCLRFR_COVRUDR);
        }
    }

    sai_dma_start_it(&hdma_sai_rx, (uint32_t)&hsai_rx.Instance->DR, (uint32_t)data, size);

    /* Enable SAI DMA Request */
    hsai_rx.Instance->CR1 |= SAI_xCR1_DMAEN;

    /* Enable SAI peripheral */
    __HAL_SAI_ENABLE(&hsai_rx);
}

void quasar_audio_set_sai_tx_dma_cplt_callback(void (*callback)(void))
{
    sai_dma_tx_irq_callback = callback;
}

void quasar_audio_set_sai_rx_dma_cplt_callback(void (*callback)(void))
{
    sai_dma_rx_irq_callback = callback;
}

void quasar_audio_sai_start_write_non_blocking(void)
{
    if (sai_dma_tx_irq_callback != NULL) {
        sai_dma_tx_irq_callback();
    }
}

void quasar_audio_sai_start_read_non_blocking(void)
{
    if (sai_dma_rx_irq_callback != NULL) {
        sai_dma_rx_irq_callback();
    }
}

void quasar_audio_sai_stop_write_non_blocking(void)
{
    if (hsai_tx.hdmatx->Instance->CCR & (DMA_IT_TC | DMA_IT_DTE)) {
        __HAL_DMA_DISABLE(&hdma_sai_tx);
    }
}

void quasar_audio_sai_stop_read_non_blocking(void)
{
    if (hsai_rx.hdmarx->Instance->CCR & (DMA_IT_TC | DMA_IT_DTE)) {
        __HAL_DMA_DISABLE(&hdma_sai_rx);
    }
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Initialize the SAI clock.
 *
 *  The HSE is at 16 MHz. The clock source of SAI peripheral is PLL2 of the
 *  clock tree, and in order to achieve a frequency of 12.288 MHz for the codec,
 *  a PLL fractional multiplier is used.
 */
static void audio_init_sai_clocks(quasar_bsp_status_t *err)
{
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    __HAL_RCC_SAI1_CLK_ENABLE();

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_SAI1;
    PeriphClkInit.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLL2;
    PeriphClkInit.PLL2.PLL2Source = RCC_PLLSOURCE_HSE;
    PeriphClkInit.PLL2.PLL2M = 4;
    PeriphClkInit.PLL2.PLL2N = 107;
    PeriphClkInit.PLL2.PLL2P = 35;
    PeriphClkInit.PLL2.PLL2Q = 2;
    PeriphClkInit.PLL2.PLL2R = 2;
    PeriphClkInit.PLL2.PLL2RGE = RCC_PLLVCIRANGE_0;
    PeriphClkInit.PLL2.PLL2FRACN = QUASAR_PLL2_FRACN_DEFAULT_VALUE;
    PeriphClkInit.PLL2.PLL2ClockOut = RCC_PLL2_DIVP;
    QUASAR_BSP_CHECK_ERROR(HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK, err, QUASAR_ERR_AUDIO_SAI_CLOCK_CONFIG,
                           return);
}

/** @brief Set the SAI Rx to Mono or Stereo.
 *
 *  @param[in] mode  Select Mono or Stereo mode enum.
 */
static void configure_sai_rx_monostereo_mode(quasar_sai_mono_stereo_t mode)
{
    if (mode == QUASAR_SAI_MODE_MONO) {
        hsai_rx.Init.MonoStereoMode = SAI_MONOMODE;
    } else {
        hsai_rx.Init.MonoStereoMode = SAI_STEREOMODE;
    }
}

/** @brief Set the SAI Tx to Mono or Stereo.
 *
 *  @param[in] mode  Select Mono or Stereo mode enum.
 */
static void configure_sai_tx_monostereo_mode(quasar_sai_mono_stereo_t mode)
{
    if (mode == QUASAR_SAI_MODE_MONO) {
        hsai_tx.Init.MonoStereoMode = SAI_MONOMODE;
    } else {
        hsai_tx.Init.MonoStereoMode = SAI_STEREOMODE;
    }
}

/** @brief Initialize the SAI DMA's NVIC.
 */
static void audio_init_sai_nvic(void)
{
    HAL_NVIC_SetPriority(QUASAR_NVIC_SAI_DMA_TX_CPLT, QUASAR_DEF_PRIO_AUDIO_SAI_DMA_TX_IRQ, 0);
    HAL_NVIC_EnableIRQ(QUASAR_NVIC_SAI_DMA_TX_CPLT);

    HAL_NVIC_SetPriority(QUASAR_NVIC_SAI_DMA_RX_CPLT, QUASAR_DEF_PRIO_AUDIO_SAI_DMA_RX_IRQ, 0);
    HAL_NVIC_EnableIRQ(QUASAR_NVIC_SAI_DMA_RX_CPLT);
}

/** @brief Disable the SAI DMA's NVIC.
 */
static void audio_deinit_sai_nvic(void)
{
    HAL_NVIC_DisableIRQ(QUASAR_NVIC_SAI_DMA_TX_CPLT);
    HAL_NVIC_DisableIRQ(QUASAR_NVIC_SAI_DMA_RX_CPLT);
}

/** @brief Initialize the DMA controller for the SAI peripheral.
 *
 *  Initialize the SAI into Master Tx and Master Rx.
 */
static void audio_init_sai_dma(quasar_bsp_status_t *err)
{
    QUASAR_BSP_CHECK_ERROR(HAL_DMA_Init(&hdma_sai_tx) != HAL_OK, err, QUASAR_ERR_AUDIO_DMA_TX_INIT, return);
    __HAL_LINKDMA(&hsai_tx, hdmatx, hdma_sai_tx);
    hdma_sai_tx.XferCpltCallback = sai_dma_tx_complete_callback;

    QUASAR_BSP_CHECK_ERROR(HAL_DMA_Init(&hdma_sai_rx) != HAL_OK, err, QUASAR_ERR_AUDIO_DMA_RX_INIT, return);
    __HAL_LINKDMA(&hsai_rx, hdmarx, hdma_sai_rx);
    hdma_sai_rx.XferCpltCallback = sai_dma_rx_complete_callback;
}

/** @brief Deinitialize the DMA controller for the SAI peripheral.
 *
 *  Initialize the SAI into Master Tx and Master Rx.
 */
static void audio_deinit_sai_dma(quasar_bsp_status_t *err)
{
    /* Since the audio does not use the quasar_dma driver, HAL is utilized for deinitialization rather than the DMA
     * driver's function.
     */
    QUASAR_BSP_CHECK_ERROR(HAL_DMA_DeInit(&hdma_sai_tx) != HAL_OK, err, QUASAR_ERR_AUDIO_DMA_TX_DEINIT, return);

    QUASAR_BSP_CHECK_ERROR(HAL_DMA_DeInit(&hdma_sai_rx) != HAL_OK, err, QUASAR_ERR_AUDIO_DMA_RX_DEINIT, return);
}

/** @brief Start the DMA transfer to or from the SAI peripheral.
 *
 *  @param[in] hdma              Pointer of the DMA_handler of the SAI.
 *  @param[in] source_addr       The source memory Buffer address
 *  @param[in] destination_addr  The destination memory Buffer address
 *  @param[in] size              The number of bytes to transfer.
 */
static void sai_dma_start_it(DMA_HandleTypeDef *hdma, uint32_t source_addr, uint32_t destination_addr, uint32_t size)
{
    /* Disable the peripheral */
    __HAL_DMA_DISABLE(hdma);

    /* Configure DMA Channel data length */
    MODIFY_REG(hdma->Instance->CBR1, DMA_CBR1_BNDT, (size & DMA_CBR1_BNDT));

    /* Clear all interrupt flags */
    __HAL_DMA_CLEAR_FLAG(hdma, DMA_FLAG_TC | DMA_FLAG_HT | DMA_FLAG_DTE | DMA_FLAG_ULE | DMA_FLAG_USE | DMA_FLAG_SUSP |
                                   DMA_FLAG_TO);

    /* Configure DMA Channel destination address */
    hdma->Instance->CDAR = destination_addr;

    /* Configure DMA Channel source address */
    hdma->Instance->CSAR = source_addr;

    /* Enable the Half transfer complete interrupt as well */
    __HAL_DMA_ENABLE_IT(hdma, DMA_IT_TC);

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

/** @brief Retrieve the configuration of the I2C peripheral used for audio.
 *
 *  @return Configuration of the I2C peripheral.
 */
static quasar_i2c_config_t audio_get_i2c_config(void)
{
    quasar_gpio_config_t gpio_config_i2c_scl = {
        .port = QUASAR_DEF_AUDIO_I2C_SCL_PORT,
        .pin = QUASAR_DEF_AUDIO_I2C_SCL_PIN,
        .mode = QUASAR_GPIO_MODE_ALTERNATE,
        .type = QUASAR_GPIO_TYPE_OD,
        .pull = QUASAR_GPIO_PULL_UP,
        .speed = QUASAR_GPIO_SPEED_LOW,
        .alternate = QUASAR_GPIO_ALTERNATE_AF4,
    };
    quasar_gpio_config_t gpio_config_i2c_sda = {
        .port = QUASAR_DEF_AUDIO_I2C_SDA_PORT,
        .pin = QUASAR_DEF_AUDIO_I2C_SDA_PIN,
        .mode = QUASAR_GPIO_MODE_ALTERNATE,
        .type = QUASAR_GPIO_TYPE_OD,
        .pull = QUASAR_GPIO_PULL_UP,
        .speed = QUASAR_GPIO_SPEED_LOW,
        .alternate = QUASAR_GPIO_ALTERNATE_AF4,
    };

    quasar_i2c_config_t i2c_config = {
        .gpio_config_scl = gpio_config_i2c_scl,
        .gpio_config_sda = gpio_config_i2c_sda,
        .i2c_selection = QUASAR_DEF_I2C_SELECTION_CODEC,
        .irq_priority = QUASAR_IRQ_PRIORITY_NONE,
    };

    return i2c_config;
}

/** @brief Retrieve the configurations of the GPIOs used for the SAI peripheral in the audio system.
 *
 *  @return Configuration of the SAI GPIOs.
 */
static audio_sai_gpios_config_t audio_get_sai_gpios_config(void)
{
    quasar_gpio_config_t gpio_mclk_config = {
        .port = QUASAR_DEF_AUDIO_SAI_MCLK_PORT,
        .pin = QUASAR_DEF_AUDIO_SAI_MCLK_PIN,
        .mode = QUASAR_GPIO_MODE_ALTERNATE,
        .type = QUASAR_GPIO_TYPE_PP,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate = QUASAR_GPIO_ALTERNATE_AF13,
    };
    quasar_gpio_config_t gpio_sck_config = {
        .port = QUASAR_DEF_AUDIO_SAI_SCK_PORT,
        .pin = QUASAR_DEF_AUDIO_SAI_SCK_PIN,
        .mode = QUASAR_GPIO_MODE_ALTERNATE,
        .type = QUASAR_GPIO_TYPE_PP,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate = QUASAR_GPIO_ALTERNATE_AF13,
    };
    quasar_gpio_config_t gpio_fs_config = {
        .port = QUASAR_DEF_AUDIO_SAI_FS_PORT,
        .pin = QUASAR_DEF_AUDIO_SAI_FS_PIN,
        .mode = QUASAR_GPIO_MODE_ALTERNATE,
        .type = QUASAR_GPIO_TYPE_PP,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate = QUASAR_GPIO_ALTERNATE_AF13,
    };
    quasar_gpio_config_t gpio_sda_config = {
        .port = QUASAR_DEF_AUDIO_SAI_SD_A_PORT,
        .pin = QUASAR_DEF_AUDIO_SAI_SD_A_PIN,
        .mode = QUASAR_GPIO_MODE_ALTERNATE,
        .type = QUASAR_GPIO_TYPE_PP,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate = QUASAR_GPIO_ALTERNATE_AF13,
    };
    quasar_gpio_config_t gpio_sdb_config = {
        .port = QUASAR_DEF_AUDIO_SAI_SD_B_PORT,
        .pin = QUASAR_DEF_AUDIO_SAI_SD_B_PIN,
        .mode = QUASAR_GPIO_MODE_ALTERNATE,
        .type = QUASAR_GPIO_TYPE_PP,
        .pull = QUASAR_GPIO_PULL_NONE,
        .speed = QUASAR_GPIO_SPEED_VERY_HIGH,
        .alternate = QUASAR_GPIO_ALTERNATE_AF13,
    };
    audio_sai_gpios_config_t sai_gpios_config = {
        .gpio_mclk_config = gpio_mclk_config,
        .gpio_sck_config = gpio_sck_config,
        .gpio_fs_config = gpio_fs_config,
        .gpio_sda_config = gpio_sda_config,
        .gpio_sdb_config = gpio_sdb_config,
    };
    return sai_gpios_config;
}

/* audio_init_codec_mux_gpio: not implemented — PC2 (MUX_SEL) is NC on U535.
 * I2S signals go directly to the expansion port; codec requires a jumper. */
