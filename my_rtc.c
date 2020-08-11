/*

 * my_rtc.c
 *
 *  Created on: 09 Mar 2020
 *      Author: nicholas

 */

#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#define TX_INTERVAL 1
uint8_t minutes = TX_INTERVAL;
bool TxFlag = false;

//![Simple RTC Config]
//Time is Saturday, November 12th 1955 10:03:00 PM
const RTC_C_Calendar currentTime =
{
	0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x0000
};
//![Simple RTC Config]

void RtcInit( void ) {
	 /* Configuring pins for peripheral/crystal usage and LED for output */
	    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_PJ,
	            GPIO_PIN0 | GPIO_PIN1, GPIO_PRIMARY_MODULE_FUNCTION);

	/* Starting LFXT in non-bypass mode without a timeout. */


	    //![Simple RTC Example]
	    /* Initializing RTC with current time as described in time in
	     * definitions section */
	    MAP_RTC_C_initCalendar(&currentTime, RTC_C_FORMAT_BCD);

	    /* Setup Calendar Alarm for 10:04pm (for the flux capacitor) */
//	    MAP_RTC_C_setCalendarAlarm(0x04, 0x22, RTC_C_ALARMCONDITION_OFF,
//	            RTC_C_ALARMCONDITION_OFF);

	    /* Specify an interrupt to assert every minute */
	    MAP_RTC_C_setCalendarEvent(RTC_C_CALENDAREVENT_MINUTECHANGE);

	    /* Enable interrupt for RTC Ready Status, which asserts when the RTC
	     * Calendar registers are ready to read.
	     * Also, enable interrupts for the Calendar alarm and Calendar event. */
	    MAP_RTC_C_clearInterruptFlag(
	            RTC_C_CLOCK_READ_READY_INTERRUPT | RTC_C_TIME_EVENT_INTERRUPT
	                    | RTC_C_CLOCK_ALARM_INTERRUPT);
	    MAP_RTC_C_enableInterrupt(
	            RTC_C_CLOCK_READ_READY_INTERRUPT | RTC_C_TIME_EVENT_INTERRUPT
	                    | RTC_C_CLOCK_ALARM_INTERRUPT);

	    /* Start RTC Clock */
	    MAP_RTC_C_startClock();
	    //![Simple RTC Example]

	    /* Enable interrupts and go to sleep. */
	    MAP_Interrupt_enableInterrupt(INT_RTC_C);
}

void RTC_C_IRQHandler( void ) {
	uint32_t status;

	status = MAP_RTC_C_getEnabledInterruptStatus();
	MAP_RTC_C_clearInterruptFlag(status);

	if (status & RTC_C_TIME_EVENT_INTERRUPT) {
		minutes--;
		if (minutes <= 0) {
			minutes = TX_INTERVAL;
			TxFlag = true;
		}
	}
}

