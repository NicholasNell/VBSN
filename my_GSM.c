#include <bme280_defs.h>
#include <datagram.h>
#include <EC5.h>
#include <helper.h>
#include <my_GSM.h>
#include <my_systick.h>
#include <my_timer.h>
#include <MAX44009.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ti/devices/msp432p4xx/driverlib/gpio.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>
#include <ti/devices/msp432p4xx/driverlib/rom_map.h>
#include <ti/devices/msp432p4xx/driverlib/uart.h>
#include <ti/devices/msp432p4xx/inc/msp_compatibility.h>
#include <ti/devices/msp432p4xx/inc/msp432p401r.h>

int counter_read_gsm = 0; //UART receive buffer counter
char UartGSMRX[SIZE_BUFFER];
const char Strings[TOTAL_STRINGS][SIZE_COMMAND] = { "OK\0",           // index 0
		"+CMTI\0",            // index 1
		"ERROR\0" };            // index 2
char Command[SIZE_COMMAND];                              // Temp command buffer

/* Private Functions */

/*!
 * \brief Uninitialises the UART port for the gsm module.
 */
void GSM_disable( void );

//RXData = (char*)malloc(SIZE_BUFFER*sizeof(char));
/* Function which sends strings on UART to the PC*/
/*void send_UART_hex(char bufferHex)
 {
 MAP_UART_transmitData(EUSCI_A0_BASE,bufferHex);
 }
 void send_UART(char *buffer)
 {
 int count = 0;
 while(strlen(buffer)>count)
 {
 MAP_UART_transmitData(EUSCI_A0_BASE,buffer[count++]);
 }
 }*/

//const eUSCI_UART_ConfigV1 uartConfigGSM = { EUSCI_A_UART_CLOCKSOURCE_SMCLK, // SMCLK Clock Source
//		78,                                     // BRDIV = 78
//		2,                                       // UCxBRF = 2
//		0,                                       // UCxBRS = 0
//		EUSCI_A_UART_NO_PARITY,                  // No Parity
//		EUSCI_A_UART_LSB_FIRST,                  // LSB First
//		EUSCI_A_UART_ONE_STOP_BIT,               // One stop bit
//		EUSCI_A_UART_MODE,                       // UART mode
//		EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION  // Oversampling
//		};
const eUSCI_UART_ConfigV1 uartConfigGSM = { EUSCI_A_UART_CLOCKSOURCE_SMCLK, // SMCLK Clock Source
		1,                                     // BRDIV = 78
		10,                                       // UCxBRF = 2
		0,                                       // UCxBRS = 0
		EUSCI_A_UART_NO_PARITY,                  // No Parity
		EUSCI_A_UART_LSB_FIRST,                  // LSB First
		EUSCI_A_UART_ONE_STOP_BIT,               // One stop bit
		EUSCI_A_UART_MODE,             // UART mode
		EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION // Oversampling
		};
void GSM_disable( void ) {
	GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN2 | GPIO_PIN3);
	GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN2 | GPIO_PIN3);

	UART_disableModule(EUSCI_A2_BASE);

	UART_disableInterrupt(EUSCI_A2_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
	Interrupt_disableInterrupt(INT_EUSCIA2);
}

