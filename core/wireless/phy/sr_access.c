/** @file  sr_access.c
 *  @brief Hardware access layer that links the MCU control functions (GPIO, SPI, and IRQ pins and handlers) with the
 *         radio_hal instance used by the wireless core to control the Spark radio peripheral.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "sr_access.h"

#ifdef __cplusplus
extern "C" {
#endif

/* PUBLIC GLOBALS *************************************************************/
radio_hal_t radio_hal[MAX_NUMBER_OF_RADIOS];

#if SR1100
#if !RADIO_QSPI_ENABLED
spi_xfer_t spi_xfer[WPS_RADIO_COUNT];
#else
qspi_transfers_t qspi_xfer[WPS_RADIO_COUNT];
#endif /* !RADIO_QSPI_ENABLED */
#endif /* SR1100 */

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
#if !RADIO_QSPI_ENABLED
static bool sr_access_radio_spi_link(
    uint8_t radio_id, void (*transfer_full_duplex_blocking)(uint8_t *tx_data, uint8_t *rx_data, uint16_t size),
    void (*transfer_full_duplex_non_blocking)(uint8_t *tx_data, uint8_t *rx_data, uint16_t size));
#else
static bool sr_access_radio_qspi_link(
    uint8_t radio_id, void (*transfer_half_duplex_rx_blocking)(uint8_t command, uint8_t *tx_data, uint16_t size),
    void (*transfer_half_duplex_rx_non_blocking)(uint8_t command, uint8_t *tx_data, uint16_t size),
    void (*transfer_half_duplex_tx_blocking)(uint8_t command, uint8_t *rx_data, uint16_t size),
    void (*transfer_half_duplex_tx_non_blocking)(uint8_t command, uint8_t *rx_data, uint16_t size),
    void (*set_access_mode_spi)(void), void (*set_access_mode_qspi)(void));
#endif

/* PUBLIC FUNCTIONS ***********************************************************/
bool sr_access_radio_gpio_link(uint8_t radio_id, void (*set_reset_pin)(void), void (*reset_reset_pin)(void))
{
    if ((radio_id > MAX_NUMBER_OF_RADIOS) || (set_reset_pin == NULL) || (reset_reset_pin == NULL)) {
        return false;
    }

    /* Link the GPIO functions. */
    radio_hal[radio_id].set_reset_pin = set_reset_pin;
    radio_hal[radio_id].reset_reset_pin = reset_reset_pin;

    return true;
}

bool sr_access_radio_comm_link(
    uint8_t radio_id, void (*end_transfer)(void), void (*begin_transfer)(void),
    void (*transfer_half_duplex_rx_blocking)(uint8_t command, uint8_t *tx_data, uint16_t size),
    void (*transfer_half_duplex_rx_non_blocking)(uint8_t command, uint8_t *tx_data, uint16_t size),
    void (*transfer_half_duplex_tx_blocking)(uint8_t command, uint8_t *rx_data, uint16_t size),
    void (*transfer_half_duplex_tx_non_blocking)(uint8_t command, uint8_t *rx_data, uint16_t size),
    void (*transfer_full_duplex_blocking)(uint8_t *tx_data, uint8_t *rx_data, uint16_t size),
    void (*transfer_full_duplex_non_blocking)(uint8_t *tx_data, uint8_t *rx_data, uint16_t size),
    void (*set_access_mode_spi)(void), void (*set_access_mode_qspi)(void), bool (*is_transfer_busy)(void))
{
    if ((radio_id > MAX_NUMBER_OF_RADIOS) || (end_transfer == NULL) || (begin_transfer == NULL) ||
        (is_transfer_busy == NULL)) {
        return false;
    }

#if !RADIO_QSPI_ENABLED
    (void)transfer_half_duplex_rx_blocking;
    (void)transfer_half_duplex_rx_non_blocking;
    (void)transfer_half_duplex_tx_blocking;
    (void)transfer_half_duplex_tx_non_blocking;
    (void)set_access_mode_spi;
    (void)set_access_mode_qspi;
    if (!sr_access_radio_spi_link(radio_id, transfer_full_duplex_blocking, transfer_full_duplex_non_blocking)) {
        return false;
    }
#else
    (void)transfer_full_duplex_blocking;
    (void)transfer_full_duplex_non_blocking;
    if (!sr_access_radio_qspi_link(radio_id, transfer_half_duplex_rx_blocking, transfer_half_duplex_rx_non_blocking,
                                   transfer_half_duplex_tx_blocking, transfer_half_duplex_tx_non_blocking,
                                   set_access_mode_spi, set_access_mode_qspi)) {
        return false;
    }
#endif

    /* Link the radio communication functions. */
    radio_hal[radio_id].end_transfer = end_transfer;
    radio_hal[radio_id].begin_transfer = begin_transfer;
    radio_hal[radio_id].is_transfer_busy = is_transfer_busy;

    return true;
}

