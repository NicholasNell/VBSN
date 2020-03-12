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
 * Timers list head pointer
 */
static TimerEvent_t *TimerListHead = NULL;

/*!
 * \brief Adds or replace the head timer of the list.
 *
 * \remark The list is automatically sorted. The list head always contains the
 *         next timer to expire.
 *
 * \param [IN]  obj Timer object to be become the new head
 * \param [IN]  remainingTime Remaining time of the previous head to be replaced
 */
static void TimerInsertNewHeadTimer( TimerEvent_t *obj );

/*!
 * \brief Adds a timer to the list.
 *
 * \remark The list is automatically sorted. The list head always contains the
 *         next timer to expire.
 *
 * \param [IN]  obj Timer object to be added to the list
 * \param [IN]  remainingTime Remaining time of the running head after which the object may be added
 */
static void TimerInsertTimer( TimerEvent_t *obj );

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
        TIMER_A_CLOCKSOURCE_DIVIDER_8,          // SMCLK/1 = 3MHz
        TIMER_PERIOD,                           // 5000 tick period
        TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
        TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE ,    // Enable CCR0 interrupt
        TIMER_A_DO_CLEAR                        // Clear value
};
bool TimerInteruptFlag = false;

void TimerInit( TimerEvent_t *obj, void ( *callback )( void *context ) )
{
    obj->Timestamp = 0;
    obj->ReloadValue = 0;
    obj->IsStarted = false;
    obj->IsNext2Expire = false;
    obj->Callback = callback;
    obj->Context = NULL;
    obj->Next = NULL;
}

void TimerSetContext( TimerEvent_t *obj, void* context )
{
    obj->Context = context;
}


void TimerStart( TimerEvent_t *obj )
{
    uint32_t elapsedTime = 0;

    CRITICAL_SECTION_BEGIN( );

    if( ( obj == NULL ) || ( TimerExists( obj ) == true ) )
    {
        CRITICAL_SECTION_END( );
        return;
    }

    obj->Timestamp = obj->ReloadValue;
    obj->IsStarted = true;
    obj->IsNext2Expire = false;

    if( TimerListHead == NULL )
    {
        RtcSetTimerContext( );
        // Inserts a timer at time now + obj->Timestamp
        TimerInsertNewHeadTimer( obj );
    }
    else
    {
        elapsedTime = RtcGetTimerElapsedTime( );
        obj->Timestamp += elapsedTime;

        if( obj->Timestamp < TimerListHead->Timestamp )
        {
            TimerInsertNewHeadTimer( obj );
        }
        else
        {
            TimerInsertTimer( obj );
        }
    }
    CRITICAL_SECTION_END( );
}

static void TimerInsertTimer( TimerEvent_t *obj )
{
    TimerEvent_t* cur = TimerListHead;
    TimerEvent_t* next = TimerListHead->Next;

    while( cur->Next != NULL )
    {
        if( obj->Timestamp > next->Timestamp )
        {
            cur = next;
            next = next->Next;
        }
        else
        {
            cur->Next = obj;
            obj->Next = next;
            return;
        }
    }
    cur->Next = obj;
    obj->Next = NULL;
}

static void TimerInsertNewHeadTimer( TimerEvent_t *obj )
{
    TimerEvent_t* cur = TimerListHead;

    if( cur != NULL )
    {
        cur->IsNext2Expire = false;
    }

    obj->Next = cur;
    TimerListHead = obj;
    TimerSetTimeout( TimerListHead );
}

bool TimerIsStarted( TimerEvent_t *obj )
{
    return obj->IsStarted;
}

void TimerIrqHandler( void )
{
    TimerEvent_t* cur;
    TimerEvent_t* next;

    uint32_t old =  RtcGetTimerContext( );
    uint32_t now =  RtcSetTimerContext( );
    uint32_t deltaContext = now - old; // intentional wrap around

    // Update timeStamp based upon new Time Reference
    // because delta context should never exceed 2^32
    if( TimerListHead != NULL )
    {
        for( cur = TimerListHead; cur->Next != NULL; cur = cur->Next )
        {
            next = cur->Next;
            if( next->Timestamp > deltaContext )
            {
                next->Timestamp -= deltaContext;
            }
            else
            {
                next->Timestamp = 0;
            }
        }
    }

    // Execute immediately the alarm callback
    if ( TimerListHead != NULL )
    {
        cur = TimerListHead;
        TimerListHead = TimerListHead->Next;
        cur->IsStarted = false;
        ExecuteCallBack( cur->Callback, cur->Context );
    }

    // Remove all the expired object from the list
    while( ( TimerListHead != NULL ) && ( TimerListHead->Timestamp < RtcGetTimerElapsedTime( ) ) )
    {
        cur = TimerListHead;
        TimerListHead = TimerListHead->Next;
        cur->IsStarted = false;
        ExecuteCallBack( cur->Callback, cur->Context );
    }

    // Start the next TimerListHead if it exists AND NOT running
    if( ( TimerListHead != NULL ) && ( TimerListHead->IsNext2Expire == false ) )
    {
        TimerSetTimeout( TimerListHead );
    }
}

