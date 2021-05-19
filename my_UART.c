/*
 * my_UART.c
 *
 *  Created on: 09 Sep 2020
 *      Author: njnel
 */

#include <bme280.h>
#include <bme280_defs.h>
#include <helper.h>
#include <main.h>
#include <math.h>
#include <my_gpio.h>
#include <my_gps.h>
#include <my_rtc.h>
#include <my_RFM9x.h>
#include <my_scheduler.h>
#include <my_timer.h>
#include <MAX44009.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ti/devices/msp432p4xx/driverlib/gpio.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>
#include <ti/devices/msp432p4xx/driverlib/rom_map.h>
#include <ti/devices/msp432p4xx/driverlib/rtc_c.h>
#include <ti/devices/msp432p4xx/driverlib/uart.h>
#include <ti/devices/msp432p4xx/inc/msp432p401r.h>
#include "my_UART.h"

#define SIZE_BUFFER 80
#define SIZE_BUFFER_GPS 255
#define GPS_ON
#define GPS_OFF

uint8_t counter_read_pc = 0; //UART receive buffer counter
uint8_t counter_read_gps = 0;
bool UartActivityGps = false;
bool UartActivity = false;
char UartRX[SIZE_BUFFER]; //Uart receive buffer
char UartRxGPS[SIZE_BUFFER_GPS]; //Uart receive buffer
extern struct bme280_data bme280Data;
extern struct bme280_dev bme280Dev;
LocationData gpsData;

extern Gpio_t Led_rgb_blue;
extern RTC_C_Calendar currentTime;
extern bool setTimeFlag;
extern bool gpsWorking;

bool setRTCFlag = false;

extern RTC_C_Calendar currentTime;

// static functions
static void UartGPSCommands();

// http://software-dl.ti.com/msp430/msp430_public_sw/mcu/msp430/MSP430BaudRateConverter/index.html

//UART configured for 9600 Baud
const eUSCI_UART_ConfigV1 uartConfigGps = { EUSCI_A_UART_CLOCKSOURCE_SMCLK, // SMCLK Clock Source
		9,                                     // BRDIV = 9
		12,                                       // UCxBRF = 12
		34,                                       // UCxBRS = 34
		EUSCI_A_UART_NO_PARITY,                  // No Parity
		EUSCI_A_UART_LSB_FIRST,                  // LSB First
		EUSCI_A_UART_ONE_STOP_BIT,               // One stop bit
		EUSCI_A_UART_MODE,                       // UART mode
		EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION,  // Oversampling
		EUSCI_A_UART_8_BIT_LEN                  // 8 bit data length
		};

//57600
const eUSCI_UART_ConfigV1 uartConfig = { EUSCI_A_UART_CLOCKSOURCE_SMCLK, // SMCLK Clock Source
		1,                                     // BRDIV = 9
		10,                                       // UCxBRF = 12
		0,                                       // UCxBRS = 34
		EUSCI_A_UART_NO_PARITY,                  // No Parity
		EUSCI_A_UART_LSB_FIRST,                  // LSB First
		EUSCI_A_UART_ONE_STOP_BIT,               // One stop bit
		EUSCI_A_UART_MODE,                       // UART mode
		EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION,  // Oversampling
		EUSCI_A_UART_8_BIT_LEN                  // 8 bit data length
		};