bool sr_access_radio_irq_link(uint8_t radio_id, bool (*read_irq_pin)(void), void (*radio_context_switch)(void),
                              void (*disable_radio_irq)(void), void (*enable_radio_irq)(void),
                              void (*disable_radio_non_blocking_transfer_irq)(void),
                              void (*enable_radio_non_blocking_transfer_irq)(void))
{
    if ((radio_id > MAX_NUMBER_OF_RADIOS) || (read_irq_pin == NULL) || (radio_context_switch == NULL) ||
        (disable_radio_irq == NULL) || (enable_radio_irq == NULL) ||
        (disable_radio_non_blocking_transfer_irq == NULL) || (enable_radio_non_blocking_transfer_irq == NULL)) {
        return false;
    }

    /* Link the IRQ functions. */
    radio_hal[radio_id].read_irq_pin = read_irq_pin;
    radio_hal[radio_id].radio_context_switch = radio_context_switch;
    radio_hal[radio_id].disable_radio_irq = disable_radio_irq;
    radio_hal[radio_id].enable_radio_irq = enable_radio_irq;
    radio_hal[radio_id].disable_radio_non_blocking_transfer_irq = disable_radio_non_blocking_transfer_irq;
    radio_hal[radio_id].enable_radio_non_blocking_transfer_irq = enable_radio_non_blocking_transfer_irq;

    return true;
}

