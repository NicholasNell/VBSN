/** @file helper.c
 *
 * @brief
 *
 *  Created on: 23 Sep 2020
 *      Author: njnel
 */

#include <bme280.h>
#include <bme280_defs.h>
#include <helper.h>
#include <MAX44009.h>
#include <ti/devices/msp432p4xx/driverlib/rtc_c.h>

extern RTC_C_Calendar timeStamp;

extern struct bme280_dev bme280Dev;
extern struct bme280_data bme280Data;
float soilMoisture = 100;

void helper_collect_sensor_data( ) {
	get_light();
	bme280_get_data(&bme280Dev, &bme280Data);
	soilMoisture--;
	timeStamp = RTC_C_getCalendarTime();
}
