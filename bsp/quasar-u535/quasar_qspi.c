/** @file  quasar_qspi.c
 *  @brief This module configure QSPI and provides functions to transmit and receive.
 *
 *  @copyright Copyright (C) 2026 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_qspi.h"
#include "quasar_def.h"

/* CONSTANTS ******************************************************************/
/* Value to be in the CR register for an indirect write operation. */
#define INDIRECT_WRITE_OP ((uint32_t)0x00000000)

/* PRIVATE GLOBALS ************************************************************/
static OSPI_HandleTypeDef qspi_handle_ospi1 = {.Instance = OCTOSPI1};
#if defined(STM32U535xx)
static OSPI_HandleTypeDef qspi_handle_ospi2 = {.Instance = OCTOSPI1};
#else
static OSPI_HandleTypeDef qspi_handle_ospi2 = {.Instance = OCTOSPI2};
#endif
OSPI_HandleTypeDef *const qspi_handle_table[] = {&qspi_handle_ospi1, &qspi_handle_ospi2};

static OSPI_RegularCmdTypeDef qspi_cmd_ospi1 = {
    .InstructionMode = HAL_OSPI_INSTRUCTION_NONE,
    .Instruction = 0,
    .AddressSize = HAL_OSPI_ADDRESS_8_BITS,
    .AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE,
    .AlternateBytes = 0,
    .AlternateBytesSize = 0,
    .DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE,
    .SIOOMode = HAL_OSPI_SIOO_INST_ONLY_FIRST_CMD,
    .DummyCycles = 0,
};

static OSPI_RegularCmdTypeDef qspi_cmd_ospi2 = {
    .InstructionMode = HAL_OSPI_INSTRUCTION_NONE,
    .Instruction = 0,
    .AddressSize = HAL_OSPI_ADDRESS_8_BITS,
    .AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE,
    .AlternateBytes = 0,
    .AlternateBytesSize = 0,
    .DataDtrMode = HAL_OSPI_DATA_DTR_DISABLE,
    .SIOOMode = HAL_OSPI_SIOO_INST_ONLY_FIRST_CMD,
    .DummyCycles = 0,
};

/* MACROS *********************************************************************/
/* Return the QSPI cmd from quasar_qspi_selection_t. */
#define QUASAR_QSPI_GET_SELECTED_CMD(qspi_selection) \
    ((OSPI_RegularCmdTypeDef *)((qspi_selection == QUASAR_QSPI_SELECTION_OSPI1) ? &qspi_cmd_ospi1 : &qspi_cmd_ospi2))

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void qspi_configure_and_enable_clock(quasar_qspi_selection_t qspi_selection);
static void default_irq_callback(void);
static void (*octospi1_irq_callback)(void) = default_irq_callback;
static void (*octospi2_irq_callback)(void) = default_irq_callback;
static void quasar_qspi_cmd(OSPI_HandleTypeDef *hospi, OSPI_RegularCmdTypeDef *cmd);
static void quasar_qspi_receive(OSPI_HandleTypeDef *hospi, uint8_t *pData);
static void quasar_qspi_transmit(OSPI_HandleTypeDef *hospi, uint8_t *pData);
static void quasar_qspi_setup(OSPI_HandleTypeDef *hospi, uint8_t command, uint8_t size);
static void quasar_DMA_cmplt_callback(DMA_HandleTypeDef *hdma);
static void quasar_qspi_receive_dma(OSPI_HandleTypeDef *hospi, uint8_t *pData);
static void quasar_qspi_transmit_dma(OSPI_HandleTypeDef *hospi, uint8_t *pData);

