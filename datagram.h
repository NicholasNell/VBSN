/*
 * datagram.h
 *
 *  Created on: 19 Aug 2020
 *      Author: nicholas
 */

#ifndef DATAGRAM_H_
#define DATAGRAM_H_

#include <my_gps.h>
#include <stdint.h>
#include <ti/devices/msp432p4xx/driverlib/rtc_c.h>

#define RH_MAX_MESSAGE_LEN 255

// msgType Flags
#define MSG_NONE	0b00000000
#define MSG_RTS 	0b00000001
#define MSG_CTS 	0b00000010
#define MSG_DATA 	0b00000100
#define MSG_ACK 	0b00001000
#define MSG_SYNC 	0b00010000

typedef struct {
	uint8_t netDest;
	uint8_t netFlags;
	uint8_t netHops;
} NetHeader_t;

typedef struct {
	uint16_t txSlot; 	// txSlot
	uint16_t schedID; 	// Schedule ID
} Schedule_t;

typedef struct {
	uint8_t dest; // Where the message needs to go (MAC LAYER, not final destination);
	uint8_t source;	// Where the message came from, not original source
	uint16_t msgID;	// Msg ID. Unique message number.
	Schedule_t sched; // syncslot
	uint8_t flags;	// message type:
					//	RTS:	0x1
					// 	CTS:	0x2
					// 	SYNC:	0x3
					// 	Data:	0x4
					// 	Ack:	0x5
					// 	Sync:  	0x6
} MacHeader_t;

typedef struct {
	LocationData gpsData;
	float temp;
	float hum;
	float press;
	float lux;
	RTC_C_Calendar tim;
	float soilMoisture;
} MsgData_t;

typedef struct {
	MacHeader_t macHeader;
	MsgData_t data;
} Datagram_t;

#endif /* DATAGRAM_H_ */
