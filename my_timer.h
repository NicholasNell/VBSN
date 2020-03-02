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
//#include <stddef.h>
//#include <stdbool.h>
//#include <stdint.h>

/* Application Defines  */

/*
    Initialise timer_a to count up mode. Trigger interupt when TAR equals CCR0.
    Timer will be configured for ms interupts.
*/
void TimerAInteruptInit( void );

/*
    Timer A ISR Handler
*/
void TA1_0_IRQHandler( void );

/*
 * Delay function 1ms resolution
 */
void Delayms( uint32_t ms );

#endif /* MY_TIMER_H_ */
