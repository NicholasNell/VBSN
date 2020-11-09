/*
 * my_UART.c
 *
 *  Created on: 09 Sep 2020
 *      Author: njnel
 */

#include <bme280.h>
#include <bme280_defs.h>
#include <helper.h>
#include <math.h>
#include <my_gpio.h>
#include <my_gps.h>
#include <my_rtc.h>
#include <my_RFM9x.h>
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

// http://software-dl.ti.com/msp430/msp430_public_sw/mcu/msp430/MSP430BaudRateConverter/index.html

//UART configured for 9600 Baud
const eUSCI_UART_ConfigV1 uartConfig = { EUSCI_A_UART_CLOCKSOURCE_SMCLK, // SMCLK Clock Source
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

void UARTinitGPS() {
	GPS_ON
	gpsData.lat = 0.0;
	gpsData.lon = 0.0;
	/* Selecting P9.6 and P9.7 in UART mode */
	MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P9,
	GPIO_PIN6 | GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);
	MAP_UART_initModule(EUSCI_A3_BASE, &uartConfig);
	MAP_UART_enableModule(EUSCI_A3_BASE);
//	sendUARTgps("$PCAS10,3*1F\r\n"); // reset GPS module
	Delayms(1000);
//	sendUARTgps(PMTK_SET_NMEA_OUTPUT_GLLONLY); // only enable GLL
	sendUARTgps(PMTK_SET_NMEA_OUTPUT_RMCONLY);
//	sendUARTgps("$PCAS03,0,0,0,0,0,0,9,0*0B\r\n"); // set to ZDA mode
	Delayms(100);
	sendUARTgps(PMTK_STANDBY);
	/* Enabling interrupts */
	MAP_UART_enableInterrupt(EUSCI_A3_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
	MAP_Interrupt_enableInterrupt(INT_EUSCIA3);
}

void UARTinitPC() {
	/* Selecting P1.2 and P1.3 in UART mode */
	MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,
	GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);
	MAP_UART_initModule(EUSCI_A0_BASE, &uartConfig);
	MAP_UART_enableModule(EUSCI_A0_BASE);

	/* Enabling interrupts */
	MAP_UART_enableInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
	Interrupt_setPriority(INT_EUSCIA0, 0x40);
	MAP_Interrupt_enableInterrupt(INT_EUSCIA0);
}

void sendUARTpc(char *buffer) {
	int count = 0;
	while (strlen(buffer) > count) {
		MAP_UART_transmitData(EUSCI_A0_BASE, buffer[count++]);
	}
	uint32_t status = GPIO_getEnabledInterruptStatus(GPIO_PORT_P1);
	GPIO_clearInterruptFlag(GPIO_PORT_P1, status);
}

void sendUARTgps(char *buffer) {
	int count = 0;
	while (strlen(buffer) > count) {
		MAP_UART_transmitData(EUSCI_A3_BASE, buffer[count++]);
	}
	uint32_t status = GPIO_getEnabledInterruptStatus(GPIO_PORT_P9);
	GPIO_clearInterruptFlag(GPIO_PORT_P9, status);
}

void send_uart_integer(uint32_t integer) {
	char buffer[10];
	sprintf(buffer, "_%2X_", integer);
	sendUARTpc(buffer);
}

void checkUartActivity() {
	if (returnUartActivity()) {
		UartCommands();
	}
}

void UartCommands() {
	counter_read_pc = 0;
	if (UartActivity) {
		UartActivity = false;
		switch (UartRX[0]) {
		case 'N':
			sendUARTpc("Setting Normal Mode\n");
			break;
		case 'D':
			sendUARTpc("Setting Debug Mode\n");
			break;
		case 'S': {
			const char s[2] = ",";
			char *token;
			token = strtok(UartRX, s); // UartRX[0]
			token = strtok(NULL, s);
			uint8_t len = atoi(token);
			token = strtok(NULL, s);
			SX1276Send((uint8_t*) token, len);
			sendUARTpc("Sending data on radio: ");
			sendUARTpc(token);
		}
			break;
		case 'T': {
			bme280GetData(&bme280Dev, &bme280Data);
			char buffer[10];
			sprintf(buffer, "%.2f", bme280Data.temperature);
			sendUARTpc("Current temperature: ");
			sendUARTpc(buffer);
			sendUARTpc(" °C\n");
		}
			break;
		case 'P': {
			bme280GetData(&bme280Dev, &bme280Data);
			char buffer[10];
			sprintf(buffer, "%.2f", bme280Data.pressure);
			sendUARTpc("Current pressure: ");
			sendUARTpc(buffer);
			sendUARTpc(" Pa\n");
		}
			break;
		case 'H': {
			bme280GetData(&bme280Dev, &bme280Data);
			char buffer[10];
			sprintf(buffer, "%.2f", bme280Data.humidity);
			sendUARTpc("Current relative humidity: ");
			sendUARTpc(buffer);
			sendUARTpc(" %\n");
		}
			break;
		case 'L': {
			getLight();
			char buffer[10];
			sprintf(buffer, "%.2f", getLux());
			sendUARTpc("Current light intensity: ");
			sendUARTpc(buffer);
			sendUARTpc(" lux\n");
		}
			break;
		case 'A': {
			helper_collectSensorData();
			sendUARTpc("All sensor data: ");
			char buffer[10];
			sprintf(buffer, "%.2f", getLux());
			sendUARTpc("| L: ");
			sendUARTpc(buffer);
			sprintf(buffer, "%.2f", bme280Data.temperature);
			sendUARTpc("| T: ");
			sendUARTpc(buffer);
			sprintf(buffer, "%.2f", bme280Data.humidity);
			sendUARTpc("| H: ");
			sendUARTpc(buffer);
			sprintf(buffer, "%.2f", bme280Data.pressure);
			sendUARTpc("| P: ");
			sendUARTpc(buffer);
			sendUARTpc("|\n");
		}
			break;
		case 'C': { //set clock to zero
			RTC_C_Calendar zeroTime = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x2020 };
			RTC_C_holdClock();
			RtcInit(zeroTime);
			sendUARTpc("Time zeroed");
		}
			break;
		default:
			sendUARTpc("Unknown Command\n");
			break;
		}
		resetUARTArray();
	}
}

