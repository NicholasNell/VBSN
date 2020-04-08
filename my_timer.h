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

/* Application Defines  */

/*!
    \brief Initialise timer_a to count up mode. Trigger interupt when TAR equals CCR0.
    Timer will be configured for ms interupts.
*/
void TimerAInteruptInit( void );

/*!
    Timer A ISR Handler
*/
void TA1_0_IRQHandler( void );

/*!
 * Delay function 1ms resolution
 */
void Delayms( uint32_t ms );

/*!
 * \brief Sets up timer for 10 us accuracy
 */
void TimerATimerInit( void );

/*!
 * \brief Starts Timing using Timer_A0
 */
void startTiming( void );

/*!
 * \brief Stop the timer and returns time in us
 */
uint32_t stopTiming ( void );

/*!
 * \brief Gets the current elapsed time in us
 */
uint32_t getTiming ( void );

/*!
 * \brief Delay using timer without interupts
 */
void DelayLoop(uint16_t ms);

/*!
 * \brief Start a timer which will interupt after timeout ms
 * @param timeout, value in ms
 */
void startLoRaTimer(uint32_t timeout);

/*!
 *  Stopsthe LoRa module timer
 */

void stopLoRaTimer();

#endif /* MY_TIMER_H_ */
