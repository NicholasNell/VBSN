/*
 * datagram.h
 *
 *  Created on: 19 Aug 2020
 *      Author: nicholas
 */

#ifndef DATAGRAM_H_
#define DATAGRAM_H_

#include "genericDriver.h"
#include "my_MAC.h"
#include <stdbool.h>
#include <stdint.h>

#define RH_MAX_MESSAGE_LEN 255

typedef struct {
		uint8_t dest;
		uint8_t source;
		MessageType_t ID;
		schedule_t thisSchedule;
		uint8_t len;
		uint8_t hops;
} header_t;

typedef struct {
		header_t header;
		uint8_t *data;
} datagram_t;


bool datagramInit( );


#endif /* DATAGRAM_H_ */
