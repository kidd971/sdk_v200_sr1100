/** @file  wps_phy_common.c
 *  @brief Wireless protocol stack PHY control.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "swc_hal_facade.h"
#include "wps.h"
#include "wps_config.h"
#include "wps_phy.h"

/* CONSTANTS ******************************************************************/
#define HDR_SIZE_SIZE                 1
#define FAST_SYNC_TIMER_VALUE         32000
#define MAX_SLP_TIME_VAL_16           0xFFFF
#define MAX_SLP_TIME_VAL_8            0xFF
#define MAX_RX_TIMEOUT_VALUE          0x1FFF
#define FAST_SYNC_IDLE_SLEEP_VAL      0xFFFF
#define FAST_SYNC_IDLE_TIMEOUT_VAL    (0xFFFF - 8)
#define DISABLE_CCA_THRES             0
#define CCA_RETRYHDR_MASK             0x0F
#define RX_SAVED_BYTE_COUNT           2 /* Saved fields are SAVESIZE and RETRYHDR */

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void prepare_phy(wps_phy_t *phy);
static void prepare_radio(wps_phy_t *phy);
static void prepare_radio_tx(wps_phy_t *phy);
static void prepare_radio_rx(wps_phy_t *phy);
static void set_config(wps_phy_t *phy);
static void enable_radio_irq(wps_phy_t *phy);
static void check_radio_irq(wps_phy_t *phy);
static void set_header(wps_phy_t *phy);
static void set_payload(wps_phy_t *phy);
static void set_header_and_payload(wps_phy_t *phy);
static void read_events(wps_phy_t *phy);
static void read_events_syncing(wps_phy_t *phy);
static void process_event_tx(wps_phy_t *phy);
static void process_event_rx(wps_phy_t *phy);
static void handle_good_frame(wps_phy_t *phy);
static void handle_good_auto_reply(wps_phy_t *phy);
static void get_frame_header(wps_phy_t *phy);
static void get_auto_reply_header(wps_phy_t *phy);
static void get_payload(wps_phy_t *phy);
static void handle_cca_fail(wps_phy_t *phy);
static void close_spi(wps_phy_t *phy);
static void end(wps_phy_t *phy);
static void none(wps_phy_t *phy);
static void prepare_syncing(wps_phy_t *phy);
#if WPS_RADIO_COUNT == 1
static void transfer_register(wps_phy_t *phy);
static void overwrite_registers(wps_phy_t *phy);
static void overwrite_queue_get_next(circular_queue_t *queue, void **it);
static void overwrite_queue_add_transfer(circular_queue_t *queue, void *it, uint8_t starting_reg,
                                         uint16_t data);
#endif
static bool main_is_tx(wps_phy_t *phy);
static bool auto_is_tx(wps_phy_t *phy);
static bool tx_complete(read_events_t *read_events);
static bool tx_complete_auto_reply(read_events_t *read_events);
static bool rx_good(read_events_t *read_events);
static bool rx_good_auto_reply(read_events_t *read_events);
static bool rx_rejected(read_events_t *read_events);
static bool rx_rejected_auto_reply(read_events_t *read_events);
static bool rx_lost(read_events_t *read_events);
static void set_events_for_tx_with_ack(wps_phy_t *phy);
static void set_events_for_tx_without_ack(wps_phy_t *phy);
static void set_events_for_rx_with_ack(wps_phy_t *phy);
static void set_events_for_rx_without_ack(wps_phy_t *phy);
static void set_events_for_wakeup_only(wps_phy_t *phy);
static void set_events_for_empty_tx(wps_phy_t *phy);
static void enqueue_states(wps_phy_t *wps_phy, wps_phy_state_t *state);
static void init_transfer_structures(wps_phy_t *wps_phy);
static void fast_sync_config_non_stop_rx(wps_phy_t *phy);

/* TYPES **********************************************************************/
static wps_phy_state_t prepare_phy_states[]             = {prepare_phy, end};
static wps_phy_state_t set_config_states[]              = {set_config, close_spi, end};
static wps_phy_state_t set_header_states[]              = {close_spi, set_header, end};
static wps_phy_state_t set_payload_states[]             = {set_payload, end};
static wps_phy_state_t set_header_with_payload_states[] = {close_spi, set_header_and_payload, end};
static wps_phy_state_t wait_radio_states_tx[]           = {close_spi, enable_radio_irq, read_events,
                                                           close_spi, process_event_tx, end};
static wps_phy_state_t wait_radio_states_rx[]           = {close_spi, enable_radio_irq, read_events,
                                                           close_spi, process_event_rx, end};
static wps_phy_state_t get_frame_header_states[]        = {close_spi, get_frame_header, end};
static wps_phy_state_t get_auto_reply_header_states[]   = {close_spi, get_auto_reply_header, end};
static wps_phy_state_t get_payload_states[]             = {get_payload, end};
static wps_phy_state_t new_frame_states[]               = {close_spi, end};
static wps_phy_state_t syncing_states[] = {read_events_syncing, close_spi, process_event_rx, end};
static wps_phy_state_t wait_to_send_auto_reply[] = {check_radio_irq, end};
#if WPS_RADIO_COUNT == 1
static wps_phy_state_t transfer_register_states[]  = {transfer_register, end};
static wps_phy_state_t overwrite_register_states[] = {overwrite_registers, end};
#endif
static wps_phy_state_t end_states[] = {none};

/* PUBLIC FUNCTIONS ***********************************************************/
void phy_init(wps_phy_t *wps_phy, wps_phy_cfg_t *cfg)
{
    if (wps_phy->debug_cfg.enable == false) {
        wps_phy->debug_cfg.interleav                 = 0;
        wps_phy->debug_cfg.preamble_detection        = SET_PREATRKBW(2) | SET_PREADETBW(2) |
                                                SET_PREAMBTHR(12);
        wps_phy->debug_cfg.syncword_detection = SET_SOFTSWTHR(10) | SET_SWBITTOL(2) | GAINCTBW_0b1;
        wps_phy->debug_cfg.phase_offset_stats_enable = false;
    }

    wps_phy->state_step = 0;
    wps_phy->radio = cfg->radio;
    wps_phy->local_address = cfg->local_address;
    wps_phy->current_state = prepare_phy_states;
    wps_phy->end_state = end;

    memset(&wps_phy->write_request_info, 0, sizeof(xlayer_request_info_t));
    memset(&wps_phy->read_request_info, 0, sizeof(xlayer_request_info_t));

    circular_queue_init(&wps_phy->next_states, wps_phy->next_state_pool, PHY_STATE_Q_SIZE, sizeof(wps_phy_state_t **));
    circular_queue_init(&wps_phy->overwrite_regs_queue, wps_phy->overwrite_regs_pool,
                        PHY_OVERWRITE_REG_Q_SIZE, sizeof(reg_t));

    init_transfer_structures(wps_phy);

    /* Disable IRQ during INIT */
    sr_access_write_reg16(wps_phy->radio->radio_id, REG16_IRQ, 0x0000);

    sr_access_write_reg16(wps_phy->radio->radio_id, REG16_SYNCWORD_15_0, cfg->syncword_cfg.syncword);
    sr_access_write_reg16(wps_phy->radio->radio_id, REG16_SYNCWORD_31_16, cfg->syncword_cfg.syncword >> 16);

    sr_access_write_reg16(wps_phy->radio->radio_id, REG16_CRC_15_1, SET_CRC_POLY_15_1(cfg->crc_polynomial));
    sr_access_write_reg16(wps_phy->radio->radio_id, REG16_CRC_30_16, SET_CRC_POLY_15_1(cfg->crc_polynomial >> 15));

    sr_access_write_reg16(wps_phy->radio->radio_id, REG16_PREAMB_SWLEN,
                          cfg->syncword_cfg.syncword_length | SET_PREAMBLEN(cfg->preamble_len));

    sr_access_write_reg16(wps_phy->radio->radio_id, REG16_RX_TX_SIZEREG,
                          SET_TXPKTSIZE(MAX_FRAMESIZE) | SET_RXPKTSIZE(MAX_FRAMESIZE));

    sr_access_write_reg16(wps_phy->radio->radio_id, REG16_RXADDRESS, SET_RXADDRESS(cfg->local_address));

    sr_access_write_reg16(wps_phy->radio->radio_id, REG16_TIMERCFG_SLEEPCFG,
                          cfg->sleep_lvl | SLPTIMEO_0b1 | SLPTXEND_0b1 | SLPRXEND_0b1);

    sr_access_write_reg8(wps_phy->radio->radio_id, REG8_ACTIONS, FLUSHTX_0b1 | FLUSHRX_0b1);

    sr_access_write_reg16(wps_phy->radio->radio_id, REG16_IRQ, RXENDE_0b1 | TIMEOUTE_0b1 | TXENDE_0b1);

    sr_access_write_reg16(wps_phy->radio->radio_id, REG16_PRELUDE, REG16_PRELUDE_OPT);

    /* #3: The radio needs to be put in regular TX once to function properly.
     */
    sr_access_write_reg16(wps_phy->radio->radio_id, REG16_FRAMEPROC_PHASEDATA, 0x00);
    sr_access_write_reg8(wps_phy->radio->radio_id, REG8_ACTIONS, STARTTX_0b1);

    /* #1: Enabling SAVE_CRC bit in FRAMECFG_SAVETOBUF is the only way to have the radio sleep
     *     on a timeout event, enabling this prevents double timeout IRQ problems and
     *     also optimize the system for power correctly.
     */
    if (wps_phy->debug_cfg.phase_offset_stats_enable) {
        sr_access_write_reg16(wps_phy->radio->radio_id, REG16_FRAMECFG_SAVETOBUF,
                              DEFAULT_PACKET_CONFIGURATION | SAVEPHS_0b1 | SAVECRC_0b1);
    } else {
        sr_access_write_reg16(wps_phy->radio->radio_id, REG16_FRAMECFG_SAVETOBUF,
                              DEFAULT_PACKET_CONFIGURATION | SAVECRC_0b1);
    }

    sr_access_write_reg16(wps_phy->radio->radio_id, REG16_RF_GAIN_MANUGAIN, SET_PKTRFGAIN(cfg->rx_gain));

    /* #4: The TXOVRFLI interrupt can trigger if the TX FIFO is being written when the transmitter wakes up. */
    sr_access_write_reg16(wps_phy->radio->radio_id, REG16_IRQTIME, DISABUFI_0b1);

    sr_access_read_reg16(wps_phy->radio->radio_id, REG16_IRQ);
}

