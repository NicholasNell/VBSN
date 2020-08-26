/*
 * datagram.h
 *
 *  Created on: 19 Aug 2020
 *      Author: nicholas
 */

#ifndef DATAGRAM_H_
#define DATAGRAM_H_

#include <stdbool.h>
#include <stdint.h>
#include "my_MAC.h"

#define RH_MAX_MESSAGE_LEN 255

typedef struct {
		uint8_t dest;
		uint8_t source;
		MessageType_t messageType;
		schedule_t thisSchedule;
		uint8_t len;
} header_t;

typedef struct {
		header_t header;
		uint8_t *data;
} datagram_t;

datagram_t rxdatagram;

bool datagramInit( );
void createDatagram( MessageType_t messageType, uint8_t *data );
void datagramToArray( );
bool ArrayToDatagram( );


#endif /* DATAGRAM_H_ */