/*Sets up the pins and interrupts for GSM modem*/
void gsm_startup( void ) {
	/* Configure pins P3.2 and P3.3 in UART mode.
	 * Doesn't matter if they are set to inputs or outputs
	 */
	GPIO_setAsPeripheralModuleFunctionInputPin(
	GPIO_PORT_P3,
	GPIO_PIN2 | GPIO_PIN3,
	GPIO_PRIMARY_MODULE_FUNCTION);
	/* Configuring UART_A1 Module */
	UART_initModule(EUSCI_A2_BASE, &uartConfigGSM);  //GSM
	/* Enable UART_A1 module */
	UART_enableModule(EUSCI_A2_BASE); //GSM
	/* Enabling interrupts GSM UART*/
	MAP_UART_enableInterrupt(EUSCI_A2_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
	MAP_Interrupt_enableInterrupt(INT_EUSCIA2);

}

bool init_gsm( void ) {

	int retval;

	gsm_startup();

	retval = check_com();

	if (retval) {
//		modem_start();
//		send_msg("AT&F0\r\n");
//		wait_check_for_reply(RESPONSE_OK, 1);
		send_msg("ATZ0\r\n");
		wait_check_for_reply(RESPONSE_OK, 1);
		send_msg(DISABLE_FLOW_CONTROL);	// disable flow control
		wait_check_for_reply(RESPONSE_OK, 1);
		delay_ms(500);
		send_msg(DEFINE_PDP_CONTEXT);	// Define PDP context
		wait_check_for_reply(RESPONSE_OK, 1);
		delay_ms(500);
		send_msg(HTTP_CONFIG_THINGSBOARD);
		wait_check_for_reply(RESPONSE_OK, 1);

		return true;
	}
	else {
		GSM_disable();
		return false;
	}

}

/* Function which sends strings on UART to the GSM modem*/
void send_gsm_uart( char *buffer ) {
	int count = 0;
	while (strlen(buffer) > count) {
//		MAP_UART_transmitData(EUSCI_A0_BASE, buffer[count]); //Echo back to the PC for now
		MAP_UART_transmitData(EUSCI_A2_BASE, buffer[count++]);

	}
}

/* EUSCI A2 UART ISR - Receives from the GSM modem */
void EUSCIA2_IRQHandler( void ) {
	uint32_t status = MAP_UART_getEnabledInterruptStatus(EUSCI_A2_BASE);

	MAP_UART_clearInterruptFlag(EUSCI_A2_BASE, status);

	if (status & EUSCI_A_UART_RECEIVE_INTERRUPT) {

		UartGSMRX[counter_read_gsm] = MAP_UART_receiveData(EUSCI_A2_BASE);
		MAP_UART_transmitData(EUSCI_A0_BASE, UartGSMRX[counter_read_gsm]); //Echo back to the PC for now
		counter_read_gsm++;
	}
	//if(counter_read_gsm == 80)
	// 	counter_read_gsm=0;
	if (counter_read_gsm == SIZE_BUFFER) {
		counter_read_gsm = 0;
	}
}

//                      CHECKS COMMS WITH MODEM
int check_com( ) {
	counter_read_gsm = 0;                     // Reset buffer counter
	send_gsm_uart("AT\r");                     // Send Attention Command
//	delay_gsm_respond(50);                       // Delay a maximum of X seconds
	delay_ms(100);
//	wait_Check_ForReply("OK", 1);
	counter_read_gsm = 0;                     // Reset buffer counter
	return (string_search(OK));            // Check for OK response
}

/*Sends a msg to the GSM*/
void send_msg( char *buffer ) {
//	send_UART(buffer);
	counter_read_gsm = 0;                     // Reset buffer counter
	send_gsm_uart(buffer);                     // Send Attention Command
	counter_read_gsm = 0;                     // Reset buffer counter
}

/*Command to check the signal strength which the modem is receiving*/
void check_signal( ) {
	counter_read_gsm = 0;                     // Reset buffer counter
	send_gsm_uart("AT+CSQ\r");                     // Send Attention Command
	delay_gsm_respond(5000);                     // Delay a maximum of X seconds
	counter_read_gsm = 0;                     // Reset buffer counter
}

/*Check for the position of a certain string in the receive buffer*/
int check_for_string_position( char *checkFor ) {
	char *s;
	s = strstr(UartGSMRX, checkFor);

	//send_UART("this = "+(s-UartGSMRX));
	if (s != NULL) {
		return (s - UartGSMRX);
	}
	return (0);
}

/*Pulls the signal strength from the RX buffer at a certain index*/
int get_signal_strength( int index ) {
	int i;
	char temp[2];
	for (i = 0; i < 2; i++) {
		temp[i] = UartGSMRX[index + 6 + i];
	}
	int tempNum;
	sscanf(temp, "%d", &tempNum);
	return tempNum;
}

/*Checks whether the modem is started by checking that it responds and also the signal strength*/
int modem_start( void ) {
	int ii;
	int checked = check_com();
	for (ii = 0; ii < 5; ii++) {
		check_com();
		delay_ms(10);
	}

	if (checked) {
//		send_UART("modem alive\r");
		//set_AmIServer(true); //Sets the IAMModem parameter to true
	}
	else {
//		send_UART("modem DEAD\r");
		//set_AmIServer(false); //Sets the IAMModem parameter to false
		return 0;
	}
	disable_command_echo();
	int strength;
	int index;
	do {
		check_signal();
		index = check_for_string_position("+CSQ:");
		strength = get_signal_strength(index);
		delay_ms(500);
	} while ((strength > 31) || (strength <= 1)); //Check that the signal is in the correct range
//	send_UART(UartGSMRX[index + 6] + UartGSMRX[index + 7]);
	return 1;
}

//						Set the GSM modem to POWER SAVING mode
void gsm_power_save_on( ) {
	counter_read_gsm = 0;                     // Reset buffer counter
	send_gsm_uart("AT+CFUN=5\r");              // Send Attention Command
//	delay_gsm_respond(5000);                  // Delay a maximum of X seconds
	delay_ms(500);
	counter_read_gsm = 0;                     // Reset buffer counter
}
//						Set the GSM modem to NORMAL mode
void gsm_power_save_off( ) {
	counter_read_gsm = 0;                     // Reset buffer counter
	send_gsm_uart("AT+CFUN=1\r");                     // Send Attention Command
//	delay_gsm_respond(5000);                     // Delay a maximum of X seconds
	delay_ms(500);
	counter_read_gsm = 0;                     // Reset buffer counter
}

void disable_command_echo( ) {
	send_msg("ATE0\r");
}

//                         LIMITED DELAY
int delay_gsm_respond( int Delay_ctr ) {
	counter_read_gsm = 0;                     // Reset buffer counter
	run_systick_function_ms(Delay_ctr);
	while ((counter_read_gsm == 0) && (!systimer_ready_check())) // stay here until modem responds (X Seconds is arbitrary)
	{
		delay_ms(2);
	}
	delay_ms(20);
	if ((counter_read_gsm == 0) && (Delay_ctr == 0)) return (1);
	if ((counter_read_gsm == 0) && (Delay_ctr > 0)) {
		return (0);
	}
	return (0);
}
int delay_UART_respond( int Delay_ctr ) {
	counter_read_gsm = 0;                     // Reset buffer counter
	while ((counter_read_gsm == 0) && (Delay_ctr > 0)) // stay here until modem responds (X Seconds is arbitrary)
	{
		delay_ms(2);
		// SysCtlDelay(20000);
		Delay_ctr--;
	}
	if ((counter_read_gsm == 0) && (Delay_ctr == 0)) return (1);
	if ((counter_read_gsm == 0) && (Delay_ctr > 0)) {
		return (0);
	}
	return (0);
}

void gsm_setup_modem_ping( void ) {
	send_msg("AT#SGACT=1,1\r");
	//delay_gsm_respond(5000);
}

void gsm_ping_google( void ) {
	//gsm_setupModemPing();
	send_msg("AT#PING=\"www.google.co.za\"\r");
	//delay_gsm_respond(50);
	//counter_read_gsm=0;

}

void check_registration( void ) {
	send_msg("AT+CREG?\r");
	delay_ms(500);
	send_msg("AT+COPS?\r");
	delay_ms(500);
	send_msg("AT+CPOL?\r");
	delay_ms(500);
}

void check_gprs_attached( void ) {
	send_msg("AT+CGATT?\r\n");
	delay_ms(500);
	send_msg("AT+CGREG?\r\n");
	delay_ms(500);
	send_msg("AT+CGREG=1?\r\n");
	delay_ms(500);
	send_msg("AT&K=0\r\n");
	delay_ms(500);
	send_msg("AT#SCFG?\r\n");
	delay_ms(500);
	send_msg("AT+CGDCONT=1,\"IP\",\"VodaCom-SA\"\r\n");
	delay_ms(500);
	send_msg("AT+CREG=1\r\n");
	delay_ms(500);
	send_msg("AT#GPRS=1\r\n");
	delay_ms(500);
	//sendmsg("AT#GPRS?\r");
	//sendmsg("AT#GPRS=1\r");
	//sendmsg("AT+CIPSHUT\r");
	//sendmsg("AT+CIPMUX=0\r");
}

bool wait_check_for_reply( char *reply, uint8_t delay_s ) {
	run_systick_function_second(delay_s);
	do {
		uint8_t index = check_for_string_position(reply);
		if (index != 0) {
			stop_systick();
			return true;
		}
		delay_ms(10);
	} while (!systimer_ready_check());
	return false;
}

void setup_gprs_settings( void ) {
	send_msg("AT#GPRS?\r\n");
	delay_ms(100);
	if (wait_check_for_reply("#GPRS: 0", 1)) send_msg("AT#GPRS=1\r\n");
	__no_operation();
}
void http_connect( void ) {
//	sendmsg("AT#SKTD=0,80,\"www.telit.net\"\r");
	send_msg("AT#SD=1,0,80,\"www.m2msupport.net\"\r");	//working
//	sendmsg("AT#SD=1,0,80,\"api.thingspeak.com\"\r\n"); //working2
			//sendmsg("AT#SKTD=0,80,\"www.theage.com.au\"\r");
//	sendmsg("AT#SD=1,0,80,\"www.openweathermap.org\"\r\n");
	delay_ms(50);
	if (wait_check_for_reply("CONNECT", 5)) {
//		send_UART("Connected");
		//send_gsmUART("GET /data/2.5/weather?q=London,uk HTTP/1.1\r\n");
		send_gsm_uart("POST /dweet/for/GSMmodem?temp=120 HTTP/1.1\r\n"); //working2
		//send_gsmUART("POST /dweet/for/GSMmodem HTTP/1.1\r\n");
		//send_gsmUART("GET /m2msupport/http_get_test.php HTTP/1.1\r\n");//working
		delay_ms(5);
		//SysCtlDelay(2);
		//send_gsmUART("Host: www.openweathermap.org\r\n");
		send_gsm_uart("Host: dweet.io\r\n"); //working2
		//send_gsmUART("Host:www.m2msupport.net\r\n");//working
		delay_ms(5);

		//interrupt_delayms(5);
		//SysCtlDelay(2);
		send_gsm_uart("Connection:close\r\n");		//working2
		//send_gsmUART("Connection:keep-alive\r\n");//working
		//interrupt_delayms(5);
		send_gsm_uart("Accept: */*\r\n"); //working2
		//send_gsmUART("Accept: application/json\r\n");
		//send_gsmUART("{\"id\":1,\"name\":\"Node1\",\"value\":1100}\r\n");
		delay_ms(5);
		//SysCtlDelay(2);
		send_gsm_uart("\r\n\r");		//working
		send_msg("\x1A\r");		//working

	}
	wait_check_for_reply("NO CARRIER", 15);
	//sendmsg("GET /m2msupport/http_get_test.php HTTP/1.1");

}

void cmd_load( int index ) {
	uint16_t var = 0;                                    // temp index for array
	memset(Command, NULL, SIZE_COMMAND);       // Reset data array index to NULL
	while ((Strings[index][var] != NULL) && (var < SIZE_COMMAND)) // Copy data from main "Strings" to commparing array.
	{
		Command[var] = Strings[index][var]; // Copy into temp array the strings from Main Database
		var++;                           // Up index
	}
}
int string_search( int index )         // index' is Strings[index][SIZE_COMMAND]
{                                    // See defines in .h
	cmd_load(index);             // Loads into temp array the string to be found
	if (strstr(UartGSMRX, Command) != NULL) // Find String or Command in main Buffer
		return (1);                        // Return 1 if found
	else return (0);                        // Return 0 if not found.
}
//               LOADS TO TEMP ARRAY THE SEARCHABLE STRING

/*
 * Sending HTTP requests (GET / HEAD / DELETE)
 With the command
 AT#HTTPQRY=<prof_id>,<command>,<resources>,<extra_header_lines> is
 possible to send a request to HTTP server.
 Parameters:
 <prof_id> - Numeric parameter indicating the profile identifier. Range: 0-2
 <command> - Numeric parameter indicating the command requested to HTTP server:
 0 – GET
 1 – HEAD
 2 – DELETE
 <resources> - String parameter indicating the HTTP resource (uri), object of the
 request
 <extra_header_lines> - String parameter indicating optional HTTP header line.
 If sending ends successfully, the response is OK; otherwise an error code is
 reported.
 Note: the HTTP request header sent with #HTTPQRY always contains the
 “Connection:close” line, and it cannot be removed. When the HTTP server answer is
 received, the following URC is printed on the serial port:
 #HTTPRING: <prof_id>,<http_status_code>,<content_type>,<data_size>
 Where:
 <prof_id> - is defined above
 <http_status_code> - is the numeric status code, as received from the server
 (see RFC 2616)
 <content_type> - is a string reporting the “Content-Type” header line, as received from
 the server (see RFC 2616)
 <data_size> - is the byte amount of data received from the server. If the server doesn’t
 report the "Content-Length:" header line, the parameter is 0.
 *
 *
 *
 * Sending HTTP POST or PUT
 With the command
 AT#HTTPSND=<prof_id>,<command>,<resource>,<data_len>,<post_param>,
 <extra_header_line> is possible to send data to HTTP server with POST or PUT
 commands.
 Parameters:
 <prof_id> - Numeric parameter indicating the profile identifier. Range: 0-2
 <command> - Numeric parameter indicating the command requested to HTTP server:
 0 – POST
 1 – PUT
 <resource> - String parameter indicating the HTTP resource (uri), object of the request
 <data_len> - Numeric parameter indicating the data length to input in bytes
 <post_param> - Numeric/string parameter indicating the HTTP Contenttype identifier,
 used
 only for POST command, optionally followed by colon character (:) and a string that
 extends
 with sub-types the identifier:
 “0[:extension]” – “application/x-www-form-urlencoded” with optional extension
 “1[:extension]” – “text/plain” with optional extension
 “2[:extension]” – “application/octet-stream” with optional extension
 “3[:extension]” – “multipart/form-data” with optional extension other content –
 free string corresponding to other content type and possible sub-types
 <extra_header_line> - String parameter indicating optional HTTP header line.
 */
extern struct bme280_dev bme280Dev;
extern struct bme280_data bme280Data;
void http_send_data( void ) {

	helper_collect_sensor_data();
	char buf[4];
	double temp = bme280Data.temperature;
	sprintf(buf, "%.1f", temp);
	char buf1[5];
	double temp1 = get_lux();
	sprintf(buf1, "%.1f", temp1);

//	gsmPowerSaveOFF();
	send_msg("AT#SGACT=1,1\r\n");	// Context Activation
//	wait_Check_ForReply("#SGACT:", 2);
	delay_ms(2000);
	send_msg("AT#HTTPCFG=0,\"api.thingspeak.com\",80,0,,,0,120,1\r\n");	// HTTP Config

//	wait_Check_ForReply("OK", 2);
	delay_ms(1000);
	send_msg("AT#HTTPQRY=0,0,\"https://api.thingspeak.com/update?api_key=");
	send_msg(TEST_API_KEY);
	send_msg("&field1=");
	send_msg(buf);
	send_msg("&field2=");
	sprintf(buf, "%.1f", bme280Data.humidity);
	send_msg(buf);
	send_msg("&field3=");
	send_msg(buf1);
	send_msg("\"\r\n");
//	sendmsg(
//			"AT#HTTPSND=0,0,\"https://api.thingspeak.com/update?api_key=8DXP0M9I0CD8Y8PK"); // send data POST request;
//
////	sendmsg(buf);
//	sendmsg("&field2=1");
//	sendmsg("\",1,1,0\r\n");
//	Delayms(5000);
//	sendmsg("0\r\n");
//	wait_Check_ForReply("OK", 5);
	delay_ms(5000);
	send_msg("AT#SGACT=1,0\r\n");	// Context Deactivation
//	wait_Check_ForReply("OK", 2);
	delay_ms(1000);
//	gsmPowerSaveON();
}

extern nodeAddress _nodeID;
extern LocationData gpsData;
static float loc = 0;
void gsm_upload_my_data( ) {

	if (loc > 90) {
		loc = 0;
	}
	/*
	 * field1 = temp
	 * field2 = humidity
	 * field3 = pressure
	 * field4 = Light
	 * field5 = VWC
	 * Not adding time yet
	 */
	double localTemperature = bme280Data.temperature;
	double localHumidity = bme280Data.humidity;
	double localPressure = bme280Data.pressure;
	double localVWC = get_latest_value();
	double localLight = get_lux();
//	char *api_key = TEST_API_KEY;
	char postBody[255];
	memset(postBody, 0, 255);
//	int lenWritten =
//			sprintf(
//					postBody,
//					"api_key=%s&field1=%.1f&field2=%.1f&field3=%.1f&field4=%.1f&field5=%.1f\r\n",
//					api_key,
//					localTemperature,
//					localHumidity,
//					localPressure,
//					localLight,
//					localVWC);

	int lenWritten =
			sprintf(
					postBody,
					"{\"nodeID\": %d,\"Temperature\":%.1f,\"Humidity\":%.1f,\"Pressure\": %.0f,\"VWC\":%.1f,\"Light\":%.1f,\"Latitude\": %f,\"Longitude\":%f}\r\n",
					_nodeID,
					localTemperature,
					localHumidity,
					localPressure,
					localVWC,
					localLight,
					loc++,
					loc);
//	int lenWritten = sprintf(
//			postBody,
//			"{\"nodeID\": %d,\"Temperature\":%.1f}\r\n",
//			_nodeID,
//			localTemperature);
	char postCommand[255];
	memset(postCommand, 0, 255);
	sprintf(
			postCommand,
			"AT#HTTPSND=1,0,\"http://meesters.ddns.net:8008/api/v1/%s/telemetry\",%d,\"application/json\"\r\n",
			ACCESS_TOKEN,
			lenWritten);

	send_msg(CONTEXT_ACTIVATION);
//	if (!wait_check_for_reply("OK", 1)) {
//
//	}
	delay_ms(1000);
	send_msg(postCommand);
//	if (!wait_check_for_reply(">>>", 5)) {
//
//	}
//	delay_ms(2000);
	delay_ms(5000);
	send_msg(postBody);
//	if (!wait_check_for_reply("OK", 5)) {
////		return;
//	}
//	delay_ms(2000);
	delay_ms(5000);
	send_msg(CONTEXT_DEACTIVATION);
//	if (!wait_check_for_reply("OK", 1)) {
////		return;
//	}

}
