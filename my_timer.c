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
#include "my_MAC.h"

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
bool *timerAtempFlag;
bool *timer32tempFlag;

#define DELAY_TIMER TIMER_A0_BASE
#define DELAY_INT INT_TA0_0

#define COUNTER_TIMER TIMER_A1_BASE
#define COUNTER_INT INT_TA1_0

#define LORA_TIMER TIMER_A2_BASE
#define LORA_INT INT_TA2_0

#define DELAY_TICK_TO_MS 0.5
#define DELAY_MS_TO_TICK 1/DELAY_TICK_TO_MS

#define COUNTER_TICK_TO_MS 0.5
#define COUNTER_MS_TO_TICK 1/COUNTER_TICK_TO_MS

#define LORA_TICK_TO_MS 0.5
#define LORA_MS_TO_TICK 1/LORA_TICK_TO_MS

#define TIMER32_TICK_TO_US (1000000.0/1500000.00)
#define TIMER32_US_TO_TICK 1.0/TIMER32_TICK_TO_US

// max delay of 32.768 seconds with resolution of 0.5ms
const Timer_A_ContinuousModeConfig contConfigDelay = {
TIMER_A_CLOCKSOURCE_ACLK,
TIMER_A_CLOCKSOURCE_DIVIDER_64,
TIMER_A_TAIE_INTERRUPT_DISABLE,
TIMER_A_SKIP_CLEAR };

const Timer_A_UpModeConfig upDelayConfig = {
TIMER_A_CLOCKSOURCE_ACLK,
TIMER_A_CLOCKSOURCE_DIVIDER_64, 65535,
TIMER_A_TAIE_INTERRUPT_DISABLE,
TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,
TIMER_A_DO_CLEAR };

Timer_A_UpModeConfig upConfigCounter = {
TIMER_A_CLOCKSOURCE_ACLK,
TIMER_A_CLOCKSOURCE_DIVIDER_64, 1,
TIMER_A_TAIE_INTERRUPT_DISABLE,
TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,
TIMER_A_DO_CLEAR };

bool isTimerAcounterRunning = false;

void Delayms(uint32_t ms) {

	stopDelayTimer();
	if (!isDelayTimerRunning)
		startDelayTimer();
	uint32_t oldValue = getDelayTimerValue();
	uint32_t newValue = oldValue;
	while (newValue - oldValue < ms) {
		newValue = getDelayTimerValue();
	}
//	stopDelayTimer();
}

/*!
 * \brief Timer with max value of approx. 47.5 minutes.
 * @param period timer count in ms
 * @param flag	flag to be set true
 */
void startTimer32Counter(uint32_t period, bool *flag) {
	timer32tempFlag = flag;
	MAP_Timer32_initModule(TIMER32_0_BASE, TIMER32_PRESCALER_1, TIMER32_32BIT,
	TIMER32_PERIODIC_MODE);
	MAP_Interrupt_enableInterrupt(INT_T32_INT1);
	MAP_Timer32_setCount(TIMER32_0_BASE, period * TIMER32_US_TO_TICK * 1000);
	MAP_Timer32_enableInterrupt(TIMER32_0_BASE);
	MAP_Timer32_startTimer(TIMER32_0_BASE, false);
}

/* Timer32 ISR */
void T32_INT1_IRQHandler(void) {
	MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P2, GPIO_PIN2);
	*timer32tempFlag = true;
	MAP_Timer32_clearInterruptFlag(TIMER32_BASE);
}

void DelayTimerInit(void) {
	/* Configure Timer_A3 for timing purposes */
//	MAP_Timer_A_configureContinuousMode(DELAY_TIMER, &contConfigDelay);
	MAP_Timer_A_configureUpMode(DELAY_TIMER, &upDelayConfig);
	MAP_Interrupt_enableInterrupt(DELAY_INT);
}

