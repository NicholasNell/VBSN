/*
 * my_systick.c
 *
 *  Created on: 02 Mar 2020
 *      Author: nicholas
 */

#include <punctual.h>
#include "my_systick.h"

static uint32_t ticks;

/*!
 *
 * @param period value in ms
 * @param flag	flag to be set
 */
void SystickInit( ) {

	MAP_SysTick_enableModule();
	uint32_t value = 1 * 15 * 100;
	SysTick_setPeriod(value);
	ticks = 0;
	MAP_SysTick_enableInterrupt();
}

void SysTick_Handler( void ) {
	ticks++;
//	PunctualISR();
}

uint32_t SystickGetTime( ) {
	return ticks;
}