void phy_connect(wps_phy_t *wps_phy)
{
    sr_access_write_reg8(wps_phy->radio->radio_id, REG8_ACTIONS, FLUSHTX_0b1 | FLUSHRX_0b1 | INITIMER_0b1 | SLEEP_0b1);

    sr_access_write_reg16(wps_phy->radio->radio_id, REG16_TIMELIMIT_BIASDELAY, SET_TIMEOUT(TIMEOUT_VAL2RAW(0xFFFF)));

    sr_access_read_reg16(wps_phy->radio->radio_id, REG16_IRQ);

    sr_access_enable_radio_irq(wps_phy->radio->radio_id);
    sr_access_enable_dma_irq(wps_phy->radio->radio_id);

    wps_phy->state_step    = 0;
    wps_phy->current_state = prepare_phy_states;
    circular_queue_init(&wps_phy->next_states, wps_phy->next_state_pool, PHY_STATE_Q_SIZE, sizeof(wps_phy_state_t **));
    wps_phy->signal_main = PHY_SIGNAL_CONNECT;
}

void phy_connect_single(wps_phy_t *wps_phy)
{
    do {
        sr_access_write_reg8(wps_phy->radio->radio_id, REG8_ACTIONS, 0x0000);
        wps_phy->pwr_status_cmd = sr_access_read_reg8(wps_phy->radio->radio_id, REG8_POWER_STATE);
    } while (!GET_AWAKE(wps_phy->pwr_status_cmd));

    sr_access_write_reg16(wps_phy->radio->radio_id, REG16_SLPPERIOD_PWRUPDLAY, SET_SLPPERIOD_23_16(MAX_SLP_TIME_VAL_8));
    sr_access_write_reg16(wps_phy->radio->radio_id, REG16_SLPPERIOD_15_0, SET_SLPPERIOD_15_0(MAX_SLP_TIME_VAL_16));

    sr_access_write_reg16(wps_phy->radio->radio_id, REG16_TIMERCFG_SLEEPCFG,
                          SLEEP_IDLE | SLPTIMEO_0b1 | SLPTXEND_0b1 | SLPRXEND_0b1 | AUTOWAKE_0b1);
    phy_connect(wps_phy);
    sr_access_radio_context_switch(wps_phy->radio->radio_id);
}

void phy_connect_multi(wps_phy_t *wps_phy)
{
    do {
        sr_access_write_reg8(wps_phy->radio->radio_id, REG8_ACTIONS, 0x0000);
        wps_phy->pwr_status_cmd = sr_access_read_reg8(wps_phy->radio->radio_id, REG8_POWER_STATE);
    } while (!GET_AWAKE(wps_phy->pwr_status_cmd));

    sr_access_write_reg16(wps_phy->radio->radio_id, REG16_TIMERCFG_SLEEPCFG,
                          SLEEP_IDLE | SLPTIMEO_0b1 | SLPTXEND_0b1 | SLPRXEND_0b1);
}

void phy_wakeup_multi(wps_phy_t *wps_phy)
{
    wps_phy->radio_actions |= INITIMER_0b1;
    sr_access_write_reg8(wps_phy->radio->radio_id, REG8_ACTIONS, wps_phy->radio_actions);
}

void phy_abort_radio_events(wps_phy_t *wps_phy)
{
    while (sr_access_is_spi_busy(wps_phy->radio->radio_id)) {
        /* wait for any SPI transfer to complete */
    }

    sr_access_close(wps_phy->radio->radio_id);

    /* Disable peripherals interrupts */
    sr_access_disable_dma_irq(wps_phy->radio->radio_id);
    sr_access_disable_radio_irq(wps_phy->radio->radio_id);

    /* Disable radio interrupts */
    sr_access_write_reg16(wps_phy->radio->radio_id, REG16_IRQ, 0);
    sr_access_write_reg16(wps_phy->radio->radio_id, REG16_IRQTIME, DISABUFI_0b1);

    /* Clear radio interrupts */
    (void)sr_access_read_reg16(wps_phy->radio->radio_id, REG16_IRQ);
}

void phy_disconnect(wps_phy_t *wps_phy)
{
    uint8_t pwr_status;

    /* NOTE: There may be an issue when disconnecting while doing CCA tries.
     *       This has been patched on 1020, but we're not sure if the problem exists on 1120.
     */

    /* Reset timer configuration and disable AUTOWAKE to allow radio to wake up */
    sr_access_write_reg16(wps_phy->radio->radio_id, REG16_TIMERCFG_SLEEPCFG, 0);

    /* Wait for radio to wakeup */
    do {
        sr_access_write_reg8(wps_phy->radio->radio_id, REG8_ACTIONS, 0x00);
        pwr_status = sr_access_read_reg8(wps_phy->radio->radio_id, REG8_POWER_STATE);
    } while (!GET_AWAKE(pwr_status));

    /* Set radio to deep sleep */
    sr_access_write_reg16(wps_phy->radio->radio_id, REG16_TIMERCFG_SLEEPCFG, SLEEP_DEEP);
    sr_access_write_reg8(wps_phy->radio->radio_id, REG8_ACTIONS, SLEEP_0b1);

    /* Wait until we are in deep sleep */
    do {
        pwr_status = sr_access_read_reg8(wps_phy->radio->radio_id, REG8_POWER_STATE);
    } while (pwr_status != 0);
}

void phy_start_tx_now(wps_phy_t *wps_phy)
{
    sr_access_write_reg16(wps_phy->radio->radio_id, REG16_TIMERCFG_SLEEPCFG,
                          SLPDEPTH_WAKEONCE_0b01 | SLPTIMEO_0b1 | SLPTXEND_0b1 | SLPRXEND_0b1 | AUTOWAKE_0b1);
    wps_phy->radio_actions |= INITIMER_0b1 | STARTTX_0b1;
    sr_access_write_reg8(wps_phy->radio->radio_id, REG8_ACTIONS, wps_phy->radio_actions);
}

phy_output_signal_t phy_get_main_signal(wps_phy_t *wps_phy)
{
    return wps_phy->signal_main;
}

phy_output_signal_t phy_get_auto_signal(wps_phy_t *wps_phy)
{
    return wps_phy->signal_auto;
}

void phy_set_main_xlayer(wps_phy_t *wps_phy, xlayer_t *xlayer, xlayer_cfg_internal_t *xlayer_cfg)
{
    wps_phy->config      = xlayer_cfg;
    wps_phy->xlayer_main = xlayer;
}

void phy_set_auto_xlayer(wps_phy_t *wps_phy, xlayer_t *xlayer)
{
    wps_phy->xlayer_auto = xlayer;
}

void phy_write_register(wps_phy_t *wps_phy, uint8_t starting_reg, uint16_t data,
                        reg_write_cfg_t cfg)
{
#if WPS_RADIO_COUNT == 1
    circular_queue_t *queue = &wps_phy->overwrite_regs_queue;
    void *dequeue_ptr       = circular_queue_front_raw(queue);

    if (cfg == WRITE_ONCE) {
        wps_phy->write_request_info.target_register = starting_reg;
        wps_phy->write_request_info.data            = data;
        wps_phy->write_request_info.pending_request = true;
        enqueue_states(wps_phy, transfer_register_states);
    } else if (cfg == WRITE_PERIODIC) {
        overwrite_queue_add_transfer(queue, dequeue_ptr, starting_reg, data);
    }
#else
    (void)wps_phy;
    (void)starting_reg;
    (void)data;
    (void)cfg;
#endif
}

void phy_clear_write_register(wps_phy_t *wps_phy)
{
    circular_queue_init(&wps_phy->overwrite_regs_queue, wps_phy->overwrite_regs_pool,
                        PHY_OVERWRITE_REG_Q_SIZE, sizeof(reg_t));
}

void phy_read_register(wps_phy_t *wps_phy, uint8_t target_register, uint16_t *rx_buffer,
                       bool *xfer_cmplt)
{
#if WPS_RADIO_COUNT == 1
    wps_phy->read_request_info.rx_buffer       = rx_buffer;
    wps_phy->read_request_info.xfer_cmplt      = xfer_cmplt;
    wps_phy->read_request_info.target_register = target_register;
    wps_phy->read_request_info.pending_request = true;
    enqueue_states(wps_phy, transfer_register_states);
#else
    (void)wps_phy;
    (void)target_register;
    (void)rx_buffer;
    (void)xfer_cmplt;
#endif
}

void phy_enable_debug_feature(wps_phy_t *phy, phy_debug_cfg_t *phy_debug)
{
    (void)phy;
    (void)phy_debug;
}

void phy_enqueue_prepare(wps_phy_t *phy)
{
    phy->next_states.enqueue_it = phy->next_states.buffer_begin;
    phy->next_states.dequeue_it = phy->next_states.buffer_begin;
    phy->state_step = 0;
    phy->current_state = prepare_phy_states;
}