void uart_init_gps() {

	gpsData.lat = 0.0;
	gpsData.lon = 0.0;
	/* Selecting P9.6 and P9.7 in UART mode */
	MAP_GPIO_setAsPeripheralModuleFunctionInputPin(
	UART_GPS_PORT,
	UART_GPS_PINS,
	GPIO_PRIMARY_MODULE_FUNCTION);
	MAP_UART_initModule(UART_GPS_MODULE, &uartConfigGps);
	MAP_UART_enableModule(UART_GPS_MODULE);

	// PPS signal
	MAP_GPIO_setAsInputPinWithPullDownResistor(GPS_PPS_PORT, GPS_PPS_PIN);
	MAP_GPIO_interruptEdgeSelect(GPS_PPS_PORT, GPS_PPS_PIN,
	GPIO_LOW_TO_HIGH_TRANSITION);
	MAP_GPIO_clearInterruptFlag(GPS_PPS_PORT, GPS_PPS_PIN);
	MAP_GPIO_enableInterrupt(GPS_PPS_PORT, GPS_PPS_PIN);
	MAP_Interrupt_enableInterrupt(GPS_PPS_INT_PORT);

	//wake pin
	MAP_GPIO_setAsOutputPin(GPS_WAKE_PORT, GPS_WAKE_PIN);
	MAP_GPIO_setOutputHighOnPin(GPS_WAKE_PORT, GPS_WAKE_PIN);

//	sendUARTgps("$PCAS10,3*1F\r\n"); // reset GPS module
	gps_disable_low_power();
	delay_ms(100);
	send_uart_gps(PMTK_SET_NMEA_OUTPUT_RMCONLY); // only enable GLL
//	send_uart_gps(PMTK_SET_NMEA_OUTPUT_OFF);
//	sendUARTgps("$PCAS03,0,0,0,0,0,0,9,0*0B\r\n"); // set to ZDA mode
	delay_ms(100);
	send_uart_gps(PMTK_API_SET_FIX_CTL_200mHZ);
	delay_ms(100);
	send_uart_gps(PMTK_SET_NMEA_UPDATE_1HZ);

	delay_ms(100);
//	gps_set_low_power();

	/* Enabling interrupts */
	MAP_UART_enableInterrupt(UART_GPS_MODULE, EUSCI_A_UART_RECEIVE_INTERRUPT);
	MAP_Interrupt_enableInterrupt(UART_GPS_INT);
}

void uart_init_pc() {
	/* Selecting P1.2 and P1.3 in UART mode */
	MAP_GPIO_setAsPeripheralModuleFunctionInputPin(UART_PC_PORT, UART_PC_PINS,
	GPIO_PRIMARY_MODULE_FUNCTION);
	MAP_UART_initModule(UART_PC_MODULE, &uartConfig);
	MAP_UART_enableModule(UART_PC_MODULE);

	/* Enabling interrupts */
	MAP_UART_enableInterrupt(UART_PC_MODULE, EUSCI_A_UART_RECEIVE_INTERRUPT);
	Interrupt_setPriority(UART_PC_INT, 0x40);
	MAP_Interrupt_enableInterrupt(UART_PC_INT);
}

void send_uart_pc(char *buffer) {
#if DEBUG
	int count = 0;
	while (strlen(buffer) > count) {
		MAP_UART_transmitData(UART_PC_MODULE, buffer[count++]);
	}
	uint32_t status = GPIO_getEnabledInterruptStatus(UART_PC_PORT);
	GPIO_clearInterruptFlag(UART_PC_PORT, status);
#endif
}

void send_uart_gps(char *buffer) {
	int count = 0;
	while (strlen(buffer) > count) {
		MAP_UART_transmitData(UART_GPS_MODULE, buffer[count++]);
	}
	uint32_t status = GPIO_getEnabledInterruptStatus(UART_GPS_PORT);
	GPIO_clearInterruptFlag(UART_GPS_PORT, status);
}

void send_uart_integer(uint32_t integer) {
	char buffer[10];
	sprintf(buffer, "_%2X_", integer);
	send_uart_pc(buffer);
}

void check_uart_activity() {
	if (return_uart_activity()) {
		uart_pc_command_handler();
	}
}

