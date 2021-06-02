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

#include <my_timer.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>
#include <ti/devices/msp432p4xx/driverlib/rom_map.h>
#include <ti/devices/msp432p4xx/driverlib/timer_a.h>
#include <ti/devices/msp432p4xx/driverlib/timer32.h>
#include <ti/devices/msp432p4xx/inc/msp432p401r.h>
#include <timer.h>

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

static volatile bool *timerAtempFlag;
static volatile bool *timer32tempFlag;

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

void delay_ms(uint32_t ms) {

	stop_delay_timer();
	start_delay_timer();
	uint32_t oldValue = get_delay_timer_value();
	uint32_t newValue = oldValue;
	while (newValue - oldValue < ms) {
		newValue = get_delay_timer_value();
	}
//	stopDelayTimer();
}

void user_delay_ms(uint32_t period, void *intf_ptr) {
	/*
	 * Return control or wait,
	 * for a period amount of milliseconds
	 */
	delay_ms(period / 1000.0);
}

/*!
 * \brief Timer with max value of approx. 47.5 minutes.
 * @param period timer count in ms
 * @param flag	flag to be set true
 */
void start_timer_32_counter(uint32_t period, bool *flag) {
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
	*timer32tempFlag = true;
	MAP_Timer32_clearInterruptFlag(TIMER32_BASE);
	Timer32_haltTimer(TIMER32_0_BASE);
}

uint32_t get_timer_32_ticks_remaining() {
	return TIMER32_1->VALUE;
}

void delay_timer_init(void) {
	/* Configure Timer_A3 for timing purposes */
//	MAP_Timer_A_configureContinuousMode(DELAY_TIMER, &contConfigDelay);
	MAP_Timer_A_configureUpMode(DELAY_TIMER, &upDelayConfig);
	MAP_Interrupt_enableInterrupt(DELAY_INT);
}

void start_delay_timer(void) {

	MAP_Timer_A_stopTimer(DELAY_TIMER);
	MAP_Timer_A_clearTimer(TIMER_A3_BASE);
	MAP_Timer_A_startCounter(DELAY_TIMER, TIMER_A_UP_MODE);

}

uint32_t stop_delay_timer(void) {
	float timeVal = MAP_Timer_A_getCounterValue(DELAY_TIMER);
	MAP_Timer_A_stopTimer(DELAY_TIMER);
	MAP_Timer_A_clearTimer(DELAY_TIMER);

	return (uint32_t) timeVal;
}

uint32_t get_delay_timer_value(void) {
//	uint16_t tickVal = ;
//	uint32_t timeVal = tickVal *;
	return MAP_Timer_A_getCounterValue(DELAY_TIMER) * DELAY_TICK_TO_MS;
}

void reset_delay_timer_value(void) {
	MAP_Timer_A_stopTimer(DELAY_TIMER);
	MAP_Timer_A_clearTimer(DELAY_TIMER);
	delay_timer_init();
	MAP_Timer_A_stopTimer(DELAY_TIMER);
//	startDelayTimer();

}

/*!
 * \brief Sets up timer for \link TICK_TIME_A3_CONT \endlink us accuracy
 */
void timer_a_counter_init(void) {

	/* Configure Timer_A3 for timing purposes */
	MAP_Timer_A_configureUpMode(COUNTER_TIMER, &upConfigCounter);
}

void start_timer_a_counter(uint32_t period, bool *flag) {
	timerAtempFlag = flag;
	MAP_Timer_A_stopTimer(COUNTER_TIMER);
	upConfigCounter.timerPeriod = period * COUNTER_MS_TO_TICK;
	MAP_Timer_A_configureUpMode(COUNTER_TIMER, &upConfigCounter);
	MAP_Interrupt_enableInterrupt(COUNTER_INT);
	MAP_Timer_A_startCounter(COUNTER_TIMER, TIMER_A_UP_MODE);

}

/*!
 * \brief Stop the timer and returns time in us
 */
