/*
 * my_timer.c
 *
 *  Created on: 27 Feb 2020
 *      Author: nicholas
 */

#include "my_timer.h"
#include "utilities.h"
#include "board.h"
#include "my_RFM9x.h"
#include "timer.h"

/*
 Global variables
 */
/*!
 * Keep the value of the RTC timer when the RTC alarm is set
 * Set with the \ref RtcSetTimerContext function
 * Value is kept as a Reference to calculate alarm
 */
static TimerContext_t TimerContext;
/*
 Period (s) = (TIMER_PERIOD * DIVIDER) / (SMCLK = 3MHz)
 */
/* Application Defines  */
#define TIMER_PERIOD    375
#define REFO 128E3
#define TICK_TIME_A0_CONT (1.0 / (REFO / 128)) * 1000000.0  /* time in us per tick*/

/* Timer_A UpMode Configuration Parameter */
const Timer_A_UpModeConfig upConfig = {
TIMER_A_CLOCKSOURCE_SMCLK,              // SMCLK Clock Source
		TIMER_A_CLOCKSOURCE_DIVIDER_4,          // SMCLK/1 = 1.5MHz
		TIMER_PERIOD,                           // 375 tick period
		TIMER_A_TAIE_INTERRUPT_ENABLE,         // enable Timer interrupt
		TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,    // Enable CCR0 interrupt
		TIMER_A_SKIP_CLEAR                         // Clear value
		};

/* Configure Timer_A0 as a continuous counter for timing using aux clock sourced from REFO at 32.765kHz / 16*/
const Timer_A_ContinuousModeConfig contConfig0 = {
		TIMER_A_CLOCKSOURCE_ACLK,
		TIMER_A_CLOCKSOURCE_DIVIDER_1,
		TIMER_A_TAIE_INTERRUPT_ENABLE,
		TIMER_A_SKIP_CLEAR };

bool TimerInteruptFlag = false;
extern bool RadioTimeoutFlag;

void TimerAInteruptInit( void ) {

	/* Configuring Timer_A1 for Up Mode */
	MAP_Timer_A_configureUpMode(TIMER_A1_BASE, &upConfig);
	Timer_A_enableInterrupt(TIMER_A1_BASE);

	/* Enabling interrupts and starting the timer */
//    MAP_Interrupt_enableSleepOnIsrExit();
	MAP_Interrupt_enableInterrupt(INT_TA1_0);

	/* Enabling MASTER interrupts */
	MAP_Interrupt_enableMaster();
}

void TA1_0_IRQHandler( void ) {
	TimerInteruptFlag = true;
	Timer_A_clearInterruptFlag(TIMER_A1_BASE);
	Timer_A_clearCaptureCompareInterrupt(TIMER_A1_BASE,
	TIMER_A_CAPTURECOMPARE_REGISTER_0);
}

void Delayms( uint32_t ms ) {
	uint32_t count = 0;
//    MAP_Timer_A_stopTimer(TIMER_A1_BASE);
//    MAP_Timer_A_clearTimer(TIMER_A1_BASE);
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

/*!
 * \brief Sets up timer for TICK_TIME_A0_CONT us accuracy
 */
void TimerATimerInit( void ) {
	MAP_CS_setReferenceOscillatorFrequency(CS_REFO_128KHZ);
	MAP_CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_128);
	/* Configure Timer_A0 for timing purposes */
	Timer_A_enableInterrupt(TIMER_A3_BASE);
	Timer_A_enableCaptureCompareInterrupt(
	TIMER_A3_BASE,
	TIMER_A_CAPTURECOMPARE_REGISTER_0);
	Timer_A_configureContinuousMode(TIMER_A3_BASE, &contConfig0);
	MAP_Interrupt_enableInterrupt(INT_TA3_0);
}

/*!
 * \brief Starts Timing using Timer_A0
 */
void startTiming( void ) {
	Timer_A_stopTimer(TIMER_A3_BASE);
//	Timer_A_clearTimer(TIMER_A3_BASE);
	Timer_A_startCounter(TIMER_A3_BASE, TIMER_A_CONTINUOUS_MODE);
}

/*!
 * \brief Stop the timer and returns time in us
 */
uint32_t stopTiming( void ) {
	float timeVal = Timer_A_getCounterValue(TIMER_A3_BASE) * TICK_TIME_A0_CONT;
	Timer_A_stopTimer(TIMER_A3_BASE);
	Timer_A_clearTimer(TIMER_A3_BASE);
	return (uint32_t) timeVal;
}

/*!
 * \brief Gets the current elapsed time in us. Max 65.535 sec
 */
uint32_t getTiming( void ) {
	uint16_t tickVal = Timer_A_getCounterValue(TIMER_A3_BASE);
	uint32_t timeVal = tickVal * TICK_TIME_A0_CONT;
	return timeVal;
}