void uart_pc_command_handler() {
	counter_read_pc = 0;
	if (UartActivity) {
		UartActivity = false;
		switch (UartRX[0]) {
		case 'N':
			send_uart_pc("Setting Normal Mode\n");
			break;
		case 'D':
			send_uart_pc("Setting Debug Mode\n");
			break;
		case 'S': {
			const char s[2] = ",";
			char *token;
			token = strtok(UartRX, s); // UartRX[0]
			token = strtok(NULL, s);
			uint8_t len = atoi(token);
			token = strtok(NULL, s);
			SX1276Send((uint8_t*) token, len);
			send_uart_pc("Sending data on radio: ");
			send_uart_pc(token);
		}
			break;
		case 'T': {
			bme280_get_data(&bme280Dev, &bme280Data);
			char buffer[10];
			sprintf(buffer, "%.2f", bme280Data.temperature);
			send_uart_pc("Current temperature: ");
			send_uart_pc(buffer);
			send_uart_pc(" °C\n");
		}
			break;
		case 'P': {
			bme280_get_data(&bme280Dev, &bme280Data);
			char buffer[10];
			sprintf(buffer, "%.2f", bme280Data.pressure);
			send_uart_pc("Current pressure: ");
			send_uart_pc(buffer);
			send_uart_pc(" Pa\n");
		}
			break;
		case 'H': {
			bme280_get_data(&bme280Dev, &bme280Data);
			char buffer[10];
			sprintf(buffer, "%.2f", bme280Data.humidity);
			send_uart_pc("Current relative humidity: ");
			send_uart_pc(buffer);
			send_uart_pc(" %\n");
		}
			break;
		case 'L': {
			get_light();
			char buffer[10];
			sprintf(buffer, "%.2f", get_lux());
			send_uart_pc("Current light intensity: ");
			send_uart_pc(buffer);
			send_uart_pc(" lux\n");
		}
			break;
		case 'A': {
			helper_collect_sensor_data();
			send_uart_pc("All sensor data: ");
			char buffer[10];
			sprintf(buffer, "%.2f", get_lux());
			send_uart_pc("| L: ");
			send_uart_pc(buffer);
			sprintf(buffer, "%.2f", bme280Data.temperature);
			send_uart_pc("| T: ");
			send_uart_pc(buffer);
			sprintf(buffer, "%.2f", bme280Data.humidity);
			send_uart_pc("| H: ");
			send_uart_pc(buffer);
			sprintf(buffer, "%.2f", bme280Data.pressure);
			send_uart_pc("| P: ");
			send_uart_pc(buffer);
			send_uart_pc("|\n");
		}
			break;
		case 'C': { //set clock to zero
			RTC_C_Calendar zeroTime = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x2020 };
			RTC_C_holdClock();
			rtc_init(zeroTime);
			send_uart_pc("Time zeroed");
		}
			break;
		default:
			send_uart_pc("Unknown Command\n");
			break;
		}
		reset_uart_array();
	}
}

void send_uart_integer_next_line(uint32_t integer) {
	char buffer[10];
	sprintf(buffer, "_%2X_", integer);
	send_uart_pc(buffer);
	send_uart_pc("\n");
}

void reset_uart_array() {
	memset(UartRX, 0x00, SIZE_BUFFER);
	counter_read_pc = 0;
}

