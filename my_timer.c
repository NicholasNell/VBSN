/*
 * my_timer.c
 *
 *  Created on: 27 Feb 2020
 *      Author: nicholas
 */

#include "my_timer.h"
#include "utilities.h"
#include "board.h"
#include "rtc-board.h"

/*!
 * Safely execute call back
 */
#define ExecuteCallBack( _callback_, context ) \
    do                                         \
    {                                          \
        if( _callback_ == NULL )               \
        {                                      \
            while( 1 );                        \
        }                                      \
        else                                   \
        {                                      \
            _callback_( context );             \
        }                                      \
    }while( 0 );


/*!
 * \brief Sets a timeout with the duration "timestamp"
 *
 * \param [IN] timestamp Delay duration
 */
static void TimerSetTimeout( TimerEvent_t *obj );

/*!
 * \brief Check if the Object to be added is not already in the list
 *
 * \param [IN] timestamp Delay duration
 * \retval true (the object is already in the list) or false
 */
static bool TimerExists( TimerEvent_t *obj );
/*
    Global variables
*/
/*
    Period (s) = (TIMER_PERIOD * DIVIDER) / (SMCLK = 3MHz)
*/
/* Application Defines  */
#define TIMER_PERIOD    375

/* Timer_A UpMode Configuration Parameter */
const Timer_A_UpModeConfig upConfig =
{
        TIMER_A_CLOCKSOURCE_SMCLK,              // SMCLK Clock Source
        TIMER_A_CLOCKSOURCE_DIVIDER_4,          // SMCLK/1 = 3MHz
        TIMER_PERIOD,                           // 375 tick period
        TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
        TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE ,    // Enable CCR0 interrupt
        TIMER_A_DO_CLEAR                        // Clear value
};
bool TimerInteruptFlag = false;

void TimerInit( TimerEvent_t *obj )
{
    obj->Timestamp = 0;
    obj->ReloadValue = 0;
}

void TimerStart( TimerEvent_t *obj ) {
    CRITICAL_SECTION_BEGIN( );

    CRITICAL_SECTION_END( );
}

bool TimerIsStarted( TimerEvent_t *obj ) {
    return obj->IsStarted;
}

void TimerIrqHandler( void ) {

}

void TimerStop( TimerEvent_t *obj ) {
    CRITICAL_SECTION_BEGIN( );

    CRITICAL_SECTION_END( );
}

static bool TimerExists( TimerEvent_t *obj ) {
    if ( obj == NULL ) {
        return false;
    }
    return true;
}

void TimerReset( TimerEvent_t *obj )
{
    TimerStop( obj );
    TimerStart( obj );
}

void TimerSetValue( TimerEvent_t *obj, uint32_t value ) {

}

TimerTime_t TimerGetCurrentTime( void ) {

}

TimerTime_t TimerGetElapsedTime( TimerTime_t past ) {

}


static void TimerSetTimeout( TimerEvent_t *obj ) {

}

TimerTime_t TimerTempCompensation( TimerTime_t period, float temperature ) {
}

void TimerProcess( void )
{

}

void TimerAInteruptInit( Timer_A_UpModeConfig *upConfig ) {
    /* Configuring Timer_A1 for Up Mode */
    MAP_Timer_A_configureUpMode(TIMER_A1_BASE, &upConfig);

    /* Enabling interrupts and starting the timer */
//    MAP_Interrupt_enableSleepOnIsrExit();
    MAP_Interrupt_enableInterrupt(INT_TA1_0);

    MAP_Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_UP_MODE);
    MAP_SysCtl_enableSRAMBankRetention(SYSCTL_SRAM_BANK1);
    /* Enabling MASTER interrupts */
    MAP_Interrupt_enableMaster();
}

void TA1_0_IRQHandler( void ) {
    TimerInteruptFlag = true;
    Timer_A_clearCaptureCompareInterrupt(TIMER_A1_BASE,
            TIMER_A_CAPTURECOMPARE_REGISTER_0);
}

void Delayms( uint32_t ms ) {
    uint32_t count = 0;
    MAP_Timer_A_clearTimer(TIMER_A1_BASE);
    MAP_Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_UP_MODE);

    while (count < ms) {
        if (TimerInteruptFlag) {
            count++;
            TimerInteruptFlag = false;
        }
    }
    count = 0;
    MAP_Timer_A_stopTimer(TIMER_A1_BASE);
}