void phy_enqueue_none(wps_phy_t *phy)
{
    phy->next_states.enqueue_it = phy->next_states.buffer_begin;
    phy->next_states.dequeue_it = phy->next_states.buffer_begin;
    phy->state_step = 0;
    phy->current_state = end_states;
}

/* PRIVATE FUNCTION ***********************************************************/
#if WPS_RADIO_COUNT == 1
/** @brief Write to target register.
 *
 *  @param[in] signal_data  Data required to process the state. The type shall be wps_phy_t.
 */
static void transfer_register(wps_phy_t *phy)
{
    uint8_t tx_buffer[3] = {0};
    uint8_t rx_buffer[3] = {0};

    if (phy->write_request_info.pending_request) {
        /* Write register request */
        tx_buffer[0] = phy->write_request_info.target_register | REG_WRITE;
        tx_buffer[1] = phy->write_request_info.data;
        tx_buffer[2] = phy->write_request_info.data >> 8;
        while (sr_access_is_spi_busy(phy->radio->radio_id)) {};
        sr_access_close(phy->radio->radio_id);
        sr_access_open(phy->radio->radio_id);
        if (REG_IS_16_BITS(tx_buffer[0] & ~REG_WRITE)) {
            sr_access_spi_transfer_blocking(phy->radio->radio_id, tx_buffer, rx_buffer, 3);
        } else {
            sr_access_spi_transfer_blocking(phy->radio->radio_id, tx_buffer, rx_buffer, 2);
        }
        sr_access_close(phy->radio->radio_id);
        phy->write_request_info.pending_request = false;
    } else if (phy->read_request_info.pending_request) {
        /* Read register request */
        tx_buffer[0] = phy->read_request_info.target_register;
        while (sr_access_is_spi_busy(phy->radio->radio_id)) {};
        sr_access_close(phy->radio->radio_id);
        sr_access_open(phy->radio->radio_id);
        if (REG_IS_16_BITS(tx_buffer[0] & ~REG_WRITE)) {
            sr_access_spi_transfer_blocking(phy->radio->radio_id, tx_buffer, rx_buffer, 3);
        } else {
            sr_access_spi_transfer_blocking(phy->radio->radio_id, tx_buffer, rx_buffer, 2);
        }
        sr_access_close(phy->radio->radio_id);
        *phy->read_request_info.rx_buffer = rx_buffer[1] | (rx_buffer[2] << 8);
        /* Thread safety */
        do {
            *phy->read_request_info.xfer_cmplt = true;
        } while (!(*phy->read_request_info.xfer_cmplt));
        /* Read register request */
        phy->read_request_info.pending_request = false;
    }
}

/** @brief Overwrite registers.
 *
 *  @param[in] signal_data  Data required to process the state. The type shall be wps_phy_t.
 */
static void overwrite_registers(wps_phy_t *phy)
{
    circular_queue_t *queue = &phy->overwrite_regs_queue;
    void *dequeue_ptr       = circular_queue_front_raw(queue);
    uint8_t tx_buffer[3];

    sr_access_close(phy->radio->radio_id);
    for (uint8_t i = 0; i < circular_queue_size(queue); i++) {
        sr_access_open(phy->radio->radio_id);
        tx_buffer[0] = ((reg_t *)dequeue_ptr)->addr;
        tx_buffer[1] = ((reg_t *)dequeue_ptr)->val;
        tx_buffer[2] = ((reg_t *)dequeue_ptr)->val >> 8;
        if (REG_IS_16_BITS(tx_buffer[0] & ~REG_WRITE)) {
            sr_access_spi_transfer_blocking(phy->radio->radio_id, tx_buffer, phy->spi_xfer.spi_dummy_buffer, 3);
        } else {
            sr_access_spi_transfer_blocking(phy->radio->radio_id, tx_buffer, phy->spi_xfer.spi_dummy_buffer, 2);
        }
        sr_access_close(phy->radio->radio_id);
        overwrite_queue_get_next(queue, &dequeue_ptr);
    }
}

/** @brief Get the next element after provided it and increment the it.
 *
 *  @note Allows to go through the queue, without dequeuing elements.
 *
 *  @param[in] queue  Cross layer queue instance.
 *  @param[in] it     Element to increment.
 */
void overwrite_queue_get_next(circular_queue_t *queue, void **it)
{
    *it = (void *)((char *)*it + queue->item_size);

    if (*it >= queue->buffer_end) {
        *it = queue->buffer_begin;
    }
}

/** @brief Add transfer to overwrite queue.
 *
 *  @note Enqueue new element if register transfer is not there,
 *        replace value if register is there.
 *
 *  @param[in] queue  Cross layer queue instance.
 *  @param[in] it     Element to increment.
 */
static void overwrite_queue_add_transfer(circular_queue_t *queue, void *it, uint8_t starting_reg,
                                         uint16_t data)
{
    reg_t *reg = (reg_t *)it;

    for (uint8_t i = 0; i < circular_queue_size(queue); i++) {
        if ((reg->addr & ~REG_WRITE) == starting_reg) {
            reg->val = data;
            return;
        }
        overwrite_queue_get_next(queue, &it);
    }

    reg = (reg_t *)circular_queue_get_free_slot_raw(queue);

    reg->addr = REG_WRITE | starting_reg;
    reg->val  = data;

    circular_queue_enqueue(queue);
}
#endif

/** @brief Enqueue a new state to the state machine.
 *
 *   @param[in] wps_phy  PHY instance struct.
 *   @param[in] state   New state to enqueue.
 */
static void enqueue_states(wps_phy_t *wps_phy, wps_phy_state_t *state)
{
    wps_phy_state_t **enqueue_states;

    enqueue_states  = (wps_phy_state_t **)circular_queue_get_free_slot_raw(&wps_phy->next_states);
    *enqueue_states = state;
    circular_queue_enqueue_raw(&wps_phy->next_states);
}

/** @brief Setup the state machine to send the frame payload to the radio.
 *
 *  @param[in] wps_phy        PHY instance struct.
 *  @param[in] payload_size   Frame payload size.
 *  @param[in] user_payload   Denotes if payload is provided from user space or from xlayer queue
 * buffer
 */
static void enqueue_tx_prepare_frame_states(wps_phy_t *wps_phy, uint8_t header_size,
                                            uint8_t payload_size, bool user_payload)
{
    /*
     * When the payload comes from user space memory, separate states are used for setting
     * the header and payload. Otherwise, both the header and payload are transferred in a single
     * SPI operation.
     */
    if (user_payload) {
        if (header_size + payload_size != 0) {
            enqueue_states(wps_phy, set_header_states);
        }

        if (payload_size != 0) {
            enqueue_states(wps_phy, set_payload_states);
        }
    } else {
        if (header_size + payload_size != 0) {
            enqueue_states(wps_phy, set_header_with_payload_states);
        }
    }
}

/** @brief Setup the state machine to receive payload from the radio.
 *
 *  @param[in] wps_phy        PHY instance struct.
 */
static void enqueue_rx_prepare_frame_states(wps_phy_t *wps_phy)
{
    enqueue_states(wps_phy, wait_radio_states_rx);
}

/** @brief Setup the PHY state machine.
 *
 *  @param[in] wps_phy        PHY instance struct.
 *  @param[in] payload_size  Frame payload size.
 */
static void prepare_phy(wps_phy_t *phy)
{
    if (phy->input_signal == PHY_SIGNAL_SYNCING) {
        enqueue_states(phy, syncing_states);
        prepare_syncing(phy);
    } else {
        enqueue_states(phy, set_config_states);
#if WPS_RADIO_COUNT == 1
        if (circular_queue_size(&phy->overwrite_regs_queue) != 0) {
            enqueue_states(phy, overwrite_register_states);
        }
#endif
        prepare_radio(phy);
    }
}

/** @brief Sub function to prepare a transmit frame.
 *
 *  @param[in] signal_data   Data required to process the state. The type shall be wps_phy_t.
 *  @param[in] radio_actions   Radio actions instance.
 */
