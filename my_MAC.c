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


void MacInit( ) {
	do {
	uint32_t temp = SX1276Random();
	nodeID = (uint8_t) temp;
	} while (nodeID == 0xFF);
	_numNeighbours = 0;
	_sleepTime = 10000;
	scheduleSetup();



}

bool MACStateMachine( ) {
	return false;
}

void scheduleSetup( ) {
	uint8_t len = sizeof(datagram_t);
	mySchedule.listenTime = SX1276GetTimeOnAir(MODEM_LORA, 255);
	mySchedule.numNeighbours = _numNeighbours;
	mySchedule.sleepTime = _sleepTime;
	mySchedule.syncTime = SX1276GetTimeOnAir(MODEM_LORA, len);
}

bool MACSend( uint8_t *data, uint8_t len ) {
	_dataLen = len;
	createDatagram(data);
	datagramToArray();
	Radio.Send(TXBuffer, sizeof(myDatagram.header) + _dataLen);
	return true;
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
			return true;
		}
	}
	return false;
}
