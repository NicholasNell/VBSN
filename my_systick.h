/*
 * my_systick.h
 *
 *  Created on: 2 Mar, 2020
 *      Author: Nicholas
 */

#ifndef MY_SYSTICK_H_
#define MY_SYSTICK_H_

/************** SYSTICK TIMER **************/
void setupSysTick_ms( void );
void setupSysTick_second( void );

void runsystickFunction_ms( uint16_t ms_delay );
void runSystickFunction_second( uint16_t second_delay );
bool SystimerReadyCheck( void );
/*
 * Sets the delay left to 0, and disables the interrupt for the module
 */
void stopSystick( void );
uint16_t delayLeft( void );

#endif
