/** @file  quasar_it.c
 *  @brief This module includes some necessary CortexM33 exception implementations as well as some STM32
 *  specific interrupts.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */

/* INCLUDES *******************************************************************/
#include "quasar_it.h"
#include "quasar_def.h"
#include "stm32u5xx_hal_dma.h"

/* EXTERNS ********************************************************************/
extern DMA_HandleTypeDef hdma_sai_rx;
extern DMA_HandleTypeDef hdma_sai_tx;

/* PRIVATE FUNCTION PROTOTYPES ************************************************/
static void default_irq_callback(void);

/* PRIVATE GLOBALS ************************************************************/
static void (*exti0_irq_callback)(void)  = default_irq_callback;
static void (*exti1_irq_callback)(void)  = default_irq_callback;
static void (*exti2_irq_callback)(void)  = default_irq_callback;
static void (*exti3_irq_callback)(void)  = default_irq_callback;
static void (*exti4_irq_callback)(void)  = default_irq_callback;
static void (*exti5_irq_callback)(void)  = default_irq_callback;
static void (*exti6_irq_callback)(void)  = default_irq_callback;
static void (*exti7_irq_callback)(void)  = default_irq_callback;
static void (*exti8_irq_callback)(void)  = default_irq_callback;
static void (*exti9_irq_callback)(void)  = default_irq_callback;
static void (*exti10_irq_callback)(void) = default_irq_callback;
static void (*exti11_irq_callback)(void) = default_irq_callback;
static void (*exti12_irq_callback)(void) = default_irq_callback;
static void (*exti13_irq_callback)(void) = default_irq_callback;
static void (*exti14_irq_callback)(void) = default_irq_callback;
static void (*exti15_rising_edge_irq_callback)(void) = default_irq_callback;
static void (*exti15_falling_edge_irq_callback)(void) = default_irq_callback;

static void (*pendsv_irq_callback)(void) = default_irq_callback;
static void (*usb_irq_callback)(void)    = default_irq_callback;

static void (*timer1_callback)(void)  = default_irq_callback;
static void (*timer2_callback)(void)  = default_irq_callback;
static void (*timer3_callback)(void)  = default_irq_callback;
static void (*timer4_callback)(void)  = default_irq_callback;
static void (*timer5_callback)(void)  = default_irq_callback;
static void (*timer6_callback)(void)  = default_irq_callback;
static void (*timer7_callback)(void)  = default_irq_callback;
static void (*timer8_callback)(void)  = default_irq_callback;
static void (*timer15_callback)(void) = default_irq_callback;
static void (*timer16_callback)(void) = default_irq_callback;
static void (*timer17_callback)(void) = default_irq_callback;

static volatile uint32_t nested_critical;

/* PUBLIC FUNCTIONS ***********************************************************/
void quasar_it_set_exti0_irq_callback(void (*irq_callback)(void))
{
    exti0_irq_callback = irq_callback;
}

void quasar_it_set_exti1_irq_callback(void (*irq_callback)(void))
{
    exti1_irq_callback = irq_callback;
}

void quasar_it_set_exti2_irq_callback(void (*irq_callback)(void))
{
    exti2_irq_callback = irq_callback;
}

void quasar_it_set_exti3_irq_callback(void (*irq_callback)(void))
{
    exti3_irq_callback = irq_callback;
}

void quasar_it_set_exti4_irq_callback(void (*irq_callback)(void))
{
    exti4_irq_callback = irq_callback;
}

void quasar_it_set_exti5_irq_callback(void (*irq_callback)(void))
{
    exti5_irq_callback = irq_callback;
}

void quasar_it_set_exti6_irq_callback(void (*irq_callback)(void))
{
    exti6_irq_callback = irq_callback;
}

void quasar_it_set_exti7_irq_callback(void (*irq_callback)(void))
{
    exti7_irq_callback = irq_callback;
}

void quasar_it_set_exti8_irq_callback(void (*irq_callback)(void))
{
    exti8_irq_callback = irq_callback;
}

void quasar_it_set_exti9_irq_callback(void (*irq_callback)(void))
{
    exti9_irq_callback = irq_callback;
}