/* PUBLIC FUNCTIONS ***********************************************************/
void quasar_qspi_init(quasar_qspi_config_t qspi_config)
{
    /* Initialize GPIOs related to QSPI. */
    quasar_gpio_init(qspi_config.gpio_config_sck);
    quasar_gpio_init(qspi_config.gpio_config_io_0);
    quasar_gpio_init(qspi_config.gpio_config_io_1);
    quasar_gpio_init(qspi_config.gpio_config_io_2);
    quasar_gpio_init(qspi_config.gpio_config_io_3);

    /* Configure and enable the clock related to QSPI.  */
    qspi_configure_and_enable_clock(qspi_config.qspi_selection);

    /* Configure and initialize QSPI peripheral. */
    OSPI_HandleTypeDef *qspi_handle = QUASAR_QSPI_GET_SELECTED_HANDLE(qspi_config.qspi_selection);
    HAL_OSPI_DLYB_CfgTypeDef HAL_OSPI_DLYB_Cfg_Struct = {0};

    /* OCTOSPI parameter configuration */
    qspi_handle->Init.FifoThreshold = 1;
    qspi_handle->Init.DualQuad = HAL_OSPI_DUALQUAD_DISABLE;
    qspi_handle->Init.MemoryType = HAL_OSPI_MEMTYPE_MICRON;
    qspi_handle->Init.DeviceSize = 32;
    qspi_handle->Init.ChipSelectHighTime = 1;
    qspi_handle->Init.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE;
    qspi_handle->Init.ClockMode = HAL_OSPI_CLOCK_MODE_0;
    qspi_handle->Init.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED;
    qspi_handle->Init.ClockPrescaler = qspi_config.qspi_prescaler;
    qspi_handle->Init.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_NONE;
    qspi_handle->Init.DelayHoldQuarterCycle = HAL_OSPI_DHQC_DISABLE;
    qspi_handle->Init.ChipSelectBoundary = 0;
    qspi_handle->Init.DelayBlockBypass = HAL_OSPI_DELAY_BLOCK_USED;
    qspi_handle->Init.MaxTran = 0;
    qspi_handle->Init.Refresh = 0;
    if (HAL_OSPI_Init(qspi_handle) != HAL_OK) {
        while (1);
    }

    HAL_OSPI_DLYB_Cfg_Struct.Units = 0;
    HAL_OSPI_DLYB_Cfg_Struct.PhaseSel = 0;
    if (HAL_OSPI_DLYB_SetConfig(qspi_handle, &HAL_OSPI_DLYB_Cfg_Struct) != HAL_OK) {
        while (1);
    }

    if (qspi_config.irq_priority != QUASAR_IRQ_PRIORITY_NONE) {
        IRQn_Type qspi_irq = QUASAR_QSPI_GET_SELECTED_IRQ(qspi_config.qspi_selection);

        /* Set the QSPI priority. */
        NVIC_SetPriority(qspi_irq, qspi_config.irq_priority);
    }
}

void quasar_qspi_enable_interrupt(quasar_qspi_selection_t qspi_selection)
{
    IRQn_Type qspi_irq = QUASAR_QSPI_GET_SELECTED_IRQ(qspi_selection);

    /* Enable the QSPI interrupt. */
    NVIC_EnableIRQ(qspi_irq);
}

void quasar_qspi_disable_interrupt(quasar_qspi_selection_t qspi_selection)
{
    IRQn_Type qspi_irq = QUASAR_QSPI_GET_SELECTED_IRQ(qspi_selection);
    OSPI_HandleTypeDef *qspi_handle = QUASAR_QSPI_GET_SELECTED_HANDLE(qspi_selection);

    /* Clean pending IRQ, used for Radio transfers. */
    QUASAR_SET_BIT(qspi_handle->Instance->FCR, OCTOSPI_FCR_CTCF_Msk);

    /* Disable the QSPI interrupt. */
    NVIC_DisableIRQ(qspi_irq);
}

void quasar_qspi_set_mode(quasar_qspi_selection_t qspi_selection, quasar_qspi_mode_t mode)
{
    OSPI_HandleTypeDef *qspi_handle = QUASAR_QSPI_GET_SELECTED_HANDLE(qspi_selection);
    OSPI_RegularCmdTypeDef *qspi_cmd = QUASAR_QSPI_GET_SELECTED_CMD(qspi_selection);

    switch (mode) {
    case QUASAR_QSPI_1_LINE_MODE_0_DUMMY:
        qspi_cmd->DummyCycles = 0;
        qspi_cmd->InstructionMode = HAL_OSPI_INSTRUCTION_NONE;
        qspi_cmd->AddressMode = HAL_OSPI_ADDRESS_1_LINE;
        qspi_cmd->AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
        qspi_cmd->DataMode = HAL_OSPI_DATA_1_LINE;
        break;
    case QUASAR_QSPI_4_LINES_MODE_0_DUMMY:
        qspi_cmd->DummyCycles = 0;
        qspi_cmd->InstructionMode = HAL_OSPI_INSTRUCTION_NONE;
        qspi_cmd->AddressMode = HAL_OSPI_ADDRESS_4_LINES;
        qspi_cmd->AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
        qspi_cmd->DataMode = HAL_OSPI_DATA_4_LINES;
        break;
    case QUASAR_QSPI_4_LINES_MODE_1_DUMMY:
        qspi_cmd->DummyCycles = 1;
        qspi_cmd->InstructionMode = HAL_OSPI_INSTRUCTION_NONE;
        qspi_cmd->AddressMode = HAL_OSPI_ADDRESS_4_LINES;
        qspi_cmd->AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
        qspi_cmd->DataMode = HAL_OSPI_DATA_4_LINES;
        break;
    default:
        /* Unimplemented mode. */
        break;
    }

    qspi_cmd->Address = 0;
    qspi_cmd->NbData = 1;

    quasar_qspi_cmd(qspi_handle, qspi_cmd);
}

