/*
 * my_UART.c
 *
 *  Created on: 09 Sep 2020
 *      Author: njnel
 */

#include <my_RFM9x.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include "my_UART.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define SIZE_BUFFER 80

uint8_t counter_read = 0; //UART receive buffer counter
bool UartActivity = false;
char UartRX[SIZE_BUFFER]; //Uart receive buffer

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
	counter_read = 0;
	if (returnUartActivity()) {
		UartActivity = false;
		switch (UartRX[0]) {
		case 'N':

			sendUARTpc("Setting Normal Mode\n");
			break;
		case 'T':

			sendUARTpc("Setting Debug Mode\n");
			break;
		case 'S': {
//			uint8_t len = 1;
//			len = UartRX[2] - '0';
//			uint8_t data[255];
//			memcpy(data, &UartRX[4], len);
//			SX1276Send(data, len);
//			sendUARTpc("Sending data: \"");
//			sendUARTpc((char*) data);
//			sendUARTpc("\" on radio.\n");
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
	counter_read = 0;
}

bool returnUartActivity() {
	return UartActivity;
}

void EUSCIA3_IRQHandler(void) {
	uint32_t status = MAP_UART_getEnabledInterruptStatus(EUSCI_A3_BASE);

	if (status & EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG) {

	}

}

void EUSCIA0_IRQHandler(void) {
	uint32_t status = MAP_UART_getEnabledInterruptStatus(EUSCI_A0_BASE);

	if (status & EUSCI_A_UART_RECEIVE_INTERRUPT) {

		UartRX[counter_read] = MAP_UART_receiveData(EUSCI_A0_BASE);
		counter_read++;
	}
	if (UartRX[counter_read - 1] == 0x0A && UartRX[counter_read - 2] == 0x0D)
		UartActivity = true;

	if (counter_read == 80) {
		counter_read = 0;
	}

}