void TimerStop( TimerEvent_t *obj )
{
    CRITICAL_SECTION_BEGIN( );

    TimerEvent_t* prev = TimerListHead;
    TimerEvent_t* cur = TimerListHead;

    // List is empty or the obj to stop does not exist
    if( ( TimerListHead == NULL ) || ( obj == NULL ) )
    {
        CRITICAL_SECTION_END( );
        return;
    }

    obj->IsStarted = false;

    if( TimerListHead == obj ) // Stop the Head
    {
        if( TimerListHead->IsNext2Expire == true ) // The head is already running
        {
            TimerListHead->IsNext2Expire = false;
            if( TimerListHead->Next != NULL )
            {
                TimerListHead = TimerListHead->Next;
                TimerSetTimeout( TimerListHead );
            }
            else
            {
                RtcStopAlarm( );
                TimerListHead = NULL;
            }
        }
        else // Stop the head before it is started
        {
            if( TimerListHead->Next != NULL )
            {
                TimerListHead = TimerListHead->Next;
            }
            else
            {
                TimerListHead = NULL;
            }
        }
    }
    else // Stop an object within the list
    {
        while( cur != NULL )
        {
            if( cur == obj )
            {
                if( cur->Next != NULL )
                {
                    cur = cur->Next;
                    prev->Next = cur;
                }
                else
                {
                    cur = NULL;
                    prev->Next = cur;
                }
                break;
            }
            else
            {
                prev = cur;
                cur = cur->Next;
            }
        }
    }
    CRITICAL_SECTION_END( );
}

static bool TimerExists( TimerEvent_t *obj )
{
    TimerEvent_t* cur = TimerListHead;

    while( cur != NULL )
    {
        if( cur == obj )
        {
            return true;
        }
        cur = cur->Next;
    }
    return false;
}

void TimerReset( TimerEvent_t *obj )
{
    TimerStop( obj );
    TimerStart( obj );
}

void TimerSetValue( TimerEvent_t *obj, uint32_t value )
{
    uint32_t minValue = 0;
    uint32_t ticks = RtcMs2Tick( value );

    TimerStop( obj );

    minValue = RtcGetMinimumTimeout( );

    if( ticks < minValue )
    {
        ticks = minValue;
    }

    obj->Timestamp = ticks;
    obj->ReloadValue = ticks;
}

TimerTime_t TimerGetCurrentTime( void )
{
    uint32_t now = RtcGetTimerValue( );
    return  RtcTick2Ms( now );
}

TimerTime_t TimerGetElapsedTime( TimerTime_t past )
{
    if ( past == 0 )
    {
        return 0;
    }
    uint32_t nowInTicks = RtcGetTimerValue( );
    uint32_t pastInTicks = RtcMs2Tick( past );

    // Intentional wrap around. Works Ok if tick duration below 1ms
    return RtcTick2Ms( nowInTicks - pastInTicks );
}

static void TimerSetTimeout( TimerEvent_t *obj )
{
    int32_t minTicks= RtcGetMinimumTimeout( );
    obj->IsNext2Expire = true;

    // In case deadline too soon
    if( obj->Timestamp  < ( RtcGetTimerElapsedTime( ) + minTicks ) )
    {
        obj->Timestamp = RtcGetTimerElapsedTime( ) + minTicks;
    }
    RtcSetAlarm( obj->Timestamp );
}

TimerTime_t TimerTempCompensation( TimerTime_t period, float temperature )
{
    return RtcTempCompensation( period, temperature );
}

void TimerProcess( void )
{
    RtcProcess( );
}

void TimerAInteruptInit( void ) {
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


