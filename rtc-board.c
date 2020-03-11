/*
 * rtc-board.c
 *
 *  Created on: 11 Mar 2020
 *      Author: nicholas
 */

#include <time.h>
#include "board-config.h"
#include "board.h"
#include "my_timer.h"
//#include "systime.h"
#include "my_gpio.h"

#include "rtc-board.h"

#define RTC_DEBUG_ENABLE                            1
#define RTC_DEBUG_DISABLE                           0

#define RTC_DEBUG_GPIO_STATE                        RTC_DEBUG_DISABLE
#define RTC_DEBUG_PRINTF_STATE                      RTC_DEBUG_DISABLE

#define MIN_ALARM_DELAY                             3 // in ticks

/*!
 * \brief Indicates if the RTC is already Initialized or not
 */
static bool RtcInitialized = false;
static volatile bool RtcTimeoutPendingInterrupt = false;
static volatile bool RtcTimeoutPendingPolling = false;

static volatile RTC_C_Calendar newTime;

//![Simple RTC Config]
// Time is Saturday, November 12th 1955 10:03:00 PM
const RTC_C_Calendar currentTime =
{
        0x00,
        0x03,
        0x22,
        0x06,
        0x12,
        0x11,
        0x1955
};

typedef enum AlarmStates_e
{
    ALARM_STOPPED = 0,
    ALARM_RUNNING = !ALARM_STOPPED
} AlarmStates_t;

/*!
 * RTC timer context
 */
typedef struct
{
    uint32_t Time;  // Reference time
    uint32_t Delay; // Reference Timeout duration
    uint32_t AlarmState;
}RtcTimerContext_t;

/*!
 * Keep the value of the RTC timer when the RTC alarm is set
 * Set with the \ref RtcSetTimerContext function
 * Value is kept as a Reference to calculate alarm
 */
static RtcTimerContext_t RtcTimerContext;

#if( RTC_DEBUG_GPIO_STATE == RTC_DEBUG_ENABLE )
Gpio_t DbgRtcPin0;
Gpio_t DbgRtcPin1;
#endif

/*!
 * Used to store the Seconds and SubSeconds.
 *
 * WARNING: Temporary fix fix. Should use MCU NVM internal
 *          registers
 */
uint32_t RtcBkupRegisters[] = { 0, 0 };

/*!
 * \brief Callback for the hw_timer when alarm expired
 */
static void RtcAlarmIrq( void );

/*!
 * \brief Callback for the hw_timer when counter overflows
 */
static void RtcOverflowIrq( void );

void RtcInit( void )
{
    if( RtcInitialized == false ) {

        MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_PJ,
                GPIO_PIN0 | GPIO_PIN1, GPIO_PRIMARY_MODULE_FUNCTION);

    /*     Setting the external clock frequency. This API is optional, but will
         * come in handy if the user ever wants to use the getMCLK/getACLK/etc
         * functions*/

/*        // RTC timer
        HwTimerInit( );
        HwTimerAlarmSetCallback( RtcAlarmIrq );
        HwTimerOverflowSetCallback( RtcOverflowIrq );*/

        CS_setExternalClockSourceFrequency(32000,48000000);

    //     Starting LFXT in non-bypass mode without a timeout.
        CS_startLFXT(CS_LFXT_DRIVE3);

        MAP_RTC_C_initCalendar(&currentTime, RTC_C_FORMAT_BCD);

    //     Specify an interrupt to assert every minute
        MAP_RTC_C_setCalendarEvent(RTC_C_CALENDAREVENT_MINUTECHANGE);

    /*     Enable interrupt for RTC Ready Status, which asserts when the RTC
         * Calendar registers are ready to read.
         * Also, enable interrupts for the Calendar alarm and Calendar event.*/

        MAP_RTC_C_clearInterruptFlag(
                RTC_C_CLOCK_READ_READY_INTERRUPT | RTC_C_TIME_EVENT_INTERRUPT
                        | RTC_C_CLOCK_ALARM_INTERRUPT);
        MAP_RTC_C_enableInterrupt(
                RTC_C_CLOCK_READ_READY_INTERRUPT | RTC_C_TIME_EVENT_INTERRUPT
                        | RTC_C_CLOCK_ALARM_INTERRUPT);

    //     Start RTC Clock
        MAP_RTC_C_startClock();
        //![Simple RTC Example]

    //     Enable interrupts and go to sleep.
        MAP_Interrupt_enableInterrupt(INT_RTC_C);

        RtcTimerContext.AlarmState = ALARM_STOPPED;
        RtcSetTimerContext( );
        RtcInitialized = true;
    }

}

