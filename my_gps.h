/** @file my_gps.h
 *
 * @brief
 *
 *  Created on: 15 Sep 2020
 *      Author: njnel
 */

#ifndef MY_GPS_H_
#define MY_GPS_H_

#include <stdint.h>

typedef struct {
	float lat;
	uint8_t latHem :1;
	float lon;
	uint8_t lonHem :1;
} locationData;

#endif /* MY_GPS_H_ */
