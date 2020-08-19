/*
 * my_timer.c
 *
 *  Created on: 27 Feb 2020
 *      Author: nicholas
 */

/*
 *
 * TIMER_A_0 is being used for the blocking ms delay function. using ACLK at 128khz/64 for 0.5ms accuracy and max delay of 32.768 s
 * TIMER_A_1 is being used for a general purpose counter for timing purposes using SMCLK at 1.5MHz / 48 for 32us resolution and max time of 2.09712 s
 * TIMER_A_2 is being used specifically for the LoRa radio timings. using ACLK at 128kHz/64 for 0.5ms accuracy and max delay of 32.768 s
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


/************** DELAY MS **************/

bool timerTick_ms = false;
bool isDelayTimerRunning = false;
bool LoRaTimerRunning = false;
extern bool sendFlag;

#define DELAY_TIMER TIMER_A0_BASE
#define DELAY_INT INT_TA0_0

#define COUNTER_TIMER TIMER_A1_BASE
#define COUNTER_INT INT_TA1_0

#define LORA_TIMER TIMER_A2_BASE
#define LORA_INT INT_TA2_0

#define DELAY_TICK_TO_MS 0.5
#define DELAY_MS_TO_TICK 1/DELAY_TICK_TO_MS

#define COUNTER_TICK_TO_US 32
#define COUNTER_US_TO_TICK 1/COUNTER_TICK_TO_US

#define LORA_TICK_TO_MS 0.5
#define LORA_MS_TO_TICK 1/LORA_TICK_TO_MS

// max delay of 32.768 seconds with resolution of 0.5ms
const Timer_A_ContinuousModeConfig contConfigDelay = {
		TIMER_A_CLOCKSOURCE_ACLK,
		TIMER_A_CLOCKSOURCE_DIVIDER_64,
		TIMER_A_TAIE_INTERRUPT_DISABLE,
		TIMER_A_SKIP_CLEAR };

const Timer_A_ContinuousModeConfig contConfigCounter = {
		TIMER_A_CLOCKSOURCE_SMCLK,
		TIMER_A_CLOCKSOURCE_DIVIDER_48,
		TIMER_A_TAIE_INTERRUPT_ENABLE,      // Enable Overflow ISR
		TIMER_A_DO_CLEAR };

bool isTimerAcounterRunning = false;

void Delayms( uint32_t ms ) {

	stopDelayTimer();
	const Timer_A_UpModeConfig upDelayConfig = {
			TIMER_A_CLOCKSOURCE_ACLK,
			TIMER_A_CLOCKSOURCE_DIVIDER_64,
			ms * DELAY_MS_TO_TICK,
			TIMER_A_TAIE_INTERRUPT_DISABLE,
			TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,
			TIMER_A_DO_CLEAR };
	MAP_Timer_A_configureUpMode(DELAY_TIMER, &upDelayConfig);
	if (!isDelayTimerRunning) startDelayTimer();
	uint32_t oldValue = getDelayTimerValue();
	uint32_t newValue = oldValue;
	while (newValue - oldValue < ms) {
		newValue = getDelayTimerValue();
	}
	stopDelayTimer();
}

void DelayTimerInit( void ) {
	/* Configure Timer_A3 for timing purposes */
//	MAP_Timer_A_configureContinuousMode(DELAY_TIMER, &contConfigDelay);

	MAP_Interrupt_enableInterrupt(DELAY_INT);
}

void startDelayTimer( void ) {


	MAP_Timer_A_stopTimer(DELAY_TIMER);
	MAP_Timer_A_clearTimer(TIMER_A3_BASE);
	MAP_Timer_A_startCounter(DELAY_TIMER, TIMER_A_UP_MODE);
	isDelayTimerRunning = true;
}

uint32_t stopDelayTimer( void ) {
	float timeVal = MAP_Timer_A_getCounterValue(DELAY_TIMER);
	MAP_Timer_A_stopTimer(DELAY_TIMER);
	MAP_Timer_A_clearTimer(TIMER_A3_BASE);
	isDelayTimerRunning = false;
	return (uint32_t) timeVal;
}

