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

void UARTinitGPS();
void UARTinitPC();
void sendUARTpc(char *buffer);
void send_uart_integer(uint32_t integer);
void send_uart_integer_nextLine(uint32_t integer);
void resetUARTArray();
bool returnUartActivity();
void UartCommands();
void checkUartActivity();
#endif /* MY_UART_H_ */
