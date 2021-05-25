#include <bme280_defs.h>
#include <datagram.h>
#include <EC5.h>
#include <helper.h>
#include <my_gpio.h>
#include <my_GSM.h>
#include <my_MAC.h>
#include <my_systick.h>
#include <my_timer.h>
#include <my_UART.h>
#include <MAX44009.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <main.h>
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
void GSM_disable(void);

static void context_deactivate_activate(void);

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
void GSM_disable(void) {
	GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN2 | GPIO_PIN3);
	GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN2 | GPIO_PIN3);

	UART_disableModule(EUSCI_A2_BASE);

	UART_disableInterrupt(EUSCI_A2_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
	Interrupt_disableInterrupt(INT_EUSCIA2);
}

/*Sets up the pins and interrupts for GSM modem*/
void gsm_startup(void) {
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

//	GPIO_setAsInputPinWithPullDownResistor(GPIO_PORT_P7, GPIO_PIN1);

}

static void context_deactivate_activate(void) {
	send_uart_pc("Defining PDP Context.\n");
	send_msg(DEFINE_PDP_CONTEXT);	// Define PDP context
	wait_check_for_reply(RESPONSE_OK, 1);

//	delay_ms(500);
	send_uart_pc("HTTP CONFIG.\n");
	send_msg(HTTP_CONFIG_THINGSBOARD);
	wait_check_for_reply(RESPONSE_OK, 1);

	counter_read_gsm = 0;
//	delay_ms(500);
//	send_msg(GSM_CONTEXT_DEACTIVATION);
//	wait_check_for_reply(RESPONSE_OK, 1);

	send_msg(CONTEXT_DEACTIVATION);
	wait_check_for_reply(RESPONSE_OK, 1);

//	delay_ms(500);
	send_msg(CONTEXT_ACTIVATION);
	send_uart_pc("Activating Context.\n");
//	while (!wait_check_for_reply("#SGAC", 1)) {
//		WDT_A_clearTimer();
//		send_uart_pc("Retry Context Activation.\n");
//
////		send_msg(DEFINE_PDP_CONTEXT);	// Define PDP context
////		wait_check_for_reply(RESPONSE_OK, 1);
//		send_msg(CONTEXT_DEACTIVATION);
//
//		wait_check_for_reply(RESPONSE_OK, 1);
//
//		send_msg(CONTEXT_ACTIVATION);
//	}

	int tries = 0;
	while (!wait_check_for_reply("#SGAC", 6)) {
		WDT_A_clearTimer();
		tries++;
		send_uart_pc("Retry Context Activation.\n");

//		send_msg(CONTEXT_DEACTIVATION);
//
//		wait_check_for_reply(RESPONSE_OK, 1);

		send_msg(CONTEXT_ACTIVATION);
		if (tries > 10) {
			return;
		}

	}
	send_uart_pc("context activation successful\n");
	WDT_A_clearTimer();
}

bool init_gsm(void) {

	int retval = false;

	gsm_startup();

	gsm_power_save_off();
	delay_ms(100);

	run_systick_function_ms(3000);
	while (!retval) {
		WDT_A_clearTimer();
		retval = check_com();
		if (systimer_ready_check()) {
			break;
		}

		if (retval) {
			break;
		}
		delay_ms(100);
	}
	if (retval) {

		send_msg(DISABLE_FLOW_CONTROL);	// disable flow control
		wait_check_for_reply(RESPONSE_OK, 1);
		disable_command_echo();
		context_deactivate_activate();

		delay_ms(1000);
		WDT_A_clearTimer();
		gsm_power_save_on();

		return true;
	} else {
		GSM_disable();
		return false;
	}

}

/* Function which sends strings on UART to the GSM modem*/
void send_gsm_uart(char *buffer) {
	int count = 0;
	while (strlen(buffer) > count) {
//		MAP_UART_transmitData(EUSCI_A0_BASE, buffer[count]); //Echo back to the PC for now
		MAP_UART_transmitData(EUSCI_A2_BASE, buffer[count++]);

	}
}

