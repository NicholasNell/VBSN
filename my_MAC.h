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
	SYNC_MAC,
	MAC_SLEEP,
	NODE_DISC
} MACappState_t;

typedef enum {
	SYNC = 0, ACK, DATA, RTS, CTS
} MessageType_t;

typedef struct {
		uint8_t nodeID;	// Node ID to which this schedule belongs
		uint16_t sleepTime;	// Time until next wake period
} schedule_t;

schedule_t scheduleTable[MAX_NEIGHBOURS];

uint8_t neighbourTable[MAX_NEIGHBOURS];
uint16_t _RTSTime;	// the time this node will listen in ms
uint16_t _CTSTime;
uint16_t _dataTime;
uint16_t _syncTime;	// The time it takes to send a SYNC message in ms, node will use this to listen initially.
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
uint8_t txDataArray[MAX_MESSAGE_LEN];

void MacInit( );

void MACreadySend( uint8_t *dataToSend, uint8_t datalen );

bool MACStateMachine( );

bool MACSend( MessageType_t messageType, uint8_t *data, uint8_t len );

bool MACRx( uint32_t timeout );

#endif /* MY_MAC_H_ */