#if SR1100
#if !RADIO_QSPI_ENABLED
void sr_access_setup_transfer_structures(uint8_t radio_id, sr_registers_t *sr_reg)
{
    /* Radio configuration memory mapping. */
    spi_xfer[radio_id].radio_cfg_out.addr_irq = REG16_IRQ;

    spi_xfer[radio_id].radio_cfg_out.addr_actions = REG_WRITE | REG8_ACTIONS;
    sr_reg->actions_write = &spi_xfer[radio_id].radio_cfg_out.actions;

    spi_xfer[radio_id].radio_cfg_out.addr_rx_address = REG_WRITE | REG16_RXADDRESS;
    sr_reg->rx_address = &spi_xfer[radio_id].radio_cfg_out.rx_address;

    spi_xfer[radio_id].radio_cfg_out.addr_tx_address = REG_WRITE | REG16_TXADDRESS;
    sr_reg->tx_address = &spi_xfer[radio_id].radio_cfg_out.tx_address;

    spi_xfer[radio_id].radio_cfg_out.addr_rx_tx_size = REG_WRITE | REG16_RX_TX_SIZEREG;
    sr_reg->rx_tx_size = &spi_xfer[radio_id].radio_cfg_out.rx_tx_size;

    spi_xfer[radio_id].radio_cfg_out.addr_phy_0_1 = REG_WRITE | REG16_PHY_0_1;
    sr_reg->phy_0_1 = &spi_xfer[radio_id].radio_cfg_out.phy_0_1;

    spi_xfer[radio_id].radio_cfg_out.addr_harddisables_ioconfig = REG_WRITE | REG16_HARDDISABLES_IOCONFIG;
    sr_reg->harddisables_ioconfig = &spi_xfer[radio_id].radio_cfg_out.harddisables_ioconfig;
    SR_UINT16_WRITE(sr_reg->harddisables_ioconfig, sr_access_read_reg16(radio_id, REG16_HARDDISABLES_IOCONFIG));

    spi_xfer[radio_id].radio_cfg_out.addr_preamb_debug = REG_WRITE | REG16_PREAMB_DEBUG;
    sr_reg->preamb_debug = &spi_xfer[radio_id].radio_cfg_out.preamb_debug;

    spi_xfer[radio_id].radio_cfg_out.burst_write_start_addr = REG_WRITE_BURST | REG16_CCA_SETTINGS;
    sr_reg->cca_settings = &spi_xfer[radio_id].radio_cfg_out.cca_settings;
    sr_reg->cca_thres_gain = &spi_xfer[radio_id].radio_cfg_out.cca_thres_gain;
    sr_reg->rf_gain_manugain = &spi_xfer[radio_id].radio_cfg_out.rf_gain_manu;
    sr_reg->if_baseband_gain_lna = &spi_xfer[radio_id].radio_cfg_out.if_bb_gain_lna;
    sr_reg->rxbandfre_cfg1freq = &spi_xfer[radio_id].radio_cfg_out.rxbandfre_cfg1freq;
    sr_reg->cfg2freq_cfg3freq = &spi_xfer[radio_id].radio_cfg_out.cfg2freq_cfg3freq;
    sr_reg->cfg_widths_txpwr_randpulse = &spi_xfer[radio_id].radio_cfg_out.cfg_widths_txpwr_randpulse;
    sr_reg->tx_pulse_pos = &spi_xfer[radio_id].radio_cfg_out.tx_pulse_pos;
    sr_reg->slpperiod_15_0 = &spi_xfer[radio_id].radio_cfg_out.slpperiod_15_0;
    sr_reg->slpperiod_pwrupdlay = &spi_xfer[radio_id].radio_cfg_out.slpperiod_pwrupdlay;
    sr_reg->timelimit_biasdelay = &spi_xfer[radio_id].radio_cfg_out.timelimit_biasdelay;
    sr_reg->timercfg_sleepcfg_write = &spi_xfer[radio_id].radio_cfg_out.timercfg_sleepcfg;
    sr_reg->irq_write = &spi_xfer[radio_id].radio_cfg_out.irq;
    sr_reg->frameproc_phasedata_write = &spi_xfer[radio_id].radio_cfg_out.frameproc_phasedata;

    /* Fill header memory mapping. */
    spi_xfer[radio_id].fill_header_out.addr_fifo = REG_WRITE_BURST | REG8_FIFOS;
    sr_reg->fifo.send_header_size = &spi_xfer[radio_id].fill_header_out.data_fifo[0];
    sr_reg->fifo.send_header = &spi_xfer[radio_id].fill_header_out.data_fifo[1];

    /* Read events. */
    spi_xfer[radio_id].read_events_out.addr_pwr_status = REG8_POWER_STATE;
    sr_reg->power_state = &spi_xfer[radio_id].read_events_in.pwr_status;

#if WPS_RADIO_COUNT == 2
    spi_xfer[radio_id].read_events_out.addr_timercfg_sleepcfg = REG_WRITE | REG16_TIMERCFG_SLEEPCFG;
    sr_reg->timercfg_sleepcfg_write_read_events = &spi_xfer[radio_id].read_events_out.set_timercfg_sleepcfg;
#endif

    spi_xfer[radio_id].read_events_out.addr_set_actions = REG_WRITE | REG8_ACTIONS;
    spi_xfer[radio_id].read_events_out.addr_actions = REG8_ACTIONS;
    sr_reg->actions_read = &spi_xfer[radio_id].read_events_in.actions;

    /* Disable IRQ sources to be sure it does not trigger after a failed reception. */
    spi_xfer[radio_id].read_events_out.addr_set_irq = REG_WRITE | REG16_IRQ;
    spi_xfer[radio_id].read_events_out.addr_irq = REG16_IRQ;
    sr_reg->irq_read = &spi_xfer[radio_id].read_events_in.irq;

    /* Read info. */
    spi_xfer[radio_id].read_info_out.addr_frame_size = REG8_FIFOS;
    sr_reg->fifo.recv_frame_size = &spi_xfer[radio_id].read_info_in.data_frame_size;

    spi_xfer[radio_id].read_info_out.addr_header_size = REG8_FIFOS;
    sr_reg->fifo.recv_header_size = &spi_xfer[radio_id].read_info_in.data_header_size;

    spi_xfer[radio_id].read_info_out.burst_read_start_addr = REG_READ_BURST | REG16_FRAMEPROC_PHASEDATA;
    sr_reg->frameproc_phasedata_read = &spi_xfer[radio_id].read_info_in.frameproc_phasedata;
    sr_reg->rssi_rnsi = &spi_xfer[radio_id].read_info_in.rssi_rnsi;
    sr_reg->rxtime = &spi_xfer[radio_id].read_info_in.rxtime;

    /* Read CIR info. */
    spi_xfer[radio_id].read_cir_info_out.addr_cir_info = REG_READ_BURST | REG8_FIFOS;
    sr_reg->fifo.recv_cir_info = spi_xfer[radio_id].read_cir_info_in.data_cir_info;
}

