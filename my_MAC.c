/*
 * my_MAC.c
 *
 *  Created on: 02 Apr 2020
 *      Author: Nicholas
 */
#include "my_MAC.h"
#include <string.h>

uint8_t seqid = 0;


bool sendALOHAmessage( uint8_t *buffer, uint8_t size ) {

	Radio.Send(buffer, 4);

	return true;

}

void sendAck( uint8_t buffer[] ) {

	buffer[0] = 1;
	Radio.Send(buffer, 4);

}

uint8_t crc8( const uint8_t *data, int len ) {
	unsigned int crc = 0;
	int j;
	int i;
	for (j = len; j; j--, data++) {
		crc ^= (*data << 8);
		for (i = 8; i; i--) {
			if (crc & 0x8000) {
				crc ^= (0x1070 << 3);
			}
			crc <<= 1;
		}
	}

	return (uint8_t) (crc >> 8);
}

bool MACSend( uint8_t *buffer, uint8_t size ) {
	return sendALOHAmessage(buffer, size);
}

bool MACStateMachine( ) {

	return true;
}
