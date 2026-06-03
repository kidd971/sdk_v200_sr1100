/** @file  audio_unidirectional_i2s_backend.c
 *  @brief Implement audio unidrectional facade prototype functions for I2S audio.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "audio_unidirectional_facade.h"
#include "quasar.h"
#include "sac_cfg.h"
#if !defined(AV_IND)
#include "max98091.h"
#endif

/* TYPES **********************************************************************/
/** @brief SAI configuration structure.
 */
typedef struct sai_cfg {
    uint8_t                bit_depth;
    uint8_t                tx_nb_ch;
    uint8_t                rx_nb_ch;
    quasar_sai_mode_t      sai_mode;
    quasar_sai_frequency_t sai_frequency;
} sai_cfg_t;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void configure_sai(sai_cfg_t sai_cfg);
#if !defined(AV_IND)
static void codec_i2c_write(uint8_t dev_addr, uint8_t mem_addr, uint8_t data);
static void codec_i2c_read(uint8_t dev_addr, uint8_t mem_addr, uint8_t *data);
static void configure_max98091(bool input_enabled, bool output_enabled);
#endif

/* PRIVATE GLOBALS ************************************************************/
#ifdef SAI_CLKGEN_TEST
static int32_t s_sine_dma_buf[48 * 2];

static const int16_t s_sine_1khz_48k[48] = {
    0,      4276,   8480,   12539,  16383,  19947,  23169,  25995,
    28377,  30272,  31650,  32486,  32767,  32486,  31650,  30272,
    28377,  25995,  23169,  19947,  16383,  12539,  8480,   4276,
    0,      -4276,  -8480,  -12539, -16383, -19947, -23169, -25995,
    -28377, -30272, -31650, -32486, -32767, -32486, -31650, -30272,
    -28377, -25995, -23169, -19947, -16383, -12539, -8480,  -4276,
};

static void sine_tx_dma_complete(void)
{
    quasar_audio_sai_write_non_blocking((uint8_t *)s_sine_dma_buf, 96);
}
#endif

#if !defined(AV_IND)
static max98091_i2c_hal_t codec_hal = {
    .i2c_addr = MAX98091A_I2C_ADDR,
    .read = codec_i2c_read,
    .write = codec_i2c_write,
};
#endif
/* I2S_FMT_DEFAULT: 0=RJF (default), 1=LJF, 2=STD — controlled via cmake -DI2S_FMT_DEFAULT=N */
#if defined(I2S_FMT_DEFAULT) && (I2S_FMT_DEFAULT) == 1
static quasar_sai_protocol_t s_sai_protocol = QUASAR_SAI_PROTOCOL_I2S_MSBJUSTIFIED;
#elif defined(I2S_FMT_DEFAULT) && (I2S_FMT_DEFAULT) == 2
static quasar_sai_protocol_t s_sai_protocol = QUASAR_SAI_PROTOCOL_I2S_STANDARD;
#else
static quasar_sai_protocol_t s_sai_protocol = QUASAR_SAI_PROTOCOL_I2S_LSBJUSTIFIED;
#endif

/* PUBLIC FUNCTIONS ***********************************************************/
void facade_audio_coord_init(void)
{
    sai_cfg_t sai_cfg = {
        .bit_depth     = I2S_BIT_DEPTH,
        .rx_nb_ch      = MAIN_CHANNEL_CHANNEL_COUNT,
#if defined(I2S_MASTER_MODE) && (I2S_MASTER_MODE)
        .sai_mode      = QUASAR_SAI_MASTER_MODE,
        .sai_frequency = QUASAR_SAI_AUDIO_FREQUENCY_48K,
#else
        .sai_mode      = QUASAR_SAI_SLAVE_MODE_MCLK,
#endif
    };

#if defined(AV_IND)
    /* AV IND board: no codec, route I2S to external expansion port. */
    configure_sai(sai_cfg);
    quasar_audio_set_i2s_mux_selection(QUASAR_SELECT_EXT_CODEC);
#else
    /* SPARK EVK: init on-board MAX98091 codec. */
    quasar_audio_init_i2c();
    max98091_reset_codec(&codec_hal);
    quasar_timer_delay_ms(1);
    configure_sai(sai_cfg);
    configure_max98091(true, false);
#endif
}

