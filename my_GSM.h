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
#define SIZE_BUFFER 500
#define SIZE_COMMAND    10   // Sets the Max command Length including '\0'
#define TOTAL_STRINGS    7   // Total number of Searchable strings
#define OK       0          // 1 <-- Required
#define CMTI    1          // 2 <-- Required
#define ERROR   2          // 3 <-- Required

#define TEST_API_KEY "8DXP0M9I0CD8Y8PK"
#define ACCESS_TOKEN "TXtBD0VH60mdBvvtMJHk"

// GSM Module Common Messages
#define AT "AT\n"
#define CONTEXT_ACTIVATION "AT#SGACT=1,1\r\n"
#define CONTEXT_DEACTIVATION "AT#SGACT=1,0\r\n"
#define HTTP_CONFIG_THINGSPEAK "AT#HTTPCFG=1,\"api.thingspeak.com\",80,0,,,0,120,1\r\n"
#define HTTP_CONFIG_THINGSBOARD "AT#HTTPCFG=1,\"meesters.ddns.net\",8008,0,,,0,120,1\r\n"
#define DEFINE_PDP_CONTEXT "AT+CGDCONT=1,\"IP\",\"internet\",\"0.0.0.0\",0,0\r\n"
#define RESPONSE_OK "OK"
#define DISABLE_FLOW_CONTROL "AT&K=0\r\n"

/*String and other buffers*/

//Strings[][];		//Store the possible returns from the GSM modem
//char Receive_String[SIZE_BUFFER];                        // Serial Buffer
/*Functions*/
/*Send a single character to the UART*/
//void send_UART_hex(char bufferHex);
/*Send a char array through the specified UART channel*/
//void send_UART(char *buffer);
/*Initialises the GSM module, and checks if one is present */
bool init_gsm(void);
/*Sets up the pins and interrupts for GSM modem*/
void gsm_startup(void);
/* Function which sends strings on UART to the GSM modem*/
void send_gsm_uart(char *buffer);
/*Interrupt handler for UARTA0*/
void EUSCIA0_IRQHandler(void);
/* EUSCI A1 UART ISR - Receives from the GSM modem */
void EUSCIA2_IRQHandler(void);
/*delay the CPU for a set time or a bit returned by gsm*/
int delay_gsm_respond(int Delay_ctr);
/*Set the modem to POWER SAVE mode*/
void gsm_power_save_off();
/*Set the modem to NORMAL mode*/
void gsm_power_save_on();
/*check if the modem is connected*/
int check_com();
/*Waits for the modem to reply with a valid signal strength*/
int modem_start(void);
/*Search through the received buffer for a certain string*/
int string_search(int index);
/*Loads a command from the strings buffer into a temp buffer for checking*/
void cmd_load(int index);
/*Sends a message to the gsm modem from buffer*/
void send_msg(char *buffer);
/*Sets up the Modem for a ping*/
void gsm_setup_modem_ping(void);
/*Pings the google website*/
void gsm_ping_google(void);
/*Checks network registration*/
void check_registration(void);
/*Checks to see if the GPRS stack is attached*/
void check_gprs_attached(void);
/*Checks that GPRS is set on modem, if not then sets it up*/
void setup_gprs_settings(void);
/*check for a certain string in return*/
int check_for_string_position(char *checkFor);
/*Waits for a certain time (in seconds) and checks if a character string
 * has been returned by the GSM modem during that time. If timeout occurs it returns false
 */
bool wait_check_for_reply(char *reply, uint8_t delay_s);

void check_signal();
/*Returns the signal strength from the GSM modem*/
int get_signal_strength(int index);
/*Connects to an HTTPS server and attempts to get info from it*/
void http_connect(void);

void http_send_data(void);
/*Disables the command echo from the GSM*/
void disable_command_echo(void);

//!
//! Uploads the data collected by this node only.
bool gsm_upload_my_data();

//!
//! Uploads all stored datagrams to the database
void gsm_upload_stored_datagrams(void);

//!
//! @brief Uploads the datagram it just received
void upload_current_datagram(void);

//!
//! @brief Resets the module
void reset_gsm_module(void);

#endif /* MY_GSMMODEM_H_ */