uint32_t stop_timer_a_counter(void) {
	uint16_t tickVal = Timer_A_getCounterValue(COUNTER_TIMER);
	MAP_Timer_A_stopTimer(COUNTER_TIMER);
	MAP_Timer_A_clearTimer(COUNTER_TIMER);

	return (uint32_t) tickVal * COUNTER_TICK_TO_MS;
}

/*!
 * \brief Gets the current elapsed time in us.
 *
 */

uint32_t get_timer_a_counter_value(void) {
	return MAP_Timer_A_getCounterValue(COUNTER_TIMER) * COUNTER_TICK_TO_MS;
}

/*!
 *  \brief Sets the timer counter back to zero
 *
 */
void reset_timer_a_counter_value(void) {
	MAP_Timer_A_stopTimer(COUNTER_TIMER);

	MAP_Timer_A_clearTimer(COUNTER_TIMER);
	timer_a_counter_init();

}

/*!
 * \brief Start a timer which will interrupt after timeout ms. Used specifically for LoRa radio.
 * @param timeout, value in ms
 */
void start_lora_timer(uint32_t timeout) {

	uint16_t timer_period = (uint16_t) timeout * LORA_MS_TO_TICK;

	Timer_A_UpModeConfig upConfigA2 = {
	TIMER_A_CLOCKSOURCE_ACLK,
	TIMER_A_CLOCKSOURCE_DIVIDER_64, timer_period,
	TIMER_A_TAIE_INTERRUPT_DISABLE, // Disable Timer interrupt
			TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE, // Enable CCR0 interrupt
			TIMER_A_DO_CLEAR         // Clear value
			};

	MAP_Interrupt_enableInterrupt(LORA_INT);
	MAP_Timer_A_configureUpMode(LORA_TIMER, &upConfigA2);
	MAP_Timer_A_startCounter(LORA_TIMER, TIMER_A_UP_MODE);
}

void stop_lora_timer() {
	MAP_Timer_A_clearInterruptFlag(LORA_TIMER);
	MAP_Timer_A_clearCaptureCompareInterrupt(LORA_TIMER,
	TIMER_A_CAPTURECOMPARE_REGISTER_0);
	MAP_Timer_A_clearTimer(LORA_TIMER);
	MAP_Timer_A_stopTimer(LORA_TIMER);
}

uint32_t lora_timer_ms_to_tick(uint32_t milliseconds) {
	return milliseconds * LORA_MS_TO_TICK;
}

uint32_t set_timer_context(void) {
	TimerContext.Time = (uint32_t) MAP_Timer_A_getCounterValue(LORA_TIMER);
	return (uint32_t) TimerContext.Time;
}

uint32_t get_timer_elapsed_time(void) {

	uint32_t calendarValue = (uint32_t) MAP_Timer_A_getCounterValue(LORA_TIMER);

	return ((uint32_t) (calendarValue - TimerContext.Time));
}

uint32_t get_timer_context(void) {
	return TimerContext.Time;
}

void stop_alarm(void) {
	// Disable the Alarm A interrupt
	MAP_Timer_A_disableInterrupt(LORA_TIMER);
	MAP_Timer_A_clearInterruptFlag(LORA_TIMER);
	MAP_Timer_A_clearCaptureCompareInterrupt(
	LORA_TIMER,
	TIMER_A_CAPTURECOMPARE_REGISTER_0);
}

uint32_t get_timer_value(void) {
	uint32_t calendarValue = MAP_Timer_A_getCounterValue(LORA_TIMER);
	return (calendarValue);
}

uint32_t lora_tick_to_ms(uint32_t tick) {
	return tick * LORA_TICK_TO_MS;
}

uint32_t get_minimum_timeout(void) {
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
void set_alarm(uint32_t timeout) {
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

void start_alarm(uint32_t timeout) {

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
	*timerAtempFlag = !(*timerAtempFlag);
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
	timer_irq_handler();

}

