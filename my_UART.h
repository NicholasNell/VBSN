/*
 * my_UART.h
 *
 *  Created on: 09 Sep 2020
 *      Author: njnel
 */

#ifndef MY_UART_H_
#define MY_UART_H_
#include <stdint.h>
#include <stdbool.h>

// NMEA Command Sentences
#define PMTK_FULL_POWER_MODE					"$PMTK225,0*2B\r\n"	////< Full power mode
#define PMTK_BACKUP_MODE	 					"$PMTK225,4*2F\r\n"	////< Lowest power mode, must be woken by pulling wake pin high
#define PMTK_STANDBY  							"$PMTK161,0*28\r\n" ///< standby command & boot successful message
#define PMTK_AWAKE 								"$PMTK010,002*2D\r\n"             ///< Wake up

#define PMTK_SET_NMEA_UPDATE_1HZ 				"$PMTK220,1000*1F\r\n" ///<  1 Hz
#define PMTK_SET_NMEA_UPDATE_200_MILLIHERTZ		"$PMTK220,5000*1B\r\n" ///< Once every 5 seconds, 200 millihertz.

#define PMTK_API_SET_FIX_CTL_1HZ 				"$PMTK300,1000,0,0,0,0*1C\r\n" ///< 1 Hz

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

void UARTinitGPS();
void UARTinitPC();
void sendUARTpc(char *buffer);
void sendUARTgps(char *buffer);
void UartGPSCommands();
void send_uart_integer(uint32_t integer);
void send_uart_integer_nextLine(uint32_t integer);
void resetUARTArray();
bool returnUartActivity();
void UartCommands();
void checkUartActivity();
#endif /* MY_UART_H_ */