void startDelayTimer(void) {

	MAP_Timer_A_stopTimer(DELAY_TIMER);
	MAP_Timer_A_clearTimer(TIMER_A3_BASE);
	MAP_Timer_A_startCounter(DELAY_TIMER, TIMER_A_UP_MODE);
	isDelayTimerRunning = true;
}

uint32_t stopDelayTimer(void) {
	float timeVal = MAP_Timer_A_getCounterValue(DELAY_TIMER);
	MAP_Timer_A_stopTimer(DELAY_TIMER);
	MAP_Timer_A_clearTimer(TIMER_A3_BASE);
	isDelayTimerRunning = false;
	return (uint32_t) timeVal;
}

uint32_t getDelayTimerValue(void) {
//	uint16_t tickVal = ;
//	uint32_t timeVal = tickVal *;
	return MAP_Timer_A_getCounterValue(DELAY_TIMER) * DELAY_TICK_TO_MS;
}

void resetDelayTimerValue(void) {
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
void TimerACounterInit(void) {

	/* Configure Timer_A3 for timing purposes */
	MAP_Timer_A_configureUpMode(COUNTER_TIMER, &upConfigCounter);
}

void startTimerAcounter(uint32_t period, bool *flag) {
	timerAtempFlag = flag;
	MAP_Timer_A_stopTimer(COUNTER_TIMER);
	upConfigCounter.timerPeriod = period * COUNTER_MS_TO_TICK;
	MAP_Timer_A_configureUpMode(COUNTER_TIMER, &upConfigCounter);
	MAP_Interrupt_enableInterrupt(COUNTER_INT);
	MAP_Timer_A_startCounter(COUNTER_TIMER, TIMER_A_UP_MODE);
	isTimerAcounterRunning = true;
}

/*!
 * \brief Stop the timer and returns time in us
 */
uint32_t stopTimerACounter(void) {
	uint16_t tickVal = Timer_A_getCounterValue(COUNTER_TIMER);
	MAP_Timer_A_stopTimer(COUNTER_TIMER);
	MAP_Timer_A_clearTimer(COUNTER_TIMER);
	isTimerAcounterRunning = false;
	return (uint32_t) tickVal * COUNTER_TICK_TO_MS;
}

/*!
 * \brief Gets the current elapsed time in us.
 *
 */

uint32_t getTimerAcounterValue(void) {
	return MAP_Timer_A_getCounterValue(COUNTER_TIMER) * COUNTER_TICK_TO_MS;
}

/*!
 *  \brief Sets the timer counter back to zero
 *
 */
void resetTimerAcounterValue(void) {
	MAP_Timer_A_stopTimer(COUNTER_TIMER);
	isTimerAcounterRunning = false;
	MAP_Timer_A_clearTimer(COUNTER_TIMER);
	TimerACounterInit();

	isTimerAcounterRunning = true;
}

/*!
 * \brief Start a timer which will interrupt after timeout ms. Used specifically for LoRa radio.
 * @param timeout, value in ms
 */
void startLoRaTimer(uint32_t timeout) {

	uint16_t timer_period = (uint16_t) timeout * LORA_MS_TO_TICK;

	Timer_A_UpModeConfig upConfigA2 = {
	TIMER_A_CLOCKSOURCE_ACLK,
	TIMER_A_CLOCKSOURCE_DIVIDER_64, timer_period,
	TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
			TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,    // Enable CCR0 interrupt
			TIMER_A_DO_CLEAR         // Clear value
			};

	MAP_Interrupt_enableInterrupt(LORA_INT);
	MAP_Timer_A_configureUpMode(LORA_TIMER, &upConfigA2);
	MAP_Timer_A_startCounter(LORA_TIMER, TIMER_A_UP_MODE);
}

void stopLoRaTimer() {
	MAP_Timer_A_clearInterruptFlag(LORA_TIMER);
	MAP_Timer_A_clearCaptureCompareInterrupt(LORA_TIMER,
	TIMER_A_CAPTURECOMPARE_REGISTER_0);
	MAP_Timer_A_clearTimer(LORA_TIMER);
	MAP_Timer_A_stopTimer(LORA_TIMER);
}

uint32_t LoRaTimerMs2Tick(uint32_t milliseconds) {
	return milliseconds * LORA_MS_TO_TICK;
}

uint32_t SetTimerContext(void) {
	TimerContext.Time = (uint32_t) MAP_Timer_A_getCounterValue(LORA_TIMER);
	return (uint32_t) TimerContext.Time;
}

uint32_t GetTimerElapsedTime(void) {

	uint32_t calendarValue = (uint32_t) MAP_Timer_A_getCounterValue(LORA_TIMER);

	return ((uint32_t) (calendarValue - TimerContext.Time));
}

uint32_t GetTimerContext(void) {
	return TimerContext.Time;
}

void StopAlarm(void) {
	// Disable the Alarm A interrupt
	MAP_Timer_A_disableInterrupt(LORA_TIMER);
	MAP_Timer_A_clearInterruptFlag(LORA_TIMER);
	MAP_Timer_A_clearCaptureCompareInterrupt(
	LORA_TIMER,
	TIMER_A_CAPTURECOMPARE_REGISTER_0);
}

uint32_t GetTimerValue(void) {
	uint32_t calendarValue = MAP_Timer_A_getCounterValue(LORA_TIMER);
	return (calendarValue);
}

uint32_t LoRaTick2Ms(uint32_t tick) {
	return tick * LORA_TICK_TO_MS;
}

uint32_t GetMinimumTimeout(void) {
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
void SetAlarm(uint32_t timeout) {
	Timer_A_CompareModeConfig CMC = {
	TIMER_A_CAPTURECOMPARE_REGISTER_0,
	TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE,
	TIMER_A_OUTPUTMODE_OUTBITVALUE };
	//	Timer_A_enableInterrupt(TIMER_A3_BASE);
	uint32_t nowTime = MAP_Timer_A_getCounterValue(LORA_TIMER);
	uint32_t CCRTime = nowTime + timeout;
	MAP_Timer_A_initCompare(LORA_TIMER, &CMC);
	MAP_Timer_A_setCompareValue(
	LORA_TIMER,
	TIMER_A_CAPTURECOMPARE_REGISTER_0, CCRTime);
}

void StartAlarm(uint32_t timeout) {

	Timer_A_CompareModeConfig CMC = {
	TIMER_A_CAPTURECOMPARE_REGISTER_0,
	TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE,
	TIMER_A_OUTPUTMODE_OUTBITVALUE };
//	Timer_A_enableInterrupt(TIMER_A3_BASE);
	uint32_t nowTime = MAP_Timer_A_getCounterValue(LORA_TIMER);
	uint32_t CCRTime = nowTime + timeout;
	MAP_Timer_A_initCompare(LORA_TIMER, &CMC);
	MAP_Timer_A_setCompareValue(
	LORA_TIMER,
	TIMER_A_CAPTURECOMPARE_REGISTER_0, CCRTime);

}

void TA0_0_IRQHandler(void) {

	MAP_Timer_A_clearInterruptFlag(DELAY_TIMER);
	MAP_Timer_A_clearCaptureCompareInterrupt(DELAY_TIMER,
	TIMER_A_CAPTURECOMPARE_REGISTER_0);

}

//Counter interupt
void TA1_0_IRQHandler(void) {
	*timerAtempFlag = true;
//	stopTimerACounter();
	MAP_Timer_A_clearCaptureCompareInterrupt(COUNTER_TIMER,
	TIMER_A_CAPTURECOMPARE_REGISTER_0);
}

/*
 * LoRa Interrupt
 */
void TA2_0_IRQHandler(void) {

	MAP_Timer_A_clearInterruptFlag(LORA_TIMER);
	MAP_Timer_A_clearCaptureCompareInterrupt(LORA_TIMER,
	TIMER_A_CAPTURECOMPARE_REGISTER_0);
//	Timer_A_stopTimer(TIMER_A3_BASE);
	TimerIrqHandler();

}

