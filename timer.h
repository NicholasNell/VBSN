/*!
 * \file      timer.h
 *
 * \brief     Timer objects and scheduling management implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */
#ifndef __TIMER_H__
#define __TIMER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/*!
 * \brief Timer object description
 */
typedef struct TimerEvent_s {
	uint32_t Timestamp;                  //! Current timer value
	uint32_t ReloadValue;                //! Timer delay value
	bool IsStarted;                   //! Is the timer currently running
	bool IsNext2Expire;                  //! Is the next timer to expire
	void (*Callback)(void *context); //! Timer IRQ callback function
	void *Context;     //! User defined data object pointer to pass back
	struct TimerEvent_s *Next;     //! Pointer to the next Timer object.
} TimerEvent_t;

/*!
 * \brief Timer time variable definition
 */
#ifndef TimerTime_t
typedef uint32_t TimerTime_t;
#define TIMERTIME_T_MAX                             ( ( uint32_t )~0 )
#endif

#define TIMERTIME_T_MIN 1

/*!
 * \brief Initializes the timer object
 *
 * \remark TimerSetValue function must be called before starting the timer.
 *         this function initializes timestamp and reload value at 0.
 *
 * \param [IN] obj          Structure containing the timer object parameters
 * \param [IN] callback     Function callback called at the end of the timeout
 */
void timer_init(TimerEvent_t *obj, void (*callback)(void *context));

/*!
 * \brief Sets a user defined object pointer
 *
 * \param [IN] context User defined data object pointer to pass back
 *                     on IRQ handler callback
 */
void timer_set_context(TimerEvent_t *obj, void *context);

/*!
 * Timer IRQ event handler
 */
void timer_irq_handler(void);

/*!
 * \brief Starts and adds the timer object to the list of timer events
 *
 * \param [IN] obj Structure containing the timer object parameters
 */
void timer_start(TimerEvent_t *obj);

/*!
 * \brief Checks if the provided timer is running
 *
 * \param [IN] obj Structure containing the timer object parameters
 *
 * \retval status  returns the timer activity status [true: Started,
 *                                                    false: Stopped]
 */
bool timer_is_started(TimerEvent_t *obj);

/*!
 * \brief Stops and removes the timer object from the list of timer events
 *
 * \param [IN] obj Structure containing the timer object parameters
 */
void timer_stop(TimerEvent_t *obj);

/*!
 * \brief Resets the timer object
 *
 * \param [IN] obj Structure containing the timer object parameters
 */
void timer_reset(TimerEvent_t *obj);

/*!
 * \brief Set timer new timeout value
 *
 * \param [IN] obj   Structure containing the timer object parameters
 * \param [IN] value New timer timeout value
 */
void timer_set_value(TimerEvent_t *obj, uint32_t value);

/*!
 * \brief Read the current time
 *
 * \retval time returns current time
 */
TimerTime_t timer_get_current_time(void);

/*!
 * \brief Return the Time elapsed since a fix moment in Time
 *
 * \remark TimerGetElapsedTime will return 0 for argument 0.
 *
 * \param [IN] past         fix moment in Time
 * \retval time             returns elapsed time
 */
TimerTime_t timer_get_elapsed_time(TimerTime_t past);

#ifdef __cplusplus
}
#endif

#endif // __TIMER_H__
