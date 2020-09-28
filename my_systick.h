/*
 * my_systick.h
 *
 *  Created on: 02 Mar 2020
 *      Author: nicholas
 */

#ifndef MY_SYSTICK_H_
#define MY_SYSTICK_H_

#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

void SystickInit();
void SysTick_Handler(void);
uint32_t SystickGetTime();

#endif /* MY_SYSTICK_H_ */
