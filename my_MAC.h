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
		uint8_t nodeID;	// Node ID to which this schedule belongs
		uint16_t sleepTime;	// The time this node will sleep in ms
		uint16_t listenTime;	// the time this node will listen in ms
		uint8_t numNeighbours;// The number of neighbours this node has in nodes
		uint16_t syncTime;// The time it takes to send a SYNC message in ms, node will use this to listen initially.
} schedule_t;

typedef struct {
		uint8_t numNeighbours;
		schedule_t scheduleTable[];
} scheduleTable_t;


volatile uint8_t _nodeID;

volatile uint8_t _dataLen;
volatile uint8_t _numNeighbours;
volatile uint16_t _sleepTime;

volatile schedule_t mySchedule;

volatile MACState_t MACState;
extern uint8_t RXBuffer[255];

void MacInit( );

bool MACStateMachine( );

bool MACSend( uint8_t *data, uint8_t len );

bool MACRx( );

#endif /* MY_MAC_H_ */