uint32_t RtcSetTimerContext( void )
{
//    TODO: FIX THIS! RTCTIM0 is not correct. Find a way to get the time in uint32_t
    RtcTimerContext.Time = ( uint32_t ) (RTCTIM0);
    return ( uint32_t )RtcTimerContext.Time;
}

uint32_t RtcGetTimerContext( void )
{
    return RtcTimerContext.Time;
}

uint32_t RtcGetMinimumTimeout( void )
{
    return( MIN_ALARM_DELAY );
}

uint32_t RtcMs2Tick( TimerTime_t milliseconds )
{
    return ( uint32_t )( milliseconds );
}

TimerTime_t RtcTick2Ms( uint32_t tick )
{
    uint32_t seconds = tick >> 10;

    tick = tick & 0x3FF;
    return ( ( seconds * 1000 ) + ( ( tick * 1000 ) >> 10 ) );
}

void RtcDelayMs( TimerTime_t milliseconds )
{
    uint32_t delayTicks = 0;
    uint32_t refTicks = RtcGetTimerValue( );

    delayTicks = RtcMs2Tick( milliseconds );

    // Wait delay ms
    while( ( ( RtcGetTimerValue( ) - refTicks ) ) < delayTicks )
    {
        __NOP( );
    }
}

void RtcSetAlarm( uint32_t timeout )
{
    RtcStartAlarm( timeout );
}

void RtcStopAlarm( void )
{
    RtcTimerContext.AlarmState = ALARM_STOPPED;
}

void RtcStartAlarm( uint32_t timeout )
{
    CRITICAL_SECTION_BEGIN( );

    RtcStopAlarm( );

    RtcTimerContext.Delay = timeout;

    RtcTimeoutPendingInterrupt = true;
    RtcTimeoutPendingPolling = false;

    RtcTimerContext.AlarmState = ALARM_RUNNING;
    /*if( HwTimerLoadAbsoluteTicks( RtcTimerContext.Time + RtcTimerContext.Delay ) == false )
    {
        // If timer already passed
        if( RtcTimeoutPendingInterrupt == true )
        {
            // And interrupt not handled, mark as polling
            RtcTimeoutPendingPolling = true;
            RtcTimeoutPendingInterrupt = false;
        }
    }*/
    CRITICAL_SECTION_END( );
}

uint32_t RtcGetTimerValue( void )
{
//    return ( uint32_t )HwTimerGetTime( );
}

uint32_t RtcGetTimerElapsedTime( void )
{
//    return ( uint32_t)( HwTimerGetTime( ) - RtcTimerContext.Time );
}

uint32_t RtcGetCalendarTime( uint16_t *milliseconds )
{
/*    uint32_t ticks = 0;

    uint32_t calendarValue = HwTimerGetTime( );

    uint32_t seconds = ( uint32_t )calendarValue >> 10;

    ticks =  ( uint32_t )calendarValue & 0x3FF;

    *milliseconds = RtcTick2Ms( ticks );

    return seconds;*/
}

void RtcBkupWrite( uint32_t data0, uint32_t data1 )
{
    CRITICAL_SECTION_BEGIN( );
    RtcBkupRegisters[0] = data0;
    RtcBkupRegisters[1] = data1;
    CRITICAL_SECTION_END( );
}

void RtcBkupRead( uint32_t* data0, uint32_t* data1 )
{
    CRITICAL_SECTION_BEGIN( );
    *data0 = RtcBkupRegisters[0];
    *data1 = RtcBkupRegisters[1];
    CRITICAL_SECTION_END( );
}

void RtcProcess( void )
{
    CRITICAL_SECTION_BEGIN( );

    if( (  RtcTimerContext.AlarmState == ALARM_RUNNING ) && ( RtcTimeoutPendingPolling == true ) )
    {
        if( RtcGetTimerElapsedTime( ) >= RtcTimerContext.Delay )
        {
            RtcTimerContext.AlarmState = ALARM_STOPPED;

            // Because of one shot the task will be removed after the callback
            RtcTimeoutPendingPolling = false;
            // NOTE: The handler should take less then 1 ms otherwise the clock shifts
            TimerIrqHandler( );
        }
    }
    CRITICAL_SECTION_END( );
}

TimerTime_t RtcTempCompensation( TimerTime_t period, float temperature )
{
    return period;
}

static void RtcAlarmIrq( void )
{
    RtcTimerContext.AlarmState = ALARM_STOPPED;
    // Because of one shot the task will be removed after the callback
    RtcTimeoutPendingInterrupt = false;

    // NOTE: The handler should take less then 1 ms otherwise the clock shifts
    TimerIrqHandler( );
}

static void RtcOverflowIrq( void )
{
    //RtcTimerContext.Time += ( uint64_t )( 1 << 32 );
}