/* EUSCI A2 UART ISR - Receives from the GSM modem */
void EUSCIA2_IRQHandler(void) {
	uint32_t status = MAP_UART_getEnabledInterruptStatus(EUSCI_A2_BASE);

	MAP_UART_clearInterruptFlag(EUSCI_A2_BASE, status);

	if (status & EUSCI_A_UART_RECEIVE_INTERRUPT) {

		UartGSMRX[counter_read_gsm] = MAP_UART_receiveData(EUSCI_A2_BASE);
#if DEBUG
		MAP_UART_transmitData(EUSCI_A0_BASE, UartGSMRX[counter_read_gsm]); //Echo back to the PC for now
#endif
		counter_read_gsm++;
	}
	//if(counter_read_gsm == 80)
	// 	counter_read_gsm=0;
	if (counter_read_gsm >= SIZE_BUFFER) {
		counter_read_gsm = 0;
	}
}

//                      CHECKS COMMS WITH MODEM
int check_com() {
	counter_read_gsm = 0;                     // Reset buffer counter
	send_gsm_uart("AT\r");                     // Send Attention Command
//	delay_gsm_respond(50);                       // Delay a maximum of X seconds
	delay_ms(100);
//	wait_Check_ForReply("OK", 1);
	counter_read_gsm = 0;                     // Reset buffer counter
	return (string_search(OK));            // Check for OK response
}

/*Sends a msg to the GSM*/
void send_msg(char *buffer) {
//	send_UART(buffer);
	counter_read_gsm = 0;                     // Reset buffer counter
	send_gsm_uart(buffer);                     // Send Attention Command
	counter_read_gsm = 0;                     // Reset buffer counter
}

/*Command to check the signal strength which the modem is receiving*/
void check_signal() {
	counter_read_gsm = 0;                     // Reset buffer counter
	send_gsm_uart("AT+CSQ\r");                     // Send Attention Command
	delay_gsm_respond(5000);                     // Delay a maximum of X seconds
	counter_read_gsm = 0;                     // Reset buffer counter
}

/*Check for the position of a certain string in the receive buffer*/
int check_for_string_position(char *checkFor) {
	char *s;
	s = strstr(UartGSMRX, checkFor);

	//send_UART("this = "+(s-UartGSMRX));
	if (s != NULL) {
		return (s - UartGSMRX);
	}
	return (0);
}

