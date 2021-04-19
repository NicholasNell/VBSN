/*

 * my_rtc.h
 *
 *  Created on: 09 Mar 2020
 *      Author: nicholas
 */

#ifndef MY_RTC_H_
#define MY_RTC_H_
#include <ti/devices/msp432p4xx/driverlib/rtc_c.h>

#define RTC_ZERO_TIME {0x00,0x00,0x00,0x00,0x00,0x00,0x1970} // zero time

//!
//! \brief Initilises the RTC without starting the clock
void rtc_init( );

//!
//! \brief starts the RTC clock
void rtc_start_clock( void );

//!
//! \brief holds the RTC clock
void rtc_stop_clock( void );

//!
//! \brief holds the clock and then sets the new time, then starts the clock again
void rtc_set_calendar_time( void );

#endif  /*MY_RTC_H_*/

