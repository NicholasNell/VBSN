/*
 * datagram.c
 *
 *  Created on: 19 Aug 2020
 *      Author: nicholas
 */


#include "datagram.h"

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
	datagram_t *rxdatagram = 0;

	uint8_t len = sizeof(myDatagram.header);
//	memcpy(rxdatagram, RXBuffer, len);
//	memcpy(rxdatagram + len, RXBuffer, rxdatagram->header.len);

}


