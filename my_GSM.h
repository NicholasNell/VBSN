/** @file my_GSM.h
 *
 * @brief
 *
 *  Created on: 30 Nov 2020
 *      Author: njnel
 */

#ifndef MY_GSMMODEM_H_
#define MY_GSMMODEM_H_

#include <stdbool.h>
#include <stdint.h>
/* DriverLib Includes */
//#include "driverlib.h"
#define SIZE_BUFFER 80
#define SIZE_COMMAND    10   // Sets the Max command Length including '\0'
#define TOTAL_STRINGS    7   // Total number of Searchable strings
#define OK       0          // 1 <-- Required
#define CMTI    1          // 2 <-- Required
#define ERROR   2          // 3 <-- Required
/*String and other buffers*/

//Strings[][];		//Store the possible returns from the GSM modem
//char Receive_String[SIZE_BUFFER];                        // Serial Buffer
/*Functions*/
/*Send a single character to the UART*/
//void send_UART_hex(char bufferHex);
/*Send a char array through the specified UART channel*/
//void send_UART(char *buffer);
/*Sets up the pins and interrupts for GSM modem*/
void GSM_startup( void );
/* Function which sends strings on UART to the GSM modem*/
void send_gsmUART( char *buffer );
/*Interrupt handler for UARTA0*/
void EUSCIA0_IRQHandler( void );
/* EUSCI A1 UART ISR - Receives from the GSM modem */
void EUSCIA2_IRQHandler( void );
/*delay the CPU for a set time or a bit returned by gsm*/
int delay_gsm_respond( int Delay_ctr );
/*Set the modem to POWER SAVE mode*/
void gsmPowerSaveOFF( );
/*Set the modem to NORMAL mode*/
void gsmPowerSaveON( );
/*check if the modem is connected*/
int CHECK_COM( );
/*Waits for the modem to reply with a valid signal strength*/
int modem_start( void );
/*Search through the received buffer for a certain string*/
int STRING_SEARCH( int index );
/*Loads a command from the strings buffer into a temp buffer for checking*/
void CMD_LOAD( int index );
/*Sends a message to the gsm modem from buffer*/
void sendmsg( char *buffer );
/*Sets up the Modem for a ping*/
void gsm_setupModemPing( void );
/*Pings the google website*/
void gsm_ping_google( void );
/*Checks network registration*/
void checkRegistration( void );
/*Checks to see if the GPRS stack is attached*/
void checkGPRSattached( void );
/*Checks that GPRS is set on modem, if not then sets it up*/
void setup_GPRSSettings( void );
/*check for a certain string in return*/
int checkForStringposition( char *checkFor );
/*Waits for a certain time (in seconds) and checks if a character string
 * has been returned by the GSM modem during that time. If timeout occurs it returns false
 */
bool wait_Check_ForReply( char *reply, uint8_t delay_s );
/*Returns the signal strength from the GSM modem*/
int getSignalStrength( int index );
/*Connects to an HTTPS server and attempts to get info from it*/
void HTTP_connect( void );
/*Disables the command echo from the GSM*/
void disablecommandEcho( void );
#endif /* MY_GSMMODEM_H_ */