#else

void sr_access_setup_transfer_structures(uint8_t radio_id, sr_registers_t *sr_reg)
{
    /* Radio configuration memory mapping. */
    sr_reg->actions_write = &qspi_xfer[radio_id].radio_cfg.actions;

    sr_reg->rx_address = &qspi_xfer[radio_id].radio_cfg.rx_address;

    sr_reg->tx_address = &qspi_xfer[radio_id].radio_cfg.tx_address;

    sr_reg->rx_tx_size = &qspi_xfer[radio_id].radio_cfg.rx_tx_size;

    sr_reg->phy_0_1 = &qspi_xfer[radio_id].radio_cfg.phy_0_1;

    sr_reg->harddisables_ioconfig = &qspi_xfer[radio_id].radio_cfg.harddisables_ioconfig;
    SR_UINT16_WRITE(sr_reg->harddisables_ioconfig, sr_access_read_reg16(radio_id, REG16_HARDDISABLES_IOCONFIG));

    sr_reg->preamb_debug = &qspi_xfer[radio_id].radio_cfg.preamb_debug;

    sr_reg->cca_settings = &qspi_xfer[radio_id].radio_cfg.cca_settings;
    sr_reg->cca_thres_gain = &qspi_xfer[radio_id].radio_cfg.cca_thres_gain;
    sr_reg->rf_gain_manugain = &qspi_xfer[radio_id].radio_cfg.rf_gain_manu;
    sr_reg->if_baseband_gain_lna = &qspi_xfer[radio_id].radio_cfg.if_bb_gain_lna;
    sr_reg->rxbandfre_cfg1freq = &qspi_xfer[radio_id].radio_cfg.rxbandfre_cfg1freq;
    sr_reg->cfg2freq_cfg3freq = &qspi_xfer[radio_id].radio_cfg.cfg2freq_cfg3freq;
    sr_reg->cfg_widths_txpwr_randpulse = &qspi_xfer[radio_id].radio_cfg.cfg_widths_txpwr_randpulse;
    sr_reg->tx_pulse_pos = &qspi_xfer[radio_id].radio_cfg.tx_pulse_pos;
    sr_reg->slpperiod_15_0 = &qspi_xfer[radio_id].radio_cfg.slpperiod_15_0;
    sr_reg->slpperiod_pwrupdlay = &qspi_xfer[radio_id].radio_cfg.slpperiod_pwrupdlay;
    sr_reg->timelimit_biasdelay = &qspi_xfer[radio_id].radio_cfg.timelimit_biasdelay;
    sr_reg->timercfg_sleepcfg_write = &qspi_xfer[radio_id].radio_cfg.timercfg_sleepcfg;
    sr_reg->irq_write = &qspi_xfer[radio_id].radio_cfg.irq;
    sr_reg->frameproc_phasedata_write = &qspi_xfer[radio_id].radio_cfg.frameproc_phasedata;

    /* Fill header memory mapping. */
    sr_reg->fifo.send_header_size = &qspi_xfer[radio_id].fill_header.header[0];
    sr_reg->fifo.send_header = &qspi_xfer[radio_id].fill_header.header[1];

    /* Read events. */
    sr_reg->power_state = &qspi_xfer[radio_id].read_events.pwr_status;

    sr_reg->actions_read = &qspi_xfer[radio_id].read_events.actions;

    /* Disable IRQ sources to be sure it does not trigger after a failed reception. */
    sr_reg->irq_read = &qspi_xfer[radio_id].read_events.irq;

    /* Read info. */
    sr_reg->fifo.recv_frame_size = &qspi_xfer[radio_id].read_info.frame_size;
    sr_reg->fifo.recv_header_size = &qspi_xfer[radio_id].read_info.header_size;

    sr_reg->frameproc_phasedata_read = &qspi_xfer[radio_id].read_info.frameproc_phasedata;
    sr_reg->rssi_rnsi = &qspi_xfer[radio_id].read_info.rssi_rnsi;
    sr_reg->rxtime = &qspi_xfer[radio_id].read_info.rxtime;

    /* Read CIR info. */
    sr_reg->fifo.recv_cir_info = qspi_xfer[radio_id].read_cir_info.data_cir_info;
}

