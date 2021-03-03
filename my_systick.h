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

void run_systick_function_ms( uint16_t ms_delay );
void run_systick_function_second( uint16_t second_delay );
bool systimer_ready_check( void );
/*
 * Sets the delay left to 0, and disables the interrupt for the module
 */
void stop_systick( void );
uint16_t delay_left( void );

#endif
