/*

 * my_rtc.c
 *
 *  Created on: 09 Mar 2020
 *      Author: nicholas



 Statics
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
 static volatile RTC_C_Calendar newTime;

 //![Simple RTC Config]
  Time is Saturday, November 12th 1955 10:03:00 PM
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
 //![Simple RTC Config]


void RtcInit( void ) {
    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_PJ,
            GPIO_PIN0 | GPIO_PIN1, GPIO_PRIMARY_MODULE_FUNCTION);

     Setting the external clock frequency. This API is optional, but will
     * come in handy if the user ever wants to use the getMCLK/getACLK/etc
     * functions

    CS_setExternalClockSourceFrequency(32000,48000000);

     Starting LFXT in non-bypass mode without a timeout.
    CS_startLFXT(CS_LFXT_DRIVE3);

    MAP_RTC_C_initCalendar(&currentTime, RTC_C_FORMAT_BCD);

     Specify an interrupt to assert every minute
    MAP_RTC_C_setCalendarEvent(RTC_C_CALENDAREVENT_MINUTECHANGE);

     Enable interrupt for RTC Ready Status, which asserts when the RTC
     * Calendar registers are ready to read.
     * Also, enable interrupts for the Calendar alarm and Calendar event.
    MAP_RTC_C_clearInterruptFlag(
            RTC_C_CLOCK_READ_READY_INTERRUPT | RTC_C_TIME_EVENT_INTERRUPT
                    | RTC_C_CLOCK_ALARM_INTERRUPT);
    MAP_RTC_C_enableInterrupt(
            RTC_C_CLOCK_READ_READY_INTERRUPT | RTC_C_TIME_EVENT_INTERRUPT
                    | RTC_C_CLOCK_ALARM_INTERRUPT);

     Start RTC Clock
    MAP_RTC_C_startClock();
    //![Simple RTC Example]

     Enable interrupts and go to sleep.
    MAP_Interrupt_enableInterrupt(INT_RTC_C);

}

 RTC ISR
void RTC_C_IRQHandler(void)
{
    uint32_t status;

    status = MAP_RTC_C_getEnabledInterruptStatus();
    MAP_RTC_C_clearInterruptFlag(status);

    if (status & RTC_C_CLOCK_READ_READY_INTERRUPT)
    {

    }

    if (status & RTC_C_TIME_EVENT_INTERRUPT)
    {
         Interrupts every minute - Set breakpoint here
        newTime = MAP_RTC_C_getCalendarTime();

    }

    if (status & RTC_C_CLOCK_ALARM_INTERRUPT)
    {

    }

}
*/