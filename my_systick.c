/*
 * my_systick.c
 *
 *  Created on: 02 Mar 2020
 *      Author: nicholas
 */

#include "my_systick.h"
bool *tempFlag;

/*!
 *
 * @param period value in ms
 * @param flag	flag to be set
 */
void SystickInit( uint32_t period, bool *flag ) {
	/* Configuring SysTick to trigger at 3000 (MCLK is 1.5MHz so this will
	 * make it toggle every 2ms) */
	tempFlag = flag;
    MAP_SysTick_enableModule();
	uint32_t value = period * 15 * 100;
	MAP_SysTick_setPeriod(value);
//    MAP_Interrupt_enableSleepOnIsrExit();
    MAP_SysTick_enableInterrupt();
}

void SysTick_Handler( void ) {
	*tempFlag = true;
	SysTick_disableModule();
}