void quasar_it_set_exti10_irq_callback(void (*irq_callback)(void))
{
    exti10_irq_callback = irq_callback;
}

void quasar_it_set_exti11_irq_callback(void (*irq_callback)(void))
{
    exti11_irq_callback = irq_callback;
}

void quasar_it_set_exti12_irq_callback(void (*irq_callback)(void))
{
    exti12_irq_callback = irq_callback;
}

void quasar_it_set_exti13_irq_callback(void (*irq_callback)(void))
{
    exti13_irq_callback = irq_callback;
}

void quasar_it_set_exti14_irq_callback(void (*irq_callback)(void))
{
    exti14_irq_callback = irq_callback;
}

void quasar_it_set_rising_edge_exti15_irq_callback(void (*irq_callback)(void))
{
    exti15_rising_edge_irq_callback = irq_callback;
}

void quasar_it_set_falling_edge_exti15_irq_callback(void (*irq_callback)(void))
{
    exti15_falling_edge_irq_callback = irq_callback;
}

void quasar_it_set_pendsv_callback(void (*irq_callback)(void))
{
    pendsv_irq_callback = irq_callback;
}

void quasar_it_set_usb_irq_callback(void (*irq_callback)(void))
{
    usb_irq_callback = irq_callback;
}

void quasar_it_set_timer1_callback(void (*irq_callback)(void))
{
    timer1_callback = irq_callback;
}

void quasar_it_set_timer2_callback(void (*irq_callback)(void))
{
    timer2_callback = irq_callback;
}

void quasar_it_set_timer3_callback(void (*irq_callback)(void))
{
    timer3_callback = irq_callback;
}

void quasar_it_set_timer4_callback(void (*irq_callback)(void))
{
    timer4_callback = irq_callback;
}

void quasar_it_set_timer5_callback(void (*irq_callback)(void))
{
    timer5_callback = irq_callback;
}

void quasar_it_set_timer6_callback(void (*irq_callback)(void))
{
    timer6_callback = irq_callback;
}

void quasar_it_set_timer7_callback(void (*irq_callback)(void))
{
    timer7_callback = irq_callback;
}

void quasar_it_set_timer8_callback(void (*irq_callback)(void))
{
    timer8_callback = irq_callback;
}

void quasar_it_set_timer15_callback(void (*irq_callback)(void))
{
    timer15_callback = irq_callback;
}

void quasar_it_set_timer16_callback(void (*irq_callback)(void))
{
    timer16_callback = irq_callback;
}

void quasar_it_set_timer17_callback(void (*irq_callback)(void))
{
    timer17_callback = irq_callback;
}

void quasar_it_enter_critical(void)
{
    if (!nested_critical) {
        /* First time enter critical */
        __disable_irq();
    }
    ++nested_critical;
}

void quasar_it_exit_critical(void)
{
    --nested_critical;
    if (!nested_critical) {
        /* Last time exit critical */
        __enable_irq();
    }
}

/* ST HAL FUNCTIONS IMPLEMENTATION ********************************************/
/** @brief Implementation of weak alias for GPDMA1 Channel 7 IRQ handler.
 *
 *  Implementation for the SAI TX transfer complete Exception handler.
 */
void GPDMA1_Channel7_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_sai_tx);
}

/** @brief Implementation of weak alias for GPDMA1 Channel 8 IRQ handler.
 *
 *  Implementation for the SAI RX reception complete Exception handler.
 */
void GPDMA1_Channel8_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_sai_rx);
}

/** @brief Implementation of weak alias for EXTI0_IRQ handler.
 */
void EXTI0_IRQHandler(void)
{
    /* Clear the rising and falling edge flags. */
    QUASAR_SET_BIT(EXTI->RPR1, EXTI_RPR1_RPIF0_Msk);
    QUASAR_SET_BIT(EXTI->FPR1, EXTI_FPR1_FPIF0_Msk);
    exti0_irq_callback();
}

/** @brief Implementation of weak alias for EXTI1_IRQ handler.
 */
void EXTI1_IRQHandler(void)
{
    /* Clear the rising and falling edge flags. */
    QUASAR_SET_BIT(EXTI->RPR1, EXTI_RPR1_RPIF1_Msk);
    QUASAR_SET_BIT(EXTI->FPR1, EXTI_FPR1_FPIF1_Msk);
    exti1_irq_callback();
}

