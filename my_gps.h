/** @file my_gps.h
 *
 * @brief
 *
 *  Created on: 15 Sep 2020
 *      Author: njnel
 */

#ifndef MY_GPS_H_
#define MY_GPS_H_

#define GPS_PPS_PORT 	GPIO_PORT_P3
#define GPS_PPS_PIN 	GPIO_PIN0
#define UART_GPS_PORT 	GPIO_PORT_P9
#define UART_GPS_PINS 	GPIO_PIN6 | GPIO_PIN7
#define UART_GPS_MODULE EUSCI_A3_BASE
#define UART_GPS_INT 	INT_EUSCIA3
#define GPS_PPS_INT_PORT INT_PORT3
#define GPS_WAKE_PORT 	GPIO_PORT_P6
#define GPS_WAKE_PIN	GPIO_PIN0

// NMEA Command Sentences
#define PMTK_FULL_POWER_MODE					"$PMTK225,0*2B\r\n"	////< Full power mode
#define PMTK_BACKUP_MODE	 					"$PMTK225,4*2F\r\n"	////< Lowest power mode, must be woken by pulling wake pin high
#define PMTK_STANDBY  							"$PMTK161,0*28\r\n" ///< standby command & boot successful message
#define PMTK_AWAKE 								"$PMTK010,002*2D\r\n"             ///< Wake up
#define PMTK_ALWAYS_LOCATE_9					"$PMTK225,9*22\r\n" ///always locate mode
#define PMTK_ALWAYS_LOCATE_8					"$PMTK225,8*23\r\n" ///always locate mode

#define PMTK_SET_NMEA_UPDATE_1HZ 				"$PMTK220,1000*1F\r\n" ///<  1 Hz
#define PMTK_SET_NMEA_UPDATE_200_MILLIHERTZ		"$PMTK220,5000*1B\r\n" ///< Once every 5 seconds, 200 millihertz.

#define PMTK_API_SET_FIX_CTL_200mHZ 			"$PMTK300,5000,0,0,0,0*18\r\n" ///< 1 Hz
#define PMTK_API_SET_FIX_CTL_1HZ 				"$PMTK300,1000,0,0,0,0*1C\r\n" ///< 1 Hz
#define PMTK_API_SET_FIX_CTL_2HZ 				"$PMTK300,500,0,0,0,0*1C\r\n" ///< 2 Hz

#define PMTK_SET_NMEA_OUTPUT_GLLONLY 			"$PMTK314,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n" ///< turn on only the GPGLL sentence
#define PMTK_SET_NMEA_OUTPUT_RMCONLY 			"$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n" ///< turn on only the GPRMC sentence
#define PMTK_SET_NMEA_OUTPUT_VTGONLY 			"$PMTK314,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n" ///< turn on only the GPVTG
#define PMTK_SET_NMEA_OUTPUT_GGAONLY 			"$PMTK314,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n" ///< turn on just the GPGGA
#define PMTK_SET_NMEA_OUTPUT_GSAONLY 			"$PMTK314,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n" ///< turn on just the GPGSA
#define PMTK_SET_NMEA_OUTPUT_GSVONLY 			"$PMTK314,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n" ///< turn on just the GPGSV
#define PMTK_SET_NMEA_OUTPUT_RMCGGA 			"$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n" ///< turn on GPRMC and GPGGA
#define PMTK_SET_NMEA_OUTPUT_RMCGGAGSA 			"$PMTK314,0,1,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n" ///< turn on GPRMC, GPGGA and GPGSA
#define PMTK_SET_NMEA_OUTPUT_ALLDATA 			"$PMTK314,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n" ///< turn on ALL THE DATA
#define PMTK_SET_NMEA_OUTPUT_OFF				"$PMTK314,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28\r\n" ///< turn off output

#define PMTK_SET_BAUD_115200 					"$PMTK251,115200*1F\r\n" ///< 115200 bps
#define PMTK_SET_BAUD_57600 					"$PMTK251,57600*2C\r\n"   ///<  57600 bps
#define PMTK_SET_BAUD_9600 						"$PMTK251,9600*17\r\n"     ///<   9600 bps

#define PMTK_LOCUS_STARTLOG 					"$PMTK185,0*22\r\n" ///< Start logging data
#define PMTK_LOCUS_STOPLOG 						"$PMTK185,1*23\r\n"  ///< Stop logging data
#define PMTK_LOCUS_STARTSTOPACK     			"$PMTK001,185,3*3C\r\n" ///< Acknowledge the start or stop command
#define PMTK_LOCUS_QUERY_STATUS 				"$PMTK183*38\r\n"  ///< Query the logging status
#define PMTK_LOCUS_ERASE_FLASH 					"$PMTK184,1*22\r\n" ///< Erase the log flash data

#include <stdint.h>

typedef struct {
	float lat;
	float lon;
} LocationData;

//!
//! Sets the Gps module in low power mode
void gps_set_low_power(void);

//!
//! disables the low power mode of the gps
void gps_disable_low_power(void);

//! @return returns current gps data
LocationData get_gps_data(void);

void set_gps_data(LocationData loc);
#endif /* MY_GPS_H_ */
