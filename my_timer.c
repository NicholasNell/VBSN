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

/*!
 * Timers list head pointer
 */
static TimerEvent_t *TimerListHead = NULL;

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

void StartTimer( TimerEvent_t *obj ) {
    uint32_t elapsedTime = 0;

    MAP_Interrupt_disableMaster();  //Disable all interupts while things are being set up

    if ( ( obj == NULL ) || (TimerExists( obj ) == true ) ) {
        MAP_Interrupt_enableMaster();
        return;
    }// If the timer object does not exist or the timer is already on the timer list return

    obj->Timestamp = obj->ReloadValue;
    obj->IsStarted = true;
    obj->IsNext2Expire = false;


}

void TimerInit( TimerEvent_t *obj, void ( *callback )( void *context ) ) {
    obj->Timestamp = 0;
    obj->ReloadValue = 0;
    obj->IsStarted = false;
    obj->IsNext2Expire = false;
    obj->Callback = callback;
    obj->Context = NULL;
    obj->Next = NULL;
}

void TimerSetContext( TimerEvent_t *obj, void* context ) {
    obj->Context = context;
}
