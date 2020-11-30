/*
 * gsmModem.c
 *
 *  Created on: Aug 10, 2016
 *      Author: Jonathan
 */

#include <my_GSM.h>
#include <my_timer.h>
#include <stdint.h>
#include <stdio.h>
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

const eUSCI_UART_ConfigV1 uartConfigGSM = { EUSCI_A_UART_CLOCKSOURCE_SMCLK, // SMCLK Clock Source
		78,                                     // BRDIV = 78
		2,                                       // UCxBRF = 2
		0,                                       // UCxBRS = 0
		EUSCI_A_UART_NO_PARITY,                  // No Parity
		EUSCI_A_UART_LSB_FIRST,                  // LSB First
		EUSCI_A_UART_ONE_STOP_BIT,               // One stop bit
		EUSCI_A_UART_MODE,                       // UART mode
		EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION  // Oversampling
		};

/*Sets up the pins and interrupts for GSM modem*/
void GSM_startup( void ) {
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

/* Function which sends strings on UART to the GSM modem*/
void send_gsmUART( char *buffer ) {
	int count = 0;
	while (strlen(buffer) > count) {
		//MAP_UART_transmitData(EUSCI_A0_BASE,buffer[count]); //Echo back to the PC for now
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
	if (counter_read_gsm == 80) {
		counter_read_gsm = 0;
	}
}

/* EUSCI A0 UART ISR - Echoes data back to PC host */
/*void EUSCIA0_IRQHandler(void)
 {

 uint32_t status = MAP_UART_getEnabledInterruptStatus(EUSCI_A0_BASE);

 MAP_UART_clearInterruptFlag(EUSCI_A0_BASE, status);

 if(status & EUSCI_A_UART_RECEIVE_INTERRUPT)
 {

 UartRX[counter_read_gsm] =  MAP_UART_receiveData(EUSCI_A0_BASE);
 MAP_UART_transmitData(EUSCI_A0_BASE,UartRX[counter_read_gsm]);
 counter_read_gsm++;
 }
 //if(counter_read_gsm == 80)
 // 	counter_read_gsm=0;
 if(counter_read_gsm==80)
 {counter_read_gsm=0;}

 }*/
//                      CHECKS COMMS WITH MODEM
int CHECK_COM( ) {
	counter_read_gsm = 0;                     // Reset buffer counter
	send_gsmUART("AT\r");                     // Send Attention Command
	delay_gsm_respond(50);                       // Delay a maximum of X seconds
	counter_read_gsm = 0;                     // Reset buffer counter
	return (STRING_SEARCH(OK));            // Check for OK response
}
//                      CHECKS COMMS WITH UART
int CHECK_COM_UART( ) {
	counter_read_gsm = 0;                     // Reset buffer counter
//	send_UART("AT\r");                     // Send Attention Command
	delay_gsm_respond(50);                       // Delay a maximum of X seconds
	counter_read_gsm = 0;                     // Reset buffer counter
	return (STRING_SEARCH(OK));            // Check for OK response
}
/*Sends a msg to the GSM*/
void sendmsg( char *buffer ) {
//	send_UART(buffer);
	counter_read_gsm = 0;                     // Reset buffer counter
	send_gsmUART(buffer);                     // Send Attention Command
	//delay_gsm_respond(5000);                        // Delay a maximum of X seconds
	counter_read_gsm = 0;                     // Reset buffer counter
}
/*Command to check the signal strength which the modem is receiving*/
void checkSignal( ) {
	counter_read_gsm = 0;                     // Reset buffer counter
	send_gsmUART("AT+CSQ\r");                     // Send Attention Command
	delay_gsm_respond(5000);                     // Delay a maximum of X seconds
	counter_read_gsm = 0;                     // Reset buffer counter
}
/*Check for the position of a certain string in the receive buffer*/
int checkForStringposition( char *checkFor ) {
	char *s;
	s = strstr(UartGSMRX, checkFor);

	//send_UART("this = "+(s-UartGSMRX));
	if (s != NULL) {
		return (s - UartGSMRX);
	}
	return (0);
}
/*Pulls the signal strength from the RX buffer at a certain index*/
int getSignalStrength( int index ) {
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

	for (ii = 0; ii < 5; ii++) {
		CHECK_COM();
		Delayms(50);
	}
	int checked = CHECK_COM();
	if (checked) {
//		send_UART("modem alive\r");
		//set_AmIServer(true); //Sets the IAMModem parameter to true
	}
	else {
//		send_UART("modem DEAD\r");
		//set_AmIServer(false); //Sets the IAMModem parameter to false
		return 0;
	}
	disablecommandEcho();
	int strength;
	int index;
	do {
		checkSignal();
		index = checkForStringposition("+CSQ:");
		strength = getSignalStrength(index);
		Delayms(500);
	} while ((strength > 31) || (strength <= 1)); //Check that the signal is in the correct range
//	send_UART(UartGSMRX[index + 6] + UartGSMRX[index + 7]);
	return 1;
}
//						Set the GSM modem to POWER SAVING mode
void gsmPowerSaveON( ) {
	counter_read_gsm = 0;                     // Reset buffer counter
	send_gsmUART("AT+CFUN=7\r");                     // Send Attention Command
	delay_gsm_respond(5000);                     // Delay a maximum of X seconds
	counter_read_gsm = 0;                     // Reset buffer counter
}
//						Set the GSM modem to NORMAL mode
void gsmPowerSaveOFF( ) {
	counter_read_gsm = 0;                     // Reset buffer counter
	send_gsmUART("AT+CFUN=1\r");                     // Send Attention Command
	delay_gsm_respond(5000);                     // Delay a maximum of X seconds
	counter_read_gsm = 0;                     // Reset buffer counter
}

void disablecommandEcho( ) {
	sendmsg("ATE0\r");
}

//                         LIMITED DELAY
int delay_gsm_respond( int Delay_ctr ) {
	counter_read_gsm = 0;                     // Reset buffer counter
//	runsystickFunction_ms(Delay_ctr);
	while ((counter_read_gsm == 0) /*&& (!SystimerReadyCheck)*/) // stay here until modem responds (X Seconds is arbitrary)
	{
		Delayms(2);
	}
	Delayms(20);
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
		Delayms(2);
		// SysCtlDelay(20000);
		Delay_ctr--;
	}
	if ((counter_read_gsm == 0) && (Delay_ctr == 0)) return (1);
	if ((counter_read_gsm == 0) && (Delay_ctr > 0)) {
		return (0);
	}
	return (0);
}

void gsm_setupModemPing( void ) {
	sendmsg("AT#SGACT=1,1\r");
	//delay_gsm_respond(5000);
}

void gsm_ping_google( void ) {
	//gsm_setupModemPing();
	sendmsg("AT#PING=\"www.sport24.co.za\"\r");
	//delay_gsm_respond(50);
	//counter_read_gsm=0;

}

void checkRegistration( void ) {
	sendmsg("AT+CREG?\r");
	Delayms(500);
	sendmsg("AT+COPS?\r");
	Delayms(500);
	sendmsg("AT+CPOL?\r");
	Delayms(500);
}

void checkGPRSattached( void ) {
	sendmsg("AT+CGATT?\r");
	Delayms(500);
	sendmsg("AT+CGREG?\r");
	Delayms(500);
	sendmsg("AT+CGREG=1?\r");
	Delayms(500);
	sendmsg("AT&K=0\r");
	Delayms(500);
	sendmsg("AT#SCFG?\r");
	Delayms(500);
	sendmsg("AT+CGDCONT=1,\"IP\",\"myMTN\"\r");
	Delayms(500);
	sendmsg("AT+CREG=1\r");
	Delayms(500);
	setup_GPRSSettings();
	Delayms(500);
	//sendmsg("AT#GPRS?\r");
	//sendmsg("AT#GPRS=1\r");
	//sendmsg("AT+CIPSHUT\r");
	//sendmsg("AT+CIPMUX=0\r");
}

bool wait_Check_ForReply( char *reply, uint8_t delay_s ) {
//	runSystickFunction_second(delay_s);
	do {
		uint8_t index = checkForStringposition(reply);
		if (index != 0) {
//			stopSystick();
			return true;
		}
		Delayms(50);
//	} while (!SystimerReadyCheck());
	} while (true);

	return false;
}

void setup_GPRSSettings( void ) {
	sendmsg("AT#GPRS?\r");
	Delayms(50);
	if (wait_Check_ForReply("#GPRS: 0", 1)) sendmsg("AT#GPRS=1\r");
	__no_operation();
}
void HTTP_connect( void ) {
	//sendmsg("AT#SKTD=0,80,\"www.telit.net\"\r");
	//sendmsg("AT#SD=1,0,80,\"www.m2msupport.net\"\r");//working
	sendmsg("AT#SD=1,0,80,\"www.dweet.io\"\r"); //working2
	//sendmsg("AT#SKTD=0,80,\"www.theage.com.au\"\r");
	//sendmsg("AT#SD=1,0,80,\"www.openweathermap.org\"\r");
	Delayms(50);
	if (wait_Check_ForReply("CONNECT", 5)) {
//		send_UART("Connected");
		//send_gsmUART("GET /data/2.5/weather?q=London,uk HTTP/1.1\r\n");
		send_gsmUART("POST /dweet/for/GSMmodem?temp=120 HTTP/1.1\r\n"); //working2
		//send_gsmUART("POST /dweet/for/GSMmodem HTTP/1.1\r\n");
		//send_gsmUART("GET /m2msupport/http_get_test.php HTTP/1.1\r\n");//working
		Delayms(5);
		//SysCtlDelay(2);
		//send_gsmUART("Host: www.openweathermap.org\r\n");
		send_gsmUART("Host: dweet.io\r\n"); //working2
		//send_gsmUART("Host:www.m2msupport.net\r\n");//working
		Delayms(5);

		//interrupt_delayms(5);
		//SysCtlDelay(2);
		send_gsmUART("Connection:close\r\n");		//working2
		//send_gsmUART("Connection:keep-alive\r\n");//working
		//interrupt_delayms(5);
		send_gsmUART("Accept: */*\r\n"); //working2
		//send_gsmUART("Accept: application/json\r\n");
		//send_gsmUART("{\"id\":1,\"name\":\"Node1\",\"value\":1100}\r\n");
		Delayms(5);
		//SysCtlDelay(2);
		send_gsmUART("\r\n\r");		//working
		sendmsg("\x1A\r");		//working

	}
	wait_Check_ForReply("NO CARRIER", 15);
	//sendmsg("GET /m2msupport/http_get_test.php HTTP/1.1");

}

void CMD_LOAD( int index ) {
	uint16_t var = 0;                                    // temp index for array
	memset(Command, NULL, SIZE_COMMAND);       // Reset data array index to NULL
	while ((Strings[index][var] != NULL) && (var < SIZE_COMMAND)) // Copy data from main "Strings" to commparing array.
	{
		Command[var] = Strings[index][var]; // Copy into temp array the strings from Main Database
		var++;                           // Up index
	}
}
int STRING_SEARCH( int index )         // index' is Strings[index][SIZE_COMMAND]
		{                                    // See defines in .h
	CMD_LOAD(index);             // Loads into temp array the string to be found
	if (strstr(UartGSMRX, Command) != NULL) // Find String or Command in main Buffer
		return (1);                        // Return 1 if found
	else return (0);                        // Return 0 if not found.
}
//               LOADS TO TEMP ARRAY THE SEARCHABLE STRING

