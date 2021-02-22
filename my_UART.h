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

// UART PINS
#define UART_PC_PORT GPIO_PORT_P1
#define UART_PC_PINS GPIO_PIN2 | GPIO_PIN3
#define UART_PC_MODULE EUSCI_A0_BASE
#define UART_PC_INT INT_EUSCIA0

void UARTinitGPS( );
void UARTinitPC( );
void sendUARTpc( char *buffer );
void sendUARTgps( char *buffer );
void send_uart_integer( uint32_t integer );
void send_uart_integer_nextLine( uint32_t integer );
void resetUARTArray( );
bool returnUartActivity( );
void UartPCCommandHandler( );
void checkUartActivity( );
#endif /* MY_UART_H_ */
