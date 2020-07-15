/*
 * my_systick.c
 *
 *  Created on: 02 Mar 2020
 *      Author: nicholas
 */

#include "my_systick.h"

void SystickInit( void ) {
    /* Configuring SysTick to trigger at 3000 (MCLK is 3MHz so this will
     * make it toggle every 1ms) */
    MAP_SysTick_enableModule();
    MAP_SysTick_setPeriod(3000);
//    MAP_Interrupt_enableSleepOnIsrExit();
    MAP_SysTick_enableInterrupt();
}

void SysTick_Handler( void ) {

}