void quasar_qspi_transfer_half_duplex_rx_blocking(quasar_qspi_selection_t qspi_selection, uint8_t command,
                                                  uint8_t *data, uint8_t size, uint16_t timeout)
{
    OSPI_HandleTypeDef *qspi_handle = QUASAR_QSPI_GET_SELECTED_HANDLE(qspi_selection);
    (void)timeout;

    quasar_qspi_setup(qspi_handle, command, size);
    quasar_qspi_receive(qspi_handle, data);
}

void quasar_qspi_transfer_half_duplex_tx_blocking(quasar_qspi_selection_t qspi_selection, uint8_t command,
                                                  uint8_t *data, uint8_t size, uint16_t timeout)
{
    OSPI_HandleTypeDef *qspi_handle = QUASAR_QSPI_GET_SELECTED_HANDLE(qspi_selection);
    (void)timeout;

    quasar_qspi_setup(qspi_handle, command, size);
    quasar_qspi_transmit(qspi_handle, data);
}

void quasar_qspi_transfer_half_duplex_rx_non_blocking(quasar_qspi_selection_t qspi_selection, uint8_t command,
                                                      uint8_t *data, uint8_t size)
{
    OSPI_HandleTypeDef *qspi_handle = QUASAR_QSPI_GET_SELECTED_HANDLE(qspi_selection);

    quasar_qspi_setup(qspi_handle, command, size);
    quasar_qspi_receive_dma(qspi_handle, data);
}

void quasar_qspi_transfer_half_duplex_tx_non_blocking(quasar_qspi_selection_t qspi_selection, uint8_t command,
                                                      uint8_t *data, uint8_t size)
{
    OSPI_HandleTypeDef *qspi_handle = QUASAR_QSPI_GET_SELECTED_HANDLE(qspi_selection);

    quasar_qspi_setup(qspi_handle, command, size);
    quasar_qspi_transmit_dma(qspi_handle, data);
}

void quasar_qspi_set_qspi1_callback(void (*irq_callback)(void))
{
    octospi1_irq_callback = irq_callback;
}

void quasar_qspi_set_qspi2_callback(void (*irq_callback)(void))
{
    octospi2_irq_callback = irq_callback;
}

/* PRIVATE FUNCTIONS **********************************************************/
/** @brief Configure and enable the clock for the selected QSPI.
 *
 *  @param[in] qspi_selection  Selected QSPI to enable the clock.
 */
static void qspi_configure_and_enable_clock(quasar_qspi_selection_t qspi_selection)
{
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /* Initializes the peripherals clock. */
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_OSPI;
    PeriphClkInit.OspiClockSelection = RCC_OSPICLKSOURCE_SYSCLK;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        while (1);
    }

#if !defined(STM32U535xx)
    __HAL_RCC_OSPIM_CLK_ENABLE();
#endif

    switch (qspi_selection) {
    case QUASAR_QSPI_SELECTION_OSPI1:
#if !defined(STM32U535xx)
        __HAL_RCC_GPIOF_CLK_ENABLE();
#endif
        __HAL_RCC_OSPI1_CLK_ENABLE();
        break;
#if !defined(STM32U535xx)
    case QUASAR_QSPI_SELECTION_OSPI2:
        __HAL_RCC_GPIOH_CLK_ENABLE();
        __HAL_RCC_GPIOI_CLK_ENABLE();
        __HAL_RCC_OSPI2_CLK_ENABLE();
        break;
#endif
    default:
        /* Unimplemented timer. */
        break;
    }
}

/** @brief Default IRQ callback to prevent page fault.
 */
