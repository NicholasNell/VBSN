/*
 * my_MAC.c
 *
 *  Created on: 02 Apr 2020
 *      Author: Nicholas
 */
#include "my_MAC.h"
#include "datagram.h"
#include "my_timer.h"


void scheduleSetup( );
void processRXBuffer( );


void MacInit( ) {
	do {
	uint32_t temp = SX1276Random();
		_nodeID = (uint8_t) temp;
	} while (_nodeID == 0xFF);
	_numNeighbours = 0;
	_sleepTime = 10000;
	scheduleSetup();



}

bool MACStateMachine( ) {
	return false;
}

void scheduleSetup( ) {
	uint8_t len = sizeof(datagram_t);
	mySchedule.nodeID = _nodeID;
	mySchedule.listenTime = SX1276GetTimeOnAir(MODEM_LORA, 255);
	mySchedule.numNeighbours = _numNeighbours;
	mySchedule.sleepTime = _sleepTime;
	mySchedule.syncTime = SX1276GetTimeOnAir(MODEM_LORA, len);
}

bool MACSend( uint8_t *data, uint8_t len ) {
	_dataLen = len;
	createDatagram(data);
	datagramToArray();
	startLoRaTimer(5000);
	Radio.Send(TXBuffer, sizeof(myDatagram.header) + _dataLen);
	MACState = TX;
	while (true) {
		if (MACState == TXDONE) {
			return true;
		}
		else if (MACState == TXTIMEOUT) {
			return false;
		}
	}
}

bool MACRx( ) {
	Radio.Rx(0);
	MACState = RX;
	startLoRaTimer(5000);
	while (true) {
		if (MACState == RXTIMEOUT) {
			return false;
		}
		else if (MACState == RXERROR) {
			return false;
		}
		else if (MACState == RXDONE) {
			processRXBuffer();
			return true;
		}
	}
}

void processRXBuffer( ) {
	ArrayToDatagram();
//	datagram_t receivedDatagram;
//	receivedDatagram.header.source = RXBuffer[0];
//	receivedDatagram.header.dest = RXBuffer[1];
//	receivedDatagram.header.messageType = (MessageType_t) RXBuffer[2];
//	receivedDatagram.header.thisSchedule.nodeID = RXBuffer[3];
//	receivedDatagram.header.thisSchedule.sleepTime = (uint16_t) RXBuffer[4]
//			+ (uint16_t) RXBuffer[5] << 8;
//	__no_operation();

}