/** @brief Implementation of weak alias for EXTI2_IRQ handler.
 */
void EXTI2_IRQHandler(void)
{
    /* Clear the rising and falling edge flags. */
    QUASAR_SET_BIT(EXTI->RPR1, EXTI_RPR1_RPIF2_Msk);
    QUASAR_SET_BIT(EXTI->FPR1, EXTI_FPR1_FPIF2_Msk);
    exti2_irq_callback();
}

/** @brief Implementation of weak alias for EXTI3_IRQ handler.
 */
void EXTI3_IRQHandler(void)
{
    /* Clear the rising and falling edge flags. */
    QUASAR_SET_BIT(EXTI->RPR1, EXTI_RPR1_RPIF3_Msk);
    QUASAR_SET_BIT(EXTI->FPR1, EXTI_FPR1_FPIF3_Msk);
    exti3_irq_callback();
}

/** @brief Implementation of weak alias for EXTI4_IRQ handler.
 */
void EXTI4_IRQHandler(void)
{
    /* Clear the rising and falling edge flags. */
    QUASAR_SET_BIT(EXTI->RPR1, EXTI_RPR1_RPIF4_Msk);
    QUASAR_SET_BIT(EXTI->FPR1, EXTI_FPR1_FPIF4_Msk);
    exti4_irq_callback();
}

/** @brief Implementation of weak alias for EXTI5_IRQ handler.
 */
void EXTI5_IRQHandler(void)
{
    /* Clear the rising and falling edge flags. */
    QUASAR_SET_BIT(EXTI->RPR1, EXTI_RPR1_RPIF5_Msk);
    QUASAR_SET_BIT(EXTI->FPR1, EXTI_FPR1_FPIF5_Msk);
    exti5_irq_callback();
}

/** @brief Implementation of weak alias for EXTI6_IRQ handler.
 */
void EXTI6_IRQHandler(void)
{
    /* Clear the rising and falling edge flags. */
    QUASAR_SET_BIT(EXTI->RPR1, EXTI_RPR1_RPIF6_Msk);
    QUASAR_SET_BIT(EXTI->FPR1, EXTI_FPR1_FPIF6_Msk);
    exti6_irq_callback();
}

/** @brief Implementation of weak alias for EXTI7_IRQ handler.
 */
void EXTI7_IRQHandler(void)
{
    /* Clear the rising and falling edge flags. */
    QUASAR_SET_BIT(EXTI->RPR1, EXTI_RPR1_RPIF7_Msk);
    QUASAR_SET_BIT(EXTI->FPR1, EXTI_FPR1_FPIF7_Msk);
    exti7_irq_callback();
}

/** @brief Implementation of weak alias for EXTI8_IRQ handler.
 */
void EXTI8_IRQHandler(void)
{
    /* Clear the rising and falling edge flags. */
    QUASAR_SET_BIT(EXTI->RPR1, EXTI_RPR1_RPIF8_Msk);
    QUASAR_SET_BIT(EXTI->FPR1, EXTI_FPR1_FPIF8_Msk);
    exti8_irq_callback();
}

/** @brief Implementation of weak alias for EXTI9_IRQ handler.
 */
void EXTI9_IRQHandler(void)
{
    /* Clear the rising and falling edge flag. */
    QUASAR_SET_BIT(EXTI->RPR1, EXTI_RPR1_RPIF9_Msk);
    QUASAR_SET_BIT(EXTI->FPR1, EXTI_FPR1_FPIF9_Msk);
    exti9_irq_callback();
}

/** @brief Implementation of weak alias for EXTI10_IRQ handler.
 */
void EXTI10_IRQHandler(void)
{
    /* Clear the rising and falling edge flag. */
    QUASAR_SET_BIT(EXTI->RPR1, EXTI_RPR1_RPIF10_Msk);
    QUASAR_SET_BIT(EXTI->FPR1, EXTI_FPR1_FPIF10_Msk);
    exti10_irq_callback();
}

