/*

 * my_rtc.c
 *
 *  Created on: 09 Mar 2020
 *      Author: nicholas

 */

#include <my_scheduler.h>
#include <stdint.h>
#include <ti/devices/msp432p4xx/driverlib/gpio.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>
#include <ti/devices/msp432p4xx/driverlib/rom_map.h>
#include <ti/devices/msp432p4xx/driverlib/rtc_c.h>

bool setTimeFlag;
bool macFlag = false;
bool schedFlag = false;
bool rtcInitFlag = false;
extern bool gpsWakeFlag;

//![Simple RTC Config]
//Time is Saturday, November 12th 1955 10:03:00 PM
RTC_C_Calendar currentTime;

void RtcInit() {

	//![Simple RTC Example]
	/* Initializing RTC with current time as described in time in
	 * definitions section */
	MAP_RTC_C_initCalendar(&currentTime, RTC_C_FORMAT_BCD);

	/* Specify an interrupt to assert every minute */
	MAP_RTC_C_setCalendarEvent(RTC_C_CALENDAREVENT_MINUTECHANGE);

	RTC_C_setCalendarAlarm(RTC_C_ALARMCONDITION_OFF, 0x12,
	RTC_C_ALARMCONDITION_OFF, RTC_C_ALARMCONDITION_OFF);

	/* Enable interrupt for RTC Ready Status, which asserts when the RTC
	 * Calendar registers are ready to read.
	 * Also, enable interrupts for the Calendar alarm and Calendar event. */
	MAP_RTC_C_clearInterruptFlag(
			RTC_C_CLOCK_READ_READY_INTERRUPT | RTC_C_TIME_EVENT_INTERRUPT
					| RTC_C_CLOCK_ALARM_INTERRUPT);
	MAP_RTC_C_enableInterrupt(
	RTC_C_CLOCK_READ_READY_INTERRUPT | RTC_C_TIME_EVENT_INTERRUPT);

	/* Start RTC Clock */
	MAP_RTC_C_startClock();
	//![Simple RTC Example]

	/* Enable interrupts and go to sleep. */
	MAP_Interrupt_enableInterrupt(INT_RTC_C);
	rtcInitFlag = true;
}

void RTC_C_IRQHandler(void) {

	uint32_t status;
	status = MAP_RTC_C_getEnabledInterruptStatus();
	MAP_RTC_C_clearInterruptFlag(status);

	if (rtcInitFlag) {
		schedFlag = true;
		incrementSlotCount();

		if (getSlotCount() == MAX_SLOT_COUNT + 1) {
			setSlotCount(0);
		}

		GPIO_toggleOutputOnPin(GPIO_PORT_P2, GPIO_PIN2);

	}
//	if (time.minutes % 0x5 == 0) {
//		setSlotCount(0);
//	}
//	incrementSlotCount();

//	if (getSlotCount() == MAX_SLOT_COUNT + 1) {
//		setSlotCount(0);
//	}

	if (status & RTC_C_TIME_EVENT_INTERRUPT) {
//		gpsWakeFlag = true;
		setSlotCount(0);
	}

	if (status & RTC_C_CLOCK_ALARM_INTERRUPT) {
		gpsWakeFlag = true;
//			setSlotCount(0);
	}

}

