/*
 * my_systick.c
 *
 *  Created on: 02 Mar 2020
 *      Author: nicholas
 */

#include "my_systick.h"
bool *timerAtempFlag;

/*!
 *
 * @param period value in ms
 * @param flag	flag to be set
 */
void SystickInit( uint32_t period, bool *flag ) {

	timerAtempFlag = flag;
    MAP_SysTick_enableModule();
	uint32_t value = period * 15 * 100;
	MAP_SysTick_setPeriod(value);
    MAP_SysTick_enableInterrupt();
}

void SysTick_Handler( void ) {
	*timerAtempFlag = true;
	SysTick_disableModule();
}