static void prepare_radio_tx(wps_phy_t *phy)
{
    uint8_t header_size;
    uint8_t rx_packet_size;
    uint8_t tx_payload_size;
    uint16_t cca_action = (phy->config->cca_fail_action == CCA_FAIL_ACTION_TX) ? TXANYWAY_0b1 :
                                                                                 TXANYWAY_0b0;

    tx_payload_size = phy->xlayer_main->frame.payload_end_it -
                      phy->xlayer_main->frame.payload_begin_it;
    header_size = phy->xlayer_main->frame.header_end_it - phy->xlayer_main->frame.header_begin_it;

#if WPS_RADIO_COUNT == 1
    phy->spi_xfer.radio_cfg_out.timercfg_sleepcfg = phy->config->next_sleep_level | SLPTIMEO_0b1 | SLPTXEND_0b1 |
                                                    SLPRXEND_0b1 | AUTOWAKE_0b1;
#else
    if (wps_phy_multi_get_tx_wakeup_mode() == MULTI_TX_WAKEUP_MODE_AUTO) {
        /* Replying radio will autowake. */
        phy->spi_xfer.radio_cfg_out.timercfg_sleepcfg = phy->config->next_sleep_level | SLPTIMEO_0b1 | SLPTXEND_0b1 |
                                                        SLPRXEND_0b1 | AUTOWAKE_0b1;
    } else {
        /* following radio will be manually awakened. */
        phy->spi_xfer.radio_cfg_out.timercfg_sleepcfg = phy->config->next_sleep_level | SLPTIMEO_0b1 | SLPTXEND_0b1 |
                                                        SLPRXEND_0b1;
    }
#endif

    if (phy->xlayer_auto != NULL) {
        /* Autoreply mode */
        phy->spi_xfer.radio_cfg_out.phy_0_1 = EXPECRP0_0b1;
        if (phy->xlayer_auto->frame.payload_memory_size + phy->xlayer_auto->frame.header_memory_size == 0) {
            rx_packet_size = RX_SAVED_BYTE_COUNT;
        } else {
            rx_packet_size = phy->xlayer_auto->frame.payload_memory_size + phy->xlayer_auto->frame.header_memory_size +
                             HDR_SIZE_SIZE;
        }
        set_events_for_tx_with_ack(phy);
    } else if (phy->config->expect_ack) {
        /* Ack mode */
        rx_packet_size = RX_SAVED_BYTE_COUNT;
        phy->spi_xfer.radio_cfg_out.phy_0_1 = EXPECRP0_0b1;
        set_events_for_tx_with_ack(phy);
    } else {
        /* Nack mode */
        phy->spi_xfer.radio_cfg_out.phy_0_1 = EXPECRP0_0b0;
        rx_packet_size = 0;
        set_events_for_tx_without_ack(phy);
    }

    if ((header_size == 0) && !phy->config->certification_header_en) {
        /* If header_size == 0 and certification header is enabled, send an empty payload to
         * simulate an ACK.
         */
        rx_packet_size = 0;
        phy->spi_xfer.radio_cfg_out.cca_thres_gain = SET_CCATHRES(DISABLE_CCA_THRES);
        if ((phy->config->sleep_level == SLEEP_IDLE) || (phy->config->sleep_level == SLEEP_IDLE_NO_WAKEONCE)) {
            phy->spi_xfer.radio_cfg_out.actions = FLUSHTX_0b1 | FLUSHRX_0b1;
            set_events_for_wakeup_only(phy);
        } else {
            /* #5: In shallow sleep, WAKEUP interrupt usage is unreliable. */
            phy->spi_xfer.radio_cfg_out.actions = FLUSHTX_0b1 | FLUSHRX_0b1 | STARTTX_0b1;
            phy->spi_xfer.radio_cfg_out.phy_0_1 = EXPECRP0_0b0;
            set_events_for_empty_tx(phy);
        }
    } else {
        phy->spi_xfer.radio_cfg_out.actions = FLUSHTX_0b1 | FLUSHRX_0b1 | STARTTX_0b1;
        if (phy->config->cca_threshold == 0xFF) {
            phy->spi_xfer.radio_cfg_out.cca_thres_gain = SET_CCATHRES(DISABLE_CCA_THRES);
        } else {
            phy->spi_xfer.radio_cfg_out.cca_thres_gain = SET_CCATHRES(phy->config->cca_threshold);
        }
    }

    phy->spi_xfer.radio_cfg_out.rx_tx_size = SET_RXPKTSIZE(rx_packet_size);
    if ((header_size + tx_payload_size) == 0) {
        phy->spi_xfer.radio_cfg_out.rx_tx_size |= SET_TXPKTSIZE(0);
    } else {
        phy->spi_xfer.radio_cfg_out.rx_tx_size |= SET_TXPKTSIZE(header_size + tx_payload_size +
                                                                HDR_SIZE_SIZE);
    }
    phy->spi_xfer.radio_cfg_out.frameproc_phasedata = 0;
    phy->spi_xfer.radio_cfg_out.timelimit_biasdelay = SET_TIMEOUT(MAX_RX_TIMEOUT_VALUE);

    phy->spi_xfer.radio_cfg_out.cca_settings = SET_CCAINTERV(CCAINTERV_VAL2RAW(phy->config->cca_retry_time)) |
                                               SET_MAXRETRY(phy->config->cca_max_try_count) |
                                               SET_CCAONTIME(phy->config->cca_on_time) | IGNORPKT_0b1 | cca_action;

    phy->spi_xfer.radio_cfg_out.tx_address = SET_TXADDRESS(phy->xlayer_main->frame.destination_address);
    phy->spi_xfer.radio_cfg_out.rx_address = SET_RXADDRESS(phy->xlayer_main->frame.source_address);

    enqueue_tx_prepare_frame_states(phy, header_size, tx_payload_size,
                                    phy->xlayer_main->frame.user_payload);
    enqueue_states(phy, wait_radio_states_tx);
}

/** @brief Sub function to prepare a receive frame.
 *
 *  @param[in] signal_data   Data required to process the state. The type shall be wps_phy_t.
 *  @param[in] radio_actions   Radio actions instance.
 */
static void prepare_radio_rx(wps_phy_t *phy)
{
    uint8_t payload_size;
    uint8_t header_size;
    uint8_t tx_packet_size;
    uint16_t tx_address;

#if WPS_RADIO_COUNT == 1
    phy->spi_xfer.radio_cfg_out.timercfg_sleepcfg = phy->config->next_sleep_level | SLPTIMEO_0b1 | SLPTXEND_0b1 |
                                                    SLPRXEND_0b1 | AUTOWAKE_0b1;
#else
    phy->spi_xfer.radio_cfg_out.timercfg_sleepcfg = phy->config->next_sleep_level | SLPTIMEO_0b1 | SLPTXEND_0b1 |
                                                    SLPRXEND_0b1;
#endif

    if (phy->xlayer_auto != NULL) {
        /* Autoreply mode */
        payload_size = phy->xlayer_auto->frame.payload_end_it -
                       phy->xlayer_auto->frame.payload_begin_it;
        header_size = phy->xlayer_auto->frame.header_end_it -
                      phy->xlayer_auto->frame.header_begin_it;
        tx_packet_size = (header_size + payload_size == 0) ?
                             0 :
                             (header_size + payload_size + HDR_SIZE_SIZE);
        tx_address     = phy->xlayer_auto->frame.destination_address;
        phy->spi_xfer.radio_cfg_out.frameproc_phasedata = RX_MODE | RPLYTXEN_0b1;
        phy->spi_xfer.radio_cfg_out.phy_0_1             = EXPECRP0_0b1 | RPLYADD0_0b0;
        phy->spi_xfer.radio_cfg_out.rx_tx_size          = SET_TXPKTSIZE(tx_packet_size);
        set_events_for_rx_with_ack(phy);
        enqueue_tx_prepare_frame_states(phy, header_size, payload_size,
                                        phy->xlayer_auto->frame.user_payload);
        enqueue_states(phy, wait_radio_states_rx);
    } else if (phy->config->expect_ack) {
        /* Ack mode */
        tx_address                                      = phy->xlayer_main->frame.source_address;
        phy->spi_xfer.radio_cfg_out.frameproc_phasedata = RX_MODE | RPLYTXEN_0b1;
        phy->spi_xfer.radio_cfg_out.phy_0_1             = EXPECRP0_0b1 | RPLYADD0_0b0;
        phy->spi_xfer.radio_cfg_out.rx_tx_size          = SET_TXPKTSIZE(0);
        set_events_for_rx_with_ack(phy);
        enqueue_rx_prepare_frame_states(phy);
    } else {
        /* Nack mode */
        tx_address                                      = phy->xlayer_main->frame.source_address;
        phy->spi_xfer.radio_cfg_out.phy_0_1             = 0;
        phy->spi_xfer.radio_cfg_out.frameproc_phasedata = RX_MODE;
        set_events_for_rx_without_ack(phy);
        enqueue_rx_prepare_frame_states(phy);
    }

    phy->spi_xfer.radio_cfg_out.tx_address = SET_TXADDRESS(tx_address);
    phy->spi_xfer.radio_cfg_out.rx_tx_size |= SET_RXPKTSIZE(
        phy->xlayer_main->frame.payload_memory_size + phy->xlayer_main->frame.header_memory_size +
        HDR_SIZE_SIZE);

    phy->spi_xfer.radio_cfg_out.timelimit_biasdelay = SET_TIMEOUT(
        TIMEOUT_VAL2RAW(phy->config->rx_timeout));

    /* Disable CCA */
    phy->spi_xfer.radio_cfg_out.cca_thres_gain = SET_CCATHRES(DISABLE_CCA_THRES);
    phy->spi_xfer.radio_cfg_out.actions = FLUSHTX_0b1 | FLUSHRX_0b1;
    phy->spi_xfer.radio_cfg_out.rx_address = SET_RXADDRESS(phy->local_address);
}

/** @brief State : prepare the radio to send or receive a frame.
 *
 *  @param[in] signal_data  Data required to process the state. The type shall be wps_phy_t.
 */
static void prepare_radio(wps_phy_t *phy)
{
    phy->signal_auto   = PHY_SIGNAL_NONE;
    phy->radio_actions = 0;

    sr_reg_pattern_t *pattern = &phy->config->channel->reg_pattern;

    phy->spi_xfer.radio_cfg_out.if_bb_gain_lna = pattern->if_baseband_gain_lna;
    phy->spi_xfer.radio_cfg_out.rxbandfre_cfg1freq = pattern->rxbandfre_cfg1freq;
    phy->spi_xfer.radio_cfg_out.cfg2freq_cfg3freq = pattern->cfg2freq_cfg3freq;
    phy->spi_xfer.radio_cfg_out.cfg_widths_txpwr_randpulse = pattern->cfg_widths_txpwr_randpulse;
    phy->spi_xfer.radio_cfg_out.tx_pulse_pos = pattern->tx_pulse_pos;

    if (main_is_tx(phy)) {
        prepare_radio_tx(phy);
    } else {
        prepare_radio_rx(phy);
    }

    phy->spi_xfer.radio_cfg_out.slpperiod_15_0 = SET_SLPPERIOD_15_0(phy->config->sleep_time);
    phy->spi_xfer.radio_cfg_out.slpperiod_pwrupdlay =
        SET_SLPPERIOD_23_16(phy->config->sleep_time >> 16) |
        SET_PWRUPDLAY(PWRUPDELAY_VAL2RAW(phy->config->power_up_delay));

    phy->spi_xfer.radio_cfg_out.rf_gain_manu = MANUGAIN_DEFAULT | SET_PKTRFGAIN(0);
    phy->spi_xfer.radio_cfg_out.actions |= SLEEP_0b1;

    phy->spi_xfer.radio_cfg_out.phy_0_1 |= phy->config->fec | phy->config->modulation | phy->config->chip_repet |
                                           SET_ISIMITIG0(phy->config->isi_mitig);

#if WPS_RADIO_COUNT == 2
    /* Deactivate autowake before setting radio to sleep. */
    phy->spi_xfer.read_events_out.addr_timercfg_sleepcfg = REG_WRITE | REG16_TIMERCFG_SLEEPCFG;
    phy->spi_xfer.read_events_out.set_timercfg_sleepcfg = phy->config->sleep_level | SLPTIMEO_0b1 | SLPTXEND_0b1 |
                                                          SLPRXEND_0b1;
#endif
}