void facade_audio_node_init(void)
{
    sai_cfg_t sai_cfg = {
        .bit_depth     = I2S_BIT_DEPTH,
        .tx_nb_ch      = MAIN_CHANNEL_CHANNEL_COUNT,
#if defined(I2S_MASTER_MODE) && (I2S_MASTER_MODE)
        .sai_mode      = QUASAR_SAI_MASTER_MODE,
        .sai_frequency = QUASAR_SAI_AUDIO_FREQUENCY_48K,
#else
        .sai_mode      = QUASAR_SAI_SLAVE_MODE_MCLK,
#endif
    };

#if defined(AV_IND)
    /* AV IND board: no codec, route I2S to external expansion port. */
    configure_sai(sai_cfg);
    quasar_audio_set_i2s_mux_selection(QUASAR_SELECT_EXT_CODEC);
#else
    /* SPARK EVK: init on-board MAX98091 codec. */
    quasar_audio_init_i2c();
    max98091_reset_codec(&codec_hal);
    quasar_timer_delay_ms(1);
    configure_sai(sai_cfg);
    configure_max98091(false, true);
#endif

#ifdef SAI_CLKGEN_TEST
    for (int i = 0; i < 48; i++) {
        int32_t s = (int32_t)s_sine_1khz_48k[i] << 8;
        s_sine_dma_buf[i * 2]     = s;
        s_sine_dma_buf[i * 2 + 1] = s;
    }
    quasar_audio_set_sai_tx_dma_cplt_callback(sine_tx_dma_complete);
    quasar_audio_sai_write_non_blocking((uint8_t *)s_sine_dma_buf, 96);
#endif
}

void facade_audio_deinit(void)
{
    quasar_bsp_status_t quasar_err = QUASAR_OK;

    quasar_audio_deinit_sai(&quasar_err);
    ASSERT_QUASAR_BSP_STATUS(quasar_err);

#if !defined(AV_IND)
    max98091_reset_codec(&codec_hal);
#endif
}

