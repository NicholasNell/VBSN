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
void TimerAInit( void );

/*
    Timer A ISR Handler
*/
void TA1_0_IRQHandler( void );

/*
 * Delay function 1ms resolution
 */
void Delayms( uint32_t ms );



#endif /* MY_TIMER_H_ */