/** @brief State : Send the Radio config through the SPI.
 *
 *  @param[in] signal_data Data required to process the state. The type shall be wps_phy_t.
 */
static void set_config(wps_phy_t *phy)
{
    uint8_t pwr_state;

    /* Changing sleep level while PROC_ON is 1 causes issues for some sleep level transitions.
     * We need to wait until PROC_ON is 0 before changing sleep level.
     * It is also recommended to change the sleep level before the sleep period.
     */
    if (((phy->config->sleep_level != SLEEP_IDLE) && (phy->config->next_sleep_level == SLEEP_IDLE)) ||
        ((phy->config->sleep_level == SLEEP_SHALLOW) && (phy->config->next_sleep_level == SLEEP_DEEP)) ||
        ((phy->config->sleep_level == SLEEP_DEEP) && (phy->config->next_sleep_level == SLEEP_SHALLOW))) {
        pwr_state = sr_access_read_reg8(phy->radio->radio_id, REG8_POWER_STATE);
        while (GET_PROC_ON(pwr_state)) {
            pwr_state = sr_access_read_reg8(phy->radio->radio_id, REG8_POWER_STATE);
        }
    }

    /* When switching from chip clock to XTAL clock timer, reset XTAL clock timer on wake up*/
    if ((phy->config->sleep_level == SLEEP_IDLE) && (phy->config->next_sleep_level != SLEEP_IDLE)) {
        phy->spi_xfer.radio_cfg_out.timercfg_sleepcfg |= SYNWAKUP_0b1;
    }

    sr_access_write_reg16(phy->radio->radio_id, REG16_TIMERCFG_SLEEPCFG, phy->spi_xfer.radio_cfg_out.timercfg_sleepcfg);

    phy->signal_main = PHY_SIGNAL_PREPARE_DONE;
    sr_access_spi_transfer_non_blocking(phy->radio->radio_id, (uint8_t *)&phy->spi_xfer.radio_cfg_out,
                                        (uint8_t *)&phy->spi_xfer.spi_dummy_buffer, sizeof(radio_cfg_t));
}

/** @brief State : Fill the header of the frame in the radio tx fifo.
 *
 *  @param[in] signal_data  Data required to process the state. The type shall be wps_phy_t.
 */
static void set_header(wps_phy_t *phy)
{
    xlayer_frame_t *frame = main_is_tx(phy) ? &phy->xlayer_main->frame : &phy->xlayer_auto->frame;
    uint8_t hdr_len       = frame->header_end_it - frame->header_begin_it;

    sr_access_disable_radio_irq(phy->radio->radio_id);

    phy->signal_main = PHY_SIGNAL_YIELD;

    phy->spi_xfer.fill_header_out.data_fifo[0] = hdr_len;
    memcpy(&phy->spi_xfer.fill_header_out.data_fifo[1], frame->header_begin_it, hdr_len);
    sr_access_spi_transfer_non_blocking(phy->radio->radio_id, (uint8_t *)&phy->spi_xfer.fill_header_out,
                                        (uint8_t *)&phy->spi_xfer.spi_dummy_buffer,
                                        hdr_len + HDR_SIZE_SIZE + EMPTY_BYTE);
}

/** @brief State : Fill the payload of the frame in the radio tx fifo.
 *
 *  @param[in] signal_data  Data required to process the state. The type shall be wps_phy_t.
 */
static void set_payload(wps_phy_t *phy)
{
    xlayer_frame_t *frame = main_is_tx(phy) ? &phy->xlayer_main->frame : &phy->xlayer_auto->frame;

    if (phy->input_signal != PHY_SIGNAL_DMA_CMPLT) {
        phy->signal_main = PHY_SIGNAL_ERROR;
        return;
    }

    phy->signal_main = PHY_SIGNAL_YIELD;

    sr_access_spi_transfer_non_blocking(phy->radio->radio_id, frame->payload_begin_it, phy->spi_xfer.spi_dummy_buffer,
                                        frame->payload_end_it - frame->payload_begin_it);
}

/** @brief State: Fills the radio TX FIFO with the frame header and payload
 *         as a single SPI operation. This process uses a continuous memory
 *         area from the xlayer frame data for SPI communication.
 *
 *  @param[in] signal_data  Data required to process the state. The type shall be wps_phy_t.
 */
static void set_header_and_payload(wps_phy_t *phy)
{
    if (phy->input_signal != PHY_SIGNAL_DMA_CMPLT) {
        phy->signal_main = PHY_SIGNAL_ERROR;
        return;
    }

    xlayer_frame_t *frame = main_is_tx(phy) ? &phy->xlayer_main->frame : &phy->xlayer_auto->frame;

    sr_access_disable_radio_irq(phy->radio->radio_id);

    phy->signal_main = PHY_SIGNAL_YIELD;

    uint8_t *spi_tx_fifo = frame->header_begin_it - XLAYER_QUEUE_SPI_COMM_ADDITIONAL_BYTES;
    uint8_t header_size  = frame->header_end_it - frame->header_begin_it;
    uint8_t payload_size = frame->payload_end_it - frame->payload_begin_it;

    /* Set register TXFIFO as burst mode to send whole frame */
    spi_tx_fifo[XLAYER_QUEUE_SPI_COMM_REG_POSITION_OFFSET] = REG_WRITE_BURST | REG8_FIFOS;
    /* Set header size */
    spi_tx_fifo[XLAYER_QUEUE_SPI_COMM_HEADER_SIZE_POSITION_OFFSET] = header_size;

    sr_access_spi_transfer_non_blocking(phy->radio->radio_id, spi_tx_fifo, phy->spi_xfer.spi_dummy_buffer,
                                        header_size + payload_size + XLAYER_QUEUE_SPI_COMM_ADDITIONAL_BYTES);
}

/** @brief State : Re-enable the radio when the data have been written on the radio.
 *
 *  @param[in] signal_data  Data required to process the state. The type shall be wps_phy_t.
 */
static void enable_radio_irq(wps_phy_t *phy)
{
    if (phy->input_signal != PHY_SIGNAL_DMA_CMPLT) {
        phy->signal_main = PHY_SIGNAL_ERROR;
        return;
    }
    phy->signal_main = PHY_SIGNAL_CONFIG_COMPLETE;
    sr_access_enable_radio_irq(phy->radio->radio_id);

    /* If we missed the rising edge, do a context switch */
    if (sr_access_read_irq_pin(phy->radio->radio_id)) {
        if (rx_lost(&phy->spi_xfer.read_events_in) && GET_RX_EN(phy->spi_xfer.read_events_in.pwr_status)) {
            /* #2: When a rx timeout occured and the RXEN bit of the transceiver was set, clear any pending IRQs. */
            sr_access_read_reg16(phy->radio->radio_id, REG16_IRQ);
        } else {
            sr_access_radio_context_switch(phy->radio->radio_id);
        }
    }
}

/** @brief State : Re-enable the radio when the data have been written on the radio.
 *
 *  @param[in] signal_data  Data required to process the state. The type shall be wps_phy_t.
 */
static void check_radio_irq(wps_phy_t *phy)
{
    /* irq pin is low, auto-reply has not finish to send */
    sr_access_enable_radio_irq(phy->radio->radio_id);
    if (!sr_access_read_irq_pin(phy->radio->radio_id)) {
        phy->signal_main = PHY_SIGNAL_YIELD;
    }
}

/** @brief State : Ask the radio for the irq flags after a radio interrupt.
 *
 *  @param[in] signal_data  Data required to process the state. The type shall be wps_phy_t.
 */
static void read_events(wps_phy_t *phy)
{
    if (phy->input_signal != PHY_SIGNAL_RADIO_IRQ) {
        phy->signal_main = PHY_SIGNAL_ERROR;
        return;
    }

    phy->signal_main = PHY_SIGNAL_YIELD;

    sr_access_spi_transfer_non_blocking(phy->radio->radio_id, (uint8_t *)&phy->spi_xfer.read_events_out,
                                        (uint8_t *)&phy->spi_xfer.read_events_in, sizeof(read_events_t));
}

/** @brief State : Ask the radio for the IRQ flags after a radio interrupt when syncing.
 *
 *  @param[in] signal_data  Data required to process the state. The type shall be wps_phy_t.
 */
static void read_events_syncing(wps_phy_t *phy)
{
    if (phy->input_signal != PHY_SIGNAL_RADIO_IRQ) {
        phy->signal_main = PHY_SIGNAL_ERROR;
        return;
    }

    sr_access_write_reg16(phy->radio->radio_id, REG16_TIMERCFG_SLEEPCFG,
                          SLPDEPTH_WAKEONCE_0b01 | SLPTIMEO_0b1 | SLPTXEND_0b1 | SLPRXEND_0b1 | AUTOWAKE_0b1);

    phy->signal_main = PHY_SIGNAL_YIELD;

    sr_access_spi_transfer_non_blocking(phy->radio->radio_id, (uint8_t *)&phy->spi_xfer.read_events_out,
                                        (uint8_t *)&phy->spi_xfer.read_events_in, sizeof(read_events_t));
}