void facade_set_audio_complete_callback(void (*tx_callback)(void), void (*rx_callback)(void))
{
    quasar_audio_set_sai_tx_dma_cplt_callback(tx_callback);
    quasar_audio_set_sai_rx_dma_cplt_callback(rx_callback);
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Configure the SAI peripheral.
 *
 *  @param[in] sai_cfg  SAI configuration structure.
 */
static void configure_sai(sai_cfg_t sai_cfg)
{
    quasar_bsp_status_t quasar_err = QUASAR_OK;

    quasar_sai_config_t sai_config = {
        .sai_mode            = sai_cfg.sai_mode,
        .sai_audio_frequency = sai_cfg.sai_frequency,
        .sai_protocol        = s_sai_protocol,
    };

    /* Configure SAI bit depth. */
    switch (sai_cfg.bit_depth) {
    case 16:
        sai_config.sai_bit_depth = QUASAR_SAI_BIT_DEPTH_16BITS;
        break;
    case 18:
    case 20:
    case 24:
        sai_config.sai_bit_depth = QUASAR_SAI_BIT_DEPTH_24BITS;
        break;
    case 32:
        sai_config.sai_bit_depth = QUASAR_SAI_BIT_DEPTH_32BITS;
        break;
    default:
        facade_print_error_string("Invalid quasar SAI bit depth...\r\n");
        while (1);
    }

    /* Configure SAI rx mono/stereo mode.
     * Note: In mono mode, the SAI will read only from the left channel to create a mono stream.
     */
    switch (sai_cfg.rx_nb_ch) {
    case 0:
        /* RX stream is disabled. */
        break;
    case 1:
        sai_config.rx_sai_mono_stereo = QUASAR_SAI_MODE_MONO;
        break;
    case 2:
        sai_config.rx_sai_mono_stereo = QUASAR_SAI_MODE_STEREO;
        break;
    default:
        facade_print_error_string("Invalid quasar SAI rx mono/stereo mode...\r\n");
        while (1);
    }

    /* Configure SAI tx mono/stereo mode.
     * Note: In mono mode, the SAI will write the mono stream to both left channel and right channel.
     */
    switch (sai_cfg.tx_nb_ch) {
    case 0:
        /* TX stream is disabled. */
        break;
    case 1:
        sai_config.tx_sai_mono_stereo = QUASAR_SAI_MODE_MONO;
        break;
    case 2:
        sai_config.tx_sai_mono_stereo = QUASAR_SAI_MODE_STEREO;
        break;
    default:
        facade_print_error_string("Invalid quasar SAI tx mono/stereo mode...\r\n");
        while (1);
    }

    /* Initialize the SAI peripheral. */
    quasar_audio_init_sai(sai_config, &quasar_err);
    ASSERT_QUASAR_BSP_STATUS(quasar_err);
}

#if !defined(AV_IND)
/** @brief Wrapper for I2C write to match MAX98091 driver expected signature.
 *
 *  @param[in] dev_addr  I2C device address.
 *  @param[in] mem_addr  Memory/register address.
 *  @param[in] data      1 Byte size data to be send.
 */
static void codec_i2c_write(uint8_t dev_addr, uint8_t mem_addr, uint8_t data)
{
    quasar_bsp_status_t quasar_err = QUASAR_OK;

    quasar_audio_i2c_write_byte_blocking(dev_addr, mem_addr, data, &quasar_err);
    ASSERT_QUASAR_BSP_STATUS(quasar_err);
}

/** @brief Wrapper for I2C read to match MAX98091 driver expected signature.
 *
 *  @param[in]  dev_addr  I2C device address.
 *  @param[in]  mem_addr  Memory/register address.
 *  @param[out] data      Pointer to store read data.
 */
static void codec_i2c_read(uint8_t dev_addr, uint8_t mem_addr, uint8_t *data)
{
    quasar_bsp_status_t quasar_err = QUASAR_OK;

    quasar_audio_i2c_read_byte_blocking(dev_addr, mem_addr, data, &quasar_err);
    ASSERT_QUASAR_BSP_STATUS(quasar_err);
}

/** @brief Configure the MAX98091 codec.
 *
 *  @param[in] input_enabled   True if the codec input is enabled.
 *  @param[in] output_enabled  True if the codec output is enabled.
 */
static void configure_max98091(bool input_enabled, bool output_enabled)
{
    max98091_codec_cfg_t cfg = {
        .record_enabled = input_enabled,
        .playback_enabled = output_enabled,
        .record_filter_enabled = false,
        .playback_filter_enabled = false,
    };

    /* Configure Codec sampling rate. */
    switch (I2S_SAMPLE_RATE_HZ) {
    case 96000:
        cfg.sampling_rate = MAX98091_AUDIO_96KHZ;
        break;
    case 48000:
        cfg.sampling_rate = MAX98091_AUDIO_48KHZ;
        break;
    case 44100:
        cfg.sampling_rate = MAX98091_AUDIO_44_1KHZ;
        break;
    case 32000:
        cfg.sampling_rate = MAX98091_AUDIO_32KHZ;
        break;
    case 24000:
        cfg.sampling_rate = MAX98091_AUDIO_24KHZ;
        break;
    case 16000:
        cfg.sampling_rate = MAX98091_AUDIO_16KHZ;
        break;
    case 12000:
        cfg.sampling_rate = MAX98091_AUDIO_12KHZ;
        break;
    case 8000:
        cfg.sampling_rate = MAX98091_AUDIO_8KHZ;
        break;
    default:
        facade_print_error_string("Invalid max98091 sample rate...\r\n");
        while (1);
    }

    /* Configure Codec word size. */
    switch (I2S_BIT_DEPTH) {
    case 16:
        cfg.word_size = MAX98091_AUDIO_16BITS;
        break;
    case 18:
    case 20:
        cfg.word_size = MAX98091_AUDIO_20BITS;
        break;
    case 24:
        cfg.word_size = MAX98091_AUDIO_24BITS;
        break;
    case 32:
        cfg.word_size = MAX98091_AUDIO_32BITS;
        break;
    default:
        facade_print_error_string("Invalid max98091 bit depth...\r\n");
        while (1);
    }

    max98091_init(&codec_hal, &cfg);
}
#endif /* !AV_IND */
