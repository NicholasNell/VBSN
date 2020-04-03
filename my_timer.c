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

/*
 Global variables
 */
/*
 Period (s) = (TIMER_PERIOD * DIVIDER) / (SMCLK = 3MHz)
 */
/* Application Defines  */
#define TIMER_PERIOD    375
#define REFO 32765.0
#define TICK_TIME_A0_CONT (1.0 / (REFO / TIMER_A_CLOCKSOURCE_DIVIDER_16)) * 1000000.0  /* time in us per tick*/

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
		TIMER_A_CLOCKSOURCE_DIVIDER_16,
		TIMER_A_TAIE_INTERRUPT_DISABLE,
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
	MAP_CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);
	/* Configure Timer_A0 for timing purposes */
	Timer_A_configureContinuousMode(TIMER_A0_BASE, &contConfig0);
}

/*!
 * \brief Starts Timing using Timer_A0
 */
void startTiming( void ) {
	Timer_A_stopTimer(TIMER_A0_BASE);
	Timer_A_clearTimer(TIMER_A0_BASE);
	Timer_A_startCounter(TIMER_A0_BASE, TIMER_A_CONTINUOUS_MODE);
}

/*!
 * \brief Stop the timer and returns time in us
 */
uint32_t stopTiming( void ) {
	float timeVal = Timer_A_getCounterValue(TIMER_A0_BASE) * TICK_TIME_A0_CONT;
	Timer_A_stopTimer(TIMER_A0_BASE);
	Timer_A_clearTimer(TIMER_A0_BASE);
	return (uint32_t) timeVal;
}

/*!
 * \brief Gets the current elapsed time in us Max 32.00244163 sec
 */
uint32_t getTiming( void ) {
	uint16_t tickVal = Timer_A_getCounterValue(TIMER_A0_BASE);
	uint32_t timeVal = tickVal * TICK_TIME_A0_CONT;
	return timeVal;
}

/*!
 * \brief Start a timer which will interupt after timeout ms. Used specifically for LoRa radio.
 * @param timeout, value in ms
 */
void startLoRaTimer( uint32_t timeout ) {
	float time = timeout / 1000.0;
	time = time * 1.5E6 / 48.0;
	uint16_t timer_period = (uint16_t) time;

	Timer_A_UpModeConfig upConfigA2 = {
	TIMER_A_CLOCKSOURCE_SMCLK,              // SMCLK Clock Source
			TIMER_A_CLOCKSOURCE_DIVIDER_48,          // SMCLK/1 = 1.5MHz
			timer_period,                           // 375 tick period
			TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
			TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,    // Enable CCR0 interrupt
			TIMER_A_SKIP_CLEAR                         // Clear value
			};

	Interrupt_enableInterrupt(INT_TA2_0);
	Timer_A_configureUpMode(TIMER_A2_BASE, &upConfigA2);
	Timer_A_startCounter(TIMER_A2_BASE, TIMER_A_UP_MODE);
}

void TA2_0_IRQHandler( void ) {
	Timer_A_clearInterruptFlag(TIMER_A2_BASE);
	MAP_Timer_A_clearCaptureCompareInterrupt(TIMER_A2_BASE,
	TIMER_A_CAPTURECOMPARE_REGISTER_0);
	Timer_A_stopTimer(TIMER_A2_BASE);
	RadioTimeoutFlag = true;
//	SX1276SetSleep();
}
