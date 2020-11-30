/** @file my_gps.h
 *
 * @brief
 *
 *  Created on: 15 Sep 2020
 *      Author: njnel
 */

#ifndef MY_GPS_H_
#define MY_GPS_H_

#define GPS_PPS_PORT GPIO_PORT_P3
#define GPS_PPS_PIN GPIO_PIN0

#include <stdint.h>

typedef struct {
		float lat;
		float lon;
} LocationData;

#endif /* MY_GPS_H_ */