static void default_irq_callback(void)
{
    return;
}

/** @brief Set the QSPI command configuration.
 *
 *  @note This method is an optimized version of the HAL_OSPI_Command method.
 *
 *  @param[in] hospi  OSPI Handle.
 *  @param[in] cmd    Structure that contains the command configuration information.
 */
static void quasar_qspi_cmd(OSPI_HandleTypeDef *hospi, OSPI_RegularCmdTypeDef *cmd)
{
    while ((__HAL_OSPI_GET_FLAG(hospi, HAL_OSPI_FLAG_BUSY)) != RESET) {}
    __IO uint32_t *ccr_reg;
    __IO uint32_t *wccr_reg;
    __IO uint32_t *tcr_reg;

    /* Re-initialize the value of the functional mode. */
    MODIFY_REG(hospi->Instance->CR, OCTOSPI_CR_FMODE, 0U);

    /* Configure the flash ID - DUAL QUAD disable. */
    MODIFY_REG(hospi->Instance->CR, OCTOSPI_CR_MSEL, cmd->FlashId);

    /* Setup for default operation type (0x00000000U). */
    ccr_reg = &(hospi->Instance->CCR);
    wccr_reg = &(hospi->Instance->WCCR);
    tcr_reg = &(hospi->Instance->TCR);

    /* Configure the CCR register with DQS and SIOO modes. */
    *ccr_reg = (cmd->DQSMode | cmd->SIOOMode);

    /* Configure the TCR register with the number of dummy cycles. */
    MODIFY_REG((*tcr_reg), OCTOSPI_TCR_DCYC, cmd->DummyCycles);

    /* Configure the DLR register with the number of data. */
    hospi->Instance->DLR = (cmd->NbData - 1U);

    /* ---- Command with address and data ---- */

    /* Configure the CCR register with all communication parameters. */
    MODIFY_REG((*ccr_reg),
               (OCTOSPI_CCR_IMODE | OCTOSPI_CCR_ADMODE | OCTOSPI_CCR_ADDTR | OCTOSPI_CCR_ADSIZE | OCTOSPI_CCR_ABMODE |
                OCTOSPI_CCR_DMODE | OCTOSPI_CCR_DDTR),
               (cmd->InstructionMode | cmd->AddressMode | cmd->AddressDtrMode | cmd->AddressSize |
                cmd->AlternateBytesMode | cmd->DataMode | cmd->DataDtrMode));

    /* Configure the Write CCR register with all communication parameters. */
    MODIFY_REG((*wccr_reg),
               (OCTOSPI_CCR_IMODE | OCTOSPI_CCR_ADMODE | OCTOSPI_CCR_ADDTR | OCTOSPI_CCR_ADSIZE | OCTOSPI_CCR_ABMODE |
                OCTOSPI_CCR_DMODE | OCTOSPI_CCR_DDTR),
               (cmd->InstructionMode | cmd->AddressMode | cmd->AddressDtrMode | cmd->AddressSize |
                cmd->AlternateBytesMode | cmd->DataMode | cmd->DataDtrMode));

    /* Configure the AR register with the instruction value. */
    hospi->Instance->AR = cmd->Address;
}

/** @brief Receive an amount of data in blocking mode.
 *
 *  @note This method is an optimized version of the HAL_OSPI_Receive method.
 *
 *  @note Reception size shall be initialized through the QSPI command.
 *
 *  @param[in] hospi  OSPI handle.
 *  @param[in] pData  Pointer to data buffer.
 */
static void quasar_qspi_receive(OSPI_HandleTypeDef *hospi, uint8_t *pData)
{
    __IO uint32_t *data_reg = &hospi->Instance->DR;
    uint32_t addr_reg = hospi->Instance->AR;

    /* Configure counters and size. */
    uint32_t xfer_count = READ_REG(hospi->Instance->DLR) + 1U;

    /* Configure CR register with functional mode as indirect read. */
    MODIFY_REG(hospi->Instance->CR, OCTOSPI_CR_FMODE, ((uint32_t)OCTOSPI_CR_FMODE_0));

    /* Trig the transfer by re-writing address or instruction register. */
    WRITE_REG(hospi->Instance->AR, addr_reg);
    do {
        /* Wait till fifo threshold or transfer complete flags are set to read received data. */
        while ((__HAL_OSPI_GET_FLAG(hospi, (HAL_OSPI_FLAG_FT | HAL_OSPI_FLAG_TC))) != SET) {}
        *pData = *((__IO uint8_t *)data_reg);
        pData++;
    } while ((--xfer_count) > 0U);

    /* Wait till transfer complete flag is set to go back in idle state. */
    while ((__HAL_OSPI_GET_FLAG(hospi, (HAL_OSPI_FLAG_TC))) != SET) {}
    /* Clear transfer complete flag. */
    __HAL_OSPI_CLEAR_FLAG(hospi, HAL_OSPI_FLAG_TC);
}