/*!
 * \brief Start a timer which will interrupt after timeout ms. Used specifically for LoRa radio.
 * @param timeout, value in ms
 */
void startLoRaTimer( uint32_t timeout ) {
	float time = timeout / 1000.0;
	time = time * 32765 / 12.0;
	uint16_t timer_period = (uint16_t) time;

	Timer_A_UpModeConfig upConfigA2 = {
	TIMER_A_CLOCKSOURCE_ACLK, // SMCLK Clock Source
			TIMER_A_CLOCKSOURCE_DIVIDER_12,          // SMCLK/1 = 1.5MHz
			timer_period,                           // 375 tick period
			TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
			TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,    // Enable CCR0 interrupt
			TIMER_A_SKIP_CLEAR                         // Clear value
			};

	Interrupt_enableInterrupt(INT_TA2_0);
	Timer_A_configureUpMode(TIMER_A2_BASE, &upConfigA2);
	Timer_A_startCounter(TIMER_A2_BASE, TIMER_A_UP_MODE);
}

void stopLoRaTimer( ) {
	Timer_A_clearInterruptFlag(TIMER_A2_BASE);
	MAP_Timer_A_clearCaptureCompareInterrupt(TIMER_A2_BASE,
	TIMER_A_CAPTURECOMPARE_REGISTER_0);
	Timer_A_clearTimer(TIMER_A2_BASE);
	Timer_A_stopTimer(TIMER_A2_BASE);
}

void TA2_0_IRQHandler( void ) {
	Timer_A_clearInterruptFlag(TIMER_A2_BASE);
	MAP_Timer_A_clearCaptureCompareInterrupt(TIMER_A2_BASE,
	TIMER_A_CAPTURECOMPARE_REGISTER_0);
	Timer_A_stopTimer(TIMER_A2_BASE);
	RadioTimeoutFlag = true;
//	SX1276SetSleep();
}



uint32_t TimerMs2Tick( uint32_t milliseconds ) {
	return milliseconds;
}

uint32_t SetTimerContext( void ) {
	TimerContext.Time = (uint32_t) Timer_A_getCounterValue(TIMER_A3_BASE);
	return (uint32_t) TimerContext.Time;
}

uint32_t GetTimerElapsedTime( void ) {

	uint32_t calendarValue = (uint32_t) Timer_A_getCounterValue(TIMER_A3_BASE);

	return ((uint32_t) (calendarValue - TimerContext.Time));
}

uint32_t GetTimerContext( void ) {
	return TimerContext.Time;
}

void StopAlarm( void ) {
	// Disable the Alarm A interrupt
	Timer_A_disableInterrupt(TIMER_A3_BASE);
	Timer_A_clearInterruptFlag(TIMER_A3_BASE);
	Timer_A_clearCaptureCompareInterrupt(
	TIMER_A3_BASE,
	TIMER_A_CAPTURECOMPARE_REGISTER_0);
}

uint32_t GetTimerValue( void ) {
	uint32_t calendarValue = Timer_A_getCounterValue(TIMER_A3_BASE);
	return (calendarValue);
}

uint32_t Tick2Ms( uint32_t tick ) {
	return tick;
}

uint32_t GetMinimumTimeout( void ) {
	return ( MIN_ALARM_DELAY);
}

void SetAlarm( uint32_t timeout ) {
	Timer_A_CompareModeConfig CMC = {
	TIMER_A_CAPTURECOMPARE_REGISTER_0,
										TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE,
										TIMER_A_OUTPUTMODE_OUTBITVALUE };
	//	Timer_A_enableInterrupt(TIMER_A3_BASE);
	uint32_t nowTime = Timer_A_getCounterValue(TIMER_A3_BASE);
	uint32_t CCRTime = nowTime + timeout;
	Timer_A_initCompare(TIMER_A3_BASE, &CMC);
	Timer_A_setCompareValue(
	TIMER_A3_BASE,
	TIMER_A_CAPTURECOMPARE_REGISTER_0, CCRTime);
}

