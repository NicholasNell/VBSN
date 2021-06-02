/*

 * my_rtc.c
 *
 *  Created on: 09 Mar 2020
 *      Author: nicholas

 */

#include <helper.h>
#include <my_gpio.h>
#include <my_gps.h>
#include <my_scheduler.h>
#include <stdbool.h>
#include <stdint.h>
#include <ti/devices/msp432p4xx/driverlib/gpio.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>
#include <ti/devices/msp432p4xx/driverlib/rom_map.h>
#include <ti/devices/msp432p4xx/driverlib/rtc_c.h>
#include <my_rtc.h>
#include <ti/devices/msp432p4xx/driverlib/wdt_a.h>

bool setTimeFlag;
bool macFlag = false;
bool schedFlag = false;
bool rtcInitFlag = false;
extern bool gpsWakeFlag;

extern Gpio_t Led_user_red;

//![Simple RTC Config]
//Time is Saturday, November 12th 1955 10:03:00 PM
RTC_C_Calendar currentTime = RTC_ZERO_TIME;

void rtc_init() {

	//[Simple RTC Example]
	/* Initializing RTC with current time as described in time in
	 * definitions section */

	MAP_RTC_C_initCalendar(&currentTime, RTC_C_FORMAT_BCD);

	/* Specify an interrupt to assert every minute */
//	MAP_RTC_C_setCalendarEvent(RTC_C_CALENDAREVENT_HOURCHANGE);
	MAP_RTC_C_setCalendarAlarm(0x05, RTC_C_ALARMCONDITION_OFF,
	RTC_C_ALARMCONDITION_OFF, RTC_C_ALARMCONDITION_OFF);

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

	//![Simple RTC Example]
	/* Enable interrupts and go to sleep. */
	MAP_Interrupt_enableInterrupt(INT_RTC_C);
	rtcInitFlag = true;
	rtc_stop_clock();
}

void rtc_start_clock(void) {
	MAP_RTC_C_startClock();
}

void rtc_stop_clock(void) {
	MAP_RTC_C_holdClock();
}

void rtc_set_calendar_time(void) {
//	rtc_stop_clock();
	MAP_RTC_C_initCalendar(&currentTime, RTC_C_FORMAT_BCD);
	int temp = convert_hex_to_dec_by_byte(RTC_C_getCalendarTime().minutes) * 60
			+ convert_hex_to_dec_by_byte(RTC_C_getCalendarTime().seconds);

	set_slot_count(temp);
//	rtc_start_clock();
}

bool resetFlag = false;
extern bool isRoot;
void RTC_C_IRQHandler(void) {

	uint32_t status;
	status = MAP_RTC_C_getEnabledInterruptStatus();
	MAP_RTC_C_clearInterruptFlag(status);
#if DEBUG
	gpio_toggle(&Led_user_red);
#endif

	if (rtcInitFlag) {
//		currentTime = MAP_RTC_C_getCalendarTime();
		schedFlag = true;
//		int temp = convert_hex_to_dec_by_byte(RTC_C_getCalendarTime().minutes)
//				* 60
//				+ convert_hex_to_dec_by_byte(RTC_C_getCalendarTime().seconds);
//		set_slot_count(temp);
//		increment_slot_count();
		int temp = convert_hex_to_dec_by_byte(RTC_C_getCalendarTime().minutes)
				* 60
				+ convert_hex_to_dec_by_byte(RTC_C_getCalendarTime().seconds);

		set_slot_count(temp);

	}

	if (get_slot_count() % (WINDOW_TIME_SEC * 3) == 0) {
		if (!isRoot) {
			resetFlag = true;
		}
	}

	if (status & RTC_C_TIME_EVENT_INTERRUPT) {
//		gpsWakeFlag = true;
//		set_slot_count(0);

	}

	if (status & RTC_C_CLOCK_ALARM_INTERRUPT) {	// Resync with GPS
		gpsWakeFlag = true;
//		resetFlag = true;
	}

}

