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
#define MAX_MESSAGE_LEN 255
#define DEFAULT_SYNC_TIME 70
#define DEFAULT_RX_TIME 1000
#define DEFAULT_SLEEP_TIME 20000
#define MAX_NEIGHBOURS 255

typedef enum {
	TX = 0, RX, TXDONE, RXDONE, RXTIMEOUT, TXTIMEOUT, RXERROR, RADIO_SLEEP
} MACRadioState_t;

typedef enum {
	SYNCRX,
	SLEEP,
	LISTEN_RTS,
	SEND_CTS,
	SYNCTX,
	SCHEDULE_SETUP,
	SCHEDULE_ADOPT
} MACappState_t;

typedef enum {
	SYNC = 0, ACK, DATA, RTS, CTS
} MessageType_t;

typedef struct {
		uint8_t nodeID;	// Node ID to which this schedule belongs
		uint16_t sleepTime;	// The time this node will sleep in ms
		uint16_t RTSTime;	// the time this node will listen in ms
		uint16_t CTSTime;
		uint16_t dataTime;
		uint8_t numNeighbours;// The number of neighbours this node has in nodes
		uint16_t syncTime;// The time it takes to send a SYNC message in ms, node will use this to listen initially.
} schedule_t;

schedule_t scheduleTable[MAX_NEIGHBOURS];

uint32_t _ranNum;
volatile uint8_t _nodeID;
volatile uint8_t _dataLen;
volatile uint8_t _numNeighbours;
volatile uint16_t _sleepTime;
volatile uint8_t _destID;
uint32_t _totalScheduleTime;
volatile uint32_t _scheduleTimeLeft;
extern uint8_t RXBuffer[MAX_MESSAGE_LEN];
volatile MACRadioState_t RadioState;
extern uint8_t TXBuffer[MAX_MESSAGE_LEN];

void MacInit( );

bool MACStateMachine( );

bool MACSend( MessageType_t messageType, uint8_t *data, uint8_t len );

bool MACRx( uint32_t timeout );

#endif /* MY_MAC_H_ */