/*Pulls the signal strength from the RX buffer at a certain index*/
int get_signal_strength(int index) {
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
int modem_start(void) {
	int ii;
	int checked = check_com();
	for (ii = 0; ii < 5; ii++) {
		check_com();
		delay_ms(10);
	}

	if (checked) {
//		send_UART("modem alive\r");
		//set_AmIServer(true); //Sets the IAMModem parameter to true
	} else {
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
void gsm_power_save_on() {
//	send_gsm_uart("AT+CFUN=5\r");              // Send Attention Command

	send_uart_pc("gsm power save on.\n");
}
//						Set the GSM modem to NORMAL mode
void gsm_power_save_off() {
//	bool flag = false;
//	start_timer_a_counter(5000, &flag);
//	while (!flag) {
//
//		if (!GPIO_getInputPinValue(GPIO_PORT_P7, GPIO_PIN1)) {
//
//			send_gsm_uart("AT+CFUN=1\r");          // Send Attention Command
//
//			if (string_search(OK)) {
//				break;
//			}
//		}
//	}
//	stop_timer_a_counter();
//	WDT_A_clearTimer();
//	delay_ms(2000);
//	WDT_A_clearTimer();
//	delay_ms(2000);
////	WDT_A_clearTimer();
////	delay_ms(2000);
////	WDT_A_clearTimer();
////	delay_ms(2000);
////	WDT_A_clearTimer();
////	delay_ms(2000);
////	WDT_A_clearTimer();
//	send_uart_pc("gsm power save off.\n");
}

void disable_command_echo() {
	send_msg("ATE0\r");
}

//                         LIMITED DELAY
int delay_gsm_respond(int Delay_ctr) {
	counter_read_gsm = 0;                     // Reset buffer counter
	run_systick_function_ms(Delay_ctr);
	while ((counter_read_gsm == 0) && (!systimer_ready_check())) // stay here until modem responds (X Seconds is arbitrary)
	{
		delay_ms(2);
	}
	delay_ms(20);
	if ((counter_read_gsm == 0) && (Delay_ctr == 0))
		return (1);
	if ((counter_read_gsm == 0) && (Delay_ctr > 0)) {
		return (0);
	}
	return (0);
}
int delay_UART_respond(int Delay_ctr) {
	counter_read_gsm = 0;                     // Reset buffer counter
	while ((counter_read_gsm == 0) && (Delay_ctr > 0)) // stay here until modem responds (X Seconds is arbitrary)
	{
		delay_ms(2);
		// SysCtlDelay(20000);
		Delay_ctr--;
	}
	if ((counter_read_gsm == 0) && (Delay_ctr == 0))
		return (1);
	if ((counter_read_gsm == 0) && (Delay_ctr > 0)) {
		return (0);
	}
	return (0);
}

bool wait_check_for_reply(char *reply, uint8_t delay_s) {
	run_systick_function_second(delay_s);
	do {
		WDT_A_clearTimer();
		uint8_t index = check_for_string_position(reply);
		if (index != 0) {
			stop_systick();
			return true;
		}
		delay_ms(10);
	} while (!systimer_ready_check());
	return false;
}

void cmd_load(int index) {
	uint16_t var = 0;                                // temp index for array
	memset(Command, NULL, SIZE_COMMAND);   // Reset data array index to NULL
	while ((Strings[index][var] != NULL) && (var < SIZE_COMMAND)) // Copy data from main "Strings" to commparing array.
	{
		Command[var] = Strings[index][var]; // Copy into temp array the strings from Main Database
		var++;                           // Up index
	}
}
int string_search(int index)       // index' is Strings[index][SIZE_COMMAND]
		{                                    // See defines in .h
	cmd_load(index);         // Loads into temp array the string to be found
	if (strstr(UartGSMRX, Command) != NULL) // Find String or Command in main Buffer
		return (1);                        // Return 1 if found
	else
		return (0);                        // Return 0 if not found.
}

extern struct bme280_dev bme280Dev;
extern struct bme280_data bme280Data;
extern Gpio_t Led_rgb_red;

extern nodeAddress _nodeID;
extern LocationData gpsData;
extern RTC_C_Calendar currentTime;
extern bool hasData;

bool gsm_upload_my_data() {

//	gsm_power_save_off();
//	context_deactivate_activate();
	WDT_A_clearTimer();
	if (!hasData) {
		helper_collect_sensor_data();
		hasData = true;
	}
	WDT_A_clearTimer();
	double localTemperature = bme280Data.temperature;
	double localHumidity = bme280Data.humidity;
	double localPressure = bme280Data.pressure;
	double localVWC = get_latest_value();
	double localLight = get_lux();
	double localLat = gpsData.lat;
	double localLon = gpsData.lon;
	nodeAddress localAddress = _nodeID;
	char postBody[255];
	memset(postBody, 0, 255);
	WDT_A_clearTimer();
	int lenWritten =
			sprintf(postBody,
					"{\"nodeID\": %d,\"Temperature\":%.1f,\"Humidity\":%.1f,\"Pressure\": %.0f,\"VWC\":%.1f,\"Light\":%.1f,\"Latitude\": %f,\"Longitude\":%f,\"Time\":%d.%d.%d}\r\n",
					localAddress, localTemperature, localHumidity,
					localPressure, localVWC, localLight, localLat, localLon,
					convert_hex_to_dec_by_byte(currentTime.hours),
					convert_hex_to_dec_by_byte(currentTime.minutes),
					convert_hex_to_dec_by_byte(currentTime.seconds));

	char postCommand[255];
	memset(postCommand, 0, 255);
//	sprintf(postCommand,
//			"AT#HTTPSND=1,0,\"http://meesters.ddns.net:8008/api/v1/%s/telemetry\",%d,\"application/json\"\r\n",
//			ACCESS_TOKEN, lenWritten);
	sprintf(postCommand,
			"AT#HTTPSND=1,0,\"/api/v1/%s/telemetry\",%d,\"application/json\"\r\n",
			ACCESS_TOKEN, lenWritten);

	WDT_A_clearTimer();

	send_msg(postCommand);
	WDT_A_clearTimer();
	int retries = 0;
	while (!wait_check_for_reply(">>>", 5)) {
		check_com();
		send_uart_pc("Try PostCommand.\n");
		WDT_A_clearTimer();
		retries++;
		send_msg(postCommand);

		WDT_A_clearTimer();
		if (retries > 1) {

			send_uart_pc("Own Data failed POSTCOMMAND.\n");
			return false;
		}

	}

	retries = 0;

	WDT_A_clearTimer();
	send_msg(postBody);
	while (!wait_check_for_reply("#HTTP", 5)) {
		check_com();
		send_uart_pc("Try PostBody.\n");
		WDT_A_clearTimer();
		retries++;
		send_msg(postBody);

		WDT_A_clearTimer();
		if (retries > 1) {
			send_uart_pc("Own Data failed POSTBODY.\n");
			return false;
		}

	}
	retries = 0;

	WDT_A_clearTimer();
	return true;
}

extern bool isRoot;

void gsm_upload_stored_datagrams() {
	counter_read_gsm = 0;
	gsm_power_save_off();
//	context_deactivate_activate();
	int i = 0;

	int attempts = 0;

	if (!isRoot) {
		send_uart_pc("start gateway upload.\n");
		WDT_A_clearTimer();
		while (!gsm_upload_my_data()) {
			WDT_A_clearTimer();
			check_com();
//		gsm_power_save_on();
//		gsm_power_save_off();
			attempts++;
			send_uart_pc("Retrying gsm_upload_my_data\n");
			if (attempts > 1) {
				attempts = 0;
				send_uart_pc("failed upload my data.\n");
				WDT_A_clearTimer();
				return;
			}
		}
		WDT_A_clearTimer();
		send_uart_pc("gsm own upload succesful.\n");
		attempts = 0;
		delay_ms(2000);
	}
	WDT_A_clearTimer();
	check_com();
	WDT_A_clearTimer();
	uint8_t numToSend = get_received_messages_index();
	bool success = false;
	if (numToSend > 0) {
		WDT_A_clearTimer();
		send_uart_pc("Starting batch upload.\n");
		for (i = 0; i < numToSend; i++) {
			WDT_A_clearTimer();
			send_uart_pc("next datagram.\n");
			success = upload_current_datagram(i);
			while (!success) {
				check_com();
//				gsm_power_save_on();
//				gsm_power_save_off();
				WDT_A_clearTimer();
				success = upload_current_datagram(i);
				attempts++;
				send_uart_pc("Retrying gsm_upload_stored_data\n");
				if (attempts > 1) {
					attempts = 0;
					send_uart_pc("failed upload stored data.\n");
					return;
				}
			}

			// Do not delete! This time delay is crucial.
			WDT_A_clearTimer();
			delay_ms(2000);
			WDT_A_clearTimer();

		}
	}

	gsm_power_save_on();
	WDT_A_clearTimer();
}

bool upload_current_datagram(int index) {

//	gsm_power_save_off();
	WDT_A_clearTimer();
//	context_deactivate_activate();
	Datagram_t *pointerToData = get_received_messages();

	int lenWritten = 0;

	double localTemperature = pointerToData[index].data.sensData.temp;
	double localHumidity = pointerToData[index].data.sensData.hum;
	double localPressure = pointerToData[index].data.sensData.press;
	double localVWC = pointerToData[index].data.sensData.soilMoisture;
	double localLight = pointerToData[index].data.sensData.lux;
	double localLat = pointerToData[index].data.sensData.gpsData.lat;
	double localLon = pointerToData[index].data.sensData.gpsData.lon;
	nodeAddress localAddress = pointerToData[index].msgHeader.netSource;
	int localHr = convert_hex_to_dec_by_byte(
			pointerToData[index].data.sensData.tim.hours);
	int localMin = convert_hex_to_dec_by_byte(
			pointerToData[index].data.sensData.tim.minutes);
	int LocalSec = convert_hex_to_dec_by_byte(
			pointerToData[index].data.sensData.tim.seconds);
	float localSNR = pointerToData[index].radioData.snr;
	float localRSSI = pointerToData[index].radioData.rssi;
	int localRoutes = pointerToData[index].netData.numRoutes;
	int localnumDataSent = pointerToData[index].netData.numDataSent;
	int localnumNeighbours = pointerToData[index].netData.numNeighbours;
	int localRTSMissed = pointerToData[index].netData.rtsMissed;
	char postCommand[SIZE_BUFFER];
	memset(postCommand, 0, SIZE_BUFFER);
	char postBody[SIZE_BUFFER];
	memset(postBody, 0, SIZE_BUFFER);
	WDT_A_clearTimer();
	lenWritten =
			sprintf(postBody,
					"{\"ID\": %d,\"T\":%.1f,\"H\":%.1f,\"P\":%.0f,\"V\":%.1f,\"L\":%.1f,\"Lat\":%f,\"Lon\":%f,\"Tim\":%d.%d.%d,\"SNR\":%.1f,\"R\":%.1f,\"Ro\":%d,\"DS\":%d,\"NN\":%d,\"rts\":%d}\r\n",
					localAddress, localTemperature, localHumidity,
					localPressure, localVWC, localLight, localLat, localLon,
					localHr, localMin, LocalSec, localSNR, localRSSI,
					localRoutes, localnumDataSent, localnumNeighbours,
					localRTSMissed);

	//	sprintf(postCommand,
	//			"AT#HTTPSND=1,0,\"http://meesters.ddns.net:8008/api/v1/%s/telemetry\",%d,\"application/json\"\r\n",
	//			ACCESS_TOKEN, lenWritten);
	sprintf(postCommand,
			"AT#HTTPSND=1,0,\"/api/v1/%s/telemetry\",%d,\"application/json\"\r\n",
			ACCESS_TOKEN, lenWritten);

	WDT_A_clearTimer();
	send_msg(postCommand);
	WDT_A_clearTimer();
	int retries = 0;
	while (!wait_check_for_reply(">>>", 5)) {
		check_com();
		send_uart_pc("Try PostCommand.\n");
		retries++;
		send_msg(postCommand);

		WDT_A_clearTimer();
		if (retries > 1) {
			WDT_A_clearTimer();
			send_uart_pc("Uploadstored data failed POSTCOMMAND.\n");
			return false;
		}

	}

	retries = 0;

	WDT_A_clearTimer();
	send_msg(postBody);

	while (!wait_check_for_reply("#HTTP", 5)) {
		check_com();
		send_uart_pc("Try PostBody.\n");
		retries++;
		WDT_A_clearTimer();
		send_msg(postBody);

		WDT_A_clearTimer();
		if (retries > 1) {
			WDT_A_clearTimer();
			send_uart_pc("Uploadstored Data failed POSTBODY.\n");
			return false;
		}

	}
	retries = 0;

	WDT_A_clearTimer();

//	send_gsm_uart("AT+CFUN=0\r");
//	gsm_power_save_on();
	// reduce index by one
	decrease_received_message_index();
	WDT_A_clearTimer();
	return true;

}

void reset_gsm_module(void) {
	counter_read_gsm = 0;                     // Reset buffer counter
	send_gsm_uart("AT&F\r");              // Send Attention Command
	//	delay_gsm_respond(5000);                  // Delay a maximum of X seconds
	delay_ms(500);
	counter_read_gsm = 0;                     // Reset buffer counter
	init_gsm();
}