/** @brief Links the radio QSPI communication functions for the specified radio ID.
 *
 *  @param[in] radio_id                              The ID of the radio to link the QSPI communication functions for.
 *  @param[in] transfer_half_duplex_rx_blocking      Function pointer for the blocking half-duplex RX transfer function.
 *  @param[in] transfer_half_duplex_rx_non_blocking  Function pointer for the non-blocking half-duplex RX transfer
 *                                                   function.
 *  @param[in] transfer_half_duplex_tx_blocking      Function pointer for the blocking half-duplex TX transfer function.
 *  @param[in] transfer_half_duplex_tx_non_blocking  Function pointer for the non-blocking half-duplex TX transfer
 *                                                   function.
 *  @param[in] set_access_mode_spi                   Function pointer for the function to set the SPI access mode.
 *  @param[in] set_access_mode_qspi                  Function pointer for the function to set the QSPI access mode.
 *  @return true if the radio QSPI link was successfully accessed, false otherwise.
 */
static bool sr_access_radio_qspi_link(
    uint8_t radio_id, void (*transfer_half_duplex_rx_blocking)(uint8_t command, uint8_t *tx_data, uint16_t size),
    void (*transfer_half_duplex_rx_non_blocking)(uint8_t command, uint8_t *tx_data, uint16_t size),
    void (*transfer_half_duplex_tx_blocking)(uint8_t command, uint8_t *rx_data, uint16_t size),
    void (*transfer_half_duplex_tx_non_blocking)(uint8_t command, uint8_t *rx_data, uint16_t size),
    void (*set_access_mode_spi)(void), void (*set_access_mode_qspi)(void))
{
    /* TODO: Remove when QSPI dual radio is supported. */
    if (radio_id == 1) {
        return true;
    }

    if ((radio_id > MAX_NUMBER_OF_RADIOS) || (transfer_half_duplex_rx_blocking == NULL) ||
        (transfer_half_duplex_rx_non_blocking == NULL) || (transfer_half_duplex_tx_blocking == NULL) ||
        (transfer_half_duplex_tx_non_blocking == NULL) || (set_access_mode_spi == NULL) ||
        (set_access_mode_qspi == NULL)) {
        return false;
    }

    /* Link the radio QSPI communication functions. */
    radio_hal[radio_id].transfer_half_duplex_rx_blocking = transfer_half_duplex_rx_blocking;
    radio_hal[radio_id].transfer_half_duplex_rx_non_blocking = transfer_half_duplex_rx_non_blocking;
    radio_hal[radio_id].transfer_half_duplex_tx_blocking = transfer_half_duplex_tx_blocking;
    radio_hal[radio_id].transfer_half_duplex_tx_non_blocking = transfer_half_duplex_tx_non_blocking;
    radio_hal[radio_id].set_access_mode_spi = set_access_mode_spi;
    radio_hal[radio_id].set_access_mode_qspi = set_access_mode_qspi;

    return true;
}

#endif /* RADIO_QSPI_ENABLED */
#endif /* SR1100 */

#if !RADIO_QSPI_ENABLED
/** @brief Access the radio SPI link by linking the provided SPI communication functions to the radio HAL structure.
 *
 *  @param[in] radio_id                           The ID of the radio to access the SPI link for.
 *  @param[in] transfer_full_duplex_blocking      Function pointer to the radio SPI communication function that performs
 *                                                a full-duplex blocking transfer.
 *  @param[in] transfer_full_duplex_non_blocking  Function pointer to the radio SPI communication function that performs
 *                                                a full-duplex non-blocking transfer.
 *  @return true if the radio SPI link was successfully accessed, false otherwise.
 */
static bool sr_access_radio_spi_link(
    uint8_t radio_id, void (*transfer_full_duplex_blocking)(uint8_t *tx_data, uint8_t *rx_data, uint16_t size),
    void (*transfer_full_duplex_non_blocking)(uint8_t *tx_data, uint8_t *rx_data, uint16_t size))
{
    if ((radio_id > MAX_NUMBER_OF_RADIOS) || (transfer_full_duplex_blocking == NULL) ||
        (transfer_full_duplex_non_blocking == NULL)) {
        return false;
    }

    /* Link the radio SPI communication functions. */
    radio_hal[radio_id].transfer_full_duplex_blocking = transfer_full_duplex_blocking;
    radio_hal[radio_id].transfer_full_duplex_non_blocking = transfer_full_duplex_non_blocking;

    return true;
}
#endif