void StartAlarm( uint32_t timeout ) {
	Timer_A_CompareModeConfig CMC = {
	TIMER_A_CAPTURECOMPARE_REGISTER_0,
										TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE,
										TIMER_A_OUTPUTMODE_OUTBITVALUE };
//	Timer_A_enableInterrupt(TIMER_A3_BASE);
	uint32_t nowTime = Timer_A_getCounterValue(TIMER_A3_BASE);
	uint32_t CCRTime = nowTime + timeout;
	Timer_A_initCompare(TIMER_A3_BASE, &CMC);
	Timer_A_setCompareValue(
	TIMER_A3_BASE,
	TIMER_A_CAPTURECOMPARE_REGISTER_0, CCRTime);
	/*
	Timer_A_enableCaptureCompareInterrupt(
	 TIMER_A3_BASE,
	TIMER_A_CAPTURECOMPARE_REGISTER_0);
	 */


	/*uint16_t rtcAlarmSubSeconds = 0;
	 uint16_t rtcAlarmSeconds = 0;
	 uint16_t rtcAlarmMinutes = 0;
	 uint16_t rtcAlarmHours = 0;
	 uint16_t rtcAlarmDays = 0;
	 RTC_TimeTypeDef time = RtcTimerContext.CalendarTime;
	 RTC_DateTypeDef date = RtcTimerContext.CalendarDate;

	 StopAlarm();

	 reverse counter
	 rtcAlarmSubSeconds = PREDIV_S - time.SubSeconds;
	 rtcAlarmSubSeconds += (timeout & PREDIV_S);
	 // convert timeout  to seconds
	 timeout >>= N_PREDIV_S;

	 // Convert microsecs to RTC format and add to 'Now'
	 rtcAlarmDays = date.Date;
	 while (timeout >= TM_SECONDS_IN_1DAY) {
	 timeout -= TM_SECONDS_IN_1DAY;
	 rtcAlarmDays++;
	 }

	 // Calc hours
	 rtcAlarmHours = time.Hours;
	 while (timeout >= TM_SECONDS_IN_1HOUR) {
	 timeout -= TM_SECONDS_IN_1HOUR;
	 rtcAlarmHours++;
	 }

	 // Calc minutes
	 rtcAlarmMinutes = time.Minutes;
	 while (timeout >= TM_SECONDS_IN_1MINUTE) {
	 timeout -= TM_SECONDS_IN_1MINUTE;
	 rtcAlarmMinutes++;
	 }

	 // Calc seconds
	 rtcAlarmSeconds = time.Seconds + timeout;

	 ***** Correct for modulo********
	 while (rtcAlarmSubSeconds >= (PREDIV_S + 1)) {
	 rtcAlarmSubSeconds -= (PREDIV_S + 1);
	 rtcAlarmSeconds++;
	 }

	 while (rtcAlarmSeconds >= TM_SECONDS_IN_1MINUTE) {
	 rtcAlarmSeconds -= TM_SECONDS_IN_1MINUTE;
	 rtcAlarmMinutes++;
	 }

	 while (rtcAlarmMinutes >= TM_MINUTES_IN_1HOUR) {
	 rtcAlarmMinutes -= TM_MINUTES_IN_1HOUR;
	 rtcAlarmHours++;
	 }

	 while (rtcAlarmHours >= TM_HOURS_IN_1DAY) {
	 rtcAlarmHours -= TM_HOURS_IN_1DAY;
	 rtcAlarmDays++;
	 }

	 if (date.Year % 4 == 0) {
	 if (rtcAlarmDays > DaysInMonthLeapYear[date.Month - 1]) {
	 rtcAlarmDays = rtcAlarmDays % DaysInMonthLeapYear[date.Month - 1];
	 }
	 }
	 else {
	 if (rtcAlarmDays > DaysInMonth[date.Month - 1]) {
	 rtcAlarmDays = rtcAlarmDays % DaysInMonth[date.Month - 1];
	 }
	 }

	 Set RTC_AlarmStructure with calculated values
	 RtcAlarm.AlarmTime.SubSeconds = PREDIV_S - rtcAlarmSubSeconds;
	 RtcAlarm.AlarmSubSecondMask = ALARM_SUBSECOND_MASK;
	 RtcAlarm.AlarmTime.Seconds = rtcAlarmSeconds;
	 RtcAlarm.AlarmTime.Minutes = rtcAlarmMinutes;
	 RtcAlarm.AlarmTime.Hours = rtcAlarmHours;
	 RtcAlarm.AlarmDateWeekDay = (uint8_t) rtcAlarmDays;
	 RtcAlarm.AlarmTime.TimeFormat = time.TimeFormat;
	 RtcAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
	 RtcAlarm.AlarmMask = RTC_ALARMMASK_NONE;
	 RtcAlarm.Alarm = RTC_ALARM_A;
	 RtcAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	 RtcAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;

	 // Set RTC_Alarm
	 HAL_RTC_SetAlarm_IT(&RtcHandle, &RtcAlarm, RTC_FORMAT_BIN);*/

}

void TA3_0_IRQHandler( void ) {
	Timer_A_clearInterruptFlag(TIMER_A3_BASE);
	MAP_Timer_A_clearCaptureCompareInterrupt(TIMER_A3_BASE,
	TIMER_A_CAPTURECOMPARE_REGISTER_0);
//	Timer_A_stopTimer(TIMER_A3_BASE);
	TimerIrqHandler();

}
