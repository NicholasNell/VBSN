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
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2020 };

void RtcInit(void) {
	/* Configuring pins for peripheral/crystal usage and LED for output */
	MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_PJ,
	GPIO_PIN0 | GPIO_PIN1, GPIO_PRIMARY_MODULE_FUNCTION);

	/* Starting LFXT in non-bypass mode without a timeout. */
	MAP_CS_startLFXT(false);
	MAP_RTC_C_holdClock();
	//ROM_RTC_C_initCalendar(&newtime, RTC_C_FORMAT_BCD);
	RTC_C->CTL0 = (RTC_C->CTL0 & ~RTC_C_CTL0_KEY_MASK) | RTC_C_KEY;

	BITBAND_PERI(RTC_C->CTL13, RTC_C_CTL13_HOLD_OFS) = 1;

	BITBAND_PERI(RTC_C->CTL13, RTC_C_CTL13_BCD_OFS) = 1;

	RTC_C->TIM0 = (currentTime.minutes << RTC_C_TIM0_MIN_OFS)
			| currentTime.seconds;
	__no_operation();
	RTC_C->TIM1 = (currentTime.dayOfWeek << RTC_C_TIM1_DOW_OFS)
			| currentTime.hours;
	__no_operation();
	RTC_C->DATE = (currentTime.month << RTC_C_DATE_MON_OFS)
			| currentTime.dayOfmonth;
	__no_operation();
	RTC_C->YEAR = currentTime.year;

	BITBAND_PERI(RTC_C->CTL0, RTC_C_CTL0_KEY_OFS) = 0;
	__no_operation();
	MAP_RTC_C_startClock();

	/* Enable interrupt for RTC Ready Status, which asserts when the RTC
	 * Calendar registers are ready to read.
	 * Also, enable interrupts for the Calendar alarm and Calendar event. */
	MAP_RTC_C_clearInterruptFlag(
			RTC_C_CLOCK_READ_READY_INTERRUPT | RTC_C_TIME_EVENT_INTERRUPT
					| RTC_C_CLOCK_ALARM_INTERRUPT);
	MAP_RTC_C_enableInterrupt(
			RTC_C_CLOCK_READ_READY_INTERRUPT | RTC_C_TIME_EVENT_INTERRUPT
					| RTC_C_CLOCK_ALARM_INTERRUPT);
	/* Sets a interrupt for every minute*/
	MAP_RTC_C_setCalendarEvent(RTC_C_CALENDAREVENT_MINUTECHANGE);
	MAP_RTC_C_startClock();
	MAP_Interrupt_enableInterrupt(INT_RTC_C);
	P4->DIR |= BIT3;
	P4->SEL1 |= BIT3;
	P4->SEL0 &= ~BIT3;
	RTC_C_setCalibrationFrequency(RTC_C_CALIBRATIONFREQ_512HZ);
	RTC_C_setCalibrationData(RTC_C_CALIBRATION_UP1PPM, 90);
}

void RTC_C_IRQHandler(void) {
	uint32_t status;
	status = MAP_RTC_C_getEnabledInterruptStatus();
	MAP_RTC_C_clearInterruptFlag(status);

	if (status & RTC_C_TIME_EVENT_INTERRUPT) {
		minutes--;
		if (minutes <= 0) {
			minutes = TX_INTERVAL;
		}
	}
}

