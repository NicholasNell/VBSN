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


/* Application Defines  */

/*!
 * \brief Timer object description
 */
typedef struct TimerEvent_s
{
    uint32_t Timestamp;                  //! Current timer value
    uint32_t ReloadValue;                //! Timer delay value
    bool IsStarted;                      //! Is the timer currently running
    uint_fast16_t compareValue;          //! Value to compare to
    uint32_t timer;                      //! Timer to use
    uint_fast16_t timerMode;             //! Timer Mode
    uint_fast16_t captureCompareRegister; //! CCR to use

/*
    bool IsNext2Expire;                  //! Is the next timer to expire
    void ( *Callback )( void* context ); //! Timer IRQ callback function
    void *Context;                       //! User defined data object pointer to pass back
    struct TimerEvent_s *Next;           //! Pointer to the next Timer object.
*/
}TimerEvent_t;

/*!
 * \brief Timer time variable definition
 */
#ifndef TimerTime_t
typedef uint32_t TimerTime_t;
#define TIMERTIME_T_MAX                             ( ( uint32_t )~0 )
#endif

/*!
 * \brief Initializes the timer object
 *
 * \remark TimerSetValue function must be called before starting the timer.
 *         this function initializes timestamp and reload value at 0.
 *
 * \param [IN] obj          Structure containing the timer object parameters
 * \param [IN] callback     Function callback called at the end of the timeout
 */
void TimerInit( TimerEvent_t *obj );


/*!
 * \brief Starts and adds the timer object to the list of timer events
 *
 * \param [IN] obj Structure containing the timer object parameters
 */
void TimerStart( TimerEvent_t *obj );

/*!
 * \brief Checks if the provided timer is running
 *
 * \param [IN] obj Structure containing the timer object parameters
 *
 * \retval status  returns the timer activity status [true: Started,
 *                                                    false: Stopped]
 */
bool TimerIsStarted( TimerEvent_t *obj );

/*!
 * \brief Stops and removes the timer object from the list of timer events
 *
 * \param [IN] obj Structure containing the timer object parameters
 */
void TimerStop( TimerEvent_t *obj );

/*!
 * \brief Resets the timer object
 *
 * \param [IN] obj Structure containing the timer object parameters
 */
void TimerReset( TimerEvent_t *obj );

/*!
 * \brief Set timer new timeout value
 *
 * \param [IN] obj   Structure containing the timer object parameters
 * \param [IN] value New timer timeout value
 */
void TimerSetValue( TimerEvent_t *obj, uint32_t value );

/*!
 * \brief Read the current time
 *
 * \retval time returns current time
 */
TimerTime_t TimerGetCurrentTime( TimerEvent_t *obj );

uint_fast16_t TimerGetElapsedTime( TimerEvent_t *obj );

/*!
 * \brief Computes the temperature compensation for a period of time on a
 *        specific temperature.
 *
 * \param [IN] period Time period to compensate
 * \param [IN] temperature Current temperature
 *
 * \retval Compensated time period
 */
TimerTime_t TimerTempCompensation( TimerTime_t period, float temperature );


/*
    Initialise timer_a to count up mode. Trigger interupt when TAR equals CCR0.
    Timer will be configured for ms interupts.
*/
void TimerAInteruptInit( void );

/*
    Timer A ISR Handler
*/
void TA1_0_IRQHandler( void );

/*
 * Delay function 1ms resolution
 */
void Delayms( uint32_t ms );

#endif /* MY_TIMER_H_ */
