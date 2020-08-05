/*
 * my_MAC.h
 *
 *  Created on: 02 Apr 2020
 *      Author: Nicholas
 */

#ifndef MY_MAC_H_
#define MY_MAC_H_

#include <stdint.h>
#include <stdbool.h>
#include "my_gpio.h"
#include "my_spi.h"
#include "radio.h"
#include "my_timer.h"
#include "my_RFM9x.h"
#include "sx1276Regs-Fsk.h"
#include "sx1276Regs-LoRa.h"
#include <stdio.h>

typedef enum {
	SLEEP = 0, PENDING, RETRANSMIT, EXPIRED, ACK_RESP
} AlohaState_t;

typedef enum {
	LOWPOWER = 0, IDLE,

	RX, RX_TIMEOUT, RX_ERROR,

	TX, TX_TIMEOUT,

	CAD, CAD_DONE
} AppStates_t;

#define ALOHA_MAX_ATTEMPT 5

typedef struct {
		uint8_t fid;
		uint8_t no;
} HeaderStruct;

typedef struct {
		uint8_t pd0;
		uint8_t pd1;
} DataStruct;

void createAlohaPacket(
		uint8_t *output,
		HeaderStruct *header,
		DataStruct *data );

bool dissectAlohaPacket(
		uint8_t *input,
		HeaderStruct *header,
		DataStruct *data );


bool sendALOHAmessage( uint8_t *buffer, uint8_t size );

void sendAck( int id );

bool MACSend( uint8_t *buffer, uint8_t size );

bool MACStateMachine( );

#endif /* MY_MAC_H_ */