/** @brief Transmit an amount of data in blocking mode.
 *
 *  @note This method is an optimized version of the HAL_OSPI_Transmit method.
 *
 *  @note Transmission size shall be initialized through the QSPI command.
 *
 *  @param[in] hospi  OSPI handle.
 *  @param[in] pData  Pointer to data buffer.
 */
static void quasar_qspi_transmit(OSPI_HandleTypeDef *hospi, uint8_t *pData)
{
    __IO uint32_t *data_reg = &hospi->Instance->DR;

    /* Configure counters and size. */
    uint32_t xfer_count = READ_REG(hospi->Instance->DLR) + 1U;

    /* Configure CR register with functional mode as indirect write. */
    MODIFY_REG(hospi->Instance->CR, OCTOSPI_CR_FMODE, INDIRECT_WRITE_OP);

    do {
        /* Wait till fifo threshold flag is set to send data. */
        while ((__HAL_OSPI_GET_FLAG(hospi, (HAL_OSPI_FLAG_FT))) != SET) {}
        *((__IO uint8_t *)data_reg) = *pData;
        pData++;
    } while ((--xfer_count) > 0U);

    /* Wait till transfer complete flag is set to go back in idle state. */
    while ((__HAL_OSPI_GET_FLAG(hospi, (HAL_OSPI_FLAG_TC))) != SET) {}

    /* Clear transfer complete flag. */
    __HAL_OSPI_CLEAR_FLAG(hospi, HAL_OSPI_FLAG_TC);
}

/** @brief Receive an amount of data in non-blocking mode with DMA.
 *
 *  @note This method is an optimized version of the HAL_OSPI_Receive_DMA method.
 *
 *  @note Reception size shall be initialized through the QSPI command.
 *
 *  @param[in] hospi  OSPI handle.
 *  @param[in] pData  Pointer to data buffer.
 */
static void quasar_qspi_receive_dma(OSPI_HandleTypeDef *hospi, uint8_t *pData)
{
    uint32_t data_size = hospi->Instance->DLR + 1U;
    uint32_t addr_reg = hospi->Instance->AR;

    hospi->XferCount = data_size;
    hospi->pBuffPtr = pData;

    /* Configure CR register with functional mode as indirect read. */
    MODIFY_REG(hospi->Instance->CR, OCTOSPI_CR_FMODE, ((uint32_t)OCTOSPI_CR_FMODE_0));

    /* Clear flags related to interrupt. */
    __HAL_OSPI_CLEAR_FLAG(hospi, HAL_OSPI_FLAG_TE | HAL_OSPI_FLAG_TC);

    /* Enable the Transfer Complete interrupt. */
    QUASAR_SET_BIT(hospi->Instance->CR, OCTOSPI_CR_TCIE);

    /* Update the state. */
    hospi->State = HAL_OSPI_STATE_BUSY_RX;

    /* Set the DMA transfer complete callback. */
    hospi->hdma->XferCpltCallback = quasar_DMA_cmplt_callback;

    /* Set the DMA Half transfer complete callback. */
    hospi->hdma->XferHalfCpltCallback = NULL;

    /* Set the DMA error callback. */
    hospi->hdma->XferErrorCallback = NULL;

    /* Clear the DMA abort callback. */
    hospi->hdma->XferAbortCallback = NULL;

    /* Configure the direction of the DMA. */
    MODIFY_REG(hospi->hdma->Instance->CTR1, (DMA_CTR1_SINC | DMA_CTR1_DINC), (DMA_SINC_FIXED | DMA_DINC_INCREMENTED));
    MODIFY_REG(hospi->hdma->Instance->CTR2, DMA_CTR2_DREQ, DMA_PERIPH_TO_MEMORY);

    /* Update the DMA channel state. */
    hospi->hdma->State = HAL_DMA_STATE_BUSY;

    /* Set DMA config. */
    /* Configure the DMA channel data size. */
    MODIFY_REG(hospi->hdma->Instance->CBR1, DMA_CBR1_BNDT, (data_size & DMA_CBR1_BNDT));

    /* Clear all interrupt flags. */
    __HAL_DMA_CLEAR_FLAG(hospi->hdma, DMA_FLAG_TC | DMA_FLAG_HT | DMA_FLAG_DTE | DMA_FLAG_ULE | DMA_FLAG_USE |
                                          DMA_FLAG_SUSP | DMA_FLAG_TO);

    /* Configure DMA channel source address. */
    hospi->hdma->Instance->CSAR = (uint32_t)&hospi->Instance->DR;

    /* Configure DMA channel destination address. */
    hospi->hdma->Instance->CDAR = (uint32_t)pData;

    /* Enable common interrupts: Transfer Complete and Transfer Errors ITs. */
    __HAL_DMA_ENABLE_IT(hospi->hdma, (DMA_IT_TC | DMA_IT_DTE | DMA_IT_ULE | DMA_IT_USE | DMA_IT_TO));

    /* Enable DMA channel. */
    __HAL_DMA_ENABLE(hospi->hdma);

    /* Enable the transfer error interrupt. */
    __HAL_OSPI_ENABLE_IT(hospi, HAL_OSPI_IT_TE);

    /* Trig the transfer by re-writing address or instruction register. */
    WRITE_REG(hospi->Instance->AR, addr_reg);

    /* Enable the DMA transfer by setting the DMAEN bit . */
    SET_BIT(hospi->Instance->CR, OCTOSPI_CR_DMAEN);
}

