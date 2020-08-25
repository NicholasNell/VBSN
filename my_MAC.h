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



#define BROADCAST_ADDRESS 0xFF
#define MAX_LEN 255
#define DEFAULT_SYNC_TIME 70
#define DEFAULT_RX_TIME 1000
#define DEFAULT_SLEEP_TIME 20000

typedef enum {
	TX = 0, RX, TXDONE, RXDONE, RXTIMEOUT, TXTIMEOUT, RXERROR
} MACRadioState_t;

typedef enum {
	SYNCRX, SLEEP, LISTEN, SYNCTX, SCHEDULE_SETUP, SCHEDULE_ADOPT, INIT_SETUP
} MACappState_t;

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

schedule_t scheduleTable[255];

uint32_t _ranNum;
volatile uint8_t _nodeID;
volatile uint8_t _dataLen;
volatile uint8_t _numNeighbours;
volatile uint16_t _sleepTime;
extern uint8_t RXBuffer[255];
volatile MACRadioState_t RadioState;
extern uint8_t TXBuffer[255];

void MacInit( );

bool MACStateMachine( );

bool MACSend( uint8_t *data, uint8_t len );

bool MACRx( uint32_t timeout );

#endif /* MY_MAC_H_ */
