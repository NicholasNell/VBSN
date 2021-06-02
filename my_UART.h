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

void uart_init_gps();
void uart_init_pc();
void send_uart_pc(char *buffer);
void send_uart_gps(char *buffer);
void send_uart_integer(uint32_t integer);
void send_uart_integer_next_line(uint32_t integer);
void reset_uart_array();
bool return_uart_activity();
void uart_pc_command_handler();
void check_uart_activity();
bool get_gps_uart_flag();
void reset_gps_uart_flag();

#endif /* MY_UART_H_ */