/** @brief Send an amount of data in non-blocking mode with DMA.
 *
 *  @note This method is an optimized version of the HAL_OSPI_Transmit_DMA method.
 *
 *  @note Transmission size shall be initialized through the QSPI command.
 *
 *  @param[in] hospi  OSPI handle.
 *  @param[in] pData  Pointer to data buffer.
 */
static void quasar_qspi_transmit_dma(OSPI_HandleTypeDef *hospi, uint8_t *pData)
{
    uint32_t data_size = hospi->Instance->DLR + 1U;

    hospi->pBuffPtr = pData;

    /* Configure CR register with functional mode as indirect write. */
    MODIFY_REG(hospi->Instance->CR, OCTOSPI_CR_FMODE, 0);

    /* Clear flags related to interrupt. */
    __HAL_OSPI_CLEAR_FLAG(hospi, HAL_OSPI_FLAG_TE | HAL_OSPI_FLAG_TC);

    /* Enable the Transfer Complete interrupt. */
    QUASAR_SET_BIT(hospi->Instance->CR, OCTOSPI_CR_TCIE);

    /* Update the state. */
    hospi->State = HAL_OSPI_STATE_BUSY_TX;

    /* Set the DMA transfer complete callback. */
    hospi->hdma->XferCpltCallback = quasar_DMA_cmplt_callback;
    hospi->hdma->XferHalfCpltCallback = NULL;
    hospi->hdma->XferErrorCallback = NULL;
    hospi->hdma->XferAbortCallback = NULL;

    /* Configure the direction of the DMA. */
    MODIFY_REG(hospi->hdma->Instance->CTR1, (DMA_CTR1_SINC | DMA_CTR1_DINC), (DMA_SINC_INCREMENTED | DMA_DINC_FIXED));
    MODIFY_REG(hospi->hdma->Instance->CTR2, DMA_CTR2_DREQ, DMA_MEMORY_TO_PERIPH);

    /* Update the DMA channel state. */
    hospi->hdma->State = HAL_DMA_STATE_BUSY;

    /* Update the DMA channel error code. */
    hospi->hdma->ErrorCode = HAL_DMA_ERROR_NONE;

    /* Configure the DMA channel data size. */
    MODIFY_REG(hospi->hdma->Instance->CBR1, DMA_CBR1_BNDT, (data_size & DMA_CBR1_BNDT));

    /* Clear all interrupt flags. */
    __HAL_DMA_CLEAR_FLAG(hospi->hdma, DMA_FLAG_TC | DMA_FLAG_HT | DMA_FLAG_DTE | DMA_FLAG_ULE | DMA_FLAG_USE |
                                          DMA_FLAG_SUSP | DMA_FLAG_TO);

    /* Configure DMA channel source address. */
    hospi->hdma->Instance->CSAR = (uint32_t)pData;

    /* Configure DMA channel destination address. */
    hospi->hdma->Instance->CDAR = (uint32_t)&hospi->Instance->DR;

    /* Enable common interrupts: Transfer Complete and Transfer Errors ITs. */
    __HAL_DMA_ENABLE_IT(hospi->hdma, (DMA_IT_TC | DMA_IT_DTE | DMA_IT_ULE | DMA_IT_USE | DMA_IT_TO));

    /* Enable DMA channel. */
    __HAL_DMA_ENABLE(hospi->hdma);

    /* Enable the transfer error interrupt. */
    __HAL_OSPI_ENABLE_IT(hospi, HAL_OSPI_IT_TE);

    /* Enable the DMA transfer by setting the DMAEN bit . */
    SET_BIT(hospi->Instance->CR, OCTOSPI_CR_DMAEN);
}

