/** @file quasar_it.h
 *  @brief This module includes some necessary CortexM33 exception implementations as well as some STM32
 *  specific interrupts.
 *
 *  @copyright Copyright (C) 2023 SPARK Microsystems International Inc. All rights reserved.
 *  @license   This source code is proprietary and subject to the SPARK Microsystems
 *             Software EULA found in this package in file EULA.txt.
 *  @author    SPARK FW Team.
 */
#ifndef QUASAR_IT_H_
#define QUASAR_IT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* TYPES **********************************************************************/
/** @brief List of the available interrupt request priorities.
 */
typedef enum quasar_irq_priority {
    /*! Interrupt request priority 0 (highest priority). */
    QUASAR_IRQ_PRIORITY_0  = 0,
    /*! Interrupt request priority 1. */
    QUASAR_IRQ_PRIORITY_1  = 1,
    /*! Interrupt request priority 2. */
    QUASAR_IRQ_PRIORITY_2  = 2,
    /*! Interrupt request priority 3. */
    QUASAR_IRQ_PRIORITY_3  = 3,
    /*! Interrupt request priority 4. */
    QUASAR_IRQ_PRIORITY_4  = 4,
    /*! Interrupt request priority 5. */
    QUASAR_IRQ_PRIORITY_5  = 5,
    /*! Interrupt request priority 6. */
    QUASAR_IRQ_PRIORITY_6  = 6,
    /*! Interrupt request priority 7. */
    QUASAR_IRQ_PRIORITY_7  = 7,
    /*! Interrupt request priority 8. */
    QUASAR_IRQ_PRIORITY_8  = 8,
    /*! Interrupt request priority 9. */
    QUASAR_IRQ_PRIORITY_9  = 9,
    /*! Interrupt request priority 10. */
    QUASAR_IRQ_PRIORITY_10 = 10,
    /*! Interrupt request priority 11. */
    QUASAR_IRQ_PRIORITY_11 = 11,
    /*! Interrupt request priority 12. */
    QUASAR_IRQ_PRIORITY_12 = 12,
    /*! Interrupt request priority 13. */
    QUASAR_IRQ_PRIORITY_13 = 13,
    /*! Interrupt request priority 14. */
    QUASAR_IRQ_PRIORITY_14 = 14,
    /*! Interrupt request priority 15 (lowest priority). */
    QUASAR_IRQ_PRIORITY_15 = 15,
    /*! Disable interrupt */
    QUASAR_IRQ_PRIORITY_NONE = 0xFFFF
} quasar_irq_priority_t;

/* PUBLIC FUNCTION PROTOTYPES *************************************************/
/** @brief This function sets the function callback for the EXTI0 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_it_set_exti0_irq_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the EXTI1 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_it_set_exti1_irq_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the EXTI2 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_it_set_exti2_irq_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the EXTI3 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_it_set_exti3_irq_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the EXTI4 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_it_set_exti4_irq_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the EXTI5 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_it_set_exti5_irq_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the EXTI6 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_it_set_exti6_irq_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the EXTI7 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_it_set_exti7_irq_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the EXTI8 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_it_set_exti8_irq_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the EXTI9 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_it_set_exti9_irq_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the EXTI10 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_it_set_exti10_irq_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the EXTI11 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_it_set_exti11_irq_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the EXTI12 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_it_set_exti12_irq_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the EXTI13 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_it_set_exti13_irq_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the EXTI14 interrupt.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_it_set_exti14_irq_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the EXTI15 rising edge interrupt.
 *
 *  @note EXTI15 is used for both the falling and rising edges, which is why its configuration differs from the other
 *        EXTI lines.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_it_set_rising_edge_exti15_irq_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the EXTI15 falling edge interrupt.
 *
 *  @note EXTI15 is used for both the falling and rising edges, which is why its configuration differs from the other
 *        EXTI lines.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_it_set_falling_edge_exti15_irq_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the pendsv.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_it_set_pendsv_callback(void (*irq_callback)(void));

/** @brief This function sets the function callback for the USB IRQ.
 *
 *  @param[in] irq_callback  External interrupt callback function pointer.
 */
void quasar_it_set_usb_irq_callback(void (*irq_callback)(void));

/** @brief Set Timer 1 interrupt callback.
 *
 *  @param[in] irq_callback  Timer 1 callback.
 */
void quasar_it_set_timer1_callback(void (*irq_callback)(void));

/** @brief Set Timer 2 interrupt callback.
 *
 *  @param[in] irq_callback  Timer 2 callback.
 */
void quasar_it_set_timer2_callback(void (*irq_callback)(void));

/** @brief Set Timer 3 interrupt callback.
 *
 *  @param[in] irq_callback  Timer 3 callback.
 */
void quasar_it_set_timer3_callback(void (*irq_callback)(void));

/** @brief Set Timer 4 interrupt callback.
 *
 *  @param[in] irq_callback  Timer 4 callback.
 */
void quasar_it_set_timer4_callback(void (*irq_callback)(void));

/** @brief Set Timer 5 interrupt callback.
 *
 *  @param[in] irq_callback  Timer 5 callback.
 */
void quasar_it_set_timer5_callback(void (*irq_callback)(void));

/** @brief Set Timer 6 interrupt callback.
 *
 *  @param[in] irq_callback  Timer 6 callback.
 */
void quasar_it_set_timer6_callback(void (*irq_callback)(void));

/** @brief Set Timer 7 interrupt callback.
 *
 *  @param[in] irq_callback  Timer 7 callback.
 */
void quasar_it_set_timer7_callback(void (*irq_callback)(void));

/** @brief Set Timer 8 interrupt callback.
 *
 *  @param[in] irq_callback  Timer 8 callback.
 */
void quasar_it_set_timer8_callback(void (*irq_callback)(void));

/** @brief Set Timer 15 interrupt callback.
 *
 *  @param[in] irq_callback  Timer 15 callback.
 */
void quasar_it_set_timer15_callback(void (*irq_callback)(void));

/** @brief Set Timer 16 interrupt callback.
 *
 *  @param[in] irq_callback  Timer 16 callback.
 */
void quasar_it_set_timer16_callback(void (*irq_callback)(void));

/** @brief Set Timer 17 interrupt callback.
 *
 *  @param[in] irq_callback  Timer 17 callback.
 */
void quasar_it_set_timer17_callback(void (*irq_callback)(void));

/** @brief Enter a critical section by disabling interrupts.
 *
 *  This function is used to ensure atomic operations by preventing
 *  interrupts from occurring. Always pair with "quasar_it_exit_critical".
 */
void quasar_it_enter_critical(void);

/** @brief Exit a critical section by re-enabling interrupts.
 *
 *  This function re-enables the interrupts after a critical section.
 *  It should always be used in pair with "quasar_it_enter_critical".
 */
void quasar_it_exit_critical(void);

/** @brief Error handling used by STM32 HAL.
 */
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* QUASAR_IT_H_ */
