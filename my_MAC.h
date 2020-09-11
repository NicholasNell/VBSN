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
#define DEFAULT_RX_TIME 1000
#define SLEEP_TIME 60 * 2 * 1000
#define MAX_NEIGHBOURS 255

typedef enum {
	TX = 0, RX, TXDONE, RXDONE, RXTIMEOUT, TXTIMEOUT, RXERROR, RADIO_SLEEP
} MACRadioState_t;

typedef enum {
	SYNC_MAC, MAC_SLEEP, NODE_DISC, MAC_RTS, MAC_CTS, MAC_DATA
} MACappState_t;

typedef enum {
	SYNC = 0, ACK, DATA, RTS, CTS
} MessageType_t;

typedef struct {
	uint8_t nodeID;	// Node ID to which this schedule belongs
	uint32_t sleepTime;	// Time until next wake period
} schedule_t;

void MacInit();

void MACreadySend(uint8_t *dataToSend, uint8_t datalen);

bool MACStateMachine();

bool MACSend(MessageType_t messageType, uint8_t *data, uint8_t len);

bool MACRx(uint32_t timeout);

#endif /* MY_MAC_H_ */