static void quasar_qspi_setup(OSPI_HandleTypeDef *hospi, uint8_t command, uint8_t size)
{
    /* Block if requested read size is 0. This should never happen and user's app should block if it does. */
    while (size == 0);

    while ((__HAL_OSPI_GET_FLAG(hospi, HAL_OSPI_FLAG_BUSY)) != RESET) {}

    /* Re-initialize the value of the functional mode. */
    MODIFY_REG(hospi->Instance->CR, OCTOSPI_CR_FMODE, 0U);

    /* Configure the DLR register with the number of data. */
    hospi->Instance->DLR = (size - 1U);

    /* Configure the AR register with the instruction value. */
    hospi->Instance->AR = command;
}

/** @brief DMA OSPI process complete callback.
 *
 *  @note Local implementation of the OSPI_DMACplt
 *
 *  @param[in] hdma  DMA handle.
 */
static void quasar_DMA_cmplt_callback(DMA_HandleTypeDef *hdma)
{
    OSPI_HandleTypeDef *hospi = (OSPI_HandleTypeDef *)(hdma->Parent);

    hospi->XferCount = 0;
    /* Disable the DMA transfer on the OctoSPI side. */
    CLEAR_BIT(hospi->Instance->CR, OCTOSPI_CR_DMAEN);

    /* Disable the DMA channel. */
    __HAL_DMA_DISABLE(hdma);

    /* Enable the OSPI transfer complete Interrupt. */
    __HAL_OSPI_ENABLE_IT(hospi, HAL_OSPI_IT_TC);
}

/* ST HAL FUNCTIONS IMPLEMENTATION ********************************************/
/** @brief This function handles OCTOSPI 1 interrupt.
 *
 *  @note This handler is use for the DMA transmission performed by
 *        the wireless core when the mode is QSPI.
 */
void OCTOSPI1_IRQHandler(void)
{
    uint32_t sr_reg = OCTOSPI1->SR;

    if (QUASAR_READ_BIT(sr_reg, OCTOSPI_SR_TCF_Pos)) {
        QUASAR_SET_BIT(OCTOSPI1->FCR, OCTOSPI_FCR_CTCF_Msk);

        /* Disable the Transfer Complete interrupt. */
        QUASAR_CLEAR_BIT(qspi_handle_ospi1.Instance->CR, OCTOSPI_CR_TCIE);

        /* Handle the callback only on TX, DMA takes care of RX. */
        if (qspi_handle_ospi1.State == HAL_OSPI_STATE_BUSY_TX) {
            octospi1_irq_callback();
        }
    }
}

#if !defined(STM32U535xx)
/** @brief This function handles OCTOSPI 2 interrupt.
 */
void OCTOSPI2_IRQHandler(void)
{
    uint32_t sr_reg = OCTOSPI2->SR;

    if (QUASAR_READ_BIT(sr_reg, OCTOSPI_SR_TCF_Pos)) {
        QUASAR_SET_BIT(OCTOSPI2->FCR, OCTOSPI_FCR_CTCF_Msk);

        /* Disable the Transfer Complete interrupt. */
        QUASAR_CLEAR_BIT(qspi_handle_ospi2.Instance->CR, OCTOSPI_CR_TCIE);

        /* Handle the callback only on TX, DMA takes care of RX. */
        if (qspi_handle_ospi2.State == HAL_OSPI_STATE_BUSY_TX) {
            octospi2_irq_callback();
        }
    }
}
#endif