uint32_t getDelayTimerValue( void ) {
	uint16_t tickVal = Timer_A_getCounterValue(DELAY_TIMER);
	uint32_t timeVal = tickVal * DELAY_TICK_TO_MS;
	return timeVal;
}

void resetDelayTimerValue( void ) {
	MAP_Timer_A_stopTimer(DELAY_TIMER);
	MAP_Timer_A_clearTimer(DELAY_TIMER);
	DelayTimerInit();
	MAP_Timer_A_stopTimer(DELAY_TIMER);
//	startDelayTimer();
	isDelayTimerRunning = true;
}

/*!
 * \brief Sets up timer for \link TICK_TIME_A3_CONT \endlink us accuracy
 */
void TimerACounterInit( void ) {

	/* Configure Timer_A3 for timing purposes */

	MAP_Timer_A_configureContinuousMode(COUNTER_TIMER, &contConfigCounter);
}


void startTimerAcounter( void ) {
	Timer_A_stopTimer(COUNTER_TIMER);
//	Timer_A_clearTimer(TIMER_A3_BASE);
	Timer_A_startCounter(COUNTER_TIMER, TIMER_A_CONTINUOUS_MODE);
	isTimerAcounterRunning = true;
}

/*!
 * \brief Stop the timer and returns time in us
 */
uint32_t stopTimerACounter( void ) {
	uint16_t tickVal = Timer_A_getCounterValue(COUNTER_TIMER);
	Timer_A_stopTimer(COUNTER_TIMER);
	Timer_A_clearTimer(TIMER_A3_BASE);
	isTimerAcounterRunning = false;
	TimerACounterInit();

	return (uint32_t) tickVal * COUNTER_TICK_TO_US;
}

/*!
 * \brief Gets the current elapsed time in us.
 *
 */

uint32_t getTimerAcounterValue( void ) {
	return Timer_A_getCounterValue(COUNTER_TIMER) * COUNTER_TICK_TO_US;
}

/*!
 *  \brief Sets the timer counter back to zero
 *
 */
void resetTimerAcounterValue( void ) {
	Timer_A_stopTimer(COUNTER_TIMER);
	isTimerAcounterRunning = false;
	Timer_A_clearTimer(COUNTER_TIMER);
	TimerACounterInit();
	startTimerAcounter();
	isTimerAcounterRunning = true;
}

/*!
 * \brief Start a timer which will interrupt after timeout ms. Used specifically for LoRa radio.
 * @param timeout, value in ms
 */
void startLoRaTimer( uint32_t timeout ) {


	uint16_t timer_period = (uint16_t) timeout * LORA_MS_TO_TICK;

	Timer_A_UpModeConfig upConfigA2 = {
	TIMER_A_CLOCKSOURCE_ACLK,
										TIMER_A_CLOCKSOURCE_DIVIDER_64,
										timer_period,
			TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
			TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,    // Enable CCR0 interrupt
										TIMER_A_DO_CLEAR         // Clear value
			};

	Interrupt_enableInterrupt(LORA_INT);
	Timer_A_configureUpMode(LORA_TIMER, &upConfigA2);
	Timer_A_startCounter(LORA_TIMER, TIMER_A_UP_MODE);
}

void stopLoRaTimer( ) {
	Timer_A_clearInterruptFlag(LORA_TIMER);
	MAP_Timer_A_clearCaptureCompareInterrupt(LORA_TIMER,
	TIMER_A_CAPTURECOMPARE_REGISTER_0);
	Timer_A_clearTimer(LORA_TIMER);
	Timer_A_stopTimer(LORA_TIMER);
}

uint32_t LoRaTimerMs2Tick( uint32_t milliseconds ) {
	return milliseconds * LORA_MS_TO_TICK;
}

uint32_t SetTimerContext( void ) {
	TimerContext.Time = (uint32_t) Timer_A_getCounterValue(LORA_TIMER);
	return (uint32_t) TimerContext.Time;
}

