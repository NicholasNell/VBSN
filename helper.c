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

extern float lux;
extern struct bme280_dev bme280Dev;
extern struct bme280_data bme280Data;
float soilMoisture = 100;

void helper_collectSensorData() {

	getLight(&lux);
	bme280GetData(&bme280Dev, &bme280Data);
	soilMoisture--;

}
