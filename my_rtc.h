/*

 * my_rtc.h
 *
 *  Created on: 09 Mar 2020
 *      Author: nicholas
 */

#ifndef MY_RTC_H_
#define MY_RTC_H_
#include <ti/devices/msp432p4xx/driverlib/rtc_c.h>

void RtcInit(const RTC_C_Calendar currentTime);

#endif  /*MY_RTC_H_*/