/** @brief Implementation of weak alias for EXTI11_IRQ handler.
 */
void EXTI11_IRQHandler(void)
{
    /* Clear the rising and falling edge flag. */
    QUASAR_SET_BIT(EXTI->RPR1, EXTI_RPR1_RPIF11_Msk);
    QUASAR_SET_BIT(EXTI->FPR1, EXTI_FPR1_FPIF11_Msk);
    exti11_irq_callback();
}

/** @brief Implementation of weak alias for EXTI12_IRQ handler.
 */
void EXTI12_IRQHandler(void)
{
    /* Clear the rising and falling edge flag. */
    QUASAR_SET_BIT(EXTI->RPR1, EXTI_RPR1_RPIF12_Msk);
    QUASAR_SET_BIT(EXTI->FPR1, EXTI_FPR1_FPIF12_Msk);
    exti12_irq_callback();
}

/** @brief Implementation of weak alias for EXTI13_IRQ handler.
 */
void EXTI13_IRQHandler(void)
{
    /* Clear the rising and falling edge flag. */
    QUASAR_SET_BIT(EXTI->RPR1, EXTI_RPR1_RPIF13_Msk);
    QUASAR_SET_BIT(EXTI->FPR1, EXTI_FPR1_FPIF13_Msk);
    exti13_irq_callback();
}

/** @brief Implementation of weak alias for EXTI14_IRQ handler.
 */
void EXTI14_IRQHandler(void)
{
    /* Clear the rising and falling edge flag. */
    QUASAR_SET_BIT(EXTI->RPR1, EXTI_RPR1_RPIF14_Msk);
    QUASAR_SET_BIT(EXTI->FPR1, EXTI_FPR1_FPIF14_Msk);
    exti14_irq_callback();
}

/** @brief Implementation of weak alias for EXTI15_IRQ handler.
 */
void EXTI15_IRQHandler(void)
{
    uint32_t rising_edge = 0;
    uint32_t falling_edge = 0;

    rising_edge = QUASAR_READ_BIT(EXTI->RPR1, EXTI_RPR1_RPIF15_Pos);
    falling_edge = QUASAR_READ_BIT(EXTI->FPR1, EXTI_FPR1_FPIF15_Pos);

    /* Clear the rising and falling edge flag. */
    QUASAR_SET_BIT(EXTI->RPR1, EXTI_RPR1_RPIF15_Msk);
    QUASAR_SET_BIT(EXTI->FPR1, EXTI_FPR1_FPIF15_Msk);

    if (rising_edge) {
        exti15_rising_edge_irq_callback();
    }

    if (falling_edge) {
        exti15_falling_edge_irq_callback();
    }
}

/** @brief Implementation of weak alias for OTG_HS_IRQ handler.
 */
void OTG_HS_IRQHandler(void)
{
    usb_irq_callback();
}

/** @brief This function handles Timer 1 interrupt.
 */
void TIM1_UP_IRQHandler(void)
{
    /* Check whether the interruption is linked to an update (end of cycle) */
    if ((TIM1->SR & TIM_SR_UIF) != 0) {
        /* Clear the interruption flag */
        TIM1->SR = ~((uint16_t) TIM_SR_UIF);
        timer1_callback();
    }
}

/** @brief This function handles Timer 2 interrupt.
 */
void TIM2_IRQHandler(void)
{
    /* Check whether the interruption is linked to an update (end of cycle) */
    if ((TIM2->SR & TIM_SR_UIF) != 0) {
        /* Clear the interruption flag */
        TIM2->SR = ~((uint16_t) TIM_SR_UIF);
        timer2_callback();
    }
}

/** @brief This function handles Timer 3 interrupt.
 */
void TIM3_IRQHandler(void)
{
    /* Check whether the interruption is linked to an update (end of cycle) */
    if ((TIM3->SR & TIM_SR_UIF) != 0) {
        /* Clear the interruption flag */
        TIM3->SR = ~((uint16_t) TIM_SR_UIF);
        timer3_callback();
    }
}

/** @brief This function handles Timer 4 interrupt.
 */
