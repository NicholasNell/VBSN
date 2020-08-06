/*
 * my_timer.h
 *
 *  Created on: 27 Feb 2020
 *      Author: nicholas
 */

#ifndef MY_TIMER_H_
#define MY_TIMER_H_

/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#define MIN_ALARM_DELAY 1

/* Application Defines  */
typedef struct {
		uint32_t Time;         // Reference time
} TimerContext_t;

/*!
    \brief Initialise timer_a to count up mode. Trigger interupt when TAR equals CCR0.
    Timer will be configured for ms interupts.
*/
void TimerAInteruptInit( void );

/*!
    Timer A ISR Handler
*/
void TA1_0_IRQHandler( void );

/*!
 * Delay function 1ms resolution
 */
void Delayms( uint32_t ms );

/*!
 * \brief Sets up timer for 10 us accuracy
 */
void TimerACounterInit( void );

/*!
 * \brief Starts Timing using Timer_A0
 */
void startTimerAcounter( void );

/*!
 * \brief Stop the timer and returns time in us
 */
uint32_t stopTimerACounter ( void );

/*!
 * \brief Gets the current elapsed time in us
 */
uint32_t getTimerAcounterValue ( void );



/*!
 *  \brief Sets the timer counter back to zero
 *
 */
void resetTimerAcounterValue( void );

/*!
 * \brief Start a timer which will interupt after timeout ms
 * @param timeout, value in ms
 */
void startLoRaTimer( uint32_t timeout );

/*!
 *  Stopsthe LoRa module timer
 */

void stopLoRaTimer();

/*!
 * \brief converts time in ms to time in ticks
 *
 * \param[IN] milliseconds Time in milliseconds
 * \retval returns time in timer ticks
 */
uint32_t TimerMs2Tick( uint32_t milliseconds );

/*!
 * \brief Sets the RTC timer reference, sets also the RTC_DateStruct and RTC_TimeStruct
 *
 * \param none
 * \retval timerValue In ticks
 */
uint32_t SetTimerContext( void );

/*!
 * \brief Get the RTC timer elapsed time since the last Alarm was set
 *
 * \retval RTC Elapsed time since the last alarm in ticks.
 */
uint32_t GetTimerElapsedTime( void );

/*!
 * \brief Gets the RTC timer reference
 *
 * \retval value Timer value in ticks
 */
uint32_t GetTimerContext( void );

/*!
 * \brief Stops the Alarm
 */
void StopAlarm( void );

/*!
 * \brief Get the RTC timer value
 *
 * \retval RTC Timer value
 */
uint32_t GetTimerValue( void );

/*!
 * \brief converts time in ticks to time in ms
 *
 * \param[IN] time in timer ticks
 * \retval returns time in milliseconds
 */
uint32_t Tick2Ms( uint32_t tick );

/*!
 * \brief returns the wake up time in ticks
 *
 * \retval wake up time in ticks
 */
uint32_t GetMinimumTimeout( void );

/*!
 * \brief Sets the alarm
 *
 * \note The alarm is set at now (read in this function) + timeout
 *
 * \param timeout Duration of the Timer ticks
 */
void SetAlarm( uint32_t timeout );

/*!
 * \brief Starts wake up alarm
 *
 * \note  Alarm in RtcTimerContext.Time + timeout
 *
 * \param [IN] timeout Timeout value in ticks
 * \param [IN] flag to be set when alarm expires
 */
void StartAlarm( uint32_t timeout, bool *flag );

void startDelayTimer( void );

uint32_t stopDelayTimer( void );

uint32_t getDelayTimerValue( void );

void resetDelayTimerValue( void );
void DelayTimerInit( void );

#endif /* MY_TIMER_H_ */
