/*
 * mysystick.c
 *
 *  Created on: 2020
 *      Author: Nicholas
 */

#include <stdint.h>
#include <stdbool.h>
#include <ti/devices/msp432p4xx/driverlib/systick.h>
#include <ti/devices/msp432p4xx/driverlib/rom_map.h>
#include "my_systick.h"
/************** SYSTICK TIMER **************/
static volatile uint16_t delay = 0;
static volatile bool timerReady = false;
/************** SYSTICK TIMER **************/

void SysTick_Handler(void) //INTERRUPT ROUTINE
		{
	// MAP_GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN0);
	if (delay == 0) {
		timerReady = true;
		MAP_SysTick_disableInterrupt();
	}
	delay--;
}

void setupSysTick_ms(void) {
	//Sets the sysTick to 1 millisecond
	MAP_SysTick_disableModule();
	MAP_SysTick_setPeriod(1500);
	MAP_SysTick_enableModule();
}
void setupSysTick_second(void) {
	//Sets the sysTick to 1 second

	MAP_SysTick_disableModule();
	MAP_SysTick_setPeriod(1500000);
	MAP_SysTick_enableModule();
}

void run_systick_function_ms(uint16_t ms_delay) {
	setupSysTick_ms();
	delay = ms_delay;
	timerReady = false;
	MAP_SysTick_enableInterrupt();
}
void run_systick_function_second(uint16_t second_delay) {
	setupSysTick_second();
	delay = second_delay;
	timerReady = false;
	MAP_SysTick_enableInterrupt();
}

void stop_systick(void) {
	delay = 0;
	MAP_SysTick_disableInterrupt();
}

bool systimer_ready_check(void) {
	return timerReady;
}

uint16_t delay_left(void) {
	return delay;
}
