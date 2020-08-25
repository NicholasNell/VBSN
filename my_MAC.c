/*
 * my_MAC.c
 *
 *  Created on: 02 Apr 2020
 *      Author: Nicholas
 */
#include "my_MAC.h"
#include "datagram.h"
#include "my_timer.h"
#include "my_RFM9x.h"

bool schedule_setup = false;
volatile schedule_t mySchedule;
extern datagram_t myDatagram;
volatile MACappState_t MACState = SYNCRX;




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
	do {
		_ranNum = SX1276Random();
	} while (_ranNum > 32000 || _ranNum < 10000);
}

bool MACStateMachine( ) {
	switch (MACState) {
		case INIT_SETUP: {
			if (MACRx(_ranNum)) {

			}
			else {

			}
		}
			break;
		case SYNCRX: {
			bool rxSuccess = false;
			if (schedule_setup) {
				rxSuccess = MACRx(mySchedule.syncTime);
			}
			else {
				rxSuccess = MACRx(DEFAULT_SYNC_TIME);
			}

			if (rxSuccess) {

			}
		}
			break;
		case SYNCTX:
			break;
		case SLEEP:
			break;
		case LISTEN:
			break;
		case SCHEDULE_SETUP:
			break;
		case SCHEDULE_ADOPT:
			break;
		default:
			break;
	}
	return true;
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
	RadioState = TX;
	while (true) {
		if (RadioState == TXDONE) {
			return true;
		}
		else if (RadioState == TXTIMEOUT) {
			return false;
		}
	}
}

bool MACRx( uint32_t timeout ) {
	Radio.Rx(0);
	RadioState = RX;
	startLoRaTimer(timeout);
	while (true) {
		if (RadioState == RXTIMEOUT) {
			return false;
		}
		else if (RadioState == RXERROR) {
			return false;
		}
		else if (RadioState == RXDONE) {
			processRXBuffer();
			return true;
		}
	}
}

void processRXBuffer( ) {
	ArrayToDatagram();
	switch (rxdatagram.header.messageType) {
		case SYNC:
		{
			bool contains = false;
			uint8_t i = 0;

			for (i = 0; i < MAX_NEIGHBOURS; i++) {
				if (rxdatagram.header.thisSchedule.nodeID
						== scheduleTable[i].nodeID) {
					contains = true;

					break;
				}
			}
			if (!contains) {
				_numNeighbours += 1;
				scheduleTable[_numNeighbours] = rxdatagram.header.thisSchedule;
			}
		}
			break;
		case ACK:
			break;
		case DATA:
			break;
		case RTS:
			break;
		case CTS:
			break;
		default:
			break;
	}
}
