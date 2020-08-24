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
#include <string.h>
#include "my_gpio.h"
#include "my_spi.h"
#include "radio.h"
#include "my_timer.h"
#include "my_RFM9x.h"
#include "sx1276Regs-Fsk.h"
#include "sx1276Regs-LoRa.h"
#include <stdio.h>


#define BROADCAST_ADDRESS 0xFF
#define MAX_LEN 255

typedef enum {
	TX = 0, RX, TXDONE, RXDONE, RXTIMEOUT, TXTIMEOUT, RXERROR
} MACState_t;

typedef enum {
	SYNC = 0, ACK, DATA, RTS, CTS
} MessageType_t;

typedef struct {
		uint16_t sleepTime;
		uint16_t listenTime;
		uint8_t numNeighbours;
		uint16_t syncTime;
} schedule_t;

typedef struct {
		uint8_t neighbourID;
		schedule_t scheduleTable[];
} scheduleTable_t;

volatile uint8_t nodeID;

volatile uint8_t _dataLen;
volatile uint8_t _numNeighbours;
volatile uint16_t _sleepTime;

volatile schedule_t mySchedule;

volatile MACState_t MACState;



void MacInit( );

bool MACStateMachine( );

bool MACSend( uint8_t *data, uint8_t len );

bool MACRx( );

#endif /* MY_MAC_H_ */
