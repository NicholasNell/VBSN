/*

 * my_rtc.c
 *
 *  Created on: 09 Mar 2020
 *      Author: nicholas

 */

#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#define TX_INTERVAL 1
uint8_t minutes = TX_INTERVAL;
bool setTimeFlag;
bool macFlag = false;

//![Simple RTC Config]
//Time is Saturday, November 12th 1955 10:03:00 PM
//RTC_C_Calendar currentTime = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2020 };

void RtcInit(const RTC_C_Calendar currentTime) {

	RTC_C->CTL0 = RTC_C_KEY | RTC_C_CTL0_TEVIE;
	RTC_C->CTL13 = RTC_C_CTL13_HOLD |
	RTC_C_CTL13_MODE |
	RTC_C_CTL13_BCD |
	RTC_C_CTL13_TEV_0;

	RTC_C->YEAR = currentTime.year;                   // Year
	RTC_C->DATE = (currentTime.month << RTC_C_DATE_MON_OFS) | // Month
			(currentTime.dayOfmonth | RTC_C_DATE_DAY_OFS);   // Day
	RTC_C->TIM1 = (currentTime.dayOfWeek << RTC_C_TIM1_DOW_OFS) | // Day of week
			(currentTime.hours << RTC_C_TIM1_HOUR_OFS);  // Hour
	RTC_C->TIM0 = (currentTime.minutes << RTC_C_TIM0_MIN_OFS) | // Minute
			(currentTime.seconds << RTC_C_TIM0_SEC_OFS);   // Seconds

	// Start RTC calendar mode
	RTC_C->CTL13 = RTC_C->CTL13 & ~(RTC_C_CTL13_HOLD);

	// Lock the RTC registers
	RTC_C->CTL0 = RTC_C->CTL0 & ~(RTC_C_CTL0_KEY_MASK);

	NVIC->ISER[0] = 1 << ((RTC_C_IRQn) & 31);
}

void RTC_C_IRQHandler(void) {
	uint32_t status;
	status = MAP_RTC_C_getEnabledInterruptStatus();
	MAP_RTC_C_clearInterruptFlag(status);
	static RTC_C_Calendar time;
	time = RTC_C_getCalendarTime();
	if (status & RTC_C_TIME_EVENT_INTERRUPT) {
		minutes--;

		if (time.minutes % 1 == 0) {
			macFlag = true;
		}
		if (minutes <= 0) {
			minutes = TX_INTERVAL;
		}
	}

	if (status & RTC_C_CLOCK_ALARM_INTERRUPT) {
		/* Interrupts every hour on 31min  */
		setTimeFlag = false;
	}
}