void TIM4_IRQHandler(void)
{
    /* Check whether the interruption is linked to an update (end of cycle) */
    if ((TIM4->SR & TIM_SR_UIF) != 0) {
        /* Clear the interruption flag */
        TIM4->SR = ~((uint16_t) TIM_SR_UIF);
        timer4_callback();
    }
}

/** @brief This function handles Timer 5 interrupt.
 */
void TIM5_IRQHandler(void)
{
    /* Check whether the interruption is linked to an update (end of cycle) */
    if ((TIM5->SR & TIM_SR_UIF) != 0) {
        /* Clear the interruption flag */
        TIM5->SR = ~((uint16_t) TIM_SR_UIF);
        timer5_callback();
    }
}

/** @brief This function handles Timer 6 interrupt.
 */
void TIM6_IRQHandler(void)
{
    /* Check whether the interruption is linked to an update (end of cycle) */
    if ((TIM6->SR & TIM_SR_UIF) != 0) {
        /* Clear the interruption flag */
        TIM6->SR = ~((uint16_t) TIM_SR_UIF);
        timer6_callback();
    }
}

/** @brief This function handles Timer 7 interrupt.
 */
void TIM7_IRQHandler(void)
{
    /* Check whether the interruption is linked to an update (end of cycle) */
    if ((TIM7->SR & TIM_SR_UIF) != 0) {
        /* Clear the interruption flag */
        TIM7->SR = ~((uint16_t) TIM_SR_UIF);
        timer7_callback();
    }
}

/** @brief This function handles Timer 8 interrupt.
 */
void TIM8_UP_IRQHandler(void)
{
    /* Check whether the interruption is linked to an update (end of cycle) */
    if ((TIM8->SR & TIM_SR_UIF) != 0) {
        /* Clear the interruption flag */
        TIM8->SR = ~((uint16_t) TIM_SR_UIF);
        timer8_callback();
    }
}

/** @brief This function handles Timer 15 interrupt.
 */
void TIM15_IRQHandler(void)
{
    /* Check whether the interruption is linked to an update (end of cycle) */
    if ((TIM15->SR & TIM_SR_UIF) != 0) {
        /* Clear the interruption flag */
        TIM15->SR = ~((uint16_t) TIM_SR_UIF);
        timer15_callback();
    }
}

/** @brief This function handles Timer 16 interrupt.
 */
void TIM16_IRQHandler(void)
{
    /* Check whether the interruption is linked to an update (end of cycle) */
    if ((TIM16->SR & TIM_SR_UIF) != 0) {
        /* Clear the interruption flag */
        TIM16->SR = ~((uint16_t) TIM_SR_UIF);
        timer16_callback();
    }
}

/** @brief This function handles Timer 17 interrupt.
 */
void TIM17_IRQHandler(void)
{
    /* Check whether the interruption is linked to an update (end of cycle) */
    if ((TIM17->SR & TIM_SR_UIF) != 0) {
        /* Clear the interruption flag */
        TIM17->SR = ~((uint16_t) TIM_SR_UIF);
        timer17_callback();
    }
}

/** @brief Implementation of weak alias for Exception Handler.
 */
void Error_Handler(void)
{
    while (1);
}

/** @brief Implementation of weak alias for Exception Handler.
 */
void HardFault_Handler(void)
{
    while (1);
}

/** @brief Implementation of weak alias for MemManage handler.
 */
void MemManage_Handler(void)
{
    while (1);
}

/** @brief Implementation of weak alias for BusFault Handler.
 */
void BusFault_Handler(void)
{
    while (1);
}

/** @brief Implementation of weak alias for UsageFault Handler.
 */
void UsageFault_Handler(void)
{
    while (1);
}

#if !defined(RTOS_ENABLED)

/** @brief This function handles pendable request for system service.
 *
 */
void PendSV_Handler(void)
{
    CLEAR_BIT(SCB->ICSR, SCB_ICSR_PENDSVSET_Msk);
    pendsv_irq_callback();
}

/** @brief Implementation of weak alias for SysTick Handler.
 *
 */
void SysTick_Handler(void)
{
}

#endif

/* PRIVATE FUNCTION ***********************************************************/
/** @brief Default IRQ callback to prevent page fault.
 */
static void default_irq_callback(void)
{
    return;
}
