/** @file  puretone_headset_i2s_backend.c
 *  @brief Implement puretone headset facade prototype functions for I2S audio.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "puretone_headset_facade.h"
#include "quasar.h"
#include "sac_cfg.h"
#if !NO_CODEC
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
#if !NO_CODEC
static void codec_i2c_write(uint8_t dev_addr, uint8_t mem_addr, uint8_t data);
static void codec_i2c_read(uint8_t dev_addr, uint8_t mem_addr, uint8_t *data);
static void configure_max98091(bool input_enabled, bool output_enabled);
#endif
static void configure_sai(sai_cfg_t sai_cfg);

/* PRIVATE GLOBALS ************************************************************/
#if !NO_CODEC
static max98091_i2c_hal_t codec_hal = {
    .i2c_addr = MAX98091A_I2C_ADDR,
    .read = codec_i2c_read,
    .write = codec_i2c_write,
};
#endif
static sai_cfg_t s_sai_cfg;
#if defined(I2S_FMT_DEFAULT) && (I2S_FMT_DEFAULT) == 1
static quasar_sai_protocol_t s_sai_protocol = QUASAR_SAI_PROTOCOL_I2S_MSBJUSTIFIED;
#elif defined(I2S_FMT_DEFAULT) && (I2S_FMT_DEFAULT) == 2
static quasar_sai_protocol_t s_sai_protocol = QUASAR_SAI_PROTOCOL_I2S_STANDARD;
#else
static quasar_sai_protocol_t s_sai_protocol = QUASAR_SAI_PROTOCOL_I2S_LSBJUSTIFIED;
#endif
static quasar_i2s_mux_select_t s_mux_select   = QUASAR_SELECT_ON_BOARD_CODEC;

/* PUBLIC FUNCTIONS ***********************************************************/
void facade_audio_coord_init(bool i2s_master_mode)
{
    sai_cfg_t sai_cfg = {
        .bit_depth     = I2S_BIT_DEPTH,
        .rx_nb_ch      = MAIN_CHANNEL_CHANNEL_COUNT,
        .tx_nb_ch      = BACK_CHANNEL_CHANNEL_COUNT,
        .sai_mode      = i2s_master_mode ? QUASAR_SAI_MASTER_MODE : QUASAR_SAI_SLAVE_MODE_MCLK,
        .sai_frequency = QUASAR_SAI_AUDIO_FREQUENCY_96K,
    };
    s_sai_cfg = sai_cfg;

#if !NO_CODEC
    /* Initialize the Codec's I2C interface. */
    quasar_audio_init_i2c();

    /* Reset codec before initializing the SAI. */
    max98091_reset_codec(&codec_hal);
    quasar_timer_delay_ms(1);
#endif

    /* Initialize the SAI peripheral. */
    configure_sai(sai_cfg);

#if NO_CODEC
    quasar_audio_set_i2s_mux_selection(QUASAR_SELECT_EXT_CODEC);
#else
    /* Configure the codec. */
    configure_max98091(true, true);
#endif
}

void facade_audio_node_init(bool i2s_master_mode)
{
    sai_cfg_t sai_cfg = {
        .bit_depth     = I2S_BIT_DEPTH,
        .rx_nb_ch      = BACK_CHANNEL_CHANNEL_COUNT,
        .tx_nb_ch      = MAIN_CHANNEL_CHANNEL_COUNT,
        .sai_mode      = i2s_master_mode ? QUASAR_SAI_MASTER_MODE : QUASAR_SAI_SLAVE_MODE_MCLK,
        .sai_frequency = QUASAR_SAI_AUDIO_FREQUENCY_96K,
    };
    s_sai_cfg = sai_cfg;

#if !NO_CODEC
    /* Initialize the Codec's I2C interface. */
    quasar_audio_init_i2c();

    /* Reset codec before initializing the SAI. */
    max98091_reset_codec(&codec_hal);
    quasar_timer_delay_ms(1);
#endif

    /* Initialize the SAI peripheral. */
    configure_sai(sai_cfg);

#if NO_CODEC
    quasar_audio_set_i2s_mux_selection(QUASAR_SELECT_EXT_CODEC);
#else
    /* Configure the codec. */
    configure_max98091(true, true);
#endif
}

void facade_audio_deinit(void)
{
    quasar_bsp_status_t quasar_err = QUASAR_OK;

    quasar_audio_deinit_sai(&quasar_err);
    ASSERT_QUASAR_BSP_STATUS(quasar_err);

#if !NO_CODEC
    max98091_reset_codec(&codec_hal);
#endif
}

void facade_set_audio_complete_callback(void (*tx_callback)(void), void (*rx_callback)(void))
{
    quasar_audio_set_sai_tx_dma_cplt_callback(tx_callback);
    quasar_audio_set_sai_rx_dma_cplt_callback(rx_callback);
}

void facade_i2s_backend_track_mux(quasar_i2s_mux_select_t select)
{
    s_mux_select = select;
}

/* PRIVATE FUNCTIONS **********************************************************/
#if !NO_CODEC
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
    /* Init codec and generate SCLK */
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
#endif /* !NO_CODEC */

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