void send_uart_integer_nextLine(uint32_t integer) {
	char buffer[10];
	sprintf(buffer, "_%2X_", integer);
	sendUARTpc(buffer);
	sendUARTpc("\n");
}

void resetUARTArray() {
	memset(UartRX, 0x00, SIZE_BUFFER);
	counter_read_pc = 0;
}

bool returnUartActivity() {
	return UartActivity;
}

void UartGPSCommands() {
	if (UartActivityGps) {

		SX1276Send((uint8_t*) UartRxGPS, counter_read_gps);

		const char c[1] = ",";
		const char a[2] = "*";
//		"$GNRMC,093037.675,A,5606.1725,N,01404.0622,E,0.00,248.06,091120,0.00,W,E,N*5A\r\n"
//		char *dummyGPS =
//				"$GNRMC,093037.675,A,5606.1725,N,01404.0622,E,0.00,248.06,091120,0.00,W,E,N*5A\r\n";
//				"$GNGLL,3355.7142,S,01851.9869,E,085806.000,A,A*5A\r\n";
		char *CMD;

//		memcpy(UartRxGPS, dummyGPS, strlen(dummyGPS));
		CMD = strtok(UartRxGPS, a);
		UartActivityGps = false;

		if (!memcmp(CMD, "$GNRMC", 6)) {
//			$--GLL,ddmm.mm,a,dddmm.mm,a,hhmmss.ss,A,x*hh<CR><LF>
			if (strpbrk(CMD, "A")) {
				gpsWorking = true;
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
				uint16_t delayLeft = 0;
				memset(s, 0, 5);
				sprintf(s, "%c%c%c", CMD[7], CMD[8], CMD[9]);
				uint16_t msSec = strtol(s, NULL, 10);
				uint16_t msOver = 1000 - msSec;
				delayLeft = msOver;

				/* Time is Saturday, November 12th 1955 10:03:00 PM */
				//				 const RTC_C_Calendar currentTime =
				//				 {
				//				         0x00,
				//				         0x03,
				//				         0x22,
				//				         0x06,
				//				         0x12,
				//				         0x11,
				//				         0x1955
				//				 };
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

				const RTC_C_Calendar currentTime = { (uint8_t) sec, minute, hr,
						0, (uint_fast8_t) day, month, year };

				MAP_RTC_C_holdClock();
				RtcInit(currentTime);

				CMD = strtok(NULL, c);
				// Magnetic Variation
				CMD = strtok(NULL, c);
				//Mode A
				CMD = strtok(NULL, c);
				// checksum

				sendUARTgps(PMTK_STANDBY);
			}

		}
	}
	memset(UartRxGPS, 0x00, SIZE_BUFFER_GPS);
	counter_read_gps = 0;
}

void EUSCIA3_IRQHandler(void) {
	uint32_t status = MAP_UART_getEnabledInterruptStatus(EUSCI_A3_BASE);

	if (status & EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG) {
		UartRxGPS[counter_read_gps] = MAP_UART_receiveData(EUSCI_A3_BASE);
//		MAP_UART_transmitData(EUSCI_A0_BASE, UartRxGPS[counter_read_gps]);
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
	uint32_t status = MAP_UART_getEnabledInterruptStatus(EUSCI_A0_BASE);

	if (status & EUSCI_A_UART_RECEIVE_INTERRUPT) {

		UartRX[counter_read_pc] = MAP_UART_receiveData(EUSCI_A0_BASE);
//		MAP_UART_transmitData(EUSCI_A3_BASE, UartRX[counter_read_pc]);
		counter_read_pc++;
	}
	if (UartRX[counter_read_pc - 1] == 0x0A
			&& UartRX[counter_read_pc - 2] == 0x0D) {
		UartActivity = true;
		UartCommands();
	}

	if (counter_read_pc == SIZE_BUFFER) {
		counter_read_pc = 0;
	}
}