uint32_t GetTimerElapsedTime( void ) {

	uint32_t calendarValue = (uint32_t) Timer_A_getCounterValue(LORA_TIMER);

	return ((uint32_t) (calendarValue - TimerContext.Time));
}

uint32_t GetTimerContext( void ) {
	return TimerContext.Time;
}

void StopAlarm( void ) {
	// Disable the Alarm A interrupt
	Timer_A_disableInterrupt(LORA_TIMER);
	Timer_A_clearInterruptFlag(LORA_TIMER);
	Timer_A_clearCaptureCompareInterrupt(
	LORA_TIMER,
	TIMER_A_CAPTURECOMPARE_REGISTER_0);
}

uint32_t GetTimerValue( void ) {
	uint32_t calendarValue = Timer_A_getCounterValue(LORA_TIMER);
	return (calendarValue);
}

uint32_t LoRaTick2Ms( uint32_t tick ) {
	return tick * LORA_TICK_TO_MS;
}

uint32_t GetMinimumTimeout( void ) {
	return ( MIN_ALARM_DELAY);
}

/*
 #define TIMER_A_CAPTURECOMPARE_REGISTER_0                                  0x02
 #define TIMER_A_CAPTURECOMPARE_REGISTER_1                                  0x04
 #define TIMER_A_CAPTURECOMPARE_REGISTER_2                                  0x06
 #define TIMER_A_CAPTURECOMPARE_REGISTER_3                                  0x08
 #define TIMER_A_CAPTURECOMPARE_REGISTER_4                                  0x0A
 #define TIMER_A_CAPTURECOMPARE_REGISTER_5                                  0x0C
 #define TIMER_A_CAPTURECOMPARE_REGISTER_6                                  0x0E
 */
void SetAlarm( uint32_t timeout ) {
	Timer_A_CompareModeConfig CMC = {
	TIMER_A_CAPTURECOMPARE_REGISTER_0,
										TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE,
										TIMER_A_OUTPUTMODE_OUTBITVALUE };
	//	Timer_A_enableInterrupt(TIMER_A3_BASE);
	uint32_t nowTime = Timer_A_getCounterValue(LORA_TIMER);
	uint32_t CCRTime = nowTime + timeout;
	Timer_A_initCompare(LORA_TIMER, &CMC);
	Timer_A_setCompareValue(
	LORA_TIMER,
	TIMER_A_CAPTURECOMPARE_REGISTER_0, CCRTime);
}

void StartAlarm( uint32_t timeout ) {

	Timer_A_CompareModeConfig CMC = {
	TIMER_A_CAPTURECOMPARE_REGISTER_0,
										TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE,
										TIMER_A_OUTPUTMODE_OUTBITVALUE };
//	Timer_A_enableInterrupt(TIMER_A3_BASE);
	uint32_t nowTime = Timer_A_getCounterValue(LORA_TIMER);
	uint32_t CCRTime = nowTime + timeout;
	Timer_A_initCompare(LORA_TIMER, &CMC);
	Timer_A_setCompareValue(
	LORA_TIMER,
	TIMER_A_CAPTURECOMPARE_REGISTER_0, CCRTime);

}


void TA0_0_IRQHandler( void ) {

	Timer_A_clearInterruptFlag(DELAY_TIMER);
	MAP_Timer_A_clearCaptureCompareInterrupt(DELAY_TIMER,
	TIMER_A_CAPTURECOMPARE_REGISTER_0);

}

void TA1_0_IRQHandler( void ) {

	Timer_A_clearInterruptFlag(COUNTER_TIMER);
	MAP_Timer_A_clearCaptureCompareInterrupt(COUNTER_TIMER,
	TIMER_A_CAPTURECOMPARE_REGISTER_0);

}

void TA2_0_IRQHandler( void ) {

	Timer_A_clearInterruptFlag(LORA_TIMER);
	MAP_Timer_A_clearCaptureCompareInterrupt(LORA_TIMER,
	TIMER_A_CAPTURECOMPARE_REGISTER_0);
//	Timer_A_stopTimer(TIMER_A3_BASE);
	TimerIrqHandler();


}