/** @brief State : Read the IRQ flags and take action regarding of the outcome for TX.
 *
 *  @param[in] signal_data  Data required to process the state. The type shall be wps_phy_t.
 */
static void process_event_tx(wps_phy_t *phy)
{
    if (phy->input_signal != PHY_SIGNAL_DMA_CMPLT) {
        phy->signal_main = PHY_SIGNAL_ERROR;
        return;
    }

    phy->config->cca_try_count = GET_TXRETRIES(phy->spi_xfer.read_events_in.actions);

    /* Handle CCA fail */
    if (GET_CCAFAILI(phy->spi_xfer.read_events_in.irq)) {
        handle_cca_fail(phy);
        /* Handle autoreply reception */
    } else if (rx_good_auto_reply(&phy->spi_xfer.read_events_in)) {
        phy->xlayer_main->frame.frame_outcome = FRAME_SENT_ACK;
        if (phy->xlayer_auto != NULL) {
            phy->xlayer_auto->frame.frame_outcome = FRAME_RECEIVED;
        }
        handle_good_auto_reply(phy);
    } else if (rx_lost(&phy->spi_xfer.read_events_in)) {
        phy->xlayer_main->frame.frame_outcome = FRAME_SENT_ACK_LOST;
        if (phy->xlayer_auto != NULL) {
            phy->xlayer_auto->frame.frame_outcome = FRAME_LOST;
        }
        phy->signal_main = PHY_SIGNAL_FRAME_SENT_NACK;
        phy->signal_auto = PHY_SIGNAL_FRAME_MISSED;
        enqueue_states(phy, prepare_phy_states);
    } else if (rx_rejected_auto_reply(&phy->spi_xfer.read_events_in)) {
        phy->xlayer_main->frame.frame_outcome = FRAME_SENT_ACK_REJECTED;
        if (phy->xlayer_auto != NULL) {
            phy->xlayer_auto->frame.frame_outcome = FRAME_REJECTED;
        }
        phy->signal_main = PHY_SIGNAL_FRAME_SENT_NACK;
        phy->signal_auto = PHY_SIGNAL_FRAME_MISSED;
        enqueue_states(phy, prepare_phy_states);
        /* Handle TX */
    } else if (tx_complete(&phy->spi_xfer.read_events_in)) {
        phy->signal_main                      = PHY_SIGNAL_FRAME_SENT_NACK;
        phy->signal_auto                      = PHY_SIGNAL_FRAME_MISSED;
        phy->xlayer_main->frame.frame_outcome = FRAME_SENT_ACK_LOST;
        if (phy->xlayer_auto != NULL) {
            phy->xlayer_auto->frame.frame_outcome = FRAME_LOST;
        }
        enqueue_states(phy, prepare_phy_states);
    } else if (GET_WAKEUPI(phy->spi_xfer.read_events_in.irq)) {
        phy->signal_main                      = PHY_SIGNAL_FRAME_SENT_NACK;
        phy->signal_auto                      = PHY_SIGNAL_FRAME_MISSED;
        phy->xlayer_main->frame.frame_outcome = FRAME_WAIT;
        if (phy->xlayer_auto != NULL) {
            phy->xlayer_auto->frame.frame_outcome = FRAME_LOST;
        }
        enqueue_states(phy, prepare_phy_states);
    }
}

/** @brief State : Read the IRQ flags and take action regarding of the outcome for RX.
 *
 *  @param[in] signal_data  Data required to process the state. The type shall be wps_phy_t.
 */
static void process_event_rx(wps_phy_t *phy)
{
    if (phy->input_signal != PHY_SIGNAL_DMA_CMPLT) {
        phy->signal_main = PHY_SIGNAL_ERROR;
        return;
    }

    /* Handle RX frame */
    if (rx_good(&phy->spi_xfer.read_events_in)) {
        if (phy->xlayer_auto != NULL) {
            phy->xlayer_auto->frame.frame_outcome = FRAME_SENT_ACK;
        }
        phy->xlayer_main->frame.frame_outcome = FRAME_RECEIVED;
        handle_good_frame(phy);
    } else if (rx_lost(&phy->spi_xfer.read_events_in)) {
        /* #2: When a timeout occurs, check if the RXEN bit of the transceiver is set and if so, clear
         *     any pending IRQs and disable the transceiver interrupts to ensure proper operation.
         *     Interrupts will be automatically re-enabled in a subsequent stage of the state machine.
         */
        if (GET_RX_EN(phy->spi_xfer.read_events_in.pwr_status)) {
            sr_access_disable_radio_irq(phy->radio->radio_id);
            /* Wait for RX to turn off. */
            while (GET_RX_EN(sr_access_read_reg8(phy->radio->radio_id, REG8_POWER_STATE))) {};
        }
        if (phy->xlayer_auto != NULL) {
            phy->xlayer_auto->frame.frame_outcome = FRAME_SENT_ACK_LOST;
        }
        phy->xlayer_main->frame.frame_outcome = FRAME_LOST;
        phy->signal_auto = (phy->xlayer_auto != NULL) ? PHY_SIGNAL_FRAME_NOT_SENT :
                                                        PHY_SIGNAL_FRAME_SENT_NACK;
        phy->signal_main = PHY_SIGNAL_FRAME_MISSED;
        enqueue_states(phy, prepare_phy_states);
    } else if (rx_rejected(&phy->spi_xfer.read_events_in)) {
        if (phy->xlayer_auto != NULL) {
            phy->xlayer_auto->frame.frame_outcome = FRAME_SENT_ACK_REJECTED;
        }
        phy->xlayer_main->frame.frame_outcome = FRAME_REJECTED;
        phy->signal_auto = (phy->xlayer_auto != NULL) ? PHY_SIGNAL_FRAME_NOT_SENT :
                                                        PHY_SIGNAL_FRAME_SENT_NACK;
        phy->signal_main = PHY_SIGNAL_FRAME_MISSED;
        enqueue_states(phy, prepare_phy_states);
    }
}

/** @brief Handle a good frame received by the radio.
 *
 *  Ask the radio for RX wait time, RSSI, RNSI and payload size.
 *
 *  @param[in] signal_data  Data required to process the state. The type shall be wps_phy_t.
 */
static void handle_good_frame(wps_phy_t *phy)
{
    uint8_t transfer_size = sizeof(read_info_t);

    phy->signal_main = PHY_SIGNAL_YIELD;

    if (phy->xlayer_auto != NULL) {
        if (auto_is_tx(phy) && !GET_BRDCASTI(phy->spi_xfer.read_events_in.irq)) {
            phy->wait_for_ack_tx = true;
            if (!tx_complete_auto_reply(&phy->spi_xfer.read_events_in)) {
                /* Tx end is enable to wait the transmission of the autoreply.*/
                sr_access_write_reg16(phy->radio->radio_id, REG16_IRQ, ARTXENDE_0b1);
                sr_access_disable_radio_irq(phy->radio->radio_id);
            } else {
                phy->wait_for_ack_tx = false;
            }

            phy->signal_auto = PHY_SIGNAL_FRAME_SENT_NACK;
        }
    }
    phy->config->rx_cca_retry_count = sr_access_read_reg8(phy->radio->radio_id, REG8_FIFOS) & CCA_RETRYHDR_MASK;

    sr_access_spi_transfer_non_blocking(phy->radio->radio_id, (uint8_t *)&phy->spi_xfer.read_info_out,
                                        (uint8_t *)&phy->spi_xfer.read_info_in, transfer_size);
    enqueue_states(phy, get_frame_header_states);
}

/** @brief Handle a good auto reply received by the radio.
 *
 *  Ask the radio for RX wait time, RSSI, RNSI and payload size.
 *
 *  @param[in] signal_data  Data required to process the state. The type shall be wps_phy_t.
 */
static void handle_good_auto_reply(wps_phy_t *phy)
{
    uint8_t transfer_size = sizeof(read_info_t);

    phy->signal_main = PHY_SIGNAL_YIELD;

    sr_access_spi_transfer_non_blocking(phy->radio->radio_id, (uint8_t *)&phy->spi_xfer.read_info_out,
                                        (uint8_t *)&phy->spi_xfer.read_info_in, transfer_size);
    enqueue_states(phy, get_auto_reply_header_states);
}

/** @brief Handle a Clear Channel Assessment (CCA) fail.
 *
 *  @param[in] signal_data  Data required to process the state. The type shall be wps_phy_t.
 */
static void handle_cca_fail(wps_phy_t *phy)
{
    phy->signal_main                      = PHY_SIGNAL_FRAME_SENT_NACK;
    phy->xlayer_main->frame.frame_outcome = FRAME_WAIT;
    enqueue_states(phy, prepare_phy_states);
}

/** @brief State : Ask the radio to get the frame header.
 *
 *  @param[in] signal_data  Data required to process the state. The type shall be wps_phy_t.
 */
