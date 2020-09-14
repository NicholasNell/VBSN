/*
 * my_UART.c
 *
 *  Created on: 09 Sep 2020
 *      Author: njnel
 */

#include <bme280.h>
#include <bme280_defs.h>
#include <my_RFM9x.h>
#include <MAX44009.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ti/devices/msp432p4xx/driverlib/gpio.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>
#include <ti/devices/msp432p4xx/driverlib/rom_map.h>
#include <ti/devices/msp432p4xx/driverlib/uart.h>
#include <ti/devices/msp432p4xx/inc/msp432p401r.h>
#include "my_UART.h"

#define SIZE_BUFFER 80
#define SIZE_BUFFER_GPS 255

uint8_t counter_read_pc = 0; //UART receive buffer counter
uint8_t counter_read_gps = 0;
bool UartActivityGps = false;
bool UartActivity = false;
char UartRX[SIZE_BUFFER]; //Uart receive buffer
char UartRxGPS[SIZE_BUFFER_GPS]; //Uart receive buffer
extern struct bme280_data bme280Data;
extern struct bme280_dev bme280Dev;

extern float lux;

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
	/* Selecting P1.2 and P1.3 in UART mode */
	MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P9,
	GPIO_PIN6 | GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);
	MAP_UART_initModule(EUSCI_A3_BASE, &uartConfig);
	MAP_UART_enableModule(EUSCI_A3_BASE);

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
	// write_string_toSD(buffer);
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
			getLight(&lux);
			char buffer[10];
			sprintf(buffer, "%.2f", lux);
			sendUARTpc("Current light intensity: ");
			sendUARTpc(buffer);
			sendUARTpc(" lux\n");
		}
			break;
		case 'A': {
			getLight(&lux);
			bme280GetData(&bme280Dev, &bme280Data);
			sendUARTpc("All sensor data: ");
			char buffer[10];
			sprintf(buffer, "%.2f", lux);
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
	/*const char s[2] = ",";
	 char *token;
	 token = strtok(UartRX, s); // UartRX[0]
	 token = strtok(NULL, s);
	 uint8_t len = atoi(token);
	 token = strtok(NULL, s);
	 SX1276Send((uint8_t*) token, len);
	 sendUARTpc("Sending data on radio: ");
	 sendUARTpc(token);
	 */

	if (UartActivityGps) {
		const char s[2] = ",";
		char *token;
		token = strtok(UartRxGPS, s);
		UartActivityGps = false;

		if (!memcmp(token, "$GNGGA", 6)) {
//			SX1276Send((uint8_t*) UartRxGPS, counter_read_gps);
		} else if (false) {

		} else {

		}
		memset(UartRxGPS, 0x00, SIZE_BUFFER_GPS);
		counter_read_gps = 0;
	}
}

void EUSCIA3_IRQHandler(void) {
	uint32_t status = MAP_UART_getEnabledInterruptStatus(EUSCI_A3_BASE);

	if (status & EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG) {
		UartRxGPS[counter_read_gps] = MAP_UART_receiveData(EUSCI_A3_BASE);
		counter_read_gps++;
	}
	if (UartRxGPS[counter_read_gps - 1] == 0x0A
			&& UartRxGPS[counter_read_gps - 2] == 0x0D) {
		UartActivityGps = true;
	}

	if (counter_read_gps == SIZE_BUFFER_GPS) {
		counter_read_gps = 0;
	}

}

void EUSCIA0_IRQHandler(void) {
	uint32_t status = MAP_UART_getEnabledInterruptStatus(EUSCI_A0_BASE);

	if (status & EUSCI_A_UART_RECEIVE_INTERRUPT) {

		UartRX[counter_read_pc] = MAP_UART_receiveData(EUSCI_A0_BASE);
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
