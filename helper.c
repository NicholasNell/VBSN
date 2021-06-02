/** @file helper.c
 *
 * @brief
 *
 *  Created on: 23 Sep 2020
 *      Author: njnel
 */

#include <bme280.h>
#include <bme280_defs.h>
#include <EC5.h>
#include <helper.h>
#include <my_i2c.h>
#include <MAX44009.h>
#include <ti/devices/msp432p4xx/driverlib/rtc_c.h>
#include <main.h>

int convert_hex_to_dec_by_byte(uint_fast8_t hex) {
	int tens = (hex & 0xF0) >> 4;
	int ones = (hex & 0x0F);

	return tens * 10 + ones;
}