static void get_frame_header(wps_phy_t *phy)
{
    if (phy->input_signal != PHY_SIGNAL_DMA_CMPLT) {
        phy->signal_main = PHY_SIGNAL_ERROR;
        return;
    }

    uint8_t expected_frame_size = phy->config->expected_header_size +
                                  phy->config->expected_payload_size + HDR_SIZE_SIZE;

    phy->config->rx_wait_time = GET_RXSYNTIME(phy->spi_xfer.read_info_in.rxtime);

    if (phy->debug_cfg.phase_offset_stats_enable) {
        phy->config->phase_offset[0] = GET_PHASEDATA(
            phy->spi_xfer.read_info_in.frameproc_phasedata);
    }
    phy->config->rssi_raw = GET_RSSI(phy->spi_xfer.read_info_in.rssi_rnsi);
    phy->config->rnsi_raw = GET_RNSI(phy->spi_xfer.read_info_in.rssi_rnsi);

    if (phy->spi_xfer.read_info_in.data_frame_size == 0 ||
        (phy->spi_xfer.read_info_in.data_frame_size > expected_frame_size)) {
        phy->xlayer_main->frame.payload_end_it = phy->xlayer_main->frame.header_begin_it;
        phy->signal_auto                       = PHY_SIGNAL_FRAME_SENT_NACK;
        phy->signal_main                       = PHY_SIGNAL_FRAME_MISSED;
        enqueue_states(phy, prepare_phy_states);
    } else {
        phy->spi_xfer.read_info_in.data_frame_size -= HDR_SIZE_SIZE;
        phy->header_size = phy->spi_xfer.read_info_in.data_header_size;

        if (phy->header_size > phy->config->expected_header_size) {
            phy->xlayer_main->frame.payload_end_it = phy->xlayer_main->frame.header_begin_it;
            phy->signal_auto                       = PHY_SIGNAL_FRAME_SENT_NACK;
            phy->signal_main                       = PHY_SIGNAL_FRAME_MISSED;
            enqueue_states(phy, prepare_phy_states);
        } else {
            phy->signal_main = PHY_SIGNAL_YIELD;

            phy->xlayer_main->frame.header_begin_it = phy->xlayer_main->frame.header_memory;
            phy->xlayer_main->frame.payload_end_it   = phy->xlayer_main->frame.header_memory +
                                                     phy->header_size + EMPTY_BYTE;

            phy->spi_xfer.spi_dummy_buffer[0] = REG_READ_BURST | REG8_FIFOS;
            sr_access_spi_transfer_non_blocking(phy->radio->radio_id, phy->spi_xfer.spi_dummy_buffer,
                                                phy->xlayer_main->frame.header_memory, phy->header_size + EMPTY_BYTE);

            enqueue_states(phy, get_payload_states);
            enqueue_states(phy, prepare_phy_states);
        }
    }
}

/** @brief State : Ask the radio to get the auto reply header.
 *
 *  If the payload is empty, the user is notified.
 *
 *  @param[in] signal_data  Data required to process the state. The type shall be wps_phy_t.
 */
static void get_auto_reply_header(wps_phy_t *phy)
{
    if (phy->input_signal != PHY_SIGNAL_DMA_CMPLT) {
        phy->signal_main = PHY_SIGNAL_ERROR;
        return;
    }

    phy->config->rssi_raw = GET_RSSI(phy->spi_xfer.read_info_in.rssi_rnsi);
    phy->config->rnsi_raw = GET_RNSI(phy->spi_xfer.read_info_in.rssi_rnsi);

    if (phy->spi_xfer.read_info_in.data_frame_size == 0) {
        phy->signal_main = PHY_SIGNAL_FRAME_SENT_ACK;
        phy->signal_auto = PHY_SIGNAL_FRAME_MISSED;
        enqueue_states(phy, prepare_phy_states);
    } else {
        phy->spi_xfer.read_info_in.data_frame_size -= HDR_SIZE_SIZE;
        phy->header_size = phy->spi_xfer.read_info_in.data_header_size;
        phy->signal_main = PHY_SIGNAL_YIELD;

        phy->xlayer_auto->frame.header_begin_it = phy->xlayer_auto->frame.header_memory;
        phy->xlayer_auto->frame.payload_end_it   = phy->xlayer_auto->frame.header_memory +
                                                 phy->header_size + EMPTY_BYTE;

        phy->spi_xfer.spi_dummy_buffer[0] = REG_READ_BURST | REG8_FIFOS;
        sr_access_spi_transfer_non_blocking(phy->radio->radio_id, phy->spi_xfer.spi_dummy_buffer,
                                            phy->xlayer_auto->frame.header_memory, phy->header_size + EMPTY_BYTE);
        enqueue_states(phy, get_payload_states);
        enqueue_states(phy, prepare_phy_states);
    }
}

/** @brief State : Ask the radio to get the payload.
 *
 *  If the payload is empty, the user is notified.
 *
 *  @param[in] signal_data  Data required to process the state. The type shall be wps_phy_t.
 */
static void get_payload(wps_phy_t *phy)
{
    if (phy->input_signal != PHY_SIGNAL_DMA_CMPLT) {
        phy->signal_main = PHY_SIGNAL_ERROR;
        return;
    }

    uint8_t payload_size        = phy->spi_xfer.read_info_in.data_frame_size - phy->header_size;
    phy_output_signal_t *signal = main_is_tx(phy) ? &phy->signal_auto : &phy->signal_main;
    xlayer_frame_t *frame = main_is_tx(phy) ? &phy->xlayer_auto->frame : &phy->xlayer_main->frame;

    *signal = PHY_SIGNAL_FRAME_RECEIVED;

    /* Update frame payload data pointer */
    phy->config->update_payload_buffer(phy->mac, frame, payload_size);

    if (payload_size == 0) {
        sr_access_close(phy->radio->radio_id);
    } else {
        if (frame->payload_begin_it != NULL) {
            sr_access_spi_transfer_non_blocking(phy->radio->radio_id, phy->spi_xfer.spi_dummy_buffer,
                                                frame->payload_begin_it, payload_size);
            frame->payload_end_it = frame->payload_begin_it + payload_size;
            enqueue_states(phy, new_frame_states);
        } else {
            /* This situation can happen when more connections are assign to the same timeslot and there is no free
             * space in current RX connection.
             */
            sr_access_close(phy->radio->radio_id);
        }
    }
    if (phy->xlayer_auto != NULL) {
        if (auto_is_tx(phy) && phy->wait_for_ack_tx) {
            enqueue_states(phy, wait_to_send_auto_reply);
        } else {
            phy->signal_main = PHY_SIGNAL_FRAME_SENT_ACK;
        }
    }
}

/** @brief State : Close the spi after transmission or reception.
 *
 *  @param[in] signal_data  Data required to process the state. The type shall be wps_phy_t.
 */
static void close_spi(wps_phy_t *phy)
{
    if (phy->input_signal != PHY_SIGNAL_DMA_CMPLT) {
        phy->state_step--;
        phy->signal_main = PHY_SIGNAL_YIELD;
        return;
    }

    sr_access_close(phy->radio->radio_id);
}

/** @brief State : End of a state machine sequence.
 *
 *  @param[in] signal_data  Data required to process the state. The type shall be wps_phy_t.
 */
static void end(wps_phy_t *phy)
{
    wps_phy_state_t **dequeue_state;

    phy->state_step    = 0;
    dequeue_state      = (wps_phy_state_t **)circular_queue_front_raw(&phy->next_states);
    phy->current_state = *dequeue_state;
    circular_queue_dequeue_raw(&phy->next_states);
}

static void none(wps_phy_t *phy)
{
    (void)phy;
}

static void prepare_syncing(wps_phy_t *phy)
{
    uint16_t autoreply_settings = 0;

    phy->signal_main = PHY_SIGNAL_YIELD;

    phy->config->sleep_level = SLEEP_IDLE;

    sr_access_write_reg16(phy->radio->radio_id, REG16_RX_TX_SIZEREG, SET_TXPKTSIZE(0) | SET_RXPKTSIZE(MAX_FRAMESIZE));

    sr_access_write_reg16(phy->radio->radio_id, REG16_TXADDRESS, phy->xlayer_main->frame.source_address);

    sr_access_write_reg16(phy->radio->radio_id, REG16_RXADDRESS, SET_RXADDRESS(phy->local_address));
    /* Disable CCA */
    sr_access_write_reg16(phy->radio->radio_id, REG16_CCA_THRES_GAIN, SET_CCATHRES(DISABLE_CCA_THRES));

    if (phy->config->expect_ack) {
        autoreply_settings = EXPECRP0_0b1 | RPLYADD0_0b0;

        sr_access_write_reg16(phy->radio->radio_id, REG16_FRAMEPROC_PHASEDATA, RX_MODE | RPLYTXEN_0b1);
    } else {
        sr_access_write_reg16(phy->radio->radio_id, REG16_FRAMEPROC_PHASEDATA, RX_MODE);
    }

    sr_access_write_reg16(phy->radio->radio_id, REG16_PHY_0_1,
                          phy->config->fec | phy->config->modulation | phy->config->chip_repet | autoreply_settings);

    sr_access_write_reg16(phy->radio->radio_id, REG16_IRQ, RXENDE_0b1);

    sr_access_write_reg16(phy->radio->radio_id, REG16_TIMERCFG_SLEEPCFG,
                          SLEEP_IDLE_NO_WAKEONCE | AUTOWAKE_0b1 | SLPTIMEO_0b1 | SLPTXEND_0b1 | SLPRXEND_0b1 |
                              SYNRXSTA_0b1);

    fast_sync_config_non_stop_rx(phy);

    sr_access_write_reg8(phy->radio->radio_id, REG8_ACTIONS, FLUSHTX_0b1 | FLUSHRX_0b1 | SLEEP_0b1);

    sr_access_write_reg16(phy->radio->radio_id, REG16_RF_GAIN_MANUGAIN, MANUGAIN_DEFAULT | SET_PKTRFGAIN(0));

    sr_access_write_reg16(phy->radio->radio_id, REG16_IF_BASEBAND_GAIN_LNA,
                          phy->config->channel->reg_pattern.if_baseband_gain_lna);
    sr_access_write_reg16(phy->radio->radio_id, REG16_RXBANDFRE_CFG1FREQ,
                          phy->config->channel->reg_pattern.rxbandfre_cfg1freq);
    sr_access_write_reg16(phy->radio->radio_id, REG16_CFG2FREQ_CFG3FREQ,
                          phy->config->channel->reg_pattern.cfg2freq_cfg3freq);
    sr_access_write_reg16(phy->radio->radio_id, REG16_CFG_WIDTHS_TXPWR_RANDPULSE,
                          phy->config->channel->reg_pattern.cfg_widths_txpwr_randpulse);
    sr_access_write_reg16(phy->radio->radio_id, REG16_TX_PULSE_POS, phy->config->channel->reg_pattern.tx_pulse_pos);

    sr_access_enable_radio_irq(phy->radio->radio_id);
}

