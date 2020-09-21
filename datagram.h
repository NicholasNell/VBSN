/*
 * datagram.h
 *
 *  Created on: 19 Aug 2020
 *      Author: nicholas
 */

#ifndef DATAGRAM_H_
#define DATAGRAM_H_

#include <my_gps.h>
#include <stdbool.h>
#include <stdint.h>
#include <ti/devices/msp432p4xx/driverlib/rtc_c.h>
#include "my_MAC.h"

#define RH_MAX_MESSAGE_LEN 255

typedef struct {
	uint8_t dest; // Where the message needs to go (MAC LAYER, not final destination);
	uint8_t source;	// Where the message came from, not original source
	uint16_t msgID;	// Msg ID. Unique message number.
	uint8_t flags;	// message type:
					//	RTS:	0x1
					// 	CTS:	0x2
					// 	SYNC:	0x3
					// 	Data:	0x4
					// 	Ack:	0x5
					// 	Disc:  	0x6
} Header_t;

typedef struct {
	uint8_t min; // Minute of the hour this node starts its cycle (i.e: min=1; will sleep until 20 and send a data packet on 21;)
} syncPacket_t;

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
	Header_t header;
	MsgData_t data;
} Datagram_t;

bool datagramInit();
void createDatagram();
void datagramToArray();
bool ArrayToDatagram();

#endif /* DATAGRAM_H_ */
