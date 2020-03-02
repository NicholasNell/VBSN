/*
 * my_timer.c
 *
 *  Created on: 27 Feb 2020
 *      Author: nicholas
 */

#include "my_timer.h"

/*
    Global variables
*/
bool TimerInteruptFlag = false;

void TimerAInteruptInit( void ) {
    /* Configuring Timer_A1 for Up Mode */
    MAP_Timer_A_configureUpMode(TIMER_A1_BASE, &upConfig);

    /* Enabling interrupts and starting the timer */
    MAP_Interrupt_enableSleepOnIsrExit();
    MAP_Interrupt_enableInterrupt(INT_TA1_0);

    /* Enabling MASTER interrupts */
    MAP_Interrupt_enableMaster();


}

void TA1_0_IRQHandler( void ) {
    TimerInteruptFlag = true;
    MAP_Timer_A_clearCaptureCompareInterrupt(TIMER_A1_BASE,
            TIMER_A_CAPTURECOMPARE_REGISTER_0);
}

void Delayms( uint32_t ms ) {
    uint32_t count = 0;
    MAP_Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_UP_MODE);

    while (count < ms) {
        if (TimerInteruptFlag) {
            count++;
        }
    }
    count = 0;
    MAP_Timer_A_stopTimer(TIMER_A1_BASE);
}

/*void StartTimer( void ) {
//    uint32_t elapsedTime = 0;

    MAP_Interrupt_disableMaster();  //Disable all interupts while things are being set up
}*/


