/*
 * datagram.c
 *
 *  Created on: 19 Aug 2020
 *      Author: nicholas
 */


#include "datagram.h"
#include <stdbool.h>
#include <stdint.h>
#include <my_MAC.h>

datagram_t myDatagram;
uint8_t tempDataArray[];
extern schedule_t mySchedule;
extern uint8_t TXBuffer[255];

header_t createHeader( );

bool datagramInit( ) {
	return false;
}

void createDatagram( uint8_t *data ) {
	myDatagram.header = createHeader();
	myDatagram.data = data;
}

header_t createHeader( ) {
	header_t header;
	header.source = _nodeID;
	header.dest = BROADCAST_ADDRESS;
	header.messageType = SYNC;
	header.thisSchedule = mySchedule;
	header.len = _dataLen;
	header.hops = 0;
	return header;
}

void datagramToArray( ) {
	uint8_t len = sizeof(myDatagram.header);
	memcpy(TXBuffer, &myDatagram.header, len);
	memcpy(TXBuffer + len, myDatagram.data, _dataLen);
}

void ArrayToDatagram( ) {
	uint8_t len = sizeof(myDatagram.header);
	memcpy(&rxdatagram.header, RXBuffer, len);
	rxdatagram.data = tempDataArray;
	memcpy(tempDataArray, RXBuffer + len, rxdatagram.header.len);
}


