/** @file my_gps.c
 *
 * @brief
 *
 *  Created on: 12 Mar 2021
 *      Author: njnel
 */

#include "my_gps.h"
#include <my_UART.h>
#include <ti/devices/msp432p4xx/driverlib/gpio.h>
extern LocationData gpsData;

void gps_set_low_power( void ) {
	GPIO_setOutputLowOnPin(GPS_WAKE_PORT, GPS_WAKE_PIN);
	send_uart_gps("$PMTK161,0*28\r\n");	// puts the gps in Standby mode
}

void gps_disable_low_power( void ) {
	send_uart_gps("H\r\n");	// send any byte to wakeup board
}

LocationData get_gps_data( void ) {
	return gpsData;
}