static void fast_sync_config_non_stop_rx(wps_phy_t *phy)
{
    sr_access_write_reg16(phy->radio->radio_id, REG16_SLPPERIOD_PWRUPDLAY,
                          SET_PWRUPDLAY(PWRUPDELAY_VAL2RAW(phy->config->power_up_delay)) |
                              SET_SLPPERIOD_23_16(MAX_SLP_TIME_VAL_8));
    sr_access_write_reg16(phy->radio->radio_id, REG16_SLPPERIOD_15_0, SET_SLPPERIOD_15_0(FAST_SYNC_IDLE_SLEEP_VAL));
    sr_access_write_reg16(phy->radio->radio_id, REG16_SLPPERIOD_PWRUPDLAY,
                          SET_PWRUPDLAY(PWRUPDELAY_VAL2RAW(phy->config->power_up_delay)) |
                              SET_SLPPERIOD_23_16(FAST_SYNC_IDLE_SLEEP_VAL >> 16));
    sr_access_write_reg16(phy->radio->radio_id, REG16_TIMELIMIT_BIASDELAY,
                          SET_TIMEOUT(TIMEOUT_VAL2RAW(FAST_SYNC_IDLE_TIMEOUT_VAL)));
}

/** @brief Get if the main frame is in transmit mode.
 *
 *  @param[in] phy Layer one instance.
 *  @retval true
 *  @retval false
 */
static bool main_is_tx(wps_phy_t *phy)
{
    return (phy->xlayer_main->frame.destination_address != phy->local_address);
}

/** @brief Get if the main frame is in transmit mode.
 *
 *  @param[in] phy Layer one instance.
 *  @retval true
 *  @retval false
 */
static bool auto_is_tx(wps_phy_t *phy)
{
    return (phy->xlayer_auto->frame.destination_address != phy->local_address);
}

/** @brief Get the TX complete status.
 *
 *  @param[in] read_events
 *  @retval true
 *  @retval false
 */
static bool tx_complete(read_events_t *read_events)
{
    return ((GET_TXENDI(read_events->irq) && !GET_RXENDI(read_events->irq) &&
             !GET_TIMEOUTI(read_events->irq)) ||
            GET_TXUDRFLI(read_events->irq));
}

/** @brief Get the TX complete of auto-reply frame status.
 *
 *  @note This is slightly different then tx_complete, since
 *        NEW_PACKET_IT is trigger during reception of frame, so
 *        only event to check is TX_END_IT
 *
 *  @param[in] read_events
 *  @retval true
 *  @retval false
 */
static bool tx_complete_auto_reply(read_events_t *read_events)
{
    return (GET_TXENDI(read_events->irq) && !GET_TIMEOUTI(read_events->irq)) ||
           GET_TXUDRFLI(read_events->irq);
}

/** @brief Get the RX good status.
 *
 *  @param[in] read_events
 *  @retval true
 *  @retval false
 */
static bool rx_good(read_events_t *read_events)
{
    return GET_RXENDI(read_events->irq) && GET_CRCPASSI(read_events->irq) &&
           (GET_ADDRMATI(read_events->irq) || GET_BRDCASTI(read_events->irq));
}

/** @brief Get the RX good status for auto replies.
 *
 *  @param[in] read_events
 *  @retval true
 *  @retval false
 */
static bool rx_good_auto_reply(read_events_t *read_events)
{
    return GET_ARRXENDI(read_events->irq) && GET_CRCPASSI(read_events->irq) &&
           (GET_ADDRMATI(read_events->irq) || GET_BRDCASTI(read_events->irq));
}

/** @brief Get the RX rejected status.
 *
 *  @param[in] read_events
 *  @retval true
 *  @retval false
 */
static bool rx_rejected(read_events_t *read_events)
{
    return GET_RXENDI(read_events->irq) &&
           (!GET_CRCPASSI(read_events->irq) ||
            !(GET_ADDRMATI(read_events->irq) || GET_BRDCASTI(read_events->irq)));
}

/** @brief Get the RX rejected status for auto replies.
 *
 *  @param[in] read_events
 *  @retval true
 *  @retval false
 */
static bool rx_rejected_auto_reply(read_events_t *read_events)
{
    return GET_ARRXENDI(read_events->irq) &&
           (!GET_CRCPASSI(read_events->irq) ||
            !(GET_ADDRMATI(read_events->irq) || GET_BRDCASTI(read_events->irq)));
}

/** @brief Get the RX lost status.
 *
 *  @param[in] read_events
 *  @retval true
 *  @retval false
 */
static bool rx_lost(read_events_t *read_events)
{
    return GET_TIMEOUTI(read_events->irq) && !GET_RXENDI(read_events->irq);
}

/** @brief Set the events for tx with ack.
 *
 *  @param[in] phy Layer one instance.
 */
static void set_events_for_tx_with_ack(wps_phy_t *phy)
{
    phy->spi_xfer.radio_cfg_out.irq = ARRXENDE_0b1 | TIMEOUTE_0b1 | CCAFAILE_0b1;
}

/** @brief Set the events for tx without ack.
 *
 *  @param[in] phy Layer one instance.
 */
static void set_events_for_tx_without_ack(wps_phy_t *phy)
{
    phy->spi_xfer.radio_cfg_out.irq = TXENDE_0b1 | CCAFAILE_0b1;
}

/** @brief Set the events for rx with ack.
 *
 *  @param[in] phy Layer one instance.
 */
static void set_events_for_rx_with_ack(wps_phy_t *phy)
{
    phy->spi_xfer.radio_cfg_out.irq = RXENDE_0b1 | TIMEOUTE_0b1;
}

/** @brief Set the events for rx without ack.
 *
 *  @param[in] phy Layer one instance.
 */
static void set_events_for_rx_without_ack(wps_phy_t *phy)
{
    phy->spi_xfer.radio_cfg_out.irq = RXENDE_0b1 | TIMEOUTE_0b1;
}

/** @brief Set the events for rx with payload.
 *
 *  @param[in] phy Layer one instance.
 */
static void set_events_for_wakeup_only(wps_phy_t *phy)
{
    phy->spi_xfer.radio_cfg_out.irq = WAKEUPE_0b1;
}

/** @brief Set the events for empty TX.
 *
 *  @param[in] phy Layer one instance.
 */
static void set_events_for_empty_tx(wps_phy_t *phy)
{
    phy->spi_xfer.radio_cfg_out.if_bb_gain_lna = 0;
    phy->spi_xfer.radio_cfg_out.rxbandfre_cfg1freq = 0;
    phy->spi_xfer.radio_cfg_out.cfg2freq_cfg3freq = 0;
    phy->spi_xfer.radio_cfg_out.cfg_widths_txpwr_randpulse = 0;
    phy->spi_xfer.radio_cfg_out.tx_pulse_pos = 0;
    phy->spi_xfer.radio_cfg_out.irq = TXENDE_0b1;
}

static void init_transfer_structures(wps_phy_t *wps_phy)
{
    wps_phy->spi_xfer.radio_cfg_out.addr_actions           = REG_WRITE | REG8_ACTIONS;
    wps_phy->spi_xfer.radio_cfg_out.addr_rx_address        = REG_WRITE | REG16_RXADDRESS;
    wps_phy->spi_xfer.radio_cfg_out.addr_tx_address        = REG_WRITE | REG16_TXADDRESS;
    wps_phy->spi_xfer.radio_cfg_out.addr_rx_tx_size        = REG_WRITE | REG16_RX_TX_SIZEREG;
    wps_phy->spi_xfer.radio_cfg_out.addr_phy_0_1           = REG_WRITE | REG16_PHY_0_1;
    wps_phy->spi_xfer.radio_cfg_out.burst_write_start_addr = REG_WRITE_BURST | REG16_CCA_SETTINGS;

    wps_phy->spi_xfer.fill_header_out.addr_fifo = REG_WRITE_BURST | REG8_FIFOS;

    wps_phy->spi_xfer.read_events_out.addr_pwr_status = REG8_POWER_STATE;
    wps_phy->spi_xfer.read_events_out.addr_irq            = REG16_IRQ;
    wps_phy->spi_xfer.read_events_out.addr_set_actions    = REG_WRITE | REG8_ACTIONS;
    wps_phy->spi_xfer.read_events_out.set_actions         = SLEEP_0b1;
    wps_phy->spi_xfer.read_events_out.addr_actions        = REG8_ACTIONS;
    /* Disable IRQ sources to be sure it does not trigger after a failed reception. */
    wps_phy->spi_xfer.read_events_out.addr_set_irq        = REG_WRITE | REG16_IRQ;
    wps_phy->spi_xfer.read_events_out.set_irq             = 0;

    wps_phy->spi_xfer.read_info_out.addr_frame_size       = REG8_FIFOS;
    wps_phy->spi_xfer.read_info_out.addr_header_size      = REG8_FIFOS;
    wps_phy->spi_xfer.read_info_out.burst_read_start_addr = REG_READ_BURST |
                                                            REG16_FRAMEPROC_PHASEDATA;
}
