/*
 * my_timer.h
 *
 *  Created on: 27 Feb 2020
 *      Author: nicholas
 */

#ifndef MY_TIMER_H_
#define MY_TIMER_H_

/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/*!
 * \brief Timer object description
 */
typedef struct TimerEvent_s
{
    uint32_t Timestamp;                  //! Current timer value
    uint32_t ReloadValue;                //! Timer delay value
    bool IsStarted;                      //! Is the timer currently running
    bool IsNext2Expire;                  //! Is the next timer to expire
    void ( *Callback )( void* context ); //! Timer IRQ callback function
    void *Context;                       //! User defined data object pointer to pass back
    struct TimerEvent_s *Next;           //! Pointer to the next Timer object.
}TimerEvent_t;


/* Application Defines  */

/*
    Period (s) = (TIMER_PERIOD * DIVIDER) / (SMCLK = 3MHz)
*/
#define TIMER_PERIOD    375

/* Timer_A UpMode Configuration Parameter */
const Timer_A_UpModeConfig upConfig =
{
        TIMER_A_CLOCKSOURCE_SMCLK,              // SMCLK Clock Source
        TIMER_A_CLOCKSOURCE_DIVIDER_8,          // SMCLK/1 = 3MHz
        TIMER_PERIOD,                           // 375 tick period
        TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
        TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE ,    // Enable CCR0 interrupt
        TIMER_A_DO_CLEAR                        // Clear value
};

/*
    Initialise timer_a it count up mode. Trigger interupt when TAR equals CCR0.
    Timer will be configured for ms interupts.
*/
void TimerAInteruptInit( void );

/*
    initilise a new timer object
*/
void TimerInit( TimerEvent_t *obj, void ( *callback )( void *context ) );

/*
    Timer A ISR Handler
*/
void TA1_0_IRQHandler( void );

/*
 * Delay function 1ms resolution
 */
void Delayms( uint32_t ms );

/*
    Start a new timer A1
*/
void StartTimer( TimerEvent_t *obj );

/*!
 * \brief Sets a user defined object pointer
 *
 * \param [IN] context User defined data object pointer to pass back
 *                     on IRQ handler callback
 */
void TimerSetContext( TimerEvent_t *obj, void* context );




#endif /* MY_TIMER_H_ */
