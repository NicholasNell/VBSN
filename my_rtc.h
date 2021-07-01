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
void rtc_init();

//!
//! \brief starts the RTC clock
void rtc_start_clock(void);

//!
//! \brief holds the RTC clock
void rtc_stop_clock(void);

//!
//! \brief holds the clock and then sets the new time, then starts the clock again
void rtc_set_calendar_time(void);

/*!
 * \brief	inititial the scheduler, set tx slot and data collection slot
 */
void init_scheduler();

RTC_C_Calendar get_current_time();

void set_current_time(RTC_C_Calendar time);

bool get_mac_state_machine_enabled();
void reset_mac_state_machine_enabled();

bool get_upload_gsm_flag();
void reset_upload_gsm_flag();

bool get_collect_data_flag();
void reset_collect_data_flag();

bool get_save_flash_data_flag();
void reset_save_flash_data_flag();

bool get_reset_flag();
void reset_reset_flag();

bool get_rtc_init_flag();
void set_rtc_init_flag();
void reset_rtc_init_flag();

uint16_t get_time_to_route();
void set_time_to_route_flag();
void reset_time_to_route_flag();

void set_route_expired_flag();
void reset_route_expired_flag();
bool get_route_expired_flag();

#endif  /*MY_RTC_H_*/

