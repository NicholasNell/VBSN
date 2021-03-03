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

void user_delay_ms(uint32_t period, void *intf_ptr); // only use for bme280

/*!
 \brief Initialise timer_a to count up mode. Trigger interupt when TAR equals CCR0.
 Timer will be configured for ms interupts.
 */
/*!
 \brief Initialise timer_a to count up mode. Trigger interupt when TAR equals CCR0.
 Timer will be configured for ms interupts.
 */
void timer_a_interupt_init(void);

/*!
 Timer A ISR Handler
 */
void TA1_0_IRQHandler(void);

/*!
 * Delay function 1ms resolution
 */
void delay_ms(uint32_t ms);

/*!
 * \brief Sets up timer for 10 us accuracy
 */
void timer_a_counter_init(void);

/*!
 * \brief Starts Timing using Timer_A0
 */
void start_timer_a_counter(uint32_t period, bool *flag);

/*!
 * \brief Stop the timer and returns time in us
 */
uint32_t stop_timer_a_counter(void);

/*!
 * \brief Gets the current elapsed time in us
 */
uint32_t get_timer_a_counter_value(void);

/*!
 *  \brief Sets the timer counter back to zero
 *
 */
void reset_timer_a_counter_value(void);

/*!
 * \brief Start a timer which will interupt after timeout ms
 * @param timeout, value in ms
 */
void start_lora_timer(uint32_t timeout);

/*!
 *  Stopsthe LoRa module timer
 */

void stop_lora_timer();

/*!
 * \brief converts time in ms to time in ticks
 *
 * \param[IN] milliseconds Time in milliseconds
 * \retval returns time in timer ticks
 */
uint32_t lora_timer_ms_to_tick(uint32_t milliseconds);

/*!
 * \brief Sets the RTC timer reference, sets also the RTC_DateStruct and RTC_TimeStruct
 *
 * \param none
 * \retval timerValue In ticks
 */
uint32_t set_timer_context(void);

/*!
 * \brief Get the RTC timer elapsed time since the last Alarm was set
 *
 * \retval RTC Elapsed time since the last alarm in ticks.
 */
uint32_t get_timer_elapsed_time(void);

/*!
 * \brief Gets the RTC timer reference
 *
 * \retval value Timer value in ticks
 */
uint32_t get_timer_context(void);

/*!
 * \brief Stops the Alarm
 */
void stop_alarm(void);

/*!
 * \brief Get the RTC timer value
 *
 * \retval RTC Timer value
 */
uint32_t get_timer_value(void);

/*!
 * \brief converts time in ticks to time in ms
 *
 * \param[IN] time in timer ticks
 * \retval returns time in milliseconds
 */
uint32_t lora_tick_to_ms(uint32_t tick);

/*!
 * \brief returns the wake up time in ticks
 *
 * \retval wake up time in ticks
 */
uint32_t get_minimum_timeout(void);

/*!
 * \brief Sets the alarm
 *
 * \note The alarm is set at now (read in this function) + timeout
 *
 * \param timeout Duration of the Timer ticks
 */
void set_alarm(uint32_t timeout);

/*!
 * \brief Starts wake up alarm
 *
 * \note  Alarm in RtcTimerContext.Time + timeout
 *
 * \param [IN] timeout Timeout value in ticks
 * \param [IN] flag to be set when alarm expires
 */
void start_alarm(uint32_t timeout);

void start_delay_timer(void);

uint32_t stop_delay_timer(void);

uint32_t get_delay_timer_value(void);

void reset_delay_timer_value(void);
void delay_timer_init(void);

void start_timer_32_counter(uint32_t period, bool *flag);
uint32_t get_timer_32_ticks_remaining();

#endif /* MY_TIMER_H_ */
