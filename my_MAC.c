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
#include "my_systick.h"
#include "my_gpio.h"


volatile bool hasData = false;
bool schedule_setup = false;
volatile schedule_t mySchedule;
extern datagram_t myDatagram;
volatile MACappState_t MACState = NODE_DISC;
uint8_t emptyArray[0];
extern SX1276_t SX1276;
bool sleepFlag = false;
bool syncFlag = false;
bool RTSFlag = false;
bool CTSFlag = false;
bool DataFlag = false;

extern Gpio_t Led_rgb_green;
extern Gpio_t Led_rgb_blue;
extern Gpio_t Led_rgb_red;

static void scheduleSetup( );
/*!
 * \brief Return true if message is successfully processed. Returns false if not.
 */
static bool processRXBuffer( );

static void syncSchedule( );
static bool RxStateMachine( );
static bool TxStateMachine( );

void MacInit( ) {
	do {
		uint8_t temp = SX1276Random8Bit();
		_nodeID = temp;
		_ranNum = (uint16_t) (temp * 125);
	} while (_nodeID == 0xFF);
	_numNeighbours = 0;
	_sleepTime = _ranNum;
}

void MACreadySend( uint8_t *dataToSend, uint8_t dataLen ) {
	hasData = true;
	_dataLen = dataLen;
	memcpy(txDataArray, dataToSend, _dataLen);
}

bool MACStateMachine( ) {

		return RxStateMachine();

}

static void scheduleSetup( ) {
	mySchedule.nodeID = _nodeID;
	mySchedule.sleepTime = _sleepTime;
}

bool MACSend( MessageType_t messageType, uint8_t *data, uint8_t len ) {
	_dataLen = len;
	createDatagram(messageType, data);
	datagramToArray();
//	startLoRaTimer(5000);
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
//			if (processRXBuffer())
//				return true;
//			else return false;
			processRXBuffer();
			return true;
		}
	}
}

static bool processRXBuffer( ) {
	if (ArrayToDatagram()) {
		switch (rxdatagram.header.messageType) {
		 case SYNC: {
//				syncSchedule();
				bool containsNode = false;
				uint8_t i = 0;
				for (i = 0; i < _numNeighbours; i++) {
					if (rxdatagram.header.source == neighbourTable[i]) {
						containsNode = true;
						break;
					}
				}
				if (!containsNode) {
					neighbourTable[_numNeighbours++] = rxdatagram.header.source;
				}
		 }
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
		return true;
	}
	else return false;
}

static void syncSchedule( ) {
	mySchedule.sleepTime = rxdatagram.header.nextWake;
	_sleepTime = mySchedule.sleepTime;
	mySchedule.nodeID = _nodeID;
	startTimerAcounter(mySchedule.sleepTime, &sleepFlag);
}

static bool RxStateMachine( ) {

		switch (MACState) {
			case NODE_DISC: {
				bool rxSyncMsg = MACRx(_sleepTime);
				if (rxSyncMsg) {
					GpioWrite(&Led_rgb_blue, 1);
					GpioWrite(&Led_rgb_green, 0);
				GpioWrite(&Led_rgb_red, 0);
					syncSchedule();
				MACSend(SYNC, emptyArray, 00);
					MACState = MAC_SLEEP;
				SX1276SetSleep();
				}
				else {
					scheduleSetup();
				startTimerAcounter(mySchedule.sleepTime, &sleepFlag);
				if (MACSend(SYNC, emptyArray, 00)) {
						GpioWrite(&Led_rgb_green, 1);
						GpioWrite(&Led_rgb_blue, 0);
					GpioWrite(&Led_rgb_red, 0);
					MACRx(100);
						MACState = MAC_SLEEP;
					SX1276SetSleep();
					}
					else {
						MACState = NODE_DISC;
					}
				}
			}
				break;
			case MAC_SLEEP:
				if (sleepFlag) {
					sleepFlag = false;
				GpioWrite(&Led_rgb_blue, 0);
				GpioWrite(&Led_rgb_green, 0);
				GpioWrite(&Led_rgb_red, 1);
				startTimerAcounter(mySchedule.sleepTime, &sleepFlag);
				MACState = SYNC_MAC;
				}
				break;
		case SYNC_MAC:
			if (hasData) {
				if (MACSend(SYNC, emptyArray, 0)) {
					MACState = MAC_RTS;
				}
				else {
					MACState = MAC_SLEEP;
				}
			}
			else {
				if (MACRx(1000)) {
					MACState = MAC_RTS;
				}
				else {
					MACState = MAC_SLEEP;
				}
			}
			break;
		case MAC_RTS:
			if (hasData) {
				if (MACSend(RTS, emptyArray, 0)) {
					MACState = MAC_CTS;
				}
				else {
					MACState = MAC_SLEEP;
				}
			}
			else {
				if (MACRx(1000)) {
					MACState = MAC_CTS;
				}
				else {
					MACState = MAC_SLEEP;
				}
			}
			break;
		case MAC_CTS:
			if (hasData) {
				if (MACRx(1000)) {
					MACState = MAC_DATA;
				}
				else {
					MACState = MAC_SLEEP;
				}
			}
			else {
				if (MACSend(CTS, emptyArray, 0)) {
					MACState = MAC_DATA;
				}
				else {
					MACState = MAC_SLEEP;
				}
			}
			break;
		case MAC_DATA:
			if (hasData) {
				if (MACSend(DATA, txDataArray, 5)) {
					MACState = MAC_SLEEP;
					hasData = false;
				}
				else {
					MACState = MAC_SLEEP;
				}
			}
			else {
				if (MACRx(1000)) {
					MACState = MAC_SLEEP;
				}
				else {
					MACState = MAC_SLEEP;
				}
			}
			break;
		}

	return true;
}

static bool TxStateMachine( ) {
	return true;

}
