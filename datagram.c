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

header_t createHeader( MessageType_t messageType );

bool datagramInit( ) {
	return false;
}

void createDatagram( MessageType_t messageType, uint8_t *data ) {
	myDatagram.header = createHeader(messageType);
	myDatagram.data = data;
}

header_t createHeader( MessageType_t messageType ) {
	header_t header;
	header.source = _nodeID;
	if (messageType == SYNC) {
	header.dest = BROADCAST_ADDRESS;
	}
	else {
		header.dest = _destID;
	}
	header.messageType = messageType;
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

// returns true if received message is not corrupt by checking node ID's against each other.
// Also checks to see if the message was meant for this node.
bool ArrayToDatagram( ) {
	uint8_t len = sizeof(myDatagram.header);
	memcpy(&rxdatagram.header, RXBuffer, len);
	rxdatagram.data = tempDataArray;
	memcpy(tempDataArray, RXBuffer + len, rxdatagram.header.len);

	if ((rxdatagram.header.dest == _nodeID
			|| rxdatagram.header.dest == BROADCAST_ADDRESS)
			&& (rxdatagram.header.source
					== rxdatagram.header.thisSchedule.nodeID)) {
		return true;
	}
	else {
		return false;
	}
}