bool return_uart_activity() {
	return UartActivity;
}
bool gotGPSUartflag = false;
void UartGPSCommands() {

	if (UartActivityGps) {

//		SX1276Send((uint8_t*) UartRxGPS, counter_read_gps);	// Debugging, send uart data OTA to pc

		const char c[1] = ",";
		const char a[2] = "*";
//		"$GNRMC,093037.675,A,5606.1725,N,01404.0622,E,0.00,248.06,091120,0.00,W,E,N*5A\r\n"
//		char *dummyGPS =
//				"$GNRMC,093037.675,A,5606.1725,N,01404.0622,E,0.00,248.06,091120,0.00,W,E,N*5A\r\n";
		char *CMD;

//		memcpy(UartRxGPS, dummyGPS, strlen(dummyGPS));
		CMD = strtok(UartRxGPS, a);
		UartActivityGps = false;

		if (!memcmp(CMD, "$GNRMC", 6)) {
			if (strpbrk(CMD, "A")) {

				float deg = 0.0;
				float min = 0.0;
				CMD = strtok(UartRxGPS, c);
				CMD = strtok(NULL, c);

				//Time
				uint8_t hr;
				uint8_t minute;
				uint8_t sec;
				uint8_t day;
				uint8_t month;
				uint16_t year;

				char s[5];
				memset(s, 0, 5);
				sprintf(s, "%c%c", CMD[0], CMD[1]);
				hr = (uint8_t) strtol(s, NULL, 16);
				memset(s, 0, 5);
				sprintf(s, "%c%c", CMD[2], CMD[3]);
				minute = (uint8_t) strtol(s, NULL, 16);
				memset(s, 0, 5);
				sprintf(s, "%c%c", CMD[4], CMD[5]);
				sec = strtol(s, NULL, 16);

				CMD = strtok(NULL, c);
				CMD = strtok(NULL, c);

				// Location
				char tempDeg[2] = { 0 };
				tempDeg[0] = CMD[0];
				tempDeg[1] = CMD[1];
				deg = atoi(tempDeg);

				char tempMin[8] = { 0 };
				memcpy(tempMin, &CMD[2], 8);
				min = atof(tempMin);

				min = min / 60.0;
				deg += min;
				if (abs(deg) > 90) { // cannot be more than 90 maximum for latitude
					return;
				}
				gpsData.lat = deg;
				CMD = strtok(NULL, c);

				if (strpbrk(CMD, "N")) {

				} else if (strpbrk(CMD, "S")) {
					gpsData.lat *= -1;
				}

				CMD = strtok(NULL, c);

				char temp[3] = { 0 };
				memcpy(temp, CMD, 3);
				deg = 0;
				deg = atoi(temp);

				memset(tempMin, 0, 8);
				memcpy(tempMin, &CMD[3], 8);
				min = atof(tempMin);

				min = min / 60.0;
				deg += min;
				if (abs(deg) > 180) { // cannot be more than 180 maximum for longitude
					return;
				}
				gpsData.lon = deg;

				CMD = strtok(NULL, c);

				if (strpbrk(CMD, "E")) {

				} else if (strpbrk(CMD, "W")) {
					gpsData.lon *= -1;

				}

				CMD = strtok(NULL, c);
				// Speed over Ground "0.00"
				{
					memset(s, 0, 5);
					sprintf(s, "%c%c%c", CMD[0], CMD[1], CMD[2]);
					float sog = strtof(s, NULL);
				}
				CMD = strtok(NULL, c);
				// Course over Ground "000.00
				{
					memset(s, 0, 5);
					sprintf(s, "%c%c%c%c%c", CMD[0], CMD[1], CMD[2], CMD[3],
							CMD[4]);
					float cog = strtof(s, NULL);
				}
				CMD = strtok(NULL, c);
				// Date ddmmyy
				memset(s, 0, 5);
				sprintf(s, "%c%c", CMD[0], CMD[1]);
				day = (uint8_t) strtol(s, NULL, 16);
				memset(s, 0, 5);
				sprintf(s, "%c%c", CMD[2], CMD[3]);
				month = (uint8_t) strtol(s, NULL, 16);
				memset(s, 0, 5);
				sprintf(s, "%c%c", CMD[4], CMD[5]);
				year = 0x2000 + (uint8_t) strtol(s, NULL, 16);

//				rtc_set_calendar_time();
				if (sec != 0x59) {
					setRTCFlag = true;
					RTC_C_Calendar newCal = { (uint8_t) (sec + 0x01), minute,
							hr, 0, (uint_fast8_t) day, month, year };
					currentTime = newCal;

				}

//				if (convert_hex_to_dec_by_byte(currentTime.minutes) % 2 == 0) {
				if (sec == 0x30) {
					gotGPSUartflag = true;
				}
//				}
//				int slot = convert_hex_to_dec_by_byte(currentTime.minutes) * 60
//						+ convert_hex_to_dec_by_byte(currentTime.seconds);
//				set_slot_count(slot);

//				CMD = strtok(NULL, c);
//				// Magnetic Variation
//				CMD = strtok(NULL, c);
//				//Mode A
//				CMD = strtok(NULL, c);
//				// checksum

			}

		}
	}
//	memset(UartRxGPS, 0x00, SIZE_BUFFER_GPS);
	counter_read_gps = 0;
}

void EUSCIA3_IRQHandler(void) {
	uint32_t status = MAP_UART_getEnabledInterruptStatus(UART_GPS_MODULE);

	if (status & EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG) {
		UartRxGPS[counter_read_gps] = MAP_UART_receiveData(UART_GPS_MODULE);
#if DEBUG
		MAP_UART_transmitData(UART_PC_MODULE, UartRxGPS[counter_read_gps]);
#endif
		counter_read_gps++;
	}
	if (UartRxGPS[counter_read_gps - 1] == 0x0A
			&& UartRxGPS[counter_read_gps - 2] == 0x0D) {
		UartActivityGps = true;
		UartGPSCommands();
	}

	if (counter_read_gps == SIZE_BUFFER_GPS) {
		counter_read_gps = 0;
	}
}

void EUSCIA0_IRQHandler(void) {
	uint32_t status = MAP_UART_getEnabledInterruptStatus(UART_PC_MODULE);

	if (status & EUSCI_A_UART_RECEIVE_INTERRUPT) {

		UartRX[counter_read_pc] = MAP_UART_receiveData(UART_PC_MODULE);
//		MAP_UART_transmitData(EUSCI_A2_BASE, UartRX[counter_read_pc]);
		counter_read_pc++;
	}
	if (UartRX[counter_read_pc - 1] == 0x0A
			&& UartRX[counter_read_pc - 2] == 0x0D) {
		UartActivity = true;
		uart_pc_command_handler();
	}

	if (counter_read_pc == SIZE_BUFFER) {
		counter_read_pc = 0;
	}
}
