/*
 * datagram.h
 *
 *  Created on: 19 Aug 2020
 *      Author: nicholas
 */

#ifndef DATAGRAM_H_
#define DATAGRAM_H_

#include <my_MAC.h>
#include <stdbool.h>
#include <stdint.h>

extern uint8_t TXBuffer[255];

#define RH_MAX_MESSAGE_LEN 255

typedef struct {
		uint8_t dest;
		uint8_t source;
		MessageType_t messageType;
		schedule_t thisSchedule;
		uint8_t len;
		uint8_t hops;
} header_t;

typedef struct {
		header_t header;
		uint8_t *data;
} datagram_t;

datagram_t myDatagram;


bool datagramInit( );
void createDatagram( uint8_t *data );
void datagramToArray( );


#endif /* DATAGRAM_H_ */
